#ifndef IMGV_PLUGIN_PATTERN_HPP
#define IMGV_PLUGIN_PATTERN_HPP


/*!

\defgroup pluginPattern Pattern plugin
@{
\ingroup plugins

Create various image patterns.<br>
Currently, the available patterns are

- checkerboard
- leopard

Ports
-----

|      # | Name | Direction | Description |
| ------ | ---- | --------- | ----------- |
| 0      | in   | in        | queue for input recycled images + control messages |
| 1      | out  | out       | queue for image output |
| 2      | log  | out       | log output |

Events
------

| #    | Name  | Description |
| ---- | ----  | ----------- |
| -1   | saved | Image was saved (not operational) |



OSC commands
------------

### `/set/leopard <int w> <int h> <int nb> <int freq> <bool blur>`

  Setup a leopard pattern. The image will be size (_w_ x _h_).<br>
  The number of pattern output is _nb_.

### `/set/checkerboard <int w> <int h> <int squareSize>`

  Setup a checkboard pattern with image size (_w_ x _h_).<br>

### `/pid <string id>`

  Identification for the profiler. Not needed unless doing timing measurements.

### `/set/view <string view>`

  Set the view of the output images.<br>
  This eventualy controls where the image will be displayed.

### `/manual`

  Set the manual mode.<br>
  In this mode, the command `/go` must be received in order
  to generate the next pattern.<br>

### `/automatic`
  Set the automatic mod, where all patterns are generated as fast as possible.<br>
  This is the default mode.

### `/go`

  In manual mode, trigger the generation of the next image.<br>
  In automatic mode, does nothing.

Sample Code
--------------

- \ref playPattern

@}
*/



#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>



#define PATTERN_NONE	-1
#define PATTERN_LEOPARD	0
#define PATTERN_CHECKERBOARD 1

class pluginPattern : public plugin<blob> {
	// LEOPARD arguments
	int32 w,h; // [LEOPARD, CHECKERBOARD] width/height of image
	int32 freq; // [LEOPARD]
	bool blur; // [LEOPARD]

	int32 nb; // [LEOPARD, evventually other plugins... set to -1 to deactivate]

	// LEOPARD
	// qui sont recycle a l'interne
	cv::Mat m_complexR;
	cv::Mat m_complexI;
	cv::Mat m_complex;
	cv::Mat m_thresh;
	mutable cv::RNG *rng; // * parce qu'il faut pouvoir l'initialiser

	// CHECKERBOARD arguments
	int32 squareSize; // [CHECKERBOARD]

	// pour profilometre (/pid )
	string pid;

	//
	// interne
	//
	int n; // frame number
	int pattern; // quel pattern on affiche?
	bool playAutomatic; // true: auto play as fast as possible. false: wait for /go
	string view; // can be empty... use /set/view 

    public:
	pluginPattern(); //int w,int h, int nb, int freq, bool blur,string pid);
	~pluginPattern();

	void init();
	void uninit();
	bool loop();
	bool decode(const osc::ReceivedMessage &m);
	void dump(ofstream &file);

  private:
	void doOnePattern(cv::Mat &m);
	void getLeopard(int n,cv::Mat &pattern);
	void getCheckerBoard(int n,cv::Mat &pattern);
};


#endif

