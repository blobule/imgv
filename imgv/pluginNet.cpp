
#include <imgv/pluginNet.hpp>

#include <sys/time.h>


#undef VERBOSE
//#define VERBOSE

using namespace std;

#define Q_IN	0
#define Q_OUT	1
#define Q_CMD	2
#define Q_LOG	3


pluginNet::pluginNet() {
        // ports
        ports["in"]=Q_IN; // -> mode==SEND: Everything here (image+ctrl) is forwarded to the net.
        ports["out"]=Q_OUT; // -> mode==RECEIVE : Everything received is put here.
        ports["cmd"]=Q_CMD; // to control this plugin
        ports["log"]=Q_LOG;

	portsIN["in"]=true;
	portsIN["cmd"]=true;

	buf=NULL;
}

pluginNet::~pluginNet() { log << "delete" <<warn;}

void pluginNet::init() {
	log << "init! "<<warn;
	mode=MODE_NONE;
	type=TYPE_UDP;

	n=0;
	realtime=true; // skip images if net too slow or images arrive to fast
	n0=-1;

	scale=-1.0; // no reduction
	mtu=1500;
	delay=0;
	debug=false;

	if( buf!=NULL ) free(buf);
	buf=(unsigned char *)malloc(mtu+100);
}

void pluginNet::uninit() {
	log << "uninit"<<warn;
	if( buf!=NULL ) { free(buf);buf=NULL; }
}

int pluginNet::checksum(unsigned char *data,int size) {
	unsigned char m=0;
	for(int i=0;i<size;i++) m=m^data[i];
	return(m);
}

bool pluginNet::loop() {
	//log << "loop triage size " <<triage.size()<<info;
	if( mode==MODE_NONE ) {
		// check commands only
		pop_front_ctrl_wait(Q_CMD);
		return true;
	}
	// check for commands in case...
	pop_front_ctrl_nowait(Q_CMD);

	if( mode==MODE_OUT ) {
		// empty completely the IN queue to the network
		// Since we don't decode, we ask for raw message
		for(;;) {
			message *m;
			blob *image;
			pop_front_wait_nohandler(Q_IN,m,image);
			if( m==NULL && image==NULL ) break; // queue is empty
			if( m!=NULL ) {
				if( debug ) log << "got message size="<<m->size<<" , sending out" << info;
				if( type==TYPE_TCP ) {
					char preamble[4];
					oscutils::FromInt32(preamble,m->size);
					if( debug )printf("sending premble 0x%02x%02x%02x%02x\n",preamble[0],preamble[1],preamble[2],preamble[3]);
					tcp_send_data(&tcp,preamble,4);
					tcp_send_data(&tcp,(const char *)m->data,m->size);
				}else{
					// UDP or MULTICAST
					udp_send_data(&udp,(unsigned char *)m->data,m->size);
				}
				recyclable::recycle((recyclable **)&m);
			}
			if( image!=NULL ) {
				if( debug ) log << "got image n="<<image->n<<info;

				//checkOutgoingMessages("image-sending",now,-1,-1); // $x is image number

				sendImage(image);


				// we could send it to an output port instead...
				//recyclable::recycle((recyclable **)&image);
				push_back(Q_OUT,&image);
				// event! we just sent an image!
				struct timeval tv;
				gettimeofday(&tv,NULL);
				double now=tv.tv_sec+tv.tv_usec/1000000.0;
				checkOutgoingMessages("sent",now,-1,-1); // $x is image number
			}
		} 
		return true;
	}
	if( mode==MODE_IN ) {
		if( type==TYPE_TCP ) {
			int k;
			if( !tcp_server_is_connected(&tcp) ) {
				log << "waiting for tcp connexion"<<warn;
				tcp_server_wait(&tcp);
				if( !tcp_server_is_connected(&tcp) ) {
					log << "error getting tcp connexion"<<warn;
					tcp_server_close(&tcp);
					mode=MODE_NONE;
				}
				log << "got tcp connexion"<<warn;
			}
			int32 len;
			char preamble[4];
			// get packet size
			k=tcp_receive_data_exact(&tcp,preamble,4);
			if( debug ) printf("k=%d, got premble 0x%02x%02x%02x%02x\n",k,preamble[0],preamble[1],preamble[2],preamble[3]);
			len=oscutils::ToInt32(preamble);
			if( k==0 ) {
				log << "lost TCP connexion"<<err;
				tcp_server_close_connection(&tcp);
				tcp_server_close(&tcp);
				return true;
			}
			if( len>mtu ) {
				// skip this packet
				log << "TCP packet size "<<len<<" too big. Skipping."<<err;
				while( len>0 ) {
					int n=len;
					if( n>mtu ) n=mtu;
					k=tcp_receive_data_exact(&tcp,(const char *)buf,n);
					if( k==0 ) break; // lost connexion. done.
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
			int sz=udp_receive_data(&udp,buf,mtu);
			if( debug ) log << "got NET message size="<<sz<<info;
			processInMessage(buf,sz);
		}
	}
	return true;
}


bool pluginNet::decode(const osc::ReceivedMessage &m) {
	// verification pour un /defevent
	log << "decoding "<<m<<" triage size "<<triage.size()<<info;
	if( plugin::decode(m) ) return true;

	const char *address=m.AddressPattern();

	// en tout temps...
	if( oscutils::endsWith(address,"/real-time") ) {
		m.ArgumentStream() >> realtime >> osc::EndMessage;
		//log << "real-time set to "<<realtime<<warn;
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
	}else if( oscutils::endsWith(address,"/out/udp") ) {
		// check if already running... if so, close the socket.
		// ...
		mode=MODE_NONE;
		const char *ip;
		int32 port;
		int32 delay;
		m.ArgumentStream() >> ip >> port >> delay >> osc::EndMessage;
		this->delay=delay;
		log << "out UDP to "<< ip <<" port "<<port<<" delay "<<delay<<warn;
		int k=udp_init_sender(&udp,ip,port,UDP_TYPE_NORMAL); // _BROADCAST, _MULTICAST
		if( k<0 ) {
			log << "Unable to access network for "<<ip<<":"<<port<<err;
		}else{
			mode=MODE_OUT;
			type=TYPE_UDP;
		}
	}else if( oscutils::endsWith(address,"/out/multicast") ) {
		// check if already running... if so, close the socket.
		//...
		mode=MODE_NONE;
		const char *ip;
		int32 port;
		int32 delay;
		m.ArgumentStream() >> ip >> port >> delay >> osc::EndMessage;
		this->delay=delay;
		log << "out MULTICAST to "<< ip <<" port "<<port<<" delay "<<delay<<warn;
		int k=udp_init_sender(&udp,ip,port,UDP_TYPE_MULTICAST); // _BROADCAST, _MULTICAST
		if( k<0 ) {
			log << "Unable to access network for "<<ip<<":"<<port<<err;
		}else{
			mode=MODE_OUT;
			type=TYPE_MULTICAST;
		}
	}else if( oscutils::endsWith(address,"/in/udp") ) {
		// make sure mode is NONE (close everything)
		mode=MODE_NONE;
		int32 port;
		m.ArgumentStream() >> port >> osc::EndMessage;
		log << "in UDP from "<<" port "<<port<<warn;
		int k=udp_init_receiver(&udp,port,NULL);
		if( k<0 ) {
			log << "network: Unable to listen to port "<<port<<err;
		}else{
			log<<"Listening to "<<port<<warn;
			mode=MODE_IN;
			type=TYPE_UDP;
		}
	}else if( oscutils::endsWith(address,"/in/multicast") ) {
		// make sure mode is NONE (close everything)
		mode=MODE_NONE;
		const char *ip;
		int32 port;
		m.ArgumentStream() >> ip >> port >> osc::EndMessage;
		log << "in MULTICAST from "<< ip << " : " << port<<warn;
		int k=udp_init_receiver(&udp,port,(char *)ip);
		if( k<0 ) {
			log << "network: Unable to listen to ip "<<ip<<" port "<<port<<err;
		}else{
			log<<"Listening to "<<ip<<" : "<<port<<warn;
			mode=MODE_IN;
			type=TYPE_MULTICAST;
		}
	}else if( oscutils::endsWith(address,"/out/tcp") ) {
		// make sure mode is NONE (close everything)
		mode=MODE_OUT;
		type=TYPE_TCP;
		const char *ip;
		int32 port;
		m.ArgumentStream() >> ip >> port >> osc::EndMessage;
		log << "out TCP to "<< ip <<" port "<<port<<warn;
		int k=tcp_client_init(&tcp,ip,port);
		if( k ) {
			log << "unable to open outgoing tcp to " << ip << " port " <<port << err;
			mode=MODE_NONE;
		}else{
			log << "connected outgoing tcp to " << ip << " port " <<port << err;
		}
	}else if( oscutils::endsWith(address,"/in/tcp") ) {
		// make sure mode is NONE (close everything)
		mode=MODE_IN;
		type=TYPE_TCP;
		int32 port;
		m.ArgumentStream() >> port >> osc::EndMessage;
		log << "in TCP from "<<" port "<<port<<warn;
		int k=tcp_server_init(&tcp,port);
		if( k ) {
			log << "unable to listen for TCP connexion on port "<<port<<err;
			mode=MODE_NONE;
		}
	}else if( oscutils::endsWith(address,"/debug") ) {
		m.ArgumentStream() >> debug >> osc::EndMessage;
	}else{
		log << "commande inconnue: "<<m<<err;
		return false;
	}
	return true;
}


void pluginNet::processInMessage(unsigned char *buf,int sz) {
	message *m=getMessage(sz);
	m->set((const char *)buf,sz);
	// check special /@@@/stream/image or /@@@/stream/data to reconstruct images
	// SHOULD CHECK FASTER IF IT IS A /@@@ MESSAGE!
	m->decode();
	int imaging=0;
	for(int i=0;i<m->msgs.size();i++) {
		const char *address=m->msgs[i].AddressPattern();
		if( oscutils::endsWith(address,"/@@@/stream/image") ) {
			receiveImageStart(m->msgs[i]);
			imaging=1;
		}else if( oscutils::endsWith(address,"/@@@/stream/data") ) {
			receiveImageData(m->msgs[i]);
			imaging=1;
		}
	}
	if( imaging==0 ) push_back(Q_OUT,&m); // send out the complete message since it is not /@@@
	else recyclable::recycle((recyclable **)&m); // don't send since its an image. just recycle
}


//
// receive an image : start of image
// we known the message is /@@@/stream/image
//
void pluginNet::receiveImageStart(const osc::ReceivedMessage &m)
{
	// send current image, if not already sent
	if( i0!=NULL ) {
		log << "event received check" << warn;
		checkOutgoingMessages("received",0,-1,-1);
		push_back(Q_OUT,&i0);n0=-1;
	}
	//log << "received "<<m<< warn;
		
	int32 num,w,h,sz,type;
	const char *view;
	m.ArgumentStream() >> n0 >> num >> w >> h >> sz >> type >> view >> dsz >> osc::EndMessage;
	if( debug ) {
		log << "got image "<< n0 << " " << num << " " << w << " " << h << " " << sz << " " << type << " " << view << " " << dsz << info;
	}

	// get a recyclable image... maybe we should auto-allocate... (/realtime)
	i0=pop_front_wait(Q_IN);
	i0->create(cv::Size(w,h),type);
	//dsz=w*h*i0->elemSize(); // target size
	//i0->n=num;
	if( !this->view.empty() ) {
		// the view was set with /view, so replace it.
		i0->view=this->view;
	}else{
		i0->view=string(view);
	}
	//log << "starting img "<<w<<" x "<<h<<" dsz="<<dsz<<warn;
}


//
// receive an image : data of image
// we known the message is /@@@/stream/data
//
void pluginNet::receiveImageData(const osc::ReceivedMessage &m)
{
	osc::Blob bobo;
	int32 n0r,b;
	m.ArgumentStream() >> n0r >> b >> bobo >> osc::EndMessage;
	if( n0r!=n0 ) {
		if( debug ) log << "data packet "<<n0r<<" does not match "<<n0<<err;
		return;
	}
	memcpy(i0->data+b,bobo.data,bobo.size);
	if( debug ) log << "got data "<<b<<" size "<<bobo.size<<" =?= "<<dsz<<info;
	if( b+bobo.size==dsz ) {
		// this was the last bloc. send the image
		log << "event received check" << warn;
		checkOutgoingMessages("received",0,-1,-1);
		push_back(Q_OUT,&i0);
		n0=-1;
	}
}


//
// send an image
//
void pluginNet::sendImage(blob *i1)
{
	log << "sending images" << info;
	message *m=getMessage(mtu);

        cv::Mat ix;

        if( scale>0.0 ) cv::resize(*i1,ix, cv::Size(), scale,scale);
        else ix=*i1; // no data copy...

        int sz=ix.elemSize();
        int ty=ix.type();
        int dsz=ix.cols * ix.rows * sz;
        int chunk=mtu-160;

        // send general info about a new image
        *m<<osc::BeginMessage("/@@@/stream/image")
                        << n
                        << i1->n
                        << ix.cols
                        << ix.rows
                        << sz
                        << ty
                        << (this->view.empty()?i1->view:this->view)
                        << dsz
                        << osc::EndMessage;
	if( type==TYPE_UDP || type==TYPE_MULTICAST ) {
                udp_send_data(&udp,(unsigned char *)m->ops->Data(),m->ops->Size());
                if( delay ) usleepSafe(delay);
        }else if( type==TYPE_TCP ) {
                //log << "********** tcp out "<<m->ops->Size()<<info;
		char preamble[4];
		oscutils::FromInt32(preamble,m->ops->Size());
                tcp_send_data(&tcp,preamble,4);
                tcp_send_data(&tcp,m->ops->Data(),m->ops->Size());
                //log << "********** checksum "<<checksum((unsigned char *)m->ops->Data(),m->ops->Size()) << info;
        }
        m->reset();

	int len;
        for(int b=0;b<dsz;b+=len) {
                len=chunk;
                if( b+len>dsz ) { len=dsz-b; }
                osc::Blob bobo(ix.data+b,len);

                *m<<osc::BeginMessage("/@@@/stream/data")<< n << b << bobo << osc::EndMessage;
		if( type==TYPE_UDP || type==TYPE_MULTICAST ) {
			udp_send_data(&udp,(unsigned char *)m->ops->Data(),m->ops->Size());
                        if( delay ) usleepSafe(delay);
        	}else if( type==TYPE_TCP ) {
			char preamble[4];
			oscutils::FromInt32(preamble,m->ops->Size());
                        tcp_send_data(&tcp,preamble,4);
                        tcp_send_data(&tcp,m->ops->Data(),m->ops->Size());
			//log << "** checksum "<<checksum((unsigned char *)m->ops->Data(),m->ops->Size()) << info;
                }
                //log << "sending b="<<b<<" len="<<len<<" sz="<<m->ops->Size()<<err;
                m->reset(); // pour envoyer un autre message...
        }
        recyclable::recycle((recyclable **)&m); // reuse m

}














