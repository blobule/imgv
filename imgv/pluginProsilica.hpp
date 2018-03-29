#ifndef IMGV_PLUGIN_PROSILICA_HPP
#define IMGV_PLUGIN_PROSILICA_HPP


/*!

\defgroup pluginProsilica Prosilica plugin
@{
\ingroup plugins


This plugin captures images from Allied Prosilica cameras. It uses the [vimba] SDK.

[vimba]: http://www.alliedvisiontec.com/apac/products/software/vimba-sdk.html ""


Ports
-----

|      # | Name | Direction | Description |
| ------ | ---- | --------- | ----------- |
| 0      | in   | in        | input queue for recycled images and messages |
| 1      | out  | out       | output queue for images |

Events
------

| #    | Name  | Description |
| ---- | ----  | ----------- |
| -1   | photo-sent | A picture was just sent out |


OSC commands
------------

Supports the [basic plugin commands](@ref triage).


### `/open <string id> <string view>`

Open a camera with a specific _id_.<br>
If _id_ is empty (""), then the first camera is opened.

### `/close <string id>`

Close a camera with a specific _id_, previously opened using `/open`.<br>
The special id "*" will close all currently opened cameras.

### `/reset-timestamp`

Reset the internal timestamp of all opened cameras to 0.<br>
This allows synchronization of the camera clocks. Usually, the sync is within 1ms.

### `/start-capture <string id>`

Start the capture engine for camera _id_ (or all cameras if _id_ id "*").<br>
The camera must be already opened with `/open`.<br>
Some commands that change the image format (like resolution, depth, etc...) can not be
used after `\start-capture` is executed.

### `stop-capture <string id>`

Stop the capture engine for camera _id_ (or all cameras if _id_ is "*").<br>
The camera must have executed `/start-capture` previously.

### `/start-acquisition <string id>`

Start the acquisition process for camera _id_ (or all cameras if _id_ is "*").<br>
The capture engine must be already started using `/start-capture`.<br>
After this command is issued, any triggeredd image will be recorded and sent to the output port.

The Vimba command executed is _AcquisitionStart_.

### `/stop-acquisition <string id>`

Stop the acquisition process for camera _id_ (or all cameras if _id_ is "*").<br>
The camera must be already doing acquisition (using commande `/start-acquisition`).

### `/manual <string id>`

Put the camera _id_ (or all cameras if _id_ is "*") in manual trigger mode.<br>
This is a shortcut for the following sequence of commands:
~~~
TriggerMode On
TriggerSource Software
~~~

### `/auto <string id> <float fps>`

Put the camera _id_ (or all cameras if _id_ is "*") in automatic mode, where images
are acquired at the framrate _fps_.<br>
This is a shortcut for the following sequence of commands:
~~~
TriggerMode On
TriggerSource FixedRate
AcquisitionFrameRateAbs <fps>
~~~

### `/set <string id> <string feature> [<value>]`

Set a camera parameter _feature_ to a specific _value_, for camera _id_ (or all cameras
if _id_ is "*").<br>
The type of value (int32, int64, float, double, Symbol, String, Bool) determines which call
is made in the vimba API. Note that Symbol is used to specify Enumeration values.

If there is no value, then it is assumed that <feature> is a command to run.

### `/wait-for-recycle  <bool>`

   Specify if the plugin should wait or not for the image recycle queue.<br>
 By default, waiting is set to true, so the memory footprint is garanteed but images could be lost.<br>
 Set to false,
images will be added to the recycle queue as needed, so the framerate is garanteed.


Example 1
----------

Typical capture

~~~
/open ""
/set "" Width 1360
/set "" Height 1024
/start-capture ""
/auto "" 10.0
/reset-timestamp
/start-acquisition ""
~~~

Todo
----

- event multiple pour cameras multiples
- check if `\close ""` works

@}


*/



#include <imgv/imgv.hpp>


#include <imgv/plugin.hpp>

// pour ce plugin
#include <VimbaC.h>

//#define VERBOSE
//#define STATS

//using namespace std;

//
// prosilica plugin
//
// lit une camera avec driver vimba
//

#define ROT 10

class pluginProsilica;




class pluginProsilica : public plugin<blob> {
	private:
	class camData; // forward declaration

	vector<camData*> cams; // les cameras!

	// temporaire... statisiques
	//double last;
    public:
	pluginProsilica();
	~pluginProsilica();

    int getCamCounter(int i);

	void init();
	void uninit();

	bool loop();

	private:

	bool waitForRecycle; // true: normal wait. false: allocate images if needed

	bool decode(const osc::ReceivedMessage &m);

	//
	// context[0] contient l'info camera
	// context[1] est le -> plugin
	//
	public:
	void FrameDoneCallback(const VmbHandle_t hCamera, VmbFrame_t * pFrame );

	static void VMB_CALL StaticFrameDoneCallback(const VmbHandle_t hCamera, VmbFrame_t * pFrame);

	private:

	class camData {
		private:
		// pour les log
		plugin<blob>::mystream &log;
		public:
		string refCamId;	// ce qu'on voulait
		const char * camId;    // ce qu'on a trouve

		// numero de l'image (sequence)
		int n;

        int type; //pixel type: CV_8UC1, CV_8UC3, CV_16UC1 or CV_16UC3

		// est-ce que la camera est ouverte?
		bool opened;
		// est-ce que la camera est "startCapture"?
		bool capturing;
		// est-ce que la camera est "startAcquisition"?
		bool acquiring;

		// la camera ouverte
		VmbHandle_t cameraHandle;
		// frames for capture
		VmbFrame_t Frame[ROT];
		int width,height;
		string uniform; // where to display this image. ex: /main/A/tex
				// its /[winname]/[quadname]/[uniname]
		int64_t tickFrequency; // nb de tick par seconde du timestamp

		// is this camera using trigger software (/manual vs /auto)?
		// if so, then an incomplet frame is automatically re-triggered.
		//bool manual;


		camData(plugin<blob>::mystream &log);
		~camData();

		// retourne 0 si ok, <0 si probleme
		int selectCamera(string idref,string uniform);

		// retourn 0 si ok, <0 si erreur
		int openCamera(); // opened : false -> true
		int closeCamera(); // opened : true -> false

		int startCapture(pluginProsilica *plugin); // capturing : false -> true
		int stopCapture(); // capturing : true -> false

		// on peut aussi faire /set/<camid>/AcquisitionStart et AcquisitionStop directement
		int startAcquisition(); // acquiring: false -> true
		int stopAcquisition(); // acquiring: true -> false

		//int stopAcquisition()
		//int closeCamera()
	};


};





#endif

