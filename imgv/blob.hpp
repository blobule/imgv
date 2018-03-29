#ifndef BLOB_HPP
#define BLOB_HPP

#include "config.hpp"
#include "recyclable.hpp"

#include <opencv2/opencv.hpp>

#ifdef HAVE_LZ4
  #include <lz4.h>
  #include <lz4hc.h>
#endif


//
// type de base pour les plugins imgv
//

class blob : public recyclable, public cv::Mat {
	// on mettrait les tags (key/value) typiques de imgu
	// le timestamp, heure d'origine de l'image
    public:
    int n; // numero d'image. utilisable pour tout ce qui genere des sequences
	double timestamp; // si on utilise epoch, on a 10 digits pour les secondes
	double yaw,pitch;
			// et 5 digits pour les fractions (donc 0.01ms precision)

	// special pour les video et autre contenu a duree limitee... sum=length
	int frames_since_begin; // frame count, start at 0, increaase until video length
	int frames_until_end;  // frame count, start at video length, deacrese until 0
    bool frame_last; //true if this is the last blob for the current frame, useful in dripmulti 

    string  view; // where to display image? /winName/quadName/uniName

	//
	// key/text associated with image
	std::map<std::string,std::string> texts;

	// exemple qui ne servent a rien...
        //void set(int i ) { n=i; }
        //int get() { return n; }

        blob();
        blob(cv::Mat m);
        blob(const blob &b);
        blob(int xs,int ys,int type);
        ~blob();

	static blob pngread(const string &filename);
	static blob pngread(const string &filename, cv::FileStorage *fs);

	static bool pngwrite(const string &filename,const blob &img,int compress);
	static bool pngwrite(const string &filename,const blob &img,int compress,cv::FileStorage *fs); // will release the filestorage

    static blob pbmread(const string &filename);
    static blob pbmread(const string &filename, cv::FileStorage *fs);

    static bool pbmwrite(const string &filename,const blob &img);
    static bool pbmwrite(const string &filename,const blob &img, cv::FileStorage *fs);

    static blob lz4read(const string &filename,char *buf);
    static blob lz4read(const string &filename, cv::FileStorage *fs,char *buf);

    static bool lz4write(const string &filename,const blob &img,char *buf);
    static bool lz4write(const string &filename,const blob &img, cv::FileStorage *fs,char *buf);

    bool inBound(double x,double y);
    int interpolateBilinear(double x,double y,double *val);

	void dumpText();

};


#endif
