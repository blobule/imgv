#ifndef IMGV_PLUGIN_VIEWERGLFT_HPP
#define IMGV_PLUGIN_VIEWERGLFT_HPP


/*!

\defgroup pluginViewerGLFW ViewerGLFW plugin
@{
\ingroup plugins

This plugin provides display services.


Ports
-----

| Name | Direction | Description |
| ---- | --------- | ----------- |
| in   | in        | queue for input images and control messages |
| out  | out       | queue for output images |

Events
------

| Name  | Description |
| ----  | ----------- |
| mouse | mouse has moved. $x,$y,$xr,$yr set to mouse position |
| up | Key up |
| down | Key down |
| left | Key left |
| right | Key right |
| "A".."Z", "0".."9" | alpha numeric key |



OSC initialization-only commands
------------

### `/init-done`

  Signal the end of initialization of the plugin.<br>
  No image is ever processed until this command is received.



OSC initialization-only commands
---------------



OSC _anytime_ commands
---------------

### `/set/<windowName>/<quadName>/<uniformName>  <float|int value>`<br>

Set the _value_ of a specific _uniform_ in a specific _quad_ of a specific _window_.<br>
The value can be float or int.

### `/vec4/<windowName>/<quadName>/<uniformName>  <float v0> <float v1> <float v2> <float v3>`

Set the _value_ of a specific _uniform_ in a specific _quad_ of a specific _window_.<br>
The value is a 4-vector.


### `/mat4/<windowName>/<quadName>/<uniformName>  <float v0> ... <float v15>`

Set the _value_ of a specific _uniform_ in a specific _quad_ of a specific _window_.<br>
The value is a 4-matrix provided as 16 floats.


### `/new-window/<windowName> <int offx> <int offy> <int w> <int h> <bool decor> <string title>`

Create a new window named _windowName_. The widht and height are _w_ and _h_, and the window
is opened with an offset of _offx_ and _offy_ pixels. The windows decorations are controled with _decor_ and it is possible to set the window _title_.


### `/new-monitor/<windowName> [<string monitorName>]`

Create a new full screen window. Optionaly, the monitor name can be specified (see xrandr).

### `/new-quad/<windowName>/<quadName> <float x1> <float x2> <float y1> <float y2> <float z> <string shaderName> [<int clamping>]`

Create a new quad in the window _windowName_.<br>
The position of the quad (x1,x2,y1,y2) is given in relative coordinates (0 to 1).
To occupy the full window, simply use (x1,x2,y1,y2) = (0.,1.,0.,1.).<br>
The clamping parameter should be "clamp-to-border" (default) or "clamp-to-edge". These translate directly to GL_CLAMP_TO_BORDER and GL_CLAMP_TO_EDGE

### `/new-fbo/<windowName>/<fboName> <float x> <float y> <string textureName> <string shaderName> <int clamping>`

Create a new FBO of size (x,y).
The clamping parameter should be "clamp-to-border" (default) or "clamp-to-edge". These translate directly to GL_CLAMP_TO_BORDER and GL_CLAMP_TO_EDGE


### `/del-window/<windowName>`

Delete the specified window.

### `/del-quad/<windowName>/<quadName>`

Delete the specified quad.

### `/manual <bool v>`

### `/render`

### `/output <string drawName> <string portName>`




Example 1
-----------------

Typical window for displaying a single image. Use the view is _/main/A/tex_.

~~~~
/new-window/main 100 100 640 480 true "Main window"
/new-quad/main/A 0.0f 1.0f 0.0f 1.0f -10.0f "normal"
~~~~


Example 2
-----------------

Single window with two images, side by side. Use the views _/main/A/tex_ and _/main/B/tex_.

~~~~
/new-window/main 100 100 640 480 true "Main window"
/new-quad/main/A 0.0f 0.5f 0.0f 1.0f -10.0f "normal"
/new-quad/main/B 0.5f 1.0f 0.0f 1.0f -10.0f "normal"
~~~~

Example 3
------------

To change the value of an uniform in a specific shader, use:

~~~
/set/main/A/fade 0.5f
~~~


@}
*/







#include "imgv.hpp"
#include "plugin.hpp"
#include "config.hpp"

#include "viewerGLFW.hpp"


// arbitrairement fixe une limite sur le nb de UID pour l'auto-skip
#define NBUID	780

#define USE_AUTOSKIP

//
// un viewer glfw!
//
class pluginViewerGLFW : public plugin<blob>, GLFWViewerListener {
	blob **uidDisplayed; // [NBUID] (allocated on create to save the stack); // pour l'auto-skip
	// arguments
	//int offx,offy,w,h;
	//bool decor;
	//string wname;
	//string shader;
	bool kill; // ask to stop

	int32 noDrawSleep; // time to sleep when no draw

    bool manual;
    bool rendered;

	// internal
	int n; // pour l'instant, compte le nombre de outputs
	GLFWViewer V;
	// pour l'instant, une seule fenetre...
	//int wid0;
	//int qid0;
	//int uid0;
	//int tid0,tid1;
	bool started;

	// to decide wether to redraw (used in loop and decode)
	bool changed;

	// pour les statistiques
	double last;

	public:
	pluginViewerGLFW();

	// initialisation par une liste de messages osc
        pluginViewerGLFW(message &m);

	~pluginViewerGLFW();

  private:
	//
	// 
	//
	bool decode(const osc::ReceivedMessage &m);
    int output(std::string drawName,std::string portName);
    double timestamp(void);

	int clampDecode(const char *clamping);


  public:

	void init();
	void uninit();
	bool loop();

	//
	// fonction du GLFWViewerListener
	//
	void key(const char *wname,int k,double x,double y,int w,int h);
        void mouse(const char *wname,double x,double y,int w,int h);

	// on doit redefinir le stop...
	// attention, ceci est appelle du thread principal, pas celui du plugin
	// mais leavemainLoop est threadsafe
	void stop();

	void dump(ofstream &file);

};


#endif
