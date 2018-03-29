#ifndef IMGV_PLUGIN_SYNC_HPP
#define IMGV_PLUGIN_SYNC_HPP


/*!

\defgroup pluginSync Sync plugin
@{
\ingroup plugins

This plugin lets images go through while generating messages, with an optional delay,
to trigger various functions in other plugins.<br>

Every image received in port 0 is sent to port 1 while generating an event "no-delay".
After an optional delay, an event "delay" is triggered.

The output can also be distributed across many ports, using `/distribute`. In that case,
the image is sent to the queue with the lowest number of images. By default, the
plugin effectively assumes `/distribute out` (only a single output port, "out").

Note that the delay is effectively setting the maximum fps of the plugin at 1/delay, since
images are received and processed sequentially.

It is possible to control when the image go through using the `/manual` command and then `/go` for each image.

Ports
-----

| Name | Direction | Description |
| ---- | --------- | ----------- |
| in   | in        | input queue of images and control messages |
| out  | out       | output queue of images |
| log  | log       | Log queue |

Events
------

| Name | Description |
| ---- | ----------- |
| no-delay | An image was received and was sent to the output port. |
| delay | The delay after an received image has passed. |


OSC commands
------------

### `/go`

  Let one image through from the in port to the out port.

### `/count <int togo>`

  Set the number of image that should go through. In that case the plugin is done.<br>
  The value 0 mean that no counting is done, which is the default.

### `/manual`

   Specifies that image should only be processed after receiving a `/go` command.<br>

### `/auto`

	Specifies that image can go through as soon as they ar received. This is the default.

### `/delay <float val>`

    Specifies a delay (in seconds) after which an event "delay" is triggered.<br>
  Note that when a delay is used, no new images will be processed until the delay has
  past.<br>
  This can be useful to trigger a photo capture after an image has been sent to the
  display.<br>
  A negative delay indicates that no delay is used, and "delay" is not triggered.

### `/distribute <string port1> <string port2> ... <string port-n>`

  Indicate that the output is distributed to the specified ports. Each received image
is put into the queue with the smallest number of elements

@}
*/



#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>


class pluginSync : public plugin<blob> {
    public:
	static const int EVENT_NO_DELAY = -1;
	static const int EVENT_DELAY = -2;
    private:
	double delay; // en secondes, avant d'envoyer le sync
	double offset;
	bool manual; // true: we have to wait for /go to send an image.
	int nbGo; // number of /go received. can be >1
	int32 togo; // number of images to let pass, then die. (0=no limit) use /count
	int n; // image counter
    bool send_events;

	// the output ports
	int nbOut;
	std::string outputNames[100]; // [nbout] "out"
	int outputNums[100]; // [nbout] -1 if unknown

    public:
	pluginSync();
	~pluginSync();
	void init();
	void uninit();
	bool loop();

	//
	// decodage de messages a l'initialisation (init=true)
	// ou pendant l'execution du plugin (init=false)
	// (dans messageDecoder)
	//
    private:
	bool decode(const osc::ReceivedMessage &m);
};


#endif

