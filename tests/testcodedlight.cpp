

/** test coded light **/


#include <imgv/imgv.hpp>
#include <imgv/qpmanager.hpp>

//#define PROJ_FILE_NAME	"leopard_1280_800_100_32_B_%03d.png"
//#define CAM_FILE_NAME	"/home/roys/projet/raspberry/scan-A-%03d.png"
//#define NB_IMG		50

//#include <codedlight/patterns/leopard.h>
#include <codedlight/patterns/phaseshift.h>
#include <codedlight/patterns/graycode.h>
//#include <codedlight/patterns/microphaseshift.h>

using namespace cl3ds;

class pluginLocal : public plugin<blob> {
	//LeopardGenerator LG;
	//LeopardGenerator *LG;
	//PatternGenerator *LG;
	GrayCodeGenerator *G;
	

	int n;

	public:
	 pluginLocal() {
		ports["in"]=0;
		ports["out"]=1;
		portsIN["in"]=true;
		name="local";
	}
	~pluginLocal() { }
	void init() {
		log << "local graycode stated." << warn;
		//LG=new LeopardGenerator(cv::Size(800,600), 50, 32.0, true);
		//LG=new PhaseShiftGenerator(cv::Size(800,600),PatternGenerator::HORIZONTAL);

		/*
		double periods[] = { 21.123, 20.266, 21.822, 22.047 };
		vector<double> vperiods;
		for(int i=0;i<4;i++) vperiods.push_back(periods[i]);
		LG=new MicroPhaseShiftGenerator(cv::Size(800,600),PatternGenerator::HORIZONTAL,
			21.489,3,vperiods);
		*/

		G=new GrayCodeGenerator();
		G->setSize(cv::Size(640,480));
		log << "Count is "<<G->count()<<info;
		log << "NbBits X="<<G->nbBitsX()<< " Y="<<G->nbBitsY()<<info;

		n=0;
	}
	bool loop() {
		if( G->hasNext() ) {
			blob *i1=pop_front_wait(0);
			cv::Mat img;
			img=G->get();
			img.copyTo(*i1); // ne detruit pas blob, alors que *i1=img efface le blob
			i1->n=n;
			i1->view=string("/main/A/tex");
			log << "graycode " << n << warn;
			//char buf[100];
			//sprintf(buf,"yo%04d.png",n);
			//cv::imwrite(buf,*i1);
			push_back(1,&i1);
			n++;
			usleep(300000);
		}else{
			usleep(300000);
		}
		return true;
	}
};



int main(int argc,char *argv[]) {
	cout << "-- test codedlight --" << endl;

	qpmanager::initialize();

	pluginLocal L;

    qpmanager::addQueue("recycle");
    qpmanager::addQueue("display");

	qpmanager::addEmptyImages("recycle",10);

	qpmanager::addPlugin("V","pluginViewerGLFW");

	L.setQ("in",qpmanager::getQueue("recycle"));
	L.setQ("out",qpmanager::getQueue("display"));
	qpmanager::connect("display","V","in");
	
    message *m=new message(1042);
    *m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	// go!

	qpmanager::start("V");
	L.start();

	for(;;) {
		cout << "Zzzz" <<endl;
		//cout << "sizes r = " << qpmanager::getQueue("recycle")->size() <<endl;
		//cout << "sizes d = " << qpmanager::getQueue("display")->size() <<endl;
        sleep(1);
	}
	

}


