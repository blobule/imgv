

/** test die **/
/*
 make sure killing a plugin is ok, regardless of its state of waiting for a queue
*/


#include <imgv/imgv.hpp>
#include <imgv/qpmanager.hpp>

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2


class pluginLocal : public plugin<blob> {
	int n;

	public:
	 pluginLocal() {
		ports["in"]=Q_IN;
		ports["out"]=Q_OUT;
		ports["log"]=Q_LOG;
		portsIN["in"]=true;
	}
	~pluginLocal() { }
	void init() {
		//log << "init tid=" << tid << warn;
		n=0;
	}
	// dans le thread du plugin
	bool loop() {
		// wait for an image
		blob *i1=pop_front_wait(Q_IN);
		int num=i1->n;
		log << "local got image " << num << warn;
		i1->create(100,100,CV_8UC1);
		i1->n=n;
		push_back(Q_OUT,&i1);
		n++;
		usleepSafe(500000);
		return true;
	}
	void uninit() {
		//cout << "uninit tid=" << tid<<endl;
		log << "uninit" << warn;
	}
	private:
	bool decode(const osc::ReceivedMessage &m) {
		if( plugin::decode(m) ) return true; // check basic messages
		log << "decoding "<<m<<info;
		return true;
	}
};


void stop(string s) {
	cout << "** stopping "<<s<<endl;
	qpmanager::stop(s);
	cout << "** done stopping "<<s<<endl;
}


int main(int argc,char *argv[]) {
	cout << "-- test codedlight --" << endl;

	qpmanager::initialize();

	REGISTER_PLUGIN_TYPE("pluginLocal",pluginLocal);

	qpmanager::addPlugin("L","pluginLocal");

	qpmanager::addQueue("recycle");
	qpmanager::addQueue("display");

	qpmanager::addEmptyImages("recycle",10);

	qpmanager::addPlugin("V","pluginViewerGLFW");

	qpmanager::connect("recycle","L","in");
	qpmanager::connect("display","L","out");

	qpmanager::connect("display","V","in");
	
	message *m=new message(1042);
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "fenetre principale" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << "normal" << osc::EndMessage;
	qpmanager::reservePort("V","key");
	*m << osc::BeginMessage("/defevent-ctrl") << "Q" << "key" << "/key" << "Q" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "V" << "key" << "/key" << "V" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "L" << "key" << "/key" << "L" << osc::EndMessage;
	*m << osc::BeginMessage("/defevent-ctrl") << "X" << "key" << "/test" << 1234 << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	qpmanager::addQueue("keys");
	qpmanager::connect("keys","V","key");

	// go!

	qpmanager::start("V");
	qpmanager::start("L");

	qpmanager::dumpDot("out.dot");

	// connect au plugin syslog
	
	//
	// this decoder will store the messages so we can process then without "callback"
	//
	class myDecoder : public messageDecoder {
		public:
		vector<osc::ReceivedMessage> msgs;
		bool decode(const osc::ReceivedMessage &m) {
			//std::cout << "got " << m << std::endl;
			msgs.push_back(m);
			return true;
		}
	};

	bool done=false;

	for(;!done;) {
		cout << "..." <<endl;
		//cout << "size is " << qpmanager::queueSizeCtrl("keys") <<endl;
		//
		// process messages
		//
		for(;;) {
			message *m1=qpmanager::pop_front_ctrl_nowait("keys");
			//message *m1=dynamic_cast<message *>(qpmanager::getQueue("keys")->pop_front_ctrl());
			if( m1==NULL ) break;
			myDecoder md;
			m1->decode(md);
			cout << "total: got "<<md.msgs.size()<<endl;
			for(int i=0;i<md.msgs.size();i++) {
				cout << "msg "<<i<<" is "<<md.msgs[i]<<endl;
				const char *address=md.msgs[i].AddressPattern();
				if( oscutils::endsWith(address,"/key") ) {
					const char *s;
					md.msgs[i].ArgumentStream() >> s >> osc::EndMessage;
					cout << "KEY "<<s<<endl;
					if( s[0]=='V' ) { stop("V"); }
					if( s[0]=='L' ) { stop("L"); }
					if( s[0]=='Q' ) { done=true; }
				}else{
					cout << "UNKNOWN : "<<md.msgs[i]<<endl;
				}
			}
			recyclable::recycle((recyclable **)&m1); // after this, can't use m1 anymore
			if( !qpmanager::status("L") ) done=true;
			if( !qpmanager::status("V") ) done=true;
		}
		usleep(500000);
	}

	cout << "the end..."<<endl;

	qpmanager::stop("V");
	qpmanager::stop("L");
}






