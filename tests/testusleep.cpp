

/** test usleep **/
/*
It seems there is a bug when you cancel a thread while it is inside usleep
*/


#include <imgv/imgv.hpp>
#include <imgv/qpmanager.hpp>

#define Q_IN	0
#define Q_LOG	1


class pluginLocal : public plugin<blob> {
	long int type;

	public:
	 pluginLocal() {
		ports["in"]=Q_IN; portsIN["in"]=true;
		ports["log"]=Q_LOG;

		type=0;
	}
	~pluginLocal() { }
	void init() {
		log << "Init!" << warn;
	}
	// dans le thread du plugin
	bool loop() {
		// check for messages. DO NOT WAIT.
		pop_front_ctrl_nowait(Q_IN);

		if( type==0 ) {
			// this should work
			usleepSafe(500000);
		}else if( type==1 ) {
			// this should not work
			usleep(500000);
		}else if( type==2 ) {
			// this should work
#ifdef WIN32

#else
			sleep(1);
#endif
		}

		log << "local!" << info;
		return true;
	}
	void uninit() {
		log << "Uninit!" << warn;
	}
	private:
        bool decode(const osc::ReceivedMessage &m) {
		log << "decoding "<<m<<info;
		const char *address=m.AddressPattern();

		if( oscutils::endsWith(address,"/type") ) {
			m.ArgumentStream() >> type >> osc::EndMessage;
		}

                return true;
        }

};




int main(int argc,char *argv[]) {
	cout << "-- test usleep --" << endl;
	cout << "-- usage: testusleep   -> test fixed usleep" << endl;
	cout << "-- usage: testusleep -nofix  -> test original usleep (should crash)" << endl;
	cout << "-- usage: testusleep -sleep  -> test original sleep (should not crash)" << endl;

	int type=0;
	if( argc>1 && strcmp(argv[1],"-nofix")==0 ) type=1;
	if( argc>1 && strcmp(argv[1],"-sleep")==0 ) type=2;

	qpmanager::initialize();

	REGISTER_PLUGIN_TYPE("pluginLocal",pluginLocal);

	qpmanager::addPlugin("L","pluginLocal");

	qpmanager::addQueue("local");
	qpmanager::connect("local","L","in");

	// go!

	qpmanager::start("L");

	message *m=new message(1042);
        *m << osc::BeginMessage("/type") << type << osc::EndMessage;
	qpmanager::add("local",m);
	

	for(int i=0;i<5;i++) {
		cout << "..." <<endl;
		usleep(500000);
		if( !qpmanager::status("L") ) break;
	}

	// stop explicitely the plugin
	qpmanager::stop("L");

	cout << "done!"<<endl;
}






