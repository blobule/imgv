#ifndef FFMPEG_HPP
#define FFMPEG_HPP

#include <sys/time.h>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

//#define DATA_MOSH

// 0 si ok, -1 si pas ok
int ffmpegOpen(string fname,string format,AVFormatContext **Ctx) {
	int k;
	AVInputFormat *fmt = NULL;
#ifdef FFMPEG_OLD
	AVFormatParameters ap;
	memset(&ap, 0, sizeof(ap));
	ap.width = 0; //frame_width;
	ap.height= 0; // frame_height;
	ap.time_base= (AVRational){1, 25};
	ap.pix_fmt = PIX_FMT_NONE; // frame_pix_fmt;

	if( !format.empty() ) {
		// We have a specific format in mind!!!
		//show_formats();
		fmt = av_find_input_format(format.c_str());
		if(fmt==NULL) {
			    cout << "could not find specified input format '"<<format<<"'. Trying to guess."<<endl;
		}
	}

	k=-1;
	if( fmt!=NULL ) {
		k=av_open_input_file(Ctx, fname.c_str(), fmt, 0, &ap);
		if( k==-1 ) {
			cout << "[ffmpeg] Error opening with format '"<<format<<"'. Trying to guess."<<endl;
		}
	}
        if( k!=0 ) k=av_open_input_file(Ctx,fname.c_str(), NULL, 0, NULL);
        if( k!=0 ) {
           cout << "[ffmpeg] Can't open "<<fname<<". error "<<k<<"("
               <<AVERROR(EIO)<<","<<AVERROR(EILSEQ)<<","<<AVERROR(EDOM)<<")"<<endl;
	   return(-1);
         }
#else
	AVDictionary *options = NULL;
        //av_dict_set(&options, "video_size", "640x480", 0);
        //av_dict_set(&options, "pixel_format", "rgb24", 0);

	if( !format.empty() ) {
                fmt = av_find_input_format(format.c_str());
                if( fmt==NULL ) {
                    cout << "could not find specified input format '"<<format<<"'. Trying to guess."<<endl;
                }
        }
	k=-1;
	if( fmt!=NULL ) {
		// avec format specifie
		cout << "[ffmpeg] opening with format '"<<fname.c_str()<<"'"<<endl;
		k=avformat_open_input(Ctx, fname.c_str(), fmt, &options);
		if( k==-1 ) {
			cout << "[ffmpeg] Error opening with format '"<<format<<"'. Trying to guess."<<endl;
		}
        }
	if( k!=0 ) {
		cout << "[ffmpeg] opening '"<<fname.c_str()<<"'"<<endl;
		k=avformat_open_input(Ctx, fname.c_str(), NULL, &options);
		cout << "[ffmpeg] errno="<<errno<<endl;
	}
        if( k!=0 ) {
           cout << "[ffmpeg] Can't open "<<fname<<". error "<<k<<" "<<errno<<" ("
               <<AVERROR(EIO)<<","<<AVERROR(EILSEQ)<<","<<AVERROR(EDOM)<<")"<<endl;
	   char err[100];
	   av_strerror(k, err, sizeof(err));
	   cout << "[ffmpeg] error='"<<err<<"'"<<endl;
	   return(-1);
         }
        // les options du video sont ici normalement....
        AVDictionaryEntry *t = NULL;
        while(t = av_dict_get(options,"",t,AV_DICT_IGNORE_SUFFIX)) {
                cout<<"[ffmpeg] VIDEO "<<t->key<<" = "<<t->value<<endl;
        }
        av_dict_free(&options);
#endif
	return(0);
}




int ffmpegFindStream(AVFormatContext *Ctx,int videoStreamNum) {
    //
    // Find a stream
    // take streamoffset into account.
    //
    cout << "[ffmpeg] nb streams : "<<Ctx->nb_streams<<endl;
    int i,j;
    int stream=-1;
    for(i=0,j=0;i<Ctx->nb_streams; i++) {
        Ctx->streams[i]->discard = AVDISCARD_ALL; // remove all streams
        cout << "[ffmpeg] stream "<<i<<","<<j<<" codec type "<<Ctx->streams[i]->codec->codec_type<<endl;
        if(Ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            if( videoStreamNum==j ) stream=i;
	    j++; // j counts the video streams
        }
    }
    if( stream==-1 ) {
        cout << "[ffmpeg] Could not find video stream "<<videoStreamNum<<endl;
	return(-1);
    }
    cout << "[ffmpeg] selected stream is "<<stream<<endl;
    Ctx->streams[stream]->discard = AVDISCARD_DEFAULT; // keep this stream
    return(stream);
}


AVCodec *ffmpegFindCodec(AVCodecContext *codecCtx) {
	AVCodec *c;
	//
	// Find the decoder for the video stream
	//
	c = avcodec_find_decoder(codecCtx->codec_id);
	if( c==NULL ) {
        	cout << "[ffmpeg] no codec found for this video"<<endl;
		return(NULL);
	}
	codecCtx->workaround_bugs = 1;
	codecCtx->idct_algo = FF_IDCT_AUTO;
	codecCtx->skip_frame=  AVDISCARD_DEFAULT;
	codecCtx->skip_idct= AVDISCARD_DEFAULT;
	codecCtx->skip_loop_filter=  AVDISCARD_DEFAULT;
	//codecCtx->error_recognition= FF_ER_CAREFUL;
	codecCtx->error_concealment= 3;
	return(c);
}



// lit un bout de frame
// retourne false le frame est ok, true si on a fini le video.
bool GetNextFrameNew(AVFormatContext *formatCtx, AVCodecContext *codecCtx,
    int stream, AVFrame *frame,double timeBase,double &dtssec,double &nextdtssec,AVPacket *packet)
{
    int             frameFinished;
    int k;

    int err_count=0;
    // Decode packets until we have decoded a complete frame
    while( 1 ) {

        if( formatCtx->pb->eof_reached ) {
#ifdef VERBOSE
            cout << "[ffmpeg] EOF reset & continue"<<endl;
#endif
            formatCtx->pb->eof_reached=0; // hack pour continuer
	        return true;
        }

        // get a packet
        if( (k=av_read_frame(formatCtx,packet))<0 ) {
#ifdef VERBOSE
		    cout  << "[ffmpeg] error reading packet" << endl;
#endif
            usleep(100000);
            err_count++;
		    if (err_count>=5) break; // error lecture
  	    }
        if( packet->stream_index != stream ) {
            av_free_packet(packet);
            continue;
        }
#ifdef DATA_MOSH
    int k=packet->flags&AV_PKT_FLAG_KEY;
	printf("packet key -> %d %8ld %s\n",k,packet->pos,(k==1?"**********************":""));
        if( k==1 && packet->pos>500000 ) {
	        //printf("KEY!\n");
            av_free_packet(packet);
            continue;
	}
#endif

        //avcodec_decode_video(pCodecCtx, pFrame,
        //        &frameFinished, packet->data,packet->size);
        avcodec_decode_video2(codecCtx, frame, &frameFinished, packet);

   	    if( frameFinished ) {
	/*
#ifdef SKIP
        printf("[pos=%12Ld] ",packet->pos);
        if( packet->pts!=AV_NOPTS_VALUE ) printf("[t=%12.6f pts=%8Lu] ",packet->pts*timeBase,packet->pts);
        else printf("[t=             pts=        ] ");
        if( packet->dts!=AV_NOPTS_VALUE ) printf("[t=%12.6f dts=%8Lu] ",packet->dts*timeBase,packet->dts);
        else printf("[t=             dts=        ] ");
        if( packet->convergence_duration!=AV_NOPTS_VALUE ) printf("[t=%12.6f conv=%8Ld] ",packet->convergence_duration*timeBase,packet->convergence_duration);
        else printf("[t=             conv=        ] ");
        printf("[flags=%12d] ",packet->flags);
        printf("[dur=%2d] ",packet->duration);
        if( pFrame->key_frame ) printf("[KEY] "); else printf("[   ] ");
        printf("\n");
#endif
	*/

            dtssec=packet->dts*timeBase;

            // usually, we can plan the next frame dts using current dts+duration
            // if we find they are not equal, then we should not sleep because
            // its probably a jump or seek.
            nextdtssec=dtssec+packet->duration*timeBase;

	        // frame->key_frame can be used... maybe
            //printf("packet pos=%10ld t=%12.6f dts=%lu %s\n",packet->pos,packet->dts*timeBase,packet->dts,frame->key_frame?"******KEY*****":"");
	    }
	//printf("frame key -> %d\n",frame->key_frame);

        av_free_packet(packet);
        if( frameFinished ) return(false); // we have a complete frame!
    }
    return(true);
}




double getTimeNow(void)
{
struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec+tv.tv_usec/1000000.0;
}















#endif
