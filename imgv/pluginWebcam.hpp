#ifndef IMGV_PLUGIN_WEBCAM_HPP
#define IMGV_PLUGIN_WEBCAM_HPP


#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>

/*!

\defgroup pluginWebcam Webcam plugin
@{
\ingroup plugins

This plugin uses opencv _videocapture_ to get images from a webcam.<br>
Not very accurate or highly flexible, but simple.<br>
Optionaly, a line can be defined in the image. The image content under the line is
used as a reference, and after that any image change under the line will generate
an event with the position and strength of the change.


Ports
-----

| Name | Direction | Description |
| ---- | --------- | ----------- |
| in   | in        | queue for input recycled images + control messages |
| out  | out       | queue for output images |
| log  | out       | queue for log messages |

Events
------

| Name  | Description |
| ----  | ----------- |
| line  | A change was observed under the defined line.<br>$x is position x100<br>$y is strength<br>$xr is relative position (0..1)  |



OSC commands
------------

### `/cam <int num> <string view>`

  Open the _num_ videocapture stream in opencv.<br>
  A valid _num_ is >=0. Any _num_<0 will stop the current camera.<br>
  The _view_ will be associtated to each image.

### `/line/begin <int x> <int y>`

  Defines the starting point of a line in the image.<br>

### `/line/end <int x> <int y>`

   Defines the ending point of a line in the image.<br>

### `/line/background`

   Set the next image as a reference background for the line.

Example
-------------

Start using the first webcam (0), and set its view for normal display.

~~~
/cam 0 /main/A/tex
~~~

Sample code
-----------
- /ref/playCamera

@}
*/


class pluginWebcam : public plugin<blob> {
	// Homog arguments
	int camnum;

	cv::VideoCapture stream; // un *stream avec new va bugger au delete... bug 

	int n; // frame sequence number

	// pour l'auto detect sur une ligne
	int ptOK; // 0=non, 1=pt1, 2=pt2, 3=pt1+pt2
	cv::Point2f pt1;
	cv::Point2f pt2;
	bool nextImageIsBackground;
	std::vector<cv::Vec3b> fond; // reference background (use /line/background)
	std::vector<int> norm; // differences mesurees
	//int seuil; // 0..3*255 seuil pour considerer une difference importante
	double lastPos;

	string view; // view for this outgoing image


    public:
	pluginWebcam();
	~pluginWebcam();

	void init();
	void uninit();
	bool loop();
	bool decode(const osc::ReceivedMessage &m);
	void dump(ofstream &file);

    private:
	void process(cv::Mat &im);

};


#endif


