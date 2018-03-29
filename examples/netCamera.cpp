
/*!

\defgroup netCamera netCamera
@{
\ingroup examples

Send a camera image over the network to another netCamera app

Pour le raspberry pi, utilise le viewer GLES2 plutot que GLFW, et Raspicam plutot que Webcam

Usage
-----------

~~~
netCamera [-tcp] -in _port_
netCamera [-tcp] -out localhost 45678 [-delay 100] [-scale 0.5| [-realtime|-no-realtime]
~~~

To send the webcam images to the address 192.168.15.200, at port 40000.<br>
The optional delay is in microsecond, between each packet sent.

~~~
netCamera -out 192.168.15.200 40000
~~~

To receive images at local port 40000

~~~
netCamera -in 40000
~~~


Plugins used
------------
- \ref pluginStream
- \ref pluginViewerGLFW

@}

*/

//
// netCamera
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


void loadImg(string nom,string view,string qname)
{
        cv::Mat image=cv::imread(nom);
        blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
        i1->view=view;
        //Q.push_back((recyclable **)&i1);
        qpmanager::add(qname,i1);
}




//
// test plugin stream (receive)
//
void go_in(int port,bool tcp) {
	if( !qpmanager::pluginAvailable("pluginStream") ) {
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

	qpmanager::addPlugin("S","pluginStream");

	qpmanager::addQueue("display");

	qpmanager::connect("recycle","S","in");

	qpmanager::connect("display","S","out");

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
	// plugin stream
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
	if( tcp ) {
		*m << osc::BeginMessage("/intcp") << port << osc::EndMessage;
	}else{
		*m << osc::BeginMessage("/in") << port << osc::EndMessage;
	}
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycle",m);

	//
	// mode full automatique
	//
	qpmanager::start("S");

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

	qpmanager::stop("S");
}


//
// test plugin stream (send)
//
void go_out(const char *host,int port,float scale,bool realtime,int mtu,int delay,bool tcp,bool useDisplay) {

	// test si les plugins sont disponibles... en direct plutot qu'avec ifdef
	if( !qpmanager::pluginAvailable("pluginStream") ) {
		cout << "plugin stream missing!" <<endl;
		return;
	}
#ifdef IMGV_RASPBERRY_PI
	if( !qpmanager::pluginAvailable("pluginViewerGLES2")
	 || !qpmanager::pluginAvailable("pluginRaspicam") ) {
		cout << "ViewerGLES2 or Raspicam not available... :-("<<endl;
		return;
	}
#else
	if( !qpmanager::pluginAvailable("pluginViewerGLFW")
	 || !qpmanager::pluginAvailable("pluginWebcam") ) {
		cout << "ViewerGLFW or Webcam not available... :-("<<endl;
		return;
	}
#endif
	//
	// les queues
	//
	qpmanager::addQueue("recycle");
	qpmanager::addQueue("display");
	qpmanager::addQueue("send");

	qpmanager::addEmptyImages("recycle",20);

	//
	// les plugins
	//
	qpmanager::addPlugin("S","pluginStream");
#ifdef IMGV_RASPBERRY_PI
	qpmanager::addPlugin("V","pluginViewerGLES2");
	qpmanager::addPlugin("W","pluginRaspicam");
#else
	qpmanager::addPlugin("V","pluginViewerGLFW");
	qpmanager::addPlugin("W","pluginWebcam");
#endif

	// recycle -> [W] -> display -> [V] -> send -> [S] -|

	qpmanager::connect("recycle","W","in");
if(useDisplay){
	qpmanager::connect("display","W","out");
	qpmanager::connect("display","V","in");
	qpmanager::connect("send","V","out");
}else{
	qpmanager::connect("send","W","out");
}
	qpmanager::connect("send","S","in");


	message *m;
if( useDisplay ) {
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
}


	m=new message(1024);

#ifdef IMGV_RASPBERRY_PI
	//
	// plugin Raspicam
	//
	int widthNum = 1280;
	int heightNum = 960;
	int resDen = 2;
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/view") <<"tex" << osc::EndMessage;
	*m << osc::BeginMessage("/setWidth") << widthNum/resDen << osc::EndMessage;
	*m << osc::BeginMessage("/setHeight") << heightNum/resDen << osc::EndMessage;
	*m << osc::BeginMessage("/setFormat") << 3 << osc::EndMessage;
	*m << osc::BeginMessage("/setExposure") << 1 << osc::EndMessage;
	*m << osc::BeginMessage("/setMetering") << 1 << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
#else
	//
	// plugin webcam
	//
	*m << osc::BeginMessage("/cam") << 0 << "/main/A/tex" << osc::EndMessage;
#endif
	qpmanager::add("recycle",m);

	//
	// plugin stream
	//
	m=new message(512);
	*m << osc::BeginBundleImmediate;
	// network, etc...
	*m << osc::BeginMessage("/real-time") << realtime << osc::EndMessage;
	if( tcp ) {
		*m << osc::BeginMessage("/outtcp") << host << port << osc::EndMessage;
	}else{
		*m << osc::BeginMessage("/out") << host << port << mtu << delay << osc::EndMessage;
	}
	*m << osc::BeginMessage("/scale") << scale << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("send",m);

	// LUT
	//loadImg("lut2.png","lut","send");

	//
	// mode full automatique
	//
	qpmanager::start("W");
	qpmanager::start("S");
if( useDisplay ) {
#ifdef APPLE
	qpmanager::init("V");
#else
	qpmanager::start("V");
#endif
}

	usleep(500000);
	qpmanager::dump();

	//
	// main loop
	//
	for(int i=0;;i++) {
if( useDisplay ) {
#ifdef APPLE
		usleep(10000);
		//if( !V->loop() ) break;
		if( !qpmanager::loop("V") ) break;
#else
		// rien a faire...
		if( !qpmanager::status("V") ) break;
		//if( !qpmanager::status("W") ) break;
#endif
}

		cout<<"..."<<endl;
                usleep(500000);


	}

if( useDisplay ) {
#ifdef APPLE
	qpmanager::uninit("V");
#else
	qpmanager::stop("V");
#endif
}
	qpmanager::stop("W");
	qpmanager::stop("S");
}



int main(int argc,char *argv[]) {


	qpmanager::initialize();
	bool in;
	float scale=-1; // -1 = no scaling
	int port;
	int listening_port;
	char host[100];
	bool realtime;
	int mtu;
	int delay;
	bool tcp;
	bool useDisplay;

#ifdef IMGV_RASPBERRY_PI
	cout << "** raspberry pi **"<<endl;
#endif


	tcp=false;
	in=false;
	strcpy(host,"localhost");
	listening_port=10000;
	port=45678;
	realtime=true;
	mtu=1450;
	delay=0;
	useDisplay=true;

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			cout << "netCamera -in 45678 [-tcp]\n"
			     << "netCamera -out localhost 45678 [-tcp] [-scale 0.5] [-delay 0] [-mtu 1450] [-realtime|-no-realtime] [-nodisplay]"
			     << endl;
			exit(0);
		}
		if( strcmp(argv[i],"-tcp")==0 ) {
			tcp=true;
			continue;
		}
		if( strcmp(argv[i],"-nodisplay")==0 ) {
                        useDisplay=false;
                        continue;
                }

		if( strcmp(argv[i],"-in")==0 && i+1<argc ) {
			port=atoi(argv[i+1]);
			in=true;
			i+=1;continue;
		}
		if( strcmp(argv[i],"-out")==0 && i+2<argc ) {
			strcpy(host,argv[i+1]);
			port=atoi(argv[i+2]);
			in=false;
			i+=2;continue;
		}
		if( strcmp(argv[i],"-delay")==0 && i+1<argc ) {
			delay=atoi(argv[i+1]);
			i+=1;continue;
		}
		if( strcmp(argv[i],"-listen")==0 && i+1<argc ) {
			listening_port=atoi(argv[i+1]);
			i+=1;continue;
		}
		if( strcmp(argv[i],"-mtu")==0 && i+1<argc ) {
			mtu=atoi(argv[i+1]);
			i+=1;continue;
		}
		if( strcmp(argv[i],"-scale")==0 && i+1<argc ) {
			scale=atof(argv[i+1]);
			i+=1;continue;
		}
		if( strcmp(argv[i],"-realtime")==0 ) {
			realtime=true;
			continue;
		}
		if( strcmp(argv[i],"-no-realtime")==0 ) {
			realtime=false;
			continue;
		}
	}
	cout << "opencv version " << CV_VERSION << endl;

	//qpmanager::startNetListeningUdp(listening_port);
	//cout << "listening for commands to port " << listening_port << endl;

	if( in ) 	go_in(port,tcp);
	else		go_out(host,port,scale,realtime,mtu,delay,tcp,useDisplay);
}


