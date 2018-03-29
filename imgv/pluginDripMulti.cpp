
#include <pluginDripMulti.hpp>

#include <sys/time.h>
#include <unistd.h>

//#define VERBOSE
//#define STATS

//using namespace std;

void pluginDripMulti::setStart(int start) {
	if( start<1000000 ) {
		struct timeval tv;
		gettimeofday(&tv,NULL);
		this->startsec=tv.tv_sec+start;
		this->startusec=tv.tv_usec;
	}else{
		this->startsec=start;
		this->startusec=0;
	}
	origin=this->startsec+this->startusec/1000000.0;
}

pluginDripMulti::pluginDripMulti() {
	ports["in"]=0;
	ports["log"]=1;

	portsIN["in"]=true;
}

/*
pluginDripMulti::pluginDripMulti(int start,double fps) {
	setStart(start);
	this->fps=fps;
	this->paused=false;
}

pluginDripMulti::pluginDripMulti(message &m) {
	setStart(0);
	this->fps=30.0;
	this->paused=false;
	m.decode(*this); // va appeller notre decode() du messageDecoder
}
*/

pluginDripMulti::~pluginDripMulti() { cout << "[dripmulti] delete"<<endl;}

void pluginDripMulti::init() {
	//cout << "[dripmulti] init tid=" << pthread_self() << endl;
	//struct timeval tv;
	//gettimeofday(&tv,NULL);
	//log << "[dripmulti] starting in "<<startsec-tv.tv_sec<<" sec"<< info;

	n=0;
	offset=0.0;
	//origin=startsec+startusec/1000000.0;
#ifdef STATS
	sum=0.0;
	sum2=0.0;
#endif

	// animation debut/fin de movie
	beginEventFrame=-1;
	endEventFrame=-1;
	beginEventInputNum=-1;
	endEventInputNum=-1;

	// no duplicate
	//duplicate=-1;

	// default parameters
	setStart(0);
	fps=30.0;
	paused=false;

	initializing=true; // until we get /start ... 
	sync=true; // par default, on sync les input...
	timestamp_id=false;
}

void pluginDripMulti::uninit() { cout << "[dripmulti] uninit"<<endl;  }

bool pluginDripMulti::loop() {
	if( initializing  ) {
		log << "initializing"<< info;
		// attend une image de la queue 2 (messages)
		pop_front_ctrl_wait(0);
		if( initializing ) return true;
	}
	if( paused ) {
		log << "paused" <<info;
		// attend une image de la queue 2 (messages)
		pop_front_ctrl_wait(0);
		if( paused ) { // encore en pause?  attend un peu
			log << "[dripmulti] waiting"<<info;
		}
	}else{
		// check messages
		pop_front_ctrl_nowait(0);
		// in case we receive message in the in ports...
		for(int i=0;i<in.size();i++) {
			// get port number, if unkown
			if( in[i].port<0 ) in[i].port=portMap(in[i].portname);
			pop_front_ctrl_nowait(in[i].port);            
		}

		// gather frames BEFORE waiting for the right time to send them
		int nmax=-1;
        int noffset;
		bool zero=false; // did a movie reset to 0?
		for(int i=0;i<in.size();i++) {
			in[i].img=pop_front_noctrl_wait(in[i].port);
			if( in[i].img->n==0 ) zero=true;
            if (timestamp_id)
            {
              double ts=in[i].img->timestamp;
              //printf("ORIGINAL FRAME NUMBER %d\n",in[i].img->n);
              if (in[i].img->n<=1) in[i].timestamp_first=ts; //first frame
              in[i].img->n=(int)(fps*(ts-in[i].timestamp_first)+0.5)+1;
              //printf("TIMESTAMP %f (%f, %d)\n",ts,in[i].timestamp_first,in[i].img->n);
              //printf("FRAME NUMBER %d\n",in[i].img->n);
            }
            noffset=in[i].img->n+in[i].noffset;
	        if( noffset>nmax ) nmax=noffset;
			//log << "got images "<<i<<", n="<<in[i].img->n<<", noffset="<<noffset<<", nmax="<<nmax<<info;
	    }

		if( sync ) {
			// zero: wait for all input to have n=0.
			// sinon: wait for all input <nmax to reach nmax
			// this is for looping, when n goes back to 0.
			skipAllUntil(zero?0:nmax);
		}

		// we have all the images...
		//log << "got all images. nmax="<<nmax<<" zero="<<zero<<info;

		double t;
		t=waitForFrame();

		//
		// check for event before we send the images
		//
		if( beginEventInputNum>=0 ) {
		    blob *im=in[beginEventInputNum].img;
		    if( im!=NULL && im->frames_since_begin>=0 && im->frames_since_begin<=beginEventFrame ) {
			checkOutgoingMessages(beginEventName,t, im->frames_since_begin,0,beginEventFrame,1);
		    }
		}
		if( endEventInputNum>=0 ) {
		    blob *im=in[endEventInputNum].img;
		    if( im->frames_until_end>=0 && im->frames_until_end<=endEventFrame ) {
			checkOutgoingMessages(endEventName,t, im->frames_until_end,0,endEventFrame,1);
			//cout << "[dripmulti] fade "<<im->frames_until_end<<endl;
		    }
		}

		// send duplicates first

		for(int i=0;i<dup.size();i++) {
			if( dup[i].in<0 ) continue; // skip since no input match
			blob *idup=new blob(*in[dup[i].in].img);
			if( !dup[i].view.empty() ) idup->view=std::string(dup[i].view.c_str());
			idup->timestamp=t;
			//cout << "[dripmulti] sent dup image "<<dup[i].in<<" to port "<<dup[i].port<< " view="<<idup->view<<endl;
			// get port number, if unkown
			if( dup[i].port<0 ) dup[i].port=portMap(dup[i].portname);
			push_back(dup[i].port,&idup);
		}

        int nn=0;
		// on fait sortir les images
		for(int i=0;i<out.size();i++) {
			if( out[i].in<0 ) continue; // skip since no input match
			blob *i1=in[out[i].in].img;
			if( in[out[i].in].img==NULL ) continue; // skip since no image
			in[out[i].in].img=NULL;
			i1->timestamp=t;
            nn=i1->n;
			if( !out[i].view.empty() ) i1->view=std::string(out[i].view.c_str());
			// get port number, if unkown
			if( out[i].port<0 ) out[i].port=portMap(out[i].portname);
			push_back(out[i].port,&i1);
		}

		// event "frame"
  	    checkOutgoingMessages("frame",t,nn,0,1,1);

		// recycle tout ce qui reste d'image pas envoyee
		for(int i=0;i<in.size();i++) {
			if( in[i].img==NULL ) continue;
			recycle(in[i].img);
		}
		n++;
	}
#ifdef STATS
	if( n%1000==1 ) {
	// stats
	double std = sqrt((sum2 - sum*sum/n)/n);
	double mean = sum/n;
	log << "[dripmulti] n="<<n<<"  mean="<<mean<<"ms  stddev="<<std<<"ms  offset="<<offset*1000.0<<"ms"<<info;
	}
#endif

	return true;
}

//
// skip all image of all inputs until they all have the same number "target"
//
// special case: if we get a frame 0 while trying to reach nmax>0, then we revert to all 0
//
void pluginDripMulti::skipAllUntil(int target) {
	for(int i=0;i<in.size();i++) {
	  for(;;) {
		if( in[i].img==NULL ) break;
		if( in[i].img->n==0 && target!=0 ) { skipAllUntil(0);return; }
		if( in[i].img->n+in[i].noffset>=target ) break;
		// get rid of this image
		recycle(in[i].img);
		// get a new one
		in[i].img=pop_front_noctrl_wait(in[i].port);
		//cout << "[dripmulti] got images "<<i<<", n="<<in[i].img->n<<endl;
	  }
	}
}

//
// decodage de messages a l'initialisation (init=true)
// ou pendant l'execution du plugin (init=false)
// (dans messageDecoder)
// /in 2 f1   -> in-port, in-name
// /out f3 7 /stereo/A/tex3
// /dup f1 8  /cam/F1/tex	/dup/cancel f1 8, dup-name dup-port, -> in-port
// /sync true
//
bool pluginDripMulti::decode(const osc::ReceivedMessage &m) {
	const char *address=m.AddressPattern();
	log << "got message "<<m<<info;
	if( plugin::decode(m) ) return true; // check for /defevent
	//
	// init or not...
	//
	// /event/clear	-> clear all events
	// /event/set/begin -1  60   -> event -1 is triggered when frame_since_begin is 0 to 60
	// /event/set/end -2  60   -> event -2 is triggered when frame_until_end it 60 to 0

	
	if( oscutils::endsWith(address,"/reset") ) {
			// not useful because it is stuck waiting for one image...
			initializing=true;
    }
    else if( oscutils::endsWith(address,"/in") ) {
		const char *portName;
		const char *name;
		m.ArgumentStream() >> portName >> name >> osc::EndMessage;
		portinfo A(name,string(portName));
		in.push_back(A); computeMatch(); //dumpPortInfo();
		return true;
    }else if( oscutils::endsWith(address,"/offset") ) {
		const char *name;
        int32 noffset;
		m.ArgumentStream() >> name >> noffset >> osc::EndMessage;
		for(int i=0;i<in.size();i++) {
			if( in[i].match(name) ) in[i].noffset=noffset;
		}
		return true;
	}else if( oscutils::endsWith(address,"/out") ) {
		const char *portname;
		const char *name,*view;
		m.ArgumentStream() >> name >> portname >> view >> osc::EndMessage;
		portinfo A(name,string(portname),view);
		out.push_back(A); computeMatch(); //dumpPortInfo();
		return true;
	}else if( oscutils::endsWith(address,"/dup") ) {
		const char *portname;
		const char *name,*view;
		m.ArgumentStream() >> name >> portname >> view >> osc::EndMessage;
		portinfo A(name,string(portname),view);
		dup.push_back(A); computeMatch(); //dumpPortInfo();
		return true;
	}else if( oscutils::endsWith(address,"/sync") ) {
		m.ArgumentStream() >> sync >> osc::EndMessage;
		return true;
	}else if( oscutils::endsWith(address,"/timestamp-id") ) {
		m.ArgumentStream() >> timestamp_id >> osc::EndMessage;
        return true;
    }else if( oscutils::endsWith(address,"/in/cancel") ) {
		const char *name;
		m.ArgumentStream() >> name >> osc::EndMessage;
		for(int i=0;i<in.size();i++) {
			if( in[i].match(name) ) in.erase(in.begin()+i); else i++;
		}
		computeMatch(); //dumpPortInfo();
		return true;
	}else if( oscutils::endsWith(address,"/out/cancel") ) {
		int32 port;
		const char *name;
		m.ArgumentStream() >> name >> port >> osc::EndMessage;
		for(int i=0;i<out.size();i++) {
			if( out[i].match(name) ) out.erase(out.begin()+i); else i++;
		}
		computeMatch(); //dumpPortInfo();
		return true;
	}else if( oscutils::endsWith(address,"/dup/cancel") ) {
		int32 port;
		const char *name;
		m.ArgumentStream() >> name >> port >> osc::EndMessage;
		for(int i=0;i<dup.size();i++) {
			if( dup[i].match(name) ) dup.erase(dup.begin()+i); else i++;
		}
		computeMatch(); //dumpPortInfo();
		return true;
	}else if( oscutils::endsWith(address,"/event/clear") ) {
		beginEventName="";
		beginEventFrame=-1;
		endEventName="";
		endEventFrame=-1;
		return true;
	}else if( oscutils::endsWith(address,"/event/set/begin") ) {
		const char *name;
		const char *ename;
		m.ArgumentStream() >> ename >> name >> beginEventFrame >> osc::EndMessage;
		beginEventName=ename;
		beginEventInputName=name;
		computeMatch();
		return true;
	}else if( oscutils::endsWith(address,"/event/set/end") ) {
		const char *name;
		const char *ename;
		m.ArgumentStream() >> ename >> name >> endEventFrame >> osc::EndMessage;
		endEventName=ename;
		endEventInputName=name;
		computeMatch();
		return true;
/*
	}else if( oscutils::endsWith(address,"/dup") ) {
		// -1 = no duplicate, 0..+ = output port of duplicate
		const char *vi1,*vi2;
		m.ArgumentStream() >> duplicate >> vi1 >> osc::EndMessage;
		dupView1=string(vi1);
		//dupView2=string(vi2);
		return true;
*/
	}
	if( initializing ) {
		if( oscutils::endsWith(address,"/fps") ) {
			m.ArgumentStream() >> fps >> osc::EndMessage;
			log << "[osc] reset fps to "<<fps<<warn;
		}else if( oscutils::endsWith(address,"/start") ) {
			int32 start;
            n=0;
			m.ArgumentStream() >> start >> osc::EndMessage;
			setStart(start);
			initializing=false;
            paused=false;
		}else{
			log << "commande init inconnue: "<<m<<err;
			return false;
		}
	}else{
		double newfps,remotenow;
		int key;
		// /fpsnow f [fps]
		if( oscutils::endsWith(address,"/fpsnow") ) {
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
			log << "pause : "<<m<<info;
			m.ArgumentStream() >> remotenow >> osc::EndMessage;
			if( paused ) {
				paused=false;
				double target=startsec+startusec/1000000.0+remotenow-pauseStart;
				startsec=(int)target;
				startusec=(int)((target-startsec)*1000000.0);
				log << "added offset "<<(remotenow-pauseStart)<<warn;
			}else{
				// start pause!
				paused=true;
				pauseStart=remotenow;
			}
        }else if( oscutils::endsWith(address,"/stop") ) {
			initializing=true;
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
void pluginDripMulti::resetFps(double remotenow,double newfps) {
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
double pluginDripMulti::waitForFrame() {
	double now,target,delta;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	now=tv.tv_sec+tv.tv_usec/1000000.0;

	target=startsec+startusec/1000000.0+n/fps;

	delta=(target-offset)-now;
	if( delta<=0.0 ) {
#ifdef VERBOSE
		log << "n="<<n<<" we are late by "<<delta<<" sec"<<info;
#endif
		return now-startsec-startusec/1000000.0;
	} // we are late already

#ifdef VERBOSE
	log << "delta="<<delta<<info;
#endif

	// we have time to waste. wait!

	// wait seconds first
	while( delta>0.5 ) {
		int s=(int)delta;
#ifdef VERBOSE
		cout <<"[dripmulti] zzzzz 0.5 sec"<<endl;
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
	cout<<"[dripmulti] n="<<n<<" zz "<<delta<<" offset="<<offset<<endl;
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


void pluginDripMulti::dump(ofstream &file) {
	file << "P"<<name << " [shape=\"Mrecord\" label=\"{{<0>0|<2>2}|"<<name<<"|<1>1}\"];" << endl;
	if( !badpos(0) ) file << "Q"<<Qs[0]->name << " -> " << "P"<<name <<":0" <<endl;
	if( !badpos(1) ) file << "P"<<name <<":1" << " -> " << "Q"<<Qs[1]->name <<endl;
	if( !badpos(2) ) file << "Q"<<Qs[2]->name << " -> " << "P"<<name <<":2" <<endl;

}


void pluginDripMulti::computeMatch() {
	// for each out and dup, find the name in inputs and set the 'in' port accordingly
	int i,j;
	// reset
	for(j=0;j<out.size();j++) { out[j].in=-1; }
	for(j=0;j<dup.size();j++) { dup[j].in=-1; }
	beginEventInputNum=-1;
	endEventInputNum=-1;

	// compute
	for(i=0;i<in.size();i++) {
		//cout << "checking in["<<i<<"] name="<<in[i].name<<endl;
		for(j=0;j<out.size();j++) {
			if( out[j].match(in[i]) ) out[j].in=i;
		}
		for(j=0;j<dup.size();j++) {
			if( dup[j].match(in[i]) ) dup[j].in=i;
		}
		if( in[i].match(beginEventInputName) ) beginEventInputNum=i;
		if( in[i].match(endEventInputName) ) endEventInputNum=i;
	}
	//cout << "[dripmulti] beginEvent name="<<beginEventInputName<< " in="<<beginEventInputNum<<" frame="<<beginEventFrame<<endl;
}

void pluginDripMulti::dumpPortInfo() {
	int i;
	cout << "[dripmulti] dump portinfo in="<<in.size()<<" out="<<out.size()<<" dup="<<dup.size()<<endl;
	for(i=0;i<in.size();i++) { cout << "  in[" << i <<"] ";in[i].dump(); }
	for(i=0;i<out.size();i++) { cout << " out[" << i <<"] ";out[i].dump(); }
	for(i=0;i<dup.size();i++) { cout << " dup[" << i <<"] ";dup[i].dump(); }
}




