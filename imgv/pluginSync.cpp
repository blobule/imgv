

#include <imgv/pluginSync.hpp>

#include <sys/time.h>
#include <unistd.h>


//
// sync plugin
//
// [0] : queue d'entree des images
// [1] : queue de sortie des images
// [3+] : queue de sortie des messages de synchro (/sync) 
//
// mode automatic:  laisse passer les images de [0] vers [1].
// mode manuel   :  attend un /go pour laisser passer une image de [0] vers [1]
//
// en plus, le triage permet d'associer des messages a envoyer lorsque l'image est envoyee.
// un delai optionel peut etre donne pour retarder l'envoi des messages. (/delay 0.001)
//
// si les images arrivent plus vite que le delai, ca va ralentir et eventuellement bloquer.
//
// le seul evenement auquel on peut attacher des messages est:
// EVENT_NO_DELAY
// EVENT_DELAY
//

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2

pluginSync::pluginSync() {
	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=Q_LOG;

	portsIN["in"]=true;
}

pluginSync::~pluginSync() { log << "delete"<<warn;}

void pluginSync::init() {
	//defOutgoingMessage(EVENT_DELAY,3,"/sync");
	delay=0.0;
	offset=0.0;
	manual=true;
	nbGo=0;
	togo=0; // == no limit to nb of images
	n=0; // image counter
    send_events=true;

	nbOut=1;
	outputNames[0]="out"; // by default
	outputNums[0]=Q_OUT;
	//log << "init tid=" << pthread_self() << warn;
}

void pluginSync::uninit() { log << "uninit"<<warn;  }

bool pluginSync::loop() {
	struct timeval tv;

	if( manual ) {
		// attend un (ou plus) message(s)
		pop_front_ctrl_wait(Q_IN);
	}else { pop_front_ctrl_nowait(Q_IN);nbGo=1; } // automatic: 1 /go a chaque loop

	for(;nbGo>0;nbGo--) {

		// attend une image en entree
		// pas de processing des messages
		blob *i1=pop_front_noctrl_wait(Q_IN);
        
#ifdef OLD_STUFF
		// laisse l'image pousuivre son chemin
		push_back(Q_OUT,&i1);
#else
		// new stuff... pick the smallest queue. Use image number as offset to be fair
		int i,j;
		int min=999999;
		int jmin=-1;
		for(i=0;i<nbOut;i++) {
			int j=(n+i)%nbOut;
			if( outputNums[j]<0 ) { outputNums[j]=portMap(outputNames[j]); } // map port if needed
			if( outputNums[j]<0 ) continue; // skip unknown port
			if( badpos(outputNums[j]) ) continue;
			int s=Qs[outputNums[j]]->size();
			if( s<min ) { min=s;jmin=j; }
		}
		if( jmin>=0 )
        {
          push_back(outputNums[jmin],&i1);
        }
		else recycle(i1); // should output port name problem
#endif

        if (send_events)
        {
		  // quelle heure est-il?
		  gettimeofday(&tv,NULL);
		  double now=tv.tv_sec+tv.tv_usec/1000000.0;

		  // envoi un message tout de suite
		  checkOutgoingMessages("no-delay",now,-1,-1);

		  if( delay>=0.0 ) {
			// dormir un peu...
			double target=now+delay;
			double delta=delay-offset;
			// sleep approx
			while( delta>0.5 ) {
				//int s=(int)delta;
				usleepSafe(500000);
				// check time
				gettimeofday(&tv,NULL);
				double now=tv.tv_sec+tv.tv_usec/1000000.0;
				if( target<now ) break;
				delta=(target-offset)-now;
			}
			if( delta>0.0 ) {
				// sleep precis
				usleepSafe((int)(delta*1000000));
			}
			// ajuste l'horloge
			gettimeofday(&tv,NULL);
			 // en ms
			double diff=(tv.tv_sec*1000.0+tv.tv_usec/1000.0)-target*1000.0;
			now=tv.tv_sec+tv.tv_usec/1000000.0;
			// ajustement EN DOUCEUR!!!! sinon, over/under/over/under...
			offset = offset*0.9+0.1/1000.0*diff;

			checkOutgoingMessages("delay",now,-1,-1);
		  }
        }

		n++;
		togo--;
		if( togo==0 ) return false; // we are done!!!!
	}
	return true;
}

	//
	// decodage de messages a l'initialisation (init=true)
	// ou pendant l'execution du plugin (init=false)
	// (dans messageDecoder)
	//
bool pluginSync::decode(const osc::ReceivedMessage &m) {
	// verification pour un /defevent
	if( plugin::decode(m) ) return true;

	const char *address=m.AddressPattern();
	// en tout temps...
	if( oscutils::endsWith(address,"/go") ) {
		nbGo++;
	}else if( oscutils::endsWith(address,"/count") ) {
		m.ArgumentStream() >> togo >> osc::EndMessage;
		log << "togo set to "<<togo<<info;
	}else if( oscutils::endsWith(address,"/manual") ) {
		manual=true;
	}else if( oscutils::endsWith(address,"/auto") ) {
		manual=false;
	}else if( oscutils::endsWith(address,"/send-events") ) {
		m.ArgumentStream() >> send_events >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/delay") ) {
		float val;
		m.ArgumentStream() >> val >> osc::EndMessage;
		delay=val;
		//log << "[sync] delai a "<< delay <<" secondes"<<info;
	}else if( oscutils::endsWith(address,"/distribute") ) {
		int i;
		osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		for(i=0;i<100 && arg!=m.ArgumentsEnd();i++) {
			outputNames[i]=arg->AsString();
			outputNums[i]=-1; // we solve this later when running.
				// maybe we should do it now and report error if undefined ports now.
            arg++;
		}
		nbOut=i;
	}else{
		log << "commande inconnue: "<<m<<warn;
		return false;
	}
	return true;
}



