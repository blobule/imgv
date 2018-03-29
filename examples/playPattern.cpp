
/*!
\defgroup playPattern playPattern
@{
\ingroup examples

Displays various patterns from the pattern plugin.<br>
The windows is split in two parts. On the right, the pattern changes automatically.
On the left, you must press 'Z' to get the next pattern.

Usage
------------

~~~
playPattern
~~~

Plugins used
------------
- \ref pluginPattern
- \ref pluginDrip
- \ref pluginViewerGLFW

@}
*/


#include "imgv/imgv.hpp"

//#include <imgv/pluginViewerGLFW.hpp>
//#include <imgv/pluginPattern.hpp>
//#include <imgv/pluginDrip.hpp>
//#include <imgv/pluginSyslog.hpp>

#include <iostream>
#include <string>
#include <vector>


#include <stdio.h>

#include <imgv/imqueue.hpp>

#include <imgv/qpmanager.hpp>

#ifdef HAVE_PROFILOMETRE
  #define USE_PROFILOMETRE
  #include <imgv/profilometre.h>
#endif


using namespace std;
using namespace cv;


//
// test plugin pattern
//
void go(const char *fname,int w,int h,int nb,int freq,bool blur) {

	
	//imqueue Qrecycle("recycle");
	//imqueue QrecycleX("recycleX");
	//imqueue Qdrip("drip");
	//imqueue Qdisplay("display");
	//imqueue Qlog("log");
	qpmanager::addQueue("recycle");
	qpmanager::addQueue("recycleX");
	qpmanager::addQueue("drip");
	qpmanager::addQueue("display");
	qpmanager::addQueue("log");


	//pluginSyslog Log;
	//Log.setQ("in",&Qlog);
	//Log.start();
	qpmanager::addPlugin("Log","pluginSyslog");
	qpmanager::connect("log","Log","in"); // queue "log" vers plugin "Log", port "in"
	qpmanager::start("Log");

/**
	// ajoute quelques images a recycler
	for(int i=0;i<20;i++) {
		blob *image=new blob();
		image->n=i;
		image->setRecycleQueue(&Qrecycle);
		Qrecycle.push_front(image);
	}
	for(int i=0;i<20;i++) {
		blob *image=new blob();
		image->n=i;
		image->setRecycleQueue(&QrecycleX);
		QrecycleX.push_front(image);
	}
**/
	qpmanager::addEmptyImages("recycle",20);
	qpmanager::addEmptyImages("recycleX",20);

	//
	// PLUGIN: Pattern
	//
/**
	pluginPattern P; // this one is manual
	pluginPattern Px; // this one is automatic

	P.name="P";
	Px.name="Px";
	P.setQ("log",&Qlog);
	Px.setQ("log",&Qlog);
**/
	qpmanager::addPlugin("P","pluginPattern");
	qpmanager::addPlugin("Px","pluginPattern");
	//qpmanager::connect("log","P","log");
	//qpmanager::connect("log","Px","log");


	message *m;

	m=new message(512);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/set/checkerboard") << 1920 << 1080 << 64 <<osc::EndMessage;
	*m << osc::BeginMessage("/pid") << "checker:32" << osc::EndMessage;
	*m << osc::BeginMessage("/manual") << osc::EndMessage;
	*m << osc::BeginMessage("/set/view") << "/main/A/tex" << osc::EndMessage;
	*m << osc::EndBundle;
	//Qrecycle.push_back_ctrl(m);
	qpmanager::add("recycle",m);

	m=new message(512);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/set/leopard") << w << h << nb << freq << blur <<osc::EndMessage;
	*m << osc::BeginMessage("/pid") << "leopard:32F" << osc::EndMessage;
	*m << osc::BeginMessage("/automatic") << osc::EndMessage;
	*m << osc::BeginMessage("/set/view") << "/main/B/tex" << osc::EndMessage;
	*m << osc::EndBundle;
	//QrecycleX.push_back_ctrl(m);
	qpmanager::add("recycleX",m);

	//
	// PLUGIN: viewerGLfw
	qpmanager::addPlugin("V","pluginViewerGLFW");

	string p0=qpmanager::reservePort("V");
	qpmanager::connect("recycle","V",p0);

	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 1280 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 0.5f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/B") << 0.5f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << 'Z' << p0 << "/go" << osc::EndMessage;
	// pour rire... 
	//*m << osc::BeginMessage("/defevent-ctrl") << 'X' << p0 << "/go" << osc::EndMessage;
	//*m << osc::BeginMessage("/defevent-ctrl") << 'Y' << p0 << "/go" << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	//Qdisplay.push_back_ctrl(m);
	qpmanager::add("display",m);


	//
	// PLUGIN: DRIP
	//
	m=new message(512);
	//m->reset();
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/fps") << (fname==NULL?2.0:10.0) << osc::EndMessage;
	*m << osc::BeginMessage("/start") << 3 << osc::EndMessage; // fin de l'init
	*m << osc::EndBundle;
	//Qdrip.push_back_ctrl(m);
	qpmanager::add("drip",m);

	//pluginDrip D;
	//D.setQ("log",&Qlog);
	qpmanager::addPlugin("D","pluginDrip");
	qpmanager::connect("log","D","log");

	//
	// connexions entre les plugins (par les rqueues)
	//
	// on peut utiliser directement le # de port,
	// ou donne un nom de port (varie selon le plugin)

	// pattern (manual)
	//P.setQ("in",&Qrecycle);
	//P.setQ("out",&Qdisplay);
	qpmanager::connect("recycle","P","in");
	qpmanager::connect("display","P","out");

	// pattern (automatic)... goes to drip to get an fps
	//Px.setQ("in",&QrecycleX);
	//Px.setQ("out",&Qdrip);
	qpmanager::connect("recycleX","Px","in");
	qpmanager::connect("drip","Px","out");

	// drip
	//D.setQ("in",&Qdrip);
	//D.setQ("out",&Qdisplay);
	qpmanager::connect("drip","D","in");
	qpmanager::connect("display","D","out");

	// viewer
	//V.setQ("in",&Qdisplay);
	qpmanager::connect("display","V","in");

	//
	// save
	//
	if( fname!=NULL ) {
		qpmanager::addPlugin("S","pluginSave");
		qpmanager::addQueue("save");
		qpmanager::connect("save","S","in");
		qpmanager::connect("save","V","out");
		//qpmanager::connect("log","S","log");
		m=new message(512);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/mode") << "internal" << osc::EndMessage;
		*m << osc::BeginMessage("/filename") << fname << osc::EndMessage;
		*m << osc::BeginMessage("/init-done") << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("save",m);
		qpmanager::start("S");
	}

	//P.start();
	//Px.start();
	//D.start();
	//V.start();
	qpmanager::start("P");
	qpmanager::start("Px");
	qpmanager::start("D");
	qpmanager::start("V");

	// rien a faire pendant 20*2 secondes...
	for(int i=0;;i++) {
		usleep(500000);
		/*
		if( !P.status() ) break;
		if( !Px.status() ) break;
		if( !D.status() ) break;
		if( !V.status() ) break;
		*/
		cout << "status P " <<qpmanager::status("P")<<endl;
		cout << "status Px " <<qpmanager::status("Px")<<endl;
		cout << "status D " <<qpmanager::status("D")<<endl;
		cout << "status V " <<qpmanager::status("V")<<endl;

		if( !qpmanager::status("P") ) break;
		if( !qpmanager::status("Px") ) break;
		if( !qpmanager::status("D") ) break;
		if( !qpmanager::status("V") ) break;


		if( fname!=NULL ) {
			cout << "status S " <<qpmanager::status("S")<<endl;
		}
	}

	usleep(500000);

	if( fname!=NULL ) {
		while( !qpmanager::isEmpty("save") ) { printf("...save...\n");usleep(500000); }
	}

	for(int zz=0;zz<10;zz++) usleep(500000);

	qpmanager::stop("S");
	qpmanager::stop("P");
	qpmanager::stop("Px");
	qpmanager::stop("D");
	qpmanager::stop("V");
	qpmanager::stop("Log");


	// dump
	//P.name="PatternA";
	//Px.name="PatternB";
	//D.name="Drip";
	//V.name="Viewer";

/*
	ofstream F;
	plugin<blob>::dumpBegin(F,"out.dot");

	Qrecycle.dump(F);
	QrecycleX.dump(F);
	Qdrip.dump(F);
	Qdisplay.dump(F);

	P.dump(F);
	Px.dump(F);
	D.dump(F);
	V.dump(F);

	plugin<blob>::dumpEnd(F);
*/

	qpmanager::dumpDot("ooo.dot");

}

int main(int argc,char *argv[]) {
	cout << "opencv version " << CV_VERSION << endl;
	qpmanager::initialize();

	int xsize=1920;
	int ysize=1080;
	int nb=100;
	int scale=32;
	bool blur=true;

	int i;
	for(i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			cout << "playPattern [-x 1920] [-y 1080] [-n 100] [-scale 32] [-blur|-noblur] [-h]\n";
			exit(0);
		}
		if( strcmp(argv[i],"-x")==0 && i+1<argc ) {
			xsize=atoi(argv[i+1]);
			i++;continue;
		}
		if( strcmp(argv[i],"-y")==0 && i+1<argc ) {
			ysize=atoi(argv[i+1]);
			i++;continue;
		}
		if( strcmp(argv[i],"-n")==0 && i+1<argc ) {
			nb=atoi(argv[i+1]);
			i++;continue;
		}
		if( strcmp(argv[i],"-scale")==0 && i+1<argc ) {
			scale=atoi(argv[i+1]);
			i++;continue;
		}
		if( strcmp(argv[i],"-blur")==0 ) blur=true;
		if( strcmp(argv[i],"-noblur")==0 ) blur=false;
	}

#ifdef HAVE_PROFILOMETRE
	profilometre_init();
#endif
	//go("leopard_1280_800_100_32_B_%03d.png",1280,800,100,32,true);
	//go("leopard_1280_1024_100_32_B_%03d.png",1280,800,100,32,true);
	//go("leopard_1920_1200_32B_%03d.png",1920,1200,100,32,true);
	char name[100];
	sprintf(name,"leopard_%d_%d_%d%s_%%03d.png",xsize,ysize,scale,blur?"B":"");
	cout << "Nom : " << name <<"\n";
	go(name,xsize,ysize,nb,scale,blur);
	//go(NULL,1280,800,100,32,true);
#ifdef HAVE_PROFILOMETRE
	profilometre_dump();
#endif
}


