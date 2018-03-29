//
// playFaezeh
//
// images stream sur raspbery pi
//
// plugins used:  ViewerGLES2
//
// utiliser expr `date +%s` + 10
//
// test
//

/*!

  \defgroup playFaezeh playFaezeh
  @{
  \ingroup examples

  Envoie les images de la raspicam vers le reseau<br/>

  Usage
  -----------

  ~~~
  playFazeh
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

// definir pout utiliser syslog
#define USE_SYSLOG

// trop lent!!!
//#define USE_PATTERN

// incoming data
#define IMG_PORT	44444
#define CMD_PORT	44445


#define USE_READ
#define USE_READ_FNAME "/home/pi/patterns/leopard_1280_800_100_32_B_%03d.png"
#define USE_READ_NB	100



#include <imgv/pluginViewerGLES2.hpp>
#include <imgv/pluginRaspicam.hpp>
#ifdef USE_SYSLOG
#include <imgv/pluginSyslog.hpp>
#endif

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
    public:
        pluginDetect() {
		ports["in"]=0;
		ports["out"]=1;

		portsIN["in"]=true;
	 }
        ~pluginDetect() { }
        void init() {
		log << "Bonjour!" << warn;
		int k=udp_init_sender(&reseau,"127.0.0.1",55555,UDP_TYPE_NORMAL);
		log << "reseau = " << k << warn;
		n=0;
	}
        bool loop() {
            //blob *i2=dynamic_cast<blob*>(Q[0]->pop_back_wait());
            blob *ie=pop_front_wait(0); // recycle
            cvtColor(*ie, *ie, CV_BGR2GRAY);
            //GaussianBlur(*ie, *ie, Size(7,7), 1.5, 1.5);
            //Canny(*ie, *ie, 0, niveau, 3);

		threshold(*ie,*ie,235,255,THRESH_BINARY);

		std::vector < std::vector<cv::Point2i > > blobs;
		FindBlobs(*ie, blobs);

		ie->create(ie->size(), CV_8UC3);

		char buf[200];
		sprintf(buf,"image %d\n",n);
		udp_send_data(&reseau,(unsigned char *)buf,strlen(buf));
		log << "nb blob = " << blobs.size() << "  " << buf << info;

		unsigned char *p=ie->data;
		for(int i=0;i<ie->rows*ie->cols*3;i++) p[i]=0;

		    // Randomy color the blobs
    for(size_t i=0; i < blobs.size(); i++) {
        unsigned char r = 255 * (rand()/(1.0 + RAND_MAX));
        unsigned char g = 255 * (rand()/(1.0 + RAND_MAX));
        unsigned char b = 255 * (rand()/(1.0 + RAND_MAX));

        for(size_t j=0; j < blobs[i].size(); j++) {
            int x = blobs[i][j].x;
            int y = blobs[i][j].y;

            (*ie).at<cv::Vec3b>(y,x)[0] = b;
            (*ie).at<cv::Vec3b>(y,x)[1] = g;
            (*ie).at<cv::Vec3b>(y,x)[2] = r;
        }
    }

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
void go(int inImgPort,int inCmdPort,int startTime, bool raspicamIsUsed, bool videoIsUsed,bool useDetect) {

	// test si les plugins sont disponibles... en direct plutot qu'avec ifdef
	if( !qpmanager::pluginAvailable("pluginViewerGLES2")
			|| !qpmanager::pluginAvailable("pluginStream") 
			|| !qpmanager::pluginAvailable("pluginPattern")
			|| !qpmanager::pluginAvailable("pluginRaspicam") ){
		cout << "Some plugins are not available... :-("<<endl;
		return;
	}

	//	qpmanager::addPlugin("V","pluginViewerGLES2");
	//	qpmanager::addPlugin("C", "pluginRaspicam");
	qpmanager::addPlugin("V","pluginViewerGLES2");
	qpmanager::addQueue("display");
	qpmanager::connect("display","V","in");
	if(raspicamIsUsed){
		qpmanager::addPlugin("C", "pluginRaspicam");
		qpmanager::addQueue("recycleCam");
		qpmanager::addEmptyImages("recycleCam", 55);

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

	pluginDetect D;
	qpmanager::addQueue("detection");
	D.setQ("in",qpmanager::getQueue("detection"));
	D.setQ("out",qpmanager::getQueue("display"));

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
	qpmanager::connect("logq","R","log");

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
	qpmanager::connect("logq","D","log");

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

	//*m << osc::BeginMessage("/add-layer") << "es2-tunnel"/*"es2Simple"*/ << osc::EndMessage;
	//*m << osc::BeginMessage("/add-layer") << "es2-tunnel" << osc::EndMessage;
	if(raspicamIsUsed && videoIsUsed)
		*m << osc::BeginMessage("/add-layer") << "es2-video-cam" << osc::EndMessage;
	else
		if(raspicamIsUsed)
		*m << osc::BeginMessage("/add-layer") << /*"es2-tunnel"*/"es2-simple" << osc::EndMessage;
		else
		*m << osc::BeginMessage("/add-layer") << "es2-video" << osc::EndMessage;
	//*m << osc::BeginMessage("/add-layer") << "es2-effect-nop" << osc::EndMessage;
	//*m << osc::BeginMessage("/add-layer") << "es2-effect-lut" << osc::EndMessage;
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
	loadImg(std::string(IMGV_SHARE)+"/images/lut.png","lut","display");


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
		D.start();
	}

	if( inImgPort>0 ) {
		qpmanager::connect("logq","InImg","log");
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

	//Log log("logq");

	cout << "entering loop..."<<endl;
	if(raspicamIsUsed){
		int widthNum = 1280;
		int heightNum = 960;
		int resDen = 4;
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
	for(int i=0;i<100;i++) {
		cout<<"..."<<endl;
		// rien a faire...
		sleep(1);
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

#ifdef USE_SYSLOG
	// last to be killed!
	qpmanager::stop("Log");
#endif
#ifdef USE_PATTERN
	qpmanager::stop("P");
#endif
#ifdef USE_READ
	qpmanager::stop("R");
	qpmanager::stop("D");
#endif

	if( useDetect ) { D.stop(); }

}


int main(int argc,char *argv[]) {
	//int inImgPort; // tcp incoming images
	//int inCmdPort; // udp incoming commands
	int startTime;

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
			cout << "usage: playPi [-start timestamp|delay] [-video] [-rapicam] [-detect]"<<endl;
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
	}

	go(IMG_PORT,CMD_PORT,startTime, raspicamIsUsed, videoIsUsed,useDetect);

	//qpmanager::dump();
	qpmanager::dumpDot("out.dot");
}


