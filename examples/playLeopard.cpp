//
// playLeopard
//
// test le pluginLeoprd
//

/*!

  \defgroup playLeopard playLeopard
  @{
  \ingroup examples

  Teste le plugin Leopard.<br>

  Usage
  -----------

  ~~~
  playLeopard
  ~~~

  Plugins used
  ------------
  - \ref pluginLeopard

  @}

 */

#include <imgv/imgv.hpp>
#include "config.hpp"
#include <imqueue.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <imgv/qpmanager.hpp>

//#define FILE_CAM "/home/roys/projet/raspberry/scan-A-%03d.png"
//#define FILE_PROJ "/home/roys/projet/raspberry/leopard/leopard_1280_800_100_32_B_%03d.png"

//#define FILE_CAM "/home/faezeh/svn3d/imgv/other/remote/scan_damier999_picam1/cam%03d.png"
//#define FILE_PROJ "/home/faezeh/Videos/leopard1/leopard_1280_1024_100_32_B_%03d.png"
//#define IMAGE_COUNT 50





using namespace std;
using namespace cv;
//string campath="/home/faezeh/svn3d/imgv/other/remote/scan_damier999_picam1/cam%03d.png";
//string projpath="/home/faezeh/Videos/leopard1/leopard_1280_1024_100_32_B_%03d.png";
int  ImageCount;
const char *FileCam;
const char *FileProj;
int display; 

// pour match.cml
int screenWidth;
int screenHeight;
float pixelsPerMM;

int seuil;

int iter;
int iterstep;

int setupViewer(){

	// PLUGIN: viewerGLfw
	qpmanager::addPlugin("V","pluginViewerGLFW");
	qpmanager::addQueue("display");
	qpmanager::connect("display","V","in");

        message *m=new message(1042);
        *m << osc::BeginBundleImmediate;
        //*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
        *m << osc::BeginMessage("/new-window/main") << 100 << 100 << 1280 << 480 << true << "fenetre principale" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 0.5f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
        *m << osc::BeginMessage("/new-quad/main/B") << 0.5f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
        *m << osc::EndBundle;
        qpmanager::add("display",m);


	qpmanager::start("V");
}


void loadImg(string nom,string view,string qname)
{
	cv::Mat image=cv::imread(nom,-1);
	//cout << "image "<<nom << " has "<<image.channels()<<" channels"<<endl;

	// convert to gray!!
	if( image.channels()!=1 ) cv::cvtColor(image,image,CV_BGR2GRAY);

	blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
	i1->view=view;
	qpmanager::add(qname,i1);
}

void go()
{
	if(display)setupViewer();

	// test si les plugins sont disponibles... en direct plutot qu'avec ifdef
	if( !qpmanager::pluginAvailable("pluginLeopard")){
		cout << "Some plugins are not available... :-("<<endl;
		return;
	}

	qpmanager::addPlugin("L","pluginLeopard");
	qpmanager::addQueue("camera");
	qpmanager::addQueue("projecteur");
	qpmanager::addQueue("recycle");
	qpmanager::addQueue("resultat");

	qpmanager::connect("camera", "L","incam");
	qpmanager::connect("projecteur", "L","inproj");
	//qpmanager::connect("resultat", "L","out");
	qpmanager::connect("display", "L","out");
	qpmanager::connect("recycle", "L","recycle");

	qpmanager::addEmptyImages("recycle",10);

	message *m;

	int count=ImageCount;

	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/set/view") << "/main/A/tex" << "/main/B/tex" << osc::EndMessage;
	*m << osc::BeginMessage("/set/iter") << iter << iterstep << osc::EndMessage;
	*m << osc::BeginMessage("/set/count") << count << osc::EndMessage;
	*m << osc::BeginMessage("/set/screen") << screenWidth << screenHeight << pixelsPerMM << osc::EndMessage;
	*m << osc::BeginMessage("/set/activeDiff") << seuil << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("camera",m);

	qpmanager::start("L");

	// read all images and send them to the leopard matcher

	for(int i=0;i<count;i++) {
		cout << i << endl;
		char name[100];
		sprintf(name,FileProj,i);
		loadImg(name,"blub","projecteur");

		//sprintf(name,"leopard_1280_800_100_32_B_%03d.png",i);
		sprintf(name,FileCam,i);
		loadImg(name,"blub","camera");

		usleep(1000);
	}

	// wait until the matcher sends back something
	
	for(int i=0;;i++) {
		cout<<"..."<<endl;
		// rien a faire...
		usleep(500000);
		//qpmanager::dump();
		//		qpmanager::loop("V");
		//		qpmanager::loop("C");
		//	qpmanager::loop("V");
		//	qpmanager::loop("C");
		if( !qpmanager::status("L") ) break;
	}

	qpmanager::stop("L");
}

//"/home/faezeh/svn3d/imgv/other/remote/scan_damier999_picam1/cam%03d.png"
// playLeopard -cam /home/..../cam%03d.png  -proj /home/..../leopard%03d.png -count 50


//"/home/faezeh/svn3d/imgv/other/remote/scan_damier999_picam1/cam%03d.png"
//"/home/faezeh/Videos/leopard1/leopard_1280_1024_100_32_B_%03d.png"

int main(int argc,char *argv[]) {

//screenWidth=1296;
//screenHeight=972;
//pixelsPerMM=38.03;

screenWidth=1920;
screenHeight=1200;
pixelsPerMM=38.76;
iter=15;
iterstep=5;

seuil=96;

FileCam="/home/faezeh/svn3d/imgv/other/remote/scan_damier999_picam1/cam%03d.png";
FileProj="/home/faezeh/Videos/leopard1/leopard_1280_1024_100_32_B_%03d.png";
ImageCount=50;
display=0;	
for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			printf("Usage: %s [-h] [-cam camerapath] [-proj leopard_1280_1024_100_32_B_] [-number n] [-display] [-screen 1920 1200 38.76]\n",argv[0]);
		
			exit(0);
		}else if( strcmp(argv[i],"-seuil")==0 && i+1<argc) {
			seuil = atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-cam")==0 && i+1<argc) {
			FileCam = argv[i+1];
			i++;continue;
		}else if( strcmp(argv[i],"-proj")==0 && i+1<argc) {
			FileProj= argv[i+1];
			i++;continue;
		}else if( strcmp(argv[i],"-number")==0 && i+1<argc) {
			ImageCount=atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-display")==0 ) {
                        display=1;
                        continue;
		}else if( strcmp(argv[i],"-screen")==0 && i+3<argc) {
			// pour le match.xml, on veut la dimension reelle de l'ecran
			screenWidth=atoi(argv[i+1]);
			screenHeight=atoi(argv[i+2]);
			pixelsPerMM=atof(argv[i+3]);
			i+=3;continue;
		}else if( strcmp(argv[i],"-iter")==0 && i+1<argc) {
			iter=atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-iterstep")==0 && i+1<argc) {
			iterstep=atoi(argv[i+1]);
			i++;continue;
                }else{
			printf("Unkown path '%s'\n",argv[i]);
			exit(0);
	
	}
	}

	if( FileCam==NULL || FileProj==NULL || ImageCount<=0 ) {
		cout << "missing options"<<endl;
		printf("Usage: %s [-h] [-cam camerapath] [-proj leopard_1280_1024_100_32_B_] [-number n] [-display] [-seuil 96]\n",argv[0]);
		exit(0);
	}

	printf("Using Cam = %s\n",FileCam);
	printf("Using Proj= %s\n",FileProj);

	cout << "opencv version " << CV_VERSION << endl;
	qpmanager::initialize();

	go();

	//qpmanager::dump();
	//qpmanager::dumpDot("out.dot");
}




