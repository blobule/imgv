//
// scanBleuet
//

#include "imgv/imgv.hpp"
#include "config.hpp"

#include <sys/time.h>
#include <unistd.h>

#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/pluginProsilica.hpp>
#include <imgv/pluginSave.hpp>
#include <imgv/pluginSyslog.hpp>
#include <imgv/pluginStream.hpp>

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

imqueue Qrecycle("recycle");
imqueue Qdisplay("display");
imqueue Qsave("save");
imqueue Qlog("log");
imqueue Qstream("stream");
imqueue Qstreamnet("streamnet");

pluginSyslog Syslog;
pluginProsilica C;
pluginViewerGLFW V;
pluginSave Save;
pluginStream Stream;

// video: true = save as a video, false=save images with %d in the image name
void go_camera(string id,float fps,string scanOutName,int startTime,string host, int port, int mtu) {

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
	C.name="prosilica";

	message *m=new message(1042);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 10 << 10 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << SHADER << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	Qdisplay.push_back_ctrl(m);


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
	Save.name="save";
		V.setQ("out",&Qsave); // output du viewer
		Save.setQ("in",&Qsave); // input du save
		Save.setQ("log",&Qlog); // input du save
		// mesages d'init
		m=new message(1024);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/mode") << "internal" << osc::EndMessage;
		/*
		*m << osc::BeginMessage("/filename") << filename << osc::EndMessage;
		*m << osc::BeginMessage("/count") << 10 << osc::EndMessage;
		*/
		
		*m << osc::BeginMessage("/mapview") << "/main/A/tex" << scanOutName << osc::EndMessage;
		// save une seule image
	// normalement 102, mais noir - blanc c'est tres lent...
		*m << osc::BeginMessage("/count") << "/main/A/tex" << 105 << osc::EndMessage;
	
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
		*m << osc::BeginMessage("/open") << id << "/main/A/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "StreamBytesPerSecond" << 30000000 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningHorizontal" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningVertical" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Width" << 1360 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Height" << 1024 << osc::EndMessage;
		// after "start-capture", you cant change image parameters (width, height, format,.)
		*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("Mono8") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "02-2011B-06136" << "PixelFormat" << osc::Symbol("BayerRG8") << osc::EndMessage;
		*m << osc::BeginMessage("/start-capture") << "*" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "ExposureTimeAbs" << 30000.0f << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "AcquisitionFrameRateAbs" << fps << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
		//*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
		*m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
		
		*m << osc::EndBundle;
		Qrecycle.push_back_ctrl(m);

	//
	// streaming to control outside pi
	//
	Stream.setQ("in",&Qstream);
	Stream.setQ("cmd",&Qstreamnet);
	Stream.setQ("log",&Qlog);

	Stream.start();

	m=new message(512);
	*m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/outmsg") << host << port << mtu << 0 << osc::EndMessage;
    //*m << osc::BeginMessage("/outmsgtcp") << host << 44445 << osc::EndMessage;
    //*m << osc::BeginMessage("/scale") << scale << osc::EndMessage;
    *m << osc::BeginMessage("/init-done") << osc::EndMessage;
    *m << osc::EndBundle;
    Qstream.push_back_ctrl(m);

	// send something...
	// prepare output for scan
	// on doit separer les messages.... PAS DE BUNDLE POUR LE NET!
	m=new message(512);
    *m << osc::BeginMessage("/display/shader") << 0 << "es2-simple" << osc::EndMessage;
    Qstreamnet.push_back_ctrl(m);

	m=new message(512);
    *m << osc::BeginMessage("/display/shader") << 1 << "es2-effect-nop" << osc::EndMessage;
    Qstreamnet.push_back_ctrl(m);

	m=new message(512);
    *m << osc::BeginMessage("/recycle-r/max") << (102-1) << osc::EndMessage;
    Qstreamnet.push_back_ctrl(m);

	m=new message(512);
    *m << osc::BeginMessage("/recycle-r/init-done") << osc::EndMessage;
    Qstreamnet.push_back_ctrl(m);

	m=new message(512);
    *m << osc::BeginMessage("/drip/count") << 102 << osc::EndMessage;
    Qstreamnet.push_back_ctrl(m);

	m=new message(512);
    *m << osc::BeginMessage("/drip/start") << startTime << osc::EndMessage;
    Qstreamnet.push_back_ctrl(m);

}

void start_cam() {
	message *m;
	m=new message(1024);
	*m << osc::BeginMessage("/start-acquisition") << "*" << osc::EndMessage;
	Qrecycle.push_back_ctrl(m);
}

void do_nothing() {
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
	Stream.stop();

	Syslog.stop();
}


void waitUntilTime(double target) {
    double now,delta;
    struct timeval tv;

    gettimeofday(&tv,NULL);
    now=tv.tv_sec+tv.tv_usec/1000000.0;

    delta=target-now;
    if( delta<=0.0 ) return; // we are late already

    // we have time to waste. wait!

    // wait seconds first
    while( delta>1.0 ) {
        int s=(int)delta;
        sleep(s);
        // recompute the time... (could have been waken by a signal)
        gettimeofday(&tv,NULL);
        now=tv.tv_sec+tv.tv_usec/1000000.0;
        if( target<now ) return; // we are late already
        delta=target-now;
    }
    // precise sleep
    usleep((int)(delta*1000000));

    //gettimeofday(&tv,NULL);
    // en ms
    //double diff=(tv.tv_sec*1000.0+tv.tv_usec/1000.0)-target*1000.0;
    //now=tv.tv_sec+tv.tv_usec/1000000.0;
    return;
}




int main(int argc,char *argv[]) {

char filename[100];
bool video=false;
int startOffset;
int start;
string host;
string outname;
int port;


	filename[0]=0;
	startOffset=10;
	host="192.168.5.160";
	port=44445;
	outname="scan-A-%03d.png";

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			printf("Usage: %s\n",argv[0]);
			exit(0);
		}else if( strcmp(argv[i],"-start")==0 ) {
			startOffset=atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-o")==0 && i+1<argc ) {
			outname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-host")==0 && i+2<argc ) {
			host=string(argv[i+1]);
			port=atoi(argv[i+2]);
			i+=2;continue;
		}
	}
	printf("starting in %d...\n",startOffset);

    struct timeval tv;
    gettimeofday(&tv,NULL);
    start=tv.tv_sec+startOffset;


#ifdef USE_PROFILOMETRE
	profilometre_init();
#endif

	cout << "opencv version " << CV_VERSION << endl;
	go_camera("",4.0f,outname,start,host,port,1500);

	cout << "waiting to start capture..."<<endl;
	waitUntilTime((double)start+0.125); // fps=4, ,decale 1/2 frame

	start_cam();

	do_nothing();

#ifdef USE_PROFILOMETRE
	profilometre_dump();
#endif
}


