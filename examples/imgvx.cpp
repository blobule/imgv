

/*!

\defgroup imgv imgv Example
@{
\ingroup examples

Get images from one or more image source, pass them through some process, then send them to some image output.

Usage
-----------

~~~
imgv -image toto.jpg -display


 un exemple qui fait plus...

 imgv [-in ...] /toto/blub 123 [-in ... ] [-process ...] [-process] [-out ...]
 

 le in d'un -in est un recycle
 le in d'un -process est le dernier out
 le in d'un -out est le derniet out

 -in : Qrecycle -> in, out -> current
 -process : current -> in, out -> current+1
 -out : current -> in, out -> rien

 in:
 -image toto.jpg
 -image toto%03d.jpg 0 4
 -video toto.mp4
 -webcam 0
 -raspicam 0
 -prosilica 1234
 -pattern 10 leopard 1920 1080 32
 -receive udp 10000
 -receive multicast 226.0.0.1 10000
 -receive tcp 10000

 process:
 -drip 5 30.0 100
 -homog
 -transform
 -monitor 640 480 (comme -display, mais avec un out)

 out:
 -save toto%03d.jpg 30
 -display 640 480
 -send-udp 192168.2.10 10000
 -send-multicast 226.0.0.1 10000
 -send-tcp 192.2.278.22 10000


 imgv -receive udp 1000 -display 640 480 -receive udp 1001 -save toto%06d.jpg 100


inputs:
[-receive-udp port]
[-receive-multicast host port] 
[-receive-tcp port]

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

//if( !qpmanager::pluginAvailable("type") ) {

// remember all plugins, so we can stop them eventually, and check the status
vector<string> plugins;

//
// -in : use your own recycle queue as input, create new currentQout for output
// -process : use currentQout for input, create a new currentQout for output.
// -out : use currentQout for input, does not create any output queue or connexion.
//

string currentQcmd; // this is the Q to control the latest added plugin (used by -osc /....)
string currentQout; // this is the output Q of the latest added plugin 


#define DIRECTION_IN	0
#define DIRECTION_OUT	1
#define MODE_UDP	0
#define MODE_MULTICAST	0
#define MODE_TCP	0

//
// -receive-udp, -receive-multicast -receive-tcp
//
int setupNetwork(int direction,int mode,int port) {
	string p=qpmanager::nextAnonymousPlugin();
	plugins.push_back(p);
	qpmanager::addPlugin(p,"pluginNet");

	string Qrecycle=qpmanager::nextAnonymousQueue();
	qpmanager::addQueue(Qrecycle);
	qpmanager::addEmptyImages(Qrecycle,10);

	string currentQcmd=qpmanager::nextAnonymousQueue();
	qpmanager::addQueue(currentQcmd);

	if( currentQout.empty() ) {
		currentQout=qpmanager::nextAnonymousQueue();
		qpmanager::addQueue(currentQout);
	}

	cout << "# Creating plugin "<<Qrecycle <<"/"<<currentQcmd << " -> "<<p<<" -> "<<currentQout<<endl;

	qpmanager::connect(Qrecycle,p,"in");
	qpmanager::connect(currentQcmd,p,"cmd");
	qpmanager::connect(currentQout,p,"out");

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
	qpmanager::add(currentQcmd,m);

	qpmanager::start(p);
	return 0;
}


//
// OUT: Display
//
int setupDisplay() {
	if( currentQout.empty() ) { cout << "nothing to display!" <<endl; return -1; }

	cout << "# -display IN="<<currentQout<<endl;

	string p=qpmanager::nextAnonymousPlugin();
	plugins.push_back(p);
#ifdef IMGV_RASPBERRY_PI
	qpmanager::addPlugin(p,"pluginViewerGLES2");
#else
	qpmanager::addPlugin(p,"pluginViewerGLFW");
#endif
	qpmanager::connect(currentQout,p,"in");
	currentQcmd=currentQout;

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
	qpmanager::add(currentQcmd,m);

	qpmanager::start(p);
	return 0;
}




int main(int argc,char *argv[]) {

	int port;

	cout << "imgv   version " << IMGV_VERSION << endl;
	cout << "opencv version " << CV_VERSION << endl;
	qpmanager::initialize();

#ifdef IMGV_RASPBERRY_PI
	cout << "** raspberry pi **"<<endl;
#endif

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			cout << "imgvx [-receive-udp 10000] [-save toto%05d.jpg] [-display]"
				<< endl;
			exit(0);
		}

		//
		// in
		//
		if( strcmp(argv[i],"-receive-udp")==0 && i+1<argc ) {
			port=atoi(argv[i+1]);
			if( setupNetwork(DIRECTION_IN,MODE_UDP,port) ) exit(0);
			i+=1;continue;
		}

		//
		// out
		//
		if( strcmp(argv[i],"-display")==0 ) {
			if( setupDisplay() ) exit(0);
			continue;
		}
/*
		if( strcmp(argv[i],"-save")==0 && i+1<argc ) {
			if( setupSave(argv[i+1]) ) exit(0);
			i+=1;continue;
		}
*/
	}

	// main loop!
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


