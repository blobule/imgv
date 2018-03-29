#include <iostream>
#include <string> 
#include <vector> 


#include <pluginRaspicam.hpp>

#include <stdio.h>

#include <sys/prctl.h>
#include <sys/time.h>

#define Q_IN 0
#define Q_OUT 1
#define Q_LOG 2
using namespace cv; //Sans namespace, le create ne fonctionne pas. 

//#define USE_PROFILOMETRE
#include <profilometre.h>

#define VERBOSE

pluginRaspicam::pluginRaspicam(){

	ports["in"] = Q_IN;
	ports["out"] = Q_OUT;
	ports["log"] = Q_LOG;

	portsIN["in"] = true;
}

pluginRaspicam::~pluginRaspicam(){

	log << "delete" << warn;
}

void pluginRaspicam::init(){
	
	width = 649;//1280;
	height = 480;//960;

	initialized = false;
	paused=false;
	snapshot=0;

	log << "init!" << std::hex << pthread_self() << warn;

	prctl(PR_SET_NAME, ("imgv:"+name).c_str(), 0, 0, 0);

	n=0;

/*
	camera.setWidth(width);
	camera.setHeight(height);
	camera.setBrightness(50);
	camera.setSharpness(0);
	camera.setContrast(0);
	camera.setSaturation(0);
	camera.setShutterSpeed(0);
	camera.setISO(400);
	camera.setVideoStabilization(true);
	camera.setExposureCompensation(0);
	camera.setFormat(picam::RASPICAM_FORMAT_GRAY);
	camera.setExposure(picam::RASPICAM_EXPOSURE_AUTO);
	camera.setAWB(picam::RASPICAM_AWB_AUTO);
	//camera.setImageEffect(picam::RASPICAM_IMAGE_EFFECT_NEGATIVE);
*/

	/*if(!camera.open()){

	  cout <<"Error opening camera" <<endl;
	  }*/

	//cout << "Connected to camera = " << camera.getId() << "bufs= "<< camera.getImageBufferSize() << endl;
}

void pluginRaspicam::uninit(){
	log << "Closing raspicam" << info;
	camera.close();
}

bool pluginRaspicam::loop(){
	if( !initialized ) {
		// processing commands only, waiting for init-done
		pop_front_ctrl_wait(Q_IN);
		return true;
	}
	pop_front_ctrl_wait(Q_IN); // hope for a snapshot
	usleep(1000);
	return true;
}


bool pluginRaspicam::decode(const osc::ReceivedMessage &m) {
	//log << "decoding " << m << info;
	int k;
	if( plugin::decode(m) ) return true; // verifie pour le /defevent

	const char *address=m.AddressPattern();

	if( oscutils::endsWith(address,"/snapshot") ) {
		if( !paused ) {
			log << "/snapshot require /disable first" << err;
		}else{
			snapshot++;
		}
	}
	else if( oscutils::endsWith(address,"/enable") ) {
		paused=false;
	}
	else if( oscutils::endsWith(address,"/disable") ) {
		paused=true;
	}
	else if( oscutils::endsWith(address,"/view") ) {
		const char *view;
		m.ArgumentStream() >> view >> osc::EndMessage;
		this->view=view;
	}
	else if ( oscutils::endsWith(address, "/setWidth") ){ 
		int32 x;
		m.ArgumentStream() >> x >> osc::EndMessage;
		width=x;
		camera.setWidth(width);
	}
	else if ( oscutils::endsWith(address, "/setHeight") ){
		int32 x;
		m.ArgumentStream() >> x >> osc::EndMessage;
		height=x;
		camera.setHeight(height);
	}
	else if( oscutils::endsWith(address, "/setFormat") ){
		int32 format;
		m.ArgumentStream() >>  format >> osc::EndMessage;
		picam::RASPICAM_FORMAT fmt = (picam::RASPICAM_FORMAT) format;
		if(fmt == picam::RASPICAM_FORMAT_GRAY)
			formatIsGray = true;
		else
			formatIsGray = false;
		camera.setFormat(fmt);
	}	
	else if( oscutils::endsWith(address, "/setBrightness") ){
		int32 brightness;
		m.ArgumentStream() >> brightness >> osc::EndMessage;
		camera.setBrightness(brightness);
	}
	else if( oscutils::endsWith(address, "/setSharpness") ){
		int32 sharpness;
		m.ArgumentStream() >> sharpness >> osc::EndMessage;
		camera.setSharpness(sharpness);
	}
	else if( oscutils::endsWith(address, "/setContrast") ){
		int32 contrast;
		m.ArgumentStream() >> contrast >> osc::EndMessage;
		camera.setContrast(contrast);
	}
	else if( oscutils::endsWith(address, "/setSaturation") ){
		int32 saturation;
		m.ArgumentStream() >> saturation >> osc::EndMessage;
		camera.setSaturation(saturation);
	}
	else if( oscutils::endsWith(address, "/setShutterSpeed") ){
		int32 shutterSpeed;
		m.ArgumentStream() >> shutterSpeed >> osc::EndMessage;
		camera.setShutterSpeed(shutterSpeed);
	}
	else if( oscutils::endsWith(address, "/setIso") ){
		int32 ISO; 
		m.ArgumentStream() >> ISO >> osc::EndMessage;
		camera.setISO(ISO);
	}	
	else if( oscutils::endsWith(address, "/setVideoStabilization") ){
		bool stabilization;
		m.ArgumentStream() >> stabilization >> osc::EndMessage;
		camera.setVideoStabilization(stabilization);
	}
	else if( oscutils::endsWith(address, "/setExposureCompensation") ){
		int32 exposureCompensation;
		m.ArgumentStream() >> exposureCompensation >> osc::EndMessage;
		camera.setExposureCompensation(exposureCompensation);
	}
	else if( oscutils::endsWith(address, "/setAWB") ){
		int32 AWB;
		m.ArgumentStream() >> AWB >> osc::EndMessage;
		picam::RASPICAM_AWB enumAWB = (picam::RASPICAM_AWB) AWB;
		camera.setAWB(enumAWB);
	}
 	else if( oscutils::endsWith(address, "/setExposure") ){
		int32 exposure;
		m.ArgumentStream() >> exposure >> osc::EndMessage;
		picam::RASPICAM_EXPOSURE enumExposure = (picam::RASPICAM_EXPOSURE) exposure;
		camera.setExposure(enumExposure);
	}		
	else if( oscutils::endsWith(address, "/setImageEffect") ){
		int32 imageEffect;
		m.ArgumentStream() >> imageEffect >> osc::EndMessage;
		picam::RASPICAM_IMAGE_EFFECT effect = (picam::RASPICAM_IMAGE_EFFECT) imageEffect;
		camera.setImageEffect(effect);	
	}
	else if( oscutils::endsWith(address, "/setMetering") ){
		int32 metering;
		m.ArgumentStream() >> metering >> osc::EndMessage;
		picam::RASPICAM_METERING enumMetering = (picam::RASPICAM_METERING) metering;
		camera.setMetering(enumMetering);
	}
	else if( oscutils::endsWith(address, "/init-done") ){
		initialized = true;
		if(!camera.open()){
			cout <<"Error opening camera" <<endl;
		}
		if(!camera.startCapture((void *)this,pluginRaspicam::process)){
			cout <<"Error startcapture" <<endl;
		}
	}
	else{
		  log << "unknown command: "<<m<<warn;
		  return false;
	  }
	return true;
}

void pluginRaspicam::process(void *plugin,unsigned char *data,int len,int64_t pts) {
	//cout << "yo" << len << endl;
	//printf(" len = %d\n",len);
	pluginRaspicam *rp=(pluginRaspicam *)plugin;

	if( !rp->initialized ) return;
	if( rp->paused ) {
		if( rp->snapshot==0 ) return;
		rp->snapshot--; // we take one snapshot!
	}

	profilometre_start("process");
	blob *i1=rp->pop_front_wait(Q_IN); // we could receive more /snapshot here... be careful
	if(rp->formatIsGray)	i1->create(cv::Size(rp->width,rp->height),CV_8UC1);
	else			i1->create(cv::Size(rp->width,rp->height), CV_8UC3);

	profilometre_start("memcpy");
	memcpy(i1->data,data,len);
	profilometre_stop("memcpy");

/*
	struct timeval tv;
	gettimeofday(&tv,NULL);
	i1->timestamp=(double)tv.tv_sec+tv.tv_usec/1000000.0;
*/
	i1->timestamp=(double)pts/1000000.0;
	i1->view=rp->view;
	i1->n=rp->n++;
	rp->push_back(Q_OUT,&i1);
	profilometre_stop("process");
}




