//
// playSuperMapper
//
// example for sample plugins

//
// plugins used:  ViewerGLFW
//

/*!

\defgroup playSuperMapper playSuperMapper
@{
\ingroup examples

Do mapping with multiple quads in multiple windows, for multiple sources and multiple content.

Usage
-----------

just testing for now.
~~~
playSuperMapper
~~~

Display the images provided on the command line.
~~~
playSuperMapper toto.png
~~~

Plugins used
------------
- \ref pluginViewerGLFW
- \ref pluginHomog

@}

*/

#include <imgv/imgv.hpp>
//#include <oscpack/osc/OscTypes.h>

#include <sys/stat.h>

#include <imqueue.hpp>

#include <imgv/qpmanager.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

using namespace std;
using namespace cv;

bool quit;

// 1.0f si on ne fait pas le fade nous meme
#define DEFAULT_FADE 1.0f

// pour les touches left/right/up/down
#define KEYSTEP	0.005

/*
#define VIDEO1 "/home/vision/Videos/FinalMP4/TF2MP4.mp4"
#define VIDEO2 "/home/vision/Videos/FinalMP4/TFROTMP4.mp4"
*/
#define VIDEO1 "/home/roys/Videos/FinalMP4/TF2MP4.mp4"
#define VIDEO2 "/home/roys/Videos/FinalMP4/TFROTMP4.mp4"

//
// utilise une homographie pour la position du quad (quad projecteur)
// utilise une homographie pour le contenu texture (quad source)
//
#define SHADER_SOURCE "homographieQuad"
#define SHADER_PROJ "homographieQuadLut"

class quadinfo {
	public:
	string qname; // 'A', 'B', etc...
	string wname; // 'proj1'  -> nom du projecteur ou de la source
	string fullname; // 'boite 1A' ou 'ecran' etc...
	float px[4];
	float py[4];
	//float qx[4];
	//float qy[4];

	quadinfo(string q,string w,string full) {
		qname=q;
		wname=w;
		fullname=full;
		//qx[0]=0; qx[1]=1; qx[2]=1; qx[3]=0;
		//qy[0]=0; qy[1]=0; qy[2]=1; qy[3]=1;
	}

	void dump() {
		cout << "  quad "<<wname<<"-"<<qname<< "  ("<<fullname<<")"<<endl;
	}

	int findClosestPoint(double x,double y) {
		double L2;
		double L2min=9999.0;
		int imin=-1;
		for(int i=0;i<4;i++) {
			L2=(px[i]-x)*(px[i]-x)+(py[i]-y)*(py[i]-y);
			if( imin<0 || L2<L2min ) { L2min=L2;imin=i; }
		}
		return imin;
	}

	double distance2From(int p,double x,double y) { return ((px[p]-x)*(px[p]-x)+(py[p]-y)*(py[p]-y)) ; }

	// update le points p du quad i avec les points x,y
	void updatePoints(int p,float x,float y) {
		px[p]=x;
		py[p]=y;
		// send info to homo
		string tag=wname+"-"+qname;
		message *m0=new message(1024);
		*m0 << osc::BeginBundleImmediate;
		*m0 << osc::BeginMessage("/set/q") << tag << (int)p << x << y << osc::EndMessage;
		*m0 << osc::EndBundle;
		qpmanager::add("recycle",m0);
		// tell the interface about this point
		cout << "************************** "<<x<<","<<y<<endl;
		m0=new message(1024);
		*m0 << osc::BeginMessage("/set/quad/point") << wname << qname << fullname << (int)p << x << y << osc::EndMessage;
		qpmanager::add("netOut",m0);
	}

};

vector<quadinfo> quads;


int findQuad(string qname,string wname) {
	int i;
	for(i=0;i<quads.size();i++) {
		if( quads[i].qname==qname && quads[i].wname==wname ) return i;
	}
	return -1;
}

// for quads sharing wname with quad[i], make all points closet than radius from (x,y) = (x,y)
void snapQuads(string wname,float rad,float x,float y) {
	double d2,d2min=rad*rad;
	for(int i=0;i<quads.size();i++) {
		if( quads[i].wname!=wname ) continue;
		int p=quads[i].findClosestPoint(x,y);
		d2=quads[i].distance2From(p,x,y);
		if( d2<d2min ) quads[i].updatePoints(p,x,y);
	}
	
}


// utilise le nom complet du quad
int findQuadFromName(string fullname,string wname) {
	int i;
	for(i=0;i<quads.size();i++) {
		if( quads[i].fullname==fullname && quads[i].wname==wname ) return i;
	}
	return -1;
}

int dumpQuads() {
	for(int i=0;i<quads.size();i++) { cout <<"------ "<<i;quads[i].dump(); }
	
}


class presetinfo {
	public:
	string preset; // nom du preset
	string proj;
	string projQuad;
	string projQuadUni;
	string src;
	string srcQuad;
	int sourceActivated; // 1 si src est deja activee
};

vector<presetinfo> presets;


//
// load une image et place la dans une queue d'images
//
void loadImg(string nom,string view,string qname)
{
        cv::Mat image=cv::imread(nom,CV_LOAD_IMAGE_UNCHANGED);
	if( image.data==NULL ) { cout << "*********************************************** "<<nom<<endl; return; }
	cout << "**************** image "<<nom<<" channels is " << image.channels() << endl;
        blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
        i1->view=view;
        qpmanager::add(qname,i1);
}
void loadOverlay(string text,string view,string qname)
{
	Mat image(240,320,CV_8UC4,Scalar(255,255,0,0));
	cout << "**************** overlay image "<<text<<" channels is " << image.channels() << endl;
	
        blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.

	Scalar blanc(255,0,255,255);
	Point pos1(20, 40);
	putText(image, text, pos1, FONT_HERSHEY_SIMPLEX, 1.0, blanc, 2,8);

	// send
        i1->view=view;
        qpmanager::add(qname,i1);
}

void loadText(string text,string extraText,Scalar fg,Scalar bg,string view,string qname)
{
	Mat image(240,320,CV_8UC3,bg);
        blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.

	int fontFace = FONT_HERSHEY_SIMPLEX;
	double fontScale = 2;
	int thickness = 4;  

	int baseline=0;
	Size textSize = getTextSize(text, fontFace, fontScale, thickness, &baseline);
	baseline += thickness;
	Size textSizeExtra = getTextSize(extraText, fontFace, fontScale, thickness, &baseline);

	// ... and the baseline first
	line(image, Point(0,0), Point(image.cols-1,image.rows-1), Scalar(128,128,128), thickness);
	line(image, Point(image.cols-1,0), Point(0,image.rows-1), Scalar(128,128,128), thickness);

	// center the text
	Point textOrg((image.cols - textSize.width)/2, (image.rows + textSize.height)/2);
	putText(image, text, textOrg, fontFace, fontScale, fg, thickness,8);

	// extra text
	Point textOrgExtra((image.cols - textSizeExtra.width)/2, (image.rows + textSizeExtra.height)/2+textSize.height);
	putText(image, extraText, textOrgExtra, fontFace, fontScale, fg, thickness,8);

	Scalar blanc(255,255,255);
	// les coins, 1,2,3,4
	//Point pos1((image.cols - textSize.width)/2, (image.rows + textSize.height)/2);
	Point pos1(thickness, thickness+textSize.height);
	Point pos2(image.cols-thickness-(textSize.width)/2, thickness+textSize.height);
	Point pos3(image.cols-thickness-(textSize.width)/2, image.rows-thickness-textSize.height/2);
	Point pos4(thickness, image.rows-thickness-textSize.height/2);
	putText(image, "1", pos1, fontFace, 1, blanc, 2,8);
	putText(image, "2", pos2, fontFace, 1, blanc, 2,8);
	putText(image, "3", pos3, fontFace, 1, blanc, 2,8);
	putText(image, "4", pos4, fontFace, 1, blanc, 2,8);

	// draw the box
	rectangle(image, Point(thickness,thickness), Point(image.cols-1-thickness,image.rows-1-thickness), fg,thickness*2);

	// send
        i1->view=view;
        qpmanager::add(qname,i1);
}

//
// setup a network plugins to play with the interface
// this is outgoing only since the reception of command and images will go through qpmanager
//
void setupNetworkOutgoing(string host,int port) {
	qpmanager::addPlugin("N","pluginNet");
	qpmanager::addQueue("netOut");
	qpmanager::connect("netOut","N","in");
	qpmanager::addQueue("netCmd");
	qpmanager::connect("netCmd","N","cmd");
	
	message *m=new message(512);
	*m << osc::BeginMessage("/out/udp") << host << port << 0 << osc::EndMessage;
	qpmanager::add("netCmd",m);
	qpmanager::start("N");
}

void unsetupNetworkOutgoing() {
	qpmanager::stop("N");
}

//
// setup une fenetre
// from 'A' to 'C', par exemple...
// si c'est une source, on ajoute un quad "bg" qui va recevoir le contenu
// si c'est une destination (proj), on a pas besoin d'un bg
//
void setupWindow(string wname,int dx,int dy,int w,int h,bool source,bool bordure)
{
	message *m;
	//
	// create window
	//
	m=new message(1024);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/"+wname) << dx << dy << w << h << bordure << wname << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/"+wname+"/bg") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;

	*m << osc::EndBundle;
	qpmanager::add("display",m);
}

void unsetupWindow(string wname)
{
	message *m;
	m=new message(1024);
	*m << osc::BeginMessage("/del-window/"+wname) << osc::EndMessage;
	qpmanager::add("display",m);
}

//
// setup a quad inside a window, ready for homography
// une queue est nommee 'A' - 'Z'
//
// view : /source/A
// tag  : /tag/source-A
//
void setupHQuad(string wname,char qnameChar,string qnameExtraText,int red,int green,int blue,string Huniform,float x0,float y0,float x1,float y1,float x2,float y2,float x3,float y3,string shader) {
	string qname(1,qnameChar);
	string htag=wname+"-"+qname;
	string view="/"+wname+"/"+qname;
	message *m;
	m=new message(1024);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-quad"+view) << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << shader << osc::EndMessage;
	*m << osc::BeginMessage("/set"+view+"/fade") << 0.2f << osc::EndMessage;
	// set the initial homographies (homog et deform, mais c'est surtout homog qui est important...)
	// homog: deformation interieure (texture), deform : deformation exterieure du quad
	*m << osc::BeginMessage("/mat3"+view+"/homog") << 1.0f << 0.0f << 0.0f << 0.0f << 1.0f << 0.0f << 0.0f << 0.0f << 1.0f << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	// les tags sont <window>-<quad> pour etre unique

	m=new message(1024);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/tag")<<htag<<osc::EndMessage;

	*m << osc::BeginMessage("/set/out/lookup")<<false<<osc::EndMessage; // no lookup table needed
	*m << osc::BeginMessage("/set/out/matrix")<<true<<osc::EndMessage; // we only want the matrix
	// init homo a identite, initialiement
	// cest ce qui fait qu'on voit la texture au complet dans le quad.
	/*
	*m << osc::BeginMessage("/set/q") << 0 << 0.0f << 0.0f << osc::EndMessage;
	*m << osc::BeginMessage("/set/q") << 1 << 0.0f << 1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/set/q") << 2 << 1.0f << 1.0f << osc::EndMessage;
	*m << osc::BeginMessage("/set/q") << 3 << 1.0f << 0.0f << osc::EndMessage;
	*m << osc::BeginMessage("/set/out/matrix-cmd")<< "/mat3"+view+"/homog" <<osc::EndMessage; // we only want the matrix
	*/

	// les points...
	*m << osc::BeginMessage("/set/q") << 0 << x0 << y0 << osc::EndMessage;
	*m << osc::BeginMessage("/set/q") << 1 << x1 << y1 << osc::EndMessage;
	*m << osc::BeginMessage("/set/q") << 2 << x2 << y2 << osc::EndMessage;
	*m << osc::BeginMessage("/set/q") << 3 << x3 << y3 << osc::EndMessage;


	*m << osc::BeginMessage("/go")<<osc::EndMessage; // first compute
	*m << osc::BeginMessage("/set/out/matrix-cmd")<< "/mat3"+view+"/"+Huniform <<osc::EndMessage; // we only want the matrix
	*m << osc::BeginMessage("/go")<<osc::EndMessage; // first compute
	*m << osc::EndBundle;
	qpmanager::add("recycle",m);

	quadinfo qi(qname,wname,qnameExtraText);
	quads.push_back(qi);

	loadText(qname,qnameExtraText,Scalar(red,green,blue),Scalar(64,64,64),view+"/tex","display");
	//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg",view+"/tex","display");

	// envoie les points de depart...
	int i=findQuad(qname,wname);
	cout << "found quad at "<<i<<endl;
	quads[i].px[0]=x0; quads[i].py[0]=y0;
	quads[i].px[1]=x1; quads[i].py[1]=y1;
	quads[i].px[2]=x2; quads[i].py[2]=y2;
	quads[i].px[3]=x3; quads[i].py[3]=y3;
}


//
// setup a quad inside a window, to display an source of some material
// the quad is /meterial/video1 and the view for this will be /material/video1/tex
// this quad comes with a second overlay quad with a text.
// keys here will control the material (play stop, etc).
//
// from-to : horizontal spacing (0..1)
//
void setupMaterialImageQuad(string wname,string qname,float from,float to) {
	string view="/"+wname+"/"+qname;

	message *m;
	m=new message(1024);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-quad"+view) << from << to << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad"+view+"-over") << from << to << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	//*m << osc::BeginMessage("/set"+view+"/fade") << 0.5f << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	// start a content plugin
	// pour l'instant, le contenu est une image seulement
	//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg",view+"/tex","display");
	//loadImg(std::string(IMGV_SHARE)+"/images/trinket.png",view+"/tex","display");

	//loadText(qname,Scalar(255,255,0),Scalar(64,64,64),view+"/tex","display");
	loadOverlay(qname,view+"-over/tex","display");
	//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg",view+"/tex","display");
}

inline bool fileExists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0); 
}

#define TYPE_VIDEO	0
#define TYPE_IMAGE	1

void setupMaterialVideoQuad(string wname,int w,int h,int ox,int oy,string videoName,int type) {

	string view="/"+wname+"/bg";

	message *m;
	m=new message(1024);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/"+wname) << ox << oy << w << h << true << videoName << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad"+view) << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal"  << osc::EndMessage;
	//*m << osc::BeginMessage("/new-quad"+view) << from << to << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	//*m << osc::BeginMessage("/new-quad"+view+"-over") << from << to << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	//*m << osc::BeginMessage("/set"+view+"/fade") << 0.5f << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	// start a content plugin
	// pour l'instant, le contenu est une image seulement
	//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg",view+"/tex","display");
	//loadImg(std::string(IMGV_SHARE)+"/images/trinket.png",view+"/tex","display");

	//loadText(qname,Scalar(255,255,0),Scalar(64,64,64),view+"/tex","display");
	//loadOverlay(qname,view+"-over/tex","display");
	//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg",view+"/tex","display");

	if( type==TYPE_VIDEO ) {
		// joue un video...
		qpmanager::addPlugin("ffmpeg"+wname,"pluginFfmpeg");
		qpmanager::addQueue("ffmpeg-recycle-"+wname);
		qpmanager::addEmptyImages("ffmpeg-recycle-"+wname,20);
		qpmanager::start("ffmpeg"+wname);
		qpmanager::connect("ffmpeg-recycle-"+wname,"ffmpeg"+wname,"in");
		//qpmanager::connect("display","ffmpeg"+wname,"out");

		// controle le framerate
		qpmanager::addPlugin("drip"+wname,"pluginDrip");
		qpmanager::addQueue("drip-in-"+wname);
		qpmanager::start("drip"+wname);
		qpmanager::connect("drip-in-"+wname,"ffmpeg"+wname,"out");
		qpmanager::connect("drip-in-"+wname,"drip"+wname,"in");
		qpmanager::connect("display","drip"+wname,"out");

		// start the movie!
		m=new message(1024);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/max-fps") << 40.0f << osc::EndMessage;
		*m << osc::BeginMessage("/start") << videoName << "" << "rgb" << view+"/tex" << 0 << osc::EndMessage;
		// au cas ou l'estimation se trompe
		*m << osc::BeginMessage("/set/length") << -1 << osc::EndMessage;
		*m << osc::BeginMessage("/pause") << true << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("ffmpeg-recycle-"+wname,m);

		// start the dripper
		m=new message(1024);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/fps") << 30.0 << osc::EndMessage;
		*m << osc::BeginMessage("/start") << 0 << osc::EndMessage;
		*m << osc::BeginMessage("/pausenow") << true << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("drip-in-"+wname,m);



	}else if( type==TYPE_IMAGE ) {
		string fname=videoName;
		if( !fileExists(fname) ) {
			fname=std::string(IMGV_SHARE)+"/images/"+fname;
			if( !fileExists(fname) ) {
				fname=std::string(IMGV_SHARE)+"/images/trinket.png";
			}
		}
		loadImg(fname,view+"/tex","display");
	}

}





//
// initialize setup
//
void init() {
	message *m;

	// test si les plugins sont disponibles... en direct plutot qu'avec ifdef
	if( !qpmanager::pluginAvailable("pluginViewerGLFW")
	 || !qpmanager::pluginAvailable("pluginHomog") ) {
		cout << "Some plugins are not available... :-("<<endl;
		return;
	}

	// this is the main queue for managing the interface
	qpmanager::addQueue("map");

	//
	// plugin ViewerGLFW
	//


	qpmanager::addPlugin("V","pluginViewerGLFW");
	qpmanager::addQueue("display");
	qpmanager::connect("display","V","in");


	m=new message(4096);
	*m << osc::BeginBundleImmediate;
	// temporaire
	*m << osc::BeginMessage("/defevent-ctrl") << "0" << "toMain" << "/seek" << osc::Symbol("$window") << osc::EndMessage;
	// homographies
	*m << osc::BeginMessage("/defevent-ctrl") << "1" << "toMain" << "/set/q" << 0 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "2" << "toMain" << "/set/q" << 1 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "3" << "toMain" << "/set/q" << 2 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "4" << "toMain" << "/set/q" << 3 << osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "left" << "toMain" << "/left" <<  osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "down" << "toMain" << "/down" <<   osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") <<osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "up" << "toMain" << "/up" <<   osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "right" << "toMain" << "/right" <<   osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "=" << "toMain" << "/snap" <<   osc::Symbol("$xr") << osc::Symbol("$yr") << osc::Symbol("$window") << osc::EndMessage;
	for(char c='A';c<='Z';c++) {
		string cs(1,c);
		*m << osc::BeginMessage("/defevent-ctrl") << cs << "toMain" << "/tag" << cs << osc::Symbol("$window") << osc::EndMessage;
	}
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	//
	// plugin homographie
	//

	qpmanager::addQueue("recycle"); // useless unle you need lookup tables
	//qpmanager::addEmptyImages("recycle",5);

	qpmanager::addPlugin("H","pluginHomog");
	qpmanager::connect("recycle","H","in");
	qpmanager::connect("display","H","out"); // homography info goes direct to display

	qpmanager::reservePort("V","toMain");
	qpmanager::connect("map","V","toMain");

	//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg","/main/A/tex","display");
	
	qpmanager::start("V");
	qpmanager::start("H");
}

void uninit() {
	qpmanager::stop("V");
	qpmanager::stop("H");
}

//
// activate/deactivate a preset
// called on /preset/activate
//
void activatePreset(string preset,bool activate) {
	// vide les sourceActivated...
	for(int i=0;i<presets.size();i++) presets[i].sourceActivated=0;
	for(int i=0;i<presets.size();i++) {
		cout << "checking "<<presets[i].preset<< " and "<<preset<<endl;
		if( presets[i].preset==preset ) {
			cout << "active connexion "<<presets[i].proj<<":"<<presets[i].projQuad<<":"<<presets[i].projQuadUni<<" -> "<<presets[i].src<<":"<<presets[i].srcQuad<<endl;
			//
			// activate the connexion
			//
			int p=findQuadFromName(presets[i].projQuad,presets[i].proj);
			int s=findQuadFromName(presets[i].srcQuad,presets[i].src);
			string uni=presets[i].projQuadUni;
			if( p>=0 && s>=0 ) {
				string viewproj="/"+quads[p].wname+"/"+quads[p].qname;
				string viewsrc="/"+quads[s].wname+"/"+quads[s].qname;
				string viewsrcimg="/"+quads[s].wname+"/bg/tex";
				cout << "redirect "<<viewproj+"/homog"<<" to "<<viewsrc+"/deform"<<endl;
				cout << "redirect "<<viewproj+"/"+uni<<" to "<<viewsrcimg<<endl;
				message *m2=new message(1024);
				*m2 << osc::BeginBundleImmediate;
				message *m1=new message(1024);
				*m1 << osc::BeginBundleImmediate;
				message *m0=new message(1024);
				*m0 << osc::BeginBundleImmediate;
				// est-ce que cette source a deja ete activee?
				int alreadyActive=0;
				for(int j=0;j<i;j++) {
					if( presets[j].preset!=preset ) continue;
					if( presets[j].src!=presets[i].src ) continue;
					if( presets[j].sourceActivated ) alreadyActive=1;
				}
				// normalement, les /homog et /fade sont seulement une fois...
				// devraient aller dans activated==0
				if( activate ) {
					// pour le display
					*m0 << osc::BeginMessage("/redirect") << viewproj+"/homog" << viewsrc+"/deform" << osc::EndMessage;
					*m0 << osc::BeginMessage("/redirect") << viewproj+"/"+uni << viewsrcimg << osc::EndMessage;
				}else{
					*m0 << osc::BeginMessage("/redirect") << viewproj+"/homog" << osc::EndMessage;
					*m0 << osc::BeginMessage("/redirect") << viewproj+"/"+uni << osc::EndMessage;
				}
				*m0 << osc::BeginMessage("/set"+viewproj+"/fade") << DEFAULT_FADE << osc::EndMessage;
				*m0 << osc::EndBundle;
				qpmanager::add("display",m0);

				if( alreadyActive==0 ) {
				if( activate ) {
					// pour le video
					*m1 << osc::BeginMessage("/rewind") << osc::EndMessage;
					*m1 << osc::BeginMessage("/pause") << false << osc::EndMessage;
					// on ne devrait pas flusher ?
					// pour le drip
					*m2 << osc::BeginMessage("/flush") << osc::EndMessage;
					*m2 << osc::BeginMessage("/reset") << osc::EndMessage;
					*m2 << osc::BeginMessage("/start") << 1 << osc::EndMessage;
				}else{
					*m1 << osc::BeginMessage("/pause") << true << osc::EndMessage;
					*m2 << osc::BeginMessage("/pausenow") << true << osc::EndMessage;
				}
				//
				// si c'est un video... redemarrer le video, sinon pause...
				//
					*m1 << osc::EndBundle;
					qpmanager::add("ffmpeg-recycle-"+quads[s].wname,m1);
					*m2 << osc::EndBundle;
					qpmanager::add("drip-in-"+quads[s].wname,m2);
					// marque source comme activee
					presets[i].sourceActivated=1;
				}

			}else{
				cout << "impossible de trouver les quad a connecter"<<endl;
			}
		}
	}
}


// update le points p du quad i avec les points x,y
void updateQuadPoints(int i,int p,float x,float y) {
	quads[i].px[p]=x;
	quads[i].py[p]=y;
	// send info to homo
	message *m0=new message(1024);
	*m0 << osc::BeginBundleImmediate;
	*m0 << osc::BeginMessage("/set/q") << (int)p << x << y << osc::EndMessage;
	*m0 << osc::EndBundle;
	qpmanager::add("recycle",m0);
	// tell the interface about this point
	cout << "************************** "<<x<<","<<y<<endl;
	m0=new message(1024);
	*m0 << osc::BeginMessage("/set/quad/point") << quads[i].wname << quads[i].qname << quads[i].fullname << (int)p << (float)x << (float)y << osc::EndMessage;
	qpmanager::add("netOut",m0);
}

//
// relais des messages de l'interface
//

string currentQuad; // last quad
string currentWin;

class myDecoder : public messageDecoder {
    bool decode(const osc::ReceivedMessage& m) {
	cout << "*** got "<<m<<endl;
	const char *address=m.AddressPattern();
	int nb=oscutils::extractNbArgs(address);

	if( oscutils::endsWith(address,"/tag") ) {
		//
		// /tag is received from letters... a,b,c,d,... to select the current quad in a window
		//
		// /tag <quad> <window>
		const char *quad,*win;
		m.ArgumentStream() >> quad >> win >> osc::EndMessage;
		string tag=string(win)+"-"+string(quad);
		currentQuad=quad;
		currentWin=win;
		cout << "** -> quad " << quad << "  win " << win << " : " << tag << endl;
		// annonce au homog le tag courant
		message *m0=new message(1024);
		*m0 << osc::BeginMessage("/tag") << tag << osc::EndMessage;
		qpmanager::add("recycle",m0);
		// set fade values to all quad
		m0=new message(4096);
		*m0 << osc::BeginBundleImmediate;
		for(int i=0;i<quads.size();i++) {
			// si projecteur -> 1 / 0.6, sinon 0.7 et 0.5
			float f=0.7;
			if( quads[i].wname==win && quads[i].qname!=quad ) f=0.5;
			*m0 << osc::BeginMessage(string("/set/")+quads[i].wname+"/"+quads[i].qname+"/fade")
			    << f << osc::EndMessage;
		}
		*m0 << osc::EndBundle;
		qpmanager::add("display",m0);
		// tell outside
		m0=new message(1024);
		*m0 << osc::BeginMessage("/yo") << tag << osc::EndMessage;
		qpmanager::add("netOut",m0);
	}else if( oscutils::endsWith(address,"/set/q") ) {
		//
		// this is received when pressing 1,2,3,4 inside a window and apply to the current tag
		// /set/q 0 0.22 0.34 proj1
		int32 p;
		float x,y;
		const char *win;
		m.ArgumentStream() >> p >> x >> y >> win >> osc::EndMessage;
		// update quad info
		if( win==currentWin ) {
			int i=findQuad(currentQuad,win);
			cout << "found quad at "<<i<<endl;
			updateQuadPoints(i,p,x,y);
		}
	}else if( oscutils::endsWith(address,"/left") ) {
		const char *win;
		float x,y;
		m.ArgumentStream() >> x >> y >> win >> osc::EndMessage;
		cout << "left win="<<win<<"  currentwin="<<currentWin<<endl;
		if( win==currentWin ) {
			// we have a tag ... so we move the closest point
			int i=findQuad(currentQuad,win);
			cout << "found quad at "<<i<<endl;
			int j=quads[i].findClosestPoint(x,y);
			updateQuadPoints(i,j,quads[i].px[j]-KEYSTEP,quads[i].py[j]);
		}
	}else if( oscutils::endsWith(address,"/right") ) {
		const char *win;
		float x,y;
		m.ArgumentStream() >> x >> y >> win >> osc::EndMessage;
		cout << "left win="<<win<<"  currentwin="<<currentWin<<endl;
		if( win==currentWin ) {
			// we have a tag ... so we move the closest point
			int i=findQuad(currentQuad,win);
			cout << "found quad at "<<i<<endl;
			int j=quads[i].findClosestPoint(x,y);
			updateQuadPoints(i,j,quads[i].px[j]+KEYSTEP,quads[i].py[j]);
		}
	}else if( oscutils::endsWith(address,"/up") ) {
		const char *win;
		float x,y;
		m.ArgumentStream() >> x >> y >> win >> osc::EndMessage;
		cout << "left win="<<win<<"  currentwin="<<currentWin<<endl;
		if( win==currentWin ) {
			// we have a tag ... so we move the closest point
			int i=findQuad(currentQuad,win);
			cout << "found quad at "<<i<<endl;
			int j=quads[i].findClosestPoint(x,y);
			updateQuadPoints(i,j,quads[i].px[j],quads[i].py[j]-KEYSTEP);
		}
	}else if( oscutils::endsWith(address,"/down") ) {
		const char *win;
		float x,y;
		m.ArgumentStream() >> x >> y >> win >> osc::EndMessage;
		cout << "left win="<<win<<"  currentwin="<<currentWin<<endl;
		if( win==currentWin ) {
			// we have a tag ... so we move the closest point
			int i=findQuad(currentQuad,win);
			cout << "found quad at "<<i<<endl;
			int j=quads[i].findClosestPoint(x,y);
			updateQuadPoints(i,j,quads[i].px[j],quads[i].py[j]+KEYSTEP);
		}
	}else if( oscutils::endsWith(address,"/snap") ) {
		// take currentwin and currenquad and use it as reference.
		// make any point from any other quad (in same wname) within 0.1 to the same value
		const char *win;
		float x,y;
		m.ArgumentStream() >> x >> y >> win >> osc::EndMessage;
		cout << "snap win="<<win<<"  currentwin="<<currentWin<<endl;
		if( win==currentWin ) {
			// we have a tag ... so we find the closest point as reference
			int i=findQuad(currentQuad,win);
			int j=quads[i].findClosestPoint(x,y);
			// for all other quads with wame window....
			snapQuads(win,0.05,x,y);
		}
	}else if( oscutils::endsWith(address,"/ajoute/projecteur") ) {
		// ajoute un projecteur
		int32 w,h,ox,oy;
		const char *nom;
		m.ArgumentStream() >> nom >> w >> h >> ox >> oy >> osc::EndMessage;
		cout <<  "** ajoute un projecteur "<<nom<<" pos ("<<w<<","<<h<<") offset ("<<ox<<","<<oy<<")"<<endl;
		//
		setupWindow(nom,ox,oy,w,h,false,false);
		//
	}else if( oscutils::endsWith(address,"/ajoute/source") ) {
		const char *nom,*fichier;
		m.ArgumentStream() >> nom >> fichier >> osc::EndMessage;
		cout << "** ajoute une source "<<nom<<" avec fichier "<<fichier<<endl;
		int type=TYPE_IMAGE;
		string fi(fichier);
		if( fi.substr( fi.length() - 4 ) == ".mp4") {
			type=TYPE_VIDEO;
		}
		//
		setupMaterialVideoQuad(nom,640,480,700,0,fichier,type);
		//
	}else if( oscutils::endsWith(address,"/ajoute/preset") ) {
		const char *nom;
		m.ArgumentStream() >> nom >> osc::EndMessage;
		cout << "** ajoute un preset "<<nom<<endl;
		//
		// rien a faire
		//
	}else if( oscutils::endsWith(address,"/elimine/projecteur") ) {
		const char *nomProj;
		m.ArgumentStream() >> nomProj >> osc::EndMessage;
		cout << "** elimine le projecteur "<<nomProj<<endl;
		//
		unsetupWindow(nomProj);
		//
	}else if( oscutils::endsWith(address,"/elimine/source") ) {
		const char *nom;
		m.ArgumentStream() >> nom >> osc::EndMessage;
		cout << "** elimine la source "<<nom<<endl;
		//
		// on doit arreter/tuer le plugin media si c'est un video...
		//
	}else if( oscutils::endsWith(address,"/projecteur/ajoute/quad") ) {
		float x1,y1,x2,y2,x3,y3,x4,y4;
		const char *nomProj,*nomQuad,*shader;
		char lettre;
		m.ArgumentStream() >> nomProj >> lettre >> nomQuad >> shader >> x1 >> y1 >> x2 >> y2 >> x3 >> y3 >> x4 >> y4 >> osc::EndMessage;
		cout << "** ajoute quad "<<nomQuad<<" au projecteur "<<nomProj<<endl;
		// 'A' doit autuo-incrementer...
		setupHQuad(nomProj,lettre,nomQuad,255,255,0,"deform",x1,y1,x2,y2,x3,y3,x4,y4,shader /*SHADER_PROJ*/);
		string view="/"+string(nomProj)+"/"+string(1,lettre);
		loadImg(std::string(IMGV_SHARE)+"/images/lut.png",view+"/lut","display");
		//loadImg(std::string(IMGV_SHARE)+"/images/indianHead.jpg",view+"/tex","display");
	}else if( oscutils::endsWith(address,"/source/ajoute/quad") ) {
		float x1,y1,x2,y2,x3,y3,x4,y4;
		const char *nomSource,*nomQuad;
		char lettre;
		m.ArgumentStream() >> nomSource >> lettre >> nomQuad >> x1 >> y1 >> x2 >> y2 >> x3 >> y3 >> x4 >> y4 >> osc::EndMessage;
		cout << "** ajoute quad "<<nomQuad<<" a la source "<<nomSource<<endl;
		//
		setupHQuad(nomSource,lettre,nomQuad,255,255,0,"deform",x1,y1,x2,y2,x3,y3,x4,y4,SHADER_SOURCE);
		//
	}else if( oscutils::endsWith(address,"/preset/ajoute/connexion") ) {
		const char *nomPreset,*nomProj,*nomProjQuad,*nomProjQuadUni, *nomSrc,*nomSrcQuad;
		m.ArgumentStream() >> nomPreset >> nomProj >> nomProjQuad >> nomProjQuadUni >> nomSrc >> nomSrcQuad >> osc::EndMessage;
		cout << "** connexion preset="<<nomPreset<<" proj="<<nomProj<<" projquad="<<nomProjQuad<<" projquaduni="<<nomProjQuadUni << " src="<<nomSrc<<" srcquad="<<nomSrcQuad<<endl;
		presetinfo pi;
		pi.preset=nomPreset;
		pi.proj=nomProj;
		pi.projQuad=nomProjQuad;
		pi.projQuadUni=nomProjQuadUni;
		pi.src=nomSrc;
		pi.srcQuad=nomSrcQuad;
		presets.push_back(pi);
	}else if( nb>=3
		&& oscutils::matchArg(address,2,"preset") 
		&& oscutils::matchArg(address,1,"toggle") ) {
		// .../preset/toggle/<nom du preset>
		int pos,len;
		oscutils::extractArg(address,0,pos,len);
		string nomPreset(address+pos,len);
		float state;
		m.ArgumentStream() >>state >> osc::EndMessage;
		cout << "TOGGLE " << nomPreset << " state="<<state<<endl;
		activatePreset(nomPreset,state>0.5);
	}else if( oscutils::endsWith(address,"/preset/activate") ) {
		const char *nomPreset;
		m.ArgumentStream() >> nomPreset >> osc::EndMessage;
		activatePreset(nomPreset,true);
	}else if( oscutils::endsWith(address,"/preset/deactivate") ) {
		const char *nomPreset;
		m.ArgumentStream() >> nomPreset >> osc::EndMessage;
		activatePreset(nomPreset,false);
	}else if( oscutils::endsWith(address,"/seek") ) {
		const char *win;
		m.ArgumentStream() >> win >> osc::EndMessage;
		cout << "SEEK!!!!!!!!!!!!"<<endl;
		message *m0=new message(1024);
		*m0 << osc::BeginMessage("/seek") << 200.0 << osc::EndMessage;
		qpmanager::add("ffmpeg-recycle-"+string(win),m0);
	}else if( oscutils::endsWith(address,"/quit") ) {
		quit=true;
	}
    }
};





int main(int argc,char *argv[]) {
	cout << "opencv version " << CV_VERSION << endl;

	int inPort=12345;
	int outPort=10000;
	string outHost="localhost";

	for(int i=1;i<argc;i++) {
		if( strcmp("-h",argv[i])==0 ) {
			cout << "Usage: "<<argv[0]<<" [-h] [-listen 12345] [-send localhost 10000]" << endl;
			exit(0);
		}
		if( strcmp("-listen",argv[i])==0 && i+1<argc ) {
			inPort=atoi(argv[i+1]);
			i++;continue;
		}
		if( strcmp("-send",argv[i])==0 && i+2<argc ) {
			outHost=argv[i+1];
			outPort=atoi(argv[i+2]);
			i+=2;continue;
		}
	}


	qpmanager::initialize();

	qpmanager::startNetListeningUdp(inPort);

	setupNetworkOutgoing(outHost,outPort);

	//qpmanager::dump();

	myDecoder decoder;

	init();
	dumpQuads();

	quit=false;

	for(;!quit;) {
		message *m=qpmanager::pop_front_ctrl_wait("map");
		m->decode(decoder);

		if( !qpmanager::status("V") ) break;
	}
	
	uninit();

	unsetupNetworkOutgoing();
}







