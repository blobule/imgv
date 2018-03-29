#ifndef IMGUV_PLUGIN_DRIP_HPP
#define IMGUV_PLUGIN_DRIP_HPP


/*!

\defgroup pluginDrip Drip plugin
@{
\ingroup plugins

This plugins lets images go through at a specific rate. It supports starting at a
predefined time, pause, and various events to control fading, for exemple.<br>
This plugin must be used when intending to play a video at a controled speed.


Ports
-----

|      # | Name | Direction | Description |
| ------ | ---- | --------- | ----------- |
| 0      | in   | in        | queue d'entrée des images + messages |
| 1      | out  | out       | queue de sortie des images |

Events
------

| #    | Name  | Description |
| ---- | ----  | ----------- |
| none |       | no specific event by default |

Events can be defined using specific (`/event`) commands.


OSC initialization-only commands
------------

### `/fps <doulbe fps>`

  Set the framerate. This must be specified during the initialization.

### `/start <int time>`

  Start the output of images at the specified time.<br>
  A time with a small number (<1000000) indicates a time relative to "now"
in seconds. 
A large number indicate an absolute time in epoch format (in seconds).<br>
This command ends the initialization of the plugin.

### `/predisplay-first`

  Display the first image immediately. All the other images will be displayed at their scheduled time.
  The default behavior is not to predisplay.

OSC _after-initialization_ commands
------------


### `/fpsnow <double fps>`

  Change the fps for a new value.<br> This is done _now_, so there is no way to
synchronize multiple drip instances.<br>

### `/fps <double timeReference> <double fps>`

  Change the fps for a new value.<br> The _timeReference_ specifies the exact absolute time when the fps is changed. This allows the synchronization of the framerate change between sperate instances of the plugin.<br>Note that the command is executed when it is received, so
it should be queued with the images to take effect at the right time.


### `/pause <double timeReference>`

Stop (or restart) the flow of images.<br>This is essentialy equivalent as setting the fps to 0.<br>
The _timeReference_ specifies the exact absolute time when the flow is stopped, 
for synchronization purposes.<br>
Note that the command is executed when received, so it should be queued properly with the
images.



OSC _anytime_ commands
------------

### `/debug <bool T|F>`

  Turn debugging on/off.

### `/event/clear`

  Clear all events (actually, only the beginFrame and endFrame events).

### `/event/set/begin <string beginEventName> <int beginEventFrame>`

  Set the event _beginEventName_ to be triggered when the image _frame-since-begin_ is
between 0 and _beginEventFrame_.<br>
This is used to generate fadein events.

### `/event/set/end <string endEventName> <int endEventFrame>`

  Set the event _endEventName_ to be triggered when the image _frame-until-end_ is
between _endEventFrame_ and 0.<br>
This is used to generate fadeout events.

### `/dup <portName> <view>`

  Ask for a duplicate of each image on the specified _portName_ and set a specific _view_. <br>
 Attention: the duplicated image is sharing its data with the original image. No data copying is performed, so it is suggested to use the duplicate image as read-only.

  Only one dup is allowed at any time (for now).

### `/nodup`

  Stop duplication of images setup by `/dup`.

Example 1
-----------

Start playing a movie in 3 seconds, at 30 images per second.

~~~
/fps 30.0
/start 3
~~~

Example 2
-----------

Setup fadein and fadeout of 30 images at the start and end of the movie.<br>
During the beginning 30 frames of the movie, an event -100 will be generated with $xr set to
values interpolated between 0 and 1.<br>
During the ending 30 frames of the movie, an event -101 will be generated with $xr set to
values interpolated between 0 and 1.<br>

~~~
/devevent -100 2 "/set/main/A/fade" "$xr"
/devevent -101 2 "/set/main/A/fade" "$xr"
/event/set/begin -100 30
/event/set/end -101 30
~~~


Sample Code
-----------

- \ref playMovie

@}
*/


#include "imgv.hpp"


#include "plugin.hpp"



class pluginDrip : public plugin<blob> {
	// arguments
	int startsec; // sec since epoch
	int startusec; // usec for starting time
	double fps;

	int n; // frame number
	int32 count; // number of frame to go through. After that, back to initialized
	double offset; // timer ajustment
	double origin; // time of frame #0. = startsec if fps is not changed.

	bool paused; // true if we are in pause.
	double pauseStart; // heure du debut de la pause (valide si paused=true)

	bool initializing; // true = only accept intializing commands
			   // until /init/done

	bool flush; // try to flush any image in the input queue until empty

	bool predisplay; // display first frame now instead of at scheduled time

	bool debug; // debuging messages

	//
	// events definitions...
	// events are at begin or end of video. They generate "fade" commands
	//
	std::string beginEventName;	// event generated
	int32 beginEventFrame;	// frame for animating from 0. <0=do not use (done)
	std::string endEventName;	// event number generated
	int32 endEventFrame;	// frame for animating from frame to 0 (left) <0=none
	//

	// duplication
	int duplicate; // output port for duplicate image (-1=no duplicate)
	string dupView1; // view to give the image
	string dupView2; // view to give the image

	// statistiques
	// on garde la moyenne et dev standard, en ms
#ifdef STATS
	double sum;
	double sum2;
#endif
    private:
	void setStart(int start);
    public:
	pluginDrip();
	//pluginDrip(int start,double fps);
	//pluginDrip(message &m);
	~pluginDrip();

	void init();
	void uninit();
	bool loop();

	//
	// decodage de messages a l'initialisation (init=true)
	// ou pendant l'execution du plugin (init=false)
	// (dans messageDecoder)
	//
	bool decode(const osc::ReceivedMessage &m);

	void dump(ofstream &file);

  private:
	// reset fps
	void resetFps(double remotenow,double newfps);

	// wait for frame n, based on current time, startsec, startusec and fps
	// ajust offset as you go.
	// retourne le timestamp
	double waitForFrame();
};


#endif

