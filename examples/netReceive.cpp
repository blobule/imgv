
/*!

\defgroup netReceive netReceive
@{
\ingroup examples

Receive images from a net connexion. Assume that the images are sent using the Net plugin.
Pour le raspberry pi, utilise le viewer GLES2 plutot que GLFW

Usage
-----------

~~~
netReceive { [-udp port | -multicast host port | -tcp port] [-save yo%03d.jpg] [-display] }

inputs:
[-udp port]
[-multicast host port] 
[-tcp port]

outputs:
[-save toto%06d.jpg]

display:
[-display]

Normaly, each input is followed by one output. A single display is permitted, with or without a save
~~~


To receive images at local port 10000 with udp

~~~
netReceive -udp 10000
~~~


Plugins used
------------
- \ref pluginNet
- \ref pluginViewerGLFW or pluginViewerGLES2

@}

*/

//
// netReceive
//
//
// plugin used: Stream
//
//

#include <imgv/imgv.hpp>

#include <imqueue.hpp>

#include <imgv/qpmanager.hpp>


#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>


using namespace std;
using namespace cv;

#ifdef APPLE
	#define NORMAL_SHADER "normal150"
#else
	#define NORMAL_SHADER "normal"
#endif

//
// assume udp pour l'instant
//
void go(int port) {
	if( !qpmanager::pluginAvailable("pluginNet") ) {
		cout << "J'ai besoin du pluginStream!"<<endl;
		return;
	}
#ifdef IMGV_RASPBERRY_PI
	if( !qpmanager::pluginAvailable("pluginViewerGLES2") ) {
		cout << "ViewerGLES2 is missing!" <<endl;
		return;
	}
#else
	if( !qpmanager::pluginAvailable("pluginViewerGLFW") ) {
		cout << "ViewerGLFW or Webcam is missing!" << endl;
		return;
	}
#endif

	//
	// les queues
	//
	qpmanager::addQueue("recycle");
	qpmanager::addQueue("netcmd");

        // ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",20);

	//
	// les plugins
	//
#ifdef IMGV_RASPBERRY_PI
	qpmanager::addPlugin("V","pluginViewerGLES2");
#else
	qpmanager::addPlugin("V","pluginViewerGLFW");
#endif

	qpmanager::addPlugin("N","pluginNet");

	qpmanager::addQueue("display");

	qpmanager::connect("recycle","N","in");
	qpmanager::connect("netcmd","N","cmd");
	qpmanager::connect("display","N","out");
	qpmanager::connect("display","V","in");

	message *m;
	m=new message(1042);

#ifdef IMGV_RASPBERRY_PI
	//
	// plugin ViewerGLES2
	//
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/add-layer") << "es2-simple" << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
#else
	//
	// plugin ViewerGLFW
	//
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f
        << -10.0f << NORMAL_SHADER << osc::EndMessage;
	*m << osc::EndBundle;
#endif
	qpmanager::add("display",m);

	//
	// plugin Netl
	//
	m=new message(512);
	*m << osc::BeginBundleImmediate;

	// make sure to change the view so we can dislpay the image
#ifdef IMGV_RASPBERRY_PI
	*m << osc::BeginMessage("/view") << "tex" << osc::EndMessage;
#else
	*m << osc::BeginMessage("/view") << "/main/A/tex" << osc::EndMessage;
#endif
	// network, etc...
	//*m << osc::BeginMessage("/intcp") << port << osc::EndMessage;
	*m << osc::BeginMessage("/in/udp") << port << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("netcmd",m);

	//
	// mode full automatique
	//
	qpmanager::start("N");

#ifdef APPLE
	qpmanager::init("V");
#else
	qpmanager::start("V");
#endif

	usleep(500000);
	qpmanager::dump();

	//
	// main loop....
	//
	for(int i=0;;i++) {
#ifdef APPLE
		usleep(10000);
		//if( !V->loop() ) break;
		if( !qpmanager::loop("V") ) break;
#else
		// rien a faire...
		cout<<"..."<<endl;
		usleep(500000);
		if( !qpmanager::status("V") ) break;
		//if( !qpmanager::status("W") ) break;
#endif
	}

#ifdef APPLE
	qpmanager::uninit("V");
#else
	qpmanager::stop("V");
#endif

	qpmanager::stop("N");
}


// le plugin s'appelle 'name' et la queue de sortie name-out
// return the output queue name

// remember all plugins, so we can stop them eventually, and check the status
vector<string> plugins;



#define MODE_UDP	0

//
// Udp / multicast / tcp (MODE_UDP/MULTICAST/TCP)
//
string setupNetwork(int mode,int port) {
	string p=qpmanager::nextAnonymousPlugin();
	plugins.push_back(p);
	qpmanager::addPlugin(p,"pluginNet");

	string Qrecycle=qpmanager::nextAnonymousQueue();
	qpmanager::addQueue(Qrecycle);
	qpmanager::addEmptyImages(Qrecycle,20);

	string Qout=qpmanager::nextAnonymousQueue();
	qpmanager::addQueue(Qout);

	string Qcmd=qpmanager::nextAnonymousQueue();
	qpmanager::addQueue(Qcmd);

	cout << "Creating plugin "<<Qrecycle << " -> "<<p<<" -> "<<Qout<<endl;

	qpmanager::connect(Qrecycle,p,"in");
	qpmanager::connect(Qcmd,p,"cmd");
	qpmanager::connect(Qout,p,"out");

	message *m=new message(512);
	*m << osc::BeginBundleImmediate;
	// make sure to change the view so we can dislpay the image
#ifdef IMGV_RASPBERRY_PI
	*m << osc::BeginMessage("/view") << "tex" << osc::EndMessage;
#else
	*m << osc::BeginMessage("/view") << "/main/A/tex" << osc::EndMessage;
#endif
	*m << osc::BeginMessage("/in/udp") << port << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add(Qcmd,m);

	qpmanager::start(p);
	return Qout;
}


//
// Display
//
void setupDisplay(string Qin) {
	if( Qin.empty() ) { cout << "no input Q"<<endl; return; }
	cout << "Creating display "<<Qin<<endl;
	string p=qpmanager::nextAnonymousPlugin();
	plugins.push_back(p);
#ifdef IMGV_RASPBERRY_PI
	qpmanager::addPlugin(p,"pluginViewerGLES2");
#else
	qpmanager::addPlugin(p,"pluginViewerGLFW");
#endif

	message *m=new message(1042);
#ifdef IMGV_RASPBERRY_PI
	//
	// plugin ViewerGLES2
	//
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/add-layer") << "es2-simple" << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
#else
	//
	// plugin ViewerGLFW
	//
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f
        << -10.0f << NORMAL_SHADER << osc::EndMessage;
	*m << osc::EndBundle;
#endif
	qpmanager::add(Qin,m);
	qpmanager::connect(Qin,p,"in");

	qpmanager::start(p);
}



int main(int argc,char *argv[]) {

	int port;

	qpmanager::initialize();
	cout << "opencv version " << CV_VERSION << endl;

#ifdef IMGV_RASPBERRY_PI
	cout << "** raspberry pi **"<<endl;
#endif

//[-udp port]
//[-multicast host port] 
//[-tcp port]

	string currentQ;

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			cout << "netReceive [-udp 10000] "
				<< "[-display] "
				<< endl;
			exit(0);
		}
		if( strcmp(argv[i],"-udp")==0 && i+1<argc ) {
			port=atoi(argv[i+1]);
			currentQ=setupNetwork(MODE_UDP,port);
			i+=1;continue;
		}
		if( strcmp(argv[i],"-display")==0 ) {
			setupDisplay(currentQ);
			i+=1;continue;
		}
	}

	// all are started...

	bool done=false;
	for(;!done;) {
		cout << "..." << endl;
		usleep(500000); usleep(500000);
		for(int i=0;i<plugins.size();i++) {
			if( !qpmanager::status(plugins[i]) ) done=true;
		}
	}

	// stop everyone
	for(int i=0;i<plugins.size();i++) {
		qpmanager::stop(plugins[i]);
	}

	//qpmanager::startNetListeningUdp(44445);
	//cout << "listening to port 44445 for commands" << endl;

	//go(port);
}


