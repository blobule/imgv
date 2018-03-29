
#ifndef QPMANAGER_HPP
#define QPMANAGER_HPP

/*!

\defgroup gqpmanager QP (Queues & Plugins) manager
@{

This module provides functions to manage queues and plugins directly.

The details are documented in \ref qpmanager

The functions could, in theory, be called from any plugin, but the intent of this
class is to call al the functions from the main thread.

Queues
-----

Instead of declaring Queues directly in C++ like

~~~
imqueue Qxyz("xyz");
~~~

you can simply call:
~~~
qpmanager::addQueue("xyz");
~~~

The queue will be referenced by its names.

The typical layout of a program using qpmanager will look like this:

Example code 1
----------

~~~
check plugin availability
if( !qpmanager::pluginAvailable("pluginPattern") ) { exit(0); }
if( !qpmanager::pluginAvailable("pluginDrip") ) { exit(0); }
if( !qpmanager::pluginAvailable("pluginViewerGLFW") ) { exit(0); }

create a few queues
qpmanager::addQueue("recycle");
qpmanager::addQueue("drip");
qpmanager::addQueue("display");
qpmanager::addQueue("log");

create a few plugins
qpmanager::addPlugin("P","pluginPattern");
qpmanager::addPlugin("D","pluginDrip");
qpmanager::addPlugin("V","pluginViewerGLES2");
qpmanager::addPlugin("L","pluginLog");

It is possible to register a plugin directly :


connect queues and plugins
qpmanager::connect("recycle","P","in");
qpmanager::connect("drip","P","out");
qpmanager::connect("drip","D","in");
qpmanager::connect("display","D","out");
qpmanager::connect("display","V","in");

qpmanager::connect("log","L","in");
qpmanager::connect("log","P","log");
qpmanager::connect("log","D","log");
qpmanager::connect("log","V","log");

put some images in the recycle queue
qpmanager::addEmptyImages("recycle",20);

put initialization message in queues
...

start plugins
qpmanager::start("P");
qpmanager::start("D");
qpmanager::start("V");
qpmanager::start("L");

loop until viewer is killed
for(;;) {
	if( !qpmanager::status("V") ) break;
}

stop all plugins
qpmanager::stop("P");
qpmanager::stop("D");
qpmanager::stop("V");
qpmanager::stop("L");
~~~


@}
*/



#include <imgv/imqueue.hpp>
#include <imgv/plugin.hpp>

// pour pluginReseau
//#include "oscpack/ip/PacketListener.h"
//#include "oscpack/ip/UdpSocket.h"


#include <pthread.h>

//
// this class provide services to manage queues and plugins.
//
// all queues and plugins are in a global registry, so they can be found by plugins
// and anyone who wants to add a message to a queue.
//
//
//

//
// pour instancier des plugins a partir du nom
//
template<typename T> plugin<blob> * createInstance() { return new T; }
typedef std::map<std::string, plugin<blob>*(*)()> map_type;
#define REGISTER_PLUGIN_TYPE(name,type) qpmanager::registerPluginType(name,&createInstance<type>)

// pour la connection
class singleport {
	public:
	std::string pname; // plugin names (required)
	std::string portName; // can be empty for "unspecified"
	//imqueue *q; // the actual queue this plugin/port referes to (NULL ok)
	plugin<blob>* plu; // actual plugin
	singleport() { /*q=NULL;*/plu=NULL; }
};

//class pluginReseau;
class pluginNet2;


class qpmanager {
    public:

	/// Initialize the qpmanager services.
	static void initialize(bool startLog=true);
	static void registerPluginType(std::string,plugin<blob>*(*)());


/// \name Debugging help
/// @{

	/// Print a list of queues, plugins, and connexions
	static void dump();

	/// Output a .dot file describing the queues, plugins, and the connexions
	static void dumpDot(std::string fileName);

/// @}

/// \name Managing Queues
/// @{

	/// Add a new queue
	static int addQueue(std::string name);

	/// Create a unique name for a queue (does not create a Queue, just a name)
	static std::string nextAnonymousQueue();

	/// Give access to the internal structure of a Queue
	static imqueue* getQueue(std::string name);

	/// Test if a specific Queue is empty, on the image side
	static bool isEmpty(std::string qname);

	/// Test if a specific Queue is empty, on the control side
	static bool isEmptyCtrl(std::string qname);

	/// Return the size of the queue, on the image side
	static int queueSize(std::string qname);

	/// Return the size of the queue, on the control side
	static int queueSizeCtrl(std::string qname);

/// @}

/// \name Managing Plugins
/// @{

	/// Test for the availability of a specific plugin type
	static bool pluginAvailable(std::string ptype);

	/// Add a new plugin
	static int addPlugin(std::string name,std::string ptype);

	/// Create a unique name for a plugin (does not create a plugin, just a name)
	static std::string nextAnonymousPlugin();

	/// Give access to the internal structure of a Plugin
	static plugin<blob>* getPlugin(std::string name);

	/// reserve a new port in a plugin. Make sure you dont use a known port!
	static int reservePort(std::string pname,std::string portName);

	/// reserve a new port in a plugin, with a new unique created name.
	static std::string reservePort(std::string pname);

	/// Establish a connexion between a Queue and a specific port of a Plugin.
	static int connect(std::string qname,std::string pname,std::string portName);
	// you can specify directly a plugin, like when it is a local plugin
	static int connect(std::string qname,plugin<blob>* p,std::string portName);

	/// Establish a connexion between multiple Plugin ports and a single Queue
	static void connect(std::vector<singleport> &ports,std::string qname);


/// @}


/// \name Running Plugin with threads
/// @{

	/// Start a Plugin, with a new thread
	static bool start(std::string pname);

	/// Stop a Plugin (and kill its thread)
	static bool stop(std::string pname);

	/// Get the status of a Plugin
	static bool status(std::string pname);

/// @}

/// \name Running Plugin without threads
/// @{

	/// Initialize an instance of a Plugin, without creating a thread
	static bool init(std::string pname);

	/// Execute one computation loop of the Plugin
	static bool loop(std::string pname);

	/// Un-initialize an instance of a Plugin
	static bool uninit(std::string pname);

/// @}

/// \name Adding messages to queues
/// @{

	/// Add a message into a specific Queue, on the control side.
	static void add(std::string qname,message *&m);		// in control queue

	/// Add an image into a specific Queue, on the image side (obviously).
	static void addImage(std::string qname,blob *&i);		// in image queue

	/// Add a message into a specific Queue, on the image side.
	static void addWithImages(std::string qname,message *&m); // in queue with images

	/// Add an image into a specific Queue
	static void add(std::string qname,blob *&im);

	/// Add a number of empty images into a specific Queue
	static void addEmptyImages(std::string qname,int nb);

/// @}

/// \name Getting messages from queues
/// @{

	/// get message from control queue only
	/// return the number of messages...
	static message *pop_front_ctrl_nowait(std::string qname);
	static message *pop_front_ctrl_wait(std::string qname);
	// fill m and i according to a received message. Only one is non NULL. nowait: both could be NULL
	static void pop_front_nowait(std::string qname,message *&m,blob *&i);
	static void pop_front_wait(std::string qname,message *&m,blob *&i);

/// @}

/// @name Network listening
/// @{

	/// listen to OSC commands arriving at a specific port, in UDP.
	static void startNetListeningUdp(int port);
	static void startNetListeningMulticast(int port,string ip); // "226.0.0.1"

	/// listen to OSC commands arriving at a specific port, in TCP.
	static void startNetListeningTcp(int port);

	/// stop listening to the network, either in UDP or TCP
	static void stopNetListening();

	/// activate network debugging messages
	static void setNetworkDebug(bool state);

/// @}

    public:
	static bool networkDebug;

    private:

	// retourne une queue en particulier (NULL si pas de queue)
	static imqueue* getQ(std::string name);

	// retourne un plugin en particulier (NULL si pas de queue)
	static plugin<blob>* getP(std::string name);


	static bool initialized;
	// une map des noms et alias vers les queues dans le tableau
	// attention: plusieurs elements peuvent pointer sur la meme queue
	// la queue elle-meme contient son propre nom, pour debug only
	static std::map<std::string,imqueue*> qmap;
	// une map des plugins
	static std::map<std::string,plugin<blob>*> pmap;

	// plugins disponibles
	static map_type mapPNames;

	// pour la protection des donnees...
	static pthread_mutex_t *acces;  // pour acces general a la queue


	// numerotation de queues anonymes. number of next anonymous queue
	static int queueNum;
	// numerotation de plugin anonymes. number of next anonymous plugin
	static int pluginNum;

	//static pluginReseau *net; // NULL si pas actif (use startNetListening)
	static pluginNet2 *net; // NULL si pas actif (use startNetListening)


};




//
// connexion
// connect together plugin ports.
// connexion C... .add("P","in")
// ... .connect() -> will create an anonymous queue if needed
// ... .connect("recycle') will create the queue, assuming no queue exist already.
// In all cases, at most a single add() mus already be a queue,
//
class connexion {
	public:
	connexion() { err=false; }

	//connexion& operator<<(const char *s) { add(s); return *this; }

/*
	connexion& add(const char *ppname) {
		//cout << "adding pp="<<ppname<<endl;
		char buf[strlen(ppname)+1];
		strcpy(buf,ppname);
		// trouve le '.'
		int i,n,p;for(i=0,n=0;buf[i];i++) if( buf[i]=='.' ) {p=i;n++;}
		if( n!=1 ) {
			cout << "connexion: invalid format for "<<ppname<<" (n="<<n<<")"<<endl;
			err=true;
			return *this;
		}
		buf[p]=0;
		singleport sp;
		sp.pname=std::string(buf);
		sp.portName=std::string(buf+p+1);
		//cout << "added1 plugin "<<sp.pname<<" port "<<sp.portName<<endl;
		ports.push_back(sp);
		return *this;
	}
*/

/*
	connexion& add(const char *pname,int port) {
		//cout << "adding p="<<pname<<" port="<<port<<endl;
		singleport sp;
		sp.pname=std::string(pname);
		sp.portnum=port;
		ports.push_back(sp);
		return *this;
	}
*/
	connexion& add(const char *pname,const char *portName) {
		//cout << "adding p="<<pname<<" port name "<<portName<<endl;
		singleport sp;
		sp.pname=std::string(pname);
		sp.portName=std::string(portName);
		ports.push_back(sp);
		return *this;
	}
	connexion& reset() {
		ports.clear();
		err=false;
		return *this;
	}
	void connect() {
		int i;
		if( err ) { cout << "connect: failed because of errors."<<endl;return; }
		qpmanager::connect(ports,std::string());
		reset();
	}
	void connect(std::string qname) {
		int i;
		if( err ) { cout << "connect: failed because of errors."<<endl;return; }
		qpmanager::connect(ports,qname);
		reset();
	}

	private:

	// all vectors are in sync
	bool err;
	vector<singleport> ports;
};



#endif
