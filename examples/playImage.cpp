//
// playImage
//
// example for sample plugins
//
// plugins used:  ViewerGLFW
//

/*!

\defgroup playImage playImage
@{
\ingroup examples

Display an image.<br>
While the image if displayed, it is possible to use the keys '1','2','3', and '4' to
setup an homography. Just place the mouse on each desired image corner in sequence and
press 1,2,3, and 4.<br>
Also, the keys 'H','J','K','L' have been defined to move the latest corner
left, down, up, and right respectively.

Usage
-----------

Display the _indian head_ image.
~~~
playImage
~~~

Display the images provided on the command line.
~~~
playImage toto.png
~~~

Plugins used
------------
- \ref pluginViewerGLFW
- \ref pluginHomog

@}

*/


#include <imgv/imgv.hpp>
#include "config.hpp"

#include <imgv/imqueue.hpp>

#include <imgv/qpmanager.hpp>


// definir pour tester les homographies
#define HOMOGRAPHIE

// define this to use the matrix shader instead of the lookup shader
#define HOMO_USE_MATRIX



#include <imgv/pluginViewerGLFW.hpp>
#ifdef HOMOGRAPHIE
#include <imgv/pluginHomog.hpp>
#endif

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>


using namespace std;
using namespace cv;

#ifdef IMGV_RASPBERRY_PI
  #define NORMAL_SHADER	"basees2"
#else
  #ifdef HOMOGRAPHIE
    #ifdef HOMO_USE_MATRIX
      #define NORMAL_SHADER "homographieMat"
    #else
      #define NORMAL_SHADER "homographie"
    #endif
  #else
    #ifdef APPLE
      #define NORMAL_SHADER "normal150"
    #else
      #define NORMAL_SHADER "normal"
    #endif
  #endif
#endif

//
// log /[info|warn|err] <plugin name> <message> ... other stuff ...
//
/***
class Log {
    struct send_t { };
    public:
    Log(std::string where) {
        cout << "Log log to "<<where<<endl;
        Qout=qpmanager::getQueue(where);
        m=NULL;
    }
    ~Log() { if(m!=NULL) delete m; }

    // add a string to the current message
    Log& operator<<( const char* s ) {
        if( m==NULL ) begin("/info");
        *m << s;
        return *this;
    }
    Log& operator<<( send_t &s ) {
        if( &s==&end ) { flush(); }
        return *this;
    }

    Log& info() { begin("/info"); return *this; }
    Log& warn() { begin("/warn"); return *this; }
    Log& err() { begin("/err"); return *this; }

    static send_t end;
    private:
    imqueue *Qout; // where we send the message
    message *m;
    void flush() {
        if( m!=NULL ) {
            if( Qout==NULL ) { delete m;m=NULL; }
            else{
                *m << osc::EndMessage;
                Qout->push_back(m);
            }
        }
    }
    void begin(std::string head) {
        flush();
        m=new message(256);
        *m << osc::BeginMessage(head.c_str());
    }
};

Log::send_t Log::end;
***/


class toto : public recyclable {
public:
    int a,b,c;
    toto() { cout << "toto"<<endl; }
    ~toto() { cout << "~toto"<<endl; }

};

//
// load une image et place la dans une queue d'images
//
void loadImg(string nom,string view,imqueue &Q)
{
    cv::Mat image=cv::imread(nom);
    blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
    i1->view=view;
    //Q.push_back((recyclable **)&i1);
    Q.push_back(i1);
}

void loadImg(string nom,string view,string qname)
{
    cv::Mat image=cv::imread(nom);
    blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
    i1->view=view;
    //Q.push_back((recyclable **)&i1);
    qpmanager::add(qname,i1);
}

//
// test plugin viewer GLFW
//
void go(int argc,char *argv[]) {

    // test si les plugins sont disponibles... en direct plutot qu'avec ifdef
    if( !qpmanager::pluginAvailable("pluginViewerGLFW")
            || !qpmanager::pluginAvailable("pluginHomog") ) {
        cout << "Some plugins are not available... :-("<<endl;
        return;
    }

    qpmanager::addPlugin("V","pluginViewerGLFW");
    qpmanager::addQueue("display");
    qpmanager::connect("display","V","in"); // cree une queue V.0

    qpmanager::addQueue("main");

    //qpmanager::addQueue("display");
    //qpmanager::dump();

    message *m;

    //imqueue Qdisplay("display");

#ifdef HOMOGRAPHIE
    qpmanager::addQueue("recycle");
    //qpmanager::addQueue("msgH");

    // ajoute quelques images a recycler
    for(int i=0;i<5;i++) {
        blob *image=new blob();
        image->n=i;
        image->setRecycleQueue(qpmanager::getQueue("recycle"));
        //Qrecycle.push_front(image);
        qpmanager::add("recycle",image);
    }

    //
    // plugin homographie
    //

    qpmanager::addPlugin("H","pluginHomog");
    qpmanager::connect("recycle","H","in");
    qpmanager::connect("display","H","out");
    //qpmanager::connect("","H","msg"/*2*/); ????

    // set la view pour l'image output
    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/set/view")<<"/main/A/homog"<<osc::EndMessage;
    *m << osc::BeginMessage("/set/out-size")<<800 << 600<<osc::EndMessage;
#ifdef HOMO_USE_MATRIX
    *m << osc::BeginMessage("/set/out/lookup")<<false<<osc::EndMessage;
    *m << osc::BeginMessage("/set/out/matrix")<<true<<osc::EndMessage;
    *m << osc::BeginMessage("/set/out/matrix-cmd")<<"/mat3/main/A/homog"<<osc::EndMessage;
#else
    *m << osc::BeginMessage("/set/out/lookup")<<true<<osc::EndMessage;
    *m << osc::BeginMessage("/set/out/matrix")<<false<<osc::EndMessage;
#endif
    *m << osc::BeginMessage("/go")<<osc::EndMessage; // generate first "full" view
    *m << osc::EndBundle;
    qpmanager::add("recycle",m); // on pourrait donner "plugin","port" et utiliser ca
#endif

#ifdef HOMOGRAPHIE
    //qpmanager::connect("H.in","V",2); // pour les events du viewer
#endif

    //
    // plugin ViewerGLFW
    //

    m=new message(1024);
    *m << osc::BeginBundleImmediate;
    //*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
    *m << osc::BeginMessage("/new-window/main") << 1920 << 0 << 800 << 600 << false << "fenetre principale" << osc::EndMessage;
    *m << osc::BeginMessage("/new-quad/main/back") << 0.0f << 1.0f << 0.0f << 1.0f
       << -10.0f << "normal" << osc::EndMessage;
    *m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f
       << -10.0f << NORMAL_SHADER << osc::EndMessage;

#ifdef HOMOGRAPHIE
    //
    // bind keys to send mouse info to the Homog plugin
    //
    // -> V.setQ(-1,qpmanager::getQueue("H.in"));
    //plugin<blob>* V=qpmanager::getP("V");
    string zzz=qpmanager::getPlugin("V")->reservePort(true);
    qpmanager::reservePort("V","tomain");
    qpmanager::connect("main","V","tomain");
    cout << "**** port is "<<zzz<<endl;
    //qpmanager::getPlugin("V")->setQ(zzz,qpmanager::getQueue("recycle")); // H.in
    qpmanager::connect("recycle","V",zzz);

    *m << osc::BeginMessage("/defevent-ctrl") << "1" << zzz << "/set/q" << 0 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "2" << zzz << "/set/q" << 1 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "3" << zzz << "/set/q" << 2 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "4" << zzz << "/set/q" << 3 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "5" << zzz << "/set/p" << 0 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "6" << zzz << "/set/p" << 1 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "7" << zzz << "/set/p" << 2 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "8" << zzz << "/set/p" << 3 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << " " << zzz << "/go" << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "left" << zzz << "/left" << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "down" << zzz << "/down" << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "up" << zzz << "/up" << osc::EndMessage;
    *m << osc::BeginMessage("/defevent-ctrl") << "right" << zzz << "/right" << osc::EndMessage;

#endif

    *m << osc::BeginMessage("/defevent-ctrl") << "esc" << "tomain" << "/done" << osc::EndMessage;
    *m << osc::EndBundle;
    // add message to Qdisplay, so the viewer can get it only after it is started.
    //Qdisplay.push_front(m);
    qpmanager::add("display",m);


    for(int i=1;i<argc;i++) loadImg(argv[i],"/main/A/tex","display");
    if( argc==1 ) loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg","/main/A/tex","display");

    // un background
    loadImg(std::string(IMGV_SHARE)+"/images/color-ramp.jpg","/main/back/tex","display");

    //qpmanager::dump();

    //plugin<blob>* V=qpmanager::getP("V");

    // connect display queue to viewer
    // a faire***********************************
    //V.setQ(0,&Qdisplay);

    //qpmanager::dump();

    //
    // mode full automatique
    //
#ifdef APPLE
    //V->init();
    qpmanager::init("V");
#else
    //V->start();
    qpmanager::start("V");
#endif

#ifdef HOMOGRAPHIE
    qpmanager::start("H");
#endif

    //
    // lire chaque image et l'ajouter a la queue d'affichage
    //
    //sleep(2);
    //qpmanager::dump();



    cout << "entering loop..."<<endl;

    for(int i=0;;i++) {
#ifdef APPLE
        usleep(10000);
        //if( !V->loop() ) break;
        if( !qpmanager::loop("V") ) break;
#else
        //cout<<"..."<<endl;
        // rien a faire...
        usleep(100000);

        //qpmanager::dump();
        if( !qpmanager::status("V") ) break;
        //if( !V->status() ) break;

        //if( qpmanager::queueSizeCtrl("main")>0 ) break; // go /exit from pressing esc
        message *m=qpmanager::getQueue("main")->pop_front_ctrl_nowait();
        if( m!=NULL ) {
            m->decode();
            bool done=false;
            for(int i=0;i<m->msgs.size();i++) {
                osc::ReceivedMessage rm=m->msgs[i];
                const char *address=rm.AddressPattern();
                if( oscutils::endsWith(address,"/done") ) {
                    done=true;
                }
                cout << "GOT "<< m->msgs[i] <<endl;
            }
            if( done ) break;
        }

        //log.warn() << "pluginA" << "message" << Log::end;
        //log << "pluginB" << "message"; // ajoute simplement au msg courant, start info
        //log.info() << "pluginC" << "message" << Log::end;
        //log.err() << "pluginD" << "message" << Log::end;
        // send output to syslog
        //message *m=new message(256);
        //*m << osc::BeginMessage("/info") << "blub" << "toto" << osc::EndMessage;
        //qpmanager::add("Log.in",m);
#endif
    }

#ifdef APPLE
    qpmanager::uninit("V");
    //V->uninit();
#else
    //V->stop();
    qpmanager::stop("V");
#endif
#ifdef HOMOGRAPHIE
    qpmanager::stop("H");
#endif

#ifdef USE_SYSLOG
    // last to be killed!
    sleep(1);
    qpmanager::stop("Log");
#endif
}


int main(int argc,char *argv[]) {
    cout << "opencv version " << CV_VERSION << endl;
    qpmanager::initialize();



    qpmanager::startNetListeningUdp(44444);
    go(argc,argv);

    for(;;) {
        if( !qpmanager::status("V") ) break;
        usleep(500000);
    }

    qpmanager::dump();
    qpmanager::dumpDot("out.dot");
}


