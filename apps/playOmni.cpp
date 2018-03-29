//
// playMovie
//
// example for sample plugins
//
// plugins used:  Ffmpeg, Drip, ViewerGLFW
//

#include "imgv.hpp"
#include "config.hpp"


#include <imgv/pluginViewerGLFW.hpp>
	#include <imgv/pluginProsilica.hpp>
#include <imgv/pluginSave.hpp>
#include <imgv/pluginSyslog.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

#include <imgv/imqueue.hpp>
//#include <imgv/config.hpp>


using namespace std;
using namespace cv;

//#define USE_PROFILOMETRE
#ifdef USE_PROFILOMETRE
 #include <profilometre.h>
#endif

#define SHADER "normal"
// ajoute un param blub (int, 0..10000)
//#define SHADER "B"

//
// ports for defevents callback
#define DIALOG_CAM	"dialog-cam"


// video: true = save as a video, false=save images with %d in the image name
// prosilica=false -> use webcam (camnum donne le # de camera)
void three_camera(string id1,string id2,string id3,float exposure,float fps) {
	imqueue Qrecycle("recycle");
	imqueue Qdisplay("display");
	imqueue Qsave("save");
	imqueue Qlog("log");

	pluginSyslog Syslog;
	Syslog.setQ("in",&Qlog);
	Syslog.start(); // start now so we can output logs

	// ajoute quelques images a recycler
	for(int i=0;i<20;i++) {
		blob *image=new blob();
		image->n=i;
		image->setRecycleQueue(&Qrecycle);
		Qrecycle.push_front(image);
	}

	//
	// PLUGIN: Prosilica
	//
	pluginProsilica C;
	C.name="prosilica";

	message *m=new message(1042);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 10 << 10 << 1360 << 341 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 0.33f << 0.0f << 1.0f << -10.0f << SHADER << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/B") << 0.33f << 0.66f << 0.0f << 1.0f << -10.0f << SHADER << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/C") << 0.66f << 1.0f << 0.0f << 1.0f << -10.0f << SHADER << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'A' << DIALOG_CAM << "/set" << "" << "AcquisitionFrameRateAbs" << 15.0f << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'B' << DIALOG_CAM << "/set" << "" << "AcquisitionFrameRateAbs" << 1.0f << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'C' << DIALOG_CAM << "/set" << "" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'D' << DIALOG_CAM << "/set" << "" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
		*m << osc::BeginMessage("/defevent-ctrl") << 'Z' << DIALOG_CAM << "/set" << "" << "TriggerSoftware" << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	Qdisplay.push_back_ctrl(m);



	pluginViewerGLFW V;
	V.name="viewer";
	V.reservePort(DIALOG_CAM);

	//
	// connexions entre les plugins (par les rqueues)
	//
	// on peut utiliser directement le # de port,
	// ou donne un nom de port (varie selon le plugin)

	// camera
		C.setQ("in",&Qrecycle);
		C.setQ("out",&Qdisplay);
		C.setQ("log",&Qlog);

	// viewer
	V.setQ("in",&Qdisplay);
	V.setQ("log",&Qlog);
	V.setQ(DIALOG_CAM,&Qrecycle); // a quoi ca sert? pour parler a la camera.. anonyme, non?
	//C.setQ(2,&Qmsgffmpeg); // pour parler au ffmpeg
	//C.setQ(3,&Qmsgdrip); // pour parler au drip

	// le save!
	pluginSave Save;
	Save.name="save";
		V.setQ("out",&Qsave); // output du viewer
		Save.setQ("in",&Qsave); // input du save
		Save.setQ("log",&Qlog); // input du save
		// mesages d'init
		m=new message(1024);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/mode") << /*"nop"*/ "id" << osc::EndMessage;
		/*
		*m << osc::BeginMessage("/filename") << filename << osc::EndMessage;
		*m << osc::BeginMessage("/count") << 10 << osc::EndMessage;
		*/
		
		*m << osc::BeginMessage("/mapview") << "/main/A/tex" << "toto0-%02d.png" << osc::EndMessage;
		*m << osc::BeginMessage("/mapview") << "/main/B/tex" << "toto1-%02d.png" << osc::EndMessage;
		*m << osc::BeginMessage("/mapview") << "/main/C/tex" << "toto2-%02d.png" << osc::EndMessage;
		// save une seule image
		//*m << osc::BeginMessage("/count") << "/main/A/tex" << 20 << osc::EndMessage;
		//*m << osc::BeginMessage("/count") << "/main/B/tex" << 20 << osc::EndMessage;
		//*m << osc::BeginMessage("/count") << "/main/C/tex" << 20 << osc::EndMessage;
	
		*m << osc::BeginMessage("/init-done") << osc::EndMessage;
		*m << osc::EndBundle;
		Qsave.push_back_ctrl(m);
		Save.start();

	//
	// mode full automatique
	//

	C.start();

	V.start();

	m=new message(1024);
		*m << osc::BeginBundleImmediate;
		// false: high performance... dont loose frames. use all mem if needed.
		// true: low performance... loose frame if out of recycle frames
		*m << osc::BeginMessage("/wait-for-recycle") << true << osc::EndMessage;
		*m << osc::BeginMessage("/open") << id1 << "/main/A/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/open") << id2 << "/main/B/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/open") << id3 << "/main/C/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningHorizontal" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningVertical" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Width" << 1360 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Height" << 1024 << osc::EndMessage;
		// after "start-capture", you cant change image parameters (width, height, format,.)
		*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("Mono8") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "02-2011B-06136" << "PixelFormat" << osc::Symbol("BayerRG8") << osc::EndMessage;
		*m << osc::BeginMessage("/start-capture") << "*" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "ExposureTimeAbs" << exposure << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "AcquisitionFrameRateAbs" << fps << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "StreamBytesPerSecond" << 20000000 << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
		*m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
		*m << osc::BeginMessage("/start-acquisition") << "*" << osc::EndMessage;
		
		*m << osc::EndBundle;
	Qrecycle.push_back_ctrl(m);



	// rien a faire pendant 20*2 secondes...
	for(int i=0;;i++) {
		usleep(10000);
		//sleep(1);
		if( !C.status() ) break;
		if( !V.status() ) break;
	}

	Save.stop();
		C.stop();
	V.stop();

	Syslog.stop();
}


int main(int argc,char *argv[]) {

char filename[100];
bool video=false;
int exposure;
float fps;


	filename[0]=0;
	exposure=4000;
	fps=2.0f;

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			printf("Usage: %s [-exp 4000]\n",argv[0]);
			exit(0);
		}
		if( strcmp(argv[i],"-exp")==0 && i+1<argc ) {
			exposure=atoi(argv[i+1]);
			i++;continue;
		}
		if( strcmp(argv[i],"-fps")==0 && i+1<argc ) {
			fps=atof(argv[i+1]);
			i++;continue;
		}
	}

#ifdef USE_PROFILOMETRE
	profilometre_init();
#endif

	cout << "opencv version " << CV_VERSION << endl;
	//dual_cameras();
	//three_camera("02-2140A-06133","02-2140A-06134","02-2140A-06663");
	three_camera("02-2140A-06133","02-2140A-06663","02-2140A-06134",(float)exposure,fps);

#ifdef USE_PROFILOMETRE
	profilometre_dump();
#endif
}


