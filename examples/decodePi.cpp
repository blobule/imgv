extern "C"{
#include "imgv/pj.h"
#include <sys/prctl.h>
#include <sys/time.h>
#include "imgv/videoPlayer.h"
#include "imgv/videoDecode.h"
#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
}

threadArguments gArgs;
pthread_t videoThread;

void init_textures(EGLContext context, EGLDisplay display, GLuint tex);
int openFile2(threadArguments *args);

int main(){
    
    PJContext *pj;
    
    const char *shaderPath = "/usr/local/share/imgv/shaders/es2-video.glsl";
    
    EGLDisplay display;
    EGLContext context;
    GLuint tex;
    
    gArgs.fileName = "/opt/vc/src/hello_pi/hello_custom/test.mkv";

    PJContext_HostInitialize();
    pj = (PJContext *)malloc(PJContext_InstanceSize());
    PJContext_Construct(pj);
    
    PJContext_AppendLayer(pj, shaderPath);
    PJContext_PrepareMainLoop(pj);
    PJContext_SwitchFullscreen(pj);
    
    openFile2(&gArgs);
    
    PJContext_GetSurfaceContextDisplay(pj, NULL, &context, & display);
    PJContext_GetVideoTextureNum(pj, &tex);
    init_textures(context, display, tex);
    
    while (1) {
        
        PJContext_MainLoop(pj);
    }
}

void init_textures(EGLContext context, EGLDisplay display, GLuint tex){
    
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gArgs.imageWidth, gArgs.imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_NEAREST*/ GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_NEAREST*/ GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gArgs.image = eglCreateImageKHR(display, context, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)tex, 0);
    
    if(gArgs.image == EGL_NO_IMAGE_KHR){
        
        printf("eglCreateImageKHR failed. \n");
        exit(1);
    }
    else{
        printf("eglCreateImageKHR succeeded. \n");
    }
    
    pthread_create(&videoThread, NULL, videoDecodeWithGLTexture, (void *)&gArgs);
    
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
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
        printf("Could not find stream in input file %s \n", args->fileName);
        return -1;
    }
    else{
        avformat_find_stream_info(formatContext, NULL);
        int streamID = ret;
        videoStream = formatContext->streams[streamID];
        
        args->imageWidth = videoStream->codec->width;
        args->imageHeight = videoStream->codec->height;
     
        
    }
    
    avformat_free_context(formatContext);
    av_free(cdcContext);
    
	return 0;
    
}

