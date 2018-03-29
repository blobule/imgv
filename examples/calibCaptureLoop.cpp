//
// calibCaptureLoop
//
// analyse the timing between projecting an image and capturing it with a camera
//
// plugins used:  pattern, sync, prosilica, ViewerGLFW
//

#include "imgv/imgv.hpp"

#include <imgv/pluginSync.hpp>
#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/pluginPattern.hpp>
#include <imgv/pluginProsilica.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

#include <imgv/imqueue.hpp>


using namespace std;
using namespace cv;

//#define USE_PROFILOMETRE
#include <profilometre.h>

//
// [recycle1] ->     cam --[Qcompare]---> CMP
// [recycle2] -> pattern --[Qpattern]--> Sync -->[Qdisplay]--> viewer --[Qcompare]-> CMP
//
// on envoi /go au sync. On ramasse les images.
//

int getdif(cv::Mat &a,cv::Mat &b)
{
	int x,y,d;
	int sum=0;
	unsigned char *p=a.data;
	unsigned char *q=b.data;
	for(y=0;y<a.rows;y++)
	for(x=0;x<a.cols;x++,p++,q++) {
		d= ((int)*q)-((int)*p);
		if( d<0 ) sum-=d; else sum+=d;
	}
	return sum;
}



//
// notre plugin pour la comparaison
//
// genere un /go en [2] (pour declancher le sync et la capture de photo)
// recoit le pattern et la camera dans [0]
// re-trigger au besoin la camera (en [3])
// compare et envoit la difference dans un message sur [4]
//
// out: /difference <float:delai en sec> <int diff1> <int diff2>
//
//
class pluginCompare : public plugin<blob> {
	cv::Mat posRef;
	cv::Mat negRef;
	int n; // a quelle image on est rendu?
	int delai; // delai courant?


	cv::Mat dif;
  public:
	pluginCompare() { }
	~pluginCompare() { }
	void init() {
		n=-2; // on va prendre -2 et -1 pour posref et negref.
		delai=100000; // 150000
	}
	void uninit() {
	}
	bool loop() {
		// a chaque loop, on fait un projection-capture.
		message *m1=getMessage(256);
		*m1 << osc::BeginBundleImmediate;
		*m1 << osc::BeginMessage("/delay");
		if( n<0 )	*m1 << 1.0f;
		else		*m1 << (float)(delai/1000000.0);
		*m1 <<osc::EndMessage;
		*m1 << osc::BeginMessage("/go") <<osc::EndMessage;
		*m1 << osc::EndBundle;

		// tell sync to let the next pattern through
		push_front(2,&m1);

		// attend le retour de l'image affichee
		blob *pat=pop_front_wait(0);

		// attend le retour de l'image camera (loop si bad frame)
		blob *cam;
		for(;;) {
			// ramasse l'image du pattern et de la camera
			cam=pop_front_wait(0);
			if( cam->get()>=0 ) break; // status say image ok?
			cout << "re-trigg image" <<endl;
			recyclable::recycle((recyclable **)&cam);
			m1=getMessage(256);
			*m1 << osc::BeginMessage("/set") << "" << "TriggerSoftware" << osc::EndMessage;
			usleep(100000); // cam requires a delai before re-triggering...
			push_front(3,&m1);
		}

		// process les images
		if( n==-2 ) {
			posRef=cam->clone();
			cv::imwrite("test-pos.png",posRef);
		}else if( n==-1 ) {
			negRef=cam->clone();
			cv::imwrite("test-neg.png",negRef);
		}else{
			// compare!
			int r1,r2;
			if( n%2==0 ) {
				r1=getdif(*cam,posRef);
				r2=getdif(*cam,negRef);
			}else{
				r1=getdif(*cam,negRef);
				r2=getdif(*cam,posRef);
			}
			//printf("CMP %5d %7d %12d %12d\n",n,delai,r1/1000,r2/1000);


			m1=getMessage(256);
			*m1 << osc::BeginMessage("/difference");
			*m1 << (float)(delai/1000000.0);
			*m1 << (int)(r1/1000);
			*m1 << (int)(r2/1000);
			*m1 <<osc::EndMessage;
			push_back(4,&m1);

			/*
			if( (delai%10000)==0 ) {
				char buf[100];
				sprintf(buf,"test-cam-%06d.png",delai);
				cv::imwrite(buf,*cam);
			}
			*/
		}
		n++;
		if( delai>100000 ) delai-=500;
		else delai-=50;

		// recycle le tout
		recyclable::recycle((recyclable **)&pat);
		recyclable::recycle((recyclable **)&cam);
		if( delai<0 ) {
			// on averti le master parce qu'il va attendre stupidement
			// un message sans verifier le status...
			m1=getMessage(256);
			*m1 << osc::BeginMessage("/done") << osc::EndMessage;
			push_back(4,&m1);
			cout << "CMP sent /done"<<endl;
		}
		return true; //delai>0;
	}
};


struct info { float delai;int r1,r2; };


void go() {
	imqueue Qrecycle1("recycle1");
	imqueue Qrecycle2("recycle2");
	imqueue Qsync("sync");
	imqueue Qdisplay("display");
	imqueue Qcompare("compare");
	imqueue Qmsgcamera("msgcamera");
	imqueue Qmsgpattern("msgpattern");
	imqueue Qmsgsync("msgsync");
	imqueue Qmsgresult("msgresult");

	pluginSync S;
	pluginProsilica C;
	pluginViewerGLFW V;
	pluginPattern P;
	pluginCompare CMP;

	Qmsgcamera.syncAs(&Qrecycle1);
	Qmsgpattern.syncAs(&Qrecycle2);
	Qmsgsync.syncAs(&Qsync);

	// ajoute quelques images a recycler
	for(int i=0;i<10;i++) {
		blob *image=new blob();
		image->setRecycleQueue(&Qrecycle1);
		Qrecycle1.push_front(image);
	}
	for(int i=0;i<10;i++) {
		blob *image=new blob();
		image->setRecycleQueue(&Qrecycle2);
		Qrecycle2.push_front(image);
	}

#ifdef SKIP
	//
	// test de syntaxe de connection
	//
	/Q-new recycle1
	/Q-new recycle2
	/Q-new sync
	/Q-new display
	/Q-new compare
	#/Q-new msgcamera
	/Q-new msgpattern
	/Q-new msgsync
	/Q-new msgresult

	/P-new/Sync S
	/P-new/Prosilica C
	/P-new/ViewerGLFW V
	/P-new/Pattern P
	/P-new/compare CMP

	/P/C/setQ recycle1 0
	/P/C/setQ compare 1
	/P/C/setQ msgcamera 2
	...

	recycle1 -> C:0
	C:1,V:1 => CMP:0   (compare)
	S:3,CMP:3 => C:2    (msgcamera)
	recycle2 -> P:0
	msgpattern -> P:2
	P:1 => S:0  (sync)
	CMP:2 => S:2 (msgsync)
	S:1 => V:0 (display)
	CMP:4 -> msgresult

	...

	/connect [P1:p1,P2:p2,...] [Pn:pn,...] qname

	connect(P1,port1,P2,port2,[optional] Qname)

	A...

	A,B,C => D  -> une seule queue...
	A => D[0]  -> nouvelle queue
	B => D[0]  -> reutilise la queue 
	A[0],B[0] => D[0] une seule queue
	A[0] => E[0] -> reutilise la queue de A[0]

	A,B,C => D,E
	A,B,C => D
	A,B,C =cam=> E
	A => D
	B => E -> queue differente... aie.
	A => E
	B => D
	...

	recycle1 -> C[in]
	C[out],V[out] =(Qcmp)=> CMP[in]
	S[camtrig],CMP[camtrig] => C[inmsg]
	recycle2 -> P[in]
	msgpattern -> P[inmsg]
	P[out] => S[in]
	CMP[outtrig] => S[inmsg]
	S[out] => V[in]
	CMP[result] -> msgresult

	recycle1 ->[in] C [out]=>[in] CMP
	P [out]=>[in] S [out]=>[in] V [out]=>[in] CMP
	S [camtrig]=>[inmsg] C
	CMP[camtrig] => C[inmsg]
	recycle2 -> P[in]
	msgpattern -> P[inmsg]
	CMP[outtrig] => S[inmsg]
	CMP[result] -> msgresult


#endif



	//
	// connecte les queues avec les plugins
	//

	// camera
	C.setQ(0,&Qrecycle1);
	C.setQ(1,&Qcompare);
	C.setQ(2,&Qmsgcamera);

	// pattern
	P.setQ(0,&Qrecycle2);
	P.setQ(1,&Qsync);
	P.setQ(2,&Qmsgpattern);

	// sync
	S.setQ(0,&Qsync);
	S.setQ(1,&Qdisplay);
	S.setQ(2,&Qmsgsync);
	S.setQ(3,&Qmsgcamera);

	// viewer
	V.setQ(0,&Qdisplay);
	V.setQ(1,&Qcompare);

	// compare
	CMP.setQ(0,&Qcompare);
	CMP.setQ(2,&Qmsgsync);
	CMP.setQ(3,&Qmsgcamera);
	CMP.setQ(4,&Qmsgresult);

	//
	// Viewer
	//

	message *m=new message(1024);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
	//*m << osc::BeginMessage("/new-window/main") << 0 << 0 << 640 << 480 << false << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::EndBundle;
	Qdisplay.push_back(m);

	//
	// camera
	//

	m=new message(1024);
        *m << osc::BeginBundleImmediate;
        *m << osc::BeginMessage("/open") << "" << "/main/A/tex" << osc::EndMessage;
        //*m << osc::BeginMessage("/set") << "" << "Width" << 1360 << osc::EndMessage;
        //*m << osc::BeginMessage("/set") << "" << "Height" << 1024 << osc::EndMessage;
	// after "start-capture", you cant change image parameters (width, height, format,.)
        *m << osc::BeginMessage("/set") << "" << "BandwidthControlMode" << osc::Symbol("StreamBytesPerSecond") << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "StreamBytesPerSecond" << 50000000 << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "StreamFrameRateConstrain" << true << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "ExposureTimeAbs" << 10000 << osc::EndMessage;
        *m << osc::BeginMessage("/start-capture") << "" << osc::EndMessage;
/*
        *m << osc::BeginMessage("/set") << "" << "AcquisitionFrameRateAbs" << 100.0f << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
*/
        *m << osc::BeginMessage("/manual") << "" << osc::EndMessage;
        //*m << osc::BeginMessage("/auto") << "" << 10.0f << osc::EndMessage;
        *m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
        *m << osc::BeginMessage("/start-acquisition") << "" << osc::EndMessage;
	
        *m << osc::EndBundle;
        Qmsgcamera.push_back(m);

	//
	// pattern
	//
	m=new message(1024);
        *m << osc::BeginBundleImmediate;
        *m << osc::BeginMessage("/set/checkerboard") << 480 << 270 << 16 <<osc::EndMessage;
        *m << osc::BeginMessage("/pid") << "leopard:32" << osc::EndMessage;
        //*m << osc::BeginMessage("/manual") << osc::EndMessage;
        *m << osc::EndBundle;
        Qmsgpattern.push_back(m);

	//
	// sync
	//
	m=new message(1024);
        *m << osc::BeginBundleImmediate;
        *m << osc::BeginMessage("/manual") <<osc::EndMessage;
	// event -2 happen "delai"  after an image is sent. -1 is for no delay
	*m << osc::BeginMessage("/defevent") << -2 << 3 << "/set" << "" << "TriggerSoftware" << osc::EndMessage;
        *m << osc::EndBundle;
        Qmsgsync.push_back(m);


	//
	// mode full automatique
	//

	C.start();
	P.start();
	V.start();
	CMP.start();
	S.start();

	// rien a faire pendant 20*2 secondes...
	/*
	for(int i=0;;i++) {
		sleep(1);
		if( !S.status() ) break;
		if( !C.status() ) break;
		if( !P.status() ) break;
		if( !V.status() ) break;
		if( !CMP.status() ) break;
	}
	*/
	vector<info> stats;

	bool done=false;
	// on recoit les resultats
	for(int i=0;;i++) {
		message *m=(message *)Qmsgresult.pop_front_wait();
		m->decode(); // into msgs.

		for(int j=0;j<m->msgs.size();j++) {
			osc::ReceivedMessage *rm=&(m->msgs[j]);
			//cout << "Main got msg : "<< results[j] << endl;
			const char *address=rm->AddressPattern();
			if( oscutils::endsWith(address,"/difference") ) {
				info f;
				//try {
					rm->ArgumentStream() >> f.delai >> f.r1 >> f.r2 >> osc::EndMessage;
					stats.push_back(f);
				//} catch( osc::WrongArgumentTypeException ) {
				//	cout << "YO! wrong arg!!!!"<<endl;
				//}
			}else if( oscutils::endsWith(address,"/done") ) {
				done=true;
			}
		}
		// after m is recycled, the messages in the result vector are unusable.
		recyclable::recycle((recyclable **)&m);

		if( done ) break;
		if( !S.status() ) break;
		if( !C.status() ) break;
		if( !P.status() ) break;
		if( !V.status() ) break;
		if( !CMP.status() ) break;
	}



	// output statistics
	ofstream F;
	F.open("stats.m");
	F << "m={"<<endl;
	for(int i=0;i<stats.size();i++) {
		F << "{"<<stats[i].delai<<","<<stats[i].r1<<","<<stats[i].r2<<"}";
		if( i<stats.size()-1) F << ",";
		F << endl;
	}
	F << "};"<<endl;
	F.close();




	cout << "***** STOP CMP ******"<<endl;
	CMP.stop();
	cout << "***** STOP C ******"<<endl;
	C.stop();
	cout << "***** STOP P ******"<<endl;
	P.stop();
	cout << "***** STOP V ******"<<endl;
	V.stop();
	cout << "***** STOP S ******"<<endl;
	S.stop();
}

int main() {

	profilometre_init();

	cout << "opencv version " << CV_VERSION << endl;
	go();

	profilometre_dump();
}


