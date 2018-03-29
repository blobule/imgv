//
// playImage
//
// example for sample plugins
//
// plugins used:  ViewerGLFW
//

/*!

\defgroup playImage playImage
@{
\ingroup examples

Display an image.<br>
While the image if displayed, it is possible to use the keys '1','2','3', and '4' to
setup an homography. Just place the mouse on each desired image corner in sequence and
press 1,2,3, and 4.<br>
Also, the keys 'H','J','K','L' have been defined to move the latest corner
left, down, up, and right respectively.

Usage
-----------

Display the _indian head_ image.
~~~
playImage
~~~

Display the images provided on the command line.
~~~
playImage toto.png
~~~

Plugins used
------------
- \ref pluginViewerGLFW
- \ref pluginHomog
- \ref pluginSyslog

@}

*/

#include <imgv/imgv.hpp>

#include <imqueue.hpp>

#include <imgv/qpmanager.hpp>


// definir pour tester les homographies
#define HOMOGRAPHIE

// pout jouer du video
#define PLAYVIDEO

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>


using namespace std;
using namespace cv;

#ifdef HOMOGRAPHIE
		#define NORMAL_SHADER "homographie"
#else
	#ifdef APPLE
		#define NORMAL_SHADER "normal150"
	#else
		#define NORMAL_SHADER "normal"
	#endif
#endif


//
// log /[info|warn|err] <plugin name> <message> ... other stuff ...
//
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


class toto : public recyclable {
	public:
	int a,b,c;
	toto() { cout << "toto"<<endl; }
	~toto() { cout << "~toto"<<endl; }
	
};

//
// load une image et place la dans une queue d'images
//
void loadImg(string nom,string view,imqueue &Q)
{
        cv::Mat image=cv::imread(nom);
        blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
        i1->view=view;
        //Q.push_back((recyclable **)&i1);
        Q.push_back(i1);
}

void loadImg(string nom,string view,string qname)
{
        cv::Mat image=cv::imread(nom);
        blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
        i1->view=view;
        //Q.push_back((recyclable **)&i1);
        qpmanager::add(qname,i1);
}

//
// test plugin viewer GLFW
//
void go(int argc,char *argv[]) {

	// test si les plugins sont disponibles... en direct plutot qu'avec ifdef
	if( !qpmanager::pluginAvailable("pluginViewerGLFW")
	 || !qpmanager::pluginAvailable("pluginHomog") ) {
		cout << "Some plugins are not available... :-("<<endl;
		return;
	}

	qpmanager::addPlugin("V","pluginViewerGLFW");
	qpmanager::addQueue("display");
	qpmanager::connect("display","V","in"); // cree une queue V.0

	//qpmanager::addQueue("display");
	//qpmanager::dump();

	message *m;

	//imqueue Qdisplay("display");


#ifdef PLAYVIDEO
	qpmanager::addQueue("recycleVideo");
	qpmanager::addEmptyImages("recycleVideo",15);

	qpmanager::addPlugin("F","pluginFfmpeg");
	qpmanager::connect("recycleVideo","F","in");
	qpmanager::connect("display","F","out");

	// set la view pour l'image output
	m=new message(256);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/start") << "/home/roys/Videos/VID_20131207_205912.mp4" << "" << "rgb" << "/main/A/tex" << 0 << osc::EndMessage;
	*m << osc::BeginMessage("/max-fps")<< 30.0f <<osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycleVideo",m); // on pourrait donner "plugin","port" et utiliser ca

	// second video

	qpmanager::addQueue("recycleVideo2");
	qpmanager::addEmptyImages("recycleVideo2",15);

	qpmanager::addPlugin("F2","pluginFfmpeg");
	qpmanager::connect("recycleVideo2","F2","in");
	qpmanager::connect("display","F2","out");

	// set la view pour l'image output
	m=new message(256);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/start") << "/home/roys/Videos/VID_20131207_210313.mp4" << "" << "rgb" << "/main/B/tex" << 0 << osc::EndMessage;
	*m << osc::BeginMessage("/max-fps")<< 30.0f <<osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycleVideo2",m); // on pourrait donner "plugin","port" et utiliser ca
	
	
#endif



#ifdef HOMOGRAPHIE
	qpmanager::addQueue("recycle");
	qpmanager::addQueue("recycle2");
	//qpmanager::addQueue("msgH");

	// ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",5);
	qpmanager::addEmptyImages("recycle2",5);

	//
	// plugin homographie
	//
	qpmanager::addQueue("save");
	qpmanager::addQueue("save2");
	
	qpmanager::addPlugin("H","pluginHomog");
	qpmanager::connect("recycle","H","in");
	qpmanager::connect("save","H","out");
	//qpmanager::connect("","H","msg"/*2*/); ????

	qpmanager::addPlugin("H2","pluginHomog");
	qpmanager::connect("recycle2","H2","in");
	qpmanager::connect("save2","H2","out");



	qpmanager::addPlugin("S","pluginSave");
	qpmanager::connect("save","S","in");
	qpmanager::connect("display","S","out");

	qpmanager::addPlugin("S2","pluginSave");
	qpmanager::connect("save2","S2","in");
	qpmanager::connect("display","S2","out");

	m=new message(256);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/mode")<<"internal"<<osc::EndMessage; // generate first "full" view
	*m << osc::BeginMessage("/mapview")<<"/main/A/homog"<<"lut%04d.png"<<osc::EndMessage;
	*m << osc::BeginMessage("/init-done")<<osc::EndMessage; // to see the transform
	*m << osc::EndBundle;
	qpmanager::add("save",m); // on pourrait donner "plugin","port" et utiliser ca

	m=new message(256);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/mode")<<"internal"<<osc::EndMessage; // generate first "full" view
	*m << osc::BeginMessage("/mapview")<<"/main/B/homog"<<"lut%04d-2.png"<<osc::EndMessage;
	*m << osc::BeginMessage("/init-done")<<osc::EndMessage; // to see the transform
	*m << osc::EndBundle;
	qpmanager::add("save2",m); // on pourrait donner "plugin","port" et utiliser ca



	// set la view pour l'image output
	m=new message(256);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/set/view")<<"/main/A/homog"<<osc::EndMessage;
	*m << osc::BeginMessage("/go")<<osc::EndMessage; // generate first "full" view
	*m << osc::BeginMessage("/go-view")<<"/deform/A/tex"<<osc::EndMessage; // to see the transform
	*m << osc::EndBundle;
	qpmanager::add("recycle",m); // on pourrait donner "plugin","port" et utiliser ca

	m=new message(256);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/set/view")<<"/main/B/homog"<<osc::EndMessage;
	*m << osc::BeginMessage("/go")<<osc::EndMessage; // generate first "full" view
	*m << osc::BeginMessage("/go-view")<<"/deform/B/tex"<<osc::EndMessage; // to see the transform
	*m << osc::EndBundle;
	qpmanager::add("recycle2",m); // on pourrait donner "plugin","port" et utiliser ca



#endif

#ifdef HOMOGRAPHIE
	//qpmanager::connect("H.in","V",2); // pour les events du viewer
#endif

	//
	// plugin ViewerGLFW
	//

	m=new message(3048);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
	//*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 1920 << 0 << 1280 << 800 << false << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/deform") << 800 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/B") << 0.0f << 1.0f << 0.0f << 1.0f
        << -10.0f << NORMAL_SHADER << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f
        << -10.0f << NORMAL_SHADER << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/deform/A") << 0.0f << 1.0f << 0.0f << 1.0f
        << -10.0f << "normal" << osc::EndMessage;
#ifdef HOMOGRAPHIE
	//
	// bind keys to send mouse info to the Homog plugin
	//
	// -> V.setQ(-1,qpmanager::getQueue("H.in"));
	//plugin<blob>* V=qpmanager::getP("V");
	qpmanager::reservePort("V","toHomog");
	qpmanager::reservePort("V","toHomog2");
	qpmanager::reservePort("V","toF");
	qpmanager::reservePort("V","toF2");
	qpmanager::connect("recycle","V","toHomog");
	qpmanager::connect("recycle2","V","toHomog2");
	qpmanager::connect("recycleVideo","V","toF");
	qpmanager::connect("recycleVideo2","V","toF2");

	*m << osc::BeginMessage("/defevent-ctrl") << "Z" << "toF" << "/start" << /*"/home/roys/Videos/VID_20131207_205912.mp4"*/ "/media/roys/LACIE/Video/titanfall.mp4" << "" << "rgb" << "/main/A/tex" << 0 << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "Z" << "toF2" << "/start" << /*"/home/roys/Videos/VID_20131207_205912.mp4"*/ "/media/roys/LACIE/Video/simnet.mp4" << "" << "rgb" << "/main/B/tex" << 0 << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "X" << "toF" << "/stop" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "X" << "toF2" << "/stop" << osc::EndMessage;
	
	*m << osc::BeginMessage("/defevent-ctrl") << "1" << "toHomog" << "/set/q" << 0 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "2" << "toHomog" << "/set/q" << 1 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "3" << "toHomog" << "/set/q" << 2 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "4" << "toHomog" << "/set/q" << 3 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "5" << "toHomog2" << "/set/q" << 0 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "6" << "toHomog2" << "/set/q" << 1 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "7" << "toHomog2" << "/set/q" << 2 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "8" << "toHomog2" << "/set/q" << 3 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "A" << "toHomog" << "/go" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "left" << "toHomog" << "/left" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "down" << "toHomog" << "/down" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "up" << "toHomog" << "/up" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "right" << "toHomog" << "/right" << osc::EndMessage;
	// pour affiche en plus la deformation
	*m << osc::BeginMessage("/defevent-ctrl") << "1" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "2" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "3" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "4" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "5" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "6" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "7" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "8" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "A" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "left" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "down" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "up" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "right" << "toHomog" << "/go-view" << "/deform/A/tex" << osc::EndMessage;
#endif

	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	//Qdisplay.push_front(m);
	qpmanager::add("display",m);


	for(int i=1;i<argc;i++) loadImg(argv[i],"/main/A/tex","display");
	if( argc==1 ) {
		//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg","/main/A/tex","display");
		//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg","/main/B/tex","display");
		loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg","/main/A/tex","display");
		loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg","/main/B/tex","display");
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
#ifdef APPLE
	//V->init();
	qpmanager::init("V");
#else
	//V->start();
	qpmanager::start("V");
#endif

#ifdef PLAYVIDEO
	qpmanager::start("F");
	qpmanager::start("F2");
#endif

#ifdef HOMOGRAPHIE
	qpmanager::start("H");
	qpmanager::start("H2");
	qpmanager::start("S");
	qpmanager::start("S2");
#endif

	//
	// lire chaque image et l'ajouter a la queue d'affichage
	//
	usleep(500000);	
	usleep(500000);
	qpmanager::dumpDot("playDeform.dot");

	usleep(500000);
	qpmanager::start("Log");

	Log log("logq");

	for(int i=0;;i++) {
#ifdef APPLE
		usleep(10000);
		//if( !V->loop() ) break;
		if( !qpmanager::loop("V") ) break;
#else
		cout<<"..."<<endl;
		// rien a faire...
		usleep(500000);
		//qpmanager::dump();
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
#endif
	}

#ifdef APPLE
	qpmanager::uninit("V");
	//V->uninit();
#else
	//V->stop();
	qpmanager::stop("V");
#endif
#ifdef HOMOGRAPHIE
	qpmanager::stop("H");
	qpmanager::stop("H2");
	qpmanager::stop("S");
	qpmanager::stop("S2");
#endif
#ifdef PLAYVIDEO
	qpmanager::stop("F");
	qpmanager::stop("F2");
#endif

}


int main(int argc,char *argv[]) {
	cout << "opencv version " << CV_VERSION << endl;
	qpmanager::initialize();

	qpmanager::dump();


	go(argc,argv);
}


