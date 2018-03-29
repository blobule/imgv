//
// omniPlayerLive
//
// usage: omniPlayerLive -style [separate|mixrgb|omni] -cam v1.mp4 v2.mp4 v3.mp4 -luts map/left/dome.png map/right/dome.png
//
// omniPlayerLive -style omni -cam 00.mp4 01.mp4 02.mp4 -lutleft left/dome%04d.png -lutright right/dome%04d.png
//
//
// omniPlayerLive -style separate -cam 00.mp4 01.mp4 02.mp4 
//
//
// plugins used:  Ffmpeg, Drip, ViewerGLFW
//

#include <imgv/imgv.hpp>

#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/pluginFfmpeg.hpp>
#include <imgv/pluginDrip.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include <vector>

#include <sys/time.h>

#include <imgv/imqueue.hpp>

#define STR_LEN 255

// styles d'affichage

#define STYLE_SEPARATE	0
#define STYLE_MIXRGB	1
#define STYLE_OMNI	2

using namespace std;

//#undef HAVE_OCULUS

#ifdef HAVE_OCULUS
  #include "OVR.h"
  using namespace OVR;
#endif

double getTimeNow(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec+tv.tv_usec/1000000.0;
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
    Q.push_front(i1);
}


//
// complete viewer
//
// <Qrecycle> -> [ffmpeg1] -> <Qdrip1> -> [drip1] -> <Qdisplay> -> [viewerGLFW]
//            -> [ffmpeg2] -> <Qdrip2> -> [drip2] ->
//            -> [ffmpeg3] -> <Qdrip3> -> [drip3] ->
//
void go(int style,char *cam,char *lutleft, char *lutright) {
    char filename[STR_LEN];
    imqueue Qrecycle("recycle");

    imqueue Qdrip1("drip1");
    imqueue Qdrip2("drip2");
    imqueue Qdrip3("drip3");
    imqueue Qmsgdrip1("msgdrip1");
    imqueue Qmsgdrip2("msgdrip2");
    imqueue Qmsgdrip3("msgdrip3");

    imqueue Qdisplay("display");

    // groupe les queues d'input de drip1, 2, et 3
    Qmsgdrip1.syncAs(&Qdrip1);
    Qmsgdrip2.syncAs(&Qdrip2);
    Qmsgdrip3.syncAs(&Qdrip3);

    // ajoute quelques images a recycler
    for(int i=0;i<30;i++) {
        blob *image=new blob();
        image->set(i);
        image->setRecycleQueue(&Qrecycle);
        Qrecycle.push_front(image);
    }

    //
    // plugin FFMPEG
    //
    const char *shader; // pas de string, a cause des messages... ahlala... a regler!
    string view1,view2,view3;
    string format;
    switch(style) {
        case STYLE_SEPARATE:
            shader="normal";
            format="rgb";
            view1="/main/A/tex";
            view2="/main/B/tex";
            view3="/main/C/tex";
            break;
        case STYLE_MIXRGB:
            shader="mixrgb";
            format="rgb";
            view1="/main/A/texr";
            view2="/main/A/texg";
            view3="/main/A/texb";
            break;
        case STYLE_OMNI:
            shader="omni_live";
            //shader="A";
            format="rgb";
            view1="/main/A/tex1"; // special pour l'instant
            view2="/main/A/tex2";
            view3="/main/A/tex3";
            break;
    }

    imqueue Qmsgf1("msgf1");
    imqueue Qmsgf2("msgf2");
    imqueue Qmsgf3("msgf3");

    pluginFfmpeg F1;
    pluginFfmpeg F2;
    pluginFfmpeg F3;

    message *m;

    sprintf(filename,cam,0);
    m=new message(256);
    *m << osc::BeginMessage("/start") << filename << "" << format << view1 << 0 << osc::EndMessage;
    Qmsgf1.push_front(m);

    sprintf(filename,cam,1);
    m=new message(256);
    *m << osc::BeginMessage("/start") << filename << "" << format << view2 << 0 << osc::EndMessage;
    Qmsgf2.push_front(m);

    sprintf(filename,cam,2);
    m=new message(256);
    *m << osc::BeginMessage("/start") << filename << "" << format << view3 << 0 << osc::EndMessage;
    Qmsgf3.push_front(m);


#ifdef HAVE_OCULUS
    System::Init(Log::ConfigureDefaultLog(LogMask_All));

    Ptr<DeviceManager> pManager;
    Ptr<HMDDevice> pHMD;
    pManager = *DeviceManager::Create();
    pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();

    Ptr<SensorDevice> pSensor;
    SensorFusion SFusion;
    if (pHMD!=NULL)
    {
      double EyeDistance,DistortionK[4];
      HMDInfo hmd;
      if (pHMD->GetDeviceInfo(&hmd))
      {
        EyeDistance = hmd.InterpupillaryDistance;
        DistortionK[0] = hmd.DistortionK[0];
        DistortionK[1] = hmd.DistortionK[1];
        DistortionK[2] = hmd.DistortionK[2];
        DistortionK[3] = hmd.DistortionK[3];
      }

      printf("Distortion coefficients:\n");
      printf("%f %f %f %f\n",DistortionK[0],DistortionK[1],DistortionK[2],DistortionK[3]);
      fflush(stdout);

      pSensor = *pHMD->GetSensor();
      if (pSensor)
      {
        SFusion.AttachToSensor(pSensor);
      }
    }

    Quatf hmdOrient;
    float EyePitch,EyeRoll,EyeYaw;
    float yawparam,pitchparam;
#endif

    //
    // plugin viewer
    //

    m=new message(1042);
    *m << osc::BeginBundleImmediate;
    //*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
    if( style==STYLE_SEPARATE ) {
        *m << osc::BeginMessage("/new-window/main") << 100 << 100 << 1500 << 375 << true << "OmniPlayer" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 0.3333f << 0.0f << 1.0f << -10.0f << shader << "clamp-to-border" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/main/B") << 0.3333f << 0.6666f << 0.0f << 1.0f << -10.0f << shader << "clamp-to-border" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/main/C") << 0.6666f << 1.0f << 0.0f << 1.0f << -10.0f << shader << "clamp-to-border" << osc::EndMessage;
    }else if( style==STYLE_MIXRGB ) {
        *m << osc::BeginMessage("/new-window/main") << 100 << 100 << 1024 << 768 << true << "OmniPlayer" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << shader << "clamp-to-border" << osc::EndMessage;
    }else if( style==STYLE_OMNI ) {
        //*m << osc::BeginMessage("/new-window/main") << 0 << 0 << 1024 << 1024 << false << "OmniPlayer" << osc::EndMessage;
        *m << osc::BeginMessage("/new-window/main") << 0 << 0 << 1280 << 800 << false << "OmniPlayer" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << shader << "clamp-to-border" << osc::EndMessage;
#ifdef HAVE_OCULUS
      if (pHMD==NULL) {
#endif
        *m << osc::BeginMessage("/defevent") << 0 << 0 << "/set/main/A/yawparam" << osc::Symbol("$xr") << osc::EndMessage;
        *m << osc::BeginMessage("/defevent") << 0 << 0 << "/set/main/A/pitchparam" << osc::Symbol("$yr") << osc::EndMessage;
#ifdef HAVE_OCULUS
      }
#endif
    }

    *m << osc::EndBundle;
    // add message to Qdisplay, so the viewer can get it only after it is started.
    Qdisplay.push_front(m);


    pluginViewerGLFW V;

    //
    // plugin Drip
    //
    // to sync, we must provide a time...
    int begin=(int)(getTimeNow())+5;

    pluginDrip D1;
    pluginDrip D2;
    pluginDrip D3;

    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/fps") << 30.0 << osc::EndMessage;
    *m << osc::BeginMessage("/start") << begin << osc::EndMessage; // ends the init
    *m << osc::EndBundle;
    Qmsgdrip1.push_back(m);

    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/fps") << 30.0 << osc::EndMessage;
    *m << osc::BeginMessage("/start") << begin << osc::EndMessage; // ends the init
    *m << osc::EndBundle;
    Qmsgdrip2.push_back(m);

    m=new message(256);
    *m << osc::BeginBundleImmediate;
    *m << osc::BeginMessage("/fps") << 30.0 << osc::EndMessage;
    *m << osc::BeginMessage("/start") << begin << osc::EndMessage; // ends the init
    *m << osc::EndBundle;
    Qmsgdrip3.push_back(m);


    //
    // connexions entre les plugins (par les imqueues)
    //

    // ffmpeg
    F1.setQ(0,&Qrecycle);
    F1.setQ(1,&Qdrip1);
    F1.setQ(2,&Qmsgf1);

    F2.setQ(0,&Qrecycle);
    F2.setQ(1,&Qdrip2);
    F2.setQ(2,&Qmsgf2);

    F3.setQ(0,&Qrecycle);
    F3.setQ(1,&Qdrip3);
    F3.setQ(2,&Qmsgf3);

    // drip
    D1.setQ(0,&Qdrip1);
    D1.setQ(1,&Qdisplay);
    D1.setQ(2,&Qmsgdrip1);

    D2.setQ(0,&Qdrip2);
    D2.setQ(1,&Qdisplay);
    D2.setQ(2,&Qmsgdrip2);

    D3.setQ(0,&Qdrip3);
    D3.setQ(1,&Qdisplay);
    D3.setQ(2,&Qmsgdrip3);

    // viewer
    V.setQ(0,&Qdisplay);

    //
    // mode full automatique
    //
    V.start();
    //make sure that initialization is done before sending the luts
    while(!Qdisplay.empty()) usleep(1000);

    //
    // images fixes (LUT)
    //
    if( style==STYLE_OMNI ) {
        sprintf(filename,lutleft,0);
        loadImg(filename,"/main/A/lutleft0",Qdisplay);
        sprintf(filename,lutleft,1);
        loadImg(filename,"/main/A/lutleft1",Qdisplay);
        sprintf(filename,lutleft,2);
        loadImg(filename,"/main/A/lutleft2",Qdisplay);
        sprintf(filename,lutright,0);
        loadImg(filename,"/main/A/lutright0",Qdisplay);
        sprintf(filename,lutright,1);
        loadImg(filename,"/main/A/lutright1",Qdisplay);
        sprintf(filename,lutright,2);
        loadImg(filename,"/main/A/lutright2",Qdisplay);
    }

    F1.start();
    F2.start();
    F3.start();

    D1.start();
    D2.start();
    D3.start();

    for(int i=0;;i++)
    {
#ifdef HAVE_OCULUS
        if (pHMD!=NULL)
        {
          hmdOrient = SFusion.GetOrientation(); //orientation in Quaternion format
          //printf("%f %f %f %f\n",hmdOrient.x,hmdOrient.y,hmdOrient.z,hmdOrient.w);
          //fflush(stdout);

          hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&EyeYaw, &EyePitch, &EyeRoll);
          //printf("%f %f %f\n",EyeYaw,EyePitch,EyeRoll);
          //fflush(stdout);

          yawparam=(float)((EyeYaw+M_PI)/(2*M_PI));
          pitchparam=(float)((EyePitch+(M_PI/2))/M_PI);
          //printf("%f %f\n",yawparam,pitchparam);
          //fflush(stdout);

          m=new message(256);
          *m << osc::BeginBundleImmediate;
          *m << osc::BeginMessage("/set/main/A/yawparam") << yawparam << osc::EndMessage;
          *m << osc::BeginMessage("/set/main/A/pitchparam") << pitchparam << osc::EndMessage;
          *m << osc::EndBundle;
          Qdisplay.push_front(m);
        }
#endif
        //usleep(33333); //30Hz
        usleep(16666); //30Hz
        //usleep(100000); //10Hz

        if( !F1.status() ) break;
        if( !F2.status() ) break;
        if( !F3.status() ) break;
        if( !D1.status() ) break;
        if( !D2.status() ) break;
        if( !D3.status() ) break;
        if( !V.status() ) break;
    }

#ifdef HAVE_OCULUS
    System::Destroy();
#endif

    V.stop();
    D3.stop();
    D2.stop();
    D1.stop();
    F3.stop();
    F2.stop();
    F1.stop();
}


int main(int argc,char *argv[]) {
    char lutleftName[STR_LEN];
    char lutrightName[STR_LEN];
    char camName[STR_LEN];

    cout << "omniPlayerLive (imgv version " << IMGV_VERSION << ")"<<endl;

    int style=STYLE_OMNI;

    lutleftName[0]=0;
    lutrightName[0]=0;
    camName[0]=0;
    for(int i=1;i<argc;i++) {
        if( strcmp(argv[i],"-h")==0 ) {
            cout << argv[0] << " -style [separate|mixrgb|omni]"<<endl;
            exit(0);
        }
        if( strcmp(argv[i],"-style")==0 && i+1<argc ) {
            if( strcmp(argv[i+1],"separate")==0 ) style=STYLE_SEPARATE;
            else if( strcmp(argv[i+1],"mixrgb")==0 ) style=STYLE_MIXRGB;
            else if( strcmp(argv[i+1],"omni")==0 ) style=STYLE_OMNI;
            else cout << "unknown style '"<<argv[i+1]<<"'"<<endl;
            i++;
        }
        if( strcmp(argv[i],"-cam")==0 && i+1<argc ) {
            strcpy(camName,argv[i+1]);
            i++;continue;
        }
        if( strcmp(argv[i],"-lutleft")==0 && i+1<argc ) {
            strcpy(lutleftName,argv[i+1]);
            i++;continue;
        }
        if( strcmp(argv[i],"-lutright")==0 && i+1<argc ) {
            strcpy(lutrightName,argv[i+1]);
            i++;continue;
        }
    }

    go(style,camName,lutleftName,lutrightName);
}


