//
// detectTag
//
// plugins used:  Camera, ViewerGLFW, et plugin local
//

#include "imgv/imgv.hpp"

#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/pluginProsilica.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

#include <imgv/imqueue.hpp>
#include "config.hpp"


using namespace std;
using namespace cv;

//#define USE_PROFILOMETRE
#ifdef USE_PROFILOMETRE
 #include <profilometre.h>
#endif

/*
Point2f getCenter(
			center.x= (rect_points[0].x+
				rect_points[1].x+
				rect_points[2].x+
				rect_points[3].x)/4.0;
			center.y= (rect_points[0].y+
				rect_points[1].y+
				rect_points[2].y+
				rect_points[3].y)/4.0;
*/

//
// plugin local
//
class pluginLocal : public plugin<blob> {
	Mat i0;
	Mat i1;
	Mat i2;
	int n;
	public:
	pluginLocal() { }
	~pluginLocal() { }
	void init() { n=0; }
	void uninit() {}
	bool loop() {
		// lit dans [0]
		blob *img=pop_front_wait(0);

        	//Mat i0;
		//cv::cvtColor(i0g,i0, CV_GRAY2BGR);

		//imshow("source",*img); waitKey(1);

		//Mat i1 = (i0g >= 150);
		adaptiveThreshold(*img, i1, 255.0, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY,7, 10.0);

		cv::cvtColor(i1,i0, CV_GRAY2BGR);

		imshow("seuil",i1); waitKey(1);

        	vector<vector<Point> > contours;

        	vector<Vec4i> hierarchy;
        	findContours(i1, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

		vector<RotatedRect> minEllipse( contours.size() );

		// niveau des contours...
		int niveau[contours.size()];
		for(int i=0;i<contours.size();i++) niveau[i]=-2; // inconnu

		// hierarchy contient [0,1,2,3] = [next,previous,child,parent] -1 
		bool fini=false;
		while( !fini ) {
			fini=true;
			for(int i=0;i<contours.size();i++) {
				if( niveau[i]>=-1 ) continue; // contour pas bon ou connu
				if( contours[i].size()<10 ) { niveau[i]=-1;continue; }
				Vec4i v = hierarchy[i];
				//cout << "contour "<<i<<" : "<<v[0]<<","<<v[1]<<","<<v[2]<<","<<v[3]<<endl;
				if( v[3]==-1 ) {niveau[i]=0;fini=false;} // root
				else{
					if( niveau[v[3]]>=0 ) {
						niveau[i]=niveau[v[3]]+1;
						fini=false;
					}
				}
			}
		}

		// 3 : points interieurs des patterns
		// 2 : cercle interieur du pattern
		// 1 : cercle exterieur du pattern
		// 0 : tour de l'image
		// -> le pattern peut etre plus profond, mais pas moins...

		// pour chaque point de niveau >=3, on va verifie:
		// nombre de frere au meme niveau avec le meme parent = 1..7
		// le centre du parent tombe a l'interieur d'un des enfants
		// le ratio en chaque enfant et le parent doit etre similaire
		// il doit y avoir une simmetrie entre les enfants...
		// la moyenne des centres des enfant doit etre le centre d'un des enfants.

		i2=i0;
		i2=Scalar(0,0,0);
		if( n==10 ) cout << "nb contour = "<<contours.size()<<endl;
		for(int i=0;i<contours.size();i++) {

			if( niveau[i]<2 ) continue;
			// quelques statistques...

			srand(niveau[i]+145);
			Vec3b clr(rand()%256,rand()%256,rand()%256);
			//cout << "shape "<<i<<" has "<<contours[i].size()<<endl;
			Vec4i v = hierarchy[i];
			//cout << "N="<<niveau[i]<<"    H: "<< v[0]<<","<<v[1]<<","<<v[2]<<","<<v[3]<<endl;
			/*
			for(int j=0;j<contours[i].size();j++) {
				Point p=contours[i][j];
				//cout << "pts["<<j<<"] = "<<p<<endl;
				// pas au bon endroit... facteur en x...
				i2.at<Vec3b>(p.y,p.x)=clr;//Vec3b(255,255,0);  //x*1.33
			}
			*/
			minEllipse[i] = fitEllipse( Mat(contours[i]) );
			//ellipse( i2, minEllipse[i], Scalar(clr[0],clr[1],clr[2]), 2, 8 );

			Point2f rect_points[4];
			minEllipse[i].points( rect_points );

/*
			Point2f center;
			i2.at<Vec3b>(center.y,center.x)=clr;//Vec3b(255,255,0);  //x*1.33
*/

			if( n==10 ) {
				cout << "contour ["<<i<<"] has size "<<contours[i].size()<<" and parent = "<<v[3]<<endl;
				// affiche les enfants
				for(int k=v[2];k!=-1;k=hierarchy[k][0]) {
					if( niveau[k]<niveau[i] ) continue;
					cout << "   child is ["<<k<<"] with size "<<contours[k].size()<<" which has parent= "<<hierarchy[k][3]<<endl;
				}
			}

			for( int k = 0; k < 4; k++ ) {
				line( i2, rect_points[k], rect_points[(k+1)%4], Scalar(clr[0],clr[1],clr[2]), 1, 8 );
			}

			drawContours( i2, contours, i, Scalar(clr[0],clr[1],clr[2]), 1, 8 );
		}

		imshow("tags",i2); waitKey(1);

		// envoie dans [1]
		push_back(1,&img);

		n++;

		// on tourne a l'infini
		return true;
	}
};



// video: true = save as a video, false=save images with %d in the image name
void single_camera() {
	imqueue Qrecycle("recycle");
	imqueue Qprocess("process");
	imqueue Qdisplay("display");
	imqueue Qmsgcamera("msgcamera");

	Qmsgcamera.syncAs(&Qrecycle);

	// ajoute quelques images a recycler
	for(int i=0;i<10;i++) {
		blob *image=new blob();
		image->set(i);
		image->setRecycleQueue(&Qrecycle);
		Qrecycle.push_front(image);
	}


	//
	// viewer
	//
	pluginViewerGLFW V;

	message *m=new message(1042);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/new-monitor/main") << osc::EndMessage;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent") << 'A' << 2 << "/set" << "" << "AcquisitionFrameRateAbs" << 15.0f << osc::EndMessage;
	*m << osc::BeginMessage("/defevent") << 'B' << 2 << "/set" << "" << "AcquisitionFrameRateAbs" << 1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/defevent") << 'C' << 2 << "/set" << "" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent") << 'D' << 2 << "/set" << "" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent") << 'Z' << 2 << "/set" << "" << "TriggerSoftware" << osc::EndMessage;
	*m << osc::EndBundle;
	// add message to Qdisplay, so the viewer can get it only after it is started.
	Qdisplay.push_back(m);


	//
	// PLUGIN: Prosilica
	//
	pluginProsilica C;

	//
	// plugin local pour processing
	//
	pluginLocal L;

	//
	// connexions entre les plugins (par les rqueues)
	//
	// on peut utiliser directement le # de port,
	// ou donne un nom de port (varie selon le plugin)

	// camera
	C.setQ(0,&Qrecycle);
	C.setQ(1,&Qprocess);
	C.setQ(2,&Qmsgcamera);

	// local
	L.setQ(0,&Qprocess);
	L.setQ(1,&Qdisplay);

	// viewer
	V.setQ(0,&Qdisplay);

	//
	// mode full automatique
	//

	m=new message(1024);
        *m << osc::BeginBundleImmediate;
        *m << osc::BeginMessage("/open") << "" << "/main/A/tex" << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "Width" << 1360 << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "Height" << 1024 << osc::EndMessage;
	// after "start-capture", you cant change image parameters (width, height, format,.)
        *m << osc::BeginMessage("/start-capture") << "" << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "AcquisitionFrameRateAbs" << 10.0f << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "TriggerMode" << osc::Symbol("On") << osc::EndMessage;
        *m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Freerun") << osc::EndMessage;
        //*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("FixedRate") << osc::EndMessage;
        //*m << osc::BeginMessage("/set") << "" << "TriggerSource" << osc::Symbol("Software") << osc::EndMessage;
        *m << osc::BeginMessage("/reset-timestamp") << osc::EndMessage;
        *m << osc::BeginMessage("/start-acquisition") << "" << osc::EndMessage;
	
        *m << osc::EndBundle;
        Qmsgcamera.push_back(m);


	C.start();
	L.start();
	V.start();

	// rien a faire pendant 20*2 secondes...
	for(int i=0;;i++) {
		sleep(1);
		if( !C.status() ) break;
		if( !V.status() ) break;
	}

	C.stop();
	L.stop();
	V.stop();
}

int main(int argc,char *argv[]) {

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			printf("Usage: %s\n",argv[0]);
			exit(0);
		}
	}

#ifdef USE_PROFILOMETRE
	profilometre_init();
#endif

	cout << "opencv version " << CV_VERSION << endl;
	single_camera();

#ifdef USE_PROFILOMETRE
	profilometre_dump();
#endif
}


