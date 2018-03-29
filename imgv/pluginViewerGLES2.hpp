#ifndef IMGV_PLUGIN_VIEWERGLES2_HPP
#define IMGV_PLUGIN_VIEWERGLES2_HPP


/*!

\defgroup pluginViewerGLES2 ViewerGLES2 plugin
@{
\ingroup plugins

This plugin provides display services for the raspberry pi.

The full screen is used, and multiple shaders are provided for display.
The first shader output is provided in the texture prev_layer for the next shaders, which are essentially effect shaders.
The last shader is rendered on the screen.

A number of predefined uniforms are available for the shaders. These are:
 - time
 - img
 - luthi
 - lutlow
 - prev_layer

Ports
-----

| Name | Direction | Description |
| ---- | --------- | ----------- |
| in   | in        | queue for input images and control messages |
| out  | out       | queue for output images |
| log  | out       | log output |

Events
------

none (for now)

OSC initialization-only commands
------------

### `/init-done`

  Signal the end of initialization of the plugin.<br>
  No image is ever processed until this command is received.

### `/add-layer <string shader>`

  Add a layer with a specific shader. For a shader named "blub", the file is
`/usr/share/imgv/shaders/blub.glgl`.
You must add at least on layer to display something on the screen.
Most shaders name for the raspberri pi start with `es2_`.


OSC after-initialization commands
---------------

### `/shader <int layer> <string shader>`

Change the shader for a specific layer (0,1,...).


Example 1
-----------------

Typical commands to display a simple image.

~~~~
/add-layer "es2-simple"
/init-done
~~~~


Example 2
-----------------

Typical commands to display a tunnel animation, which is then distorted by a lookup table.

~~~~
/add-layer "es2-simple"
/add-layer "es2-effect-lut"
/init-done
~~~~

@}
*/






#define __STDC_FORMAT_MACROS

#include "imgv.hpp"
#include "plugin.hpp"
#include "config.hpp"

extern "C" {
#include "pj.h"
#include "videoPlayer.h"
#include "GLES/gl.h"

#include "videoDecode.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
}

//
// un viewer GLES2
//
class pluginViewerGLES2 : public plugin<blob> {

	// internal
	int n; // pour l'instant, compte le nombre de outputs

	public:
	pluginViewerGLES2();

	// initialisation par une liste de messages osc
        pluginViewerGLES2(message &m);

	~pluginViewerGLES2();

  private:
	//
	// 
	//
	bool decode(const osc::ReceivedMessage &m);
    double timestamp(void);

	PJContext *pj;

	// keep names of active shaders
	std::vector<std::string> allShaders;

        //Pour le video
	threadArguments gArgs;
	string fileNameString; //pour préserver le nom du fichier.
	pthread_t video_thread;
	bool initialized;

	void convertLut16ToLut8(blob &i1,blob &i2,int hi);

	void init_textures(EGLContext context, EGLDisplay display, GLuint tex);


  public:

	void init();
	void uninit();
	bool loop();

	// on doit redefinir le stop...
	// attention, ceci est appelle du thread principal, pas celui du plugin
	// mais leavemainLoop est threadsafe
	void stop();

	void dump(ofstream &file);

};


#endif
