
//
// plugin ffmpeg
//


#undef VERBOSE
//#define VERBOSE

#include <pluginFfmpeg.hpp>

// our support library
#include <ffmpeg.hpp>

#ifdef HAVE_PROFILOMETRE
  #define USE_PROFILOMETRE
  #include <profilometre.h>
#endif


#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2


// global locking for ffmpeg
pthread_mutex_t *pluginFfmpeg::avopen = NULL;

//
//
pluginFfmpeg::pluginFfmpeg() : plugin<blob>() {
	if( avopen==NULL ) {
		// le premier appel de constructeur va creer le lock
		// on devrait compter les constructeurs pour pouvoir detruire avopen a la fin
		avopen=new pthread_mutex_t;
		pthread_mutex_init(avopen,NULL);
	}
	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=Q_LOG;

	// for dot output
	portsIN["in"]=true;
}

pluginFfmpeg::~pluginFfmpeg() {
}

void pluginFfmpeg::init() {
	isOpened=false;
	hasClosed=false;
	paused=0;
	maxfps=-1; // no limit on fps

	convertCtx=NULL;

	// auto-looping
	//message *m=new message(512);
	//*m << osc::BeginMessage("/defevent") << "end" << "in" << "/start" << 2 << osc::EndMessage;
	//push_back_ctrl(Q_IN,&m);

	frame=NULL;
	
}

void pluginFfmpeg::startMovie(string filename,string videoFormat,string outputFormat,string view,int videoStream) {
	this->filename=filename;
	this->videoFormat=videoFormat;
	this->outputFormat=outputFormat;
	this->view=view;
	this->videoStream=videoStream;
	restartMovie();
}

// check isOpened to see if it worked...
void pluginFfmpeg::restartMovie() {

	if( isOpened ) {
		log << "can't start movie. still playing another movie."<<err;
		return;
	}

	// global lock for initialisation
	pthread_mutex_lock(avopen);

	/*namedWindow("My Image");*/
	//log << "-- init -- " << pthread_self() << warn;
	// must call this once at registration, to activate ffmpeg library

	av_register_all();
	log << "-- filename is "<< filename<<warn;

	if( outputFormat=="rgb" ) outFormat=USE_RGB;
	else if( outputFormat=="yuv" ) outFormat=USE_YUV;
	else if( outputFormat=="mono" ) outFormat=USE_MONO;
	else if( outputFormat=="rgba" ) outFormat=USE_RGBA;
	else { outFormat=USE_RGB;log << "bad outFormat "<<outFormat<<" (using rgb)"<<err; }
	log << "outformat = "<<outFormat<<info;

	// "" -> empty string
	// empty string is not NULL when askgin for c_str()
	//cout << "format empty = "<<format.empty()<<endl;
	//cout << "format empty null = "<<(format.c_str()==NULL)<<endl;

	//
	// ouverture du fichier
	//
	formatCtx=NULL;
	int k=ffmpegOpen(filename,videoFormat,&formatCtx);
	if( k!=0 ) {
		pthread_mutex_unlock(avopen);
		return;
	}


	// info sur le video
	if( formatCtx->iformat!=NULL ) {
		AVInputFormat* infmt=formatCtx->iformat;
		log << "using input format '"<<infmt->name<<"' ("<<infmt->long_name<<")"<<info;
	}

	//hack de ffplay
	if(formatCtx->pb) formatCtx->pb->eof_reached=0;

	//
	// trouve la bonne stream
	//
	stream=ffmpegFindStream(formatCtx,videoStream);
	if( stream<0 ) { 
		pthread_mutex_unlock(avopen);
		return; }

	//
	// Information sur le Codec de la video stream
	//
	avformat_find_stream_info(formatCtx,NULL); // lookup, in case of headerless file format (mpeg4)

	codecCtx=formatCtx->streams[stream]->codec;
	w=codecCtx->width;
	h=codecCtx->height;
	timeBase=av_q2d(formatCtx->streams[stream]->time_base);
	double rfps=av_q2d(formatCtx->streams[stream]->avg_frame_rate);
	int64 duration=formatCtx->streams[stream]->duration;
	frameLength=formatCtx->streams[stream]->nb_frames;
	frameNum=0;


	log << "video is ("<<w<<" x "<<h<<") timebase="<<timeBase<<" duration="<<duration<<"="<<duration*timeBase<<" : nb="<<frameLength<<" : rfps="<<rfps<<info;


	//
	// trouve un codec pour ce stream
	//
	codec=ffmpegFindCodec(codecCtx);
	if( codec==NULL ) {
		pthread_mutex_unlock(avopen);
		return;
	}

	//
	// ouvre le codec
	//
	// deprecated: avcodec_open(codecCtx,codec);
	// voir http://ffmpeg.org/doxygen/trunk/doc_2examples_2decoding_encoding_8c-example.html
	// WARNING!!!! THIS FUNCTION IS NOT THREAD SAFE!!!!
	// (starting this plugin in parallel with another similar will crash)
	//
	int res=avcodec_open2(codecCtx, codec,NULL);
	if( res<0 ) {
		log << "Can't open codec for "<<filename<<err;
		pthread_mutex_unlock(avopen);
		return;
	}

	//
	// get one frames for decoding
	//
	frame = avcodec_alloc_frame();
	if( frame==NULL ) {
		log << "OOmem frame "<<filename<<err;
		pthread_mutex_unlock(avopen);
		return;
	}

	// setup de la conversion de l'image vers l'output.
	// pour l'instant, on reste 100% 8bit.
	//convertCtx=NULL; // on fera le setup plus tard, au premier frame
	av_init_packet(&packet);
	firstdtssec=-1.0;
	firstRealTime=-1.0;

	isOpened=true;
	finished=false;

	pthread_mutex_unlock(avopen);
}

void pluginFfmpeg::stopMovie() {
	if( !isOpened ) { return; }

	// global lock for initialisation
	pthread_mutex_lock(avopen);

	// libere les ressources... a finir... il en manque!
	av_free(frame);frame=NULL;

	// ondirait que ca plante ici  temps en temps
	// Important!! On doit faire ca avant de fermer le formatCtx.
	if( avcodec_is_open(codecCtx) ) avcodec_close(codecCtx);
	codecCtx=NULL;

#ifdef FFMPEG_OLD
#else
	// do this only after you close the codec.
	avformat_close_input(&formatCtx);
#endif

	// pas absolument necessaire, parce que getCachedContexte reutilise la structure
	sws_freeContext(convertCtx);convertCtx=NULL;


	isOpened=false;
	hasClosed=true; // in case you stop and start

	// safe again!
	pthread_mutex_unlock(avopen);

	// trigger the "EVENT_THE_END"
	log << "end of movie (stop)" << warn;
	checkOutgoingMessages("end",0,-1,-1);
}

void pluginFfmpeg::uninit() {
}

bool pluginFfmpeg::loop() {
	double dtssec;
	double next=0.0;

	if( !isOpened || paused==1 ) {
		// no movie playing... or in pause... wait for messages
		pop_front_ctrl_wait(Q_IN); // check for a start
	}else{
		pop_front_ctrl_nowait(Q_IN); // check for stop, before we decode next image
	}
	if( !isOpened ) return true; // still no movie? -> loop done
	if( paused ) return true; // we are now paused. go wait for messages

	// we are playing a movie!

	finished=GetNextFrameNew(formatCtx,codecCtx,stream,frame,timeBase,dtssec,next,&packet);
	if( finished && frameNum>0 ) { 
	      stopMovie();return true;
	    } // not opened anymore! wait for next movie
	// got a frame!
	if( firstdtssec<0.0 ) firstdtssec=dtssec; // first video time
	dtssec-=firstdtssec; // relative time from start of movie

	double now=getTimeNow();
	if( firstRealTime<0.0 ) firstRealTime=now; // first real time
	now-=firstRealTime;

	//long int dts=(uint64_t)((dtssec+firstdtssec)/timeBase+0.5);

#ifdef VERBOSE
	{
		uint64_t dts=(uint64_t)((dtssec+firstdtssec)/timeBase+0.5);
		log <<"frame "<<setw(6)<<frameNum<<" t="<<setw(10)<<dtssec<<" rt="<<setw(10)<<now<<" ratio="<<setw(10)<<(dtssec/now)<<setw(12)<<" dts="<<dts<<info;
	}
#endif

	//
	// process video
	//
	// demande une image de recyclage. Attend si rien n'est disponible
	//
	blob *i1=pop_front_wait(Q_IN);

	if( i1==NULL ) {
		log << "!!!!!!!!!!!!!!! got NULL recycle image!!!!!!!"<<err;
		return true;
	}
	// c'est possible qu'on aie recu un stop... et meme un stop + start
	if( !isOpened || hasClosed ) { recycle(i1);hasClosed=false;cout << hasClosed<<endl;return true; }

	//log << "got image "<< (frameNum+frameOffset) <<info;

	// numero d'image dans la sequence video
	i1->n=frameNum+frameOffset;

	// special video... frame since begin and until end
	i1->frames_since_begin=frameNum;
	i1->frames_until_end=frameLength-1-frameNum; // negative=unknown
	//cout << "[ffmpeg] begin="<<i1->frames_since_begin<<" end="<<i1->frames_until_end<<endl;

	// timestamp original, ne sert pas a grand chose, en realite
	i1->timestamp=now;

	// set the output view
	i1->view=view;

	// si on veut deplacer la reference (0,0) de l'image
	int offsetX=0;
	int offsetY=0;
	int xs,ys;

	switch( outFormat ) {
	  case USE_YUV:
		convertCtx = sws_getCachedContext(convertCtx,w,h,codecCtx->pix_fmt,
			  w,h,PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
		xs=w;ys=h*3/2;
		i1->create(cv::Size(xs,ys),CV_8UC1);
		// on complete les offsets en U et V
		{
		int offsetXu=offsetX/2;
		int offsetYu=offsetY/2+ys*2/3;
		int offsetXv=offsetXu+xs/2;
		int offsetYv=offsetYu;

		frameRGBdata[0]=(i1->data+xs*offsetY+offsetX);
		frameRGBdata[1]=(i1->data+xs*offsetYu+offsetXu);
		frameRGBdata[2]=(i1->data+xs*offsetYv+offsetXv);
		frameRGBlinesize[0]=xs;
		frameRGBlinesize[1]=xs;
		frameRGBlinesize[2]=xs;
		}
		break;
	  case USE_MONO:
		convertCtx = sws_getCachedContext(convertCtx,w,h,codecCtx->pix_fmt,
			  w,h,PIX_FMT_GRAY8, SWS_FAST_BILINEAR, NULL, NULL, NULL);
		xs=w;ys=h;
		i1->create(cv::Size(xs,ys),CV_8UC1);
		frameRGBdata[0]=(i1->data+xs*offsetY+offsetX);
		frameRGBdata[1]=NULL;
		frameRGBdata[2]=NULL;
		frameRGBlinesize[0]=xs;
		frameRGBlinesize[1]=0;
		frameRGBlinesize[2]=0;
		break;
	  case USE_RGB:
		//
		// pour opencv, on doit utiliser BGR
		//
		convertCtx = sws_getCachedContext(convertCtx,w,h,codecCtx->pix_fmt,
			  w,h,PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
		xs=w;ys=h;
		i1->create(cv::Size(xs,ys),CV_8UC3);
		frameRGBdata[0]=(i1->data+3*(xs*offsetY+offsetX));
		frameRGBdata[1]=frameRGBdata[0]+1;
		frameRGBdata[2]=frameRGBdata[0]+2;
		frameRGBlinesize[0]=
		frameRGBlinesize[1]=
		frameRGBlinesize[2]=xs*3;
		break;
	  case USE_RGBA:
		//
		// pour opencv, on doit utiliser BGRA
		//
		convertCtx = sws_getCachedContext(convertCtx,w,h,codecCtx->pix_fmt,
			  w,h,PIX_FMT_BGRA, SWS_FAST_BILINEAR, NULL, NULL, NULL);
		xs=w;ys=h;
		i1->create(cv::Size(xs,ys),CV_8UC4);
		frameRGBdata[0]=(i1->data+4*(xs*offsetY+offsetX));
		frameRGBdata[1]=frameRGBdata[0]+1;
		frameRGBdata[2]=frameRGBdata[0]+2;
		frameRGBlinesize[0]=
		frameRGBlinesize[1]=
		frameRGBlinesize[2]=xs*4;
		break;
	}
#ifdef VERBOSE
	log << "created image "<<i1->id<<" size "<<i1->cols<<"x"<<i1->rows<<" "<<i1->elemSize1()<<info;
#endif

	// rempli de valeurs aleatoires
	//randu(*i1, Scalar::all(0), Scalar::all(255));

	//profilometre_start("swscale");
	// decode the thing
	sws_scale(convertCtx, frame->data, frame->linesize, 0, codecCtx->height,
		 frameRGBdata, frameRGBlinesize);
	//profilometre_stop("swscale");

	// send to queue 1!
	push_back(Q_OUT,&i1);

	frameNum++;

	// framerate approx. Actually, the real framerate will always be lower than this.
	// the framerate should be higher than the real framerate, in order to give this
	// plugin the chance to buffer a few images, but not steal all the cpu to decode
	// all images in advance.
	// An alternative is to limit the number of empty images in the input queue of this
	// plugin.
	if( maxfps>0.0 ) {
		usleepSafe(1000000.0/maxfps);
	}

	// special end condition... if we reset the length, then this can shorten a movie
	if( frameLength>0 && frameNum>frameLength )
    { 
      stopMovie();
    }

	return true;
}

bool pluginFfmpeg::decode(const osc::ReceivedMessage &m) {
#ifdef VERBOSE
	log << "got message ;"<<m<<info;
#endif
	if( plugin::decode(m) ) return true; // test /defevent
	const char *address=m.AddressPattern();
	if( oscutils::endsWith(address,"/pause") ) {
		if( m.ArgumentCount()==1 ) {
			// set directement l'etat (true/false)
			bool state;
			m.ArgumentStream() >> state >> osc::EndMessage;
			if( state ) paused=1; else paused=0;
		}else{
			// toggle pause
			paused=1-paused;
		}
		return true;
	}else if( oscutils::endsWith(address,"/rewind") ) {
		int64_t timestamp = 0;//ctx->priv->seek_point;
		if( isOpened ) {
			if( formatCtx->start_time != AV_NOPTS_VALUE) timestamp = formatCtx->start_time;
			int ret=av_seek_frame(formatCtx, videoStream, timestamp, AVSEEK_FLAG_BACKWARD);
			if (ret < 0) { log << "Rewind failed" << err; }
			/*for (i = 0; i < ctx->nb_outputs; i++) {
				avcodec_flush_buffers(formatCtx->st[i].st->codec);
				movie->st[i].done = 0;
			}*/
			// pas utile, a mon avis...
			formatCtx->pb->eof_reached = 0;
			// on reset le frameCount!!!
			frameNum=0;
		}else{
			// on va de voir faire un restart...
			restartMovie();
		}
	}else if( oscutils::endsWith(address,"/seek") ) {
		double time;
		m.ArgumentStream() >> time >> osc::EndMessage;
		log << "seeking to time "<<time<<info;

		uint64_t dts=(uint64_t)((time+firstdtssec)/timeBase+0.5);
		dts-=1; // it seems that you should seek one frame early
		log << "Seeking frame to "<<time<<" (assume it is dts, duration=1) -> "<<dts<<info;
		int k=av_seek_frame(formatCtx, videoStream, dts,/* AVSEEK_FLAG_BACKWARD*/AVSEEK_FLAG_ANY);
		log << "seek returned "<<k<<info;
		return true;
	}else if( oscutils::endsWith(address,"/start") ) {
		const char *filename;
		const char *videoFormat;
		const char *outputFormat;
		const char *view;
		int32 videoStream;
		m.ArgumentStream() >> filename >> videoFormat >> outputFormat >> view >> videoStream >> osc::EndMessage;
		stopMovie();
		startMovie(filename,videoFormat,outputFormat,view,videoStream);
		if( !isOpened ) {
			log << "end of movie (could not start)" << warn;
			checkOutgoingMessages("end",0,-1,-1);
		}
	}else if( oscutils::endsWith(address,"/stop") ) {
		stopMovie();
	}else if( oscutils::endsWith(address,"/set/offset") ) {
		m.ArgumentStream() >> frameOffset >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/set/length") ) {
		int32 k;
		m.ArgumentStream() >> k >> osc::EndMessage;
		// we resest the length of the movie to k
		if( k>=0 && frameLength>frameNum+k ) frameLength=frameNum+k;
		log << "length reset to " << k << err;
	}else if( oscutils::endsWith(address,"/max-fps") ) {
		float v;
		m.ArgumentStream() >> v >> osc::EndMessage;
		maxfps=v;
	}
	return false;
}





