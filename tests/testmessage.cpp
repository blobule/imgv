

/** test message **/
/*
  testing how to handle a message in the main thread.
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
		// wait, then send a message out
		usleepSafe(500000);
		message *m=getMessage(256);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/n") << n << osc::EndMessage;
		*m << osc::BeginMessage("/blub") << osc::EndMessage;
		*m << osc::BeginMessage("/test") << osc::EndMessage;
		*m << osc::BeginMessage("/toto") << 1 << 2 << "blub" << osc::EndMessage;
		*m << osc::EndBundle;
		push_back_ctrl(Q_OUT,&m);
		n++;
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
	cout << "-- test message --" << endl;

	qpmanager::initialize();

	REGISTER_PLUGIN_TYPE("pluginLocal",pluginLocal);

	qpmanager::addPlugin("L","pluginLocal");

	qpmanager::addQueue("result");

	qpmanager::connect("result","L","out");

	// go!

	qpmanager::start("L");

	for(;;) {
		cout << "..." <<endl;
		message *m=qpmanager::pop_front_ctrl_wait("result");

		m->decode(); // get all the messages in m->msgs vector of ReceivedMessage
		cout << "got message size=" << m->msgs.size() << endl;

		for(int i=0;i<m->msgs.size();i++) {
			const osc::ReceivedMessage &r=m->msgs[i];
			const char *address=r.AddressPattern();
			long int n;
			if( oscutils::endsWith(address,"/n") ) {
				r.ArgumentStream() >> n >> osc::EndMessage;
				cout << "got "<<address<< " with " << n << endl;
			}else{
				cout << "got "<<address<< endl;
			}
		}
			
		recyclable::recycle((recyclable **)&m); // after this, can't use m anymore
		if( !qpmanager::status("L") ) break;
	}

	cout << "the end..."<<endl;

	qpmanager::stop("L");
}






