//
// omniPlayer
// 
//
// usage: omniMontagne -seq 0 17 -data ~/Videos/udm_omnipolaire
//
// $data/%02d/ecamera%d.mp4   seq(0..17) cam(0..2)
// $data/%02d/left-map/dome%04d.png   seq(0..17) cam(0..2)
// $data/%02d/right-map/dome%04d.png   seq(0..17) cam(0..2)
//
//
// plugins used:  Ffmpeg, Drip, ViewerGLFW
//

#include <imgv/imgv.hpp>


// pour utiliser la webcam
//#define USE_WEBCAM

// release : version finale pour les projecteurs, sinon version "test laptop"
//#define RELEASE
#ifdef RELEASE
string base="/home/vision";
#else
string base="/home/roys";
#endif

#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/pluginFfmpeg.hpp>
//#include <imgv/pluginDrip.hpp>
#include <imgv/pluginDripMulti.hpp>
#include <imgv/pluginHomog.hpp>
#ifdef USE_WEBCAM
	#include <imgv/pluginWebcam.hpp>
#endif
#include <imgv/pluginSave.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <sys/time.h>

#include <stdio.h>

#include <imgv/imqueue.hpp>

#define STR_LEN 255



using namespace std;
using namespace cv;

double getTimeNow(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec+tv.tv_usec/1000000.0;
}

//
// convert a 16bit image into hi and low 8 bit images
//
void convert16toHL(Mat &A,Mat &H,Mat &L)
{
    int x,y;
    H.create(A.cols,A.rows,CV_8UC1);
    L.create(A.cols,A.rows,CV_8UC1);
    unsigned short *p=(unsigned short *)A.data;
    unsigned char *q=H.data;
    unsigned char *r=L.data;
    for(y=0;y<A.rows;y++)
        for(x=0;x<A.cols;x++,p++,q++,r++) {
            *q = (*p)>>8;
            *r = (*p)&0xff;
        }
}

//
// load une image et place la dans une queue d'images
//
void loadImg(string nom,string view,imqueue &Q)
{
    cv::Mat image=cv::imread(nom,-1);
    cout << "image "<<nom<<" has elemsize="<<image.elemSize1()<< " and "<<image.elemSize()<<endl;
    blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
    i1->view=view;
    Q.push_back(i1);
}

//
// global stuff so we dont lose them..
//
imqueue Qdrip1("drip1");
imqueue Qdrip2("drip2");
imqueue Qdrip3("drip3");
imqueue Qmsgdrip("msgdrip");
imqueue Qrecycle("recycle");
imqueue Qrecyclef1("recyclef1");
imqueue Qrecyclef2("recyclef2");
imqueue Qrecyclef3("recyclef3");
imqueue QrecycleDup("recycledup");
#ifdef USE_WEBCAM
  imqueue QrecycleCam("recyclecam");
#endif
imqueue QrecycleH("recycleH");
imqueue QmsgHL("msgHL");
imqueue QmsgHR("msgHR");
#ifdef USE_WEBCAM
  imqueue QmsgCam("msgCam");
#endif
imqueue Qdisplay("display");
imqueue Qstatus("status"); // messages de status des movies
imqueue QsaveL("saveL");
imqueue QsaveR("saveR");

//
// global plugins
//
pluginViewerGLFW V;
imqueue Qmsgf1("msgf1");
imqueue Qmsgf2("msgf2");
imqueue Qmsgf3("msgf3");

pluginFfmpeg F1;
pluginFfmpeg F2;
pluginFfmpeg F3;

pluginDripMulti DM;

// homographies left/right pour la projection
pluginHomog HL;
pluginHomog HR;

// webcam tracking
#ifdef USE_WEBCAM
  pluginWebcam W;
#endif

pluginSave SL;
pluginSave SR;

string myformat="rgb"; // "rgb" "mono"
string view1="/stereo/A/tex1";
string view2="/stereo/A/tex2";
string view3="/stereo/A/tex3";
string viewfish1="/camera/F1/tex";
string viewfish2="/camera/F2/tex";
string viewfish3="/camera/F3/tex";

//
// complete viewer
//
// <Qrecycle> -> [ffmpeg1] -> <Qdrip1> -> [drip1] -> <Qdisplay> -> [viewerGLFW]
//            -> [ffmpeg2] -> <Qdrip2> -> [drip2] ->
//            -> [ffmpeg3] -> <Qdrip3> -> [drip3] ->
//
void initialSetup() {
    char filename[STR_LEN];

    // groupe les queues d'input de drip1, 2, et 3
	// on ne sync plus msgdrip avec les input... c'est separe maintenant.
	// ca veut dire qu'on ne doit avoir que des images dans les drip1,2,3

    // ajoute quelques images a recycler
    for(int i=0;i<30;i++) {
        blob *image=new blob();
        image->set(i);
        image->setRecycleQueue(&Qrecyclef1);
        Qrecyclef1.push_back(image);

        image=new blob();
        image->set(i);
        image->setRecycleQueue(&Qrecyclef2);
        Qrecyclef2.push_back(image);

        image=new blob();
        image->set(i);
        image->setRecycleQueue(&Qrecyclef3);
        Qrecyclef3.push_back(image);
    }
    for(int i=0;i<30;i++) {
        blob *image=new blob();
        image->set(i);
        image->setRecycleQueue(&Qrecycle);
        Qrecycle.push_back(image);
    }
/*
    for(int i=0;i<60+10;i++) {
        blob *image=new blob();
        image->set(i);
        image->setRecycleQueue(&QrecycleDup);
        QrecycleDup.push_back(image);
    }
*/
    for(int i=0;i<5;i++) {
        blob *image=new blob();
        image->set(i);
        image->setRecycleQueue(&QrecycleH);
        QrecycleH.push_back(image);
    }
#ifdef USE_WEBCAM
    for(int i=0;i<5;i++) {
        blob *image=new blob();
        image->set(i);
        image->setRecycleQueue(&QrecycleCam);
        QrecycleCam.push_back(image);
    }
#endif



    //
    // plugin FFMPEG
    //

    //
    // plugin viewer
    //

    message *m;

    m=new message(1942);
    *m << osc::BeginBundleImmediate;
    //*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
#ifdef RELEASE
        *m << osc::BeginMessage("/new-window/stereo") << 1280 << 0 << 3840 << 1080 << false << "Left eye" << osc::EndMessage;
#else
        *m << osc::BeginMessage("/new-window/stereo") << 640 << 0 << 1280 << 720 << false << "Stereo eye" << osc::EndMessage;
#endif
	// 640+640
#ifdef RELEASE
        *m << osc::BeginMessage("/new-window/camera") << 0 << 64 << 1280 << 960 << false << "Camera" << osc::EndMessage;
#else
        *m << osc::BeginMessage("/new-window/camera") << 0 << 64 << 640 << 480 << false << "Camera" << osc::EndMessage;
#endif
        *m << osc::BeginMessage("/new-quad/stereo/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "omni2" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/camera/A") << 0.0f << 0.5f << 0.5f << 1.0f << -10.0f << "normal" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/camera/F1") << 0.5f << 1.0f << 0.0f << 0.5f << -10.0f << "A" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/camera/F2") << 0.0f << 0.5f << 0.0f << 0.5f << -10.0f << "A" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/camera/F3") << 0.5f << 1.0f << 0.5f << 1.0f << -10.0f << "A" << osc::EndMessage;
	// pour le fade
	*m << osc::BeginMessage("/set/stereo/A/fade") << 1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/set/stereo/A/eye") << 2 << osc::EndMessage; // 0=left,1=right,2=stereo
        *m << osc::BeginMessage("/defevent") << 0 << 0 << "/set/stereo/A/yawparam" << osc::Symbol("$xr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << 0 << 0 << "/set/stereo/A/pitchparam" << osc::Symbol("$yr") << osc::EndMessage;
	// pour les homographies (output queue is 4,5 pour L,R)
        *m << osc::BeginMessage("/defevent") << '1' << 4 << "/set/q" << 0 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << '2' << 4 << "/set/q" << 1 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << '3' << 4 << "/set/q" << 2 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << '4' << 4 << "/set/q" << 3 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << '5' << 5 << "/set/q" << 0 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << '6' << 5 << "/set/q" << 1 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << '7' << 5 << "/set/q" << 2 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << '8' << 5 << "/set/q" << 3 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	// pour la webcam
	*m << osc::BeginMessage("/defevent") << 'A' << 6 << "/line/begin" << osc::Symbol("$x") << osc::Symbol("$y") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent") << 'B' << 6 << "/line/end" << osc::Symbol("$x") << osc::Symbol("$y") << osc::EndMessage;
	// next movie!
	*m << osc::BeginMessage("/defevent") << ' ' << 7 << "/set/length" << 30 << osc::EndMessage;
	*m << osc::BeginMessage("/defevent") << ' ' << 8 << "/set/length" << 30 << osc::EndMessage;
	*m << osc::BeginMessage("/defevent") << ' ' << 9 << "/set/length" << 30 << osc::EndMessage;

    *m << osc::EndBundle;
    // add message to Qdisplay, so the viewer can get it only after it is started.
    Qdisplay.push_back(m);



    //
    // plugin Drip
    //
    // to sync, we must provide a time...
    int begin=(int)(getTimeNow())+5;

    m=new message(1724);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/fps") << 30.0 << osc::EndMessage;
    *m << osc::BeginMessage("/start") << begin << osc::EndMessage; // end with start
    // on definit ici le fade (une seule fois) parce que defevent additionne les event...
    *m << osc::BeginMessage("/defevent") << -1 << 5 << "/set/stereo/A/fade" << osc::Symbol("$xr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent") << -2 << 5 << "/set/stereo/A/fade" << osc::Symbol("$xr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent") << -1 << 5 << "/set/camera/F1/fade" << osc::Symbol("$xr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent") << -2 << 5 << "/set/camera/F1/fade" << osc::Symbol("$xr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent") << -1 << 5 << "/set/camera/F2/fade" << osc::Symbol("$xr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent") << -2 << 5 << "/set/camera/F2/fade" << osc::Symbol("$xr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent") << -1 << 5 << "/set/camera/F3/fade" << osc::Symbol("$xr") << osc::EndMessage;
    *m << osc::BeginMessage("/defevent") << -2 << 5 << "/set/camera/F3/fade" << osc::Symbol("$xr") << osc::EndMessage;
	//
	// connection des input et output
	//
    *m << osc::BeginMessage("/in") << 2 << "f1" << osc::EndMessage;
    *m << osc::BeginMessage("/in") << 3 << "f2" << osc::EndMessage;
    *m << osc::BeginMessage("/in") << 4 << "f3" << osc::EndMessage;
    *m << osc::BeginMessage("/out") << "f1" << 5 << view1 << osc::EndMessage;
    *m << osc::BeginMessage("/out") << "f2" << 5 << view2 << osc::EndMessage;
    *m << osc::BeginMessage("/out") << "f3" << 5 << view3 << osc::EndMessage;
// pour avoir les images fisheye visibles en "moniteur"
/*
    *m << osc::BeginMessage("/dup") << "f1" << 5 << viewfish1 << osc::EndMessage;
    *m << osc::BeginMessage("/dup") << "f2" << 5 << viewfish2 << osc::EndMessage;
    *m << osc::BeginMessage("/dup") << "f3" << 5 << viewfish3 << osc::EndMessage;
*/
    *m << osc::BeginMessage("/sync") << true << osc::EndMessage;
	// tell drip to trigger events -1 and -2 at start and end of input "f1"
    *m << osc::BeginMessage("/event/set/begin") << -1 << "f1" << 30 << osc::EndMessage;
    *m << osc::BeginMessage("/event/set/end") << -2 << "f1" << 30 << osc::EndMessage;
    *m << osc::EndBundle;
    Qmsgdrip.push_back(m);

    //
    // demande a F1 d'avertir quand le movie est fini. 3 = Qstatus
    //
    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/defevent") << -1 << 3 << "/movie-done" << osc::EndMessage;
    *m << osc::BeginMessage("/max-fps") << 40.0f << osc::EndMessage; // un peu plus que 30
    *m << osc::EndBundle;
    Qmsgf1.push_back(m);

    m=new message(256);
    *m << osc::BeginMessage("/max-fps") << 40.0f << osc::EndMessage; // un peu plus que 30
    Qmsgf2.push_back(m);

    m=new message(256);
    *m << osc::BeginMessage("/max-fps") << 40.0f << osc::EndMessage; // un peu plus que 30
    Qmsgf3.push_back(m);

    //
    // Homographie
    // genere une lut sur demande
    //

    // set la view pour l'image output
    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/set/view")<<"/stereo/A/homogL"<<osc::EndMessage;
    *m << osc::BeginMessage("/set/q") << 0 << 0.5f << 0.0f << osc::EndMessage;
    *m << osc::BeginMessage("/set/q") << 1 << 1.0f << 0.0f << osc::EndMessage;
    *m << osc::BeginMessage("/set/q") << 2 << 1.0f << 1.0f << osc::EndMessage;
    *m << osc::BeginMessage("/set/q") << 3 << 0.5f << 1.0f << osc::EndMessage;
    *m << osc::BeginMessage("/go")<<osc::EndMessage; // generate first "full" view
    *m << osc::EndBundle;
    QmsgHL.push_back(m);
    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/set/view")<<"/stereo/A/homogR"<<osc::EndMessage;
    *m << osc::BeginMessage("/set/q") << 0 << 0.0f << 0.0f << osc::EndMessage;
    *m << osc::BeginMessage("/set/q") << 1 << 0.5f << 0.0f << osc::EndMessage;
    *m << osc::BeginMessage("/set/q") << 2 << 0.5f << 1.0f << osc::EndMessage;
    *m << osc::BeginMessage("/set/q") << 3 << 0.0f << 1.0f << osc::EndMessage;
    *m << osc::BeginMessage("/go")<<osc::EndMessage; // generate first "full" view
    *m << osc::EndBundle;
    QmsgHR.push_back(m);

    //
    // controle webcam
    //
#ifdef USE_WEBCAM
    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/cam") << 0 << "/camera/A/tex" << osc::EndMessage;
    *m << osc::BeginMessage("/defevent") << -1 << 1 << "/set/stereo/A/yawparam" << osc::Symbol("$xr") << osc::EndMessage;
    *m << osc::EndBundle;
    QmsgCam.push_back(m);
#endif

    //
    // save des homographies
    //
    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/mode")<<"internal"<<osc::EndMessage;
    *m << osc::BeginMessage("/filename")<<base+"/homoL.png"<<osc::EndMessage;
    *m << osc::EndBundle;
    QsaveL.push_back(m);

    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/mode")<<"internal"<<osc::EndMessage;
    *m << osc::BeginMessage("/filename")<<base+"/homoR.png"<<osc::EndMessage;
    *m << osc::EndBundle;
    QsaveR.push_back(m);


    //
    // connexions entre les plugins (par les imqueues)
    //
	F1.reservePort("tostatus");
	F1.reservePort("msg");
	F2.reservePort("msg");
	F3.reservePort("msg");

    // ffmpeg
    F1.setQ("in",&Qrecyclef1);
    F1.setQ("out",&Qdrip1);
    F1.setQ("msg",&Qmsgf1);
    F1.setQ("tostatus",&Qstatus); // messages de status des movies. F1 suffit.

    F2.setQ("in",&Qrecyclef2);
    F2.setQ("out",&Qdrip2);
    F2.setQ("msg",&Qmgf2);

    F3.setQ("in",&Qrecyclef3);
    F3.setQ("out",&Qdrip3);
    F3.setQ("msg",&Qmsgf3);

    // drip
    DM.setQ(0,&Qmsgdrip);
	// /dup n'utilise pas de queue de recyclage.
    //DM.setQ(1,&QrecycleDup); // source d'image pour out si plus qu'un out ...
	//  /in 2 f1;  /in 3 f2; /in 4 f3
	// /out 5 f1 /mmm;  /out 5 f2 /,,,; /out 5 f3 /...;
	// /dup 5 f1 /,,,; /dup 5 f2

	//DM.reservePorts("f",...)
    DM.setQ(2,&Qdrip1); // input f1
    DM.setQ(3,&Qdrip2); // input f2
    DM.setQ(4,&Qdrip3); // input f3
    DM.setQ(5,&Qdisplay); // output for all

    // homographies
    HL.setQ(0,&QrecycleH);
    HL.setQ(1,&QsaveL);
    HL.setQ(2,&QmsgHL);

    SL.setQ(0,&QsaveL);
    SL.setQ(1,&Qdisplay);


    HR.setQ(0,&QrecycleH);
    HR.setQ(1,&QsaveR);
    HR.setQ(2,&QmsgHR);

    SR.setQ(0,&QsaveR);
    SR.setQ(1,&Qdisplay);

    // viewer
    V.setQ(0,&Qdisplay);
    V.setQ(4,&QmsgHL);
    V.setQ(5,&QmsgHR);
#ifdef USE_WEBCAM
    V.setQ(6,&QmsgCam);
#endif
    V.setQ(7,&Qmsgf1);
    V.setQ(8,&Qmsgf2);
    V.setQ(9,&Qmsgf3);

	// webcam
#ifdef USE_WEBCAM
	W.setQ(0,&QrecycleCam);
	W.setQ(1,&Qdisplay);
	W.setQ(2,&QmsgCam);
#endif

// homographie de depart
	loadImg(base+"/homoL-ref.png","/stereo/A/homogL",Qdisplay);
	loadImg(base+"/homoR-ref.png","/stereo/A/homogR",Qdisplay);

    //
    // mode full automatique
    //
    DM.start();
    V.start();

    F1.start();
    F2.start();
    F3.start();


    HL.start();
    HR.start();

    SL.start();
    SR.start();

#ifdef USE_WEBCAM
    W.start();
#endif

}



void cleanupSetup() {
    V.stop();
#ifdef USE_WEBCAM
    W.stop();
#endif
    SL.stop();
    SR.stop();
    HL.stop();
    HR.stop();
    DM.stop();
    F3.stop();
    F2.stop();
    F1.stop();
}

// retourne 0 si ok (movie ended natturaly), return -1 si some plugin killed
int startMovie(int seq,string camName,string lutleftName,string lutrightName) {

static float fovomni[18]={
172.0, 174.0, 174.0, 174.0, 174.0,
174.0, 174.0, 174.0, 174.0, 174.0,
174.0, 174.0, 174.0, 174.0, 174.0,
160.0, 178.0, 176.0,
};

    //
    // images fixes (LUT)
    //
    char fname[500];

        sprintf(fname,lutleftName.c_str(),seq,0); cout << "LUT LEFT "<<fname<<endl;
        loadImg(fname,"/stereo/A/lutleft0",Qdisplay);
        sprintf(fname,lutleftName.c_str(),seq,1); cout << "LUT LEFT "<<fname<<endl;
        loadImg(fname,"/stereo/A/lutleft1",Qdisplay);
        sprintf(fname,lutleftName.c_str(),seq,2); cout << "LUT LEFT "<<fname<<endl;
        loadImg(fname,"/stereo/A/lutleft2",Qdisplay);

        sprintf(fname,lutrightName.c_str(),seq,0);cout << "LUT RIGHT "<<fname<<endl;
        loadImg(fname,"/stereo/A/lutright0",Qdisplay);
        sprintf(fname,lutrightName.c_str(),seq,1);cout << "LUT RIGHT "<<fname<<endl;
        loadImg(fname,"/stereo/A/lutright1",Qdisplay);
        sprintf(fname,lutrightName.c_str(),seq,2);cout << "LUT RIGHT "<<fname<<endl;
        loadImg(fname,"/stereo/A/lutright2",Qdisplay);

	//
	// fovomni
	//
    message *m;
    if( seq>=0 && seq<=17 ) {
	m=new message(256);
	*m << osc::BeginMessage("/set/stereo/A/fovomni")<<(float)(fovomni[seq]*M_PI/180.0 )<<osc::EndMessage;
	Qdisplay.push_back(m);
    }

	//
	// les movies eux-meme
	//

    sprintf(fname,camName.c_str(),seq,0);cout << "CAM "<<fname<<endl;
    m=new message(256);
    *m << osc::BeginMessage("/start") << fname << "" << myformat << view1 << 0 << osc::EndMessage;
    Qmsgf1.push_back(m);

    sprintf(fname,camName.c_str(),seq,1);cout << "CAM "<<fname<<endl;

    m=new message(256);
    *m << osc::BeginMessage("/start") << fname << "" << myformat << view2 << 0 << osc::EndMessage;
    Qmsgf2.push_back(m);

    sprintf(fname,camName.c_str(),seq,2);cout << "CAM "<<fname<<endl;
    m=new message(256);
    *m << osc::BeginMessage("/start") << fname << "" << myformat << view3 << 0 << osc::EndMessage;
    Qmsgf3.push_back(m);

    // rien a faire...
    for(int h=0;;h++) {
         // check Qstatus. Is the movie done?
         message *m=Qstatus.pop_front_nowait_msg();
         if( m!=NULL ) {
             m->decode();
             for(int i=0;i<m->msgs.size();i++) {
                  cout << "DECODE " << m->msgs[i] << endl;
                  const char *address=m->msgs[i].AddressPattern();
                  if( oscutils::endsWith(address,"/movie-done") ) {
                         cout << "Movie done. Next!"<<endl;
             		 recyclable::recycle((recyclable **)&m);
                         return(0); // end of movie. ready for next
			// on devrait peut etre flusher les autres films...
			// on pourrait avoir une difference de frames...
                  }
             }
             recyclable::recycle((recyclable **)&m);
         }
         usleep(200000);

        if( !F1.status() ) break;
        if( !F2.status() ) break;
        if( !F3.status() ) break;
        if( !DM.status() ) break;
        if( !V.status() ) break;
    }
    return(-1); // end for real
}


// usage: omniMontagne -seq 0 17 -data ~/Videos/udm_omnipolaire
int main(int argc,char *argv[]) {
    int seqmin;
    int seqmax;
    string dataPrefix;
    string lutleftName;
    string lutrightName;
    string camName;

    cout << "omniMontagne (imgv version " << IMGV_VERSION << ")"<<endl;

    seqmin=0;
    seqmax=17;
#ifdef RELEASE
    dataPrefix="/home/vision/udm_omnipolaire";
#else
    dataPrefix="/home/roys/Videos/udm_omnipolaire";
#endif
    lutleftName="/%02d/left-map/dome%04d.png";
    lutrightName="/%02d/right-map/dome%04d.png";
    camName="/%02d/ecamera%d.mp4";
    for(int i=1;i<argc;i++) {
        if( strcmp(argv[i],"-h")==0 ) {
            cout << argv[0] << " blub"<<endl;
            exit(0);
        }
        if( strcmp(argv[i],"-seq")==0 && i+2<argc ) {
            seqmin=atoi(argv[i+1]);
            seqmax=atoi(argv[i+2]);
            i+=2;continue;
	}
        if( strcmp(argv[i],"-prefix")==0 && i+1<argc ) {
            dataPrefix=string(argv[i+1]);
            i++;continue;
        }
        if( strcmp(argv[i],"-cam")==0 && i+1<argc ) {
            camName=string(argv[i+1]);
            i++;continue;
        }
        if( strcmp(argv[i],"-lutleft")==0 && i+1<argc ) {
            lutleftName=string(argv[i+1]);
            i++;continue;
        }
        if( strcmp(argv[i],"-lutright")==0 && i+1<argc ) {
            lutrightName=string(argv[i+1]);
            i++;continue;
        }
    }
    lutleftName=dataPrefix+lutleftName;
    lutrightName=dataPrefix+lutrightName;
    camName=dataPrefix+camName;

    initialSetup();

    int seq;
    //for(seq=seqmin;seq<=seqmax;seq++) {
    for(int s=1;;s++) {
	seq=(s%(seqmax-seqmin+1)+seqmin);
	if( startMovie(seq,camName,lutleftName,lutrightName) ) break;
    }

    cleanupSetup();

    //go(style,camName,lutleftName,lutrightName);
}


