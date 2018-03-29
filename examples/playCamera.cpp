//
// playMovie
//
// example for sample plugins
//
// plugins used:  Ffmpeg, Drip, ViewerGLFW
//

#include <imgv/imgv.hpp>
#include "config.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

#include <imgv/imqueue.hpp>

#include <imgv/qpmanager.hpp>


using namespace std;
using namespace cv;

//#define USE_PROFILOMETRE
#ifdef USE_PROFILOMETRE
 #include <profilometre.h>
#endif

//#define SHADER "normalRGB"
//#define SHADER "normalChroma"
#define SHADER "normal"

//#define SHADER "lea"
// ajoute un param blub (int, 0..10000)
//#define SHADER "B"

//
// ports for defevents callback
#define DIALOG_CAM	"dialog-cam"


//#define CAMID1	"50-0536871842"
//#define CAMID2	"50-0536872013"


#define CAMID1	"DEV_000F315B90CB"
#define CAMID2	"DEV_000F315B90CD"
#define CAMID3	"DEV_000F315B97BF"
#define CAMID4	"DEV_000F315B9022"
//#define CAMID5	"DEV_000F315B97BD"
#define CAMID5	"DEV_000F315B93E5"
#define CAMID6	"DEV_000F315B97BE"


//
// type of camera (plugin)
//
#define CAM_1PROSILICA	0
#define CAM_2PROSILICA	1
#define CAM_6PROSILICA	2
#define CAM_6FISHEYE	3
#define CAM_WEBCAM	10
#define CAM_V4L2	20
#define CAM_PTGREY	30

#ifdef HAVE_PLUGIN_PTGREY
#include <imgv/pluginPtGrey.hpp>
#endif

void single_ptgrey() {

#ifdef HAVE_PLUGIN_PTGREY
	pluginPtGrey *toto=new pluginPtGrey();

	qpmanager::addQueue("recycle");
	qpmanager::addQueue("display");

	qpmanager::addEmptyImages("recycle",20);

	qpmanager::addPlugin("V","pluginViewerGLFW");

	qpmanager::addPlugin("P","pluginPtGrey");

	qpmanager::connect("recycle","P","in");
	qpmanager::connect("display","P","out");
	qpmanager::connect("display","V","in");

	// affichage
	message *m;
	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-monitor/main") << "DVI-D-0" << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 0 << 0 << 1920 << 1080 << false << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << SHADER << osc::EndMessage;
	// esc: tell the viewer to quit
	*m << osc::BeginMessage("/defevent-ctrl") << "esc" << "in" << "/kill" << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	// demarre la camera
	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/view") << "/main/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/camera") << 1 << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycle",m);

	qpmanager::start("V");
	qpmanager::start("P");

	usleep(500000); // a cause du status

	for(int i=0;;i++) {
		usleep(500000);
		if( !qpmanager::status("V") ) break;
	}

	qpmanager::stop("P");
	qpmanager::stop("V");
#endif
}


void dual_ptgrey(char *monitor) {
#ifdef HAVE_PLUGIN_PTGREY

	pluginPtGrey *toto=new pluginPtGrey();

	qpmanager::addQueue("recycle0");
	qpmanager::addQueue("recycle1");
	qpmanager::addQueue("display");

	qpmanager::addEmptyImages("recycle0",20);
	qpmanager::addEmptyImages("recycle1",20);

	qpmanager::addPlugin("V","pluginViewerGLFW");

	qpmanager::addPlugin("P0","pluginPtGrey");
	qpmanager::addPlugin("P1","pluginPtGrey");

	qpmanager::connect("recycle0","P0","in");
	qpmanager::connect("recycle1","P1","in");
	qpmanager::connect("display","P0","out");
	qpmanager::connect("display","P1","out");
	qpmanager::connect("display","V","in");

	// affichage
	message *m;
	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-monitor/main");
	if( monitor || monitor[0]!=0 ) {
		*m << monitor;
	}else{
 		*m << "HDMI-0"/*"DVI-I-1"*//*"DVI-D-0"*/;
	}
	*m << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 0 << 0 << 1920 << 1080 << false << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 0.5f << 0.3f << 0.9f << -10.0f << SHADER << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/B") << 0.5f << 1.0f << 0.3f << 0.9f << -10.0f << SHADER << osc::EndMessage;

	//*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << SHADER << osc::EndMessage;
	//*m << osc::BeginMessage("/new-monitor/other") << "DVI-I-1" << osc::EndMessage;
	//*m << osc::BeginMessage("/new-window/other") << 0 << 0 << 2560 << 1080 << false << "fenetre principale" << osc::EndMessage;
	//*m << osc::BeginMessage("/new-quad/other/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << SHADER << osc::EndMessage;

	// esc: tell the viewer to quit
	*m << osc::BeginMessage("/defevent-ctrl") << "esc" << "in" << "/kill" << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	// demarre la camera
	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/view") << "/main/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/camera") << 1 << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycle0",m);

	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/view") << "/main/B/tex" /*"/other/A/tex"*/ << osc::EndMessage;
	*m << osc::BeginMessage("/camera") << 0 << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycle1",m);

	qpmanager::start("V");
	qpmanager::start("P0");
	qpmanager::start("P1");

	usleep(500000); // a cause du status

	for(int i=0;;i++) {
		usleep(500000);
		if( !qpmanager::status("V") ) break;
	}

	qpmanager::stop("P0");
	qpmanager::stop("P1");
	qpmanager::stop("V");
#endif
}

// video: true = save as a video, false=save images with %d in the image name
// prosilica=false -> use webcam (camnum donne le # de camera)
void single_camera(int camType,char *filename,bool video,int camnum,int exposure) {
	if( !qpmanager::pluginAvailable("pluginViewerGLFW") ) { cout << "NO GLFW viewer" << endl;return; }

	message *m;

	qpmanager::addQueue("recycle");
	qpmanager::addQueue("display");

	// ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",20);

	// Viewer
	qpmanager::addPlugin("V","pluginViewerGLFW");

	switch( camType ) {
	  case CAM_1PROSILICA:
		if( !qpmanager::pluginAvailable("pluginProsilica") ) { cout << "NO prosilica plugin" << endl;return; }
		qpmanager::addPlugin("C","pluginProsilica");

		m=new message(1042);
		*m << osc::BeginBundleImmediate;
		// high performance... dont loose frames. use all mem if needed.
		*m << osc::BeginMessage("/wait-for-recycle") << false << osc::EndMessage;
		*m << osc::BeginMessage("/open") << "" << "/main/A/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningHorizontal" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningVertical" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "OffsetX" << 0 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "OffsetY" << 0 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "" << "Width" << 2048 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "" << "Height" << 2048 << osc::EndMessage;
		// after "start-capture", you cant change image parameters (width, height, format,.)
		*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("Mono8") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("bayerRGB8Packed") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "02-2011B-06136" << "PixelFormat" << osc::Symbol("BayerRG8") << osc::EndMessage;
		*m << osc::BeginMessage("/start-capture") << "*" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "ExposureTimeAbs" << (float)exposure << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "AcquisitionFrameRateAbs" << 10.0f << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
		*m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
		*m << osc::BeginMessage("/start-acquisition") << "*" << osc::EndMessage;
		
		*m << osc::EndBundle;
		qpmanager::add("recycle",m);
		break;
	  case CAM_WEBCAM:
		if( !qpmanager::pluginAvailable("pluginWebcam") ) { cout << "NO webcam plugin" << endl;return; }
		qpmanager::addPlugin("C","pluginWebcam");

		m=new message(1042);
		*m << osc::BeginMessage("/cam") << camnum << "/main/A/tex" << osc::EndMessage;
		qpmanager::add("recycle",m);
		break;
	  case CAM_V4L2:
		if( !qpmanager::pluginAvailable("pluginV4L2") ) { cout << "NO V4L2 plugin" << endl;return; }
		qpmanager::addPlugin("C","pluginV4L2");

		m=new message(1042);
		*m << osc::BeginMessage("/cam") << camnum << "/main/A/tex" << osc::EndMessage;
		qpmanager::add("recycle",m);
		break;
	}

	qpmanager::connect("recycle","C","in");
	qpmanager::connect("display","C","out");
	qpmanager::connect("display","V","in");

	//
	// PLUGIN: Prosilica
	//
	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/new-monitor/main") << "HDMI-0" << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 0 << 0 << 1920/*1318*/ << 1080/*986*/ << false/*true*/ << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << SHADER << osc::EndMessage;
	// esc: tell the viewer to quit
	*m << osc::BeginMessage("/defevent-ctrl") << "esc" << "in" << "/kill" << osc::EndMessage;

	/*
	if( prosilica ) {
		*m << osc::BeginMessage("/defevent-ctrl") << 'A' << DIALOG_CAM << "/set" << "" << "AcquisitionFrameRateAbs" << 15.0f << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'B' << DIALOG_CAM << "/set" << "" << "AcquisitionFrameRateAbs" << 1.0f << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'C' << DIALOG_CAM << "/set" << "" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'D' << DIALOG_CAM << "/set" << "" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'Z' << DIALOG_CAM << "/set" << "" << "TriggerSoftware" << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'W' << DIALOG_CAM << "/set" << "" << "BalanceWhiteAuto" << "Continuous" << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'Q' << DIALOG_CAM << "/set" << "" << "BalanceWhiteAuto" << "Off" << osc::EndMessage;
	}else{
		*m << osc::BeginMessage("/defevent-ctrl") << 'A' << DIALOG_CAM << "/line/begin" << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'B' << DIALOG_CAM << "/line/end" << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	}
	*/
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	qpmanager::add("display",m);

/*****
	if( !prosilica ) {
		// output de la camera ->[1] -> Qdisplay
		m=new message(256);
		*m << osc::BeginMessage("/defevent-ctrl") << -1 << 1 << "/set/main/A/blub" << osc::Symbol("$x") << osc::EndMessage;
		Qrecycle.push_back(m);
	}
******/


	//V.reservePort(DIALOG_CAM);

	//
	// connexions entre les plugins (par les rqueues)
	//
	// on peut utiliser directement le # de port,
	// ou donne un nom de port (varie selon le plugin)

	// camera

		// setup so we see the "line" command in the log
		//message *m=new message(1042);
		//*m << osc::BeginMessage("/defevent-ctrl") << "line" << "log" << "/warn" << Cweb.name << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;

	// viewer
	//V.setQ(DIALOG_CAM,&Qrecycle); // a quoi ca sert? pour parler a la camera.. anonyme, non?
	//C.setQ(2,&Qmsgffmpeg); // pour parler au ffmpeg
	//C.setQ(3,&Qmsgdrip); // pour parler au drip

	// le save!
	if( filename ) {
		qpmanager::addPlugin("S","pluginSave");
		qpmanager::addQueue("save");
		qpmanager::connect("save","V","out");
		qpmanager::connect("save","S","in");

		// mesages d'init
		m=new message(1024);
		*m << osc::BeginBundleImmediate;
		if( video ) *m << osc::BeginMessage("/mode") << "video" << osc::EndMessage;
		else	    *m << osc::BeginMessage("/mode") << "id" << osc::EndMessage;
		*m << osc::BeginMessage("/filename") << filename << osc::EndMessage;
		*m << osc::BeginMessage("/init-done") << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("save",m);
		qpmanager::start("S");
	}

	qpmanager::start("C");
	qpmanager::start("V");

	usleep(500000); // a cause du status

	for(int i=0;;i++) {
		usleep(500000);
		if( !qpmanager::status("V") ) break;
	}

	qpmanager::stop("S");
	qpmanager::stop("C");
	qpmanager::stop("V");
}



// video: true = save as a video, false=save images with %d in the image name
// prosilica=false -> use webcam (camnum donne le # de camera)
void dual_cameras() {
	float exposure=15000.0;
	if( !qpmanager::pluginAvailable("pluginViewerGLFW") ) { cout << "NO GLFW viewer" << endl;return; }

	message *m;

	qpmanager::addQueue("recycle");
	qpmanager::addQueue("display");

	// ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",20);

	// Viewer
	qpmanager::addPlugin("V","pluginViewerGLFW");

		if( !qpmanager::pluginAvailable("pluginProsilica") ) { cout << "NO prosilica plugin" << endl;return; }
		qpmanager::addPlugin("C","pluginProsilica");


		m=new message(1042);
		*m << osc::BeginBundleImmediate;
		// high performance... dont loose frames. use all mem if needed.
		*m << osc::BeginMessage("/wait-for-recycle") << false << osc::EndMessage;
		*m << osc::BeginMessage("/open") << CAMID5 << "/main/A/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/open") << CAMID6 << "/main/B/tex" << osc::EndMessage;
//50-0536872013
		*m << osc::BeginMessage("/set") << "*" << "BinningHorizontal" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningVertical" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "OffsetX" << 0 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "OffsetY" << 0 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Width" << 2048 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Height" << 2048 << osc::EndMessage;
		// after "start-capture", you cant change image parameters (width, height, format,.)
		*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("Mono8") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("bayerRGB8Packed") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "02-2011B-06136" << "PixelFormat" << osc::Symbol("BayerRG8") << osc::EndMessage;
		*m << osc::BeginMessage("/start-capture") << "*" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "ExposureTimeAbs" << (float)exposure << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "AcquisitionFrameRateAbs" << 10.0f << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
		*m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
		*m << osc::BeginMessage("/start-acquisition") << "*" << osc::EndMessage;
		
		*m << osc::EndBundle;
		qpmanager::add("recycle",m);

	qpmanager::connect("recycle","C","in");
	qpmanager::connect("display","C","out");
	qpmanager::connect("display","V","in");

	//
	// PLUGIN: Prosilica
	//
	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 1920 << 0 << 1920 << 1080 << false << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 0.5f << 0.0f << 1.0f << -10.0f << "A" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/B") << 0.5f << 1.0f << 0.0f << 1.0f << -10.0f << "A" << osc::EndMessage;
	// esc: tell the viewer to quit
	*m << osc::BeginMessage("/defevent-ctrl") << "esc" << "in" << "/kill" << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/A/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/A/angle") << (float)(30.0) << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/angle") << (float)(120.0+30) << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/A/scale") << (float)0.5 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/scale") << (float)0.5 << osc::EndMessage;
	float sep=0.05;
	*m << osc::BeginMessage("/set/main/A/sep") << (float)-sep << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/sep") << (float)sep << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	qpmanager::add("display",m);

	qpmanager::start("C");
	qpmanager::start("V");

	usleep(500000); // a cause du status

	for(int i=0;;i++) {
		usleep(500000);
		if( !qpmanager::status("V") ) break;
	}

	qpmanager::stop("S");
	qpmanager::stop("C");
	qpmanager::stop("V");
}


//
// plugin to pick stereo images from a stream
//
// when it receives  a command /pick, it will pick 6 images and send them to the dup port.
// (recycle images) -> recycle
// (stream or commands) -> in ... out -> (display)
// picked -> (stereo process)
//
class pluginPick : public plugin<blob> {
	int n;
	int receiving; // 0=no, >0 is number of pick to do
	blob *img[6]; // 6 images
    public:
        pluginPick() {
		ports["in"]=0;
		ports["out"]=1;
		ports["recycle"]=2;
		ports["picked"]=3;
		portsIN["in"]=true;
		portsIN["recycle"]=true;
	}
        ~pluginPick() { }
        void init() {
		int n=0;
		receiving=0;
		int k;
		for(k=0;k<6;k++) img[k]=NULL;
        }
        bool loop() {
		// wait for an image (and/or command)
		blob *i0=pop_front_wait(0);
	
		// no demand for images? just sent it out
		if( receiving==0 ) {
	    		push_back(1,(recyclable **)&i0); // let the image go
			n++;
			return true;
		}

		int num=-1;
		if( i0->view.compare("/main/A/tex")==0 ) num=0;
		else if( i0->view.compare("/main/B/tex")==0 ) num=1;
		else if( i0->view.compare("/main/C/tex")==0 ) num=2;
		else if( i0->view.compare("/main/D/tex")==0 ) num=3;
		else if( i0->view.compare("/main/E/tex")==0 ) num=4;
		else if( i0->view.compare("/main/F/tex")==0 ) num=5;

		log << "got image " << n << " view " << i0->view << " num " << num << info;

		if( num>=0 ) {
			if( img[num]==NULL ) img[num]=pop_front_wait(2); // from recycling
			// save the image
			i0->copyTo(*img[num]);
		}
	    	push_back(1,(recyclable **)&i0); // let the image go
		n++;

		// are we done yet? one image from 0 to 5?
		int k;
		int b=0;
		for(k=0;k<6;k++) if( img[k]!=NULL ) b++;
		if( b!=6 ) return true; // wait for more images

		log << "got size images... sending to stereo" << warn;

		for(k=0;k<6;k++) {
			push_back(3,(recyclable **)&img[k]); // let the image go
		}
		receiving--;
		return true;
        }
       bool decode(const osc::ReceivedMessage &m) {
                const char *address=m.AddressPattern();
                if( oscutils::endsWith(address,"/pick") ) {
                        m.ArgumentStream() >> osc::EndMessage;
			receiving++;
                }else{
                        log << "????? : " << m << err;
                }
        }
};




//
// stereo matcher
//
// get 6 images from input port, the match them.
// the order is always the same.
//
//

#define NB	6



class calib {
	public:
	double centerX,centerY;
	double horizon;
	double fov; // ???
	double position;
	double rotationX,rotationY,rotationZ;
	double voffset;
	double flip; // PI si camera >=3, sinon 0
	double resolutionX;
	double resolutionY;
	// computed;
	Mat M1;

	void dump() {
		cout << "centerX   "<<centerX<< endl;
		cout << "centerY   "<<centerY<< endl;
		cout << "horizon   "<<horizon<< endl;
		cout << "position  "<<position<< endl;
		cout << "rotationX "<<rotationX<< endl;
		cout << "rotationY "<<rotationY<< endl;
		cout << "rotationZ "<<rotationZ<< endl;
		cout << "resolutionX "<<resolutionX<< endl;
		cout << "resolutionY "<<resolutionY<< endl;
		cout << "voffset   "<<voffset<< endl;
		cout << "flip      "<<flip<< endl;
		cout << "M1 is     "<< M1 << endl;
	}
	void computeM1() {
		double angle;
		Mat Tinv = (Mat_<double>(4, 4) <<
		      1, 0, 0, 0,
		      0, 1, 0, voffset,
		      0, 0, 1, 3.75 /* circle_radius */,
		      0, 0, 0, 1);

		angle=position*M_PI/180.0;
		Mat Rorient = (Mat_<double>(4, 4) <<
			cos(angle), 0, sin(angle), 0,
			0, 1, 0, 0,
			-sin(angle), 0, cos(angle), 0,
			0, 0, 0, 1);

		angle=rotationY*M_PI/180.0;
		Mat Ry = (Mat_<double>(4, 4) <<
			cos(angle), 0, sin(angle), 0,
			0, 1, 0, 0,
			-sin(angle), 0, cos(angle), 0,
			0, 0, 0, 1);

		angle=rotationZ*M_PI/180.0;
		Mat Rz = (Mat_<double>(4, 4) <<
			cos(angle), -sin(angle), 0, 0,
			sin(angle), cos(angle), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);

		angle=rotationX*M_PI/180.0;
		Mat Rx = (Mat_<double>(4, 4) <<
			1, 0, 0, 0,
			0, cos(angle), -sin(angle), 0,
			0, sin(angle), cos(angle), 0,
			0, 0, 0, 1);

		Mat Rflip = (Mat_<double>(4, 4) <<
			1, 0, 0, 0,
			0, cos(flip), -sin(flip), 0,
			0, sin(flip), cos(flip), 0,
			0, 0, 0, 1);
		M1=Rx.t()*Rz.t()*Ry.t()*Tinv*Rflip*Rorient.t();
	}

	Point2d world2fisheye(Point3d p) {
		// put in fisheye space
		//Mat q = M1*Mat(p);
		Matx41d q=Matx44d(M1)*p;
		//cout  << "yo "<<q<<endl;
		q.val[0]=-q.val[0];
		double len=sqrt(q.dot(q));
		//cout << "len is "<<len<<endl;
		q.val[0]/=len;
		q.val[1]/=len;
		q.val[2]/=len;
		//cout  << "yo "<<q<<endl;
		double theta=acos(q.val[1])/(M_PI/2); // angle vertical -> -1..0..1
		double f=theta*horizon; // distance du center = angle vertical (horizon=180deg)
		Matx21d r(q.val[0],q.val[2]); // direction horizontale
		len=sqrt(r.dot(r));
		Matx21d pimg=Matx21d(r.val[0]/len*f+centerX,r.val[1]/len*f+centerY);
		//cout << "proj is "<<pimg<<endl;

/*
		q[0]=-q[0];
		double len=sqrt(q.x*q.x+q.y*q.y+q.z*q.z);
		q/=len;
		double theta=acos(q.y);
		double f=theta*2*radius/fov;
		len=sqrt(q.x*q.x+q.z*q.z);
		Vec2d pimg=Vec2d(q.x,q.z)/len*f+Vec2d(centerX,centerY);
		return pimg;
*/
		return Point2d(pimg.val[0],pimg.val[1]);
	}
	
};

#include <fstream>
#include <map>
#include <string>

// calibration... pour tout le monde
std::map<std::string,std::string> data;

void readCalib()
{
	std::ifstream infile("/home/vision/demo/calib-lab-sphere.stitch");
	std::string line;
	while (std::getline(infile, line))
	{
		int pos=line.find("=");
		string name=line.substr(0,pos);
		string val=line.substr(pos+1);
		//log << "pos is "<<pos<< ":"<<name<<":-:"<<val<<":"<<info;
		data[name]=val;
	}
}

class pluginStereo : public plugin<blob> {
	int n;
	blob *img[NB]; // 6 images
	calib C[NB]; // calibration
	calib Ctmp[6]; // calibration
    public:
        pluginStereo() {
		ports["in"]=0;
		ports["out"]=1;
		ports["recycle"]=2;
		portsIN["in"]=true;portsIN["recycle"]=true; 


		double voffset=atof(data["CAMERA_VOFFSET"].c_str());
		int i;
		for(i=0;i<6;i++) {
			ostringstream ss;
			ss << (i+1);
	     		string is=ss.str();
			Ctmp[i].centerX=atof(data["CALIB_CENTER_X_"+is].c_str());
			Ctmp[i].centerY=atof(data["CALIB_CENTER_Y_"+is].c_str());
			Ctmp[i].horizon=atof(data["CALIB_HORIZON_"+is].c_str());
			Ctmp[i].position=atof(data["CALIB_POSITION_"+is].c_str());
			Ctmp[i].rotationX=atof(data["CALIB_X_ROTATION_"+is].c_str());
			Ctmp[i].rotationY=atof(data["CALIB_Y_ROTATION_"+is].c_str());
			Ctmp[i].rotationZ=atof(data["CALIB_Z_ROTATION_"+is].c_str());
			Ctmp[i].resolutionX=atof(data["CAMERA_RESOLUTION_X"].c_str());
			Ctmp[i].resolutionY=atof(data["CAMERA_RESOLUTION_Y"].c_str());
			Ctmp[i].voffset= -voffset/2.0;
			Ctmp[i].flip=(i<3)?0.0:M_PI;
			Ctmp[i].computeM1();
			cout << "--- "<<i<<" ---"<<endl;
			Ctmp[i].dump();
		}
		C[0]=Ctmp[0];
		C[1]=Ctmp[1];
		C[2]=Ctmp[2];
		C[3]=Ctmp[3];
		C[4]=Ctmp[4];
		C[5]=Ctmp[5];

		// test
		double angle;
		for(angle=0.0;angle<=360.0;angle+=1.0) {
			double rad=2000.0;
			double a=angle*M_PI/180.0;
			Point3d p(rad*cos(a),0.0,rad*sin(a));
			int cam;
			for(cam=0;cam<NB;cam++) {
				Point2d q=C[cam].world2fisheye(p);
				cout << "#" << angle
					 << " " << cam 
					 << " " << p.x
					 << " " << p.y
					 << " " << p.z
					 << " " << q.x
					 << " " << q.y
					 << endl;
			}
		}
		// autre test
		double z;
		angle=0.0;
		for(z=40;z<200;z+=1.0) {
			double rad=z;
			double a=angle*M_PI/180.0;
			Point3d p(rad*cos(a),0.0,rad*sin(a));
			int cam;
			for(cam=0;cam<NB;cam++) {
				Point2d q=C[cam].world2fisheye(p);
				cout << "@" << angle
					 << " " << cam 
					 << " " << p.x
					 << " " << p.y
					 << " " << p.z
					 << " " << q.x
					 << " " << q.y
					 << endl;
			}
		}

		/*
		for(i=0;i<5;i++) {
			Point2d q=C[i].world2fisheye(p);
			cout << "PROJ "<<p << " cam "<<i<<" = "<<q<<endl;
		}
		*/


	}
        ~pluginStereo() { }
        void init() {
                n=0;
		int k;
		for(k=0;k<NB;k++) img[k]=NULL;
        }

	//
	// une camera
	//

        bool loop() {
            // wait for 6 images (and/or command)
		int k;
		for(k=0;k<NB;k++) {
            		img[k]=pop_front_wait(0);
		}
                log << "doing stereo... " << n << warn;
		log << "image cols="<<img[0]->cols<<", rows="<<img[0]->rows<<", es="<<img[0]->elemSize()<<", es1="<<img[0]->elemSize1() << info;

		// save images
		for(k=0;k<NB;k++) {
			char fname[]="stereo0.png";
			fname[6]='0'+k;
			cv::imwrite(fname,*img[k]);
		}

		// ... stereo ...
		blob *res=pop_front_wait(2); // from recycle
		log << "got recycled depth map" << info;
		res->create(img[0]->size(),CV_16UC1);
		int j;

		ushort *p=(ushort *)res->data;
		for(j=0;j<res->cols*res->rows;j++) p[j]=0;

		for(k=0;k<NB;k++) {
			uchar *q=(uchar *)img[k]->data;
			for(j=0;j<res->cols*res->rows;j++) p[j]+=q[j];
		}
		for(j=0;j<res->cols*res->rows;j++) p[j]=p[j]/NB*256;

		// recycle the input images
		for(k=0;k<NB;k++) recycle(img[k]);
		// send the result
		res->view="/main/depth/tex";
		push_back(1,(recyclable **)&res);
		n++;
            return true;
        }
       bool decode(const osc::ReceivedMessage &m) {
                const char *address=m.AddressPattern();
		log << "????? : " << m << err;
        }
};




// video: true = save as a video, false=save images with %d in the image name
// prosilica=false -> use webcam (camnum donne le # de camera)
void six_fisheye() {

	REGISTER_PLUGIN_TYPE("pluginPick",pluginPick);
	REGISTER_PLUGIN_TYPE("pluginStereo",pluginStereo);

	float exposure=10000.0;
	if( !qpmanager::pluginAvailable("pluginViewerGLFW") ) { cout << "NO GLFW viewer" << endl;return; }

	message *m;

	qpmanager::addQueue("recycle");
	qpmanager::addQueue("recycleStereo");
	qpmanager::addQueue("pick");
	qpmanager::addQueue("stereo");
	qpmanager::addQueue("display");

	// ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",20);
	qpmanager::addEmptyImages("recycleStereo",20);

	// Picker
	qpmanager::addPlugin("P","pluginPick");

	// Stereo
	qpmanager::addPlugin("S","pluginStereo");

	// Viewer
	qpmanager::addPlugin("V","pluginViewerGLFW");

	if( !qpmanager::pluginAvailable("pluginProsilica") ) { cout << "NO prosilica plugin" << endl;return; }
	qpmanager::addPlugin("C","pluginProsilica");

	//
	// PLUGIN : Prosilica
	//
	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	// high performance... dont loose frames. use all mem if needed.
	*m << osc::BeginMessage("/wait-for-recycle") << false << osc::EndMessage;
	*m << osc::BeginMessage("/open") << data["CAMERA_IP_1"] << "/main/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/open") << data["CAMERA_IP_2"] << "/main/B/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/open") << data["CAMERA_IP_3"] << "/main/C/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/open") << data["CAMERA_IP_4"] << "/main/D/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/open") << data["CAMERA_IP_5"] << "/main/E/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/open") << data["CAMERA_IP_6"] << "/main/F/tex" << osc::EndMessage;
//50-0536872017
	*m << osc::BeginMessage("/set") << "*" << "BinningHorizontal" << 1 << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "BinningVertical" << 1 << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "OffsetX" << 0 << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "OffsetY" << 0 << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "Width" << 1712 << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "Height" << 1712 << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "OffsetX" << 168 << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "OffsetY" << 168 << osc::EndMessage;
	// after "start-capture", you cant change image parameters (width, height, format,.)
	*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("Mono8") << osc::EndMessage;
	//*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("bayerRGB8Packed") << osc::EndMessage;
	//*m << osc::BeginMessage("/set") << "02-2011B-06136" << "PixelFormat" << osc::Symbol("BayerRG8") << osc::EndMessage;
	*m << osc::BeginMessage("/start-capture") << "*" << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "ExposureTimeAbs" << (float)exposure << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "AcquisitionFrameRateAbs" << 24.0f << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
	//*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
	*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
	//*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
	*m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
	*m << osc::BeginMessage("/start-acquisition") << "*" << osc::EndMessage;
		
	*m << osc::EndBundle;
	qpmanager::add("recycle",m);

	qpmanager::connect("recycle","C","in");
	qpmanager::connect("recycle","P","recycle");
	qpmanager::connect("pick","C","out");
	qpmanager::connect("pick","P","in");
	qpmanager::connect("display","P","out");
	qpmanager::connect("display","V","in");

	qpmanager::connect("stereo","P","picked"); // send images to stereo plugin
	qpmanager::connect("stereo","S","in"); // send images to stereo plugin

	qpmanager::connect("recycleStereo","S","recycle");
	qpmanager::connect("display","S","out"); // send stereo depth map to display

	//
	// PLUGIN: Viewer
	//
	qpmanager::reservePort("V","topick");
	qpmanager::connect("pick","V","topick");

	m=new message(2048);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 0 << 0 << /*1920*/ 1280 << /*1200*/600 << /*false*/true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 0.25f << 0.0f << 0.5f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/B") << 0.25f << 0.5f << 0.0f << 0.5f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/C") << 0.5f << 0.75f << 0.0f << 0.5f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/D") << 0.0f << 0.25f << 0.5f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/E") << 0.25f << 0.5f << 0.5f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/F") << 0.5f << 0.75f << 0.5f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/depth") << 0.75f << 1.0f << 0.0f << 0.5f << -10.0f << "normal" << osc::EndMessage;
	// esc: tell the viewer to quit
	*m << osc::BeginMessage("/defevent-ctrl") << "esc" << "in" << "/kill" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "S" << "topick" << "/pick" << osc::EndMessage;

	*m << osc::BeginMessage("/set/main/A/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/C/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/D/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/E/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/F/fade") << (float)1.0 << osc::EndMessage;

	*m << osc::BeginMessage("/set/main/A/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/C/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/D/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/E/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/F/angle") << (float)0.0 << osc::EndMessage;

	*m << osc::BeginMessage("/set/main/A/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/C/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/D/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/E/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/F/scale") << (float)1.0 << osc::EndMessage;

	*m << osc::BeginMessage("/set/main/A/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/C/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/D/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/E/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/F/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	qpmanager::add("display",m);

	qpmanager::start("C");
	qpmanager::start("P");
	qpmanager::start("S");
	qpmanager::start("V");

	usleep(500000); // a cause du status

	for(int i=0;;i++) {
		usleep(500000);
		if( !qpmanager::status("V") ) break;
	}

	qpmanager::stop("C");
	qpmanager::stop("P");
	qpmanager::stop("S");
	qpmanager::stop("V");
}



// video: true = save as a video, false=save images with %d in the image name
// prosilica=false -> use webcam (camnum donne le # de camera)
void six_cameras() {


	float exposure=20000.0;
	if( !qpmanager::pluginAvailable("pluginViewerGLFW") ) { cout << "NO GLFW viewer" << endl;return; }

	message *m;

	qpmanager::addQueue("recycle");
	qpmanager::addQueue("display");

	// ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",20);

	// Viewer
	qpmanager::addPlugin("V","pluginViewerGLFW");

		if( !qpmanager::pluginAvailable("pluginProsilica") ) { cout << "NO prosilica plugin" << endl;return; }
		qpmanager::addPlugin("C","pluginProsilica");


		m=new message(1042);
		*m << osc::BeginBundleImmediate;
		// high performance... dont loose frames. use all mem if needed.
		*m << osc::BeginMessage("/wait-for-recycle") << false << osc::EndMessage;
		*m << osc::BeginMessage("/open") << CAMID1 << "/main/A/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/open") << CAMID2 << "/main/B/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/open") << CAMID3 << "/main/C/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/open") << CAMID4 << "/main/D/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/open") << CAMID5 << "/main/E/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/open") << CAMID6 << "/main/F/tex" << osc::EndMessage;
//50-0536872017
		*m << osc::BeginMessage("/set") << "*" << "BinningHorizontal" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningVertical" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "OffsetX" << 0 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "OffsetY" << 0 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Width" << 2048 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Height" << 2048 << osc::EndMessage;
		// after "start-capture", you cant change image parameters (width, height, format,.)
		*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("Mono8") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("bayerRGB8Packed") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "02-2011B-06136" << "PixelFormat" << osc::Symbol("BayerRG8") << osc::EndMessage;
		*m << osc::BeginMessage("/start-capture") << "*" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "ExposureTimeAbs" << (float)exposure << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "AcquisitionFrameRateAbs" << 24.0f << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
		*m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
		*m << osc::BeginMessage("/start-acquisition") << "*" << osc::EndMessage;
		
		*m << osc::EndBundle;
		qpmanager::add("recycle",m);

	qpmanager::connect("recycle","C","in");
	qpmanager::connect("display","C","out");
	qpmanager::connect("display","V","in");

	//
	// PLUGIN: Prosilica
	//
	m=new message(2048);
	*m << osc::BeginBundleImmediate;
	float d=60.0/1920.0;
	*m << osc::BeginMessage("/new-window/main") << 0 << 0 << 1920 << 1200 << false << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f+d << 0.3125f+d << 0.0f << 0.5f << -10.0f << "A" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/B") << 0.3125f+d << 0.625f+d << 0.0f << 0.5f << -10.0f << "A" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/C") << 0.625f+d << 0.9375f+d << 0.0f << 0.5f << -10.0f << "A" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/D") << 0.0f+d << 0.3125f+d << 0.5f << 1.0f << -10.0f << "A" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/E") << 0.3125f+d << 0.625f+d << 0.5f << 1.0f << -10.0f << "A" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/F") << 0.625f+d << 0.9375f+d << 0.5f << 1.0f << -10.0f << "A" << osc::EndMessage;
	// esc: tell the viewer to quit
	*m << osc::BeginMessage("/defevent-ctrl") << "esc" << "in" << "/kill" << osc::EndMessage;

	*m << osc::BeginMessage("/set/main/A/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/C/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/D/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/E/fade") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/F/fade") << (float)1.0 << osc::EndMessage;

	*m << osc::BeginMessage("/set/main/A/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/C/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/D/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/E/angle") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/F/angle") << (float)0.0 << osc::EndMessage;

	*m << osc::BeginMessage("/set/main/A/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/C/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/D/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/E/scale") << (float)1.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/F/scale") << (float)1.0 << osc::EndMessage;

	*m << osc::BeginMessage("/set/main/A/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/B/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/C/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/D/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/E/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/F/sep") << (float)0.0 << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	qpmanager::add("display",m);

	qpmanager::start("C");
	qpmanager::start("V");

	usleep(500000); // a cause du status

	for(int i=0;;i++) {
		usleep(500000);
		if( !qpmanager::status("V") ) break;
	}

	qpmanager::stop("S");
	qpmanager::stop("C");
	qpmanager::stop("V");
}


int main(int argc,char *argv[]) {

char filename[100];
bool video=false;
int camnum;
int exposure;
int camType;
char monitor[100];


	readCalib();
	cout << "cam1 : " << data["CAMERA_IP_1"]<<endl;
	cout << "cam2 : " << data["CAMERA_IP_2"]<<endl;
	cout << "cam3 : " << data["CAMERA_IP_3"]<<endl;
	cout << "cam4 : " << data["CAMERA_IP_4"]<<endl;
	cout << "cam5 : " << data["CAMERA_IP_5"]<<endl;
	cout << "cam6 : " << data["CAMERA_IP_6"]<<endl;
	
	camType=CAM_PTGREY;
	filename[0]=0;
	camnum=0;
	exposure=50000;
	monitor[0]=0;

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			printf("Usage: %s [-save toto%%04d.png] [-savevideo toto.avi] [-monitor HDMI-0]  [-2prosilica|-6prosilica|-prosilica|-webcam 0|-6fisheye|-v4l2 0|-ptgrey] [-exposure 20000]\n",argv[0]);
			exit(0);
		}else if( strcmp(argv[i],"-monitor")==0 && i+1<argc ) {
			strcpy(monitor,argv[i+1]);
			i+=1;continue;
		}else if( strcmp(argv[i],"-save")==0 && i+1<argc ) {
			strcpy(filename,argv[i+1]);
			i+=1;continue;
		}else if( strcmp(argv[i],"-savevideo")==0 && i+1<argc ) {
			video=true;
			strcpy(filename,argv[i+1]);
			i+=1;continue;
		}else if( strcmp(argv[i],"-ptgrey")==0 ) {
			camType=CAM_PTGREY;
		}else if( strcmp(argv[i],"-prosilica")==0 ) {
			camType=CAM_1PROSILICA;
		}else if( strcmp(argv[i],"-2prosilica")==0 ) {
			camType=CAM_2PROSILICA;
		}else if( strcmp(argv[i],"-6prosilica")==0 ) {
			camType=CAM_6PROSILICA;
		}else if( strcmp(argv[i],"-6fisheye")==0 ) {
			camType=CAM_6FISHEYE;
		}else if( strcmp(argv[i],"-webcam")==0 && i+1<argc ) {
			camType=CAM_WEBCAM;
			camnum=atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-v4l2")==0 && i+1<argc ) {
			camType=CAM_V4L2;
			camnum=atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-exposure")==0 && i+1<argc ) {
			exposure=atoi(argv[i+1]);
			i++;continue;
		}else{
			cout << "??? " << argv[i] << endl;
			exit(0);
		}
	}

	qpmanager::initialize();


#ifdef USE_PROFILOMETRE
	profilometre_init();
#endif

	cout << "opencv version " << CV_VERSION << endl;
	switch( camType ) {
	  case CAM_1PROSILICA:
		single_camera(camType,filename[0]?filename:NULL,video,camnum,exposure);
		break;
	  case CAM_2PROSILICA:
		dual_cameras();
		break;
	  case CAM_6PROSILICA:
		six_cameras();
		break;
	  case CAM_6FISHEYE:
		six_fisheye();
		break;
	  case CAM_WEBCAM:
	  case CAM_V4L2:
		single_camera(camType,filename[0]?filename:NULL,video,camnum,exposure);
		break;
	  case CAM_PTGREY:
		dual_ptgrey(monitor);
		//single_ptgrey();
		break;
	}

#ifdef USE_PROFILOMETRE
	profilometre_dump();
#endif
}


