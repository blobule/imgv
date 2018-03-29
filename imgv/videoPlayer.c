#include "videoDecode.h"
#include <GLES/gl.h>
bool bExit = false;

void signalCallBackHandlerVideo(int signum){

	bExit = true;
}

void *videoDecodeWithGLTexture(void *arg){

	AVFormatContext *formatContext = NULL;
	AVStream *videoStream = NULL;
	bool isMatroska;
	bool isAvi;
	bool isMpeg;
	bool setupDecoderDone = false;
	const char *srcName = NULL;
	void *gEglImage;
	int gID;


	bool endOfFile = false;

	//signal(SIGUSR1, signalCallBackHandlerVideo);
	//signal(SIGINT, signalCallBackHandlerVideo);

	threadArguments *temp = (threadArguments *)arg;
	gEglImage = temp->image;
	srcName = temp->fileName;

	if(gEglImage == 0){

		printf("eglImage is null. \n");
		exit(1);
	}

	av_register_all();
	av_log_set_level(AV_LOG_INFO);

	if(avformat_open_input(&formatContext, srcName, NULL, NULL)<0){
		printf("Could not find stream information from videoPlayer %s lala \n", srcName);
		exit(1);
	}

	avformat_find_stream_info(formatContext, 0);
	av_dump_format(formatContext, 0, srcName, 0);


	isMatroska = fileIsMatroska(formatContext);
	isMpeg = fileIsMpeg(formatContext);
	isAvi = fileIsAvi(formatContext);

	videoStream = getStream(formatContext);

	OMX_Init();


	while (endOfFile == false && bExit == false) {

		OMXPacket *filledPacket = fillPacket(formatContext, videoStream, endOfFile, isMatroska, isAvi, isMpeg);        
		if(filledPacket != NULL){

			/*printf("-------------packet-------------\n");
			  printf("Size = %d \n", filledPacket->size);
			 //printf("pts = %d \n", filledPacket->pts);
			 //printf("dts = %d \n", filledPacket->dts);
			printf("stream index = %d \n", filledPacket-> streamIndex);
			printf("fps scale = %d \n", filledPacket->hints.fpsScale);
			printf("fps rate = %d \n", filledPacket->hints.fpsRate);
			printf("height = %d \n", filledPacket->hints.height);
			printf("width = %d \n", filledPacket->hints.width);
			printf("level = %d \n", filledPacket->hints.level);
			printf("profile = %d \n", filledPacket->hints.profile);
			printf("ptsInvalid = %d \n", filledPacket->hints.ptsInvalid);
			printf("extra size = %d \n", filledPacket->hints.extraSize);
			printf("\n\n\n");
			 */ 
			if(setupDecoderDone == false){
				setupDecoder(filledPacket->hints);
				setupDecoderDone = true;
			}
			int k;
			if(isMpeg){
				k=decode(filledPacket->data, filledPacket->size, DVD_NOPTS_VALUE, DVD_NOPTS_VALUE, gEglImage);

			}
			else{
				k=decode(filledPacket->data, filledPacket->size, filledPacket->pts, filledPacket->dts, gEglImage);
			}

			//printf("k = %d \n", k);
			free(filledPacket->data);
			free(filledPacket);

			/*	unsigned char *p = malloc(1920*1080*4);
				int x,y,r;
				r = random()%256; 
				for (y = 0; y<1080; y++){
				for(x = 0; x<1920; x++){
				int i = (y*1920+x)*4;
				p[i] = y*255/1080;
				p[i+1] = x * 255/1920;
				p[i+2] = r;
				p[i+3] = 255;
				}
				}
				glBindTexture(GL_TEXTURE_2D, 4);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);

				free(p);*/


		}
	}
	waitCompletion();
	closeReader();
	return (void *) 0;
}
