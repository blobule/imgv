#ifndef IMGUV_PLUGIN_DRIPMULTI_HPP
#define IMGUV_PLUGIN_DRIPMULTI_HPP

/*!

\defgroup pluginDripMulti DripMulti plugin
@{
\ingroup plugins

This plugins lets group of images go through at a specific rate. It will ensure that
all the image sources are synchronized before sending the images as a group.<br>
It supports starting at a
predefined time, pause, and various events to control fading, for exemple.<br>
This plugin must be used when intending to play a video at a controled speed.


Ports
-----

|      # | Name | Direction | Description |
| ------ | ---- | --------- | ----------- |
| 0      | in   | in        | queue for control only. |
| 1+     |      |           | input stream of images (see `/in`)<br> output stream of images (see `/out`)<br> duplicate stream of images (see `/dup`)  |

Events
------

| Name  | Description |
| ----  | ----------- |
| frame | a set of images has been received and sent out.<br> This event is triggered _after_ the images are sent. |

Additional user-defined events can be specified using specific (`/event`) commands.


OSC initialization-only commands
------------

(initialization commands are only receveid on port 0 'in')

### `/fps <double fps>`

  Set the framerate. This must be specified during the initialization.

### `/start <int time>`

  Start the output of images at the specified time.<br>
  A time with a small number (<1000000) indicates a time relative to "now"
in seconds. 
A large number indicate an absolute time in epoch format (in seconds).<br>
This command ends the initialization of the plugin.


OSC _after-initialization_ commands
------------

### `/reset`

Goes back into initializing mode. Might not work...

### `/fpsnow <double fps>`

  Change the fps for a new value.<br> This is done _now_, so there is no way to
synchronize multiple drip instances.<br>

### `/fps <double timeReference> <double fps>`

  Change the fps for a new value.<br> The _timeReference_ specifies the exact absolute time when the fps is changed. This allows the synchronization of the framerate change between sperate instances of the plugin.<br>Note that the command is executed when it is received, so
it should be queued with the images to take effect at the right time.

### `/timestamp-id <bool timestamp_id>`

  Set the timestamp id.

### `/pause <double timeReference>`

Stop (or restart) the flow of images.<br>This is essentialy equivalent as setting the fps to 0.<br>
The _timeReference_ specifies the exact absolute time when the flow is stopped, 
for synchronization purposes.<br>
Note that the command is executed when received, so it should be queued properly with the
images.

### `/stop`

Same as reset. Goes back to initialization


OSC _anytime_ commands
------------

### `/in <string portName> <string name>`

  Defines the specific _portName_ as a stream of incoming images and associate a _name_ to it.<br>
  The name will be used to select this stream for output (or duplication).

### `/offset <string name> <int offset>`

  Define the offset for input `name`.

### `/out <string name> <string portName> <string view>`

  Defines the specific _portName_ as an outgoing stream of images, and associate the input stream _name_ to it.<br>
  When an input image is sent, it is not available anymore so there should be only one `/out` for each `/in`.
  If the _view_ is empty, then it is not changed.

### `/dup <string name> <int portName> <string view>`

  Defines the specific _portName_ as an outgoing stream of duplicate images, and associate the input stream _name_ to it.<br>
  Duplicated images share the same data as the input image, so the shoul be used asr read-only.<br>
  If the _view_ is empty, then it is not changed.<br>
  Warning: Since a duplicate shares its data with another image, there is a possibility
  that this image is recycled and chance unexpectedly. Use at your own risk.

### `/sync <bool>`

  Specifies if images from all sources should be synchronized to the same frame number. Default is _true_.

### `/in/cancel <string name>`

  Stop using the image source with the provided _name_.

### `/out/cancel <string name>`

  Stop sending the image stream with the provided _name_.

### `/dup/cancel <string name>`

  Stop sending the image dulicates with the provided _name_.


### `/event/clear`

  Clear all events (actually, only the beginFrame and endFrame events).

### `/event/set/begin <string beginEventName> <string inputName> <int beginEventFrame>`

  Set the event _beginEventName_ to be triggered when the image _frame-since-begin_ is
between 0 and _beginEventFrame_, on the specified input _inputName_.<br>
This is used to generate fadein events.

### `/event/set/end <string endEventName> <string inputName> <int endEventFrame>`

  Set the event _endEventName_ to be triggered when the image _frame-until-end_ is
between _endEventFrame_ and 0, for the specified _input.<br>
This is used to generate fadeout events.

Example 1
-----------

Start playing a movie just like regular drip would do, except use port 1 to received the images and port 2 to send them.

~~~
-> reserve ports A,B
/in A video
/out B video /main/A/tex
/fps 30.0
/start 3
~~~

Example 2
-----------

Start playing 3 movies at the same time, in perfect sync.<br>
The movies are arriving on ports 1,2,3. The output is sent to port 4, and a
duplicate of each image is also sent to port 5.

~~~
-> Reserve ports A,B,C,D,E
/in A video1
/in B video2
/in C video3
/sync true
/out D video1
/out D video2
/out D video3
/dup E video1
/dup E video2
/dup E video3
/fps 30.0
/start 3
~~~


Sample Code
-----------

- ...

ToDo
----

- must add `/copy` or `/clone` to make many independent copies of images
- must add proper management of duplicate images. The problem is that a reference image can
  be recycled and suddenly all duplicates are unusable. The solution might be a "duplicate counter' in image... an image with >0 duplicates stays inside the recycling queue. When a duplicate is recycled (they are always destroyed, never recycled in a queue) the count of it "reference image" is decremented. That might work....

@}
*/


#include "imgv.hpp"
#include "plugin.hpp"

//
// utilisation des ports
//
class portinfo {
    public:
	std::string name;
	std::string portname;
	int port; // start at -1, set based on portname
	int in; // input number (0..in.size()-1) -1 = no input found for this out/dup (by copteMatch)
	int noffset; // frame number offset
    double timestamp_first;
	std::string view; // pour out et dup
	blob *img; // image just received (if this is an input)

	portinfo(std::string n,std::string p) { name=n;portname=p;in=-1;port=-1;noffset=0; }
	portinfo(std::string n,std::string p,std::string v) { name=n;portname=p;in=-1;view=v;port=-1;noffset=0; }
	bool match(portinfo &pi) { return name.compare(pi.name)==0; }
	bool match(const char *n) { return name.compare(n)==0; }
	bool match(std::string n) { return name.compare(n)==0; }
	void dump() { cout << " name='"<<name<<"' ,port="<<portname<<",in="<<in<<" view="<<view<<endl; }
};

class pluginDripMulti : public plugin<blob> {
	// arguments
	int startsec; // sec since epoch
	int startusec; // usec for starting time
	double fps;

	int n; // frame number
	double offset; // timer ajustment
	double origin; // time of frame #0. = startsec if fps is not changed.

	bool paused; // true if we are in pause.
	double pauseStart; // heure du debut de la pause (valide si paused=true)

	bool initializing; // true = only accept intializing commands
			   // until /init/done

	std::vector<portinfo> in; // match is not used here
	std::vector<portinfo> out; // match is post-computed
	std::vector<portinfo> dup; // match is post-computed
	bool sync; // synchronize all input frame numbers?

	bool timestamp_id; // true = use timestamp and fps to generate frame number

	//
	// events definitions...
	// events are at begin or end of video. They generate "fade" commands
	//
	std::string beginEventName;	// event name generated
	int32 beginEventFrame;	// frame for animating from 0. <0=do not use (done)
	std::string beginEventInputName;
	int32 beginEventInputNum; // computed from computeMatch()

	std::string endEventName;	// event name generated
	int32 endEventFrame;	// frame for animating from frame to 0 (left) <0=none
	std::string endEventInputName;
	int32 endEventInputNum; // computed from computeMatch()
	//

	// duplication
/*
	int duplicate; // output port for duplicate image (-1=no duplicate)
	string dupView1; // view to give the image
	string dupView2; // view to give the image
*/

	// statistiques
	// on garde la moyenne et dev standard, en ms
#ifdef STATS
	double sum;
	double sum2;
#endif
    private:
	void setStart(int start);
    public:
	pluginDripMulti();
	//pluginDripMulti(int start,double fps);
	//pluginDripMulti(message &m);
	~pluginDripMulti();

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

	// compute the match of ports between in/out/dup
	void computeMatch();
	void dumpPortInfo();
	void skipAllUntil(int target);
};


#endif

