#ifndef VIEWERGLFW_HPP
#define VIEWERGLFW_HPP

//
// viewer glfw
//
// ajoute une fenetre (position, fullscreen etc...)
//     ajoute un quad (position dans la fenetre w=0..1 h=0..1, avec un shader
//         et les uniformes sampler sont disponible pour recevoir des images
//         et les autres uniformes sont la pour changer par parametre
//
// on va avoir des id de fenetre (glutwindowget/set)
// on va avoir des noms d'uniformes
//


// en premier!
#include <GL/glew.h>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>



#include <math.h>
#include <stdio.h>

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <imgv/parseShader.hpp>

using namespace std;

//#undef HAVE_OCULUS

#ifdef HAVE_OCULUS
  #ifdef WIN32
    #include <windef.h>
    #include <windows.h>	
    #define GLFW_EXPOSE_NATIVE_WIN32
    #define GLFW_EXPOSE_NATIVE_WGL
    #define OVR_OS_WIN32
  #else
    #define GLFW_EXPOSE_NATIVE_X11
    #define GLFW_EXPOSE_NATIVE_GLX
    #define OVR_OS_LINUX
  #endif
  #include <GLFW/glfw3.h>
  #include <GLFW/glfw3native.h>

  #include <OVR.h>
  #include <OVR_CAPI_GL.h>
  using namespace OVR;
#endif

/*  Function Prototypes  */

class GLFWViewer;

class GLFWViewerListener {
	public:
	virtual void key(const char *wname,int k,double x,double y,int w,int h) { cout << "KEY!!! win="<<wname<<" ("<<k<<")"<<endl; }
	virtual void mouse(const char *wname,double x,double y,int w=1,int h=1) { cout << "mouse!!! win="<<wname<<"("<<x<<","<<y<<")"<<endl; }
};

#define DRAW_TYPE_QUAD	0
#define DRAW_TYPE_FBO	1

#define UNIF_TYPE_TEX	0
#define UNIF_TYPE_FLOAT	1
#define UNIF_TYPE_INT	2
#define UNIF_TYPE_VEC4	3
#define UNIF_TYPE_MAT4	5
#define UNIF_TYPE_MAT3	6

#define MAX_NB_BUFFERS 8

class GLFWViewer {

class window {
	public:
	bool deleted;
	bool hidden; // if hideWindow() was used... 
	string name;
	GLFWwindow *window; // NULL si deleted
	
	bool hmd_window;
#ifdef HAVE_OCULUS
	ovrHmd hmd;
	float hmd_fov_tan_left;
	float hmd_fov_tan_right;
	float hmd_fov_tan_up;
	float hmd_fov_tan_down;
	ovrPosef hmd_pose[2];
	ovrFovPort hmd_eyefov[2];
	ovrEyeRenderDesc hmd_eyerender_desc[2];
    ovrGLConfig hmd_cfg;
#endif
};

class drawable {
	public:
	bool deleted;
	int type; // DRAW_TYPE_QUAD, DRAW_TYPE_FBO
	string name;
	int wid; // la fenetre de ce quad
	float x1,x2,y1,y2,z; // for quad
	int xres,yres; // for fbo
    int nbbytes; //number of bytes per color in fbo (usually 1 or 2 for 16-bit precision)
    int clamping;
	string texname; // this can be use to identify a fbo inside a shader
	string shader;
	GLuint shaderProgram;

        int nbdraws; //number of times to draw the drawable, negative number means infinite number

        int nbbuffers;
        GLuint fbo[MAX_NB_BUFFERS]; // the frame buffer object
        GLuint fbo_tex[MAX_NB_BUFFERS];
        //GLuint fbo_depth[MAX_NB_BUFFERS];
	
	bool hmd_texture;
};

class uniform {
	public:
	bool deleted;
	int type; // 0=sampler 1=float 2=int   UNI_TYPE_TEX , _FLOAT, _INT 
	int did; // dans quel drawable?
	string name;
	GLint location;  // location of the uniform in the shader
	GLuint texNum; // texture associee (si c'est un sampler)
	GLfloat valf; // valeur si float
	GLint vali; // valeur si int
	GLfloat valp[16];
	int redirect; // uid of the redirected uniform. If this is >=0, we are using another uniform 
		// instead of this one. This allows texture sharing between quads, even acros windows.
		// use the addRedirect("/main/A/tex","/main/B/tex") to use B/tex instead of A/tex
		// redirection is only used when drawing. If you send texture data to this uniform,
		// it will be sent, but never used when drawing, unless some other uniform redirects to you.
		// this is valid to share any uniform, not only texture
};

class text {
	public:
	int wid; // window where to draw text
	float x,y; // position
	float color[3];
	string text;
};


  private:
//
// un viewer glut multi-window et multi-quad
// le viewer va devoir etre global.... :-(
//
  vector<window> windows; // les id des fenetres ouvertes
  vector<drawable> drawables; // liste de tous les drawables
  vector<uniform> unifs; // tous les uniforms
  vector<text> texts; // bunch of texts to display

  //aliases for uniform names of fbos
  std::map<std::string,std::string> aliases;

  GLFWViewerListener *listener; // pour les callback
//  void (*idlef)(void *);
//  void (*keyboardf)(void *,unsigned char,int,int);
//  void (*mousef)(void *,int,int,int);
//  void *userData;

  FILE *motion_file;

  public:
  GLFWViewer();
  ~GLFWViewer();
  static void init();

  // retourne la position de la souris
  void getMouse(int wid,double &xpos,double &ypos,int &width,int &height) {
	glfwGetCursorPos(windows[wid].window,&xpos,&ypos);
	glfwGetWindowSize(windows[wid].window, &width, &height);
  }


  //
  // retourne un window id libre
  //
  int findFreeWindow();
  int findFreeDrawable();
  int findFreeUnif();

  int findWindow(string winName); // return wid, -1 si pas trouve
  int findDrawable(string winName,string drawName); // return did, -1 si pas trouve
  int findUnif(string winName,string drawName,string uniName); // return uid, -1 si pas trouve
  int findUnif(int did,string uniName); // return uid, -1 si pas trouve


  //
  // special: ajoute une fenetre fullscreen pour un moniteur
  // -> donner le nom du moniteur (voir xrandr). Pas de nom => current
  //
  int addMonitor(string winName,string monitorName="");


  //
  // ajoute une fenetre
  // retourne le wid (window id) numerique
  //
  int addWindow(string winName,int offx,int offy,int w,int h,bool deco,string title);
  int hideWindow(string winName,bool status);

  //
  // ajoute un quad
  //  un quad : x1,x2,y1,y2 (0 a 1) , + shader -> retourne un did
  //  -> apres on va extraire tous les uniformes
  //     (ou appeller adduniform(wid,did,"nom") -> on va trouver la localisation
  //     ensuite on peut fare un setup genre setuniform("nom",image), ou ,float, int etc...
  //
  //
  int addQuad(string winName,string quadName,float x1,float x2,float y1,float y2,float z,string shader,int clamping);
  int addFbo(string winName,string fboName,int xres,int yres,string texname,int nbbuffers,string shader,int clamping);
private:
  int addQuad(int wid,string quadName,float x1,float x2,float y1,float y2,float z,string shader,int clamping);
  int addFbo(int wid,string fboName,int xres,int yres,string texname,int nbbytes,string shader,int clamping);
public:

  //
  // on doit pouvoir enlever des items...
  //
  int delWindow(string winName);
  int delDrawable(string winName,string drawName);

  //
  // connexion avec les commandes

  // normalement on detecte automatiquement les uniformes dans le shader
  // ces fonctions sont appelle a l'interne
  private:
  int addUniformSampler(int did,string name);
  int addUniformFloat(int did,string name,float init);
  int addUniformInt(int did,string name,int init);
  int addUniformVec4(int did,string name,float *init);
  int addUniformMat3(int did,string name,float *init);
  int addUniformMat4(int did,string name,float *init);
  int bindUniforms(int did);
  int bindFbos(int did, int unit,int buffer);
  void addUniforms(int did,imgv::parseShader *ps);

  public:

  // trouve par nom (on utilise winname et drawname parce que drawname n'est pas unique)
  void setUniformData(string winName,string drawName,string uniName,cv::Mat image);
  void setUniformData(string winName,string drawName,string uniName,float val);
  void setUniformData(string winName,string drawName,string uniName,int val);
  void setUniformData(string winName,string drawName,string uniName,float *val,unsigned int len);
  // ...

  // special pour image seulement... "use first uniform compatible with image"
  void setUniformData(cv::Mat image);
  int getUniformFromView(string view); // retourne le uid de /win/drawable/uni (-1 si err)
  int getDrawableFromView(string view); // retourne le did de /win/drawable (-1 si err)

  float getUniformFloatValue(int uid);

  string getDrawableView(int did);
  string getUniformView(int uid);
  void addAlias(string uniName,string fboName);
  void setNbDraws(string fboName,int nbdraws);

  void readDrawableFbo(int did,cv::Mat *image);
  
  void setHmdTexture(int did,bool b);

  void setRedirect(string uniNameFrom,string uniNameTo); // the uniNameFrom will now refer to uniNameFrom
  void unsetRedirect(string uniNameFrom); // the uniNameFrom will now refer to itself

  private:
  // si on ne sait que le nom de l'uniforme
  void setUniformData(int did,string name,cv::Mat image);
  void setUniformData(int did,string name,float val);
  void setUniformData(int did,string name,int val);
  void setUniformData(int did,string name,float *val,unsigned int len);
  // ...

  void setUniformData(int uid,float val);
  void setUniformData(int uid,int val);
  void setUniformData(int uid,float *val,unsigned int len);
  public:
  void setUniformData(int uid,cv::Mat image); // special pour viewer plugin

/*
  int addText(int wid,float x,float y,float color[3],string txt);
  void setText(int tid,float x,float y,float color[3],string txt);
  void setText(int tid,string txt);
  void setText(int tid,float x,float y);
  void setText(int tid,float color[3]);
*/

  //void addMenu();

  void start(); // initialise to empty
  void stop(); // kill all windows.

  inline void setListener(GLFWViewerListener *VL) { listener=VL; }

  //inline void setCallbackUserData(void *data) { userData=data; }
  //inline void setIdleCallback(void (*f)(void *)) { idlef=f; }
  //inline void setKeyboardCallback(void (*f)(void*,unsigned char,int,int)) { keyboardf=f; }
  //inline void setMouseCallback(void (*f)(void*,int,int,int)) { mousef=f; }

  bool drawAll();
  bool draw(int wid);

  private:

  // pour glew
  bool glewInitialized;
  int OpenGLVersion[2];


  //
  // callbacks de glfw
  //
  static void resize_callback( GLFWwindow *window, GLsizei width, GLsizei height );
  static void keyboard_callback( GLFWwindow *window,  int key, int scancode, int action, int mods);
  static void mouse_callback( GLFWwindow *window,  double x,double y);
  static void error_callback( int code, const char *msg);
  
  //static void mouse(int x, int y);
  //static void idle();

  //
  // quelques fonctions de support
  //

static void initTexture2D( int texNum, cv::Mat img, int clamping );
static void initMipMap2D( int texNum, cv::Mat img, unsigned char lod, int clamping );

// loadFile - loads text file into char* fname
// allocates memory - so need to delete after use
// size of file returned in fSize
static std::string loadFile(string fname);

// printShaderInfoLog
// From OpenGL Shading Language 3rd Edition, p215-216
// Display (hopefully) useful error messages if shader fails to compile
static void printShaderInfoLog(GLint shader);

// 0 if ok, sinon -1
// not static since it will call the addUniforms as needed
static imgv::parseShader *LoadShader(string pfilePath_vs, string pfilePath_fs, GLuint &shaderProgram);

//
//
//

static void RenderString(float x, float y, void *font, const char* str, float rgb[3]);
static void RenderString(text &t);


};

//
//
//



#endif
