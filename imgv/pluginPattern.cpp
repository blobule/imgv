

#include <imgv/imgv.hpp>

#include <imgv/pluginPattern.hpp>

#ifdef HAVE_PROFILOMETRE
  #define USE_PROFILOMETRE
  #include <imgv/profilometre.h>
#endif

#include <sys/time.h>

//
// todo: enlever le playAutomatic et /manual
//


using namespace cv;

//
// pattern plugin
//
// fabrique des patterns...
//
// [0] : input image (recycled image)
// [1] : output image
//

#define Q_IN	0
#define Q_OUT	1

pluginPattern::pluginPattern() {
	// ports
	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=2;

	portsIN["in"]=true;
}

pluginPattern::~pluginPattern() { log << "deleted"<<warn; }

void pluginPattern::init() {
	//log << "init tid=" << pthread_self() << warn;
	pattern=PATTERN_NONE;
	playAutomatic=true;

	// random seed is random ba default, and can be set by /seed 65572645765
	struct timeval tv;
	gettimeofday(&tv,NULL);
	uint64 seed = ((uint64)tv.tv_sec*1000000)+tv.tv_usec;

	rng=new cv::RNG(seed);
	n=0;
	nb=-1; // no limit to the number of images
}

void pluginPattern::uninit() {
	log << "uninit"<<warn;
	delete rng;rng=NULL;
}

bool pluginPattern::loop() {
	// on process les message AVANT, pour donner une chance a l'initialisation
	if( playAutomatic ) pop_front_ctrl_nowait(Q_IN); // peut faire changer plauauto
	if( playAutomatic ) {
		blob *i1=pop_front_wait(Q_IN);
		doOnePattern(*i1);
		i1->n=n;
		if( !view.empty() ) i1->view=view;
		push_back(Q_OUT,&i1);
		n++;
	}else{
		// manuel... attend au moins un message
		// si on recoit '/go', le travail sera fait dans 'decode'
		pop_front_ctrl_wait(Q_IN);
	}

	return(nb<0?true:(n<nb));
}

void pluginPattern::doOnePattern(Mat &m) {
#ifdef HAVE_PROFILOMETRE
	profilometre_start(pid.c_str());
#endif
	switch( pattern ) {
	  case PATTERN_LEOPARD: getLeopard(n,m);break;
	  case PATTERN_CHECKERBOARD: getCheckerBoard(n,m);break;
	  case PATTERN_NONE :
		log << "no pattern selected" << info;
		m.create(100,100,CV_8UC1);
		break;
	}
#ifdef HAVE_PROFILOMETRE
	profilometre_stop(pid.c_str());
#endif
}


//
// /set/leopard <int w> <int h> <int nb> <int freq> <bool blur>
// /set/checkboard <int w> <int h> <int size>
// /pid <char *pid>
//
bool pluginPattern::decode(const osc::ReceivedMessage &m) {
	log << "decoding " << m << warn;
	const char *address=m.AddressPattern();
	if( oscutils::endsWith(address,"/set/leopard") ) {
		m.ArgumentStream() >> w >> h >> nb >> freq >> blur >> osc::EndMessage;
		pattern=PATTERN_LEOPARD;
	}else if( oscutils::endsWith(address,"/set/checkerboard") ) {
		//try{
			m.ArgumentStream() >> w >> h >> squareSize >> osc::EndMessage;
		//} catch (const std::exception& ex) {
		//} 
		pattern=PATTERN_CHECKERBOARD;
	}else if( oscutils::endsWith(address,"/pid") ) {
		const char *val;
		m.ArgumentStream() >> val >> osc::EndMessage;
		pid=val; 
	}else if( oscutils::endsWith(address,"/set/view") ) {
		const char *v;
        m.ArgumentStream() >> v >> osc::EndMessage;
        view=string(v);
	}else if( oscutils::endsWith(address,"/manual") ) {
		playAutomatic=false;
	}else if( oscutils::endsWith(address,"/automatic") ) {
		playAutomatic=true;
	}else if( oscutils::endsWith(address,"/go") ) {
		if( playAutomatic ) { log << "ignoring /go in auto mode"<<warn; }
		else{
			blob *i1=pop_front_noctrl_wait(Q_IN); // esperons que ca ne sera pas long..
			// on ne regarde pas les messages, c'est fait dans loop
			doOnePattern(*i1);
			i1->n=n;
			if( !view.empty() ) i1->view=view;
			push_back(Q_OUT,&i1);
			n++;
		}
	}else{
		log << "unknown command: "<<m<<warn;
		return false;
	}
	return true;
}



void pluginPattern::dump(ofstream &file) {
        file << "P"<<name << " [shape=\"Mrecord\" label=\"{<0>0|"<<name<<"|<1>1}\"];\n";
        if( !badpos(0) ) file << "Q"<<Qs[0]->name << " -> " << "P"<<name <<":0" <<endl;
        if( !badpos(1) ) file << "P"<<name <<":1" << " -> " << "Q"<<Qs[1]->name <<endl;
}


void pluginPattern::getCheckerBoard(int n,Mat &m) {
	m.create(Size(w,h), CV_8UC1);
	int x,y;
	unsigned char *p=m.data;
	for(y=0;y<m.rows;y++) {
		int yy=((y/squareSize)%2)^(n%2);
		for(x=0;x<m.cols;x++) {
			int xx=(x/squareSize)%2;
			//m.at<uchar>(y,x)=(xx^yy)?255:0;
			*p++=(xx^yy)?255:0;
		}
	}
}


void pluginPattern::getLeopard(int n,Mat &pattern) {
	log << "n="<<n<< info;
	Size osize(w,h);
	Size size = osize + Size(osize.height*0.1, osize.width*0.1);

	size.width = getOptimalDFTSize(size.width);
	size.height = getOptimalDFTSize(size.height);

	m_complexR.create(size.height, size.width, CV_32F);
	m_complexR = Scalar::all(0);
	m_complexI.create(size.height, size.width, CV_32F);
	m_complexI = Scalar::all(0);

	int minFreq = freq;
	int maxFreq = freq*2;
	double fx, fy;
	int minFreq2 = minFreq*minFreq;
	int maxFreq2 = maxFreq*maxFreq;

	double ratio = double(size.width)/size.height;
	double mag, phase, cosPhase, sinPhase;

	for(int j=1; j<size.height/2; ++j) {
	    int cj = size.height-j;
	    float *ptrR = m_complexR.ptr<float>(j);
	    float *ptrRc = m_complexR.ptr<float>(cj);
	    float *ptrI = m_complexI.ptr<float>(j);
	    float *ptrIc = m_complexI.ptr<float>(cj);
	    fy = j*ratio;

	    for (int i=1; i<size.width/2; ++i) {
		int ci = size.width-i;
		fx = i;
		mag = fx*fx + fy*fy;

		if (mag <= maxFreq2 && mag >= minFreq2) {
		    phase = rng->uniform(0., 2.*M_PI);
		    cosPhase = cos(phase);
		    sinPhase = sin(phase);

		    ptrR[i] = cosPhase;
		    ptrI[i] = sinPhase;
		    ptrRc[ci] = cosPhase;
		    ptrIc[ci] = -sinPhase;

		    phase = rng->uniform(0., 2.*M_PI);
		    cosPhase = cos(phase);
		    sinPhase = sin(phase);

		    ptrR[ci] = cosPhase;
		    ptrI[ci] = sinPhase;
		    ptrRc[i] = cosPhase;
		    ptrIc[i] = -sinPhase;
		}
	    }
	}

	Mat planes[] = { m_complexR, m_complexI };
	merge(planes, 2, m_complex);

	idft(m_complex, m_complexR, DFT_REAL_OUTPUT);

	threshold(m_complexR, m_thresh, 0, 255., THRESH_BINARY);
	m_thresh = Mat_<uchar>(m_thresh);

	if (blur) {
	    double avgFreq = (minFreq+maxFreq)/2.;
	    double sigmaX = (size.width/avgFreq)/6.;
	    double sigmaY = (size.height/avgFreq)/6.;
	    GaussianBlur(m_thresh, m_thresh, Size(), sigmaX, sigmaY);
	}

	m_thresh(Rect((size.width-osize.width)/2.,
	      (size.height-osize.height)/2.,
	      osize.width, osize.height)).copyTo(pattern);
}








