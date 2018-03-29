
//
// plugin viewer GLES2
//
//

#include <iostream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include <pluginViewerGLES2.hpp>

#include <stdio.h>

//
// special pj
//
#include "pj.h"


// pour changer le nom du thread
#include <sys/prctl.h>

#include <sys/time.h>


#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2


//#undef VERBOSE
#define VERBOSE


pluginViewerGLES2::pluginViewerGLES2() {
	// ports
	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=Q_LOG;

	// ports direction
	portsIN["in"]=true;

}


pluginViewerGLES2::~pluginViewerGLES2() {
	log << "delete" <<warn;
}


void pluginViewerGLES2::init() {
	// log
	log << "init! " << std::hex << pthread_self() <<warn;

	// set the process name!
	prctl(PR_SET_NAME,("imgv:"+name).c_str(),0,0,0);

	n=0; // current frame

	PJContext_HostInitialize();

	pj = (PJContext *)malloc(PJContext_InstanceSize());
	PJContext_Construct(pj);

	initialized=false;
	/**
	  const char *argv[2]={"pj","/home/pi/src/pj-0.2-src/shaders/tunnel.glsl"};
	  if(PJContext_ParseArgs(pj, 2, argv) != 0) {
	  log << "problem with ParseArgs"<<err;
	  PJContext_Destruct(pj);
	  free(pj);
	  pj=NULL;
	  }
	 **/

	//shaderPath=string(IMGV_SHARE)+"/shaders/"+"tunnel.glsl";
	//shaderPath=string(IMGV_SHARE)+"/shaders/"+"es2Simple.glsl";
#ifdef SKIP
	static string shader1Path=
		//string(IMGV_SHARE)+"/shaders/"+"es2Simple.glsl";
		string("/home/pi/src/pj-0.2-src/shaders/tunnel.glsl");
	static string shader2Path=
		string(IMGV_SHARE)+"/shaders/"+"es2-effect-lut.glsl";

	//static string shader2Path=string("/home/pi/src/pj-0.2-src/effects/blur.glsl");

	PJContext_AppendLayer(pj,shader1Path.c_str());
	PJContext_AppendLayer(pj,shader2Path.c_str());
	//"/home/pi/src/pj-0.2-src/shaders/tunnel.glsl");
	int k=PJContext_PrepareMainLoop(pj);

	PJContext_SwitchFullscreen(pj);
#endif
}


void pluginViewerGLES2::uninit() {
	log << "uninit!" << warn;

	if( pj!=NULL ) {
		log << "freeing PJContext"<<warn;
		PJContext_Destruct(pj);
		free(pj);
		pj=NULL;
	}

	PJContext_HostDeinitialize();

}

bool pluginViewerGLES2::loop() {
	//if( kill ) return false; // The End!

	//log << "loop"<<info;

	if( !initialized ) {
		// still waiting for init commands...
		pop_front_ctrl_wait(Q_IN);
		return true;
	}

	blob *i3 = NULL;
	blob *i1 = NULL;
	int i;
	for(i=0;;i++) {
		// si on met wait ici, on ne voit plus le clavier ou souris
		i1=pop_front_nowait(Q_IN); // special pas d'attente
		if( i1==NULL ) { break; }

		//log<<"got image view="<<i1->view<<"n = "<< i1->n <<" "<< (pj == NULL)<<info;

		if( pj!=NULL ) {
			if( i1->view=="lut" ) {
				//cout << "ES2 new lut!!!"<<endl;
				blob i2;
				convertLut16ToLut8(*i1,i2,1);
				PJContext_SetLutTextureHi( pj, i2.elemSize1(), i2.elemSize(), i2.cols, i2.rows, i2.data );
				convertLut16ToLut8(*i1,i2,0);
				PJContext_SetLutTextureLow( pj, i2.elemSize1(), i2.elemSize(), i2.cols, i2.rows, i2.data );
				push_back(Q_OUT,&i1); // sera probablement recyclee
			}else{
				//cout << "ES2 new img!!!"<<endl;
				if(i3 != NULL){
					push_back(Q_OUT,&i3); // sera probablement recyclee
				}
				i3 = i1;
				i1 = NULL;
			}
		}

	}
	if(pj != NULL && i3 != NULL){
		//log << i << info;
		if( i3->view=="img2" ) {
			PJContext_SetImage2Texture( pj, i3->elemSize1(), i3->elemSize(), i3->cols, i3->rows, i3->data );
		}else{
			PJContext_SetImageTexture( pj, i3->elemSize1(), i3->elemSize(), i3->cols, i3->rows, i3->data );
		}
		push_back(Q_OUT, &i3);
	}
	if(pj==NULL ) { log << "loop:no pj"<<err;return(false); }
	if( PJContext_MainLoop(pj) ) return(false); // done
	int mousex,mousey,w,h;
	int changed=PJContext_UpdateMousePosition(pj,&mousex,&mousey,&w,&h);
	//cout << "out mousex=" << mousex<<"  mousey="<<mousey<<" w="<<w<<" h="<<h<<endl;
	if( changed ) {
		//
		// send OS messages
		//
		struct timeval tv;
		gettimeofday(&tv,NULL);
		double now=tv.tv_sec+tv.tv_usec/1000000.0;
		checkOutgoingMessages("mouse",now,mousex,mousey,w,h);
		log << "mouse at "<<mousex<<","<<mousey<<" : "<<w<<","<<h<<info;
	}

	usleepSafe(1000);

	return true;
}

int openFile2(threadArguments *args){

	AVFormatContext *formatContext = NULL;
	AVCodecContext *cdcContext = NULL;
	AVStream *videoStream = NULL;

	av_register_all();
	av_log_set_level(AV_LOG_INFO);

	if(avformat_open_input(&formatContext,args->fileName, NULL, NULL)<0){
		printf("Could not find stream information \n");
		return -1;
	}

	formatContext->max_analyze_duration = 500000;
	formatContext->probesize = 100000;
	int ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if(ret<0){
		//printf("Could not find %s stream in input file %s \n", av_get_media_type_string(AVMEDIA_TYPE_VIDEO),args->fileName); (probleme de version!)
		printf("Could not find stream in input file %s \n", args->fileName);
		return -1;
	}
	else{
		avformat_find_stream_info(formatContext, NULL);
		int streamID = ret;
		videoStream = formatContext->streams[streamID];

		args->imageWidth = videoStream->codec->width;
		args->imageHeight = videoStream->codec->height;

		//cout << "image size = " << args->imageWidth <<" x " << args->imageHeight <<endl;

	}

	avformat_free_context(formatContext);
	av_free(cdcContext);

	return 0;

}


void pluginViewerGLES2::init_textures(EGLContext context, EGLDisplay display, GLuint tex)
{
	//// load three texture buffers but use them on six OGL|ES texture surfaces
	//glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gArgs.imageWidth, gArgs.imageHeight, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	//cout << "texture video " << gArgs.imageWidth << " x " << gArgs.imageHeight << endl; 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_NEAREST*/ GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_NEAREST*/ GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	/* Create EGL Image */
	gArgs.image = eglCreateImageKHR(
			display,
			context,
			EGL_GL_TEXTURE_2D_KHR,
			(EGLClientBuffer)tex,
			0);

	if (gArgs.image == EGL_NO_IMAGE_KHR)
	{
		printf("eglCreateImageKHR failed.\n");
		exit(1);
	}
	//gArgs.image = eglImage;
	//gArgs.pID = getpid();

	// Start rendering
	pthread_create(&video_thread, NULL, videoDecodeWithGLTexture, (void *)&gArgs);

	// setup overall texture environment
	//glTexCoordPointer(2, GL_FLOAT, 0, textices);

	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);



	//glEnable(GL_TEXTURE_2D);
	//glEnable(GL_CULL_FACE);


	// Bind texture surface to current vertices
	//glBindTexture(GL_TEXTURE_2D, tex);
}


bool pluginViewerGLES2::decode(const osc::ReceivedMessage &m) {
#ifdef VERBOSE
	log << "decoding: " << m <<info;
#endif
	if( plugin::decode(m) ) return true; // verifie pour le /defevent

	const char *address=m.AddressPattern();

	if( initialized ) {
		// all command available after we are initialized
		if( oscutils::endsWith(address,"/fade") ) {
			float fade;
			m.ArgumentStream() >> fade >> osc::EndMessage;
			PJContext_SetFade(pj,fade);
		}else if( oscutils::endsWith(address,"/video") ) {
			const char *movie;
			m.ArgumentStream() >> movie >> osc::EndMessage;
			fileNameString = movie; 
			gArgs.fileName = fileNameString.c_str();
			if( openFile2(&gArgs)){

				return true;
			}
			// signal(SIGINT, signalCallBackHandler);
			// signal(SIGUSR1, signalCallBackHandler);
			// bcm_host_init();



			// Start OGLES
			//init_ogl(state);

			// Setup the model world
			//init_model_proj(state);

			// initialise the OGLES texture(s)
			EGLDisplay display;
			EGLContext context;
			GLuint tex;

			//printf("Display before = %d \n", display);
			//printf("context before = %d \n", context);
			//printf("texture before = %d \n", tex);
			PJContext_GetSurfaceContextDisplay(pj, NULL, &context, &display);

			PJContext_GetVideoTextureNum(pj, &tex);

			//printf("Display after = %d \n", display);
			//printf("context after = %d \n", context);
			//printf("texture after = %d \n", tex);

			init_textures(context, display,tex);
			//printf("display in plugin = %d \n", display);

		}else if( oscutils::endsWith(address,"/shader") ) {
			int32 layerNum;
			const char *shader;
			m.ArgumentStream() >> layerNum >> shader >> osc::EndMessage;
			if( layerNum<0 || layerNum >= allShaders.size() ) {
				log << "/shader with illegal layer "<<layerNum<<err;
			}else{
				string fullShader=string(IMGV_SHARE)+"/shaders/"+shader+".glsl";
				allShaders[layerNum]=fullShader;
				PJContext_ChangeLayerShader(pj,fullShader.c_str(),layerNum);
			}
		}else{
			log << "already initialized. not executing: "<< m << err;
		}
		return true;
	}

	// all initializing commands are here

	if( oscutils::endsWith(address,"/init-done") ) {
		m.ArgumentStream() >> osc::EndMessage;
		PJContext_PrepareMainLoop(pj);
		PJContext_SwitchFullscreen(pj);
		initialized=true;
	}else if( oscutils::endsWith(address,"/add-layer") ) {
		const char *shader;
		m.ArgumentStream() >> shader >> osc::EndMessage;
		string fullShader=string(IMGV_SHARE)+"/shaders/"+shader+".glsl";
		allShaders.push_back(fullShader);
		PJContext_AppendLayer(pj,fullShader.c_str());
	}else{
		log << "unkown command " << m << err;
	}
	return true;
}


void pluginViewerGLES2::convertLut16ToLut8(blob &from,blob &to,int hi)
{
	// assume from is CV_16UC3
	if( from.type() != CV_16UC3 ) {
		log << "LUT TYPE is wrong..."<<(int)from.elemSize()<<" "<<(int)from.elemSize1()<<warn;
		if( hi==0 ) { // le low bit
			to.create(from.cols,from.rows,CV_8UC3);
			for(int i=0;i<from.cols*from.rows*3;i++) to.data[i]=0;
		}else{ // le hi
			to=from.clone();
		}
		return;
	}

	to.create(from.cols,from.rows,CV_8UC3);
	int i;
	unsigned char *p=from.data+hi;
	unsigned char *q=to.data;
	for(i=0;i<from.cols*from.rows;i++) {
		// +0,+2,+4 -> get the low.
		// +1,+3,+5 -> get the hi
		/*
		   to.data[i*3+0]=from.data[i*6+0+hi]; // x low
		   to.data[i*3+1]=from.data[i*6+2+hi]; // y low
		   to.data[i*3+2]=from.data[i*6+4+hi]; // mask low
		 */
		q[0]=p[0];
		q[1]=p[2];
		q[2]=p[4];
		p+=6;
		q+=3;
	}
}

double pluginViewerGLES2::timestamp(void)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (double)tv.tv_sec+tv.tv_usec/1000000.0;
}

// on doit redefinir le stop...
// attention, ceci est appelle du thread principal, pas celui du plugin
// mais leavemainLoop est threadsafe
/***
  void pluginViewerGLES2::stop() {
  log << "stop!"<<warn;
// averti idle qu'on veut terminer...
kill=true;
// on attend simplement que le thread finisse naturellement.
pthread_join(tid,NULL);
tid=0;
}
 ***/




