#ifndef IMGV_PLUGIN_FFMPEG_HPP
#define IMGV_PLUGIN_FFMPEG_HPP

/*!

\defgroup pluginFfmpeg Ffmpeg plugin
@{
\ingroup plugins

This plugin decodes a video and output the resulting image stream.<br>


Ports
-----

| Name | Direction | Description |
| ---- | --------- | ----------- |
| in   | in        | input queue of recycled images + control messages |
| out  | out       | output queue of images |
| log  | out       | Log output of this plugin |

Events
------

| Name |Description |
| -----|----------- |
|  end | signal the end of the movie |


OSC initialization-only commands
------------

### `/seek <double time>`

### `/start <string filename> <string videoFormat> <string outputFormat> <string view> <int videoStream>`

Start playing the movie specified by _fileName_.<br>
The _videoFormat_ can be left empty for auto-detection of format.<br>
The _outputFormat_ defines how the output images are encoded. It can have the following values:
- "rgb" : Output normal RGB image (this is the default)
- "mono" : Output monochrome images (luminance)
- "yuv" : Output YUV images, ideal for maximal performance when displaying the images
The _view_ assigns a window/quad/uniform to this image for display purposes.<br>
The _videoStream_ allow to select which stream is decoded from the video.  By default, the
firt stream is used.

### `/stop`

### `/set/length`

### `/max-fps <float fps>`

Maximum fps while playing this video. The timing is not synchronized to a clock, so it is
approximate. For example, when playing a video at 30fps using \ref pluginDrip, max-fps could be set to 40 or 50.

### `/pause`

Stop or restart the video decoding.

Todo
-------
- add the possibility of multi-stream decoding.

@}
*/



//
// imguv
//
// plugin Viewer OpenCV
//

/*
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include <stdio.h>
*/

#include "imgv.hpp"

//#include "recyclable.hpp"
//#include "rqueue.hpp"
#include "plugin.hpp"


//
// ffmpeg
//

// special ffmpeg!!!!
#ifndef UINT64_C
  	#define UINT64_C(c) (c ## ULL)
#endif
#ifndef INT64_C
	#define INT64_C(c) (c ## LL)
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

// our support library
//#include "ffmpeg.hpp"

// les trois outputformat possibles
// note: when using images for display as texture, it is much faster
// to send RGBA to the GPU than RGB
#define USE_RGB	0
#define USE_YUV	1
#define USE_MONO 2
#define USE_RGBA 3


//using namespace std;
//using namespace cv;

//
// plugin ffmpeg
//
// queues:
// [0] : recycled images
// [1] : output images
// [2] : mesages (optionel)
//

#include <pthread.h>

//
// un viewer OpenCV!
//
class pluginFfmpeg : public plugin<blob> {

    private:
	// we require a mutex to lock the avcodec_open
	static pthread_mutex_t *avopen;

	// parameters
	string filename;
	string videoFormat; // "" or some format
	string outputFormat; // "rgb"|"yuv"|"mono"
	int videoStream; // number (0,1,2,...) of video stream. audio does not count.

	bool isOpened; // video is opened and available
	bool hasClosed; // if this is true, there was a stop, maybe followed by a start
	int outFormat; // USE_RGB, USE_YUV, USE_MONO, USE_RGBA
	AVFormatContext *formatCtx; // video context.
	AVCodecContext *codecCtx; // le codec de la stream video choisie
	int stream; // actual stream number getting decoded
	int w,h; // taille du video
	double timeBase; // unites pour le temps, en secondes, pour dts et pts
	int64 frameLength; // lenght of movie in frames. 0=unknown length.
	AVCodec *codec;
	AVFrame *frame;		// ffmpeg decode dans ce frame

	int paused; // 0=no, 1=yes.

	// frame RGB decoding
	uint8_t *frameRGBdata[4];
	int frameRGBlinesize[4];


	struct SwsContext   *convertCtx;
	AVPacket packet;
	bool finished; // movie finished? (valid only when isOpened)

	double firstdtssec; // video time
	double firstRealTime; // real time

	int frameNum;
	int32 frameOffset;

	double maxfps; // maximu fps of this player. wait this much time betwen frames
			// -1 = no maximum. decode as fast as possible

	string view; // output view selection "/main/A/tex"

    public:
        pluginFfmpeg();
        ~pluginFfmpeg();

        void init();
        void uninit();
        bool loop();

	bool decode(const osc::ReceivedMessage &m);

    private:
	void startMovie(string filename,string videoFormat,string outputFormat,string view,int videoStream);
	void restartMovie();
	void stopMovie();
};



#endif
