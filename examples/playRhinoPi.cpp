//
// playRhinoPi
//
// video+raspicam+streamCam sur raspbery pi
//
//

/*!

  \defgroup playRhinoPi playRhinoPi
  @{
  \ingroup examples

  Display video+raspicam+stream on raspberri pi.<br>

  Usage
  -----------

  ~~~
  playRhinoPi -in 44444 -video toto.mp4
  ~~~

  Plugins used
  ------------
  - \ref pluginViewerGLES2
  - ...

  @}

 */

#include <imgv/imgv.hpp>
//#include <imgv/udpcast.h>
#include "config.hpp"

#include <imqueue.hpp>

#include <imgv/qpmanager.hpp>

//#define USE_PROFILOMETRE
#ifdef USE_PROFILOMETRE
	#include <profilometre.h>
#endif


// incoming data
#define IMG_PORT	44444
#define CMD_PORT	44445

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>


using namespace std;
using namespace cv;





//
// load une image et place la dans une queue d'images
//
void loadImg(string nom,string view,imqueue &Q)
{
	cv::Mat image=cv::imread(nom,-1);
	blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
	i1->view=view;
	//Q.push_back((recyclable **)&i1);
	Q.push_back(i1);
}

void loadImg(string nom,string view,string qname)
{
	cv::Mat image=cv::imread(nom,-1);
	blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
	i1->view=view;
	//Q.push_back((recyclable **)&i1);
	qpmanager::add(qname,i1);
}


void initialize(int inPort,char *videoName) {
	if( !qpmanager::pluginAvailable("pluginViewerGLES2")
			|| !qpmanager::pluginAvailable("pluginStream") 
			|| !qpmanager::pluginAvailable("pluginRaspicam") ) {
		cout << "Some plugins are not available... :-("<<endl;
		return;
	}

	//
	// display
	//
	qpmanager::addPlugin("V","pluginViewerGLES2");
	qpmanager::addQueue("display");
	qpmanager::connect("display","V","in");

	//
	// video
	//

	//
	// camera raspicam
	//
	qpmanager::addPlugin("C", "pluginRaspicam");
	qpmanager::addQueue("recycleCam");
	qpmanager::addEmptyImages("recycleCam", 30); // pour auto adjust + rapide
	qpmanager::connect("display", "C","out");
	qpmanager::connect("recycleCam", "C","in");

	//
	// camera stream
	//
	if( inPort>0 ) {
		qpmanager::addPlugin("S","pluginStream");
		qpmanager::addQueue("recycleStream");
		qpmanager::addEmptyImages("recycleStream",20);
		qpmanager::connect("recycleStream","S","in");
		qpmanager::connect("display","S","out");
	}

	message *m;

	//
	// setup display
	//

	// es2-video-cam-stream : expect uniform video/cam/stream

	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/add-layer") << "es2-video-cam-stream" << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	// start video
	*m << osc::BeginMessage("/video") << videoName /*"/home/pi/ooo.mp4"*/ << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	if( inPort>0 ) {
		m=new message(1042);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/view") << "img2" << osc::EndMessage;
		*m << osc::BeginMessage("/in") << inPort << osc::EndMessage;
		*m << osc::BeginMessage("/init-done") << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("recycleStream",m);
	}

	// ecoute pour des messages
	qpmanager::startNetListeningUdp(22222);
	//if( inCmdPort>0 ) qpmanager::startNetListeningTcp(inCmdPort);

	//
	// setup camera aspicam
	//

	int widthNum = 1280;
	int heightNum = 960;
	int resDen = 1;
	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/view") <<"img" << osc::EndMessage;
	*m << osc::BeginMessage("/setWidth") << widthNum/resDen << osc::EndMessage;
	*m << osc::BeginMessage("/setHeight") << heightNum/resDen << osc::EndMessage;
	*m << osc::BeginMessage("/setFormat") << 3 << osc::EndMessage;
	*m << osc::BeginMessage("/setExposure") << 1 << osc::EndMessage;
	*m << osc::BeginMessage("/setMetering") << 1 << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	qpmanager::add("recycleCam",m);


	qpmanager::start("V");
	qpmanager::start("C");
	qpmanager::start("S");


	for(int i=0;;i++) {
		cout<<"..."<<endl;
		// rien a faire...
		sleep(1);
		#ifdef USE_PROFILOMETRE
			if( i%10==0 ) profilometre_dump();
		#endif
		//qpmanager::dump();
		//		qpmanager::loop("V");
		//		qpmanager::loop("C");
		//	qpmanager::loop("V");
		//	qpmanager::loop("C");
		if( !qpmanager::status("V") ) break;
		//if( !V->status() ) break;

		//log.warn() << "pluginA" << "message" << Log::end;
		//log << "pluginB" << "message"; // ajoute simplement au msg courant, start info
		//log.info() << "pluginC" << "message" << Log::end;
		//log.err() << "pluginD" << "message" << Log::end;
		// send output to syslog
		//message *m=new message(256);
		//*m << osc::BeginMessage("/info") << "blub" << "toto" << osc::EndMessage;
		//qpmanager::add("Log.in",m);

	}

	//if( inCmdPort>0 ) qpmanager::stopNetListening();

	qpmanager::stop("V");
	qpmanager::stop("C");
	qpmanager::stop("S");

}

int main(int argc,char *argv[]) {
	char videoName[200];
	int inPort;

#ifdef USE_PROFILOMETRE
	profilometre_init();
#endif

	videoName[0]=0;
	inPort=44444;

	cout << "opencv version " << CV_VERSION << endl;
	qpmanager::initialize();

	int i;
	for(i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			cout << "usage: playRhinoPi [-in 44444] [-video toto.mp4]"<<endl;
			exit(0);
		}
		/**
		  if( strcmp(argv[i],"-in")==0 && i+1<argc ) {
		  inImgPort=atoi(argv[i+1]);
		  i++;continue;
		  }
		 **/
		if(strcmp(argv[i], "-video") == 0 && i+1<argc ){
			 strcpy(videoName,argv[i+1]);
			 continue;
		}
		if(strcmp(argv[i], "-in") == 0 && i+1<argc){
			inPort=atoi(argv[i+1]);
			i++;continue;
		}
	}

	initialize(inPort,videoName);

	//go(IMG_PORT,CMD_PORT,startTime, raspicamIsUsed, videoIsUsed,useDetect,lutName,imageName);



	//qpmanager::dump();
	//qpmanager::dumpDot("out.dot");
}


