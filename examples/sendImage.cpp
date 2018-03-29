
/*!

\defgroup sendImage sendImage
@{
\ingroup examples

Send a camera image over the network to someone waiting with a stream plugin


Usage
-----------


xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

~~~
netCamera -in _port_
netCamera -out localhost 45678 [-delay 100] [-scale 0.5| [-realtime|-no-realtime]
~~~

To send the webcam images to the address 192.168.15.200, at port 40000.<br>
The optional delay is in microsecond, between each packet sent.

~~~
netCamera -send 192.168.15.200 40000
~~~

To receive images at local port 40000

~~~
netCamera -receive 40000
~~~


Plugins used
------------
- \ref pluginStream
- \ref pluginViewerGLFW

@}

*/

//
// netCamera
//
//
// plugin used: Stream
//
//

#include <imgv/imgv.hpp>

#include <imqueue.hpp>

#include <imgv/qpmanager.hpp>


#include <imgv/pluginViewerGLFW.hpp>
#include <imgv/pluginStream.hpp>
#include <imgv/pluginWebcam.hpp>
#include <imgv/pluginSyslog.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

//
// to test stream for commands, in tcp
//
//#define TEST_STREAM_CMD


using namespace std;
using namespace cv;

#ifdef APPLE
	#define NORMAL_SHADER "normal150"
#else
	#define NORMAL_SHADER "normal"
#endif


void loadImg(string nom,string view,string qname)
{
        cv::Mat image=cv::imread(nom,-1); // 16 bits reste 16 bits
        blob *i1=new blob(image); // blob refere a image... ca devrait etre bon.
		i1->n=0;
        i1->view=view;
        //Q.push_back((recyclable **)&i1);
        qpmanager::add(qname,i1);
}



int main(int argc,char *argv[])
{
	qpmanager::initialize();

	bool in;
	string host;
	int port;
	string view;
	int mtu;
	int delay;

	// test si les plugins sont disponibles... en direct plutot qu'avec ifdef
	if( !qpmanager::pluginAvailable("pluginStream") ) {
		cout << "Some plugins are not available... :-("<<endl;
		exit(0);
	}

	host="localhost";
	port=45678;
	mtu=1450;
	delay=0;
	view="";

	int i;

	for(i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			cout << "sendImage -host localhost 45678 -mtu 1500 -view img -delay 0 img1.png img2.png ..."
			     << endl;
			exit(0);
		}
		if( strcmp(argv[i],"-host")==0 && i+1<argc ) {
			host=string(argv[i+1]);
			port=atoi(argv[i+2]);
			i+=2;continue;
		}else if( strcmp(argv[i],"-view")==0 && i+1<argc ) {
			view=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-delay")==0 && i+1<argc ) {
			delay=atoi(argv[i+1]);
			i+=1;continue;
		}else if( strcmp(argv[i],"-mtu")==0 && i+1<argc ) {
			mtu=atoi(argv[i+1]);
			i+=1;continue;
		}else break;
	}
	cout << "opencv version " << CV_VERSION << endl;

	//
	// les queues
	//
	qpmanager::addQueue("stream");
	qpmanager::addQueue("sent"); // the image returns here after being sent
	qpmanager::addQueue("log");
#ifdef TEST_STREAM_CMD
	qpmanager::addQueue("streamcmd");
	qpmanager::addQueue("streammsg");
#endif

	//
	// les plugins
	//
	qpmanager::addPlugin("Log","pluginSyslog");
	qpmanager::addPlugin("S","pluginStream");

	qpmanager::connect("log","Log","in");
	qpmanager::connect("log","S","log");

	qpmanager::connect("stream","S","in");
	qpmanager::connect("sent","S","out");

#ifdef TEST_STREAM_CMD
	// test sending commands directly
	qpmanager::addPlugin("SCMD","pluginStream");
	qpmanager::connect("streammsg","SCMD","in");
	qpmanager::connect("streamcmd","SCMD","cmd"); // to outside
	qpmanager::connect("log","SCMD","log");
#endif

	message *m;

	//
	// plugin stream
	//
	m=new message(512);
	*m << osc::BeginBundleImmediate;
	// send all images we give you.
	*m << osc::BeginMessage("/real-time") << false << osc::EndMessage;
	//*m << osc::BeginMessage("/out") << host << port << mtu << delay << osc::EndMessage;
	*m << osc::BeginMessage("/outtcp") << host << port /*<< mtu << delay */<< osc::EndMessage;
	//*m << osc::BeginMessage("/scale") << scale << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("stream",m);

#ifdef TEST_STREAM_CMD
	//
	// stream for commands
	//
	m=new message(512);
	*m << osc::BeginBundleImmediate;
	//*m << osc::BeginMessage("/outmsg") << host << 44445 << mtu << delay << osc::EndMessage;
	*m << osc::BeginMessage("/outmsgtcp") << host << 44445 << osc::EndMessage;
	//*m << osc::BeginMessage("/scale") << scale << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("streammsg",m);

	//
	// send something!!!
	//
	m=new message(512);
	*m << osc::BeginMessage("/display/shader") << 0 << "tunnel" << osc::EndMessage;
	qpmanager::add("streamcmd",m);
#endif


	//
	// mode full automatique
	//
	qpmanager::start("Log");
	qpmanager::start("S");
#ifdef TEST_STREAM_CMD
	qpmanager::start("SCMD");
	sleep(100);
#endif

	for(;i<argc;i++) {
		cout << "Sending image " << argv[i] << endl;
		loadImg(argv[i],view,"stream");
	}

	// wait until the image is fully sent (back in "sent" queue)
	cout << "waiting for buffers to empty..." <<endl;

	while( qpmanager::isEmpty("sent") ) { usleep(250000); }

	cout << "image sent." <<endl;

	qpmanager::stop("S");
	// on devrait lui dire de flusher plus se terminer lui-meme
	qpmanager::stop("Log");


}


