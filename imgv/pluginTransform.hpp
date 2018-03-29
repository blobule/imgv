#ifndef IMGV_PLUGIN_TRANSFORM_HPP
#define IMGV_PLUGIN_TRANSFORM_HPP


/*!

\defgroup pluginTransform Transform plugin
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

### `/unpack [true|false]`

  This command activates or deactivates the transformation of the 12bit packed into 126bit unpacked.<br>

### `/bayer8planar [true|false]`

  This command activates or deactivates the transformation of the 8bit bayer format in pixel order into planar order (better for image compression).<br>

### `/bayer12planar [true|false]`

  This command activates or deactivates the transformation of the 12bit packed bayer format in pixel order into planar order (better for image compression).<br>

### `/bayer8pixel [true|false]`

  This command activates or deactivates the transformation of the 8bit bayer format in planar order into pixel order.<br>

### `/bayer8pixel [true|false]`

  This command activates or deactivates the transformation of the 12bit packed bayer format in planar order into pixel order.<br>

### `/avt-deactivate`

  Deactivates the transformation using the AVTImageTransform library.<br>

### `/avt-activate <avt_format_in> <avt_format_out> <avt_type_out>`

  Activates a transformation using the AVTImageTransform library.<br>
  _avt_format_in_ : integer that represents an input AVT image format. Possible values include: VmbPixelFormatMono8, VmbPixelFormatMono12, VmbPixelFormatMono14, VmbPixelFormatMono16, VmbPixelFormatBayerGB8, VmbPixelFormatBgr8, VmbPixelFormatBayerGB12, VmbPixelFormatRgb16, 
VmbPixelFormatBayerGB12Packed, VmbPixelFormatBgr8, VmbPixelFormatYuv411, VmbPixelFormatYuv422, VmbPixelFormatYuv444.<br>
  _avt_format_out_ : integer that represents an output AVT image format. Possible values are the same as for input. 
  _avt_type_out_: integer that represents an OpenCV pixel format for the output image, typical values are CV_8UC1,CV_8UC3,CV_16UC1,CV_16UC3.<br>

@}
*/



#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>

class pluginTransform : public plugin<blob> {
    private:
    bool initializing;
    bool unpack;
    bool bayer8planar;
    bool bayer12planar;
    bool bayer8pixel;
    bool bayer12pixel;
    bool avt;
    int32 avt_format_in;
    int32 avt_format_out;
    int32 avt_type_out;

    public:
	pluginTransform();
	~pluginTransform();

	void init();
	void uninit();
	bool loop();
	bool decode(const osc::ReceivedMessage &m);
	void dump(ofstream &file);
};


#endif

