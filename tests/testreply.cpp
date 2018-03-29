

/** test reply **/
/*
  Concept of a "reply" :
	a message containing a special "reply port" so a replay can be sent by whoever get the message
	The format is :
	/replyto <portname> <id of reply>
	/message that require a reply
	...
	/message that require a reply

	the plugin that gets this bundle should send the reply to the port <portname> and the replay should look like

	/id <id of reply>
	/any reply material
	...
	/any reply material

	id is mandatory and should be a unique number (unique for the queue where the reply will be sent)

	anytime some entity is going to require a reply, it must reserve a port on the plugin
	replyoutport=p->reservePort(false);
	and connect it to the reply queue we are going to use.
	connect("replyqueue",plugin,replyoutport);

	now you can send the message and wait for the answer in replyqueue, sync or async (checking id)
*/


#include <imgv/imgv.hpp>
#include <imgv/qpmanager.hpp>

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2


//
// this plugin counts time, and sends its timer value outside when we ask for it...
//

class pluginLocal : public plugin<blob> {
	int n;

	// any plugin that handle replies must keep this information:
	int replyPort;
	long int replyId;

	public:
	 pluginLocal() {
		ports["in"]=Q_IN;
		ports["out"]=Q_OUT;
		ports["log"]=Q_LOG;
		portsIN["in"]=true;

		replyId=0; // default id
	}
	~pluginLocal() { }
	void init() {
		log << "Init!" << warn;
		n=0;
	}
	// dans le thread du plugin
	bool loop() {
		// are there any message waiting?
		pop_front_ctrl_nowait(Q_IN);
		// let time pass...
		usleepSafe(500000);
		n++;
		return true;
	}
	void uninit() {
		log << "Uninit!" << warn;
	}
	private:
	bool decode(const osc::ReceivedMessage &m) {
		log << "decoding "<<m<<info;
		const char *address=m.AddressPattern();
		if( oscutils::endsWith(address,"/replyto") ) {
			const char *s;
			m.ArgumentStream() >> s >> replyId >> osc::EndMessage;
			replyPort=portMap(string(s));
			cout << "replying to "<< s << " port "<<replyPort << " id " << replyId << endl;
		}else if( oscutils::endsWith(address,"/get/time") ) {
			cout << "got asked for time..." << endl;
			message *m=getMessage(256);
			*m << osc::BeginBundleImmediate;
			*m << osc::BeginMessage("/id") << (int)(replyId) << osc::EndMessage;
			*m << osc::BeginMessage("/time") << n << osc::EndMessage;
			*m << osc::EndBundle;
			push_back_ctrl(replyPort,&m);
		}else{
			cout << "got "<<address<< endl;
		}
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

	qpmanager::addQueue("query");
	qpmanager::addQueue("result");

	qpmanager::connect("query","L","in");

	// get a port for replies
	string replyPort = qpmanager::reservePort("L"); // dont care about the name...
	qpmanager::connect("result","L",replyPort); // make sure replies ends in result queue

	// go!

	qpmanager::start("L");

	for(int i=0;;i++) {
		cout << "..." <<endl;
		usleep(500000);
		// ask for time
		message *m=new message(256);
		*m << osc::BeginBundleImmediate;
		*m << osc::BeginMessage("/replyto") << replyPort << 1234 << osc::EndMessage;
		*m << osc::BeginMessage("/get/time") << osc::EndMessage;
		*m << osc::EndBundle;
		qpmanager::add("query",m);

		// check answers (blocking)
		m=qpmanager::pop_front_ctrl_wait("result");
		m->decode(); // get all the messages in m->msgs vector of ReceivedMessage

		for(int i=0;i<m->msgs.size();i++) {
			const osc::ReceivedMessage &r=m->msgs[i];
			const char *address=r.AddressPattern();
			int n;
			if( oscutils::endsWith(address,"/id") ) {
				long int id;
				r.ArgumentStream() >> id >> osc::EndMessage;
				cout << "got "<<address<< " with id=" << id << endl;
			}else if( oscutils::endsWith(address,"/time") ) {
				long int t;
				r.ArgumentStream() >> t >> osc::EndMessage;
				cout << "got "<<address<< " with time=" << t << endl;
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






