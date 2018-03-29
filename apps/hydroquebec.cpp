//
// hydroquebec
//
// capte et analyse une sequence d'image de compteurs
//

#include <imgv/imgv.hpp>
#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/qpmanager.hpp>


#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

//#include <imgv/imqueue.hpp>
//#include <imgv/config.hpp>


using namespace std;
using namespace cv;

//#define USE_PROFILOMETRE
#ifdef USE_PROFILOMETRE
 #include <profilometre.h>
#endif

// de combien on reduit verticalement l'image sur 493
#define YREDUX	403
#define XREDUX	200
#define FPS 300
#define FAC	3

#include <vector>

#include <sys/time.h>

double getTimeNow(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec+tv.tv_usec/1000000.0;
}


class point {
	public:
	int32 x,y;
	point() { x=y=0; }
	point(int x,int y) : x(x), y(y) { }

	int distance2(point Q) {
		return (x-Q.x)*(x-Q.x)+(y-Q.y)*(y-Q.y);
	}
	void div(int by) { x=(x*2+1)/(2*by);y=(y*2+1)/(2*by); }
};

//
// our own processing plugin
//
#define NBLAST	5
class pluginProcess : public plugin<blob> {
	public:
		int xs,ys; // taille des images
		vector<point> pos;
		vector<point> neg;

		// dernieres valeurs
		double lastpos[NBLAST];
		double lastneg[NBLAST];

		double previousdelta;
		double delta;

		string logName;
		FILE *F;

		// click counter
		int click;
		// image counter
		int icount;
	public:
	pluginProcess(string logname) {
			ports["in"]=0;
			ports["out"]=1;

			logName=logname;
			F=NULL;
	}
	~pluginProcess() { }
	void init() {
		xs=ys=-1; // unknown
		for(int i=0;i<NBLAST;i++) lastpos[i]=lastneg[i]=0.0;
		previousdelta=0.0;
		click=0;
		icount=0;

			F=fopen(logName.c_str(),"a");
			if( F==NULL ) { printf("Unable to open %s\n",logName.c_str());exit(0); }
	}
	void uninit() {
		if( F!=NULL ) fclose(F);
	}
	bool loop() {
		// process messages and get next image
		blob *i1=pop_front_wait(0);

		// process image
		if( xs<0 ) {
			// envoie une rampe alpha pour voir ce que ca donne...
			xs=i1->cols;
			ys=i1->rows;
			blob *i2=new blob(xs,ys,CV_8UC4);
			unsigned char *p=i2->data;
			int x,y;
			for(y=0;y<ys;y++) for(x=0;x<xs;x++) {
				p[0]=0;
				p[1]=0;
				p[2]=0;
				p[3]=0; // invisible
				p+=4;
			}
			i2->view="/main/B/tex";
			push_back(1,&i2);
		}

		// get current values
		if( pos.size()>0 && neg.size()>0 ) {
			unsigned char *p=i1->data;
			int spos=0;
			int sneg=0;
			for(int i=0;i<pos.size();i++) spos+=p[pos[i].y*i1->cols+pos[i].x];
			for(int i=0;i<neg.size();i++) sneg+=p[neg[i].y*i1->cols+neg[i].x];
			double mpos=(double)spos/pos.size();
			double mneg=(double)sneg/neg.size();
			// update history
			int i;
			for(i=1;i<NBLAST;i++) lastpos[i-1]=lastpos[i];
			for(i=1;i<NBLAST;i++) lastneg[i-1]=lastneg[i];
			lastpos[NBLAST-1]=mpos;
			lastneg[NBLAST-1]=mneg;
			// new mean
			previousdelta=delta;
			delta=0.0;
			for(i=1;i<NBLAST;i++) delta+=(lastpos[i]-lastneg[i]);
			delta/=NBLAST;
			//cout << "pos="<<mpos<<"  neg="<<mneg<< " pos-neg="<<mpos-mneg<<" delta="<<delta<<endl;
			//cout << delta <<endl;
			if( previousdelta*delta < 0 ) { click++; }
			//if( previousdelta*delta < 0 && delta>0 ) { cout <<click<<endl; }
			icount++;

			if( icount==FPS ) {
				double power=(3600.0*7.2*click/200.0); // /(images/220)
				double now=getTimeNow();
				printf("%12.3f  %3d  %8.1f\n",now,click,power);
				fprintf(F,"%12.3f  %3d  %8.1f\n",now,click,power);
				icount=0;
				click=0;
			}
		}

		//cout << "cols="<<i1->cols<<" rows="<<i1->rows<<" es="<<i1->elemSize()<<" es1="<<i1->elemSize1()<<" cont="<<i1->isContinuous()<<endl;

		push_back(1,&i1);

		return true;
	}

	int findclosest(point P,vector<point> vec) {
		int i,imin;
		int dmin;
		imin=-1;
		dmin=999999;
		for(i=0;i<vec.size();i++) {
			int d2=P.distance2(vec[i]);
			if( d2<dmin ) { dmin=d2;imin=i; }
		}
		return(imin);
	}

	bool decode(const osc::ReceivedMessage &m) {
		cout << "plugin process got message : "<<m<<endl;
	    const char *address=m.AddressPattern();
		if( oscutils::endsWith(address,"/add/pos") ) {
			point P;
			m.ArgumentStream() >> P.x >> P.y >> osc::EndMessage;
			P.div(FAC);
			pos.push_back(P);
		}else if( oscutils::endsWith(address,"/add/neg") ) {
			point P;
			m.ArgumentStream() >> P.x >> P.y >> osc::EndMessage;
			P.div(FAC);
			neg.push_back(P);
		}else if( oscutils::endsWith(address,"/del/pos") ) {
			point P;
			m.ArgumentStream() >> P.x >> P.y >> osc::EndMessage;
			P.div(FAC);
			int i=findclosest(P,pos);
			if( i>=0 ) {
				vector<point>::iterator it = pos.begin() + i;
				pos.erase(it);
			}
		}else if( oscutils::endsWith(address,"/del/neg") ) {
			point P;
			m.ArgumentStream() >> P.x >> P.y >> osc::EndMessage;
			P.div(FAC);
			int i=findclosest(P,neg);
			if( i>=0 ) {
				vector<point>::iterator it = neg.begin() + i;
				neg.erase(it);
			}
		}

		// fabrique une image
		blob *i2=new blob(xs,ys,CV_8UC4);
		unsigned char *p=i2->data;
		for(int i=0;i<xs*ys*4;i++) *p++=0;
		for(int i=0;i<pos.size();i++) {
			cout << "pos" << i << " : " << pos[i].x << "," << pos[i].y << endl;
			p=i2->data+(pos[i].y*xs+pos[i].x)*4;
			p[0]=0; p[1]=255; p[2]=0; p[3]=255;
		}
		for(int i=0;i<neg.size();i++) {
			cout << "neg" << i << " : " << neg[i].x << "," << neg[i].y << endl;
			p=i2->data+(neg[i].y*xs+neg[i].x)*4;
			p[0]=0; p[1]=0; p[2]=255; p[3]=255;
		}
		i2->view="/main/B/tex";
		push_back(1,&i2);

		return false;
	}
};





//
// filename can be NULL
//
// [recycle] -> prosilica -> [process] -> Process -> [display] -> viewer -> [save] -> Save -X
// infilename:
// [recycle] -> ffmpeg -> [process]
//
void capture(char *filename,bool video,char *infilename) {

	message *m;

	qpmanager::addQueue("recycle");
	qpmanager::addQueue("display");
	qpmanager::addQueue("save");
	qpmanager::addQueue("process");

	// ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",60);

	if( infilename==NULL ) {
		//
		// PLUGIN: Prosilica
		//
		qpmanager::addPlugin("C","pluginProsilica");

		qpmanager::connect("recycle","C","in");
		qpmanager::connect("process","C","out");

		m=new message(1024);
		*m << osc::BeginBundleImmediate;
		// high performance... dont loose frames. use all mem if needed.
		*m << osc::BeginMessage("/wait-for-recycle") << false << osc::EndMessage;
		*m << osc::BeginMessage("/open") << "" << "/main/A/tex" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningHorizontal" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "BinningVertical" << 1 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Width" << 659-XREDUX << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "Height" << 493-YREDUX << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "OffsetX" << XREDUX/2 << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "OffsetY" << YREDUX/2 << osc::EndMessage;
		// after "start-capture", you cant change image parameters (width, height, format,.)
		*m << osc::BeginMessage("/set") << "*" << "PixelFormat" << osc::Symbol("Mono8") << osc::EndMessage;
		*m << osc::BeginMessage("/start-capture") << "*" << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "ExposureTimeAbs" << 1000.0f << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "AcquisitionFrameRateAbs" << (float)FPS << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
		*m << osc::BeginMessage("/set") << "*" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
		*m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
		*m << osc::BeginMessage("/start-acquisition") << "*" << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("recycle",m);

	}else{

		qpmanager::addPlugin("C","pluginFfmpeg");

		qpmanager::connect("recycle","C","in");
		qpmanager::connect("process","C","out");

		m=new message(1024);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/start") << infilename << "" << "mono" << "/main/A/tex" << 0 << osc::EndMessage;
		*m << osc::BeginMessage("/max-fps") << 20.0f << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("recycle",m);

	}


	//
	// PLUGIN: viewer
	//
	qpmanager::addPlugin("V","pluginViewerGLFW");
	qpmanager::reservePort("V","dialog-cam");
	qpmanager::reservePort("V","dialog-process");

	qpmanager::connect("display","V","in");
	qpmanager::connect("recycle","V","dialog-cam");
	qpmanager::connect("process","V","dialog-process");

	m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 10 << 10 << (659-XREDUX)*FAC << (493-YREDUX)*FAC << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << GL_CLAMP_TO_BORDER << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/B") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << GL_CLAMP_TO_BORDER << osc::EndMessage;
	// pause le video lui-meme
	//*m << osc::BeginMessage("/defevent-ctrl") << "P" << "dialog-cam" << "/pause" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl")<< "8" << "dialog-cam" << "/max-fps" << 20.0f << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl")<< "9" << "dialog-cam" << "/max-fps" << 100.0f << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl")<< "0" << "dialog-cam" << "/max-fps" << 1.0f << osc::EndMessage;
	// affiche en pause
	*m << osc::BeginMessage("/defevent-ctrl") << "S" << "in" << "/manual" << true << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "Q" << "in" << "/manual" << false << osc::EndMessage;
	// ramasse des points
	*m << osc::BeginMessage("/defevent-ctrl") << "1" << "dialog-process" << "/add/pos" << osc::Symbol("$x") << osc::Symbol("$y") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "2" << "dialog-process" << "/add/neg" << osc::Symbol("$x") << osc::Symbol("$y") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "3" << "dialog-process" << "/del/pos" << osc::Symbol("$x") << osc::Symbol("$y") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "4" << "dialog-process" << "/del/neg" << osc::Symbol("$x") << osc::Symbol("$y") << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	//
	// PLUGIN: save
	//
	if( filename ) {
		qpmanager::addPlugin("S","pluginSave");
		qpmanager::connect("save","V","out");
		qpmanager::connect("save","S","in");

		m=new message(1024);
		*m << osc::BeginBundleImmediate;
		if( video ) {
			*m << osc::BeginMessage("/mode") << "video" << osc::EndMessage;
		}else{
			*m << osc::BeginMessage("/mode") << "id" << osc::EndMessage;
		}
		*m << osc::BeginMessage("/filename") << filename << osc::EndMessage;
		*m << osc::BeginMessage("/init-done") << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("save",m);

		qpmanager::start("S");
	}

	//
	// plugin process (local!)
	//
	pluginProcess P("meter.log");
	P.name="process";
	P.setQ("in",qpmanager::getQueue("process"));
	P.setQ("out",qpmanager::getQueue("display"));

	P.start();


	qpmanager::start("C");
	qpmanager::start("V");

	qpmanager::dumpDot("out.dot");

	for(int i=0;;i++) {
		usleep(250000);
		if( !qpmanager::status("C") ) break;
		if( filename && !qpmanager::status("S") ) break;
		if( !qpmanager::status("V") ) break;
	}

	if( filename ) { qpmanager::stop("S"); }
	qpmanager::stop("C");
	qpmanager::stop("V");
	P.stop();
}


int main(int argc,char *argv[]) {

char filename[200];
char infilename[200];
bool video;

	qpmanager::initialize();

	infilename[0]=0;
	filename[0]=0;
	video=true;

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			printf("Usage: %s [-save toto%%04d.png]\n",argv[0]);
			exit(0);
		}else if( strcmp(argv[i],"-video")==0 ) {
			video=true;
			continue;
		}else if( strcmp(argv[i],"-save")==0 && i+1<argc ) {
			strcpy(filename,argv[i+1]);
			i+=1;continue;
		}else if( strcmp(argv[i],"-in")==0 && i+1<argc ) {
			strcpy(infilename,argv[i+1]);
			i+=1;continue;
		}else{ printf("??? %s\n",argv[i]);exit(0); }
	}

#ifdef USE_PROFILOMETRE
	profilometre_init();
#endif

	cout << "opencv version " << CV_VERSION << endl;
	cout << "imgv version   " << IMGV_VERSION << endl;

	capture(filename[0]?filename:NULL,video,infilename[0]?infilename:NULL);

#ifdef USE_PROFILOMETRE
	profilometre_dump();
#endif
}


