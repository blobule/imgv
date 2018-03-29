//
// playPi
//
// images sur raspbery pi
//
// plugins used:  ViewerGLES2
//
// utiliser expr `date +%s` + 10
//

/*!

  \defgroup playPi playPi
  @{
  \ingroup examples

  Display an image on raspberri pi.<br>

  Usage
  -----------

  Display the _indian head_ image.
  ~~~
  playImage -in 44444
  ~~~

  Plugins used
  ------------
  - \ref pluginViewerGLES2
  - \ref pluginHomog
  - \ref pluginSyslog

  @}

 */

#include <imgv/imgv.hpp>
#include <imgv/udpcast.h>
#include "config.hpp"

#include <imqueue.hpp>

#include <imgv/qpmanager.hpp>

#define USE_PROFILOMETRE
#ifdef USE_PROFILOMETRE
	#include <profilometre.h>
#endif


// trop lent!!!
//#define USE_PATTERN

// incoming data
#define IMG_PORT	44444
#define CMD_PORT	44445


//#define USE_READ
//#define USE_READ_FNAME "/home/pi/patterns/leopard_1280_800_100_32_B_%03d.png"
//#define USE_READ_NB	100



#include <imgv/pluginViewerGLES2.hpp>
#include <imgv/pluginRaspicam.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>


using namespace std;
using namespace cv;


#define NORMAL_SHADER	"basees2"

//
// log /[info|warn|err] <plugin name> <message> ... other stuff ...
//
/***
  class Log {
  struct send_t { };
  public:
  Log(std::string where) {
  cout << "Log log to "<<where<<endl;
  Qout=qpmanager::getQueue(where);
  m=NULL;
  }
  ~Log() { if(m!=NULL) delete m; }

// add a string to the current message
Log& operator<<( const char* s ) {
if( m==NULL ) begin("/info");
 *m << s;
 return *this;
 }
 Log& operator<<( send_t &s ) {
 if( &s==&end ) { flush(); }
 return *this;
 }

 Log& info() { begin("/info"); return *this; }
 Log& warn() { begin("/warn"); return *this; }
 Log& err() { begin("/err"); return *this; }

 static send_t end;
 private:
 imqueue *Qout; // where we send the message
 message *m;
 void flush() {
 if( m!=NULL ) {
 if( Qout==NULL ) { delete m;m=NULL; }
 else{
 *m << osc::EndMessage;
 Qout->push_back(m);
 }
 }
 }
 void begin(std::string head) {
 flush();
 m=new message(256);
 *m << osc::BeginMessage(head.c_str());
 }
 };

 Log::send_t Log::end;
 ***/


class toto : public recyclable {
	public:
		int a,b,c;
		toto() { cout << "toto"<<endl; }
		~toto() { cout << "~toto"<<endl; }

};



void FindBlobs(const cv::Mat &binary, std::vector < std::vector<cv::Point2i> > &blobs)
{
    blobs.clear();

    // Fill the label_image with the blobs
    // 0  - background
    // 1  - unlabelled foreground
    // 2+ - labelled foreground

    cv::Mat label_image;
    binary.convertTo(label_image, CV_32SC1);

    int label_count = 2; // starts at 2 because 0,1 are used already

    for(int y=0; y < label_image.rows; y++) {
        int *row = (int*)label_image.ptr(y);
        for(int x=0; x < label_image.cols; x++) {
            if(row[x] != 1) {
                continue;
            }
            cv::Rect rect;
            cv::floodFill(label_image, cv::Point(x,y), label_count, &rect, 0, 0, 4);

            std::vector <cv::Point2i> blob;

            for(int i=rect.y; i < (rect.y+rect.height); i++) {
                int *row2 = (int*)label_image.ptr(i);
                for(int j=rect.x; j < (rect.x+rect.width); j++) {
                    if(row2[j] != label_count) {
                        continue;
                    }

                    blob.push_back(cv::Point2i(j,i));
                }
            }

            blobs.push_back(blob);

            label_count++;
        }
    }
}


// 0=in, 1=out1, 2=recycle
class pluginDetect : public plugin<blob> {
	udpcast reseau;
	int n;

	int nbScene;
	vector< vector<Point3f> > scenePoints;
	vector< vector<Point2f> > imagePoints;
    public:
        pluginDetect() {
		ports["in"]=0;
		ports["out"]=1;

		portsIN["in"]=true;
	 }
        ~pluginDetect() { }
        void init() {
		log << "Bonjour!" << warn;
		n=0;
		nbScene=0;
	}
        bool loop() {
            //blob *i2=dynamic_cast<blob*>(Q[0]->pop_back_wait());


	    // on est lent... alors on va conserver l'image la plus recente
	    blob *ie=pop_front_wait(0); // recycle
	    for(;;) {
		blob *ienew=pop_front_nowait(0);
		if( ienew==NULL ) break;
		recycle(ie);
		ie=ienew;
	    }

		{
		Mat cameraMatrix = Mat::zeros(3, 3, CV_64F);
		cameraMatrix.at<double>(0,0)=623.0;
		cameraMatrix.at<double>(0,2)=320.0;
		cameraMatrix.at<double>(1,1)=623.0;
		cameraMatrix.at<double>(1,2)=240.0;
		cameraMatrix.at<double>(2,2)=1.0;
		Mat distCoeffs = Mat::zeros(8, 1, CV_64F);
		double yo[5]={0.2158609556516055, -1.477992265387892, 0.005669519813215829, -0.01021395312642177, 3.480873076536037};
		for( int i=0;i<5;i++) distCoeffs.at<double>(0,i)=yo[i];
		log << cameraMatrix <<info;
		log << distCoeffs << info;
		blob toto;
	        undistort(*ie,toto, cameraMatrix, distCoeffs);
		toto.copyTo(*ie);
            	push_back(1,&ie);
		return true;
		}

            cvtColor(*ie, *ie, CV_BGR2GRAY); // -> into gray scale
            //GaussianBlur(*ie, *ie, Size(7,7), 1.5, 1.5);
            //Canny(*ie, *ie, 0, niveau, 3);

		log << "image size is "<<ie->cols<<","<<ie->rows<<info;

		vector<Point2f> corners;

		//resize(img, timg, Size(), scale, scale);
                bool found = findChessboardCorners(*ie, cv::Size(7,5), corners,
                    CV_CALIB_CB_ADAPTIVE_THRESH /*| CV_CALIB_CB_FAST_CHECK */| CV_CALIB_CB_NORMALIZE_IMAGE);
                    //CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);

		log << "found = "<<found << warn;
		if( found ) {

			// affiche les points
			for(int i=0;i<corners.size();i++) {
				log << "corner "<<i<<" is " <<corners[i]<< info;
			}

                	cvtColor(*ie,*ie, CV_GRAY2BGR);
                	drawChessboardCorners(*ie, cv::Size(7,5), corners, found);

			// ajoute les points images dans la liste
			imagePoints.push_back(corners);
			//for(int i=0;i<corners.size();i++) imagePoints[nbScene].push_back(corners[i]);

			// ajoute les points de la grille
			// les grids points sont pour une grille 7x5
			vector<Point3f> pts;
			double squareSize=1.0;
			for( int y = 0; y < 5; y++ )
			for( int x = 0; x < 7; x++ ){
				pts.push_back(Point3f(y*squareSize, x*squareSize, 0.0f));
			}
			scenePoints.push_back(pts);
			nbScene++;

			log << "Scene "<<nbScene<<warn;

			if( nbScene>6 ) {
				log << "CALIBRATION!"<<err;
				Mat cameraMatrix;
				cameraMatrix = Mat::eye(3, 3, CV_64F);
				Mat distCoeffs = Mat::zeros(8, 1, CV_64F);
				vector<Mat> rvecs, tvecs;
				vector<float> reprojErrs;
				cameraMatrix.at<double>(0,0) = 1.0; // CV_CALIB_FIX_ASPECT_RATIO
				double rms = calibrateCamera(scenePoints, imagePoints, cv::Size(ie->cols,ie->rows),
					 cameraMatrix, distCoeffs, rvecs, tvecs,
					 CV_CALIB_FIX_K4|CV_CALIB_FIX_K5|CV_CALIB_FIX_ASPECT_RATIO);
				log << "rms is " << rms <<info;
				log << "cameraMatrix is " << cameraMatrix << info;
				for(int i=0;i<reprojErrs.size();i++) log << "err "<<i<<" is "<<reprojErrs[i]<<info;
				log << "distCoeffs is " << distCoeffs << info;
			}
		}

		//threshold(*ie,*ie,235,255,THRESH_BINARY);

/*
		std::vector < std::vector<cv::Point2i > > blobs;
		FindBlobs(*ie, blobs);

		ie->create(ie->size(), CV_8UC3);

		//char buf[200];
		//sprintf(buf,"image %d\n",n);
		//udp_send_data(&reseau,(unsigned char *)buf,strlen(buf));
		log << "nb blob = " << blobs.size() << info;

		unsigned char *p=ie->data;
		for(int i=0;i<ie->rows*ie->cols*3;i++) p[i]=0;

		// Randomly color the blobs
	    for(size_t i=0; i < blobs.size(); i++) {
		unsigned char r = 255 * (rand()/(1.0 + RAND_MAX));
		unsigned char g = 255 * (rand()/(1.0 + RAND_MAX));
		unsigned char b = 255 * (rand()/(1.0 + RAND_MAX));
		log << "blob "<<i<<" size="<<blobs[i].size()<<info;

		for(size_t j=0; j < blobs[i].size(); j++) {
		    int x = blobs[i][j].x;
		    int y = blobs[i][j].y;
		    (*ie).at<cv::Vec3b>(y,x)[0] = b;
		    (*ie).at<cv::Vec3b>(y,x)[1] = g;
		    (*ie).at<cv::Vec3b>(y,x)[2] = r;
		}
	    }
*/

            //ie->view="canny";
            push_back(1,&ie);
	    n++;
            return true;
        }
};





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

//
// test plugin viewer GLES2
//
void go(int inImgPort,int inCmdPort,int startTime, bool raspicamIsUsed, bool videoIsUsed,bool useDetect,char *lutName,char *imageName) {

	// test si les plugins sont disponibles... en direct plutot qu'avec ifdef
	if( !qpmanager::pluginAvailable("pluginViewerGLES2")
			|| !qpmanager::pluginAvailable("pluginStream") 
			|| !qpmanager::pluginAvailable("pluginPattern") ) {
		cout << "Some plugins are not available... :-("<<endl;
		return;
	}
	if( raspicamIsUsed && !qpmanager::pluginAvailable("pluginRaspicam") ) {
		cout << "Raspicam is not available... :-("<<endl;
	}

	//	qpmanager::addPlugin("V","pluginViewerGLES2");
	//	qpmanager::addPlugin("C", "pluginRaspicam");
	qpmanager::addPlugin("V","pluginViewerGLES2");
	qpmanager::addQueue("display");
	qpmanager::connect("display","V","in");
	if(raspicamIsUsed){
		qpmanager::addPlugin("C", "pluginRaspicam");
		qpmanager::addQueue("recycleCam");
		qpmanager::addEmptyImages("recycleCam", 30); // pour auto adjust + rapide

		qpmanager::connect("display", "C","out");
		qpmanager::connect("recycleCam", "C","in");
	}	
	if( inImgPort>0 ) {
		qpmanager::addPlugin("InImg","pluginStream");
		qpmanager::addQueue("recycle");
		qpmanager::addEmptyImages("recycle",20);

		// this is receiving from the net...
		qpmanager::connect("recycle","InImg","in");
		qpmanager::connect("display","InImg","out");
	}

	qpmanager::addPlugin("D","pluginDetect");
	qpmanager::addQueue("detection");
	qpmanager::connect("detection","D","in");
	qpmanager::connect("display","D","out");

	if( useDetect ) {
		qpmanager::connect("detection", "C","out");
	}


	message *m;


#ifdef USE_READ
	qpmanager::addPlugin("R","pluginRead");
	qpmanager::addQueue("recycle-r");
	qpmanager::addQueue("drip");
	qpmanager::addEmptyImages("recycle-r",5); // HARD LIMIT ON PATTERN!! un peu de lousse
	qpmanager::connect("recycle-r","R","in");
	qpmanager::connect("drip","R","out");

	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/file") << USE_READ_FNAME << osc::EndMessage;
	*m << osc::BeginMessage("/min") << 0 << osc::EndMessage;
	*m << osc::BeginMessage("/max") << (USE_READ_NB-1) << osc::EndMessage;
	*m << osc::BeginMessage("/view") << "img" << osc::EndMessage;
	//*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycle-r",m);

	//
	// Plugin drip
	// (entre le read et le display )
	//
	qpmanager::addPlugin("D","pluginDrip");
	qpmanager::connect("drip","D","in");
	qpmanager::connect("display","D","out");

	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/fps") << 4.0 << osc::EndMessage;
	//*m << osc::BeginMessage("/start") << startTime << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("drip",m);
#endif

#ifdef USE_PATTERN
	qpmanager::addPlugin("P","pluginPattern");
	qpmanager::addQueue("recycle-p");
	qpmanager::connect("recycle-p","P","in");
	qpmanager::connect("display","P","out");
	qpmanager::addEmptyImages("recycle-p",2); // HARD LIMIT ON PATTERN!!

	m=new message(1042);
	*m << osc::BeginBundleImmediate;

	*m << osc::BeginMessage("/set/leopard") << 1280 << 800 << 100 << 32 << true <<osc::EndMessage;
	//*m << osc::BeginMessage("/set/checkerboard") << 1280 << 800 << 64 <<osc::EndMessage;
	//*m << osc::BeginMessage("/pid") << "blub" << osc::EndMessage;
	*m << osc::BeginMessage("/automatic") << osc::EndMessage;
	*m << osc::BeginMessage("set/view") << "img" << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycle-p",m);

#endif



	//qpmanager::addQueue("display");
	//qpmanager::dump();


	//imqueue Qdisplay("display");

	//
	// plugin ViewerGLES2
	//

	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
	//*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	//*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << NORMAL_SHADER << osc::EndMessage;

	*m << osc::BeginMessage("/add-layer") << "es2-tunnel"/*"es2Simple"*/ << osc::EndMessage;
	if(raspicamIsUsed && videoIsUsed)
		*m << osc::BeginMessage("/add-layer") << "es2-video-cam" << osc::EndMessage;
	else if(raspicamIsUsed)
		*m << osc::BeginMessage("/add-layer") << "es2-simple" << osc::EndMessage;
	else if(videoIsUsed) 
		*m << osc::BeginMessage("/add-layer") << "es2-video-stereo" << osc::EndMessage;
	else if( imageName!=NULL && imageName[0]!=0 )
		*m << osc::BeginMessage("/add-layer") << "es2-simple" << osc::EndMessage;
	else
		*m << osc::BeginMessage("/add-layer") << "es2-tunnel" << osc::EndMessage;
	//*m << osc::BeginMessage("/add-layer") << "es2-effect-nop" << osc::EndMessage;
	//////*m << osc::BeginMessage("/add-layer") << "es2-effect-lut" << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	//*m << osc::BeginMessage("/video") << "/opt/vc/src/hello_pi/hello_video/test.h264" << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	//Qdisplay.push_front(m);
	qpmanager::add("display",m);



	if( inImgPort>0 ) {
		m=new message(1042);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/intcp") << inImgPort << osc::EndMessage;
		*m << osc::BeginMessage("/init-done") << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("recycle",m);
	}


	// loadImg(argv[i],"/main/A/tex","display");
	//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg","/main/A/tex","display");
	//loadImg(std::string(IMGV_SHARE)+"/images/color-ramp.jpg","/main/A/tex","display");

	if( lutName==NULL || lutName[0]==0 ) {
		loadImg(std::string(IMGV_SHARE)+"/images/lut.png","lut","display");
	}else{
		loadImg(lutName,"lut","display");
	}

	if( imageName!=NULL && imageName[0]!=0 ) {
		loadImg(imageName,"img","display");
	}


	//qpmanager::dump();

	//plugin<blob>* V=qpmanager::getP("V");

	// connect display queue to viewer
	// a faire***********************************
	//V.setQ(0,&Qdisplay);

	//qpmanager::dump();

	//
	// mode full automatique
	//

	qpmanager::start("V");
	//	qpmanager::init("V");
	//	qpmanager::init("C");

	//		qpmanager::init("V");
	if(raspicamIsUsed)
		//		qpmanager::init("C");
		qpmanager::start("C");

	if( useDetect ) {
		qpmanager::start("D");
		cout << "PLUGIN D STARTED"<<endl;
	}

	if( inImgPort>0 ) {
		qpmanager::start("InImg");
	}

	//
	// lire chaque image et l'ajouter a la queue d'affichage
	//
	//sleep(2);
	//qpmanager::dump();

#ifdef USE_READ
	qpmanager::start("R");
	qpmanager::start("D");
#endif

#ifdef USE_PATTERN
	qpmanager::start("P");
#endif

	if( inCmdPort>0 ) qpmanager::startNetListeningUdp(inCmdPort);
	//if( inCmdPort>0 ) qpmanager::startNetListeningTcp(inCmdPort);


	cout << "entering loop..."<<endl;
	if(raspicamIsUsed){
		int widthNum = 1280;
		int heightNum = 960;
		int resDen = 1;
		message *m=new message(1042);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/view") <<"tex" << osc::EndMessage;
		*m << osc::BeginMessage("/setWidth") << widthNum/resDen << osc::EndMessage;
		*m << osc::BeginMessage("/setHeight") << heightNum/resDen << osc::EndMessage;
		*m << osc::BeginMessage("/setFormat") << 3 << osc::EndMessage;
		*m << osc::BeginMessage("/setExposure") << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/setMetering") << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/init-done") << osc::EndMessage;
		qpmanager::add("recycleCam",m);
	}
	if(videoIsUsed){
		cout << "Début video" << endl;
		sleep(3);
		message *m=new message(1042);
		*m << osc::BeginMessage("/video") << "/opt/vc/src/hello_pi/hello_custom/test.avi"<< osc::EndMessage;
		qpmanager::add("display",m);
	}
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

	if( inCmdPort>0 ) qpmanager::stopNetListening();

	qpmanager::stop("V");
	if(raspicamIsUsed)
		qpmanager::stop("C");
	//qpmanager::uninit("V");
	//qpmanager::uninit("C");

	if( inImgPort>0 ) {
		qpmanager::stop("InImg");
	}

#ifdef USE_PATTERN
	qpmanager::stop("P");
#endif
#ifdef USE_READ
	qpmanager::stop("R");
	qpmanager::stop("D");
#endif
	if( useDetect ) { qpmanager::stop("D"); }
}

int main(int argc,char *argv[]) {
	//int inImgPort; // tcp incoming images
	//int inCmdPort; // udp incoming commands
	int startTime;
	char lutName[200];
	char imageName[200];

#ifdef USE_PROFILOMETRE
	profilometre_init();
#endif

	lutName[0]=0;
	imageName[0]=0;

	REGISTER_PLUGIN_TYPE("pluginDetect",pluginDetect);

	bool raspicamIsUsed = false;
	bool videoIsUsed = false;
	bool useDetect = false;

	cout << "opencv version " << CV_VERSION << endl;
	qpmanager::initialize();

	//inImgPort=-1;
	startTime=3; // dans 3 secondes si on donne pas un temps -start

	int i;
	for(i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			cout << "usage: playPi [-start timestamp|delay] [-video] [-raspicam] [-detect]"<<endl;
			exit(0);
		}
		/**
		  if( strcmp(argv[i],"-in")==0 && i+1<argc ) {
		  inImgPort=atoi(argv[i+1]);
		  i++;continue;
		  }
		 **/
		if( strcmp(argv[i],"-start")==0 && i+1<argc ) {
			startTime=atoi(argv[i+1]);
			i+=1;continue;
		}
		if(strcmp(argv[i], "-video") == 0){
			videoIsUsed = true;
			 continue;
		}
		if(strcmp(argv[i], "-raspicam") == 0){
			raspicamIsUsed = true;
			continue;
		}
		if(strcmp(argv[i], "-detect") == 0){
			useDetect=true;
			continue;
		}
		if(strcmp(argv[i], "-lut") == 0 && i+1<argc){
			strcpy(lutName,argv[i+1]);
			i++;continue;
		}
		if(strcmp(argv[i], "-image") == 0 && i+1<argc){
			strcpy(imageName,argv[i+1]);
			i++;continue;
		}
	}

	go(IMG_PORT,CMD_PORT,startTime, raspicamIsUsed, videoIsUsed,useDetect,lutName,imageName);

	//qpmanager::dump();
	qpmanager::dumpDot("out.dot");
}


