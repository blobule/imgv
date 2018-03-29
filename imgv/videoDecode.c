#include "videoDecode.h"

int gDecoderExtraSize;
uint8_t *gDecoderExtraData;

bool gHdmiClockSync;
OMX_VIDEO_CODINGTYPE gDecoderCodingType;

static COMPONENT_T *gEglRender = NULL;

static OMX_BUFFERHEADERTYPE *gEglBuffer = NULL;



COMPONENT_T *gVideoDecode = NULL, *gVideoRender = NULL, *gVideoScheduler = NULL, *gClock = NULL;

COMPONENT_T *gList[5];

ILCLIENT_T *gClient;

TUNNEL_T gTunnel[4];

bool gDecoderIsStarting = true;

OMXPacket *allocPacket(int size){

	OMXPacket *pkt = (OMXPacket *)malloc(sizeof(OMXPacket));

	if(pkt){

		memset(pkt, 0, sizeof(OMXPacket));

		pkt->data = (uint8_t*) malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);

	}

	if(!pkt->data){
		free(pkt);
		pkt = NULL;
	}
	else
	{
		memset(pkt->data + size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
		pkt->size = size;
		pkt->dts  = DVD_NOPTS_VALUE;
		pkt->pts  = DVD_NOPTS_VALUE;
		pkt->now  = DVD_NOPTS_VALUE;
		pkt->duration = DVD_NOPTS_VALUE;
	}
	return pkt;
}



bool getHints(AVStream *stream, streamInfo *hints, bool isMatroska, bool isAvi, bool isMpeg){

	if(!hints || !stream)
		return false;

	hints->codec = stream->codec->codec_id;
	hints->extraData     = stream->codec->extradata;
	hints->extraSize     = stream->codec->extradata_size;
	hints->width         = stream->codec->width;
	hints->height        = stream->codec->height;
	hints->profile       = stream->codec->profile;
	hints->fpsRate       = stream->time_base.num;
	hints->fpsScale      = stream->time_base.den;
	//hints->fpsRate       = stream->r_frame_rate.num;
	//hints->fpsScale      = stream->r_frame_rate.den;

	if(isMatroska && stream->avg_frame_rate.den && stream->avg_frame_rate.num){

		hints->fpsRate = stream->avg_frame_rate.num;
		hints->fpsScale = stream->avg_frame_rate.den;
	}
	else if(
		stream->time_base.num && stream->time_base.den
		//stream->r_frame_rate.num && stream->r_frame_rate.den
		){

		hints->fpsRate = stream->avg_frame_rate.num;
		hints->fpsScale = stream->avg_frame_rate.den;
	}
	else{

		hints->fpsScale = 0;
		hints->fpsRate = 0;
	}

	if(stream->sample_aspect_ratio.num != 0)
		hints->aspect = av_q2d(stream->sample_aspect_ratio)*stream->codec->width/stream->codec->height;
	else if(stream->codec->sample_aspect_ratio.num != 0)
		hints->aspect = av_q2d(stream->codec->sample_aspect_ratio) * stream->codec->width / stream->codec->height;
	else
		hints->aspect = 0.0f;

	if(isAvi && stream->codec->codec_id == AV_CODEC_ID_H264)
		hints->ptsInvalid = true;

	return true;

}

bool fileIsMatroska(AVFormatContext *ctx){

	return strncmp(ctx->iformat->name, "matroska", 8) == 0;
}

bool fileIsAvi(AVFormatContext *ctx){

	return strncmp(ctx->iformat->name, "avi", 3) == 0;
}

bool fileIsMpeg(AVFormatContext *ctx){

	return strncmp(ctx->iformat->name, "mpeg", 4) == 0;
}

AVStream* getStream(AVFormatContext *formatContext){

	AVDictionary *opts = NULL;
	AVStream *videoStream = NULL;
	AVCodecContext *cdcContext = NULL;
	AVCodec *codec = NULL;


	int ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if(ret<0){
		printf("Could not find stream in input file \n");
		exit(1);
	}
	else{
		videoStream = formatContext->streams[ret];
		cdcContext = videoStream->codec;
		codec = avcodec_find_decoder(cdcContext->codec_id);

		if(!codec){
			printf("Failed to find codec \n");
			exit(1);
		}

		if((ret = avcodec_open2(cdcContext, codec, &opts))<0){
			printf("Failed to open codec \n" );
			exit(1);
		}
		return videoStream;
	}
	return NULL;
}
OMXPacket* fillPacket(AVFormatContext *formatContext, AVStream *videoStream, bool endOfFile, bool isMatroska, bool isAvi, bool isMpeg){

	AVPacket pkt;
	OMXPacket *tempOMXpkt = NULL;
	int result = -1;

	if(!formatContext){
		printf("No format \n");
		return NULL;
	}

	if(formatContext->pb)
		formatContext->pb->eof_reached = 0;

	pkt.size = 0;
	pkt.data = NULL;
	pkt.stream_index = MAX_OMX_STREAMS;

	result = av_read_frame(formatContext, &pkt);

	if(result <0){
		endOfFile = true;
		return NULL;
	}
	else{
		if(pkt.stream_index == AVMEDIA_TYPE_VIDEO){

			if(pkt.size < 0 || pkt.stream_index >= MAX_OMX_STREAMS){

				if(formatContext->pb && !formatContext->pb->eof_reached){
					printf("Read no valid packet \n");
				}
				av_free_packet(&pkt);
				endOfFile = true;
				return NULL;

			}
			if(pkt.dts == 0)
				pkt.dts = AV_NOPTS_VALUE;
			if(pkt.pts == 0)
				pkt.pts = AV_NOPTS_VALUE;

			if(isMatroska && videoStream->codec){
				if(videoStream->codec->codec_tag == 0)
					pkt.dts = AV_NOPTS_VALUE;
				else
					pkt.pts = AV_NOPTS_VALUE;
			}

			if(isAvi && videoStream->codec){
				pkt.pts = AV_NOPTS_VALUE;
			}

			tempOMXpkt = allocPacket(pkt.size);

			if(!tempOMXpkt){
				endOfFile = true;
				av_free_packet(&pkt);
				return NULL;
			}

			tempOMXpkt->media = videoStream->codec->codec_type;
			tempOMXpkt->size = pkt.size;

			if(pkt.data)
				memcpy(tempOMXpkt->data, pkt.data, tempOMXpkt->size);

			tempOMXpkt->streamIndex = pkt.stream_index;
			getHints(videoStream, &tempOMXpkt->hints, isMatroska, isAvi, isMpeg);

			//tempOMXpkt->dts = convertTimestamp(pkt.dts, &videoStream->time_base);
			//tempOMXpkt->pts = convertTimestamp(pkt.pts, &videoStream->time_base);

			av_free_packet(&pkt);

			return tempOMXpkt;

		}
		else{
			av_free_packet(&pkt);

			return NULL;
		}
	}
}

static void fillBufferDone(void *data, COMPONENT_T *comp){

	//printf("in callback \n");    
	if(OMX_FillThisBuffer(ilclient_get_handle(gEglRender), gEglBuffer) != OMX_ErrorNone){


		printf("OMX_FillThisBuffer failed in callback \n");
		exit(1);
	}
}

bool sendDecoderConfig(){

	if(gDecoderExtraSize > 0 && gDecoderExtraData != NULL){

		OMX_BUFFERHEADERTYPE *buf =ilclient_get_input_buffer(gVideoDecode, 130, 1);

		if(buf == NULL){
			return false;
		}
		buf->nOffset = 0;
		buf->nFilledLen = gDecoderExtraSize;
		if(buf->nFilledLen > buf->nAllocLen){
			return false;
		}

		memset((unsigned char *)buf->pBuffer, 0x0, buf->nAllocLen);
		memcpy((unsigned char *)buf->pBuffer, gDecoderExtraData, buf->nFilledLen);
		buf->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;

		if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(gVideoDecode), buf) != OMX_ErrorNone){
			return false;
		}

	}
	return true;
}

bool NaluFormatStartCodes(enum AVCodecID codec, uint8_t *inExtraData, int inExtraSize){
	switch (codec) {
		case AV_CODEC_ID_H264:
			if(inExtraSize < 7 || inExtraData == NULL){
				return true;
			}
			else if(*inExtraData != 1){
				return true;
			}
		default:
			break;
	}
	return false;
}

bool setupDecoder(streamInfo hints){

	int decoderWidth = hints.width;
	int decoderHeight = hints.height;

	float FifoSize = 80*1024*60 / (1024*1024);


	if(hints.extraSize > 0 && hints.extraData != NULL){

		gDecoderExtraSize = hints.extraSize;
		gDecoderExtraData = (uint8_t *)malloc(hints.extraSize);
		memcpy(gDecoderExtraData, hints.extraData, hints.extraSize);

	}

	switch (hints.codec) {

		case AV_CODEC_ID_H264:
			{
				switch (hints.profile) {

					case FF_PROFILE_H264_BASELINE:
						gDecoderCodingType = OMX_VIDEO_CodingAVC;
						break;

					case FF_PROFILE_H264_MAIN:
						gDecoderCodingType = OMX_VIDEO_CodingAVC;
						break;

					case FF_PROFILE_H264_HIGH:
						gDecoderCodingType = OMX_VIDEO_CodingAVC;
						break;

					case FF_PROFILE_UNKNOWN:
						gDecoderCodingType = OMX_VIDEO_CodingAVC;
						break;

					default:
						gDecoderCodingType = OMX_VIDEO_CodingAVC;
						break;
				}
			}
			break;

		case AV_CODEC_ID_MPEG4:
			gDecoderCodingType = OMX_VIDEO_CodingMPEG4;
			break;

		case AV_CODEC_ID_MPEG1VIDEO:
		case AV_CODEC_ID_MPEG2VIDEO:
			gDecoderCodingType = OMX_VIDEO_CodingMPEG2;
			break;

		case AV_CODEC_ID_H263:
			gDecoderCodingType = OMX_VIDEO_CodingMPEG4;
			break;

		case AV_CODEC_ID_VP6:
		case AV_CODEC_ID_VP6F:
		case AV_CODEC_ID_VP6A:
			gDecoderCodingType = OMX_VIDEO_CodingVP6;
			break;

		case AV_CODEC_ID_VP8:
			gDecoderCodingType = OMX_VIDEO_CodingVP8;
			break;

		case AV_CODEC_ID_THEORA:
			gDecoderCodingType = OMX_VIDEO_CodingTheora;
			break;

		case AV_CODEC_ID_MJPEG:
		case AV_CODEC_ID_MJPEGB:
			gDecoderCodingType = OMX_VIDEO_CodingMJPEG;
			break;

		case AV_CODEC_ID_VC1:
		case AV_CODEC_ID_WMV3:
			gDecoderCodingType = OMX_VIDEO_CodingWMV;
			break;

		default:
			printf("video codec unknown: %x \n", hints.codec);
			break;
	}

	if((gClient = ilclient_init()) == NULL){

		printf("Can't setup client \n");
		return false;
	}
	ilclient_set_fill_buffer_done_callback(gClient, fillBufferDone, 0);

	if(ilclient_create_component(gClient, &gVideoDecode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0){

		printf("Can't create video decode \n");
		return false;
	}
	gList[0] = gVideoDecode;

	/*if(ilclient_create_component(gClient, &gVideoRender, "video_render", ILCLIENT_DISABLE_ALL_PORTS) !=0){

	  printf("Can't create video render \n");
	  return false;
	  }*/

	if(ilclient_create_component(gClient, &gEglRender, "egl_render", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_OUTPUT_BUFFERS) != 0){

		printf("Can't create egl render \n");
		return false;
	}
	gList[1] = gEglRender;

	if(ilclient_create_component(gClient, &gVideoScheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0){
		printf("Can't create video scheduler \n");
		return false;
	}
	gList[2] = gVideoScheduler;

	if(ilclient_create_component(gClient, &gClock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0){

		printf("Can't create clock \n");
		return false;
	}
	gList[3] = gClock;

	OMX_TIME_CONFIG_CLOCKSTATETYPE clockState;
	clockState.nSize = sizeof(clockState);
	clockState.nVersion.nVersion = OMX_VERSION;
	clockState.eState = OMX_TIME_ClockStateWaitingForStartTime;
	clockState.nWaitMask = 1;

	if(gClock != NULL && OMX_SetParameter(ILC_GET_HANDLE(gClock), OMX_IndexConfigTimeClockState, &clockState) != OMX_ErrorNone){

		printf("Can't set up clock \n");
		return false;
	}
	set_tunnel(gTunnel, gVideoDecode, 131, gVideoScheduler, 10);
	set_tunnel(gTunnel+1, gVideoScheduler, 11, gEglRender, 220);
	set_tunnel(gTunnel+2, gClock, 80, gVideoScheduler, 12);

	if(ilclient_setup_tunnel(gTunnel+2, 0, 0) != 0){

		printf("Can't set up tunnel between clock and video scheduler \n");
		return false;
	}

	if(ilclient_change_component_state(gClock, OMX_StateExecuting) != 0){

		printf("Can't change clock state to OMX_StateExecuting \n");
		return false;
	}

	if(ilclient_change_component_state(gVideoDecode, OMX_StateIdle) != 0){

		printf("Can't change video decoder state to OMX_StateIdle \n");
		return false;
	}

	OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
	memset(&formatType, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	formatType.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	formatType.nVersion.nVersion = OMX_VERSION;
	formatType.nPortIndex = 130;
	formatType.eCompressionFormat = gDecoderCodingType;

	if(hints.fpsScale > 0 && hints.fpsRate > 0){

		formatType.xFramerate = (1<<16)*hints.fpsRate / hints.fpsScale;
	}
	else{
		formatType.xFramerate = 25<<16;
	}


	if(OMX_SetParameter(ILC_GET_HANDLE(gVideoDecode), OMX_IndexParamVideoPortFormat, &formatType) != OMX_ErrorNone){

		printf("Can't set parameters on video decode port \n");
		return false;
	}

	OMX_PARAM_PORTDEFINITIONTYPE portParam;
	memset(&portParam, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	portParam.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	portParam.nVersion.nVersion = OMX_VERSION;
	portParam.nPortIndex = 130;

	if(OMX_GetParameter(ILC_GET_HANDLE(gVideoDecode), OMX_IndexParamPortDefinition, &portParam) != OMX_ErrorNone){

		printf("Can't get parameters on video decode port \n");
		return false;
	}

	portParam.nPortIndex = 130;

	portParam.nBufferCountActual = FifoSize ? FifoSize * 1024 * 1024 / portParam.nBufferSize : 80;

	portParam.format.video.nFrameWidth = decoderWidth;
	portParam.format.video.nFrameHeight = decoderHeight;

	if(OMX_SetParameter(ILC_GET_HANDLE(gVideoDecode), OMX_IndexParamPortDefinition, &portParam) != OMX_ErrorNone){

		printf("Can't set parameters on video decode port \n");
		return false;
	}

	OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
	memset(&concanParam, 0, sizeof(OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE));
	concanParam.nSize = sizeof(OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE);
	concanParam.nVersion.nVersion = OMX_VERSION;
	concanParam.bStartWithValidFrame = OMX_FALSE;

	if(OMX_SetParameter(ILC_GET_HANDLE(gVideoDecode), OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam) != OMX_ErrorNone){

		printf("Can't set concanParam on video decode \n");
		return false;

	}

	if(hints.ptsInvalid){

		OMX_CONFIG_BOOLEANTYPE timeStampMode;
		memset(&timeStampMode, 0, sizeof(OMX_CONFIG_BOOLEANTYPE));
		timeStampMode.nSize = sizeof(OMX_CONFIG_BOOLEANTYPE);
		timeStampMode.nVersion.nVersion = OMX_VERSION;

		timeStampMode.bEnabled = OMX_TRUE;

		if(OMX_SetParameter(ILC_GET_HANDLE(gVideoDecode), (OMX_INDEXTYPE)OMX_IndexParamBrcmVideoTimestampFifo, &timeStampMode) != OMX_ErrorNone){

			printf("Can't set timeStampMode \n");
			return false;
		}
	}

	if(NaluFormatStartCodes(hints.codec, gDecoderExtraData, gDecoderExtraSize)){
		OMX_NALSTREAMFORMATTYPE nalStreamFormat;
		memset(&nalStreamFormat, 0, sizeof(OMX_NALSTREAMFORMATTYPE));
		nalStreamFormat.nSize = sizeof(OMX_NALSTREAMFORMATTYPE);
		nalStreamFormat.nVersion.nVersion = OMX_VERSION;
		nalStreamFormat.nPortIndex = 130;
		nalStreamFormat.eNaluFormat = OMX_NaluFormatStartCodes;

		if(OMX_SetParameter(ILC_GET_HANDLE(gVideoDecode), (OMX_INDEXTYPE) OMX_IndexParamNalStreamFormatSelect, &nalStreamFormat) != OMX_ErrorNone){

			printf("Can't set NALU \n");
			return false;
		}
	}

	/*if(gHdmiClockSync){

		OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
		memset(&latencyTarget, 0, sizeof(OMX_CONFIG_LATENCYTARGETTYPE));
		latencyTarget.nSize = sizeof(OMX_CONFIG_LATENCYTARGETTYPE);
		latencyTarget.nVersion.nVersion = OMX_VERSION;
		latencyTarget.bEnabled = OMX_TRUE;
		latencyTarget.nFilter = 2;
		latencyTarget.nTarget = 4000;
		latencyTarget.nShift = 3;
		latencyTarget.nSpeedFactor = -135;
		latencyTarget.nInterFactor = 500;
		latencyTarget.nAdjCap = 20;

		OMX_SetConfig(ILC_GET_HANDLE(gEglRender), OMX_IndexConfigLatencyTarget, &latencyTarget);
	}*/

	if(ilclient_setup_tunnel(gTunnel, 0, 0) != 0){

		printf("Can't set up tunnel between video decoder and video scheduler \n");
		return false;
	}

	if(ilclient_change_component_state(gVideoDecode, OMX_StateExecuting) != 0){

		printf("Can't change video decode state to OMX_StateExecuting \n");
		return false;
	}

	if(ilclient_change_component_state(gVideoScheduler, OMX_StateExecuting) != 0){

		printf("Can't change video scheduler state to OMX_StateExecuting \n");
		return false;
	}

	if(ilclient_enable_port_buffers(gVideoDecode, 130, NULL, NULL, NULL) != 0){

		printf("Can't enable input port on decoder \n");
	}

	if(!sendDecoderConfig())
	{
		return false;
	}
	gDecoderIsStarting = true;

	return true;

}


bool decode(uint8_t *pData, int iSize, double dts, double pts, void *gEglImage){

	if(pData || iSize > 0){

		//printf("Starting decode function \n");
		unsigned int demuxerBytes = (unsigned int)iSize;
		uint8_t *demuxerContent = pData;

		while (demuxerBytes) {
			//printf("in while \n");
			OMX_BUFFERHEADERTYPE *buf;
			//printf("before get input buffer \n");
			buf = ilclient_get_input_buffer(gVideoDecode, 130, 1);
			if(buf == NULL){
				printf("Can't get input buffer \n");
			}
			//printf("after get input buffer \n");
			buf->nFlags = 0;
			buf->nOffset = 0;

			uint64_t val = (uint64_t)(pts == DVD_NOPTS_VALUE) ? 0 : pts;

			if(gDecoderIsStarting){
				buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
				gDecoderIsStarting = false;
			}
			else{
				if(pts == DVD_NOPTS_VALUE)
					buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
			}

			buf->nTimeStamp = omx_ticks_from_s64(val);

			buf->nFilledLen = (demuxerBytes > buf->nAllocLen) ? buf->nAllocLen : demuxerBytes;

			memcpy(buf->pBuffer, demuxerContent, buf->nFilledLen);

			demuxerBytes -= buf->nFilledLen;
			demuxerContent += buf->nFilledLen;

			if(demuxerBytes == 0){

				buf->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
			}
			//printf("before OMX_EmptyThisBuffer \n");
			while (true) {
				if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(gVideoDecode), buf) == OMX_ErrorNone){
					//printf("Empty buffer done \n");
					break;
				}
				else{
					//printf("Can't empty buffer \n");
				}
			}

			//printf("waiting\n");
			if(ilclient_wait_for_event(gVideoDecode, OMX_EventPortSettingsChanged, 131, 0, 0, 1, ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 0) == 0){
				//printf("waiting done \n");

				OMX_PARAM_PORTDEFINITIONTYPE portImage;
				memset(&portImage, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
				portImage.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
				portImage.nVersion.nVersion = OMX_VERSION;
				portImage.nPortIndex = 131;
				if(OMX_GetParameter(ILC_GET_HANDLE(gVideoDecode), OMX_IndexParamPortDefinition, &portImage) != OMX_ErrorNone){
					printf("Can't get parameter port image \n");

				}

				ilclient_disable_port(gVideoDecode, 131);
				ilclient_disable_port(gVideoScheduler, 10);

				OMX_CONFIG_INTERLACETYPE interlace;
				memset(&interlace, 0, sizeof(OMX_CONFIG_INTERLACETYPE));
				interlace.nSize = sizeof(OMX_CONFIG_INTERLACETYPE);
				interlace.nVersion.nVersion = OMX_VERSION;
				interlace.nPortIndex = 131;
				if(OMX_GetConfig(ILC_GET_HANDLE(gVideoDecode), OMX_IndexConfigCommonInterlace, &interlace) != OMX_ErrorNone){
					printf("Can't get config for interlace \n");
				}

				portImage.nPortIndex = 10;

				if(OMX_SetParameter(ILC_GET_HANDLE(gVideoScheduler), OMX_IndexParamPortDefinition, &portImage) != OMX_ErrorNone){

					printf("Can't set parameter port image \n");
				}

				ilclient_enable_port(gVideoDecode, 131);
				ilclient_enable_port(gVideoScheduler, 10);

				if(ilclient_setup_tunnel(gTunnel+1, 0, 1000) != 0){

					printf("Can't set up tunnel between video scheduler and video render \n");
					return false;
				}

				if(ilclient_change_component_state(gEglRender, OMX_StateIdle) != 0){

					printf("Can't change video render state to OMX_StateExecuting \n");
					return false;
				}

				if(OMX_SendCommand(ILC_GET_HANDLE(gEglRender), OMX_CommandPortEnable, 221, NULL) != OMX_ErrorNone){

					printf("OMX_CommandPortEnable failed\n");
					return false;
				}

				if(OMX_UseEGLImage(ILC_GET_HANDLE(gEglRender), &gEglBuffer, 221, NULL, gEglImage) != OMX_ErrorNone){

					printf("OMX_UseEGLImage failed \n");
					return false;
				}

				if(ilclient_change_component_state(gEglRender, OMX_StateExecuting) != 0){

					printf("Can't change render state to OMX_StateExecuting \n");
					return false;
				}

				if(OMX_FillThisBuffer(ILC_GET_HANDLE(gEglRender), gEglBuffer) != OMX_ErrorNone){

					printf("Fail fill this buffer \n");
					return false;
				}
				else{
					//printf("Fill buffer done \n");
				}

			}
		}
		//printf("decoding packet in videoDecode done \n");
		return true;
	}
	return false;
}

void waitCompletion(){

	OMX_BUFFERHEADERTYPE *buf = ilclient_get_input_buffer(gVideoDecode, 130, 1);

	if(buf == NULL){
		printf("Fail wait for completion: ilclient_get_input_buffer \n");
		return;
	}

	buf->nOffset = 0;
	buf->nFilledLen = 0;

	buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

	if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(gVideoDecode), buf) != OMX_ErrorNone){
		printf("Fail empty this buffer in wait for completion \n");
		return;
	}

	ilclient_wait_for_event(gEglRender, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0, ILCLIENT_BUFFER_FLAG_EOS, 10000);


}

void closeReader(){

	ilclient_flush_tunnels(gTunnel, 0);

	ilclient_disable_tunnel(gTunnel);

	ilclient_disable_tunnel(gTunnel+1);

	ilclient_disable_tunnel(gTunnel+2);

	ilclient_teardown_tunnels(gTunnel);

	ilclient_state_transition(gList, OMX_StateIdle);

	ilclient_cleanup_components(gList);

	OMX_Deinit();

	ilclient_destroy(gClient);
}

