

/*!
\defgroup playMovie playMovie
@{
\ingroup examples

Play a movie.<br>
...

Usage
------------

~~~
playMovie toto.avi blub.mp4 ...
~~~
All the movies will be played one after the other.

Plugins used
------------
- \ref pluginFfmpeg
- \ref pluginDrip
- \ref pluginViewerGLFW

@}
*/


//
// playMovie
//
// example for sample plugins
//
// plugins used:  Ffmpeg, Drip, ViewerGLFW
//

#include "imgv/imgv.hpp"

/*
#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/pluginFfmpeg.hpp>
#include <imgv/pluginDrip.hpp>
#include <imgv/pluginSyslog.hpp>
*/

#include <imgv/qpmanager.hpp>

#include <iostream>
#include <string>
#include <vector>


// pour pluginReseau
#include "oscpack/ip/PacketListener.h"
#include "oscpack/ip/UdpSocket.h"

//#include <opencv2/opencv.hpp>

#include <stdio.h>

//#include "recyclable.hpp"
//#include "rqueue.hpp"
//#include "plugin.hpp"

#include <imgv/imqueue.hpp>

#ifdef HAVE_PROFILOMETRE
        #define USE_PROFILOMETRE
        #include <profilometre.h>
#endif

#define FPS	30.0


// peut etre "mono", "yuv", "rgb", "rgba"
// rgba est plus efficace que rgb
//
// timing for (swscaling street.mp4) / (decoding+gpu)
// mono:  0.55ms /  0.8ms
// yuv:   1.15ms /  0.8ms
// rgba:  3.7ms  /  2.7ms
// rgb:   3.1ms  /  8.0ms
//
// on voit donc que rgba est legerement plus lent a decoder,
// mais beaucoup plus rapide a envoyer au GPU
//
#define MOVIE_PIXEL_FORMAT	"rgb"

//#define SHADER "A"
#define SHADER	"homographie"
//#define SHADER	"normal"
#define SHADER_BASE	"normal150"

using namespace std;
using namespace cv;

//
// version plugin
//
// Qrecycle -> (ffmpeg) -> Qdrip -> (drip) -> Qdisplay -> (viewer) -> [recycle]
//
// le constructeur/destructeur de plugin roule toujours dans le thread principal
// le init/uninit/loop roule toujours dans un nouveau thread
//
// plugin A;
// A.start() -> demarre le thread
// ...
// A.stop() -> cancel et join le thread
//
// Le thread lui-meme fait ceci:
// A.init()
// A.loop() tant que ca retourne true
// A.uninit() si on cancel ou si loop retourn false
//
// on ramasse une image d'une queue comme ceci:
// blob *img=dynamic_cast<blob*>(Qin->pop_back_wait()); ou pop_front_wait()
//
// on fait sortir une image vers une queue de sortie comme ceci:
// Qout->push_front((recyclable **)&img);   ou push_back
//   note: le pointeur img sera NULL apres le push
//
// si on veut recycler une image, on fait:
//   recyclable::recycle((recyclable **)&img);
// ou recycle(img)
// (si l'image a une queue de recyclage (voir setRecycleQueue), elle ira la.
//  sinon, elle sera liberee)
//
//


void loadImg(string nom,string view,string qname)
{
        cv::Mat image=cv::imread(nom,CV_LOAD_IMAGE_UNCHANGED);
        cout << "**************** image "<<nom<<" channels is " << image.channels() << endl;
        blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
        i1->view=view;
        qpmanager::add(qname,i1);
}


// 0=in, 1=out1, 2=recycle
class pluginLocal : public plugin<blob> {
	int i;
	int niveau;
	bool sendOrig;
    public:
	pluginLocal(bool orig) { niveau=30;sendOrig=orig; }
	~pluginLocal() { }
	void init() {
 	  //i=0;cout << "[local] --- init --- " << pthread_self() << endl; 
	}
	bool loop() {
	    /*if( i==200 ) { cout << "STOP!!!!!!!!!!!!!!!!!!!!!"<<endl; return false; }*/
	    i++;
	    //blob *i2=dynamic_cast<blob*>(Q[0]->pop_back_wait());
	    blob *ie=pop_front_wait(2); // recycle
	    blob *i2=pop_front_wait(0);
	    cvtColor(*i2, *ie, CV_BGR2GRAY);
	    GaussianBlur(*ie, *ie, Size(7,7), 1.5, 1.5);
	    Canny(*ie, *ie, 0, niveau, 3);

	// test!
/*
	Mat z=*ie;
	cout << "empty is "<<z.empty()<<endl;
	cout << "z is "<<z.rows<<"x"<<z.cols<<endl;
	printf("0x%08lx\n",z.data);
	z.release();
	cout << "empty is "<<z.empty()<<endl;
	cout << "z is "<<z.rows<<"x"<<z.cols<<endl;
	printf("0x%08lx\n",z.data);
*/

	    //Q[1]->push_front((recyclable **)&i2);
	    i2->view="orig";
	    ie->view="canny";
	    if( sendOrig ) push_back(1,&i2); else recycle(i2);
	    push_back(1,&ie);
	    return true;
	}
};



// 0,1,... queues de sortie
class pluginReseau : public plugin<blob>, PacketListener {
	int port;
    public:
	pluginReseau(int port) :port(port) { cout << "[reseau] created port="<<port<<endl; }
	~pluginReseau() { cout << "[reseau] deleted"<<endl; }
	void init() { 
	  //cout << "[reseau] --- init --- " << pthread_self() << endl; 
	}
	bool loop() {
		UdpListeningReceiveSocket s( IpEndpointName( IpEndpointName::ANY_ADDRESS, port ), this );
		s.Run();
		return false;
	}

	// de PacketListener
	void ProcessPacket( const char *data, int size,
		const IpEndpointName& remoteEndpoint ) {
		cout << "[Reseau] GOT message len="<<size<<endl;
		// trouve la bonne queue pour routage
		osc::ReceivedPacket p(data,size);
		if( p.IsBundle() ) {
			cout << "[reseau] unable to route bundled messages"<<endl;
			return;
		}
		osc::ReceivedMessage m(p);
		const char *address=m.AddressPattern();
		int nb=oscutils::extractNbArgs(address);
		int pos,len;
		oscutils::extractArg(address,nb-1,pos,len); // premier arg
		cout << "add="<<address<<", premier arg is pos="<<pos<<", len="<<len<<endl;

		string qname(address+pos,len);
		imqueue *Q=(imqueue *)findQ(qname);
		if( Q==NULL ) {
			cout << "[reseau] unable to route message. No queue named "<<qname << endl;
			return;
		}
		// creer le message et l'envoyer
		message *msg=new message(size);
		msg->set(data,size);
		Q->push_front(msg);
	}

};


//
// test plugin viewer GLFW
//
// joue chaque film un apres l'autre
//
void go(int nbMovie,const char *movieNames[],const char *lutname) {
	int currentMovie; // 0,1,2,3,4...,nbMovie-1 -> cycle

	currentMovie=0;

	for(int i=0;i<nbMovie;i++) {
		printf("Movie %d : %s\n",i,movieNames[i]);
	}

	qpmanager::addQueue("recycle");
	qpmanager::addQueue("drip");
	qpmanager::addQueue("display");
	qpmanager::addQueue("status");
	//qpmanager::addQueue("log");

	// ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",30);

	//
	// LOG
	//
	//qpmanager::addPlugin("L","pluginSyslog");
	//qpmanager::connect("log","L","in");
	//qpmanager::start("L");

	//
	// PLUGIN: FFMPEG
	//
	qpmanager::addPlugin("A","pluginFfmpeg");
	//qpmanager::connect("log","A","log");

	//
	// Viewer
	//
	qpmanager::addPlugin("V","pluginViewerGLFW");
	//qpmanager::connect("log","V","log");

	message *m=new message(1042);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << (lutname==NULL?SHADER_BASE:SHADER) << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/A/fade") << 1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/set/main/A/angle") << 1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "mouse" << "in" << "/set/main/A/fade" << osc::Symbol("$xr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "mouse" << "in" << "/blub/toto/*/set/main/A/angle" << osc::Symbol("$yr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "A" << "in" << "/del-quad/main/A" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "B" << "in" << "/new-quad/main/A" << 0.2f << 0.8f << 0.2f << 0.8f << -10.0f << "A" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "B" << "in" << "/set/main/A/fade" << 1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "B" << "in" << "/set/main/A/angle" << 1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "C" << "in" << "/new-quad/main/A" << 0.0f << 0.5f << 0.0f << 1.0f << -10.0f << "A" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "Z" << "in" << "/del-window/main" << osc::EndMessage;
	// viewer out to ffmpeg
	qpmanager::reservePort("V","toFfmpeg");
	qpmanager::connect("recycle","V","toFfmpeg");
	*m << osc::BeginMessage("/defevent-ctrl") << "S" << "toFfmpeg" << "/seek" << 200.0 << osc::EndMessage;
	// set length to 90, which is the length of the fadeout...
	// add the buffers and you get the end of the movie
	*m << osc::BeginMessage("/defevent-ctrl") << "N" << "toFfmpeg" << "/set/length" << 90 << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	qpmanager::add("display",m);

	//
	// ajoute la lut
	//
	if( lutname!=NULL ) loadImg(lutname,"/main/A/homog","display");


	//
	// demarre le movie
	//
	m=new message(1042);
	// set message for end-of-movie
	qpmanager::reservePort("A","toMain");
	qpmanager::connect("status","A","toMain");
	*m << osc::BeginMessage("/defevent-ctrl") << "end" << "toMain" << "/movie-done" << osc::EndMessage;
	qpmanager::add("recycle",m);


	//
	// PLUGIN: DRIP
	//
	qpmanager::addPlugin("D","pluginDrip");
	//qpmanager::connect("log","D","log");

	m=new message(512);
	*m << osc::BeginBundleImmediate;
	// on definit ici le fade parce que defevent additionne les event...
	*m << osc::BeginMessage("/defevent-ctrl") << "fadein" << "out" << "/set/main/A/fade" << osc::Symbol("$xr") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "fadeout" << "out" << "/set/main/A/fade" << osc::Symbol("$xr") << osc::EndMessage;
	*m << osc::BeginMessage("/fps") << FPS << osc::EndMessage;
	*m << osc::BeginMessage("/start") << 3 << osc::EndMessage; // fin de l'init
	*m << osc::EndBundle;
	qpmanager::add("drip",m);

	//
	// plugin pour recevoir des messages reseau
	//
	/*
	pluginReseau R(7000);
	R.name="reseau";
	R.setQ("log",&Qlog);
	*/

	//
	// connexions entre les plugins (par les rqueues)
	//
	// on peut utiliser directement le # de port,
	// ou donne un nom de port (varie selon le plugin)

	// ffmpeg
	qpmanager::connect("recycle","A","in");
	qpmanager::connect("drip","A","out");
	//A.setQ(3,&Qstatus);    // [OUT] end of movie msg

	// drip
	qpmanager::connect("drip","D","in");
	qpmanager::connect("display","D","out");

	// viewer
	qpmanager::connect("display","V","in");
	//C.setQ(2,&Qrecycle); // pour parler au ffmpeg
	//C.setQ(3,&Qdrip); // pour parler au drip

	// on donne toutes les queues pour le routage
	//R.setQ("in",&Qdrip); // /msgdrip/...
	//R.setQ("out",&Qdisplay); // /display/...

	//
	// mode full automatique
	//

	qpmanager::start("A");
	qpmanager::start("D");
	qpmanager::start("V");

	// R.start();

	// pour recevoir des msg de l'extieur:
	// /[queueName]/joiqjoij
		usleep(500000);
		qpmanager::dumpDot("out.dot");

	bool quit=false;
	bool next=false;
    for(currentMovie=0;/*currentMovie<nbMovie*/;currentMovie++) {
		// Start the movie!!!
		// tell drip to setup events... put the messages in the image queue of Drip,
		// so they are processed at the right time... when the movie starts.
		m=new message(512);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/event/set/begin") << "fadein" << 90 << osc::EndMessage;
		*m << osc::BeginMessage("/event/set/end") << "fadeout" << 90 << osc::EndMessage;
		// send events to port 1 : Qdisplay
		*m << osc::EndBundle;
		qpmanager::add("drip",m);

		// start the movie!
		m=new message(512);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/max-fps") << -1.0f << osc::EndMessage;
		*m << osc::BeginMessage("/start") << movieNames[currentMovie%nbMovie] << "" << MOVIE_PIXEL_FORMAT << "" << 0 << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("recycle",m);

		quit=false;
		next=false;


		//
		// wait until done or dead
		//
		for(int i=0;;i++) {
			if( i==10 ) profilometre_reset(); // on oublie le debut... on veut steady state
			if( (i+1)%20==0 ) profilometre_dump();
			// check Qstatus. Is the movie done?
			message *m=qpmanager::getQueue("status")->pop_front_ctrl_nowait();
			if( m!=NULL ) {
				cout << "main got message "<< m <<endl;
				m->decode();
				for(int i=0;i<m->msgs.size();i++) {
					cout << "DECODE " << m->msgs[i] << endl;
					const char *address=m->msgs[i].AddressPattern();
					if( oscutils::endsWith(address,"/movie-done") ) {
						cout << "Movie done. Next!"<<endl;
						next=true;
					}
				}
				recyclable::recycle((recyclable **)&m);
			}
			if( next ) break;
			//usleep(200000);
			cout << "zzzz decoded frames = "<<qpmanager::queueSize("drip") <<endl;
			usleep(500000);
			if( !qpmanager::status("A") ) { quit=true; break; }
			if( !qpmanager::status("D") ) { quit=true; break; }
			if( !qpmanager::status("V") ) { quit=true; break; }
		}

		if( quit ) break;
    }

	qpmanager::stop("A");
	qpmanager::stop("D");
	qpmanager::stop("V");
	qpmanager::stop("L");
//	R.stop();
}

int main(int argc,const char *argv[]) {
	cout << "opencv version " << CV_VERSION << endl;
	qpmanager::initialize();

	profilometre_init();

	//profilometre_setup_histogram("render",0.0,60.0,1.0);
	profilometre_setup_histogram("render",0.0,10.0,0.1);

	cout << "argc is " << argc << endl;

	if( argc<2 ) {
		const char *arg[1];
		arg[0]="/home/roys/Videos/carre-phillips-anaglyph-cyl-hd.mp4";
		go(1,arg,NULL);
	}else if( strcmp("-lut",argv[1])==0 ) {
		go(argc-3,argv+3,argv[2]);
	}else{
		go(argc-1,argv+1,NULL);
	}
}


