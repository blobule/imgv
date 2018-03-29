
#include <pluginDrip.hpp>

#include <sys/time.h>
#include <unistd.h>

//#define VERBOSE
//#define STATS

//using namespace std;

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2


void pluginDrip::setStart(int start) {
	if( start<1000000 ) {
		struct timeval tv;
		gettimeofday(&tv,NULL);
		this->startsec=tv.tv_sec+start;
		this->startusec=tv.tv_usec;
	}else{
		this->startsec=start;
		this->startusec=0;
	}
}

pluginDrip::pluginDrip() {
	// default must be done here, since we use startsec in init()
	setStart(0);
	this->fps=30.0;
	this->paused=false;
	this->flush=false;

	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=Q_LOG;

	portsIN["in"]=true;
}

/*
pluginDrip::pluginDrip(int start,double fps) {
	setStart(start);
	this->fps=fps;
	this->paused=false;
}

pluginDrip::pluginDrip(message &m) {
	setStart(0);
	this->fps=30.0;
	this->paused=false;
	m.decode(*this); // va appeller notre decode() du messageDecoder
}
*/

pluginDrip::~pluginDrip() { log << "delete"<<warn;}

void pluginDrip::init() {
	//log << "init tid=" << pthread_self() << warn;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	log << "starting in "<<startsec-tv.tv_sec<<" sec"<<warn;

	n=0;
	offset=0.0;
	origin=startsec+startusec/1000000.0;
#ifdef STATS
	sum=0.0;
	sum2=0.0;
#endif

	// animation debut/fin de movie
	beginEventName="";
	beginEventFrame=-1;

	endEventName="";
	endEventFrame=-1;

	// no duplicate
	duplicate=-1;

	// no predisplay of first frame
	predisplay=false;

	// default parameters
	setStart(0);
	fps=30.0;
	paused=false;
	count=-1; // infinite!
	flush=false;

	debug=false;

	initializing=true; // until we get /start ... 
}

void pluginDrip::uninit() { log << "uninit"<<warn;  }

bool pluginDrip::loop() {
	if( flush ) {
		// empty the input queue until empty.
		// process messages while going
		for(;;) {
			blob *i1=pop_front_nowait(Q_IN);
			if( i1==NULL ) break;
			log << "flush! image " << i1->n << warn;
			recycle(i1);
		}
		flush=false;
		return true; // done for now
	}
	if( initializing  ) {
		log << "initializing"<<warn;
		pop_front_ctrl_wait(Q_IN);
		if( initializing ) return true;
	}
	if( paused ) {
		log << "paused" <<warn;
		pop_front_ctrl_wait(Q_IN);
		if( paused ) { // encore en pause?  attend un peu
			log << "ZzzzzzzZZZzzzzz"<<warn;
		}
	}else{
		double t;
		t=waitForFrame();

		blob *i1=pop_front_wait(Q_IN);

		// on ne change pas le #, mais seulement le timestamp
		i1->timestamp=t;
		//cout << "[drip] done="<<i1->frames_since_begin<<" togo="<<i1->frames_until_end<<endl;
		//cout << "[drip] frame="<<i1->get()<<endl;

		//log << "video n="<<i1->n<<" drip n="<<n<<warn;

		//
		// check for animation events
		//
		if( i1->frames_since_begin>=0 && i1->frames_since_begin<=beginEventFrame ) {
			checkOutgoingMessages(beginEventName,t, i1->frames_since_begin,0,beginEventFrame,1);
		}
		if( i1->frames_until_end>=0 && i1->frames_until_end<=endEventFrame ) {
			checkOutgoingMessages(endEventName,t, i1->frames_until_end,0,endEventFrame,1);
		}

		//
		// duplicate?
		//
		if(  duplicate>=0 ) {
		/*
			blob *i2=pop_front_wait(4);
			i1->copyTo(*i2);
		*/
			// share le data
			blob *i2=new blob(*i1);
			i2->view=dupView1;
			push_back(duplicate,&i2);

			//i2=new blob(*i1);
			//i2->view=dupView2;
			//push_back(duplicate,&i2);

			//
			// on pourrait ne pas dupliquer les datas..
			// blob *i2=new blob(*i1);
			//
		}

		//
		// send the image
		//
		if( n==0 ) log << "sent image 0 timestamp "<<i1->timestamp<<warn;
		push_back(Q_OUT,&i1);

  	    checkOutgoingMessages("frame",t,n,0,1,1);

		//log << "count="<<count<<info;

		n++;
		if( count>=0 ) {
			count--;
			if( count==0 ) {
				log << "count reached."<<warn;
				initializing=true;
			}
		}
	}
#ifdef STATS
	if( n%1000==1 ) {
	// stats
	double std = sqrt((sum2 - sum*sum/n)/n);
	double mean = sum/n;
	log << "n="<<n<<"  mean="<<mean<<"ms  stddev="<<std<<"ms  offset="<<offset*1000.0<<"ms"<<info;
	}
#endif

	return true;
}

//
// decodage de messages a l'initialisation (init=true)
// ou pendant l'execution du plugin (init=false)
// (dans messageDecoder)
//
bool pluginDrip::decode(const osc::ReceivedMessage &m) {
	const char *address=m.AddressPattern();
	log << "got message "<<m<<warn;
	if( plugin::decode(m) ) return true; // check for /defevent
	//
	// init or not...
	//
	// /event/clear	-> clear all events
	// /event/set/begin -1  60   -> event -1 is triggered when frame_since_begin is 0 to 60
	// /event/set/end -2  60   -> event -2 is triggered when frame_until_end it 60 to 0
	if( oscutils::endsWith(address,"/debug") ) {
		m.ArgumentStream() >> debug >> osc::EndMessage;
		return true;
	}else if( oscutils::endsWith(address,"/event/clear") ) {
		beginEventFrame=-1;
		endEventFrame=-1;
		return true;
	}else if( oscutils::endsWith(address,"/event/set/begin") ) {
		const char *en;
		m.ArgumentStream() >> en >> beginEventFrame >> osc::EndMessage;
		beginEventName=string(en);
		return true;
	}else if( oscutils::endsWith(address,"/event/set/end") ) {
		const char *en;
		m.ArgumentStream() >> en >> endEventFrame >> osc::EndMessage;
		endEventName=string(en);
		return true;
	}else if( oscutils::endsWith(address,"/nodup") ) {
		m.ArgumentStream() >> osc::EndMessage;
		duplicate=-1;
		return true;
	}else if( oscutils::endsWith(address,"/dup") ) {
		// -1 : no duplicate, >0 : output port of duplicate
		const char *vi1,*vi2;
		const char *dupPortName;
		m.ArgumentStream() >> dupPortName >> vi1 >> osc::EndMessage;
		duplicate=portMap(dupPortName);
		dupView1=string(vi1);
		//dupView2=string(vi2);
		return true;
	}
	if( initializing ) {
		if( oscutils::endsWith(address,"/predisplay-first") ) {
			m.ArgumentStream() >> osc::EndMessage;
			log << "predisplay first frame"<<fps<<info;
			predisplay=true;
		}else if( oscutils::endsWith(address,"/fps") ) {
			m.ArgumentStream() >> fps >> osc::EndMessage;
			log << "reset fps to "<<fps<<info;
		}else if( oscutils::endsWith(address,"/start") ) {
			int32 start;
			m.ArgumentStream() >> start >> osc::EndMessage;
			setStart(start);
			initializing=false;
			paused=false;
			n=0; // important!
			log << "start set to "<<start<<warn;
		}else if( oscutils::endsWith(address,"/count") ) {
			// negative count is infinite
			m.ArgumentStream() >> count >> osc::EndMessage;
			n=0; // super important!!!
			log << "reset count to "<<count<<info;
		}else{
			log << "commande init inconnue: "<<m<<err;
			return false;
		}
	}else{
		double newfps,remotenow;
		int key;
		// /fpsnow f [fps]
		if( oscutils::endsWith(address,"/reset") ) {
			// not useful because it is stuck waiting for one image...
			initializing=true;
		}else if( oscutils::endsWith(address,"/fpsnow") ) {
			float ffps;
			m.ArgumentStream() >> ffps >> osc::EndMessage;
			struct timeval tv;
			gettimeofday(&tv,NULL);
			double nnow=tv.tv_sec+tv.tv_usec/1000000.0;
			resetFps(nnow,ffps);
		}else if( oscutils::endsWith(address,"/fps") ) {
			m.ArgumentStream() >> remotenow >> newfps >> osc::EndMessage;
			resetFps(remotenow,newfps);
		}else if( oscutils::endsWith(address,"/pause") ) {
			log << "pause : "<<m<<warn;
			m.ArgumentStream() >> remotenow >> osc::EndMessage;
			if( paused ) {
				paused=false;
				double target=startsec+startusec/1000000.0+remotenow-pauseStart;
				startsec=(int)target;
				startusec=(int)((target-startsec)*1000000.0);
				log << "added offset "<<(remotenow-pauseStart)<<info;
			}else{
				// start pause!
				paused=true;
				pauseStart=remotenow;
			}
		}else if( oscutils::endsWith(address,"/pausenow") ) {
			log << "pausenow : "<<m<<warn;
			bool status;
			m.ArgumentStream() >> status >> osc::EndMessage;
			struct timeval tv;
			gettimeofday(&tv,NULL);
			double nnow=tv.tv_sec+tv.tv_usec/1000000.0;
			if( paused && status==false ) {
				paused=false;
				double target=startsec+startusec/1000000.0+nnow-pauseStart;
				startsec=(int)target;
				startusec=(int)((target-startsec)*1000000.0);
				log << "added offset "<<(nnow-pauseStart)<<info;
			}else if( !paused && status==true ) {
				// start pause!
				paused=true;
				pauseStart=nnow;
			}
		}else if( oscutils::endsWith(address,"/flush") ) {
			log << "flush" << warn;
			// attention: no fonctionne qu'en mode pause
			flush=true;
		}else{
			log << "commande non-init inconnue: "<<m<<err;
			return false;
		}
	}
	return true;
}

//
// private internal stuff
//

	// reset fps
void pluginDrip::resetFps(double remotenow,double newfps) {
	// calcule n en fonction de remotenow
	double remoten=(remotenow-startsec-startusec/1000000.0)*fps;
	//cout << "[osc] reset fps to "<<newfps<<"(n="<<n<<")"<< " remoten=("<<remoten<<")"<<endl;
	// mise a jour du fps
	double target=startsec+startusec/1000000.0+remoten/fps-remoten/newfps;
	//cout << "before "<<startsec<<" . "<<startusec<<endl;
	startsec=(int)target;
	startusec=(int)((target-startsec)*1000000.0);
	//cout << "after  "<<startsec<<" . "<<startusec<<endl;
	fps=newfps;
}


	// wait for frame n, based on current time, startsec, startusec and fps
	// ajust offset as you go.
	// retourne le timestamp
double pluginDrip::waitForFrame() {
	double now,target,delta;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	now=tv.tv_sec+tv.tv_usec/1000000.0;

	if( predisplay && n==0 ) target=now;
	else target=startsec+startusec/1000000.0+n/fps;

	delta=(target-offset)-now;
	if( delta<=0.0 ) {
		if( debug ) log << "n="<<n<<" we are late by "<<delta<<" sec"<<warn;
		return now-startsec-startusec/1000000.0;
	} // we are late already

	if( debug ) log << "delta="<<delta<<info;

	// we have time to waste. wait!

	// wait seconds first
	while( delta>0.5 ) {
		//int s=(int)delta;
#ifdef VERBOSE
		cout <<"[drip] zzzzz 0.5 sec"<<endl;
#endif
		usleepSafe(500000);
		// recompute the time... (could have been waken by a signal)
		gettimeofday(&tv,NULL);
		now=tv.tv_sec+tv.tv_usec/1000000.0;
		if( target<now ) return now-startsec-startusec/1000000.0; // we are late already
		delta=(target-offset)-now;
	}
	// precise sleep
#ifdef VERBOSE
	log<<"n="<<n<<" zz "<<delta<<" offset="<<offset<<info;
#endif
	usleepSafe((int)(delta*1000000));

	gettimeofday(&tv,NULL);
	// en ms
	double diff=(tv.tv_sec*1000.0+tv.tv_usec/1000.0)-target*1000.0;
	now=tv.tv_sec+tv.tv_usec/1000000.0;
	// ajustement EN DOUCEUR!!!! sinon, over/under/over/under...
	offset = offset*0.9+0.1/1000.0*diff;
	// should be 0. positive=too late, negative=early

#ifdef STATS
	sum+=diff;
	sum2+=diff*diff;
#endif
	return now-origin;
}


void pluginDrip::dump(ofstream &file) {
	//file << "P"<<name << " [shape=\"Mrecord\" label=\"{{<0>0|<2>2}|"<<name<<"|<1>1}\"];" << endl;
	file << "P"<<name << " [shape=\"Mrecord\" label=\"{<0>0|"<<name<<"|<1>1}\"];" << endl;
	if( !badpos(0) ) file << "Q"<<Qs[0]->name << " -> " << "P"<<name <<":0" <<endl;
	if( !badpos(1) ) file << "P"<<name <<":1" << " -> " << "Q"<<Qs[1]->name <<endl;
}






