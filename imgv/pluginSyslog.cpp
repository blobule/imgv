

#include <imgv/pluginSyslog.hpp>

#include <sys/time.h>
#include <unistd.h>
#include <sstream>

//
// /[info|warn|err] <plugin name> <message> ... other args ...
//
//

//oger \033[1;31mend\033[0m"<<e



pluginSyslog::pluginSyslog() {
	// ports
	ports["in"]=0;

	// ports direction
	portsIN["in"]=true;
}

pluginSyslog::~pluginSyslog() { cout << "[syslog] delete"<<endl;}

void pluginSyslog::init() {
	//cout << "[syslog] init tid=" << pthread_self() << endl;
}

void pluginSyslog::uninit() {
	cout << "[syslog] uninit"<<endl;
}

bool pluginSyslog::loop() {
	//struct timeval tv;

	// attend un message dans la queue 0, puis process tous les messages
	pop_front_ctrl_wait(0);

	pthread_testcancel();

	return true;
}

	//
	// decodage de messages a l'initialisation (init=true)
	// ou pendant l'execution du plugin (init=false)
	// (dans messageDecoder)
	//
bool pluginSyslog::decode(const osc::ReceivedMessage &m) {
	// verification pour un /defevent
	//if( plugin::decode(m) ) return true;

	const char *address=m.AddressPattern();
	//const char *plugin;
	//const char *msg;
	//m.ArgumentStream() >> plugin >> msg >> osc::EndMessage;

	string style;

	if( oscutils::endsWith(address,"/info") ) {
		style="\033[32m";
	}else if( oscutils::endsWith(address,"/warn") ) {
		style="\033[33m";
	}else if( oscutils::endsWith(address,"/err") ) {
		style="\033[31m";
	}else{
		style="\033[36m";
	}

	osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
	cout << style;
	if( arg!=m.ArgumentsEnd() ) {
		cout << "[" << arg->AsStringUnchecked() <<"]";
		arg++;
	}else cout << "[]";
	while( arg!=m.ArgumentsEnd() ) {
		cout << " ";
		if( arg->IsString() ) cout << arg->AsStringUnchecked()<<"";
		else{
			stringstream ss;
			ss << *arg;
			cout << ss.str();
		}
		arg++;
	}
	cout << "\033[0m" << std::endl;
	
	return true;
}



