
#define __STDC_FORMAT_MACROS
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
//#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bcm_host.h"

#include "ilclient.h"
#include "ilclient.h"

#include <stdbool.h>

#include <signal.h>
#define DVD_NOPTS_VALUE    (-1LL<<52)

#define DVD_TIME_BASE 1000000

#define MAX_OMX_STREAMS        100

typedef struct streamInfo {
    
	// attention aux versions!
    enum AVCodecID codec;
    //enum CodecID codec;

    int fpsScale;
    int fpsRate;
    int height;
    int width;
    float aspect;
    bool variableFrameRate;
    bool stills;
    int level;
    int profile;
    bool ptsInvalid;
    
    void *extraData;
    unsigned int extraSize;
    unsigned int codecTag;
    
}streamInfo;

typedef struct OMXPacket{
    
    double pts;
    double dts;
    double now;
    double duration;
    
    int size;
    uint8_t *data;
    int streamIndex;
    streamInfo hints;
    enum AVMediaType media;
    
}OMXPacket;

typedef struct threadArguments{
    
    void *image;
    int pID;
    const char *fileName;
    int imageWidth;
    int imageHeight;
    
}threadArguments;

OMXPacket *allocPacket(int size);
bool getHints(AVStream *stream, streamInfo *hints, bool isMatroska, bool isAvi, bool isMpeg);
bool fileIsMpeg(AVFormatContext *ctx);
bool fileIsAvi(AVFormatContext *ctx);
bool fileIsMatroska(AVFormatContext *ctx);
AVStream* getStream(AVFormatContext *formatContext);
OMXPacket* fillPacket(AVFormatContext *formatContext, AVStream *videoStream, bool endOfFile, bool isMatroska, bool isAvi, bool isMpeg);
bool sendDecoderConfig();
//bool NaluFormatStartCodes(enum CodecID codec, uint8_t *inExtraData, int inExtraSize);
bool NaluFormatStartCodes(enum AVCodecID codec, uint8_t *inExtraData, int inExtraSize);
bool setupDecoder(streamInfo hints);
static void fillBufferDone(void *data, COMPONENT_T *comp);
bool decode(uint8_t *pData, int iSize, double dts, double pts, void *gEglImage);
void waitCompletion();
void closeReader();
void signalCallBackHandler(int signum);

