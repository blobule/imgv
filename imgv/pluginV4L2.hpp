#ifndef IMGV_PLUGIN_V4L2_HPP
#define IMGV_PLUGIN_V4L2_HPP


#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>

/*!

\defgroup pluginV4L2 Linux Webcam plugin
@{
\ingroup plugins

This plugin uses V4L2 to get images from a Linux webcam.<br>
You have access to all parameters of V4L2.

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
| ...   | ...  |



OSC commands
------------

### `/cam <int num> <string view>`

  Open the /dev/video<num> v4l2 video device.<br>
  The _view_ will be associtated to each image.

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


class pluginV4L2 : public plugin<blob> {
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
	pluginV4L2();
	~pluginV4L2();

	void init();
	void uninit();
	bool loop();
	bool decode(const osc::ReceivedMessage &m);
	void dump(ofstream &file);

    private:
	void process(cv::Mat &im);

};


#endif


