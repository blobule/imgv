//
// scanImages
//
// projete des patterns, prend des photo, et sauvegarde le resultat.
//
// plugins used:  pattern, sync, prosilica, ViewerGLFW, save
//

#include "imgv/imgv.hpp"

#include <imgv/pluginSave.hpp>
#include <imgv/pluginSync.hpp>
#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/pluginPattern.hpp>
#include <imgv/pluginRead.hpp>
#include <imgv/pluginProsilica.hpp>
#include <oscpack/osc/OscTypes.h>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

#include <imgv/imqueue.hpp>


using namespace std;
using namespace cv;

//#define USE_PROFILOMETRE
#include <profilometre.h>

//
// [recycle1] ->     cam --> Save --> count
// [recycle2] -> pattern -[Qsyncpat]-> Sync1 --> viewer -> Save -> count
//
// on envoi /go au sync. On ramasse les imageSync.
// cam doit sugnaler au sync que l'image est prise.
//

void afficheStats(vector<imqueue *>VQ) {
	int *sz = new int[VQ.size()];
	for(int i=0;i<VQ.size();i++) sz[i]=VQ[i]->size();

	cout << "--- Queue stats ---"<<endl;
	for(int i=0;i<VQ.size();i++) cout << "Queue "<<setw(30)<<VQ[i]->name<< " is "<<sz[i]<<endl;
	cout << "-----------------------";
	delete sz;
}

void go(char *inFilename,char *outFilename,int nbpat,float delai) {
	imqueue Qrecyclecam("recyclecam");
	imqueue Qrecyclepat("recyclepat");
	imqueue Qdisplay("display");
	imqueue Qsavecam("savecam");
	imqueue Qsync("sync");
	imqueue Qmain("main");

	pluginProsilica C;
	pluginViewerGLFW V;
	pluginRead P;
	pluginSave Save;
	pluginSync Sync;

	// ajoute quelques images a recycler
	for(int i=0;i<100;i++) {
		blob *image=new blob();
		image->setRecycleQueue(&Qrecyclecam);
		Qrecyclecam.push_front(image);
	}
    // une image pour ralentir le read (on pourrait utiliser un drip)
	for(int i=0;i<1;i++) {
		blob *image=new blob();
		image->setRecycleQueue(&Qrecyclepat);
		Qrecyclepat.push_front(image);
	}

	//
	// connecte les queues avec les plugins
	//

	// camera
	C.setQ("in",&Qrecyclecam);
	C.setQ("out",&Qsavecam);

	// pattern
	P.setQ("in",&Qrecyclepat);
	P.setQ("out",&Qdisplay);

	// sync
	Sync.setQ("in",&Qsync);
    Sync.reservePort("tocam",true);
	Sync.setQ("tocam",&Qrecyclecam);

	// viewer
	V.setQ("in",&Qdisplay);
	V.setQ("out",&Qsync);

	// Save
	Save.setQ("in",&Qsavecam);
    Save.reservePort("tomain",false);
 	Save.setQ("tomain",&Qmain);

	//
	// camera
	//

	message *m=new message(1024);
        *m << osc::BeginBundleImmediate;
	// pour avertir le sync pattern que la photo est prise. (-1=photo done)
	//*m << osc::BeginMessage("/defevent") << -1 << 3 << "/delay" << delai << osc::EndMessage;
	//*m << osc::BeginMessage("/defevent") << -1 << 3 << "/go" << osc::EndMessage;
        *m << osc::BeginMessage("/open") << "" << "/main/A/tex" << osc::EndMessage; //le view ne sert a rien
        //*m << osc::BeginMessage("/set") << "" << "Width" << 1360 << osc::EndMessage;
        //*m << osc::BeginMessage("/set") << "" << "Height" << 1024 << osc::EndMessage;
	// after "start-capture", you cant change image parameters (width, height, format,.)
        *m << osc::BeginMessage("/set") << "" << "BandwidthControlMode" << osc::Symbol("StreamBytesPerSecond") << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "StreamBytesPerSecond" << 50000000 << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "StreamFrameRateConstrain" << true << osc::EndMessage;
        float exposure=(float)(1000000.0/30.0); //exposure in microseconds
        *m << osc::BeginMessage("/set") << "" << "ExposureTimeAbs" << exposure << osc::EndMessage;
        *m << osc::BeginMessage("/start-capture") << "" << osc::EndMessage;
/*
        *m << osc::BeginMessage("/set") << "" << "AcquisitionFrameRateAbs" << 100.0f << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
*/
        *m << osc::BeginMessage("/manual") << "" << osc::EndMessage;
        //*m << osc::BeginMessage("/auto") << "" << 10.0f << osc::EndMessage;
        *m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
        *m << osc::BeginMessage("/start-acquisition") << "" << osc::EndMessage;
        *m << osc::EndBundle;
        Qrecyclecam.push_back_ctrl(m);
    C.start();
    while(!Qrecyclecam.empty_ctrl()) usleep(1000);
  
	//
	// save
	//
	m=new message(1024);
    *m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/mode") << "internal" /*"id"*/ << osc::EndMessage;
	*m << osc::BeginMessage("/filename") << outFilename << osc::EndMessage;
	*m << osc::BeginMessage("/count") << nbpat << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "done" << "tomain" << "/fini" << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
	Qsavecam.push_back_ctrl(m);
    Save.start();
    while(!Qsavecam.empty_ctrl()) usleep(1000);

	//
	// sync
	//
	m=new message(1024);
        *m << osc::BeginBundleImmediate;
	// manual since we will wait for '/go' from camera to send next pattern
    //    *m << osc::BeginMessage("/manual") <<osc::EndMessage;
        //*m << osc::BeginMessage("/delay") << delai << osc::EndMessage;
	// event -2 happen "delai"  after an image is sent. -1 is for no delay
	// tell camera to take picture
	*m << osc::BeginMessage("/defevent-ctrl") << "delay" << "tocam" << "/set" << "*" << "TriggerSoftware" << osc::EndMessage;
        *m << osc::BeginMessage("/delay") << delai << osc::EndMessage;
        *m << osc::BeginMessage("/auto") << osc::EndMessage;
        //*m << osc::BeginMessage("/go") <<osc::EndMessage;
        *m << osc::EndBundle;
        Qsync.push_back_ctrl(m);
    Sync.start();
    while(!Qsync.empty_ctrl()) usleep(1000);

	//
	// Viewer
	//
	m=new message(1024);
	*m << osc::BeginBundleImmediate;

	*m << osc::BeginMessage("/new-window/main") << 1920 << 0 << 800 << 600 << false << "scanImages" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::EndBundle;
	Qdisplay.push_back_ctrl(m);
    V.start();
    while(!Qdisplay.empty_ctrl()) usleep(1000);
   
    sleep(1);

    //first image is displayed too quickly
	//blob *image=new blob();
	//image->setRecycleQueue(&Qrecyclepat);
	//Qdisplay.push_back(image);
    //while(!Qdisplay.empty()) usleep(1000);

    //sleep(1);

	//
	// patterns (plugin to read files)
	//
	m=new message(1024);
	*m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/view") << "/main/A/tex" <<osc::EndMessage;
    *m << osc::BeginMessage("/file") << inFilename << osc::EndMessage;
    *m << osc::BeginMessage("/play") << osc::EndMessage;
    *m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
    Qrecyclepat.push_back_ctrl(m);
    P.start();
    while(!Qrecyclepat.empty_ctrl()) usleep(1000);

	// rien a faire ...
	for(int i=0;;i++) {
		sleep(1);
        if (Qmain.size_ctrl()>0) break;
        //if (Save.getIndex()==nbpat) break;
		if( !V.status() ) break;
	}
	// on doit attendre que tout soit sauvegarde....
	cout << "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZzzzzZzzzz"<<endl;
	
	C.stop();
	P.stop();
	V.stop();
	Sync.stop();
	Save.stop();
}

int main(int argc,char *argv[]) {
    int i;
    char inFilename[200];
    char outFilename[200];
    int nbpat=1;
    float delay=0.1;

    inFilename[0]='\0';
    outFilename[0]='\0';

    for(int i=1;i<argc;i++) {
        if( strcmp("-i",argv[i])==0 && i+1<argc ) {
            strcpy(inFilename,argv[i+1]);
            i++;continue;
        }
        else if( strcmp(argv[i],"-o")==0 && i+1<argc ) {
            strcpy(outFilename,argv[i+1]);
            i++;continue;
        }
        else if( strcmp(argv[i],"-n")==0 && i+1<argc ) {
            nbpat=atoi(argv[i+1]);
            i++;continue;
        }
        else if( strcmp(argv[i],"-delay")==0 && i+1<argc ) {
            delay=atof(argv[i+1]);
            i++;continue;
        }
    }

	profilometre_init();

	cout << "opencv version " << CV_VERSION << endl;
	go(inFilename,outFilename,nbpat,delay);

	profilometre_dump();
}


