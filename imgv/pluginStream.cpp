
#include <imgv/pluginStream.hpp>

#undef VERBOSE
//#define VERBOSE

using namespace std;

#define Q_IN	0
#define Q_OUT	1
#define Q_CMD	2
#define Q_LOG	3

pluginStream::pluginStream() {
        // ports
        ports["in"]=Q_IN; // -> control for this plugin, and image to send out
				// in msg mode: any message here will be sent out too.
        ports["out"]=Q_OUT;
        ports["cmd"]=Q_CMD; // in mode OUT_MSG and OUT_MSG_TCP, put your messages here
        ports["log"]=Q_LOG;

		portsIN["in"]=true;
		portsIN["cmd"]=true;
}

pluginStream::~pluginStream() { log << "delete" <<warn;}

void pluginStream::init() {
	//log << "init! "<<pthread_self()<<warn;
	mode=MODE_NONE;
	n=0;
	initializing=true;
	transmitSocket=NULL;
	receiveSocket=NULL;
	realtime=true; // skip images if net too slow or images arrive to fast
	n0=-1;
	scale=-1.0; // no reduction
	mtu=1500;
}

void pluginStream::uninit() {
	log << "uninit"<<warn;
	if( transmitSocket!=NULL ) delete transmitSocket;
	if( receiveSocket!=NULL ) delete receiveSocket;
}

int pluginStream::checksum(unsigned char *data,int size) {
	unsigned char m=0;
	for(int i=0;i<size;i++) m=m^data[i];
	return(m);
}

bool pluginStream::loop() {
	if( initializing || (mode==MODE_OUT && transmitSocket==NULL) ) {
		// attend+process messages
		pop_front_ctrl_wait(Q_IN);
		return true;
	}

	if( mode==MODE_OUT_MSG || mode==MODE_OUT_MSG_TCP ) {
		//log << "wait no handler..."<<warn;
		// check regular message, in case
		pop_front_ctrl_nowait(Q_IN);
		// get all commands, do not look for images
		message *m=pop_front_ctrl_wait_nohandler(Q_CMD);
		if( m==NULL ) return true;
		//
		// special case... OUT_MSG or OUT_MSG_TCP
		// -> send directly any message to the outside
		//
		if( mode==MODE_OUT_MSG ) {
			if( m->size>mtu ) {
				log << "message too large! " << m <<err;
				recyclable::recycle((recyclable **)&m);
				return true;
			}
			transmitSocket->Send(m->data,m->size);
			if( delay ) usleepSafe(delay);
		}else if( mode==MODE_OUT_MSG_TCP ) {
			//log << "********** tcp out "<<m->ops->Size()<<info;
			int len=m->size;
			tcp_send_data(&tcp,(const char *)&len,4);
			tcp_send_data(&tcp,m->data,m->size);
		}
		recyclable::recycle((recyclable **)&m);
		return true;
	}

	// we are sending images...

	// get next image (and process messages if any)
	blob *i1,*i2;
	// get next image to process
	i1=pop_front_wait(Q_IN);

	if( realtime ) {
		for(;;) {
			i2=pop_front_nowait(Q_IN); // second image, if there is one...
			if( i2==NULL ) break; // no more images.
			//log << "skiping image " << i1->n << info;
			//for(int i=0;i<i1->cols*i1->rows*i1->elemSize();i++) i1->data[i]=128;
			push_back(Q_OUT,&i1); // send this image out
			i1=i2;
		}
	}

	//log << "image "<< i1->n <<info;

	// send message for now...

	//unsigned char buf[100];
	//for(int i=0;i<10;i++) buf[i]=i;

	/// lire: m.create(rows, cols, elemType);

	//log<<"size="<<dsz<<" nbchunk="<<(dsz+chunk-1)/chunk<< " last="<<dsz%chunk<<warn;
	message *m=getMessage(mtu);

	cv::Mat ix;

	if( scale>0.0 ) cv::resize(*i1,ix, cv::Size(), scale,scale);
	else ix=*i1; // no data copy...

	int sz=ix.elemSize();
	int ty=ix.type();
	dsz=ix.cols * ix.rows * sz;
	int chunk=mtu-160;

	// send general info about a new image
	*m<<osc::BeginMessage("/stream/image")
			<< n
			<< i1->n
			<< ix.cols
			<< ix.rows
			<< sz
			<< ty
			<< (this->view.empty()?i1->view:this->view)
			<< (int)(dsz)
			<< osc::EndMessage;
	if( mode==MODE_OUT ) {
		transmitSocket->Send(m->ops->Data(),m->ops->Size());
		if( delay ) usleepSafe(delay);
	}else if( mode==MODE_OUT_TCP ) {
		//log << "********** tcp out "<<m->ops->Size()<<info;
		int len=m->ops->Size();
		tcp_send_data(&tcp,(const char *)&len,4);
		tcp_send_data(&tcp,m->ops->Data(),m->ops->Size());
		//log << "********** checksum "<<checksum((unsigned char *)m->ops->Data(),m->ops->Size()) << info;
	}
	m->reset();

	int len;
	for(int b=0;b<dsz;b+=len) {
		len=chunk;
		if( b+len>dsz ) { len=dsz-b; }
		osc::Blob bobo(ix.data+b,len);

		*m<<osc::BeginMessage("/stream/data")<< n << b << bobo << osc::EndMessage;
		if( mode==MODE_OUT ) {
			transmitSocket->Send(m->ops->Data(),m->ops->Size());
			if( delay ) usleepSafe(delay);
		}else if( mode==MODE_OUT_TCP ) {
			int len=m->ops->Size();
			tcp_send_data(&tcp,(const char *)&len,4);
			tcp_send_data(&tcp,m->ops->Data(),m->ops->Size());
		    //log << "** checksum "<<checksum((unsigned char *)m->ops->Data(),m->ops->Size()) << info;
		}
		//log << "sending b="<<b<<" len="<<len<<" sz="<<m->ops->Size()<<err;
		m->reset(); // pour envoyer un autre message...
	}
	recyclable::recycle((recyclable **)&m); // reuse m
	//delete m;

	// laisse l'image pousuivre son chemin
	push_back(Q_OUT,&i1);

	n++; // global image counter

	return true;
}

	//
	// decodage de messages a l'initialisation (init=true)
	// ou pendant l'execution du plugin (init=false)
	// (dans messageDecoder)
	//
bool pluginStream::decode(const osc::ReceivedMessage &m) {
	// verification pour un /defevent
	if( plugin::decode(m) ) return true;
	// log << "[save] decoding "<<m<<info;

	const char *address=m.AddressPattern();

	// en tout temps...
	if( oscutils::endsWith(address,"/init-done") ) {
		initializing=false;
		log<<"init finished."<<warn;
	}else if( oscutils::endsWith(address,"/real-time") ) {
		m.ArgumentStream() >> realtime >> osc::EndMessage;
		log << "real-time set to "<<realtime<<warn;
	}else if( oscutils::endsWith(address,"/view") ) {
		const char *view;
		m.ArgumentStream() >> view >> osc::EndMessage;
		this->view=view;
	}else if( oscutils::endsWith(address,"/scale") ) {
		// use <0 to deactivate
		float v;
		m.ArgumentStream() >> v >> osc::EndMessage;
		scale=v;
		log << "scaling output image by a factor "<<scale<<warn;
	}else if( oscutils::endsWith(address,"/out") ) {
		if( !initializing ) {
			log<< "/out is an initialization command!"<<err;
		}else{
			const char *str;
                        int32 port;
			m.ArgumentStream() >> str >> port >> mtu >> delay >> osc::EndMessage;
			log << "out to "<<str<<" port "<<port<<warn;
			if( transmitSocket!=NULL ) delete transmitSocket;
			transmitSocket=new UdpTransmitSocket(IpEndpointName(str,port));
			mode=MODE_OUT;
		}
	}else if( oscutils::endsWith(address,"/outmsg") ) {
		if( !initializing ) {
			log<< "/outmsg is an initialization command!"<<err;
		}else{
			const char *str;
			int32 port;
			m.ArgumentStream() >> str >> port >> mtu >> delay >> osc::EndMessage;
			log << "outmsg to "<<str<<" port "<<port<<warn;
			if( transmitSocket!=NULL ) delete transmitSocket;
			transmitSocket=new UdpTransmitSocket(IpEndpointName(str,port));
			mode=MODE_OUT_MSG;
		}
	}else if( oscutils::endsWith(address,"/in") ) {
		if( !initializing ) {
			log<< "/in is an initialization command!"<<err;
		}else{
			int32 port;
			m.ArgumentStream() >> port >> osc::EndMessage;
			log << "in from "<<" port "<<port<<warn;
			if( receiveSocket!=NULL ) delete receiveSocket;
			receiveSocket=new UdpListeningReceiveSocket(IpEndpointName(IpEndpointName::ANY_ADDRESS, port ), this );
			log<<"Listening to "<<port<<warn;
			mode=MODE_IN;
			receiveSocket->Run();
		}
	}else if( oscutils::endsWith(address,"/outtcp") ) {
		if( !initializing ) {
			log<< "/outtcp is an initialization command!"<<err;
		}else{
			const char *str;
			int32 port;
			m.ArgumentStream() >> str >> port >> osc::EndMessage;
			log << "outtcp to "<<str<<" port "<<port<<warn;

			mode=MODE_OUT_TCP;
			int k=tcp_client_init(&tcp,str,port);
			if( k ) {
				log << "unable to open outgoing tcp to " << str << " port " <<port << err;
				mode=MODE_NONE;
			}
		}
	}else if( oscutils::endsWith(address,"/outmsgtcp") ) {
		if( !initializing ) {
			log<< "/outmsgtcp is an initialization command!"<<err;
		}else{
			const char *str;
			int32 port;
			m.ArgumentStream() >> str >> port >> osc::EndMessage;
			log << "outmsgtcp to "<<str<<" port "<<port<<warn;

			mode=MODE_OUT_MSG_TCP;
			int k=tcp_client_init(&tcp,str,port);
			if( k ) {
				log << "unable to open outgoing tcp to " << str << " port " <<port << err;
				mode=MODE_NONE;
			}
		}
	}else if( oscutils::endsWith(address,"/intcp") ) {
		if( !initializing ) {
			log<< "/intcp is an initialization command!"<<err;
		}else{
			int32 port;
			m.ArgumentStream() >> port >> osc::EndMessage;
			log << "intcp from "<<" port "<<port<<warn;

			mode=MODE_IN_TCP;
			int k=tcp_server_init(&tcp,port);
			char c;
			ssize_t x;
			if( k ) {
				log << "unable to listen for tcp connexion on port "<<port<<err;
			}else{
				int len;
				int nb;
				while(1) {
					log << "Waiting for a connexion..."<<warn;
				  tcp_server_wait(&tcp);
					for(;;) {
						// check for connexion
						if( recv(tcp.remote_sock,&c, 1, MSG_PEEK)==0 ) break;
						//log << "peek is " << x << info;
					  // get length of packet
					  int i=0;
					  const char *p=(const char *)&len;
					  while( i<4 ) {
						  if( recv(tcp.remote_sock,&c, 1, MSG_PEEK)==0 ) { len=-1;break; }
						  nb=tcp_receive_data(&tcp,p+i,4-i);
						  if( nb<0 ) { len=-1;break; }
						  i+=nb;
					  }
					  if( len<0 ) break; // lost connexion
					  //log << "got "<< nb<<" from tcp. len="<<len<<info;
					  if( len>sizeof(buf) ) {
							log << "packet too big. skip"<<err;
							for(i=0;i<len;i++) {
								int j=len-i;
								if( j>sizeof(buf) ) j=sizeof(buf);
						  		if( recv(tcp.remote_sock,&c, 1, MSG_PEEK)==0 ) { len=-1;break; }
								nb=tcp_receive_data(&tcp,(const char *)buf,j);
								if( nb<0 ) { len=-1;break; }
								i+=nb;
							}
							// should skip properly
							continue;
						}
					  for(i=0;i<len;) {
						  if( recv(tcp.remote_sock,&c, 1, MSG_PEEK)==0 ) { len=-1;break; }
						  nb=tcp_receive_data(&tcp,(char *)buf+i,len-i);
						  if( nb<0 ) { len=-1;break; }
						  i+=nb;
					  }
					  if( len<0 ) break;
					  //log << "checksum "<<checksum(buf,len) << info;
					  try {
					        ProcessPacket( (const char *)buf, len);
						} catch(...) {
							log << "EX!! len=" << len << err;
						}
					}
				  //tcp_send_string(&tcp,buf);
				  tcp_server_close_connection(&tcp);
				}
				// n'arrive jamais ici, normalement...
				tcp_server_close(&tcp);
			}
			mode=MODE_NONE;
		}
	}else{
		log << "commande inconnue: "<<m<<err;
		return false;
	}
	return true;
}

// de PacketListener
void pluginStream::ProcessPacket( const char *data, int size,
	const IpEndpointName& remoteEndpoint ) {
	ProcessPacket(data,size);
}


// de PacketListener
void pluginStream::ProcessPacket( const char *data, int size ) {
	//cout << "[Reseau] GOT message len="<<size<<endl;
	// we have a current image and everything else is lost.
	osc::ReceivedPacket p(data,size);
	if( p.IsBundle() ) {
		log << "unable to process bundled messages"<<err;
		return;
	}
	osc::ReceivedMessage m(p);

	const char *address=m.AddressPattern();
	if( oscutils::endsWith(address,"/stream/data") ) {
		osc::Blob bobo;
		int32 n0r,b;
		m.ArgumentStream() >> n0r >> b >> bobo >> osc::EndMessage;
		if( n0r!=n0 ) {
#ifdef VERBOSE
			log << "data packet "<<n0r<<" does not match "<<n0<<err;
#endif
			return;
		}
		memcpy(i0->data+b,bobo.data,bobo.size);
		//log << "got data "<<b<<" size "<<bobo.size<<" =?= "<<dsz<<info;
		if( b+bobo.size==dsz ) {
			// this was the last bloc. send the image
			push_back(Q_OUT,&i0);
			n0=-1;
		}
	}else if( oscutils::endsWith(address,"/stream/image") ) {
		// send current image, if not already sent
		if( i0!=NULL ) { push_back(Q_OUT,&i0);n0=-1; }
		//log << "received "<<m<< warn;
		
		int32 num,w,h,sz,type;
		const char *view;
		m.ArgumentStream() >> n0 >> num >> w >> h >> sz >> type >> view >> dsz >> osc::EndMessage;
        //log << "got image "<< n0 << " " << num << " " << w << " " << h << " " << sz << " " << type << " " << view << " " << dsz << info;

		// get a recyclable image... maybe we should auto-allocate... (/realtime)
		i0=pop_front_wait(Q_IN);
		i0->create(cv::Size(w,h),type);
		//dsz=w*h*i0->elemSize(); // target size
		i0->n=num;
		if( !this->view.empty() ) {
			// the view was set with /view, so replace it.
			i0->view=this->view;
		}else{
			i0->view=string(view);
		}

		//log << "starting img "<<w<<" x "<<h<<" dsz="<<dsz<<warn;
	}

/*
	int nb=oscutils::extractNbArgs(address);
	int pos,len;
	oscutils::extractArg(address,nb-1,pos,len); // premier arg
	cout << "add="<<address<<", premier arg is pos="<<pos<<", len="<<len<<endl;

	string qname(address+pos,len);
	imqueue *Q=(imqueue *)findQ(qname);
	if( Q==NULL ) {
		cout << "[reseau] unable to route message. No queue named "<<qname << endl;
		return;
	}
	// creer le message et l'envoyer
	message *msg=new message(size);
	msg->set(data,size);
	Q->push_front(msg);
*/
}








