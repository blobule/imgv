//
// netSlave
//
// plugins used:  all of them
//

/*!

\defgroup netSlave netSlave
@{
\ingroup examples

Generic OSC network slave.<br>
This example will listen to the network for osc commands. The only thing it does is setup a listening
protocol and port, and then execute commands.
It supports udp/tcp/multicast protocols.

Usage
-----------

Generic usage can be udp/tcp/multicast. here 12345 is the listening port.
~~~
netSlave [-udp 12345] [-tcp 12345] [-multicast 226.0.0.1 12345]
~~~

Listens to port 19999, using tcp. This is also the default behavior for netSlave.
~~~
netSlave -tcp 19999
~~~

@}

*/


#include <imgv/imgv.hpp>
#include "config.hpp"

#include <imgv/imqueue.hpp>
#include <imgv/qpmanager.hpp>

#ifdef HAVE_PROFILOMETRE
  #define USE_PROFILOMETRE
  #include <imgv/profilometre.h>
#endif



#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>


using namespace std;


int main(int argc,char *argv[]) {
    cout << "opencv version " << CV_VERSION << endl;
    qpmanager::initialize();

    profilometre_init();

    int done=0;
    for(int i=1;i<argc;i++) {
	if( done ) {
		cout << "argument '"<<argv[i]<<"' skipped"<<endl;
		continue;
	}
	if( strcmp("-udp",argv[i])==0 && i+1<argc ) {
		int port=atoi(argv[i+1]);
    		qpmanager::startNetListeningUdp(port);
		done=1;
		i++;continue;
	}else if( strcmp("-tcp",argv[i])==0 && i+1<argc ) {
		int port=atoi(argv[i+1]);
    		qpmanager::startNetListeningTcp(port);
		done=1;
		i++;continue;
	}else if( strcmp("-multicast",argv[i])==0 && i+2<argc ) {
		int port=atoi(argv[i+1]);
		string ip(argv[i+2]);
    		qpmanager::startNetListeningMulticast(port,ip);
		done=1;
		i+=2;continue;
	}else{
		cout << "unkown argument '"<<argv[i]<<"'"<<endl;
		break;
	}
    }

    if( !done ) {
	cout << "Listening to TCP port 19999" <<endl;
	qpmanager::startNetListeningTcp(19999);
    }

	qpmanager::setNetworkDebug(false);


    // now we wait... ZZZzzzzz
    for(int i=0;;i++) {
        usleep(500000);
    	if((i+1)%20==0) profilometre_dump();
    }

    //qpmanager::dump();
    //qpmanager::dumpDot("out.dot");
}


