#ifndef IMGV_PLUGIN_HOMOG_HPP
#define IMGV_PLUGIN_HOMOG_HPP



/*!

\defgroup pluginHomog Homog plugin
@{
\ingroup plugins

This plugin generates deformation lookup tables annd matrices.

The 3x3 matrix is of this form:
~~~
/homography m0 m1 m2 .. m8   (not done yet...)
~~~
and the lookup image is a 1024x1024 16bit RGB image where
- R is the x position
- G is the y position
- B is a mask (65535=inside the image, 0=outside)


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
| -1   | saved | Image was saved (not operational) |




OSC commands
------------

### `/set/p <int k> <float x> <float y>`

Set the point of control point _p_, with _k_ set from 0 to 3.<br>
These are the source points and usually set to the unit square.

### `/set/q <int k> <float x> <float y>`

Set the point of control point _q_, with _k_ set from 0 to 3.<br>
These are the destination points and usually set to the unit square.

### `/set/out-size <int w> <int h>

Set the size of the ouput lookup table.

### `/up`, `/down`, `/left`, `/right`

Move the last _q_ point by a small value.

### `/go <string view>`
### `/go`

Generate an homography deformation lookup table from the current value of parameters p and q.

### `/set/view <string v>`

Set the view of the output lookup table.


ToDo
----------
- ajouter la commande `/homography`



@}
*/


//
// homog plugin
//
// fabrique une carte de deformation representant une homographie...
//
//
//


#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>


class pluginHomog : public plugin<blob> {
    // Homog arguments

    class hInfo {
    public:
        std::vector<cv::Point2d> source;
        std::vector<cv::Point2d> dest;
        std::string view; // name of the view for output image (can be empty)
        std::string matrixCmd; // by default /homography/<tag> mais ca peut changer avec /set/out/matrix-cmd
	// on peut definir plusieurs commandes... on clear avec /clear/out/matrix-cmd
        int outw,outh; // size of output image
        bool useLookup; // shoud we make a lookup table?
        bool useMatrix; // shoudl we send the homography

        hInfo() {
            source.resize(4);
            source[0]=cv::Point2d(0.0,0.0);
            source[1]=cv::Point2d(1.0,0.0);
            source[2]=cv::Point2d(1.0,1.0);
            source[3]=cv::Point2d(0.0,1.0);

            dest.resize(4);
            dest[0]=cv::Point2d(0.0,0.0);
            dest[1]=cv::Point2d(1.0,0.0);
            dest[2]=cv::Point2d(1.0,1.0);
            dest[3]=cv::Point2d(0.0,1.0);

            //pw=1;ph=1;
            outw=512;outh=512;
            useLookup=true;
            useMatrix=false;
            view="";
            matrixCmd="";
        }
    };

    // on a des noms pour les homographies
    map<std::string,hInfo> z;

    int n; // frame sequence number

    std::string lastTag; // tag of the last homography point command
    int lastDest; // -1 is none, othwerise 0..3

    // default tag, used when no tag is specified in commands
    string currentTag;

public:
    pluginHomog();
    ~pluginHomog();

    void init();
    void uninit();
    bool loop();
    bool decode(const osc::ReceivedMessage &m);
    void dump(ofstream &file);

private:
    //void solveHomog();
    void go(std::string tmpview,const char *tmp=NULL);
    void doOnePattern(hInfo &hi,cv::Mat *m,cv::Matx33d &m33);
    //void getCheckerBoard(int n,cv::Mat &m);

};


#endif

