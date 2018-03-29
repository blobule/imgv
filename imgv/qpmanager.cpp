

// attention config.hpp ici pour assurer qu'on ne ramasse pas le config.hpp /usr/local
#include "config.hpp"

#include <imgv/qpmanager.hpp>

#include <imgv/udpcast.h>
#include <imgv/tcpcast.h>


// pour itoa()
#include <stdio.h>
//#include <stdlib.h>

#ifdef HAVE_PLUGIN_V4L2
	#include <imgv/pluginV4L2.hpp>
#endif
#ifdef HAVE_PLUGIN_WEBCAM
	#include <imgv/pluginWebcam.hpp>
#endif
#ifdef HAVE_PLUGIN_NET
	#include <imgv/pluginNet.hpp>
#endif
#ifdef HAVE_PLUGIN_STREAM
	#include <imgv/pluginStream.hpp>
#endif
#ifdef HAVE_PLUGIN_VIEWER_GLFW
	#include <imgv/pluginViewerGLFW.hpp>
#endif
#ifdef HAVE_PLUGIN_VIEWER_GLES2
	#include <imgv/pluginViewerGLES2.hpp>
#endif
#ifdef HAVE_PLUGIN_RASPICAM
	#include <imgv/pluginRaspicam.hpp>
#endif
#ifdef HAVE_PLUGIN_HOMOG
	#include <imgv/pluginHomog.hpp>
#endif
#ifdef HAVE_PLUGIN_SYSLOG
	#include <imgv/pluginSyslog.hpp>
#endif
#ifdef HAVE_PLUGIN_PATTERN
	#include <imgv/pluginPattern.hpp>
#endif
#ifdef HAVE_PLUGIN_DRIP
	#include <imgv/pluginDrip.hpp>
#endif
#ifdef HAVE_PLUGIN_DRIPMULTI
	#include <imgv/pluginDripMulti.hpp>
#endif
#ifdef HAVE_PLUGIN_SAVE
	#include <imgv/pluginSave.hpp>
#endif
#ifdef HAVE_PLUGIN_READ
	#include <imgv/pluginRead.hpp>
#endif
#ifdef HAVE_PLUGIN_FFMPEG
	#include <imgv/pluginFfmpeg.hpp>
#endif
#ifdef HAVE_PLUGIN_PROSILICA
	#include <imgv/pluginProsilica.hpp>
#endif
#ifdef HAVE_PLUGIN_PTGREY
	#include <imgv/pluginPtGrey.hpp>
#endif
#ifdef HAVE_PLUGIN_LEOPARD
	#include <imgv/pluginLeopard.hpp>
#endif


using namespace std;

//
// any commande of the form /@/ is "root" command for qpmanager:
// (must add possibility of query list of queues, list of plugins, etc...)
// /@/addqueue recycle  -> add a new queue named "recycle"
// /@/addplugin blub pluginViewerGLFW  -> add a new plugin named blub, instance of pluginViewerGLFW
// /@/reservePort portname pluginname
// /@/reservePort pluginname -> devrait retourne le nom du port...  
// /@/connect queuename pluginname portname
// /@/start pluginname
// /@/stop pluginname
// /@/addEmptyImages queuename nbimg
//



//
// pluginReseau prive
// -> receive net messages on a specified port for any active queue
//
class pluginReseau : public plugin<blob>, PacketListener {
	int port;
	bool useTcp;
	unsigned char buf[2048]; 
	tcpcast tcp;

	public:
	pluginReseau(int port,bool tcp) :port(port), useTcp(tcp) {
		ports["log"]=0;
	}
	~pluginReseau() { }

	void init() {
		log << "Init! port=" << port << " tcp="<<useTcp<< warn;
	}


	bool loop() {
	    if( useTcp ) {
			int k=tcp_server_init(&tcp,port);
		if( k ) {
			log << "unable to listen for tcp connexion on port "<<port<<err;
		}else{
			int len;
			int nb;
			char c;
			for(;;) {
				log << "waiting for connection"<< warn;
				tcp_server_wait(&tcp);
				for(;;) {
					// check connexion
					if( recv(tcp.remote_sock,&c, 1, MSG_PEEK)==0 ) { log << "client has closed." <<err; break; }
					// get length of packet
					int i=0;
					char preamble[4];
					const char *p=preamble;
					while( i<4 ) {
						if( recv(tcp.remote_sock,&c, 1, MSG_PEEK)==0 ) { len=-1;break; }
							nb=tcp_receive_data(&tcp,p+i,4-i);
							if( nb<0 ) { len=-1; break; }
							i+=nb;
						}
						len=oscutils::ToInt32(preamble);
						if( len<0 ) break;
						if( len>sizeof(buf) ) break;
						// get the packet!
						for(i=0;i<len;) {
							if( recv(tcp.remote_sock,&c, 1, MSG_PEEK)==0 ) { len=-1;break; }
							nb=tcp_receive_data(&tcp,(const char *)buf+i,len-i);
							if( nb<0 ) { len=-1; break; }
							i+=nb;
						}
						if( len<0 ) break;
						try {
							ProcessPacket((const char *)buf,len);
						} catch(...) { log << "EX! reseau\n" <<err; }
					}
					tcp_server_close_connection(&tcp);
				}
				tcp_server_close(&tcp);
			}
	    }else{
		UdpListeningReceiveSocket s( IpEndpointName( IpEndpointName::ANY_ADDRESS, port ), this );
		s.Run();
	    }
	    return false;
	}

	// de PacketListener
	void ProcessPacket( const char *data, int size, const IpEndpointName& remoteEndpoint ) {
		ProcessPacket( data, size);
	}

	void ProcessPacket( const char *data, int size) {
		log << "GOT message len="<<size<<info;
		// trouve la bonne queue pour routage
		osc::ReceivedPacket p(data,size);
		if( p.IsBundle() ) {
			log << "unable to route bundled messages"<<err;
			return;
		}
		osc::ReceivedMessage m(p);
		const char *address=m.AddressPattern();

		int nb=oscutils::extractNbArgs(address);
		int pos,len;
		if( nb>1 ) {
		oscutils::extractArg(address,1,pos,len); // second arg
		string second(address+pos,len);
		if( second=="@" ) {
			log << "special /@ command" << info;
			if( oscutils::endsWith(address,"/@/addQueue") ) {
				const char *qname;
				m.ArgumentStream() >> qname >> osc::EndMessage;
				qpmanager::addQueue(qname);
			}else if( oscutils::endsWith(address,"/@/addPlugin") ) {
				const char *pname,*ptype;
				m.ArgumentStream() >> pname >> ptype >> osc::EndMessage;
				qpmanager::addPlugin(pname,ptype);
			}else if( oscutils::endsWith(address,"/@/reservePort") ) {
				const char *portname,*pname;
				m.ArgumentStream() >> portname >> pname >> osc::EndMessage;
				qpmanager::reservePort(pname,portname);
			}else if( oscutils::endsWith(address,"/@/connect") ) {
				const char *qname,*pname,*portname;
				m.ArgumentStream() >> qname >> pname >> portname >> osc::EndMessage;
				qpmanager::connect(qname,pname,portname);
			}else if( oscutils::endsWith(address,"/@/start") ) {
				const char *pname;
				m.ArgumentStream() >> pname >> osc::EndMessage;
				qpmanager::start(pname);
			}else if( oscutils::endsWith(address,"/@/stop") ) {
				const char *pname;
				m.ArgumentStream() >> pname >> osc::EndMessage;
				qpmanager::stop(pname);
			}else if( oscutils::endsWith(address,"/@/addEmptyImages") ) {
				const char *qname;
				int32 nb;
				m.ArgumentStream() >> qname >> nb >> osc::EndMessage;
				qpmanager::addEmptyImages(qname,nb);
			}else if( oscutils::endsWith(address,"/@/dumpDot") ) {
				const char *fname;
				m.ArgumentStream() >> fname >> osc::EndMessage;
				qpmanager::dumpDot(fname);
			}
		}else{
			// route message dans la queue specifiee
			log << "add="<<address<<", premier arg is pos="<<pos<<", len="<<len<<info;

			oscutils::extractArg(address,nb-1,pos,len); // premier arg
			string qname(address+pos,len);
			imqueue* Q=qpmanager::getQueue(qname); // ou getQ? sans lock
			if( Q==NULL ) {
			    log << "unable to route message: "<< m << err;
			    return;
			}
			// creer le message et l'envoyer
			message *msg=new message(size);
			msg->set(data,size);
			Q->push_back_ctrl(msg);
		}
		}
	}
}; // pluginReseau


//
// super qpmanager network plugin...
// it can only receive, but in udp/multicast/tcp
//


class pluginNet2 : public plugin<blob> {
    public:
	static const int TYPE_TCP = 0;
	static const int TYPE_UDP = 1;
	static const int TYPE_MULTICAST = 2;
    private:
	int type; // TYPE_* Note that 
	int n; // simple counter

	imgv::rqueue<recyclable> Qimages;  // our local image queue, for receiving images...

	string ip; // only for multicast (226.0.0.1)
	int port;
	bool useTcp;

	// for tcp modes
	tcpcast tcp;
	// for udp/multicast modes
	udpcast udp;

	unsigned char buf[2048]; // devrait etre > mtu

	//
	// sending images
	//
	int mtu; // taille d'un paquet udp
	int delay; // en microsecond, when sending images

	//
	// receiving images
	//
	int32 n0; // current image id (to identify the correct packets)
	blob *i0; // current received image
	int32 dsz; // current data size
	imqueue *Q; // current image queue where we should send the image, NULL si unknown

    public:
	pluginNet2(int port,bool tcp,string ip) :port(port), useTcp(tcp), ip(ip) {
        	ports["log"]=0; // no in/out ports! very special plugin!!!
		if( useTcp ) {
			type=TYPE_TCP;
		}else if( ip.empty() ) {
			type=TYPE_UDP;
		}else{
			type=TYPE_MULTICAST;
		}
	}
	~pluginNet2() {}
	void init() {
		log << "init! type="<<type<<" port="<<port<<warn;
		n=0;
		n0=-1;
		mtu=1500;
		delay=0;
		i0=NULL;

		// add some images to local Qimages
		for(int i=0;i<3;i++) {
			blob *image=new blob();
			image->n=i;
			image->setRecycleQueue(&Qimages);
			Qimages.push_back((recyclable **)&image);
		}

		if( type==TYPE_UDP ) {
			int k=udp_init_receiver(&udp,port,NULL);
			if( k<0 ) {
				log << "network: Unable to listen to port "<<port<<err;
				exit(0);
			}
			log << "Listening UDP to port "<<port<<warn;
		}else if( type==TYPE_MULTICAST ) {
			int k=udp_init_receiver(&udp,port,(char *)ip.c_str());
			if( k<0 ) {
				log << "network: Unable to listen to ip "<<ip<<" port "<<port<<err;
				exit(0);
			}
			log << "Listening MULTICAST to port "<<port<<warn;
		}else if( type==TYPE_TCP ) {
			int k=tcp_server_init(&tcp,port);
			if( k ) {
				log << "unable to open outgoing tcp to " << ip << " port " <<port << err;
				exit(0);
			}
			log << "Listening TCP port "<<port<<warn;
		}
	}

	void uninit() {
		log << "uninit"<<warn;
		// on devrait faire udp_uninit etc...
	}


	bool loop() {
		if( type==TYPE_TCP ) {
			int k;
			if( !tcp_server_is_connected(&tcp) ) {
				log << "waiting for tcp connexion"<<warn;
				tcp_server_wait(&tcp);
			}
			int32 len;
			char preamble[4];
			// get packet size
			k=tcp_receive_data_exact(&tcp,(const char *)preamble,4);
			len=oscutils::ToInt32(preamble);
			if( k==0 ) {
				log << "lost TCP connexion"<<err;
				tcp_client_close(&tcp); // ferme officiellement la connexion
				return true;
			}
			if( len>sizeof(buf) ) {
				// skip this packet
				log << "TCP packet size "<<len<<" too big. Skipping."<<err;
				while( len>0 ) {
					int n=len;
					if( n>sizeof(buf) ) n=sizeof(buf);
					k=tcp_receive_data_exact(&tcp,(const char *)buf,n);
					if( k==0 ) {
						log << "lost TCP connexion 2"<<err;
						tcp_client_close(&tcp); // ferme officiellement la connexion
						break; // lost connexion. done.
					}
					len-=k;
				}
				return true;
			}
			// get packet
			k=tcp_receive_data_exact(&tcp,(const char *)buf,len);
			if( k>0 ) processInMessage(buf,len);
			return true;
		}else{
			// wait for something on the network (-1=err, otherwise its the size)
			// same for UDP and multicast
			int sz=udp_receive_data(&udp,buf,sizeof(buf));
			if( qpmanager::networkDebug ) log << "got UDP message size="<<sz<<info;
			processInMessage(buf,sz);
		}
		return true;
	}

    private:




void processInMessage(unsigned char *buf,int sz) {
	message *m=getMessage(sz);
	m->set((const char *)buf,sz);
	// check special /queuename/@@@/stream/image or /queuename/@@@/stream/data to reconstruct images
	// SHOULD CHECK FASTER IF IT IS A /@@@ MESSAGE!
	m->decode();
	for(int i=0;i<m->msgs.size();i++) {
		if( qpmanager::networkDebug ) log << "processing message :" << m->msgs[i] << info;
		const char *address=m->msgs[i].AddressPattern();
		int nb=oscutils::extractNbArgs(address);
		if( oscutils::endsWith(address,"/@/addQueue") ) {
			const char *qname;
			m->msgs[i].ArgumentStream() >> qname >> osc::EndMessage;
			qpmanager::addQueue(qname);
			log << "addQueue " << qname << warn;
		}else if( oscutils::endsWith(address,"/@/addPlugin") ) {
			const char *pname,*ptype;
			m->msgs[i].ArgumentStream() >> pname >> ptype >> osc::EndMessage;
			qpmanager::addPlugin(pname,ptype);
			log << "addPlugin " << pname << warn;
		}else if( oscutils::endsWith(address,"/@/reservePort") ) {
			const char *portname,*pname;
			m->msgs[i].ArgumentStream() >> portname >> pname >> osc::EndMessage;
			qpmanager::reservePort(pname,portname);
			log << "reservePort " << portname << " " << pname << warn;
		}else if( oscutils::endsWith(address,"/@/connect") ) {
			const char *qname,*pname,*portname;
			m->msgs[i].ArgumentStream() >> qname >> pname >> portname >> osc::EndMessage;
			qpmanager::connect(qname,pname,portname);
			log << "connect " << qname << " " << pname << " " << portname << warn;
		}else if( oscutils::endsWith(address,"/@/loadImage") ) {
			const char *filename,*qname,*view;
			m->msgs[i].ArgumentStream() >> filename >> qname >> view >> osc::EndMessage;
			cv::Mat image=cv::imread(filename,-1);
			blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
			i1->view=view;
			qpmanager::addImage(qname,i1);
			log << "loadImage " << filename << " to queue "<<qname<<" (view "<<view<<")" << warn;
		}else if( oscutils::endsWith(address,"/@/start") ) {
			const char *pname;
			m->msgs[i].ArgumentStream() >> pname >> osc::EndMessage;
			qpmanager::start(pname);
			log << "start " << pname << warn;
		}else if( oscutils::endsWith(address,"/@/stop") ) {
			const char *pname;
			m->msgs[i].ArgumentStream() >> pname >> osc::EndMessage;
			qpmanager::stop(pname);
			log << "stop " << pname << warn;
		}else if( oscutils::endsWith(address,"/@/addEmptyImages") ) {
			const char *qname;
			int32 nb;
			m->msgs[i].ArgumentStream() >> qname >> nb >> osc::EndMessage;
			qpmanager::addEmptyImages(qname,nb);
			log << "addEmptyImages " << qname << " " << nb << warn;
		}else if( oscutils::endsWith(address,"/@/dumpDot") ) {
			const char *fname;
			m->msgs[i].ArgumentStream() >> fname >> osc::EndMessage;
			qpmanager::dumpDot(fname);
			log << "dumpDot " << fname << warn;
		}else if( oscutils::endsWith(address,"/@@@/stream/image") ) {
			receiveImageStart(m->msgs[i]);
		}else if( oscutils::endsWith(address,"/@@@/stream/data") ) {
			receiveImageData(m->msgs[i]);
		}else{
			// route message dans la queue specifiee
			int pos,len;
			oscutils::extractArg(address,nb-1,pos,len); // premier arg
			//log << "add="<<address<<", premier arg is pos="<<pos<<", len="<<len<<info;
			string qname(address+pos,len);
			//log << "qname is "<<qname<<info;
			imqueue* Q=qpmanager::getQueue(qname); // ou getQ? sans lock
			if( Q==NULL ) {
			    log << "unable to route message: "<< m->msgs[i] << err;
			    return;
			}
			// creer le message et l'envoyer
			//log << "msg "<<i<<" size is "<<m->msgs[i].size<<info;
			message *msg=getMessage(m->msgs[i].size); // will be recycled instead of alloc/free
			msg->set(m->msgs[i].data,m->msgs[i].size);
			if( qpmanager::networkDebug ) log << "route "<<msg<< " to " << qname<<info;
			Q->push_back_ctrl(msg);
		}
	}
	recyclable::recycle((recyclable **)&m); // all received messages are recycled
}

//
// receive an image : start of image
// we known the message is /queuename/@@@/stream/image
//
void receiveImageStart(const osc::ReceivedMessage &m)
{

	// send current image, if not already sent
	if( i0!=NULL ) {
		if( Q==NULL ) {
			log << "unable to route image to queue " << err;
			recycle(i0);
		}else{
			Q->push_back(i0);
		}
		n0=-1;
	}
	//log << "received "<<m<< warn;

	const char *address=m.AddressPattern();
	int nb=oscutils::extractNbArgs(address);
	if( nb>1 ) {
		int pos,len;
		oscutils::extractArg(address,nb-1,pos,len); // premier arg
		string qname(address+pos,len);
		Q=qpmanager::getQueue(qname); // ou getQ? sans lock
	}else{
		Q=NULL; // pas de queue specifiee. on va perdre l'image
	}
		
	int32 num,w,h,sz,type;
	const char *view;
	m.ArgumentStream() >> n0 >> num >> w >> h >> sz >> type >> view >> dsz >> osc::EndMessage;
        if( qpmanager::networkDebug ) log << "got image "<< n0 << " " << num << " " << w << " " << h << " " << sz << " " << type << " " << view << " " << dsz << info;

	// get a recyclable image... maybe we should auto-allocate... (/realtime)
	recyclable *p=Qimages.pop_front_wait();
	i0=dynamic_cast<blob*>(p);
	if( i0==NULL ) { recyclable::recycle(&p); n0=-1; return;  } // pas normal... skip image
	i0->create(cv::Size(w,h),type);
	i0->view=string(view);
	//log << "starting img "<<w<<" x "<<h<<" dsz="<<dsz<<warn;
}

//
// receive an image : data of image
// we known the message is /queuename/@@@/stream/data
//
void receiveImageData(const osc::ReceivedMessage &m)
{
	osc::Blob bobo;
	int32 n0r,b;
	m.ArgumentStream() >> n0r >> b >> bobo >> osc::EndMessage;
	if( n0r!=n0 ) {
		log << "data packet "<<n0r<<" does not match "<<n0<<err;
		return;
	}
	memcpy(i0->data+b,bobo.data,bobo.size);
	if( qpmanager::networkDebug ) log << "got data "<<b<<" size "<<bobo.size<<" =?= "<<dsz<<info;
	if( b+bobo.size==dsz ) {
		// this was the last bloc. send the image
		if( Q==NULL ) {
			log << "unable to route image to queue "<< err;
			recycle(i0);
		}else{
			Q->push_back(i0);
		}
		n0=-1;
	}
}



};




///
/// Call this first, and from the main thread!
///
/// \param startLog Automatic start of \ref pluginSyslog, whith a queue name "log". This makes the log function works directly for plugins.
///
void qpmanager::initialize(bool startLog) {
	// acces aux donnees
	acces=new pthread_mutex_t;
	pthread_mutex_init(acces,NULL);


	qpmanager::initialized=true;

	// les noms des plugins disponibles
#ifdef HAVE_PLUGIN_V4L2
	mapPNames["pluginV4L2"] = &createInstance<pluginV4L2>;
#endif
#ifdef HAVE_PLUGIN_WEBCAM
	mapPNames["pluginWebcam"] = &createInstance<pluginWebcam>;
#endif
#ifdef HAVE_PLUGIN_NET
	mapPNames["pluginNet"] = &createInstance<pluginNet>;
#endif
#ifdef HAVE_PLUGIN_STREAM
	mapPNames["pluginStream"] = &createInstance<pluginStream>;
#endif
#ifdef HAVE_PLUGIN_VIEWER_GLFW
	mapPNames["pluginViewerGLFW"] = &createInstance<pluginViewerGLFW>;
#endif
#ifdef HAVE_PLUGIN_VIEWER_GLES2
	mapPNames["pluginViewerGLES2"] = &createInstance<pluginViewerGLES2>;
#endif
#ifdef HAVE_PLUGIN_RASPICAM
	mapPNames["pluginRaspicam"] = &createInstance<pluginRaspicam>;
#endif
#ifdef HAVE_PLUGIN_HOMOG
	mapPNames["pluginHomog"] = &createInstance<pluginHomog>;
#endif
#ifdef HAVE_PLUGIN_SYSLOG
	mapPNames["pluginSyslog"] = &createInstance<pluginSyslog>;
#endif
#ifdef HAVE_PLUGIN_PATTERN
	mapPNames["pluginPattern"] = &createInstance<pluginPattern>;
#endif
#ifdef HAVE_PLUGIN_DRIP
	mapPNames["pluginDrip"] = &createInstance<pluginDrip>;
#endif
#ifdef HAVE_PLUGIN_DRIPMULTI
	mapPNames["pluginDripMulti"] = &createInstance<pluginDripMulti>;
#endif
#ifdef HAVE_PLUGIN_SAVE
	mapPNames["pluginSave"] = &createInstance<pluginSave>;
#endif
#ifdef HAVE_PLUGIN_READ
	mapPNames["pluginRead"] = &createInstance<pluginRead>;
#endif
#ifdef HAVE_PLUGIN_FFMPEG
	mapPNames["pluginFfmpeg"] = &createInstance<pluginFfmpeg>;
#endif
#ifdef HAVE_PLUGIN_PROSILICA
	mapPNames["pluginProsilica"] = &createInstance<pluginProsilica>;
#endif
#ifdef HAVE_PLUGIN_PTGREY
	mapPNames["pluginPtGrey"] = &createInstance<pluginPtGrey>;
#endif
#ifdef HAVE_PLUGIN_LEOPARD
	mapPNames["pluginLeopard"] = &createInstance<pluginLeopard>;
#endif

	// auto-create the syslog plugin
	if( startLog ) {
		//
		// we predefine the log queue for logging output.
		// in a plugin, just use log >>
		//
		addQueue("log");
		addPlugin("Syslog","pluginSyslog");
		connect("log","Syslog","in");
		start("Syslog");
	}
}


void qpmanager::registerPluginType(std::string name,plugin<blob>*(*creator)()) {
	mapPNames[name]=creator;
}


void qpmanager::dump() {
	if( !initialized ) return;
	// attenion... ne pas abuser parce que c'est long....
	pthread_mutex_lock(acces);

	//
	// dump les queues
	//

	cout<<"[QP-dump] nbqueue="<<qmap.size()<<endl;
	map<std::string,imqueue*>::const_iterator itr;
	for(itr = qmap.begin(); itr != qmap.end(); itr++) {
		cout << "[QP-dump] queue name=" << itr->first << " val:" << itr->second->name<< " sz="<< itr->second->size()<<endl;
	}

	//
	// dump les plugins
	//

	cout<<"[QP-dump] nbplugin="<<pmap.size()<<endl;
	map<std::string,plugin<blob>*>::const_iterator itp;
	for(itp = pmap.begin(); itp != pmap.end(); itp++) {
		cout << "[QP-dump] plugin name=" << itp->first << " status=:" << itp->second->status()<<endl;
	}

	// dump les noms de plugins

	cout<<"[QP-dump] plugins disponibles:"<<mapPNames.size()<<endl;
	map_type::const_iterator in;
	for(in = mapPNames.begin(); in != mapPNames.end(); in++) {
		cout << "[QP-dump] plugin '"<<in->first<<"'"<<endl;
	}
	
	pthread_mutex_unlock(acces);
}


void qpmanager::dumpDot(std::string fileName) {
	if( !initialized ) return;
	// attenion... ne pas abuser parce que c'est long....
	pthread_mutex_lock(acces);

	ofstream file;
	file.open(fileName.c_str());
	file << "digraph G {\n";
	file << "  graph [rankdir=\"LR\"];\n";
	file << "  edge [style=\"bold\"];\n";

	//
	// dump les plugins
	//

	map<std::string,plugin<blob>*>::const_iterator itp;
	for(itp = pmap.begin(); itp != pmap.end(); itp++) {
		itp->second->dumpDot(file);
	}

	//
	// dump les queues
	//

	/*
	map<std::string,imqueue*>::const_iterator itr;
	for(itr = qmap.begin(); itr != qmap.end(); itr++) {
		cout << "[QP-dump] queue name=" << itr->first << " val:" << itr->second->name<< " sz="<< itr->second->size()<<endl;
	}
	*/

	file << "}\n";
	file.close();
	
	pthread_mutex_unlock(acces);
}



// retourne une queue en particulier (NULL si pas de queue)
// Attention::: cette fonction n'est pas protegee par un lock! acces interne seulement!
imqueue* qpmanager::getQ(std::string name) {
	if( !initialized ) return NULL;
	map<std::string,imqueue*>::const_iterator itr;
	itr=qmap.find(name);
	if( itr==qmap.end() ) {
		cout << "[QP] getQ did not find queue '"<<name<<"'"<<endl;
		return NULL;
	}
	return itr->second;
}

///
/// \param name Queue name (see \ref addQueue)
/// \return The actual \ref imqueue instance (NULL if queue is not found)
///
imqueue* qpmanager::getQueue(std::string name) {
	if( !initialized ) return NULL;
	pthread_mutex_lock(acces);
	imqueue *q=getQ(name);
	pthread_mutex_unlock(acces);
	return(q);
}

// retourne un plugin en particulier (NULL si pas de queue)
// Attention::: cette fonction n'est pas protegee par un lock! acces interne seulement!
plugin<blob>* qpmanager::getP(std::string name) {
	if( !initialized ) return NULL;
	map<std::string,plugin<blob>*>::const_iterator itr;
	itr=pmap.find(name);
	if( itr==pmap.end() ) {
		//cout << "[QP] getP did not find plugin '"<<name<<"'"<<endl;
		return NULL;
	}
	return itr->second;
}

///
/// \param name Plugin name (see \ref addPlugin)
/// \return The actual \ref plugin<blob> instance (NULL if plugin not found)
///
plugin<blob>* qpmanager::getPlugin(std::string name) {
	if( !initialized ) return NULL;
	pthread_mutex_lock(acces);
	plugin<blob> *p=getP(name);
	pthread_mutex_unlock(acces);
	if( p==NULL ) {
		cout << "[QP] getPlugin unable to find plugin '"<<name<<"'"<<endl;
	}
	return(p);
}

///
/// \return the name of the Queue, which can then be used in \ref addQueue
///
std::string qpmanager::nextAnonymousQueue() {
	char buf[10];
	sprintf(buf,"Q%d",queueNum);
	queueNum++;
	return std::string(buf);
}

///
/// \return the name of the Plugin, which can then be used in \ref addPlugin
///
std::string qpmanager::nextAnonymousPlugin() {
	char buf[10];
	sprintf(buf,"P%d",pluginNum);
	pluginNum++;
	return std::string(buf);
}

///
/// \param name Name of the new queue
/// \return 0 if ok, <0 if error
///
int qpmanager::addQueue(std::string name) {
	if( !initialized ) return(-2);
	pthread_mutex_lock(acces);
	// check si queue existe
	if( qmap.find(name) != qmap.end() ) {
		pthread_mutex_unlock(acces);
		cout << "[QP] queue "<<name<<" already created."<<endl;
		return(-1);
	}
	qmap[name]=new imqueue(name);
	pthread_mutex_unlock(acces);
	cout << "[QP] added queue " <<name<<endl;
	return(0);
}


///
/// \param ptype A string specifying a type of plugin. The types are the class name ("pluginStream", "pluginViewerGLFW", ...)
///
/// \return true if the specific plugin type is avaialbe
///
bool qpmanager::pluginAvailable(std::string ptype) {
	if( !initialized ) return(false);
	bool res;
	pthread_mutex_lock(acces);
	res=( mapPNames.find(ptype) != mapPNames.end() );
	pthread_mutex_unlock(acces);
	return res;
}

///
/// The plugin is not initialized or started yet. You must call start(pname) to start the plugin, and eventually stop(pname) to stop it.
///
/// The port "log" of the plugin is automatically connected to queue "log", if it exists.
///
/// \param name Name of the plugin. This name will refer to this specific instance (see \ref connect, \ref start, \ref stop, \ref status, \ref init, \ref loop, \ref uninit, and \ref reservePort)
/// \param ptype A string specifying a type of plugin. The types are the class name ("pluginStream", "pluginViewerGLFW", ...)
///
/// \return 0 if ok, <0 if error
///
int qpmanager::addPlugin(std::string name,std::string ptype) {
	if( !initialized ) return(-3);
	pthread_mutex_lock(acces);
	// check si ce nom existe deja
	if( pmap.find(name) != pmap.end() ) {
		pthread_mutex_unlock(acces);
		cout << "[QP] plugin "<<name<<" already created."<<endl;
		return(-1);
	}
	// check si le ptype existe
	if( mapPNames.find(ptype) == mapPNames.end() ) {
		pthread_mutex_unlock(acces);
		cout << "******** [QP] plugin type "<<ptype<<" inconnu."<<endl;
		return(-2);
	}
	pmap[name]=mapPNames[ptype]();
	// set plugin name
	pmap[name]->name=name;
	pmap[name]->typeName=ptype; // set type name, useful only for debuging
	pthread_mutex_unlock(acces);
	cout << "[QP] added plugin "<<name<<" du type "<<ptype<<endl;

	// auto-connect to log port of the plugin to log queue
	// if we are not using logs, then no harm done.
	connect("log",name,"log");
	return(0);
}

///
/// \param[in] qname Queue name (see \ref addQueue)
/// \param[in] m Message to add to the queue
///
void qpmanager::add(std::string qname,message *&m) {
	if( !initialized ) return;
	// on doit faire un lock parce qu'on acces au map et qu'il peut changer
	// si on ajoute des elements... :-(
	// on pourrait faire une cache, par contre....
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) {
		cout << "QPmanager unable to add message "<< m << " to queue "<<qname<<endl;
		recyclable::recycle((recyclable **)&m);return;
	}
	q->push_back_ctrl(m);
}

///
/// \param[in] qname Queue name (see \ref addQueue)
/// \param[in] i Blob image to add to the queue
///
void qpmanager::addImage(std::string qname,blob *&i) {
	if( !initialized ) return;
	// on doit faire un lock parce qu'on acces au map et qu'il peut changer
	// si on ajoute des elements... :-(
	// on pourrait faire une cache, par contre....
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) {
		cout << "QPmanager unable to add image to queue "<<qname<<endl;
		recyclable::recycle((recyclable **)&i);return;
	}
	q->push_back(i);
}


///
/// \param[in] qname Queue name (see \ref addQueue)
/// \param[in] m Message to add to the queue
///
void qpmanager::addWithImages(std::string qname,message *&m) {
	if( !initialized ) return;
	// on doit faire un lock parce qu'on acces au map et qu'il peut changer
	// si on ajoute des elements... :-(
	// on pourrait faire une cache, par contre....
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) {
		cout << "QPmanager unable to add message "<< m << " to queue "<<qname<<endl;
		recyclable::recycle((recyclable **)&m);return;
	}
	q->push_back(m);
}


///
/// \param[in] qname Queue name (see \ref addQueue)
/// \param[in] im Image to add to the queue
///
void qpmanager::add(std::string qname,blob *&im) {
	if( !initialized ) return;
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) {
		cout << "QPmanager unable to add image to queue "<<qname<<endl;
		recyclable::recycle((recyclable **)&im);return;
	}
	q->push_back(im);
}

/// Each added image will be empty, with a recycling Queue set to this queue.
///
/// \param[in] qname Queue name (see \ref addQueue)
/// \param[in] nb number of images to add to the queue
///
void qpmanager::addEmptyImages(std::string qname,int nb)
{
	if( !initialized ) return;
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) {
		cout << "QPmanager unable to add empty images to queue "<<qname<<endl;
		return;
	}
    for(int i=0;i<nb;i++) {
        blob *image=new blob();
        image->n=i;
        image->setRecycleQueue(q);
        q->push_back(image);
    }
}

///
/// \warn When a queue is empty, it does not mean it will stay empty if threaded plugins are connected to the queue. Only use this if you know that no plugin actually adds images to the queue.
///
/// \param[in] qname Queue name (see \ref addQueue)
/// \return true if the queue is empty, on the image side
///
bool qpmanager::isEmpty(std::string qname)
{
	if( !initialized ) return true;
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) return true;
	return( q->empty() );
}

///
/// \warn When a queue is empty, it does not mean it will stay empty if threaded plugins are connected to the queue. Only use this if you know that no plugin actually adds messages to the queue.
///
/// \param[in] qname Queue name (see \ref addQueue)
/// \return true if the queue is empty, on the control side
///
bool qpmanager::isEmptyCtrl(std::string qname)
{
	if( !initialized ) return true;
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) return true;
	return( q->empty_ctrl() );
}

///
/// \warn The queue size can change anytime, so this number is not always reliable.
///
/// \param[in] qname Queue name (see \ref addQueue)
/// \return queue size, on the image side
///
int qpmanager::queueSize(std::string qname)
{
	if( !initialized ) return 0;
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) return 0;
	return( q->size() ); // size_ctrl
}

///
/// \warn The queue size can change anytime, so this number is not always reliable.
///
/// \param[in] qname Queue name (see \ref addQueue)
/// \return queue size, on the image side
///
int qpmanager::queueSizeCtrl(std::string qname)
{
	if( !initialized ) return 0;
	pthread_mutex_lock(acces);
	imqueue *q=getQ(qname);
	pthread_mutex_unlock(acces);
	if( q==NULL ) return 0;
	return( q->size_ctrl() ); // size_ctrl
}

///
/// \warn Any non message in the queue (such as an image) will be recycled
///
/// \param[in] qname Queue name (see \ref addQueue)
/// \return the first ctrl message in the queue, or NULL if queue is empty
///
message *qpmanager::pop_front_ctrl_nowait(std::string qname)
{
	imqueue *q=getQ(qname);
	if( q==NULL ) return(NULL);
	for(;;) {
		recyclable *p=q->pop_front_ctrl(); // control only!
		if( p==NULL ) return(NULL); // queue empty!
		message *m=dynamic_cast<message*>(p);
		if( m!=NULL ) return(m);
		recyclable::recycle(&p); // on recycle les non-messages!
	}
}

message *qpmanager::pop_front_ctrl_wait(std::string qname)
{
	imqueue *q=getQ(qname);
	if( q==NULL ) return(NULL);
	for(;;) {
		recyclable *p=q->pop_front_ctrl_wait();
		message *m=dynamic_cast<message*>(p);
		if( m!=NULL ) return(m);
		recyclable::recycle(&p); // on recycle les non-messages!
	}
}

void qpmanager::pop_front_nowait(std::string qname,message *&m,blob *&i)
{
	m=NULL;
	i=NULL;
	imqueue *q=getQ(qname);
	if( q==NULL ) return;
	recyclable *p=q->pop_front(); // control or normal!
	if( p==NULL ) return; // queue empty!
	m=dynamic_cast<message*>(p);
	i=dynamic_cast<blob*>(p);
}

void qpmanager::pop_front_wait(std::string qname,message *&m,blob *&i)
{
	m=NULL;
	i=NULL;
	imqueue *q=getQ(qname);
	if( q==NULL ) return;
	recyclable *p=q->pop_front_wait(); // control or normal!
	m=dynamic_cast<message*>(p);
	i=dynamic_cast<blob*>(p);
}



///
/// If the name of the queue is "", then it creates an anonymous queue (not useful!)
///
/// \param[in] qname Queue name (see \ref addQueue)
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \param[in] port Port name (usually "in", "out", "log", ...) (see also reservePort)
/// \return 0 if ok, <0 if error
///
/// \todo shoud allow connect("qname","ppname.port");
///
int qpmanager::connect(std::string qname,plugin<blob>* p,std::string portName) {
	if( !initialized ) return(-3);

	if( qname.empty() ) {
		qname=nextAnonymousQueue();
		cout << "anonymous queue name is "<<qname<<endl;
		addQueue(qname);
	}
	pthread_mutex_lock(acces);
		
	imqueue *q=getQ(qname);
	if( q==NULL ) {
		pthread_mutex_unlock(acces);
		cout << "[QP] connect : unable to find queue "<<qname<<endl;
		return(-2);
	}
	// recupere le port
	int port=p->portMap(portName);
	if( port<0 ) {
		pthread_mutex_unlock(acces);
		cout << "[QP] connect : port name '"<<portName<<"' unknown to plugin "<<p->name<<endl;
		return(-1);
	}
	p->setQ(portName,q);
	// alias
#ifdef USE_ALIAS
	string alias=pname+"."+portName;
	cout << "[QP] adding alias "<<alias<<" for queue "<<qname<<endl;
	qmap[alias]=q; // auto replace the last definition
#endif
	pthread_mutex_unlock(acces);
	return(0);
}

int qpmanager::connect(std::string qname,std::string pname,std::string portName) {
	plugin<blob>* p=getP(pname);
	if( p==NULL ) {
		pthread_mutex_unlock(acces);
		cout << "[QP] connect : unable to find plugin "<<pname<<endl;
		return(-1);
	}
	return( connect(qname,p,portName) );
}


///
/// Connect all ports of all plugins to a specific queue.
/// If qname is empty, an anonymus queue is created (not useful)
///
/// \param vector<singleport> List of the plugins/ports to connect to the Queue
/// \param qname Queue name (see \ref addQueue). No connection should have already been etablish with the Queue.
///
void qpmanager::connect(std::vector<singleport> &ports,std::string qname) {
	int i;
	bool err=false;
	pthread_mutex_lock(acces);
	// find plugins
	for(i=0;i<ports.size();i++) {
		ports[i].plu=getP(ports[i].pname);
		if( ports[i].plu==NULL ) {
			cout << "connect: unable to find plugin "<<ports[i].pname<<endl;
			err=true;
		}
		int pnum=ports[i].plu->portMap(ports[i].portName);
		if( pnum<0 ) {
			cout << "connect: unable to find port "<<ports[i].portName<<" for plugin "<<ports[i].pname<<endl;
			err=true;
		}
	}
	if( err ) { pthread_mutex_unlock(acces); return; }
	if( qname.empty() ) {
		// aucune queue... on cree une queue anonyme
		qname=nextAnonymousQueue();
		cout << "connect: anonymous queue name is "<<qname<<endl;
		pthread_mutex_unlock(acces);
		addQueue(qname);
		pthread_mutex_lock(acces);
	}
	// set all ports to this queue
	imqueue *q=getQ(qname);
	cout << "YO"<<endl;
	for(i=0;i<ports.size();i++) {
		cout << "connect: set queue for "<<ports[i].pname<<"."<<ports[i].portName<<endl;
		ports[i].plu->setQ(ports[i].portName,q);
		// define alias
#ifdef USE_ALIAS
		string alias=ports[i].pname+"."+ports[i].portName;
		cout << "connect: adding alias "<<alias<<" for queue "<<qname<<endl;
		qmap[alias]=q; // auto replace the last definition
#endif
	}
	pthread_mutex_unlock(acces);
	// dump
	for(i=0;i<ports.size();i++) {
		cout << "connect "<<i<<": P="<<ports[i].pname<<" portName="<<ports[i].portName<<endl;
	}
}


///
/// Reserve a port in a Plugin.
///
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \param[in] portName name of the new port. Typical names to avoid are "in", "out", and "log".
/// \return 0 if ok, -1 if portname already exists or bad plugin
///
int qpmanager::reservePort(std::string pname,std::string portName) {
	pthread_mutex_lock(acces);
	plugin<blob>* p=getP(pname);
	if( p==NULL ) {
		pthread_mutex_unlock(acces);
		cout << "[QP] reservePort : unable to find plugin "<<pname<<endl;
		return(-1);
	}
	int k=p->reservePort(portName,true);
	pthread_mutex_unlock(acces);
	return((k<0)?-1:0);
}

///
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \return the name of the new create port
///
std::string qpmanager::reservePort(std::string pname) {
	pthread_mutex_lock(acces);
	plugin<blob>* p=getP(pname);
	if( p==NULL ) {
		pthread_mutex_unlock(acces);
		cout << "[QP] reservePort : unable to find plugin "<<pname<<endl;
		return("");
	}
	string k=p->reservePort(true);
	pthread_mutex_unlock(acces);
	return(k);
}


//
// controle de plugin
//

// start/status/stop -> version "avec thread"

///
/// \ref start, \ref stop, and \ref status are the main way to use Plugins with threads.
/// In practive, \ref start will create thread, and from that thread \ref init will be called, then \ref loop (multiple times), then \ref uninit.
///
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \return true if all went well.
///
bool qpmanager::start(std::string pname) {
	plugin<blob>* p=getPlugin(pname); // protected!
	if( p==NULL ) { cout << "[QP] start() unable to find plugin "<<pname<<endl; return(false); }
	return(p->start());
}

///
/// \ref start, \ref stop, and \ref status are the main way to use Plugins with threads.
///
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \return true if all went well.
///
bool qpmanager::stop(std::string pname) {
	plugin<blob>* p=getPlugin(pname);
	if( p==NULL ) { cout << "[QP] stop() unable to find plugin "<<pname<<endl; return(false); }
	p->stop();
	return true;
}

///
/// \ref start, \ref stop, and \ref status are the main way to use Plugins with threads.
///
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \return true if the plugin thread is running, false if no thread is active.
///
bool qpmanager::status(std::string pname) {
	plugin<blob>* p=getPlugin(pname);
	if( p==NULL ) { cout << "[QP] status() unable to find plugin "<<pname<<endl; return(false); }
	return(p->status());
}

///
/// \ref init, \ref loop, and \ref uninit are the main way to use Plugins without threads.
/// When running manually, simply call \ref init first, then \ref loop many times (or until it returns false), and then \ref uninit.
///
/// Call \ref init, \ref loop, and \ref uninit only when running a plugin without thread.
///
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \return true if all went well
///
bool qpmanager::init(std::string pname) {
	plugin<blob>* p=getPlugin(pname);
	if( p==NULL ) { cout << "[QP] init() unable to find plugin "<<pname<<endl; return(false); }
	p->init();
	return true;
}


///
/// You must call \ref init before using \ref loop.
/// Typically, loop will get one image from an input Queue, process it, and put the result in an output Queue. 
///
/// Call \ref init, \ref loop, and \ref uninit only when running a plugin without thread.
///
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \return true if all is ok, false si stopping required. When running a Plugin inside a thread, \ref loop will be called forever as long as it returns true. When it returns false, then \ref uninit is called and the thread is killed.
///
bool qpmanager::loop(std::string pname) {
	plugin<blob>* p=getPlugin(pname);
	if( p==NULL ) { cout << "[QP] loop() unable to find plugin "<<pname<<endl; return(false); }
	return(p->loop());
}

///
/// Call \ref init, \ref loop, and \ref uninit only when running a plugin without thread.
///
/// \param[in] pname Plugin name (see \ref addPlugin)
/// \return true if all went well
///
bool qpmanager::uninit(std::string pname) {
	plugin<blob>* p=getPlugin(pname);
	if( p==NULL ) { cout << "[QP] uninit() unable to find plugin "<<pname<<endl; return(false); }
	p->uninit();
	return true;
}


///
/// The incoming UDP network packets will be interpreted as OSC commands.
/// It is assumed that the root of the OSC command is the name of the queue where
/// the OSC command should be added.
/// As an example, the command "/display/test" would be added to queue "display".
///
/// For now, either \ref startNetListeningUdp or \ref startNetListeningTcp can be called, but
/// not both at the same time.
///
/// \param port port number to listen to
///
/// \todo allow UDP and TCP at the same time
///
void qpmanager::startNetListeningUdp(int port) {
	stopNetListening();
	//net=new pluginReseau(port,false);
	net=new pluginNet2(port,false,""); // UDP
	net->name="qpmanager::net";
	// connect manually to the "log" queue
	pthread_mutex_lock(acces);
	net->setQ("log",getQ("log"));
	pthread_mutex_unlock(acces);

	net->start();
	//cout << "qpmanager listening to port "<<port<< " UDP"<<endl;
}

void qpmanager::startNetListeningMulticast(int port,string ip) {
	stopNetListening();
	//net=new pluginReseau(port,false);
	net=new pluginNet2(port,false,ip); // MULTICAST
	net->name="qpmanager::net";
	// connect manually to the "log" queue
	pthread_mutex_lock(acces);
	net->setQ("log",getQ("log"));
	pthread_mutex_unlock(acces);

	net->start();
	//cout << "qpmanager listening to port "<<port<< " UDP"<<endl;
}

///
/// The incoming TCL network packets will be interpreted as OSC commands.
/// It is assumed that the root of the OSC command is the name of the queue where
/// the OSC command should be added.
/// As an example, the command "/display/test" would be added to queue "display".
///
/// Only a single connexion is processed at a time, which is not ideal...
/// Any packet sent using TCP must be preceeded by its size, in a 4 byte int. Multiple commands can be sent during one connexion.
///
/// For now, either \ref startNetListeningUdp or \ref startNetListeningTcp can be called, but
/// not both at the same time.
///
/// \param port port number to listen to
///
/// \todo allow multiple tcp connexions
///
void qpmanager::startNetListeningTcp(int port) {
	stopNetListening();
	net=new pluginNet2(port,true,""); // TCP
	net->name="qpmanager::net";
	// connect manually to the "log" queue
	pthread_mutex_lock(acces);
	net->setQ("log",getQ("log"));
	pthread_mutex_unlock(acces);

	net->start();
	//cout << "qpmanager listening to port "<<port<< " TCP"<<endl;
}

///
/// Either UDP or TCP (whichever is active) will be terminated
///
void qpmanager::stopNetListening() {
	if( qpmanager::net!=NULL ) {
		//cout <<"qpmanager: stoping netlistening"<<endl;
		net->stop();
		delete net;
		net=NULL;
	}
}

//
// activate network verbose debuging
//
void qpmanager::setNetworkDebug(bool val) {
	qpmanager::networkDebug=val;
}


//
// qpmanager static stuff
//

bool qpmanager::initialized=false;
map<std::string,imqueue*> qpmanager::qmap;
map<std::string,plugin<blob>*> qpmanager::pmap;

pthread_mutex_t *qpmanager::acces=NULL;  

pluginNet2 *qpmanager::net=NULL;

bool qpmanager::networkDebug=false;

//
// Pour creer le bon plugin, en utilisant seulement un nom de plugin
//

map_type qpmanager::mapPNames;

int qpmanager::queueNum=100;
int qpmanager::pluginNum=100;





