#ifndef IMGV_PLUGIN_READ_HPP
#define IMGV_PLUGIN_READ_HPP


/*!

\defgroup pluginRead Read plugin
@{
\ingroup plugins

Read image files.<br>

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

  Read the next frame, if the reading is paused.

### `/convert-to-gray`

  Convert each image to gray

### `/no-conversion`

  Stop any image conversion (this is the default)

@}
*/



#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>

class pluginRead : public plugin<blob> {
    private:
    bool initializing;
	string file_pattern;
    bool pause;
    bool read_one_frame;
	int count; // frame count
	int32 apply_at; //variables to apply count changes
	int32 apply_offset;
	int32 noffset;
    int32 nmin,nmax; //min/max frame number
    bool convertToGray;
	string view;
    char *internal_buffer;

    public:
	pluginRead();
	~pluginRead();

    int getCounter();
    bool isPaused();

	void init();
	void uninit();
	bool loop();
	bool decode(const osc::ReceivedMessage &m);
	void dump(ofstream &file);
};


#endif

