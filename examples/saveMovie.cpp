

/*!
\defgroup saveMovie saveMovie
@{
\ingroup examples

Save a movie as images.<br>
...

Usage
------------

~~~
saveMovie toto.avi blub%05d.png
~~~
Save the movie as a list of images

Plugins used
------------
- \ref pluginFfmpeg
- \ref pluginSave

@}
*/


//
// saveMovie
//

#include "imgv/imgv.hpp"

#include <imgv/qpmanager.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>
// pour clock()
#include <time.h>

#include <imgv/imqueue.hpp>


using namespace std;
using namespace cv;


//
// pluginSkip : only output 1 image for every n images it gets
// also, do the sum of differences of all the skipped images and extra skip if below a threshold...
//
class pluginSkip : public plugin<blob> {
        int i;
        int32 skip;
	Mat sum; // sum of all pixels, sum of all pixel squared (in channels 0 and 1)
	double seuil;
    public:
        pluginSkip() { ports["in"]=0;ports["out"]=1; portsIN["in"]=true; }
        ~pluginSkip() { }
        void init() {
		i=0;
		skip=1; // no skip by default
		seuil=-1.0; // no threshold used.
        }
        bool loop() {
	    // wait for an image (and/or command)
            blob *i0=pop_front_wait(0);
		if( i==0 ) {
			// initialize everything
			sum.create(i0->rows,i0->cols,CV_64FC2);
			double *p=(double *)sum.data;
			for(int k=0;k<i0->rows*i0->cols;k++,p+=2) p[0]=p[1]=0.0;
		}
		//log << "got image " << i << info;
		// add this image to the average (ASSUME COLOR IMAGES!)
		double *p=(double *)sum.data;
		uchar *q=(uchar *)i0->data;
		for(int k=0;k<i0->rows*i0->cols;k++,p+=2,q+=3) {
			p[0]+=(double)q[0]+q[1]+q[2];
			p[1]+=(double)q[0]*q[0]+(double)q[1]*q[1]+(double)q[2]*q[2];
		}
		if( i%skip==0 && i>0 ) {
				// decide if this image is good
				// copy avg
				// reset the counters
				double *p=(double *)sum.data;
				uchar *q=(uchar *)i0->data;
				double sumvar=0.0;
				for(int k=0;k<i0->rows*i0->cols;k++,p+=2,q+=3) {
					q[0]=q[1]=q[2]=(uchar)(p[0]/skip/3.0);
					sumvar+=(double)(p[1]-p[0]*p[0]/skip/3.0)/skip/3.0;
					p[0]=p[1]=0;
				}
				sumvar/=i0->rows*i0->cols;
				cout << "Frame "<< setw(5) << i<<" var moy = "<<setw(12) << sumvar<<endl;
			if( seuil<0.0 || sumvar < seuil ) {
				// send it out
				push_back(1,(recyclable **)&i0);
			}else{
				// recycle it
				recyclable::recycle((recyclable **)&i0);
				cout << "image " << i << " pas stable!"<<endl;
			}
		}else{
			// recycle it
			recyclable::recycle((recyclable **)&i0);
		}
	    i++;
            return true;
        }
	bool decode(const osc::ReceivedMessage &m) {
		const char *address=m.AddressPattern();
		if( oscutils::endsWith(address,"/skip") ) {
			m.ArgumentStream() >> skip >> osc::EndMessage;
			log << "Sending 1 image for every "<<skip<<" image received" << warn;
		}else if( oscutils::endsWith(address,"/seuil") ) {
			m.ArgumentStream() >> seuil >> osc::EndMessage;
			log << "Using threshold " << seuil << " on std variation per pixel" << warn;
		}else{
			log << "????? : " << m << err;
		}
	}
};





void go(const char *movieName,const char *outputName,int skip,double seuil,int nbsave) {

	string save[nbsave]; // nom du plugin
	string qsave[nbsave]; // nom de la queue pour controler le plugin


	qpmanager::addQueue("recycle");
	qpmanager::addQueue("skip");
	qpmanager::addQueue("save");
	qpmanager::addQueue("display");
	qpmanager::addQueue("status");

	// ajoute quelques images a recycler
	int nbEmpty=10; // we have to remeber this so we can wait later
	qpmanager::addEmptyImages("recycle",nbEmpty);

	// plugins
	qpmanager::addPlugin("F","pluginFfmpeg");
	qpmanager::addPlugin("K","pluginSkip");
	qpmanager::addPlugin("V","pluginViewerGLFW");
	for(int i=0;i<nbsave;i++) {
		char buf[3]="SA";
		buf[1]='A'+i;
		save[i]=string(buf);
		cout << "save plugin is "+save[i]<<endl;
		qpmanager::addPlugin(save[i],"pluginSave");
		qsave[i]=qpmanager::nextAnonymousQueue();
		qpmanager::addQueue(qsave[i]);
		cout << "queue save plugin is "+qsave[i]<<endl;
	}

	// extra ports
	qpmanager::reservePort("F","toStatus");

	qpmanager::connect("recycle","F","in");
	qpmanager::connect("skip","F","out");
	qpmanager::connect("skip","K","in");
	qpmanager::connect("save","K","out");
	for(int i=0;i<nbsave;i++) {
		qpmanager::connect("save",save[i],"in");
		qpmanager::connect(qsave[i],save[i],"control");
		qpmanager::connect("display",save[i],"out"); // pas dans le bon ordre, mais pas grave
	}
	qpmanager::connect("display","V","in");
	qpmanager::connect("status","F","toStatus");

	// viewer

	message *m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	//
	// save
	//

	for(int i=0;i<nbsave;i++) {
		m=new message(256);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/mode")<<"id"<<osc::EndMessage; // generate first "full" view
		*m << osc::BeginMessage("/mapview")<<"/main/A/tex"<< outputName <<osc::EndMessage;
		*m << osc::BeginMessage("/init-done")<<osc::EndMessage; // to see the transform
		*m << osc::EndBundle;
		qpmanager::add(qsave[i],m);
	}

	//
	// ffmpeg
	//

	// start the movie!
	m=new message(512);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/max-fps") << -1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/start") << movieName << "" << "rgb" << "/main/A/tex" << 0 << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "end" << "toStatus" << "/movie-done" << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycle",m);

	//
	// skip
	//
	m=new message(512);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/skip") << skip << osc::EndMessage;
	*m << osc::BeginMessage("/seuil") << seuil << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("skip",m);


	// start plugins

	qpmanager::start("F");
	qpmanager::start("K");
	for(int i=0;i<nbsave;i++) qpmanager::start(save[i]);
	qpmanager::start("V");

	//
	// wait until done or dead
	//
	double lastM,lastP,lastT;
	for(int i=0;;i++) {
		// for now just check if status is empty, since we only expect /movie-done
		if( qpmanager::queueSizeCtrl("status")>0 ) break;

		/*
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
				}
			}
			recyclable::recycle((recyclable **)&m);
		}
		*/

#ifndef WIN32
		struct timespec t0m,t0p,t0t;
		clock_gettime(CLOCK_MONOTONIC, &t0m);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t0p);
		clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t0t);
		double newM,newP,newT;
		newM=(double)t0m.tv_sec+t0m.tv_nsec/1000000000.0;
		newP=(double)t0p.tv_sec+t0p.tv_nsec/1000000000.0;
		newT=(double)t0t.tv_sec+t0t.tv_nsec/1000000000.0;
		double deltaM,deltaP,deltaT;
		deltaM=newM-lastM;
		deltaP=newP-lastP;
		deltaT=newT-lastT;
		lastM=newM;
		lastP=newP;
		lastT=newT;

		//cout << "MONO=" << deltaM << " PROCESS=" << deltaP << " THREAD="<<deltaT<<endl;
#endif

		usleep(500000);
		usleep(500000);
		
	}

	// attend que la queue de recyclage retourne a 100%
	// pour garantir que les images sont toutes sauvegardees
	cout << "MOVIE DONE! Waiting for save to complete..." << endl;

	for(;;) {
		if( qpmanager::queueSize("recycle")==nbEmpty ) break; // we are truly done
		cout << qpmanager::queueSize("recycle") << " images in queue"<<endl;
		usleep(500000);
	}

	qpmanager::stop("F");
	qpmanager::stop("K");
	for(int i=0;i<nbsave;i++) qpmanager::stop(save[i]);
	qpmanager::stop("V");
}

void usage() {
	cout << "Usage: saveMovie [-skip 10] -i movie.avi -o out%%05d.png [-threads 2]" << endl <<
		"for raspberry pi calibration, use: -seuil n " << endl;
}

int main(int argc,const char *argv[]) {

	int skip=1;
	const char *movieName=NULL;
	const char *outputName=NULL;
	double seuil=-1;
	int nbsave=2;
	
	for(int i=1;i<argc;i++) {
		if( strcmp("-h",argv[i])==0 ) {
			usage();exit(0);
		}
		if( strcmp("-i",argv[i])==0 && i+1<argc ) {
			movieName=argv[i+1];
			i++;continue;
		}
		if( strcmp("-o",argv[i])==0 && i+1<argc ) {
			outputName=argv[i+1];
			i++;continue;
		}
		if( strcmp("-skip",argv[i])==0 && i+1<argc ) {
			skip=atoi(argv[i+1]);
			i++;continue;
		}
		if( strcmp("-seuil",argv[i])==0 && i+1<argc ) {
			seuil=atof(argv[i+1]);
			i++;continue;
		}
		if( strcmp("-threads",argv[i])==0 && i+1<argc ) {
			nbsave=atof(argv[i+1]);
			i++;continue;
		}
	}

	if( movieName==NULL || outputName==NULL || skip<1 ) { usage();exit(0); }

	cout << "imgv   version " << IMGV_VERSION << endl;
	cout << "opencv version " << CV_VERSION << endl;
	qpmanager::initialize();

	REGISTER_PLUGIN_TYPE("pluginSkip",pluginSkip);

	go(movieName,outputName,skip,seuil,nbsave);
}


