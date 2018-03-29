#ifndef PLUGIN_HPP
#define PLUGIN_HPP



/*!

  \defgroup triage Basic plugin commands
  @{

  Each plugin can usually take OSC commands to define messages to send when events occur.

  OSC commands
  ------------

### `/defevent <event|key> <port> <message-address> <message-arguments> ...`

Associate a message (message-address and message-arguments) to an event (or key).
The message is sent to the specific port of this plugin.
event are negative integers, keys are actual keyboard symbol (use uppercase).

### `/defevent-ctrl <event|key> <port> <message-address> <message-arguments> ...`

...

Exemple 1
---------

Change the framerate of all cameras when we press 'a' on the keyboard
~~~
/defevent 'A' 5 /set "*" AcquisitionFrameRateAbs 24.0
~~~

Special symbols
-------------

You can insert special symbols int the message-arguments. They will be replaced by a value when the
message is sent. The values associated with these symbols depends on the plugin which provides them.

- $key : actual key which was pressed
- $now : current time
- $x, $y : x or y corrdinate of the mouse
- $xr, $yr : x or y coordinate of the mouse (relative, from 0 to 1)

@}
 */



// on doit inclure imgu2 parce qu'on utilise les messages
#include <imgv/imgv.hpp>

//#include "imgv::rqueue.hpp"
//#include "recyclable.hpp"

#include <pthread.h>
#include <signal.h>
#include <unistd.h> // pour le sleep

// pour le dump
#include <iostream>
#include <fstream>

#include <typeinfo>

// pout changer le nom du thread
//#include <sys/prctl.h>

#include <oscpack/osc/OscTypes.h>

using namespace std;
using namespace osc;


//
// plugin typique...
//
// - une queue d'entree (possiblement une queue de recyclage si "source")
// - une queue de sortie (ou sinon on recycle -> "sink")
//
// T doit etre un derive de "recyclable"
//
// normalement, les "ports", ou on attache les queue, ont des fonctions definies par alias
//

template <class T>
class plugin
        : public messageDecoder
{
public:
    string name; // pour debug, stats, etc.. this is set by qpmanager, typically
    string typeName; // not important. Only for output in dot files. Class name as string

protected:
    // on dirait que Q cache le Q du requeue... etrange...
    vector<imgv::rqueue<recyclable>*> Qs; // table des queues...
    bool started; // true: start() or init() was called.
    // when started is true, it is too late to initialize a plugin.

    // mapping des noms de port (set in constructor, used in port
    std::map<std::string,int> ports;

    // direction of ports. Set this to true for IN ports.
    std::map<std::string,bool> portsIN;

    // PAS NECESSAIRE!
    // list of possible events
    //std::map<std::string,bool> events;


    //
    // pour les log...
    //
private:
    int logPort; // -1 = no log. Try to set by using port["log"]
public:

    //
    // pour aider a gere l'envoi de messages vers l'exterieur
    //
    imgv::rqueue<recyclable> Qmessage; // queue speciale pour nos propres messages
    class binding {
    public:
        std::string key; // this is typically "A","B", ... for keyboard, "mouse", and other events...
        std::string portName; // usually we specify a name, from wich we get the port.
        int port; // -1 -> unkown. Defined from portName
        bool control; // should the message be sent out to control or image queue?
        // this is decided with /defevent or /defevent-ctrl

        // un message complet... element 0 doit etre une string (c'est le msg)
        int nb; // nb d'arguments (et taille des tableaux
        char *typetag; // contient le type de chaque item: s,i,d, ...
        int  *argint;  // contient les element de type 'i'
        double *argdouble; // elemets de type 'd'
        float *argfloat; // elemets de type 'f'
        string *argstring; // elements de type 's'
        string *argsymbol; // elements de type 'S'

    private:
        void alloc(int n) {
            nb=n;
            typetag=new char[nb];
            argint=new int[nb];
            argdouble=new double[nb];
            argfloat=new float[nb];
            argstring=new string[nb];
            argsymbol=new string[nb];
        }
    public:

        /*
                   void dump(ofstream &file,plugin<T> *p) {
                   file << "P"<<p->name<<":"<<port<< "("<<portName << ") -> ";
                   if( p->badpos(port) ) file << "???" <<endl;
                   else file << "Q"<<p->Qs[port]->name << endl;
                   }
                 */

        /**
                  binding(string key,int port,string msg) : key(key), port(port) {
                //cout << "binding "<<key<<" "<<msg<<endl;
                alloc(1);
                typetag[0]='s';
                argstring[0]=msg.c_str();
                control=true;
                portname=string("");
                }
                 **/
        binding(const osc::ReceivedMessage &m) {
            //cout << "binding message "<<m<<endl;
            // format du message:
            // /defevent[-ctrl] [key] [int port] [/toto] arg0 arg1 ...
            // /defevent[-ctrl] [key] [string portName] [/toto] arg0 arg1 ...
            //
            // /defevent-ctrl pour la queue de controle
            // /defevent pour la queue d'images
            // on veut extraire key et port, et conserver /toto
            const char *addr=m.AddressPattern();
            if( oscutils::endsWith(addr,"/defevent") ) control=false;
            else if( oscutils::endsWith(addr,"/defevent-ctrl") ) control=true;
            else{ /* error! not the right message! */ }

            osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
            if( arg->IsChar() ) key=string(1,arg->AsChar()); else key=arg->AsString();
            arg++;
            portName=(arg++)->AsString();
            port=-1; // unkown for now
            cout << "BINDING key is "<<key<<", portname is "<<portName<<" control="<<control<<endl;
            // copy le message qui suit...
            alloc(m.ArgumentCount()-2); // on ne compte pas le key et port

            //cout << "nb args is "<<nb<<endl;
            for(int i=0;i<nb;i++,arg++) {
                //cout << "item "<<i<<" has type "<<arg->TypeTag()<<endl;
                typetag[i]=arg->TypeTag();
                switch( typetag[i] ) {
                case 'i': argint[i]=arg->AsInt32();break;
                case 'd': argdouble[i]=arg->AsDouble();break;
                case 'f': argfloat[i]=arg->AsFloat();break;
                case 's': argstring[i]=arg->AsString();break;
                case 'S': argsymbol[i]=arg->AsSymbol();break;
                }
            }
        }
        binding(const binding &x) {
            key=x.key;
            port=x.port;
            portName=x.portName;
            control=x.control;
            alloc(x.nb);
            for(int i=0;i<nb;i++) {
                typetag[i]=x.typetag[i];
                argint[i]=x.argint[i];
                argdouble[i]=x.argdouble[i];
                argfloat[i]=x.argfloat[i];
                argstring[i]=x.argstring[i];
                argsymbol[i]=x.argsymbol[i];
            }
        }
        ~binding() {
            delete[] typetag;
            delete[] argint;
            delete[] argdouble;
            delete[] argfloat;
            delete[] argstring;
            delete[] argsymbol;
        }
        void composeMessage(message *m,double now,int x,int y,int w,int h,const char *wname) {
            *m << osc::BeginMessage(argstring[0].c_str());
            for(int i=1;i<nb;i++) {
                switch( typetag[i] ) {
                case 'i': *m << argint[i];break;
                case 'd': *m << argdouble[i];break;
                case 'f': *m << argfloat[i];break;
                case 's': *m << argstring[i];break;
                case 'S':
                    // check pour $key, $now, $x et $y
                    if( argsymbol[i]=="$key" ) *m << key;
                    else if( argsymbol[i]=="$now" ) *m << now;
                    else if( argsymbol[i]=="$x" ) *m << x;
                    else if( argsymbol[i]=="$y" ) *m << y;
                    else if( argsymbol[i]=="$xr" ) *m << (float)x/w;
                    else if( argsymbol[i]=="$yr" ) *m << (float)y/h;
                    else if( argsymbol[i]=="$window" ) *m << ((wname==NULL)?"":wname);
                    else *m << osc::Symbol(argsymbol[i].c_str());
                    break;
                case 'T': *m << true;break;
                case 'F': *m << false;break;
                }
            }
            *m << osc::EndMessage;
        }
        std::string toString() {
            std::ostringstream s;
            s << argstring[0];
            for(int i=1;i<nb;i++) {
                s << " ";
                switch( typetag[i] ) {
                case 'i': s << argint[i];break;
                case 'd': s << argdouble[i];break;
                case 'f': s << argfloat[i];break;
                case 's': s << argstring[i];break;
                case 'S': s << argsymbol[i];break;
                case 'T': s << true;break;
                case 'F': s << false;break;
                }
            }
            return s.str();
        }
    };


	// on va utiliser des *
	// mais attention, si le plugin est detruit, on doit
	// librere les bindings du triage...
    vector<binding*> triage;

    //
    // logging
    //
    class mystream: public std::ostringstream
    {
    public:
        plugin<T> &p;
        mystream(plugin<T> &pp) :std::ostringstream() , p(pp) { }
    };



public:
    pthread_t tid; // thread id. 0 = not running
    int running;
    bool kill; // set to true to stop this plugin at the next loop()

    //
    // logging (use log << ...cout style stuff << [info|warn|err];
    //
    mystream log;

    plugin() : Qmessage("msg") , log(*this) { running=0;kill=false; Qs.reserve(10); started=false;logPort=-1;cout <<"plugin create"<<endl;}
    virtual ~plugin() {
	if( running!=0 ) stop();
	for(int i=0;i<triage.size();i++) {
		cout << "deleting triage "<<i<<endl;
		delete triage[i];
	}
	cout << "delete plugin" << endl;
    }


    // retourne le # de port associe a un nom (-1 si probleme)
    virtual int portMap(std::string portName) {
        map<std::string,int>::const_iterator itr;
        itr=ports.find(portName);
        if( itr==ports.end() ) return -1;
        return itr->second;
    }


    //
    // retourne le # de port anonyme le plus petit.
    // c'est 1+Max[ports avec des noms]
    //
private:
    int portMapMax() {
        map<std::string,int>::const_iterator itr;
        int max=-1;
        for(itr=ports.begin();itr!=ports.end();itr++) {
            if(itr->second >max ) max=itr->second;
        }
        return max;
    }
    std::string portMap(int portNum) {
        map<std::string,int>::const_iterator itr;
        for(itr=ports.begin();itr!=ports.end();itr++) {
            if(itr->second==portNum ) return(itr->first);
        }
        return "";
    }

private:
    // set the Q for a specific NUMERIC port.
    // deprecated... do not use unless you are SURE of the port number
    virtual void setQ(int port,imgv::rqueue<recyclable> *q) {
        if( port<0 ) return;
        // this port SHOULD have a name
        if( portMap(port)=="" ) {
            log << "setQ port "<<port<<" has no name!"<<err;
            return;
        }
        if( Qs.size()<=port ) {
            int s=Qs.size();
            Qs.resize(port+1,NULL); // ajuste la taille du vector
            for(int i=s;i<port;i++) Qs[i]=NULL; // to be safe
        }
        Qs[port]=q;
    }
public:
    // set the Q for a specific NAMED port.
    // best method to set ports.
    virtual void setQ(std::string portName,imgv::rqueue<recyclable> *q) {
        setQ(portMap(portName),q);
    }


    // set the Q of an anonymous port.
    // an anonymous port is allocate above the named ports.
    // it can be reused if another anomymous port points to the same Q
    // the selected port is returned.
    // once assigned, an anonymous port can never be reassigned.
    // the name of an anonymous port is always "#8" for port number 8.
    // we could return the string for the name instead of the int...
    /*
           virtual int setQ(imgv::rqueue<recyclable> *q) {
           int pos;
        // create anonymous port!!! start at the max+1 of all named ports
        int pmin=portMapMax()+1;
        for(pos=pmin;pos<Qs.size();pos++) {
        if( Qs[pos]!=NULL && Qs[pos]==q ) return(pos); // reuse!
        }
        // need a new port.. see if something is available.
        for(pos=pmin;pos<Qs.size();pos++) {
        if( Qs[pos]==NULL ) break; // free spot!
        }
        string name=string("#")+NumberToString<int>(pos);
        ports[name]=pos;
        setQ(pos,q);
        return(pos);
        }
         */

    // reserve a new named port (plus we can specify a bloc of ports)
    // return the num of the first port of the group
    // if nb is >1, then each port after the first
    // will be named with "-0" to "-5" appended (if nb=6)
    // ATTENTION: if nb=1, the name will be name-0, (use reservePorts(name) otherwise)
    virtual int reservePorts(std::string portName,int nb,bool in=false) {
        // is this name already in use?
        int i;
        int pos=portMapMax()+1;
        // more ports! check all names
        for(i=0;i<nb;i++) {
            string name=portName+string("-")+NumberToString(i);
            if( portMap(name)>=0 ) {
                log << "reservePorts: port "<<name<<" already defined."<<err;
                return(-1);
            }
        }
        // all is good. resserve ports.
        for(i=0;i<nb;i++) {
            string name=portName+string("-")+NumberToString(i);
            ports[name]=pos+i;
            // this is just for dot output... otherwise never used.
            if( in ) portsIN[name]=true;
        }
        return(pos);
    }

    // reserve a new named port. return the port number, or -1 if problem
    virtual int reservePort(std::string portName,bool in) {
        // is this name already in use?
        int i;
        int pos=portMapMax()+1;
        if( portMap(portName)>=0 ) {
            log << "reservePorts: port "<<portName<<" already defined."<<err;
            return(-1);
        }
        ports[portName]=pos;
        // this is just for dot output... otherwise never used.
        if( in ) portsIN[portName]=true;
        return(pos);
    }

    virtual int reservePort(const char *portNameChar,bool in) {
        std::string portName(portNameChar);
        reservePort(portName,in);
    }

    // reserve a new anonymous port
    // return the name of the port.
    // if nb is >1, then each port after the first
    // will be named with "-0" to "-5" appended (if nb=6)
    virtual string reservePort(bool in) {
        int pos=portMapMax()+1;
        string name=string("#")+NumberToString<int>(pos);
        if( portMap(name)>=0 ) {
            log << "reservePorts: port "<<name<<" already defined. THIS IS NOT NORMAL."<<err;
            return("");
        }
        ports[name]=pos;
        // this is just for dot output... otherwise never used.
        if( in ) portsIN[name]=true;
        return(name);
    }


    // trouve une queue avec un certain nom, retourne NULL si pas trouve
    imgv::rqueue<recyclable> *findQ(string name) {
        for(int pos=0;pos<Qs.size();pos++) {
            if( Qs[pos]!=NULL && Qs[pos]->name==name ) return Qs[pos];
        }
        return NULL;
    }

    // retourne un message tout frais de la queue de message
    message *getMessage(int size) {
        message *m=dynamic_cast<message*>(Qmessage.pop_front());
        if( m==NULL ) {
            // message can't wait... create a new message.
            m=new message(size);
            m->setRecycleQueue(&Qmessage);
        }
        m->reserve(size);
        return(m);
    }

    // une queue est disponible?
    inline bool badpos(int pos) { return ( pos<0 || pos>=Qs.size() || Qs[pos]==NULL ); }

    //
    // recycle un item T. remet le pointeur a NULL
    //
    inline void recycle(T *&b) { recyclable::recycle((recyclable **)&b); }

    void push_front(int pos,recyclable **item) {
        if( !badpos(pos) ) Qs[pos]->push_front(item);
        else recyclable::recycle((recyclable **)item);
    }
    void push_front(int pos,T **item) {
        if( !badpos(pos) ) Qs[pos]->push_front((recyclable **)item);
        else recyclable::recycle((recyclable **)item);
    }
    void push_front(int pos,message **item) {
        if( !badpos(pos) ) Qs[pos]->push_front((recyclable **)item);
        else recyclable::recycle((recyclable **)item);
    }

    void push_back(int pos,recyclable **item) {
        if( !badpos(pos) ) Qs[pos]->push_back(item);
        else recyclable::recycle((recyclable **)item);
    }
    void push_back(int pos,T **item) {
        if( !badpos(pos) ) Qs[pos]->push_back((recyclable **)item);
        else recyclable::recycle((recyclable **)item);
    }
    void push_back(int pos,message **item) {
        if( !badpos(pos) ) Qs[pos]->push_back((recyclable **)item);
        else recyclable::recycle((recyclable **)item);
    }

    // pour deposer dans une queue de controle... push_back uniquement.
    void push_back_ctrl(int pos,recyclable **item) {
        if( !badpos(pos) ) Qs[pos]->push_back_ctrl(item);
        else recyclable::recycle((recyclable **)item);
    }
    void push_back_ctrl(int pos,T **item) {
        if( !badpos(pos) ) Qs[pos]->push_back_ctrl((recyclable **)item);
        else recyclable::recycle((recyclable **)item);
    }
    void push_back_ctrl(int pos,message **item) {
        if( !badpos(pos) ) Qs[pos]->push_back_ctrl((recyclable **)item);
        else recyclable::recycle((recyclable **)item);
    }


    // get message from control queue only.
    // retourne nb de message lus (>=0)
    // nom suggere: process_all_control_messages
    int pop_front_ctrl_nowait(int pos) {
        if( badpos(pos) ) return 0;
        int nb=0;
        for(;;) {
            recyclable *p=Qs[pos]->pop_front_ctrl(); // control only!
            if( p==NULL ) break;
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) recyclable::recycle(&p); // on recycle les images!
            else{ localhandler(&p);nb++; }
        }
        return(nb);
    }

    // get message from control queue only.
    // retourne the message itself without calling handlers
    message *pop_front_ctrl_nowait_nohandler(int pos) {
        if( badpos(pos) ) return NULL;
        recyclable *p=Qs[pos]->pop_front_ctrl(); // control only!
        if( p==NULL ) return NULL;
        message *m=dynamic_cast<message*>(p);
        if( m==NULL ) recyclable::recycle(&p); // on recycle les non-messages!
        return(m);
    }
    // get a message or an image. m and image are NULL if queue empty
    void pop_front_nowait_nohandler(int pos,message *&m,T *&image) {
        m=NULL;
        image=NULL;
        if( badpos(pos) ) return; // this is ok!
        recyclable *p=Qs[pos]->pop_front(); // get next from Q or Qctrl
        if( p==NULL ) return; // nothing for now? we are done.
        m=dynamic_cast<message*>(p);
        image=dynamic_cast<T*>(p);
    }
    // get a message or an image. Wait to get something.
    void pop_front_wait_nohandler(int pos,message *&m,T *&image) {
        m=NULL;
        image=NULL;
        if( badpos(pos) ) return; // this is ok!
        recyclable *p=Qs[pos]->pop_front_wait(); // get next from Q or Qctrl
        if( p==NULL ) return; // not possible.
        m=dynamic_cast<message*>(p);
        image=dynamic_cast<T*>(p);
    }



    // handle tous les message (et attend au moins un message)
    // recycle immediatement une image, si on tombe dessus
    // retourne le nb de message lus (>=1)
    int pop_front_ctrl_wait(int pos) {
        if( badpos(pos) ) return 0;
        int nb;
        recyclable *p;
        // on attend au moins un message avant d'arreter
        for(nb=0;;nb++) {
            if( nb==0 )      p=Qs[pos]->pop_front_ctrl_wait();
            else p=Qs[pos]->pop_front_ctrl();
            if( p==NULL ) break; // plus rien a lire!
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) recyclable::recycle(&p); // on recycle les images!
            else localhandler(&p); // process message
        }
        return(nb);
    }

    // handle tous les message (et attend au moins un message)
    // recycle immediatement une image, si on tombe dessus
    message *pop_front_ctrl_wait_nohandler(int pos) {
        if( badpos(pos) ) return NULL;
        recyclable *p=Qs[pos]->pop_front_ctrl_wait(); // control only!
        if( p==NULL ) return NULL; // not possible
        message *m=dynamic_cast<message*>(p);
        if( m==NULL ) recyclable::recycle(&p); // on recycle les non-messages!
        return(m);
    }


    // pop une seule image, process tous les messages disponibles
    T *pop_front_wait(int pos) {
        if( badpos(pos) ) { cout << "["<<name<<"!!!] cant wait for undefined Queue "<<pos<<endl;return NULL; }
        for(;;) {
            recyclable *p=Qs[pos]->pop_front_wait(); // prochain message Q ou Qctrl
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) return(q); // une image! on a fini.
            localhandler(&p); // process le message et retourne attendre
        }
    }

    // pop une seule image, NE process PAS les messages.
    T *pop_front_noctrl_wait(int pos) {
        if( badpos(pos) ) { cout << "["<<name<<"!!!] cant wait for undefined Queue "<<pos<<endl;return NULL; }
        for(;;) {
            recyclable *p=Qs[pos]->pop_front_noctrl_wait(); // prochain message dans Q
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) return(q); // une image! on a fini.
            // a message is put back into the control Q for later processing.
            Qs[pos]->push_back_ctrl(p);
        }
    }



    T *pop_back_wait(int pos) {
        if( badpos(pos) ) { cout << "["<<name<<"!!!] cant wait for undefined Queue "<<pos<<endl;return NULL; }
        for(;;) {
            recyclable *p=Qs[pos]->pop_back_wait(); // get next item from Q or Qctrl.
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) return(q); // une image! on a fini.
            localhandler(&p); // process le message.
        }
    }
    T *pop_front_nowait(int pos) {
        if( badpos(pos) ) return NULL; // this is ok!
        for(;;) {
            recyclable *p=Qs[pos]->pop_front(); // get next from Q or Qctrl
            if( p==NULL ) return NULL; // nothing for now? we are done.
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) return(q); // an image. we are done.
            localhandler(&p); // handle message and retry (process all messages in Q)
        }
    }
    T *pop_back_nowait(int pos) {
        if( badpos(pos) ) return NULL; // this is ok!
        for(;;) {
            recyclable *p=Qs[pos]->pop_back(); // from Q or Qctrl
            if( p==NULL ) return NULL; // queues empty. we are done.
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) return(q); // image. we are done.
            localhandler(&p); // process msg and retry
        }
    }
    // DO NOT process messsages
    T *pop_front_noctrl_nowait(int pos) {
        if( badpos(pos) ) return NULL; // this is ok!
        for(;;) {
            recyclable *p=Qs[pos]->pop_front_noctrl(); // get next from Q only.
            if( p==NULL ) return NULL; // nothing for now? we are done.
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) return(q); // an image. we are done.
            // a message is put back into the control Q for later processing.
            // notice the push is always to the back of Qctrl
            Qs[pos]->push_back_ctrl(p);
        }
    }
    // DO NOT process messsages
    T *pop_back_noctrl_nowait(int pos) {
        if( badpos(pos) ) return NULL; // this is ok!
        for(;;) {
            recyclable *p=Qs[pos]->pop_back(); // from Q or Qctrl
            if( p==NULL ) return NULL; // queues empty. we are done.
            T *q=dynamic_cast<T*>(p);
            if( q!=NULL ) return(q); // image. we are done.
            // a message is put back into the control Q for later processing.
            // notice the push is always to the back of Qctrl
            Qs[pos]->push_back_ctrl(p);
        }
    }

    // handler de type different de T, mais avec une base recyclable
    //virtual void handler(recyclable **r) =0;
    inline void localhandler(recyclable **r) {
        if( typeid(**r)==typeid(message) ) {
            message *m=dynamic_cast<message*>(*r);
            m->decode(*this); // messageDecoder, not initialisation
        }else handler(**r);
        // un message est toujours recyclé...
        recyclable::recycle(r); // pas le bon type! on recycle!
    }

    virtual void handler(recyclable &r) { cout << "<msg?>"<<endl; } // ne fait rien...
    //
    // provient de messageDecoder
    //
    bool decode(const osc::ReceivedMessage &m) {
        //cout << "def event :"<<m<<endl;
        // check the standard /defevent
        const char *addr=m.AddressPattern();
        // check if the message contains a reply port: .../!xyz/...
        // osc::util ...
        if( oscutils::endsWith(addr,"/defevent")
                || oscutils::endsWith(addr,"/defevent-ctrl") ) {
            defOutgoingMessage(m);
            return true;
        }
        return false; // did not process the message
    }



    //
    // Log
    //
    // *this << "bonjour" << endl;   // DO same as info
    // *this << info << "bonjour" << "more" << "evene more" << 123 << 123.45 << endl;
    // *this << warn << "bonjour" << endl;
    // *this << err << "bonjour" << endl;
    //
    // endl is used to send the message.
    //
    // a message will be sent to Log.in, with this form:
    // /info <plugin name> "Bonjour" other args...
    //
public:

    // true=error!!!
    bool start() {
        if( running!=0 ) return true; // already started!
        //
        // start the plugin thread
        //
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        //size_t stacksize;
        //pthread_attr_getstacksize(&attr, &stacksize );
        //cout << "** PLUGIN ** Stacksize = "<<stacksize<<endl;
        //stacksize*=2;
        //pthread_attr_setstacksize(&attr,stacksize);
        //pthread_attr_getstacksize(&attr, &stacksize );
        //cout << "** PLUGIN ** Stacksize = "<<stacksize<<endl;
        cout << "**PLUGIN** name="<<name<<" START"<<endl;
        int k=pthread_create( &tid, &attr,(void* (*)(void*))(*mainthread), this);
        running=1;
        return false;
    }

    // called from main thread. Cancel current thread.
    void stop() {
        if (running)
        {
            cout << "plugin canceling " << name<< endl;
            pthread_cancel(tid);
            cout << "plugin joining " << name << endl;
            pthread_join(tid,NULL);
            cout << "plugin stopped " << name << endl;
        }
        running=0; // ready to restart!
    }

    // retourne true si le thread est encore en vie
    bool status() {
        if( running==0 ) return false;
        if(pthread_kill(tid,0)==0) { return true; }
        return false;
    }

    // protect some code from cancel. remember to call cancelEnable!!!
    void cancelDisable() {
        int oldstate;
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    }
    void cancelEnable() {
        int oldstate;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
        pthread_testcancel();
    }
    // it seems usleep must be protected from cancel to avoid segfaults...
    void usleepSafe(useconds_t usec) {
        int oldstate;
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
        usleep(usec);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
        pthread_testcancel();
    }

    //
    // init, go and uninit are called inside the thread.
    //

    virtual void init() {
        //cout << "[plugin] -- init --" << pthread_self()<< endl;
    }
    virtual void uninit() {
        //cout << "[plugin] -- UNinit --" << pthread_self()<<endl;
    }
    virtual bool loop() {
        // do something
        usleepSafe(500000);
        return true; // ok! on continue!
    }


    //
    // triage (gestion des messages a envoyer)
    //
    void defOutgoingMessage(const osc::ReceivedMessage &m) {
        //cout << "defoutgoing : got " << m<<endl;
        triage.push_back(new binding(m));
	cout << "triage is now size "<<triage.size()<<endl;
        for(int i=0;i<triage.size();i++) {
		cout << "triage "<<i<<" is key "<<triage[i]->key<<endl;
	}
    }

	// check for key and key-window as event names (when wname is not NULL)
    void checkOutgoingMessages(std::string key,double now,int x,int y,int w=1,int h=1,const char *wname=NULL) {
	//if( wname!=NULL ) log << "event "<<key<<" for window "<<wname<<info;
        // we should check if the event is in events[]...
	//cout << "checking "<<key<<" with triage size "<<triage.size()<<endl;
	std::string key2;
	if( wname!=NULL ) key2=key+"-"+wname;
        for(int i=0;i<triage.size();i++) {
            //log << "triage checkin "<<triage[i].key<<" = "<<key<<" : "<< " :: portname="<<triage[i].portName << info;
            //cout << "triage checkin "<<triage[i]->key<<" = "<<key<<" : "<< " :: portname="<<triage[i]->portName << info;
            if( triage[i]->key!=key && (wname!=NULL && triage[i]->key!=key2) ) continue;
            message *m=getMessage(256);
            triage[i]->composeMessage(m,now,x,y,w,h,wname); // key is matching
            // envoie le message a bon port!
            // message est en avant de la queue (prioritaire)

            // pourquoi faire decode ici????? ne sert a rien!!!!
            //m->decode();

            // solve the port, if unknown
            if( triage[i]->port<0 ) triage[i]->port=portMap(triage[i]->portName);
            //cout << "triage sending message to port name "<<triage[i].portName<<" #="<<triage[i].port<<endl;
            if( triage[i]->control ) {
                // from /defevent-ctrl, this is an important/urgent message
                //log << "sent to ctrl, port is "<<triage[i].port<<" -> Q name "<< info;
                push_back_ctrl(triage[i]->port,&m);
            }else{
                // from /defevent, this is a message that will be in the image queue
                //log << "sent to normal, port is "<<triage[i].port<<" -> Q name "<< info;
                push_back(triage[i]->port,&m);
            }
        }
    }


    //
    // output as graphviz
    //
    /*
           static void dumpBegin(ofstream &file,string fname) {
           file.open(fname.c_str());
           file << "digraph G {\n";
           file << "     graph [rankdir=\"LR\"];\n";
           }
           virtual void dump(ofstream &file) {
        // dump le triage
        for(int i=0;i<triage.size();i++) triage[i].dump(file,this);
        }
        static void dumpEnd(ofstream &file) {
        file << "}\n";
        file.close();
        }
         */
    //
    // the plugin node
    // can be redefined, but this is a standard node output.
    //
    // we assume left-right alignement
    //
    virtual void dumpDot(ofstream &file) {
        file << "P"<<name << " [shape=none label=<<table border=\"1\">\n";
        // plugin name, and other useful info
        file << "<tr><td bgcolor=\"#44ff44\">"<<name << "</td></tr>\n";
        file << "<tr><td bgcolor=\"#88ff88\">("<<typeName<<")</td></tr>\n";
        // dump all ports
        map<std::string,int>::const_iterator itr;
        for(itr=ports.begin();itr!=ports.end();itr++) {
            file << "<tr><td port=\"" << itr->second << "\">";
            file << "<table border=\"0\"><tr><td bgcolor=\"#aaaaaa\" colspan=\"2\">";
            file << itr->first << " ("<<itr->second<<")</td></tr>\n";
            // if this portname is in triage, output the event
            for(int i=0;i<triage.size();i++) {
                //cout << "plugin "<<name <<  "triage "<<i<<" is "<<triage[i].portName<<" : "<<itr->first << " : " << triage[i].key<<endl;
                if( triage[i]->portName==itr->first ) {
                    //cout << "  added "<<triage[i].key<< " to "<<itr->first<<endl;
                    file << "<tr><td>" << triage[i]->key<< "</td><td align=\"left\">" << triage[i]->toString()<<"</td></tr>\n";
                }
            }
            file << "</table></td></tr>";
        }
        // all events are added too, at the end of the plugin node
        /*
               for(int i=0;i<triage.size();i++) {
               file << "|{"<< triage[i].key <<"|"<< triage[i].toString() <<"}";
               }
             */
        file << "</table>>];\n";
        // link to queues
        // special case: if a queue is "log", then make it different
        std::map<std::string,bool> dejavu;
        // link from triage
        for(int i=0;i<triage.size();i++) {
            int p=portMap(triage[i]->portName);
            if( p<0 ) continue; // err!!
            // already output?
            map<std::string,bool>::const_iterator itdj;
            itdj=dejavu.find(triage[i]->portName);
            if( itdj!=dejavu.end() ) continue;
            dejavu[triage[i]->portName]=true;
            string qname=string("Q")+Qs[p]->name;
            string pname=string("P")+name+string(":")+NumberToString(p);
            file << pname << " -> " << qname << " [color=\"red\" style=\"dashed\" constraint=false]\n";
        }

        for(itr=ports.begin();itr!=ports.end();itr++) {
            int p=itr->second;
            if( badpos(p) ) continue;
            // is this port IN or OUT?
            map<std::string,bool>::const_iterator itd;
            itd=portsIN.find(itr->first);
            string qname=string("Q")+Qs[p]->name;
            string pname=string("P")+name+string(":")+NumberToString(p);
            // cas special pour les log!!!!
            // une queue avec un nom log* sera sans liens directs
            if( (Qs[p]->name).substr(0,3)=="log" ) {
                string oldqname=qname;
                qname+=name;
                file << qname << " [shape=\"egg\" peripheries=\"2\" label=\""<<oldqname<<"\"];\n";
            }
            if( itd==portsIN.end() || !itd->second ) {
                // Not IN (we assume OUT)
                // check if already output
                map<std::string,bool>::const_iterator itdj;
                itdj=dejavu.find(itr->first);
                if( itdj==dejavu.end() ) {
                    file << pname << " -> " << qname << "\n";
                }
            }else{
                // IN
                file << qname << " -> " << pname << "\n";
            }
        }
    }




private:
    // called from thread
    static void mainthread(plugin *p) {
        // le comportement par default de Cancel est ENABLE/DEFERRED.
        //int oldstate;
        //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
        //pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldstate);

        p->init();
        // set the process name!
        // prctl(PR_SET_NAME,("imgv:"+p->name).c_str(),0,0,0);

        pthread_cleanup_push((void (*)(void*))cleanup,p);
        while( !p->kill && p->loop() ) ;
        pthread_cleanup_pop(1); // execute the cleanup
        pthread_exit(0);
    }
    static void cleanup(plugin *p) {
        std::cout << "plugin cleanup called for "<<p->name<< std::endl;
        p->uninit();
        std::cout << "plugin cleanup done for "<<p->name<< std::endl;
    }

    //
    // encore pour les logs...
    //
private:
    void logstr(const char *head,string msg,const char *style) {
        if( logPort<0 ) { logPort=portMap("log"); }
        // style only used in case we do not have the plugin activated and connected
        if( badpos(logPort) ) {
            // regular output!
            cout << "* \033[1;"<<style<<"m[" <<name<<"]\033[0m "<<msg<<endl;
        }else{
            message *m=getMessage(512);
            *m << osc::BeginMessage(head) << name << msg <<  osc::EndMessage;
            push_back_ctrl(logPort,&m);
        }
    }

public:
    static std::ostream& info(std::ostream &s) {
        mystream &ss = dynamic_cast<mystream&>(s);
        ss.p.logstr("/info",ss.str(),"32");
        ss.str("");ss.clear();
        return s;
    }
    static std::ostream& warn(std::ostream &s) {
        mystream &ss = dynamic_cast<mystream&>(s);
        ss.p.logstr("/warn",ss.str(),"33");
        ss.str("");ss.clear();
        return s;
    }
    static std::ostream& err(std::ostream &s) {
        mystream &ss = dynamic_cast<mystream&>(s);
        ss.p.logstr("/err",ss.str(),"31");
        ss.str("");ss.clear();
        return s;
    }


};





#endif
