#ifndef IMGV_PLUGIN_SAVE_HPP
#define IMGV_PLUGIN_SAVE_HPP



/*!

\defgroup pluginSave Save plugin
@{
\ingroup plugins

This plugin save all the images it receives into individual numbered files or into
a single video file.


Ports
-----

| Name | Direction | Description |
| ---- | --------- | ----------- |
| in   | in        | queue d'entrée des images + messages |
| out  | out       | queue de sortie des images |
| log  | out       | log output |

Events
------

none


OSC initialization-only commands
------------

### `/init-done`

  Signal the end of initialization of the plugin.<br>
  No image is ever processed until this command is received.



OSC commands
------------

### `/mode ["internal"|"id"|"nop"|"video"]`

  This command select the numbering scheme for output.<br>
  _internal_ : use an internal counter, which starts at 0.<br>
  _id_ : use the image number (from the image itself). This is ideal when images
         are received out of sequence.<br>
  _video_ : put all images into a single video file. Does not use mapView.<br>
  _nop_ : do not save the images. Let them pass through.<br>

### `/filename <filename>`

  Select the output filename. If using `jpg` or `png`, the output filename must contain
`%d` (or `%04d`) inside the filename to get incrementing numbers (example: `/filename out%04d.png`).<br>
  In _id_ mode, a second parameter is also available in the filename: the image timestamp as epoch in 1/100 of ms resolution.<br>

 *Note* : If you issue a `/filename` command and a video is currently output, it will close the current video and open a new one with the new filename.

### `/mapview <image view> <associated filename>`

  If you do not specify a filename (for modes _interne_ and _id_), you can associate
a filename with a specific view.<br>
  For example, if you have images with view /main/A/tex and others with /main/B/tex
(displayed in different quads), then you can save them with different filenames.<br>
The filenames should contain %d as required by the mode selected (_interne_ or _id).

### `/compression <int level>`

  Set the compression level. This will set the OpenCV flag CV_IMWRITE_PNG_COMPRESSION.<br>
By default, this flag is not set.

### `/count \<int nbimg\>'

  Indicate to save _nbimg_ images and then let all other images passthrough.<br>
  This will let any image be saved, regardless of the view they have.<br>
  Set to -1 to save all images without counting.

### `/count \<string view\> \<int nbimg\>'

  Indicate to save _nbimg_ images with the specific view, and then let them passthrough.<br>
  This will save only the image with correct view, and let any other passthrough.<br>
  Set to -1 to save all images without counting.<br>
  <del>Use the view "*" to reset the count of all views.</del>


Example 1
----------

Save all images to files out0000.png, out0001.png ...

~~~
/mode interne
/filename out%04d.png
/init-done
~~~

Example 2
----------

Save all images as a single video
~~~
/mode video
/filename capture.avi
/init-done
~~~



@}
*/




#include <imgv/imgv.hpp>

#include <imgv/plugin.hpp>


/// Save images to files (individual or video)

/// /mode interne|id|nop  -> count=le plugin compte les frames. id-> le plugin utilise l'id
///    -> donc si on genere des images dans un ordre quelquonque, le id est mieux.
/// /format png|jpg|video -> par exemple...
/// /filename toto%04d.png
///
/// -> le mode ID va auto mapper les views sur des entiers
///
/// evenement: EVENT_SAVED apres chaque image sauvegardee.
///

class pluginSave : public plugin<blob> {
    public:
	static const int EVENT_SAVED = -1;
	static const int SAVE_MODE_NOP = 0;
	static const int SAVE_MODE_INTERNAL = 1;
	static const int SAVE_MODE_ID = 2;
	static const int SAVE_MODE_VIDEO = 3; // output a video...
    private:
	int mode; // SAVE_MODE_*
	string filename;
	//cv::VideoWriter *video;  // NULL si pas d'output en cours
	FILE *F;
	char buf[500]; // pour le filename
	std::map<std::string,std::string> mapView; // map une view vers un filename (mode id)
	std::map<std::string,int> countView; // map une view vers un compteur
	int count; // for counting how many images to output
    int32 compression; //png compression
    int32 quality; //jpeg quality
    char *internal_buffer;

	std::map<std::string,int> indexView; // index interne selon les view (mode interne)
	int index; // image index, when in "internal" mode

	// pour la compression
	std::vector<int> flags;
	// initilizing : true=listen to port 2, falsse=listen to port 0
	bool initializing;
    public:
	pluginSave();
	~pluginSave();
	void init();
	void uninit();
	bool loop();
    private:
	bool decode(const osc::ReceivedMessage &m);
	void save_by_type(blob *i1);
};


#endif

