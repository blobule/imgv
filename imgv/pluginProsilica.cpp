
#include <imgv/imgv.hpp>


#include <sys/time.h>
//#include <unistd.h>


#include <imgv/pluginProsilica.hpp>

//#define VERBOSE
//#define STATS

//using namespace std;

//
// prosilica plugin
//
// lit une camera avec driver vimba
//

// normalement, on annonce les frames
#define ANNOUNCE_FRAME

#define RESET_THREADED

pluginProsilica::camData::camData(plugin<blob>::mystream &log) : log(log) {
		camId=NULL;
		opened=false;
		capturing=false;
		acquiring=false;
                n=0;
}

pluginProsilica::camData::~camData() {
	log << "cam destructor called for "<<camId<<". acquiring="<<acquiring<<" capturing="<<capturing<<" opened="<<opened <<warn;
	if( acquiring ) stopAcquisition();
	if( capturing ) stopCapture();
	if( opened ) closeCamera();
}

// retourne 0 si ok, <0 si probleme
//
int pluginProsilica::camData::selectCamera(string idref,string uniform)
{
VmbError_t e;
bool isGigE = false;    // GigE transport layer present
VmbCameraInfo_t	*pCameras = NULL;   // A list of camera details
VmbUint32_t	nCount = 0;         // Number of found cameras

	this->refCamId=idref;
	this->uniform=uniform;
	
	// Is Vimba connected to a GigE transport layer?
	e = VmbFeatureBoolGet( gVimbaHandle, "GeVTLIsPresent", &isGigE );
	if ( e==VmbErrorSuccess ) {
	    if( isGigE ) {
		// Send discovery packets to GigE cameras
		e = VmbFeatureCommandRun( gVimbaHandle, "GeVDiscoveryAllOnce");
		if ( e==VmbErrorSuccess ) {
			// you must protect an call to usleep
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
			usleep(200 * 1000);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
			pthread_testcancel();
		}else { log << "Could not ping GigE cameras. Reason: " << e << err; return -1; }
	    }
	} else {
		log << "Unable to query for GigE transport layer. Reason: " << e <<err;
		return -1;
	}


	// check interfaces...
	VmbInterfaceInfo_t Tint[100];
	VmbUint32_t nbInt;

	e=VmbInterfacesList(Tint,100,&nbInt,sizeof(VmbInterfaceInfo_t));
	log << "Interfaces err="<<e<<" nb="<<nbInt<<info;
	for(int i=0;i<nbInt;i++) {
		log << "interface "<<i<<": id="<<Tint[i].interfaceIdString
		<< ", name="<<Tint[i].interfaceName
		<< ", serial="<<Tint[i].serialString<<info;
	}


	// Get the amount of known cameras
	e = VmbCamerasList( NULL, 0, &nCount, sizeof *pCameras);
	if( e!=VmbErrorSuccess || nCount==0) {
		log << "unable to get camera list"<<err;
		return(-1);
	}
	pCameras = new VmbCameraInfo_t[nCount];
	if( pCameras==NULL ) { log << "OOM camera "<<err;return(-2); }

	// Query all static details of all known cameras without having to open the cameras
	e = VmbCamerasList( pCameras, nCount, &nCount, sizeof *pCameras );
	if( e!=VmbErrorSuccess || nCount==0) {
		log << "unable to get camera list"<<err;
		return(-3);
	}
	// Use the first camera if idref is empty
	int i;
	for(i=0;i<nCount;i++) {
		log << "Prosilica cam "<<i<<" : "<<pCameras[i].cameraIdString<< info;
		if( idref.empty() && i==0 ) { this->camId = pCameras[0].cameraIdString;break; }
		if( strcmp( idref.c_str(),pCameras[i].cameraIdString )==0 ) {
			this->camId = pCameras[i].cameraIdString;
			break;
		}
	}
	if( i==nCount ) { log << "did not find camera"<<i<<" name "<<idref<<err; return(-10); }
	log << "found camera "<<i<<" name "<<this->camId<<warn;

	return(0);
}


// retourn 0 si ok, <0 si erreur
int pluginProsilica::camData::openCamera()
{
VmbError_t e;
VmbAccessMode_t cameraAccessMode = VmbAccessModeFull;

	// reset frames to empty
	for(int i=0;i<ROT;i++) {
		Frame[i].buffer=NULL;
		Frame[i].bufferSize=0;
	}

	// Open camera
	//log << "open camera "<<camId<< "("<<pthread_self()<<")" << info;
	e = VmbCameraOpen( camId, cameraAccessMode, &cameraHandle );
	if( e!=VmbErrorSuccess ) { log <<"Unable to open cam "<<camId<<err;return(-1);}

	opened=true;
	return(0);
}

int pluginProsilica::camData::closeCamera()
{
VmbError_t e;
	if( !opened ) { log << "Camera "<<camId<<" not opened. :-("<<err;return(-1); }
	if( capturing ) { log << "Camera "<<camId<<" still capturing! :-("<<err;return(-1); }
	//log << "closing camera "<<camId<< "("<<pthread_self()<<")" << warn;
	//usleepSafe(200000);
	e = VmbCameraClose( cameraHandle );
	if( e!=VmbErrorSuccess ) { log <<"Unable to close cam "<<camId<<" err="<<e<<err;}
	opened=false;
}


int pluginProsilica::camData::startCapture(pluginProsilica *plugin)
{
VmbError_t e;
	// required: opened=true and capturing=false
	if( !opened ) { log << "Camera "<<camId<<" not opened. :-("<<err;return(-1); }
	if( capturing ) { log << "Camera "<<camId<<" already capturing! :-("<<err;return(-1); }

	// Set the GeV packet size to the highest possible value
	// (In this example we do not test whether this cam actually is a GigE cam)
	e=VmbFeatureCommandRun( cameraHandle, "GVSPAdjustPacketSize" );
	if( e==VmbErrorSuccess ) {
		bool done = false;
		while( !done ) {
			e=VmbFeatureCommandIsDone( cameraHandle, "GVSPAdjustPacketSize", &done );
			if( e!=VmbErrorSuccess ) break;
		}
	}

	VmbInt64_t packetSizeGVSP,packetSizeGevSCPS;
	VmbFeatureIntGet( cameraHandle, "GVSPPacketSize", &packetSizeGVSP );
	VmbFeatureIntGet( cameraHandle, "GevSCPSPacketSize", &packetSizeGevSCPS );
	log << "packet size: GVSP="<<packetSizeGVSP<<" GevSCPS="<<packetSizeGevSCPS<<info;

	// Set pixel format. For the sake of simplicity
	// we only support Mono and RGB in this example.
/**
	e = VmbFeatureEnumSet( cameraHandle, "PixelFormat", "Mono8"); //"RGB8Packed"
	if( e!=VmbErrorSuccess ) {
		log << "Unable to set pixelformat"<<err;
		VmbCameraClose ( cameraHandle );
		return(-2);
	}
**/

	// Read back pixel format
	const char* pPixelFormat;
	VmbFeatureEnumGet( cameraHandle, "PixelFormat", &pPixelFormat );
	log << "got pixelformat "<<pPixelFormat<<info;

	VmbInt64_t nPayloadSize;
	// Evaluate frame size
	e = VmbFeatureIntGet( cameraHandle, "PayloadSize", &nPayloadSize );
	if ( e!=VmbErrorSuccess ) {
		log << "Unable to get payload size (err="<<e<<")"<<err;
		VmbCameraClose ( cameraHandle );
		return(-3);
	}
	log << "payload size is "<<nPayloadSize<<info;

	VmbInt64_t v64;
	e = VmbFeatureIntGet( cameraHandle, "Width", &v64 );width=v64;
	e = VmbFeatureIntGet( cameraHandle, "Height", &v64 );height=v64;

	log << "image is "<<width<<" x "<<height<<info;

	//
	// timing information
	//
	e = VmbFeatureIntGet( cameraHandle, "GevTimestampTickFrequency", &v64 );
	tickFrequency=v64;
	if ( e!=VmbErrorSuccess ) {
		log << "Unable to get Tickfrequency (err="<<e<<") UPDATE CAMERA FIRMWARE!!!"<<err;
		VmbCameraClose ( cameraHandle );
		return(-3);
	}
	log << "tick frequency is "<<tickFrequency<<info;

    type=CV_8UC1;
    string format=string(pPixelFormat);
    if (format=="Mono8") {
      type=CV_8UC1;
    }
    else if (format=="Mono12") {
      type=CV_16UC1;
    }
    else if (format=="Mono14") {
      type=CV_16UC1;
    }
    else if (format=="BayerGB8") {
      type=CV_8UC1;
    }
    else if (format=="BayerGB12") {
      type=CV_16UC1;
    }
    else if (format=="BayerGB12Packed") {
      type=CV_8UC1;
      width=width*3/2;
  	  log << "image size changed to "<<width<<" x "<<height<<info;
    }
    else if (format=="RGB8Packed") {
      type=CV_8UC3;
    }
    else if (format=="BGR8Packed") {
      type=CV_8UC3;
    }
    else
    {
      log << "Unknown pixel format: "<<format<<err;
    }

	// ramasse ROT images au depart de la queue de recyclage
	for(int i=0;i<ROT;i++) {
		//image[i]=pop_front_wait(0);
		//image[i]=new blob();
		// ajuste la taille
		//image[i]->create(cv::Size(width,height),nbc==1?CV_8UC1:CV_16UC1);
		// utilise cette image pour le frame
		Frame[i].bufferSize    = (VmbUint32_t)nPayloadSize;
		Frame[i].buffer        = new unsigned char[Frame[i].bufferSize];
		Frame[i].context[0]=this;
		Frame[i].context[1]=plugin;
		Frame[i].context[2]=(void *)(long)i;
		// annonce le frame
#ifdef ANNONCE_FRAME
		e=VmbFrameAnnounce( cameraHandle, Frame+i, (VmbUint32_t)sizeof( VmbFrame_t ) );
		if ( e!=VmbErrorSuccess ) {
			log << "Unable to annonce frame "<<i<<err;
		}
#endif
	}

	// Start Capture Engine
	log << "starting capture" << info;
	e = VmbCaptureStart( cameraHandle );
	if( e!=VmbErrorSuccess ) {
		log << "Unable to start capture"<<err;
		//VmbFrameRevoke( cameraHandle, &Frame );
		//delete (const char *)Frame.buffer;
		VmbCameraClose ( cameraHandle );
		// !!!!!!!!!!!! delete les buffers!
		opened=false;
		return(-5);
	}

	// Queue Frame
	for(int i=0;i<ROT;i++) {
		//log << "queue frame... "<<i<<info;
		e = VmbCaptureFrameQueue( cameraHandle, Frame+i, pluginProsilica::StaticFrameDoneCallback );
		if( e!=VmbErrorSuccess ) {
			log << "Unable to queue frame "<<i<<err;
			//VmbFrameRevoke( cameraHandle, &Frame );
			//delete (const char *)Frame.buffer;
			VmbCameraClose ( cameraHandle );
			opened=false;
			return(-5);
		}
	}

	capturing=true;
	return(0);
}


int pluginProsilica::camData::stopCapture()
{
VmbError_t e;
	// required: opened=true and capturing=false
	if( !capturing ) { log << "Camera "<<camId<<" is not capturing. :-("<<err;return(-1); }
	if( acquiring ) { log << "Camera "<<camId<<" is still acquiring :-("<<err;return(-1); }

	log << "stopCapture camera "<<camId<<warn;

	// etrange... si on ajoute des cout entre les commandes, ca 
	// elimine le probleme de closing timeout.
	//cout << "[prosilica] queue flush"<<endl;
	VmbCaptureQueueFlush( cameraHandle );
	VmbCaptureEnd( cameraHandle );

#ifdef ANNONCE_FRAME
	VmbFrameRevokeAll( cameraHandle );
	for(int i=0;i<ROT;i++) {
		delete (unsigned char *)Frame[i].buffer;
	}
#endif

	/*
	for(int i=0;i<ROT;i++) {
		e = VmbFrameRevoke( cameraHandle, Frame+i );
		delete (unsigned char *)Frame[i].buffer;
	}
	*/
	capturing=false;
	return(0);
}


int pluginProsilica::camData::startAcquisition()
{
VmbError_t e;
	// Start Acquisition
	if( !capturing ) { log << "Camera "<<camId<<" not capturing. :-("<<err;return(-1); }
	if( acquiring ) { log << "Camera "<<camId<<" already acquiring. :-("<<err;return(-1); }
	//VmbFeatureCommandRun( cameraHandle, "GevTimestampControlReset");
	e = VmbFeatureCommandRun( cameraHandle,"AcquisitionStart" );
	if( e!=VmbErrorSuccess ) {
		log << "Unable to start acquisition err="<<e<<err;
		//VmbFrameRevoke( cameraHandle, &Frame );
		//delete (const char *)Frame.buffer;
		//VmbCameraClose ( cameraHandle );
		return(-5);
	}
	acquiring=true;
	return(0);
}

int pluginProsilica::camData::stopAcquisition()
{
VmbError_t e;
	if( !acquiring ) { log << "Camera "<<camId<<" is not acquiring. :-("<<err;return(-1); }
	for(int i=0;i<5;i++) {
		e = VmbFeatureCommandRun( cameraHandle,"AcquisitionStop" );
		if ( VmbErrorSuccess == e ) break;
		log << "Could not stop acquisition. Error code: " << e << err;
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
		usleep(300000); // important pour ne pas avoir de err -12 (timeout)
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
		pthread_testcancel();
	}
	acquiring=false;
	return(e==VmbErrorSuccess?0:-1);
}


////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////


pluginProsilica::pluginProsilica() {
	// reserved ports
	ports["in"]=0;
	ports["out"]=1;
	ports["log"]=2;

	// for dot output
	portsIN["in"]=true;
}

pluginProsilica::~pluginProsilica() { log << "delete"<<warn; }

void pluginProsilica::init() {
	//log << "init tid=" << pthread_self() << warn;
	//n=0;
	waitForRecycle=true;
	// start Vimba API
	VmbError_t e = VmbStartup();
	if( e!=VmbErrorSuccess ) {
		log << "error VmbStartup."<<err;
	}
}


void pluginProsilica::uninit() {
	//log << "uninit tid="<<pthread_self()<<warn;
	//
	// On doit s'assurer qu'on est pas dans le callback!!!
	//
	// fermer les cameras
	for(int i=0;i<cams.size();i++) delete cams[i];
	cams.clear();
	// pour chaque camera... fermer, etc...
	//usleepSafe(500000); // pour aider
	VmbShutdown();
}

bool pluginProsilica::loop() {
	// attends un message. rien d'autre a faire...
	// c' est ici qu'on process tous les messages.
	pop_front_ctrl_wait(0);
	return true;
}

int pluginProsilica::getCamCounter(int i) {
  if (i<0) return -1;
  if (i>=cams.size()) return -1;
  return cams[i]->n;
}

typedef struct{
  pthread_barrier_t *barrier;
  void *handle;
  char msg[100];
  int error;
}msg_data_t;

static void *msg_send(void *d)
{
  msg_data_t *data=(msg_data_t *)(d);
  pthread_barrier_wait(data->barrier);
  data->error=VmbFeatureCommandRun( data->handle, data->msg );     
  //std::cout << "[prosilica!] msg " << data->msg << "(" << data->error << ")" << std::endl;
  pthread_exit(NULL);
}

bool pluginProsilica::decode(const osc::ReceivedMessage &m) {
    int i,imin,imax;
	log << "decoding: " << m <<info;
	const char *address=m.AddressPattern();
	if( plugin::decode(m) ) return true; // verifie pour le /defevent

	//  /open 314653 /main/A/view
	//  /314653/framerate 10, etc...
	//  /314653/start   et  /stop pour la capture
	//  ...
	if( oscutils::endsWith(address,"/wait-for-recycle") ) {
		m.ArgumentStream() >> waitForRecycle >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/usleep") ) {
		int32 us;
		m.ArgumentStream() >> us >> osc::EndMessage;
		usleepSafe(us);
	}else if( oscutils::endsWith(address,"/open") ) {
		const char *id;
		const char *view;
		m.ArgumentStream() >> id >> view >> osc::EndMessage;
		camData *cam=new camData(log);
		if( cam->selectCamera(id,view)==0 ) {
			if( cam->openCamera()==0 ) {
				cams.push_back(cam);
			}else delete cam;
		}else delete cam;
	}else if( oscutils::endsWith(address,"/close") ) {
		const char *id;
		const char *view;
		m.ArgumentStream() >> id >> osc::EndMessage;
		// trouve la camera
		bool useAll=(strcmp("*",id)==0);
		i,imin,imax;
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/close: cam not found"<<err; }
			imin=imax=i;
		}else{ imin=0;imax=cams.size()-1; }
		for(i=imin;i<=imax;i++) {
			cams[i]->closeCamera();
			delete cams[i];
			cams.erase(cams.begin()+i);
		}
	}else if( oscutils::endsWith(address,"/reset-timestamp") ) {
#ifdef RESET_THREADED
        int ccount=0;
        for (i=0;i<cams.size();i++)
        {
     	  if( cams[i]->opened ) 
          {
            ccount++;
          }
        }
	if( ccount>0 ) {
        pthread_t *msg_threads=(pthread_t *)(malloc(sizeof(pthread_t)*cams.size()));
        msg_data_t *msg_data=(msg_data_t *)(malloc(sizeof(msg_data_t)*cams.size()));
        pthread_attr_t msg_attr;
        pthread_attr_init(&msg_attr);
        pthread_attr_setdetachstate(&msg_attr,PTHREAD_CREATE_JOINABLE);
        pthread_barrier_t msg_barrier;
        pthread_barrier_init(&msg_barrier,NULL,ccount);
        int rc;
        void *status;
		for(i=cams.size()-1;i>=0;i--)
        {
			if( cams[i]->opened ) 
            {
                msg_data[i].barrier=&msg_barrier;
                msg_data[i].handle=cams[i]->cameraHandle;
                strcpy(msg_data[i].msg,"GevTimestampControlReset");
				rc=pthread_create(&(msg_threads[i]),&msg_attr,msg_send,(void *)(&(msg_data[i])));
                if (rc) { log << "Error creating "<<msg_data[i].msg<<" thread for camera "<<i<<err; }
			}
		}
        for(i=cams.size()-1;i>=0;i--) 
        {
			if( cams[i]->opened )
            {
				rc=pthread_join(msg_threads[i],&status);
                if (rc) { log << "Error joining "<<msg_data[i].msg<<" thread for camera "<<i<<err; }
			}
		}
        pthread_attr_destroy(&msg_attr);
        pthread_barrier_destroy(&msg_barrier);
        free(msg_threads);
        free(msg_data);
	} // >0
#else
		//for(i=0;i<cams.size();i++) {
		for(i=cams.size()-1;i>=0;i--) {
			if( cams[i]->opened ) {
				rc=VmbFeatureCommandRun( cams[i]->cameraHandle, "GevTimestampControlReset");                
			}
		}
#endif
	}else if( oscutils::endsWith(address,"/start-capture") ) {
		const char *id;
		m.ArgumentStream() >> id >> osc::EndMessage;
		bool useAll=(strcmp("*",id)==0);
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/start-capture: cam not found"<<err; }
			imin=imax=i;
		}else{ imin=0;imax=cams.size()-1; }
		for(i=imin;i<=imax;i++) {
			int k=cams[i]->startCapture(this);
			if( k ) log << "unable to start capture for cam "<<cams[i]->refCamId<<err;
		}
	}else if( oscutils::endsWith(address,"/stop-capture") ) {
		const char *id;
		m.ArgumentStream() >> id >> osc::EndMessage;
		bool useAll=(strcmp("*",id)==0);
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/stop-capture: cam not found"<<err; }
			imin=imax=i;
		}else{ imin=0;imax=cams.size()-1; }
		for(i=imin;i<=imax;i++) {
			int k=cams[i]->stopCapture();
			if( k ) log << "unable to stop capture for cam "<<id<<err;
		}
	}else if( oscutils::endsWith(address,"/start-acquisition") ) {
		// ~= /set/id/AcquisitionStart
		const char *id;
		m.ArgumentStream() >> id >> osc::EndMessage;
		bool useAll=(strcmp("*",id)==0);
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/start-acquisition: cam not found"<<err; }
			imin=imax=i;
		}else{ imin=0;imax=cams.size()-1; }
#ifdef RESET_THREADED
        int ccount=0;
		for(i=imin;i<=imax;i++)
        {
	      if( cams[i]->capturing && cams[i]->acquiring==0 ) ccount++;
        }
	if( ccount>0 ) {
        pthread_t *msg_threads=(pthread_t *)(malloc(sizeof(pthread_t)*cams.size()));
        msg_data_t *msg_data=(msg_data_t *)(malloc(sizeof(msg_data_t)*cams.size()));
        pthread_attr_t msg_attr;
        pthread_attr_init(&msg_attr);
        pthread_attr_setdetachstate(&msg_attr,PTHREAD_CREATE_JOINABLE);
        pthread_barrier_t msg_barrier;
        pthread_barrier_init(&msg_barrier,NULL,ccount);
        int rc;
        void *status;
		for(i=imin;i<=imax;i++)
        {
            // Start Acquisition
	        if( !cams[i]->capturing ) { log << "Error: camera "<<i<<" not capturing."<<err; }
	        else if( cams[i]->acquiring ) { log << "Error: camera "<<i<<" already acquiring."<<err; }
            else
            {
  	          //VmbFeatureCommandRun( cameraHandle, "GevTimestampControlReset");
              msg_data[i].barrier=&msg_barrier;
              msg_data[i].handle=cams[i]->cameraHandle;
              strcpy(msg_data[i].msg,"AcquisitionStart");				
              rc=pthread_create(&(msg_threads[i]),&msg_attr,msg_send,(void *)(&(msg_data[i])));
              if (rc) { log << "Error creating " << msg_data[i].msg << " thread for camera " << i<< err; }
            }
		}
		for(i=imin;i<=imax;i++)
        {
  	        if( cams[i]->capturing && cams[i]->acquiring==0 )
            {
                rc=pthread_join(msg_threads[i],&status);
                if (rc) { log << "Error joining "<<msg_data[i].msg<<" thread for camera "<<i<<err; }
                else if (msg_data[i].error==0) cams[i]->acquiring=true;
                else log << "unable to start capture for cam "<<i<<" ("<<msg_data[i].error<<")"<<err;
            }
		}
        pthread_attr_destroy(&msg_attr);
        pthread_barrier_destroy(&msg_barrier);
        free(msg_threads);
        free(msg_data);
	}
#else
		for(i=imin;i<=imax;i++) {
			//if( delay ) usleepSafe(delay);
			int k=cams[i]->startAcquisition();
			if( k ) log << "unable to start capture for cam "<<i<<" ("<<k<<")"<<err;
		}
#endif
	}else if( oscutils::endsWith(address,"/stop-acquisition") ) {
		// ~= /set/id/AcquisitionStop
		const char *id;
		m.ArgumentStream() >> id >> osc::EndMessage;
		bool useAll=(strcmp("*",id)==0);
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/stop-acquisition: cam not found"<<err; }
			imin=imax=i;
		}else{ imin=0;imax=cams.size()-1; }
		for(i=imin;i<=imax;i++) {
			//if( delay ) usleepSafe(delay);
			int k=cams[i]->stopAcquisition();
			if( k ) log << "unable to stop capture for cam "<<id<<err;
		}
	}else if( oscutils::endsWith(address,"/manual") ) {
		VmbError_t e;
		const char *id;
		m.ArgumentStream() >> id >> osc::EndMessage;
		bool useAll=(strcmp("*",id)==0);
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/manual: cam not found"<<err; return true;}
			imin=imax=i;
		}else{ imin=0;imax=cams.size()-1; }
		for(i=imin;i<=imax;i++) {
			e=VmbFeatureEnumSet(cams[i]->cameraHandle,"TriggerMode","On");
			e=VmbFeatureEnumSet(cams[i]->cameraHandle,"TriggerSource","Software");
			//cams[i]->manual=true;
		}
	}else if( oscutils::endsWith(address,"/auto") ) {
		VmbError_t e;
		const char *id;
		float fps;
		m.ArgumentStream() >> id >> fps >> osc::EndMessage;
		bool useAll=(strcmp("*",id)==0);
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/auto: cam not found"<<err; return true;}
			imin=imax=i;
		}else{ imin=0;imax=cams.size()-1; }
		for(i=imin;i<=imax;i++) {
			e=VmbFeatureEnumSet(cams[i]->cameraHandle,"TriggerMode","On");
			e=VmbFeatureEnumSet(cams[i]->cameraHandle,"TriggerSource","FixedRate");
			e=VmbFeatureFloatSet(cams[i]->cameraHandle,"AcquisitionFrameRateAbs",(double)fps);
			//cams[i]->manual=false;
		}
	}else if( oscutils::endsWith(address,"/counter-subtract") ) {
		const char *id;
        	int32 nsub;
		m.ArgumentStream() >> id >> nsub >> osc::EndMessage;
		bool useAll=(strcmp("*",id)==0);
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/counter-subtract: cam not found"<<err; }
			imin=imax=i;
		}else{ imin=0;imax=cams.size()-1; }
		for(i=imin;i<=imax;i++) {
			cams[i]->n-=nsub;
		}
	}else if( oscutils::endsWith(address,"/set") ) {
		// on va dire: /set [id] [feature name] [feature value]
		// id peut etre le cas special: "*" pour all
		// value peut etre: int, float, symbol, string, rien=cmd
		VmbError_t e;
		const char *id;
		const char *feature;
		osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		id=(arg++)->AsString();
		feature=(arg++)->AsString();
		bool useAll=(strcmp("*",id)==0);
		// trouve la bonne camera (si !useAll)
		if( !useAll ) {
			for(i=0;i<cams.size();i++) if( strcmp(id,cams[i]->refCamId.c_str())==0 ) break;
			if( i==cams.size() ) { log <<"/set: cam not found"<<err; return true;}
			imin=imax=i;
		}else{
			imin=0;imax=cams.size()-1;
		}
		//log << "/set imin="<<imin<<",imax="<<imax<<info;
        //log << "/set feature="<<feature<<info;
		if( arg->IsInt32() ) {
			int val=(arg++)->AsInt32();
			for(i=imin;i<=imax;i++) {
				e=VmbFeatureIntSet(cams[i]->cameraHandle,feature,(VmbInt64_t)val);
				//log << "set "<<feature<<" to "<<val<<" (32) : err="<<e<<info;
			}
		}else if( arg->IsInt64() ) {
			VmbInt64_t val=(arg++)->AsInt64();
			for(i=imin;i<=imax;i++)
            {
              e=VmbFeatureIntSet(cams[i]->cameraHandle,feature,val);
		  	  //log << "set "<<feature<<" to "<<val<<" (64) : err="<<e<<info;
            }
		}else if( arg->IsFloat() ) {
			float val=(arg++)->AsFloat();
			for(i=imin;i<=imax;i++) e=VmbFeatureFloatSet(cams[i]->cameraHandle,feature,(double)val);
		}else if( arg->IsDouble() ) {
			double val=(arg++)->AsDouble();
			for(i=imin;i<=imax;i++)e=VmbFeatureFloatSet(cams[i]->cameraHandle,feature,val);
		}else if( arg->IsSymbol() ) {
			const char *val=(arg++)->AsSymbol();
			for(i=imin;i<=imax;i++)e=VmbFeatureEnumSet(cams[i]->cameraHandle,feature,val);
		}else if( arg->IsString() ) {
			const char *val=(arg++)->AsString();
			for(i=imin;i<=imax;i++)e=VmbFeatureStringSet(cams[i]->cameraHandle,feature,val);
		}else if( arg->IsBool() ) {
			bool val=(arg++)->AsBool();
			for(i=imin;i<=imax;i++)e=VmbFeatureBoolSet(cams[i]->cameraHandle,feature,(VmbBool_t)val);
		}else{
			for(i=imin;i<=imax;i++) e=VmbFeatureCommandRun(cams[i]->cameraHandle,feature);

			/*
			struct timeval tv;
			gettimeofday(&tv,NULL);
			double now=tv.tv_sec+tv.tv_usec/1000000.0;
			log << "PROSILICA delta="<<now-last<<info;
			last=now;
			*/
			if( feature=="AcqusitionStart" ) cams[i]->acquiring=true;
			else if( feature=="AcqusitionStop" ) cams[i]->acquiring=false;
		}
		// e is not valid if we use "*"
		if( !useAll ) if( e ) log << "set "<<feature<<" returned "<< e<<err;
	}else return false;
	return true;

}


//
// context[0] contient l'info camera
// context[1] est le -> plugin
//
// Attention: on roule dans un autre thread que celui du plugin!!!!!
//
// fonctions qu ine peuvent pas etre appelle pendant qu'on recoit des images:
//
// VmbShutdown
// VmbCameraOpen
// VmbCameraClose
// VmbFrameAnnounce
// VmbFrameRevoke
// VmbFrameRevokeAll
// VmbCaptureStart
// VmbCaptureStop
//
void pluginProsilica::FrameDoneCallback(const VmbHandle_t hCamera, VmbFrame_t * pFrame ) {
	VmbError_t e;
	camData *cam=(camData *)(pFrame->context[0]);
	//log << "callback: "<<pthread_self() << info;
	//log << "!!! framedone cam="<<cam->camId<<info;

 int type;
    VmbPixelFormatType pformat_in,pformat_out;

	if(VmbFrameStatusComplete==pFrame->receiveStatus){
		// frame received!
		//log << "frame received!! tid=" << pthread_self() << info;
		//double t=(Frame[r].timestamp/1000UL)/1000000.0;
		// on peut envoyer cette image!

		//struct timeval tv;
		//gettimeofday(&tv,NULL);
		//double now=tv.tv_sec+tv.tv_usec/1000000.0;
		//long int inow=tv.tv_sec*1000000+tv.tv_usec;

		blob *i0;
		if( waitForRecycle ) {
			// DO NOT PROCESS MESSAGES! (they are deferred to processing in loop() )
			i0=pop_front_noctrl_wait(0); // une image recyclee
		}else{
			// DO NOT PROCESS MESSAGES! (they are deferred to processing in loop() )
			i0=pop_front_noctrl_nowait(0); // could be null
			if( i0==NULL ) {
				// allocate a new image, since we don't want to block
				i0=new blob();
				if( !badpos(0) ) {
					i0->setRecycleQueue(Qs[0]);
				}
				//log << "Added an image!" <<info;
			}
		}

        cv::Mat base(cv::Size(cam->width,cam->height),CV_8UC1,pFrame->buffer);
        base.copyTo(*i0);

		i0->timestamp=(double)pFrame->timestamp/cam->tickFrequency;
		//double yo=(double)pFrame->timestamp/1000.0;
		//log << "@ " << i0->timestamp <<info;
		i0->view=cam->uniform;
		i0->n=cam->n; // status ok since positive.
        if (i0->n>=0 && i0->n<3) log << (1000.0*i0->timestamp) << info;
        //if (i0->n%100==0) log << i0->view << " (" << i0->n << ")" << info;
		cam->n++; // next image!
		push_back(1,&i0);
	} else {
		// frame error
		log << "frame not complete status="<<pFrame->receiveStatus<<" (0=ok,-1=incomplete,-2=toosmall,-3=invalid)" <<err;
		//recyclable::recycle((recyclable**)(image+r));
		blob *i0=pop_front_noctrl_wait(0); // une image recyclee
		cv::Mat base(cv::Size(cam->width,cam->height),type);
		//base.copyTo(*i0);
		i0->timestamp=(double)pFrame->timestamp/cam->tickFrequency;
		//double yo=(double)pFrame->timestamp/1000.0;
		//log << "@ " << i0->timestamp <<info;
		i0->view=cam->uniform;
		i0->n=-1; // status bad
		push_back(1,&i0);
	}
	// possible message pour avertir qu'on a pris la photo
	checkOutgoingMessages("sent",0,-1,-1);

	// queue frame
	e = VmbCaptureFrameQueue( hCamera, pFrame, StaticFrameDoneCallback );
	if( e!=VmbErrorSuccess ) { log << "Unable to queue frame "<<err; }
	return;
}

//
// special... callback d'image recue.
//
void VMB_CALL /*pluginProsilica::VMB_CALL*/ pluginProsilica::StaticFrameDoneCallback(const VmbHandle_t hCamera, VmbFrame_t * pFrame ) {
	//cout << "frame done!"<< endl;
	pluginProsilica *P = (pluginProsilica *)pFrame->context[1];
	P->FrameDoneCallback(hCamera,pFrame);
}





