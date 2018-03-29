

#include <imgv/imgv.hpp>

#include <imgv/pluginPtGrey.hpp>

#include <imgv/imqueue.hpp>

#include <sys/time.h>

using namespace FlyCapture2;
using namespace cv;
using namespace std;

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2


pluginPtGrey::pluginPtGrey() {
	// ports
	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=Q_LOG;

	portsIN["in"]=true;

	cout << "PTGREY CREATED!!!" <<endl;
}

pluginPtGrey::~pluginPtGrey() {
	log << "deleted"<<warn; 
}

void pluginPtGrey::init() {
	initializing=true;
	n=0;
}

void pluginPtGrey::startCamera() {

	FC2Version fc2Version;
        Utilities::GetLibraryVersion( &fc2Version );

        ostringstream version;
        version << "FlyCapture2 library version: " << fc2Version.major << "." << fc2Version.minor << "." << fc2Version.type << "." << fc2Version.build;
        cout << version.str() << endl;

        //Error error;
        //Camera cam;

	BusManager busMgr;
        unsigned int numCameras;
        error = busMgr.GetNumOfCameras(&numCameras);
        if (error != PGRERROR_OK) { error.PrintErrorTrace(); return; }

	PGRGuid guid;
	error = busMgr.GetCameraFromIndex(camnum, &guid);
	if (error != PGRERROR_OK) {  error.PrintErrorTrace(); return; }

        // Connect to a camera
        error = cam.Connect(&guid);
        if (error != PGRERROR_OK) {  error.PrintErrorTrace(); return; }

        // Get the camera information
        CameraInfo camInfo;
        error = cam.GetCameraInfo(&camInfo);
        if (error != PGRERROR_OK) {  error.PrintErrorTrace(); return; }

        //PrintCameraInfo(&camInfo);
	cout << "*** CAMERA INFORMATION ***" << endl;
        cout << "Serial number -" << camInfo.serialNumber << endl;
        cout << "Camera model - " << camInfo.modelName << endl;
        cout << "Camera vendor - " << camInfo.vendorName << endl;
        cout << "Sensor - " << camInfo.sensorInfo << endl;
        cout << "Resolution - " << camInfo.sensorResolution << endl;
        cout << "Firmware version - " << camInfo.firmwareVersion << endl;
        cout << "Firmware build time - " << camInfo.firmwareBuildTime << endl << endl;


        // Start capturing images
        error = cam.StartCapture();
        if (error != PGRERROR_OK) {  error.PrintErrorTrace(); return; }
}

void pluginPtGrey::uninit() {
	log << "uninit"<<warn;
	// Stop capturing images
        error = cam.StopCapture();
        if (error != PGRERROR_OK) { error.PrintErrorTrace(); return; }

        // Disconnect the camera
        error = cam.Disconnect();
        if (error != PGRERROR_OK) { error.PrintErrorTrace(); return; }
}

bool pluginPtGrey::loop() {
    if( initializing ) {
        log << "[read] " << "waiting for ctrl" << info;
	pop_front_ctrl_wait(Q_IN); // attend et process tous les messages
	return true;
    }


	// prochaine image de la camera
	error = cam.RetrieveBuffer( &rawImage );
        if (error != PGRERROR_OK) { error.PrintErrorTrace(); return true; }

	if( n%300==0 ) log << n << info;

	// Convert the raw image
	error = rawImage.Convert( PIXEL_FORMAT_BGR/*MONO8*/, &convImage );
	if (error != PGRERROR_OK) { error.PrintErrorTrace(); return true; }

	// va chercher une image recyclee
	blob *i1=pop_front_wait(Q_IN);


	i1->create(Size(convImage.GetCols(),convImage.GetRows()), CV_8UC3);
        unsigned char *p=i1->data;
        unsigned char *q=convImage.GetData();
	
	//log << "data size = " << convImage.GetDataSize() << warn;

	int sz=convImage.GetDataSize();

	memcpy(p,q,sz);


	i1->view=view;
	i1->n=n++;
	push_back(Q_OUT,&i1);
	
/***
    if (c!=NULL && strcasecmp(c,".png")==0)
    {
      cv::FileStorage file_info;
      img_in=blob::pngread(filename,&file_info);
      file_info["timestamp"]>>img_in.timestamp;
      file_info["yaw"]>>img_in.yaw;
      file_info["pitch"]>>img_in.pitch;
      //log << "[read] " << filename << ": timestamp is " << img_in.timestamp << info;
      file_info.release();
    }
    else if (c!=NULL && (strcasecmp(c,".ppm")==0 || strcasecmp(c,".pgm")==0))
    {
      cv::FileStorage file_info;
      img_in=blob::pbmread(filename,&file_info);
      file_info["timestamp"]>>img_in.timestamp;
      file_info["yaw"]>>img_in.yaw;
      file_info["pitch"]>>img_in.pitch;
      //log << "[read] " << filename << ": timestamp is " << img_in.timestamp << info;
      file_info.release();
    }
    else if (c!=NULL && strcasecmp(c,".lz4")==0 && internal_buffer!=NULL)
    {
      cv::FileStorage file_info;
      img_in=blob::lz4read(filename,&file_info,internal_buffer);
      file_info["timestamp"]>>img_in.timestamp;
      file_info["yaw"]>>img_in.yaw;
      file_info["pitch"]>>img_in.pitch;
      if (n%100==0) log << "[read] " << filename << ": timestamp is " << img_in.timestamp << info;
      file_info.release();
    }
    else
    {
      img_in=cv::imread(filename,-1);
      img_in.timestamp=0;
    }
    if(img_in.empty())
    {
      log << "[PtGrey] Unable to load "<<filename<<err;
      push_back(-1,&i1); //recycle...
      initializing=true;
	  // and reset counter
	  count=0;
      return true;
      //return false;
    }
    if( convertToGray && img_in.channels()>1 ) {
	cv::cvtColor(img_in,*i1, CV_BGR2GRAY);
    }else{
	    img_in.copyTo(*i1);
    }
    i1->timestamp=img_in.timestamp;
    //log << "Loading " << filename << " count="<<count<<info;
    i1->view=view;
    i1->n=n;
    push_back(Q_OUT,&i1);

    read_one_frame=false;
    count++;
    //log << "[read] count is now " << count << info;

*/
	return true;
}

bool pluginPtGrey::decode(const osc::ReceivedMessage &m) {
	//cout << "decoding " << m << endl;
	const char *address=m.AddressPattern();

	if( oscutils::endsWith(address,"/view") ) {
		const char *v;
		m.ArgumentStream() >> v >> osc::EndMessage;
	        view=string(v);
	}else if( oscutils::endsWith(address,"/camera") ) {
		m.ArgumentStream() >> camnum >> osc::EndMessage;
		startCamera();
		initializing=false;
	}else{
		log << "unknown command: "<<m<<warn;
		return false;
	}
	return true;
}


void pluginPtGrey::dump(ofstream &file) {
}



