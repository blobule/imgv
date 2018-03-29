

#include <imgv/imgv.hpp>

#include <imgv/pluginV4L2.hpp>


#include <sys/time.h>

using namespace cv;

//
// webcam plugin
//
// [0] : input image (recycled image) and control
// [1] : output image and out matrice
//

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2


pluginV4L2::pluginV4L2() {
	camnum=-1; // no camera
	// ports
	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=Q_LOG;

	// for dot output
	portsIN["in"]=true;
}
pluginV4L2::~pluginV4L2() { log << "deleted"<<warn; }

void pluginV4L2::init() {
	//log << "init tid=" << pthread_self() << warn;
	ptOK=0; // pas de points disponibles pour analyse
	nextImageIsBackground=false;
	//seuil=50;
	lastPos=-1.0; // no last pos
}

void pluginV4L2::uninit() {
	log << "uninit"<<warn;
}

bool pluginV4L2::loop() {
	// si pas de camera, on attend un message
	if( camnum<0 )	pop_front_ctrl_wait(Q_IN);
	else			pop_front_ctrl_nowait(Q_IN); // process messages now
	// camnum pourrait avoir change...
	if( camnum>=0 ) {
		// prochaine image!
		blob *i1=pop_front_noctrl_wait(Q_IN); // esperons que ca ne sera pas long..
		stream.read(*i1);

		// le timestamp
		struct timeval tv;
		gettimeofday(&tv,NULL);
		i1->timestamp=(double)tv.tv_sec+tv.tv_usec/1000000.0;

		i1->view=view;
		i1->n=n;
		n++;

		process(*i1);

		// send image
		push_back(Q_OUT,&i1);
	}
	return true; // boucle infinie;
}



//
// /cam <int camnum> -> open camera (<0 means close the camera)
//
// on devrait ajouter /start /stop
//
bool pluginV4L2::decode(const osc::ReceivedMessage &m) {
	log << "decoding " << m << info;
    int32 k;
	if( plugin::decode(m) ) return true; // verifie pour le /defevent

	const char *address=m.AddressPattern();

    if( oscutils::endsWith(address,"/cam") ) {
		const char *view;
		m.ArgumentStream() >> k >> view >> osc::EndMessage;
		this->view=view;
		stream.release();
		camnum=k;
		if( camnum>=0 ) stream.open(camnum);
		log << "webcam started cam "<<camnum<<info;
	}else if( oscutils::endsWith(address,"/line/begin") ) {
		float xr,yr;
		m.ArgumentStream() >> xr >> yr >> osc::EndMessage;
		pt1.x=xr;pt1.y=yr;ptOK|=1;nextImageIsBackground=true;
	}else if( oscutils::endsWith(address,"/line/end") ) {
		float xr,yr;
		m.ArgumentStream() >> xr >> yr >> osc::EndMessage;
		pt2.x=xr;pt2.y=yr;ptOK|=2;nextImageIsBackground=true;
	}else if( oscutils::endsWith(address,"/line/background") ) {
		nextImageIsBackground=true;
	}else{
		log << "unknown command: "<<m<<warn;
		return false;
	}
	return true;
}



void pluginV4L2::dump(ofstream &file) {
        file << "P"<<name << " [shape=\"Mrecord\" label=\"{<0>0|"<<name<<"|<1>1}\"];\n";
        if( !badpos(0) ) file << "Q"<<Qs[0]->name << " -> " << "P"<<name <<":0" <<endl;
        if( !badpos(1) ) file << "P"<<name <<":1" << " -> " << "Q"<<Qs[1]->name <<endl;
	// .. a completer
}



void pluginV4L2::process(Mat &im) {
	//cout << "image ("<<im.cols<<","<<im.rows<<") "<<ptOK<<endl;
	int i;

	if( ptOK!=3 ) return;

	cv::Point2i pt1i(pt1.x*im.cols,pt1.y*im.rows);
	cv::Point2i pt2i(pt2.x*im.cols,pt2.y*im.rows);

	//log << "got points " << pt1 << "," << pt2 << " -> "<<pt1i<<","<<pt2i<<warn;

	if( nextImageIsBackground ) {
		LineIterator itt(im, pt1i, pt2i, 8);
		fond.resize(itt.count);
		// record background
		for(i = 0; i < itt.count; i++, ++itt) fond[i] = im.at<Vec3b>(itt.pos());
		nextImageIsBackground=false;
	}

	LineIterator it(im, pt1i, pt2i, 8);
	//cout << "count="<<it.count<< ":"<<pt1-pt2<<endl;

	// check with background

	Vec3b val;
	Vec3i delta;
	norm.resize(it.count);
	//int histo[32]; // on remap 3*255 -> 32 valeurs
	//for(i=0;i<32;i++) histo[i]=0;
	for(i=0; i < it.count; i++, ++it) {
		//cout << i << ":" << it.pos() << endl;
/*
		int xx=it.pos().x;
		int yy=it.pos().y;
		if( xx<0 || xx>=im.cols || yy<0 || yy>=im.rows ) {
			cout << "BUG! SKIP!" <<i<<":"<<it.pos()<<endl;
			continue;
		}
*/
		val = im.at<Vec3b>(it.pos());
		im.at<Vec3b>(it.pos())=Vec3b(255,255,255)-val;
		delta[0]=(int)val[0]-fond[i][0];
		delta[1]=(int)val[1]-fond[i][1];
		delta[2]=(int)val[2]-fond[i][2];
		//norm[i]=abs(delta[0])+abs(delta[1])+abs(delta[2]);
		norm[i]=delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2];
		if( norm[i]>120*120 ) norm[i]=120*120;
		// remap 20*20..100*100 -> 0..80
		norm[i]=(norm[i]-30*30);
		if( norm[i]<0) norm[i]=0;
		// au final on a 0..80*80 par pixel
		/*
		int pos=norm[i]*31/(3*255);
		if( pos<0 ) pos=0;
		if( pos>31 ) pos=31;
		histo[pos]++;
		cout << i << " : " << it.pos() <<":"<<norm[i]<<endl;
		*/
		//printf("%4d : %3d\n",i,norm[i]);
	}

	// somme ponderee des valeurs au dessus du seuil.
	double sum=0;
	int sumw=0;
	int nb=0;
	for(i=0;i<it.count;i++) {
		//if( norm[i]<30 ) continue;
		sum+=(double)i*norm[i];//*norm[i];
		sumw+=norm[i];//*norm[i];
		nb++;
	}
	//cout << "sum="<<sum<<", sumw="<<sumw<<endl;
	if( sumw==0 ) sumw=1;
	if( nb==0 ) nb=1;
	double pos=sum/sumw;
	sumw/=nb;
	//position remise entre 0 et 100
	pos=pos/it.count*100.0;
	//printf("%12.2f : %12.2f : %4d\n",lastPos, pos,sumw);

	if( lastPos<0.0 ) lastPos=pos;
	if( sumw>50 ) {
		if( lastPos>=0.0 ) { pos=0.5*lastPos+0.5*pos;lastPos=pos; }
		// pos va de 0 a 100*100 = 10000
		//log << "pos "<<pos<<info;
		checkOutgoingMessages("line",0,(int)(pos*100),sumw,100*100,1);
	}else lastPos=-1.0;

	// genere un message en sortie
	// /cam/pos <double position 0..100> <int force 0..3*255>

	// histo
	/*
	int j,k;
	for(i=0;i<32;i++) {
		printf("%3d :",i);
		k=histo[i]*70/it.count;
		for(j=0;j<k;j++) printf("#");
		for(;j<70;j++) printf("-");
		printf("\n");
	}
	*/
	
/*
	for(int i=0;i<buf.size();i++) {
		printf("%3d : (%3d,%3d,%3d)\n",buf[i].x,buf[i].y,buf[i].z);
	}
*/
}




