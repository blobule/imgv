

/** test die **/
/*
 make sure killing a plugin is ok, regardless of its state of waiting for a queue
*/


#include <imgv/imgv.hpp>
#include <imgv/qpmanager.hpp>

// qin -> net(to localhost:4444)
// net(from 4444) -> qout

int main(int argc,char *argv[]) {
	cout << "-- test net plugin --" << endl;

	int in; // 0=non, 1=udp, 2=tcp
	int out; // 0=non, 1=udp, 2=tcp, 3=multicast

	char machine[100]; // seulement pour out
	int port;

	in=0;
	strcpy(machine,"localhost");
	//strcpy(machine,"226.0.0.1");
	port=4444;
	out=0;

	for(int i=1;i<argc;i++) {
		if( strcmp("-h",argv[i])==0 ) {
			cout << "usage: testnet [-in udp|tcp 4444] [-in multicast 229.0.0.1 4444] [-out udp|multicast|tcp machine 4444]"<<endl;
			cout << "on peut faire -in et -out en meme temps"<<endl;
			exit(0);
		}
		if( strcmp("-in",argv[i])==0 && i+2<argc ) {
			if( strcmp(argv[i+1],"udp")==0 ) in=1;
			else if( strcmp(argv[i+1],"tcp")==0 ) in=2;
			else if( strcmp(argv[i+1],"multicast")==0 ) {
				in=3;
				strcpy(machine,argv[i+2]);
				i++;
			}
			port=atoi(argv[i+2]);
			i+=2;continue;
		}
		if( strcmp("-out",argv[i])==0 && i+3<argc ) {
			if( strcmp(argv[i+1],"udp")==0 ) out=1;
			else if( strcmp(argv[i+1],"tcp")==0 ) out=2;
			else if( strcmp(argv[i+1],"multicast")==0 ) out=3;
			strcpy(machine,argv[i+2]);
			port=atoi(argv[i+3]);
			i+=3;continue;
		}
	}

	// nothing = both
	if( in==0 && out==0 ) { in=2;out=2; } // multicast

	qpmanager::initialize();

	message *m;

	if( out>0 ) {
		// ce qu'on met dans IN va aller sur le reseau
		qpmanager::addPlugin("A","pluginNet");
		qpmanager::addQueue("IN");
		qpmanager::connect("IN","A","in");
		qpmanager::addQueue("cmdA");
		qpmanager::connect("cmdA","A","cmd");
		//
		// tell A to send from IN port to localhost:4444
		//
		m=new message(256);
		switch( out ) {
		  case 1: *m << osc::BeginMessage("/out/udp"); break;
		  case 2: *m << osc::BeginMessage("/out/tcp"); break;
		  case 3: *m << osc::BeginMessage("/out/multicast"); break;
		}
		*m << machine << port << osc::EndMessage;
		qpmanager::add("cmdA",m);

		qpmanager::start("A");
	}
	if( in>0 ) {
		// ce qu'on recoit du reseau va aller dans OUT
		qpmanager::addPlugin("B","pluginNet");
		qpmanager::addQueue("OUT");
		qpmanager::addQueue("cmdB");
		qpmanager::connect("OUT","B","out");
		qpmanager::connect("cmdB","B","cmd");
		// on doit fournir une queue d'entree de recyclage pour les images
		qpmanager::addQueue("recycle");
		qpmanager::addEmptyImages("recycle",10);
		qpmanager::connect("recycle","B","in");
		//
		// tell B to receive from port 4444 and put evertything in OUT port
		//
		m=new message(256);
		switch( in ) {
		  case 1: *m << osc::BeginMessage("/in/udp");break;
		  case 2: *m << osc::BeginMessage("/in/tcp");break;
		  case 3: *m << osc::BeginMessage("/in/multicast") << machine ;break;
		}
		*m << port << osc::EndMessage;
		qpmanager::add("cmdB",m);

		qpmanager::start("B");
	}

	// recycling for images
	qpmanager::addQueue("recycleLocal");
	qpmanager::addEmptyImages("recycleLocal",10);

	//qpmanager::dumpDot("out.dot");

	// connect au plugin syslog

	blob *img;
	
	for(int i=0;;i++) {
		cout << "... " <<i<<endl;
		if( in>0 ) {
			// check for messages in queue OUT
			// pour l'instant, on recoit tout dans la queue normale, meme les messages ctrl
			cout << "out queue size is "<<qpmanager::queueSize("OUT")<<" ctrl="<<qpmanager::queueSizeCtrl("OUT")<<endl;
			for(;;) {
				if( out>0 ) {
					qpmanager::pop_front_nowait("OUT",m,img);
				}else{
					qpmanager::pop_front_wait("OUT",m,img);
				}
				//cout << "pop_front_wait OUT returned " << m << " and " << img<<endl;
				if( m==NULL && img==NULL ) break; // nothing to read anymore
				if( m!=NULL ) {
					cout << "decoding"<<endl;
					m->decode();
					for(int j=0;j<m->msgs.size();j++) {
						cout << "got message : "<<m->msgs[j]<<endl;
					}
					recyclable::recycle((recyclable **)&m);
				}else{ // img!=NULL
					cout << "got an image!!" <<endl;
					recyclable::recycle((recyclable **)&img);
				}
			}
		}

		if( out>0 ) {
			// make a new message to send
			m=new message(256);
			*m << osc::BeginMessage("/yo") << i << osc::EndMessage;
			cout << "adding message : /yo "<< i <<endl;
			qpmanager::add("IN",m);

			m=new message(256);
			*m << osc::BeginBundleImmediate;
			*m << osc::BeginMessage("/b1") << i << osc::EndMessage;
			*m << osc::BeginMessage("/b2") << i << osc::EndMessage;
			*m << osc::EndBundle;
			cout << "adding message : /b1 and /b2 "<<i<<endl;
			qpmanager::addWithImages("IN",m);

			m=new message(256);
			*m << osc::BeginBundleImmediate;
			*m << osc::BeginMessage("/c1") << i << osc::EndMessage;
			*m << osc::BeginMessage("/c2") << i << osc::EndMessage;
			*m << osc::EndBundle;
			cout << "adding message: /c1 and /c2 "<<i<<endl;
			qpmanager::add("IN",m);

			// make a new image to send
			qpmanager::pop_front_wait("recycleLocal",m,img); // m ne sert a rien ici
			if( m!=NULL ) recyclable::recycle((recyclable **)&m);
			img->create(24,32,CV_8UC1);
			img->n=i;
			cout << "adding image : n="<<img->n<<endl;
			qpmanager::add("IN",img);
			usleep(5000);	
		}
	}

	cout << "the end..."<<endl;

	if( out>0 )  qpmanager::stop("A");
	if( in>0 )  qpmanager::stop("B");
}






