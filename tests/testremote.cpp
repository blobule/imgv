

/** test remote **/
/*
  This example simply runs the qpmanager network plugin on port 44444.
  It will be used to test commands to manage queues and plugins.
*/


#include <imgv/imgv.hpp>
#include <imgv/qpmanager.hpp>

int main(int argc,char *argv[]) {
	cout << "-- test remote --" << endl;

	int type=-1;
	int port=4444;
	string ip="226.0.0.1";

	for(int i=1;i<argc;i++) {
		if( strcmp("-h",argv[i])==0 ) {
			type=-1;
			break;
		}else if( strcmp("-udp",argv[i])==0 && i+1<argc ) {
			type=0;
			port=atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp("-multicast",argv[i])==0 && i+2<argc ) {
			type=1;
			ip=string(argv[i+1]);
			port=atoi(argv[i+2]);
			i+=2;continue;
		}else if( strcmp("-tcp",argv[i])==0 && i+1<argc ) {
			type=2;
			port=atoi(argv[i+1]);
			i++;continue;
		}
	}

	qpmanager::initialize();

	switch( type ) {
		case -1:
			cout << "Usage: testremote [-udp 4444 | -multicast 226.0.0.1 4444 | -tcp 4444]" <<endl;
			exit(0);
		case 0: qpmanager::startNetListeningUdp(port);break;
		case 1: qpmanager::startNetListeningMulticast(port,ip);break;
		case 2: qpmanager::startNetListeningTcp(port);break;
	}


	for(;;) {
		usleep(500000);
	}

	cout << "the end..."<<endl;
}






