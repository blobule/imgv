#ifndef IMGV_PLUGIN_PTGREY_HPP
#define IMGV_PLUGIN_PTGREY_HPP


/*!

\defgroup pluginPtGrey PtGrey plugin
@{
\ingroup plugins

PtGrey plugin for Fly PtGrey cameras.<br>

Ports
-----

| Name | Direction | Description |
| ---- | --------- | ----------- |
| in   | in        | queue for input recycled images + control messages |
| out  | out       | queue for image output |
| log  | out       | log output |

OSC commands
------------

### `/file <string pattern>`

  Set the filename pattern.
  Insert %d to replace by the current frame number.<br>
  Example: /file toto%3d.png
  will generate toto000.png, toto001.png, ...

### `/view <string view>`

  Set the view of the output images.<br>
  This eventualy controls where the image will be displayed.

### `/min <int min>`

  Start number for file name. Default to 0.

### `/max <int max>`

  End number for file name. Default to -1 (no end).

### `/internal-buffer <int32 size>`

  Set the internal read buffer. By default 16mb (16000000).

### `/reinit`

  Reset image count to 0. Back to initialization mode.

### `/init-done`

  End of initialization. Start reading images with current parameters.

### `/play`

  Start reading the images if reading is paused.

### `/pause <int32 apply_at>`

  Temporarily stop reading images. The pause will only stop the reading if the
  count is equal or larger than apply_at.<br>
  To pause regardless of count, just send /pause 0.

### `/count-add <int32 apply_at> <int32 apply-offset>`

  Adjust the image count by apply-offset, but only if the count is equal or larger than
  apply_at. To jump 10 frame regarless of count, use /count-add 0 10.

### `/next`

  PtGrey the next frame, if the reading is paused.

### `/convert-to-gray`

  Convert each image to gray

### `/no-conversion`

  Stop any image conversion (this is the default)

@}
*/

#include <flycapture/FlyCapture2.h>

#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>

class pluginPtGrey : public plugin<blob> {
    private:
    bool initializing;

    FlyCapture2::Error error;
    FlyCapture2::Camera cam;
    FlyCapture2::Image rawImage;
    FlyCapture2::Image convImage;

	int n;
	int32 camnum;
	string view;

    public:
	pluginPtGrey();
	~pluginPtGrey();

	void init();
	void uninit();
	bool loop();
	bool decode(const osc::ReceivedMessage &m);
	void dump(ofstream &file);

	void startCamera();
};


#endif

