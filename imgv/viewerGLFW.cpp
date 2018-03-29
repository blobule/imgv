
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

#include "config.hpp"

#include "viewerGLFW.hpp"

#include <oscutils.hpp>


#ifdef HAVE_PROFILOMETRE
        //#define USE_PROFILOMETRE
        #include <profilometre.h>
#endif


using namespace std;

//
// pointeur global sur le viewer actif.
// c'est ce pointeur qui sert dans les fonctions de glut
// on le definit par "start"
//
//GLFWViewer *gv;

#define MAX_UNIFORMS_PER_SHADER 128

//
// definitions
//

GLFWViewer::GLFWViewer() {
    //cout << "new viewerGLFW"<<endl;
    glewInitialized=false;
    listener=NULL;

    motion_file=fopen("view-motion.txt","r");
    if (motion_file!=NULL)
    {
      printf("Motion file opened.\n");
    }

    //idlef=NULL;
    //keyboardf=NULL;
    //mousef=NULL;
    //userData=NULL;
}

GLFWViewer::~GLFWViewer() {
    //cout << "delete viewerGLFW"<<endl;
    if (motion_file!=NULL) fclose(motion_file);
}


// static
void GLFWViewer::init() {


    // to get feedback on errors
    glfwSetErrorCallback(error_callback);

    if( !glfwInit() ) { cout << "glfwfInit!!!"<< endl; exit(64); }

    //
    // monitor stuff
    // (on pourrait avoir une fonction: /new-monitor/[winname] eDP1
    //

    int count;
    GLFWmonitor **monitors=glfwGetMonitors(&count);
    //cout << "[viewer] INIT got "<<count<<" monitors"<<endl;

    for(int i=0;i<count;i++) {
        const char *name=glfwGetMonitorName(monitors[i]);
        int wmm,hmm;
        int px,py;
        glfwGetMonitorPhysicalSize(monitors[i],&wmm,&hmm);
        glfwGetMonitorPos(monitors[i],&px,&py);
        //cout << "monitor "<<i<<" is "<<name<<" physical size "<<wmm<<","<<hmm<<"  position "<<px<<","<<py<<endl;

        /**
        int n;
        const GLFWvidmode *vms=glfwGetVideoModes(monitors[i],&n);
        for(int j=0;j<n;j++) {
            cout << "  video mode "<<j<<": ("<<vms[j].width<<"x"<<vms[i].height<<")"<<endl;
        }
        const GLFWvidmode *vmc=glfwGetVideoMode(monitors[i]);
        cout << "  current is ("<<vmc->width<<"x"<<vmc->height<<")"<<endl;
        **/
    }


    const GLFWvidmode *vm=glfwGetVideoMode(glfwGetPrimaryMonitor());
    //cout << "  current monitor is ("<<vm->width<<"x"<<vm->height<<")"<<endl;
}

//
// retourne un window id libre
//
int GLFWViewer::findFreeWindow() {
    int wid;
    for(wid=0;wid<windows.size();wid++) if( windows[wid].deleted ) break;
    if( wid==windows.size() ) {
        window b;
        b.deleted=true;
        b.window=NULL;
        windows.push_back(b);
    }
    return(wid);
}
int GLFWViewer::findWindow(string winName) {
    int wid;
    for(wid=0;wid<windows.size();wid++) {
        if( windows[wid].deleted ) continue;
        if( windows[wid].name==winName ) return(wid);
    }
    return(-1);
}
int GLFWViewer::findDrawable(string winName,string drawName) {
    int wid=findWindow(winName);
    if( wid<0 ) return(-1);
    int did;
    for(did=0;did<drawables.size();did++) {
        if( drawables[did].deleted ) continue;
        if( drawables[did].name==drawName && drawables[did].wid==wid ) return(did);
    }
    return(-1);
}

int GLFWViewer::findFreeDrawable() {
    int did;
    for(did=0;did<drawables.size();did++) if( drawables[did].deleted ) break;
    if( did==drawables.size() ) {
        drawable d;
        d.deleted=true;
        drawables.push_back(d);
    }
    return(did);
}

int GLFWViewer::findFreeUnif() {
    int uid;
    for(uid=0;uid<unifs.size();uid++) if( unifs[uid].deleted ) break;
    if( uid==unifs.size() ) {
        uniform b;
        b.deleted=true;
        unifs.push_back(b);
    }
    return(uid);
}
int GLFWViewer::findUnif(string winName,string drawName,string uniName) {
    int wid=findWindow(winName);
    if( wid>=0 ) {
        int uid;
        for(uid=0;uid<unifs.size();uid++) {
            if( unifs[uid].deleted ) continue;
            int did=unifs[uid].did;
            if( drawables[did].wid!=wid ) continue;
            if( drawables[did].name!=drawName ) continue;
            if( unifs[uid].name==uniName ) return uid;
        }
    }
    //cout << "[viewer] uniform "<<uniName<<" not found"<<endl;
    return -1;
}
int GLFWViewer::findUnif(int did,string uniName) {
    int uid;
    for(uid=0;uid<unifs.size();uid++)
        if( unifs[uid].did==did && unifs[uid].name==uniName ) return uid;
    //cout << "[viewer] Uniform '"<<uniName<<"' not found"<<endl;
    return -1;
}


//
// ajoute une fenetre qui fit un monitor en particulier
//
int GLFWViewer::addMonitor(string winName,string monitorName) {
    int count;
    GLFWmonitor *mon=NULL;
    const char *name;

    if( !monitorName.empty() ) {
        GLFWmonitor **monitors=glfwGetMonitors(&count);
        int i;
        for(i=0;i<count;i++) {
            name=glfwGetMonitorName(monitors[i]);
            if( monitorName==name ) { mon=monitors[i];break; }
        }
        if( i==count ) { cout << "[viewer] unable to find monitor named "<<monitorName<<endl;return(-1); }
    }else{
        mon=glfwGetPrimaryMonitor();
        name=glfwGetMonitorName(mon);
    }

    int w,h;
    int offx,offy;
    glfwGetMonitorPos(mon,&offx,&offy);

    const GLFWvidmode *vm=glfwGetVideoMode(mon);
    w=vm->width;
    h=vm->height;

    return addWindow(winName,offx,offy,w,h,false,name);
}

//
// ajoute une fenetre
//
int GLFWViewer::addWindow(string winName,int offx,int offy,int w,int h,bool deco,string title) {
    int wid=findWindow(winName);
    if( wid>=0 ) { cout << "[viewer] window "<<winName<<" is already opened"<<endl;return(-1); }
    //cout << "[viewer] wid="<<wid<<endl;
    wid=findFreeWindow();
    //cout << "[viewer] wid="<<wid<<" size=("<<w<<","<<h<<")"<<endl;

    GLFWwindow* window;

    // on pourrait mettre offx,offy,w,h,deco,titre dans la structure window...

    //GLFW_RESIZABLE,  GLFW_VISIBLE, ...
    /*
    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    */

#ifdef APPLE
    //cout << "APPLE !!" << endl;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_DECORATED,deco?GL_TRUE:GL_FALSE);

	// in order to share the context between all window...
	// look for first window, and if there is on, share with it.
    GLFWwindow *firstW=NULL;
    for(int wid=0;wid<windows.size();wid++) {
        if( !windows[wid].deleted ) { firstW=windows[wid].window;break; }
    }

    window=glfwCreateWindow(w, h, title.c_str(), NULL, firstW); // share with firstW, if possible
    if( !window ) {
        cout << "[@@@@@@@@] unable to create window!!!!!!" <<endl;
        glfwTerminate();
        exit(1235);
    }

    //cout << "[viewer] window opened"<<endl;

    glfwSetWindowPos(window,offx,offy);

    glfwSetWindowUserPointer(window,this); // pour les callback

    windows[wid].deleted=false;
    windows[wid].hidden=false;
    windows[wid].window=window;
    windows[wid].name=winName;
	windows[wid].hmd_window=false;
    //cout << "window["<<wid<<"] as name "<<windows[wid].name<<endl;

    // a faire absolument AVANT le init de glew
    glfwMakeContextCurrent(window);

    // initialise glew if possible
    if( !glewInitialized ) {
        glewExperimental=GL_TRUE;
#ifdef APPLE
        glewExperimental=GL_FALSE;
#endif
        if( glewInit()!=GLEW_OK ) {
            cout << "No glew!"<< endl;
            exit(1236);
        }
        glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
        glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);
        cout<<"OpenGL major version = "<<OpenGLVersion[0]<<endl;
        cout<<"OpenGL minor version = "<<OpenGLVersion[1]<<endl<<endl;

        //int major, minor, rev;
        //glfwGetVersion(&major, &minor, &rev);
        //cout << "OpenGL version recieved: "<<major<<"."<<minor<<"."<<rev<<endl;
        glewInitialized=true;
    }

    // quelques callback
    glfwSetCursorPosCallback(window,mouse_callback);
    glfwSetWindowSizeCallback(window,resize_callback);
    glfwSetKeyCallback(window, keyboard_callback);

    // vsync? 0=OFF, 1=ON
    glfwSwapInterval(1);

    int width,height;
    glfwGetWindowSize(windows[wid].window, &width, &height);
    resize_callback(windows[wid].window, width, height);
    return wid;
}

//
// ajoute une fenetre
//
int GLFWViewer::hideWindow(string winName,bool status) {
    int wid=findWindow(winName);
    if( wid<0 ) { cout << "[viewer] hide: window "<<winName<<" doe not exists."<<endl;return(-1); }

    if( status ) {
	    glfwHideWindow(windows[wid].window);
    }else{
	    glfwShowWindow(windows[wid].window);
    }
    windows[wid].hidden=status;
    return(0);
}



//
// ajoute un quad
//  un quad : x1,x2,y1,y2 (0 a 1) , + shader -> retourne un qid
//  -> apres on va extraire tous les uniformes
//     (ou appeller adduniform(wid,qid,"nom") -> on va trouver la localisation
//     ensuite on peut fare un setup genre setuniform("nom",image), ou ,float, int etc...
// winName -> window ou on ajoute le quad
// drawName -> nom du nouveau quad a creer
//
//
int GLFWViewer::addQuad(string winName,string quadName,
                        float x1,float x2,float y1,float y2,float z,string shader,int clamping) {
    int wid=findWindow(winName);
    if( wid<0 ) { cout << "[viewer] no window named "<<winName<<". Unable to add quad "<<quadName<<endl;return(-1); }
    int did=findDrawable(winName,quadName);
    if( did>=0 ) { cout << "[viewer] quad "<<quadName<<" already exists in window "<<winName<<endl;return(-1); }
    return addQuad(wid,quadName,x1,x2,y1,y2,z,shader,clamping);
}

int GLFWViewer::addQuad(int wid,string quadName,float x1,float x2,float y1,float y2,float z,string shader,int clamping) {
    int did=findFreeDrawable();
    drawable Q;

    glfwMakeContextCurrent(windows[wid].window);
    Q.deleted=false;
    Q.name=quadName;
    Q.wid=wid;
    Q.type=DRAW_TYPE_QUAD;
    Q.texname="";
    Q.nbbuffers=1;
    Q.nbdraws=-1;
    Q.clamping=clamping;
    Q.x1=x1; Q.x2=x2; Q.y1=y1; Q.y2=y2; Q.z=z;
    //Q.port=-1;
    Q.hmd_texture=false;
    Q.shader=shader; // on garde le nom pour le fun
    //cout << "adding quad "<<did<<" named "<<Q.name<<" to window "<<windows[wid].name<< " shader "<<shader<< ":"<<x1 << ","<<x2<<","<<y1<<","<<y2<<endl;
    // load immediatement le shader
    string vertexName=shader+"_vert.glsl";
    string fragName  =shader+"_frag.glsl";

    imgv::parseShader *ps=LoadShader(vertexName,fragName, Q.shaderProgram);
    if( ps==NULL ) {
        cout << "Unable to load shader: " <<vertexName<<","<<fragName<<endl;
        return(-1);
    }

    drawables[did]=Q;

    cout << "quad did="<<did<<" window "<<windows[drawables[did].wid].name<<" drawName="<<drawables[did].name<<endl;

    // ajoute les uniformes qu'on a trouve
    addUniforms(did,ps);

    /*
    for(int i=0;i<nbu;i++) {
        if( types[i]==0 ) addUniformSampler(did,uniformes[i]);
        else if( types[i]==1 ) addUniformFloat(did,uniformes[i],0.0);
        else cout << "Type d'uniform inconnu "<<types[i]<<endl;
    }
    */

    return(did);
}

int GLFWViewer::addFbo(string winName,string fboName,
                       int xres,int yres,string texname,int nbbytes,string shader,int clamping) {
    int wid=findWindow(winName);
    if( wid<0 ) { cout << "[viewer] no window named "<<winName<<". Unable to add fbo "<<fboName<<endl;return(-1); }
    int did=findDrawable(winName,fboName);
    if( did>=0 ) { cout << "[viewer] fbo "<<fboName<<" already exists in window "<<winName<<endl;return(-1); }
    return addFbo(wid,fboName,xres,yres,texname,nbbytes,shader,clamping);
}

int GLFWViewer::addFbo(int wid,string fboName,int xres,int yres,string texname,int nbbytes,string shader,int clamping) {
    int did=findFreeDrawable();
    drawable F;

    GLuint maxbuffers,maxsize;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS,(GLint *) &maxbuffers);
    cout << "MAX COLOR ATTACHMENTS: " << maxbuffers << endl;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,(GLint *) &maxsize);
    cout << "MAX TEXTURE SIZE: " << maxsize << endl;
	
    if (xres>maxsize) xres=maxsize;
    if (yres>maxsize) yres=maxsize;
    //if (nbbuffers>maxbuffers) nbbuffers=maxbuffers;
    //if (nbbuffers>MAX_NB_BUFFERS) nbbuffers=MAX_NB_BUFFERS;
    //if (nbbuffers<1) nbbuffers=1;

    glfwMakeContextCurrent(windows[wid].window);
    F.deleted=false;
    F.name=fboName;
    F.wid=wid;
    F.type=DRAW_TYPE_FBO;
    F.x1=0.0; F.x2=1.0; F.y1=1.0; F.y2=0.0; F.z=0.0;
    F.xres=xres; F.yres=yres;
    //F.port=-1;
    F.texname=texname;
    F.nbbuffers=1;
    F.nbdraws=-1;
    F.clamping=clamping;
    F.nbbytes=nbbytes;
	F.hmd_texture=false;
    F.shader=shader; // on garde le nom pour le fun
    //cout << "adding fbo "<<did<<" named "<<F.name<<" to window "<<windows[wid].name<< " shader "<<shader<< ":"<<xres << ","<<yres<<","<<output<<endl;
    // load immediatement le shader
    string vertexName=shader+"_vert.glsl";
    string fragName  =shader+"_frag.glsl";

    imgv::parseShader *ps=LoadShader(vertexName,fragName, F.shaderProgram);
    if( ps==NULL ) {
        cout << "Unable to load shader: " <<vertexName<<","<<fragName<<endl;
        return(-1);
    }

    for (int b=0;b<F.nbbuffers;b++)
    {
        // create the texture
        glGenTextures(1, &(F.fbo_tex[b]));
        glBindTexture(GL_TEXTURE_2D, F.fbo_tex[b]);
        if (F.nbbytes==2) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, xres, yres, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
        else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xres, yres, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR/*GL_NEAREST*/);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamping);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamping);

        // create depth renderbuffer
        //glGenRenderbuffers(1, &(F.fbo_depth));
        //glBindRenderbuffer(GL_RENDERBUFFER, F.fbo_depth);
        //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, xres, yres);

        // create and bind an FBO
        glGenFramebuffers(1, &(F.fbo[b]));
        glBindFramebuffer(GL_FRAMEBUFFER, F.fbo[b]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+b, GL_TEXTURE_2D, F.fbo_tex[b], 0);
        //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, F.fbo_depth);

        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER); // check the status of our generated frame buffer
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE) // if the frame buffer does not report back as complete...
        {
            cout << "Couldn't create frame buffer" << endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset framebuffer binding
    }

    drawables[did]=F;

    cout << "fbo did="<<did<<" window "<<windows[drawables[did].wid].name<<" drawName="<<drawables[did].name << "("<<F.fbo<<","<< F.fbo_tex<<")" << endl;

    // ajoute les uniformes qu'on a trouve
    addUniforms(did,ps);

    return(did);
}

int GLFWViewer::delWindow(string winName) {
    int wid=findWindow(winName);
    if( wid<0 ) return -1;
    // elimine tous les quads
    for(int did=0;did<drawables.size();did++) {
        if( drawables[did].wid==wid ) delDrawable(winName,drawables[did].name);
    }
    glfwDestroyWindow(windows[wid].window);
    windows[wid].window=NULL;
    windows[wid].deleted=true;
    return 0;
}

int GLFWViewer::delDrawable(string winName,string drawName) {
    cout << "del quad "<<winName<<" , "<<drawName<<endl;
    int did=findDrawable(winName,drawName);
    if( did<0 ) { cout << "no drawable"<<endl; return(-1); }

    cout << "deleting drawable "<<did<<endl;

    if (drawables[did].type==DRAW_TYPE_FBO)
    {
        for (int b=0;b<drawables[did].nbbuffers;b++)
        {
            glDeleteTextures(1, &(drawables[did].fbo_tex[b]));
            // bind 0, which means render to back buffer, as a result, fb is unbound
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &(drawables[did].fbo[b]));
        }
    }

    // enleve tous les uniformes...
    for(int uid=0;uid<unifs.size();uid++) {
        if( unifs[uid].did==did ) {
            cout << "delete uniform "<<uid<<endl;
            unifs[uid].deleted=true;
        }
    }
    drawables[did].deleted=true;
    return 0;
}

// normalement on detecte automatiquement les uniformes dans le shader
int GLFWViewer::addUniformSampler(int did,string name) {
    glfwMakeContextCurrent(windows[drawables[did].wid].window);
    uniform U;
    U.deleted=false;
    U.did=did;
    U.redirect=-1;
    U.type=UNIF_TYPE_TEX; // sampler
    U.name=name;
    U.location=glGetUniformLocation(drawables[did].shaderProgram, name.c_str());
    //if (name.find("fbo_")!=0) glGenTextures(1,&U.texNum);
    //else U.texNum=0;

	// ici on pourrait utilisser un numero de texture deja existant.....
	// comme ca on pourrait re-utiliser les images......
	// comment faire? voir ou on call adduniformsampler
    glGenTextures(1,&U.texNum);

    cout << "found uniform "<<U.name<<" in drawable "<<U.did<<" at location "<<U.location<<endl;
    cout << "texture allocated at "<<U.texNum<<endl;
    int uid=findFreeUnif();
    unifs[uid]=U;
    return uid; // index
}

int GLFWViewer::addUniformFloat(int did,string name,float init) {
    glfwMakeContextCurrent(windows[drawables[did].wid].window);
    uniform U;
    U.deleted=false;
    U.did=did;
    U.redirect=-1;
    U.type=UNIF_TYPE_FLOAT; // float
    U.name=name;
    U.location=glGetUniformLocation(drawables[did].shaderProgram, name.c_str());
    U.valf=init;
    cout << "found uniform "<<U.name<<" in drawable "<<U.did<<" at location "<<U.location<<endl;
    int uid=findFreeUnif();
    unifs[uid]=U;
    return uid; // index
}

int GLFWViewer::addUniformInt(int did,string name,int init) {
    glfwMakeContextCurrent(windows[drawables[did].wid].window);
    uniform U;
    U.deleted=false;
    U.did=did;
    U.redirect=-1;
    U.type=UNIF_TYPE_INT; // float
    U.name=name;
    U.location=glGetUniformLocation(drawables[did].shaderProgram, name.c_str());
    U.vali=init;
    cout << "found uniform "<<U.name<<" in drawable "<<U.did<<" at location "<<U.location<<endl;
    int uid=findFreeUnif();
    unifs[uid]=U;
    return uid; // index
}

int GLFWViewer::addUniformVec4(int did,string name,float *init) {
    glfwMakeContextCurrent(windows[drawables[did].wid].window);
    uniform U;
    U.deleted=false;
    U.did=did;
    U.redirect=-1;
    U.type=UNIF_TYPE_VEC4; // vec4
    U.name=name;
    U.location=glGetUniformLocation(drawables[did].shaderProgram, name.c_str());
    for (int i=0;i<4;i++) U.valp[i]=init[i];
    cout << "found uniform "<<U.name<<" in drawable "<<U.did<<" at location "<<U.location<<endl;
    int uid=findFreeUnif();
    unifs[uid]=U;
    return uid; // index
}

int GLFWViewer::addUniformMat4(int did,string name,float *init) {
    glfwMakeContextCurrent(windows[drawables[did].wid].window);
    uniform U;
    U.deleted=false;
    U.did=did;
    U.redirect=-1;
    U.type=UNIF_TYPE_MAT4; // mat4
    U.name=name;
    U.location=glGetUniformLocation(drawables[did].shaderProgram, name.c_str());
    for (int i=0;i<16;i++) U.valp[i]=init[i];
    cout << "found uniform "<<U.name<<" in drawable "<<U.did<<" at location "<<U.location<<endl;
    int uid=findFreeUnif();
    unifs[uid]=U;
    return uid; // index
}

int GLFWViewer::addUniformMat3(int did,string name,float *init) {
    glfwMakeContextCurrent(windows[drawables[did].wid].window);
    uniform U;
    U.deleted=false;
    U.did=did;
    U.redirect=-1;
    U.type=UNIF_TYPE_MAT3; // mat3
    U.name=name;
    U.location=glGetUniformLocation(drawables[did].shaderProgram, name.c_str());
    for (int i=0;i<9;i++) U.valp[i]=init[i];
    cout << "found uniform "<<U.name<<" in drawable "<<U.did<<" at location "<<U.location<<endl;
    int uid=findFreeUnif();
    unifs[uid]=U;
    return uid; // index
}

//
//
/* special: premier uniforme compatible... */
/*
void GLFWViewer::setUniformData(cv::Mat image) {
    for(int uid=0;uid<unifs.size();uid++) {
        if( unifs[uid].type==UNIF_TYPE_TEX ) {
            //std::cout << "[viewer] using uniform "<<unifs[uid].name<<std::endl;
            setUniformData(uid,image);
            return;
        }
    }
    cout << "[viewer] No uniform defined to display image!!!!"<<endl;
}
*/
//
// special: une string identifiant /winName/drawName/uniName en une seule string
// eventuellement, /win trouvera le permier uniform dans cette win
//  et /win/drawable le permier uniform dans ce drawable
int GLFWViewer::getUniformFromView(string view) {
    int pos,len;
    if( view.empty() ) {
        // trouve le 1er utilisable...
        for(int uid=0;uid<unifs.size();uid++) {
            if( unifs[uid].type==UNIF_TYPE_TEX ) return uid;
        }
        return -1;
    }
    const char *wqi=view.c_str();
    int nb=oscutils::extractNbArgs(wqi);
    if( nb>3 || nb==0 ) { cout << "[viewer] illegal view : "<<view<<endl;return -1; }
    if( nb==3 ) {
        oscutils::extractArg(wqi,2,pos,len);
        string winName(wqi+pos,len);
        oscutils::extractArg(wqi,1,pos,len);
        string drawName(wqi+pos,len);
        oscutils::extractArg(wqi,0,pos,len);
        string uniName(wqi+pos,len);
        return findUnif(winName,drawName,uniName);
    }
    /***else if( nb==2 ) {
        oscutils::extractArg(wqi,1,pos,len);
        string winName(wqi+pos,len);
        oscutils::extractArg(wqi,0,pos,len);
        string drawName(wqi+pos,len);
        int qid=findDrawable(winName,drawName);
        if( qid<0 ) { cout << "[viewer] view drawable "<<wqi<<" not found."<<endl;return; }
        for(int uid=0;uid<unifs.size();uid++) {
            if( unifs[uid].type==UNIF_TYPE_TEX ) {
        }
    }
**/
    cout << "[viewer] illegal identifier : "<<view<<endl;
    return -1;
}

int GLFWViewer::getDrawableFromView(string view) {
    int pos,len;
    if( view.empty() ) {
        // trouve le 1er fbo utilisable...
        for(int did=0;did<drawables.size();did++) {
            if( drawables[did].type==DRAW_TYPE_FBO ) return did;
        }
        return -1;
    }
    const char *wq=view.c_str();
    int nb=oscutils::extractNbArgs(wq);
    if( nb>3 || nb==0 ) { cout << "[viewer] illegal view : "<<view<<endl;return -1; }
    if( nb==2 || nb==3 ) {
        oscutils::extractArg(wq,nb-1,pos,len);
        string winName(wq+pos,len);
        oscutils::extractArg(wq,nb-2,pos,len);
        string drawName(wq+pos,len);
        return findDrawable(winName,drawName);
    }

    cout << "[viewer] illegal view : "<<view<<endl;
    return -1;
}

string GLFWViewer::getDrawableView(int did) {
    if (did<0 || did>=drawables.size()) return "";
    return "/" + windows[drawables[did].wid].name + "/" + drawables[did].name + "/" + drawables[did].texname;
}

string GLFWViewer::getUniformView(int uid) {
    if (uid<0 || uid>=unifs.size()) return "";
    return "/" + windows[drawables[unifs[uid].did].wid].name + "/" + drawables[unifs[uid].did].name + "/" + unifs[uid].name;
}

float GLFWViewer::getUniformFloatValue(int uid) {
    if (uid<0 || uid>=unifs.size()) return 0.0f;
    return (float)(unifs[uid].valf);
}

void GLFWViewer::readDrawableFbo(int did,cv::Mat *image) {
    if (did<0 || did>=drawables.size()) return;

    //cout << "Reading FBO (" << drawables[did].xres << "," << drawables[did].yres << ")" << endl;
    image->create(drawables[did].yres,drawables[did].xres, CV_8UC3);

    //use fast 4-byte alignment (default anyway) if possible
    glPixelStorei(GL_PACK_ALIGNMENT, (image->step & 3) ? 1 : 4);
    //set length of one complete row in destination data (doesn't need to equal img.cols)
    glPixelStorei(GL_PACK_ROW_LENGTH, image->step/image->elemSize());

    glBindFramebuffer(GL_FRAMEBUFFER, drawables[did].fbo[0]);

    //read back pixels
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, drawables[did].xres, drawables[did].yres, GL_BGR, GL_UNSIGNED_BYTE, (GLvoid*)image->data);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFWViewer::setHmdTexture(int did,bool b) 
{
#ifdef HAVE_OCULUS
  int i;
  if (did<0 || did>=drawables.size()) return;
  if (b)
  {
    drawables[did].hmd_texture=true;
    int wid=drawables[did].wid;
	if (windows[wid].hmd_window==false)
	{
	  windows[wid].hmd = ovrHmd_Create(0);
      if (windows[wid].hmd)
      {
	    windows[wid].hmd_window=true;

        //Hmd recommended texture dimensions
        Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(windows[wid].hmd, ovrEye_Left,windows[wid].hmd->DefaultEyeFov[0], 1.0f);
        Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(windows[wid].hmd, ovrEye_Right,windows[wid].hmd->DefaultEyeFov[1], 1.0f);
      
        //Consider left eye fov
        windows[wid].hmd_fov_tan_left = windows[wid].hmd->DefaultEyeFov[0].LeftTan;
        windows[wid].hmd_fov_tan_right = windows[wid].hmd->DefaultEyeFov[0].RightTan;
        windows[wid].hmd_fov_tan_up = windows[wid].hmd->DefaultEyeFov[0].UpTan;
        windows[wid].hmd_fov_tan_down = windows[wid].hmd->DefaultEyeFov[0].DownTan;
        printf("Tan fovs (left eye): %f %f %f %f\n",windows[wid].hmd->DefaultEyeFov[0].LeftTan,windows[wid].hmd->DefaultEyeFov[0].RightTan,windows[wid].hmd->DefaultEyeFov[0].UpTan,windows[wid].hmd->DefaultEyeFov[0].DownTan);
        printf("Tan fovs (right eye): %f %f %f %f\n",windows[wid].hmd->DefaultEyeFov[1].LeftTan,windows[wid].hmd->DefaultEyeFov[1].RightTan,windows[wid].hmd->DefaultEyeFov[1].UpTan,windows[wid].hmd->DefaultEyeFov[1].DownTan);
  
        //Force symmetrical horizontal and vertical fields of view
        /*ovrFovPort fovLeft = windows[wid].hmd->DefaultEyeFov[ovrEye_Left];
        ovrFovPort fovRight = windows[wid].hmd->DefaultEyeFov[ovrEye_Right];
        ovrFovPort fovMax = FovPort::Max(fovLeft, fovRight);
        float combinedTanHalfFovHorizontal = max ( fovMax.LeftTan, fovMax.RightTan );
        float combinedTanHalfFovVertical = max ( fovMax.UpTan, fovMax.DownTan );
        ovrFovPort fovBoth;
        fovBoth.LeftTan = fovBoth.RightTan = combinedTanHalfFovHorizontal;
        fovBoth.UpTan = fovBoth.DownTan = combinedTanHalfFovVertical;

        windows[wid].hmd_fov_tan_left = combinedTanHalfFovHorizontal;
        windows[wid].hmd_fov_tan_right = combinedTanHalfFovHorizontal;
        windows[wid].hmd_fov_tan_up = combinedTanHalfFovHorizontal;
        windows[wid].hmd_fov_tan_down = combinedTanHalfFovHorizontal;
		//windows[wid].hmd_fov=atan(windows[wid].hmd->DefaultEyeFov[ovrEye_Left].LeftTan)+atan(windows[wid].hmd->DefaultEyeFov[ovrEye_Left].RightTan);
        double fov=2.0f * atanf ( combinedTanHalfFovHorizontal ) * 180.0 / M_PI;
        printf("Fov: %f\n",fov);
        double aspectratio = combinedTanHalfFovVertical / combinedTanHalfFovHorizontal;        
        printf("Aspect ratio: %f\n",aspectratio);

        recommenedTex0Size = ovrHmd_GetFovTextureSize(windows[wid].hmd, ovrEye_Left, fovBoth, 1.0f);
        recommenedTex1Size = ovrHmd_GetFovTextureSize(windows[wid].hmd, ovrEye_Right, fovBoth, 1.0f);*/

        int xres = recommenedTex0Size.w + recommenedTex1Size.w;
        int yres = max ( recommenedTex0Size.h, recommenedTex1Size.h );
        if (xres>drawables[did].xres || yres>drawables[did].yres)
        {
          printf("Warning: texture size (%d,%d) is smaller than recommanded render size (%d,%d).\n",drawables[did].xres,drawables[did].yres,xres,yres);
        }

	    //Start the sensor which provides the Rift’s pose and motion.
        //ovrTrackingCap_MagYawCorrection: supports yaw drift correction via a magnetometer or other means.
        ovrHmd_ConfigureTracking(windows[wid].hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection, 0); // | ovrTrackingCap_Position	  	  
        for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
        {
          ovrEyeType eye = windows[wid].hmd->EyeRenderOrder[eyeIndex];
	  	  windows[wid].hmd_eyefov[eye]=windows[wid].hmd->DefaultEyeFov[eye];
          //windows[wid].hmd_eyefov[eye]=fovBoth;
	      windows[wid].hmd_eyerender_desc[eye]=ovrHmd_GetRenderDesc(windows[wid].hmd, eye, windows[wid].hmd_eyefov[eye]);
	    }
        //ovrVector3f toffset=windows[wid].hmd_eyerender_desc[0].HmdToEyeViewOffset;
        //printf("%f %f %f\n",toffset.x,toffset.y,toffset.z);

	    //Configure OpenGL.                  
        windows[wid].hmd_cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
        windows[wid].hmd_cfg.OGL.Header.BackBufferSize = Sizei(windows[wid].hmd->Resolution.w, windows[wid].hmd->Resolution.h);
	    //cout << "Backbuffer " << windows[wid].hmd->Resolution.w << " " << windows[wid].hmd->Resolution.h << endl;
        windows[wid].hmd_cfg.OGL.Header.Multisample = 1;
        #ifdef WIN32
		  //windows[wid].hmd_cfg.OGL.Window = glfwGetWin32Window(windows[wid].window);
		  windows[wid].hmd_cfg.OGL.Window = GetActiveWindow();
		  windows[wid].hmd_cfg.OGL.DC = GetDC(windows[wid].hmd_cfg.OGL.Window);
		#else
          windows[wid].hmd_cfg.OGL.Disp = glfwGetX11Display();
		#endif
		
        //ovrDistortionCap_Chromatic now always enabled
        int render_options=ovrDistortionCap_TimeWarp|ovrDistortionCap_FlipInput|ovrDistortionCap_Vignette;
        ovrBool result = ovrHmd_ConfigureRendering(windows[wid].hmd, &windows[wid].hmd_cfg.Config, render_options , windows[wid].hmd_eyefov, windows[wid].hmd_eyerender_desc);
  	    //cout << "Configure result: " << ovrBool << endl;
      }
      // dismiss warning
	  //ovrHmd_DismissHSWDisplay(windows[wid].hmd);
	}
  }
  else
  {
    drawables[did].hmd_texture=false;
	//set window as having no hmd if none of its drawables is an hmd texture
	for(i=0;i<drawables.size();i++)
    {
      if( drawables[i].wid!=drawables[did].wid ) continue;
	  if( drawables[i].deleted ) continue;
	  if( drawables[i].hmd_texture==true ) break;
	}
	if (i>=drawables.size())
    {
 	  windows[drawables[did].wid].hmd_window=false;   
	}
  }
#else  
  cout << "No HMD installed." <<endl;
#endif
}

void GLFWViewer::setUniformData(string winName,string drawName,string uniName,cv::Mat image) {
    int uid=findUnif(winName,drawName,uniName);
    if( uid>=0 ) setUniformData(uid,image);
    //else cout << "Unable to find uniform " << winName << "/" << drawName << "/" << uniName << endl;
}
void GLFWViewer::setUniformData(string winName,string drawName,string uniName,float val) {
    int uid=findUnif(winName,drawName,uniName);
    if( uid>=0 ) setUniformData(uid,val);
    //else cout << "Unable to find uniform " << winName << "/" << drawName << "/" << uniName << endl;
}
void GLFWViewer::setUniformData(string winName,string drawName,string uniName,int val) {
    int uid=findUnif(winName,drawName,uniName);
    if( uid>=0 ) setUniformData(uid,val);
    //else cout << "Unable to find uniform " << winName << "/" << drawName << "/" << uniName << endl;
}
void GLFWViewer::setUniformData(string winName,string drawName,string uniName,float *val,unsigned int len) {
    int uid=findUnif(winName,drawName,uniName);
    if( uid>=0 ) setUniformData(uid,val,len);
    //else cout << "Unable to find uniform " << winName << "/" << drawName << "/" << uniName << endl;
}

//
//

// si on veut utiliser directement le nom des uniformes
void GLFWViewer::setUniformData(int qid,string name,cv::Mat image) {
    int uid=findUnif(qid,name);
    if( uid>=0 ) setUniformData(uid,image);
}
void GLFWViewer::setUniformData(int qid,string name,float val) {
    int uid=findUnif(qid,name);
    if( uid>=0 ) setUniformData(uid,val);
}
void GLFWViewer::setUniformData(int qid,string name,int val) {
    int uid=findUnif(qid,name);
    if( uid>=0 ) setUniformData(uid,val);
}
void GLFWViewer::setUniformData(int qid,string name,float *val,unsigned int len) {
    int uid=findUnif(qid,name);
    if( uid>=0 ) setUniformData(uid,val,len);
}


//
//

void GLFWViewer::setUniformData(int uid,cv::Mat image) {
    int did=unifs[uid].did;
    if( unifs[uid].type!=UNIF_TYPE_TEX ) {cout << "mauvais type uniform pour image ("<< unifs[uid].name << ")" << endl;return; }
    glfwMakeContextCurrent(windows[drawables[did].wid].window);
    initTexture2D( unifs[uid].texNum,image,drawables[did].clamping );
    //initMipMap2D( unifs[uid].texNum,image,lod );
}
void GLFWViewer::setUniformData(int uid,float val) {
    if( unifs[uid].type!=UNIF_TYPE_FLOAT ) {cout << "mauvais type uniform pour float ("<< uid << ", " << unifs[uid].name << ")" << endl;return; }
    unifs[uid].valf=val;
}
void GLFWViewer::setUniformData(int uid,int val) {
    if( unifs[uid].type!=UNIF_TYPE_INT ) {cout << "mauvais type uniform pour int ("<< unifs[uid].name << ")" << endl;return; }
    unifs[uid].vali=val;
}
void GLFWViewer::setUniformData(int uid,float *val,unsigned int len) {
    int i;
    //if( len!=4 && len!=16 ) {cout << "mauvaise taille pour uniform vec4 ou mat4"<<endl;return; }
    if( len==4 && unifs[uid].type!=UNIF_TYPE_VEC4 ) {cout << "mauvais type uniform pour vec4 ("<< unifs[uid].name << ")" << endl;return; }
    if( len==16 && unifs[uid].type!=UNIF_TYPE_MAT4 ) {cout << "mauvais type uniform pour mat4 ("<< unifs[uid].name << ")" << endl;return; }
    if( len==9 && unifs[uid].type!=UNIF_TYPE_MAT3 ) {cout << "mauvais type uniform pour mat3 ("<< unifs[uid].name << ")" << endl;return; }
    for (i=0;i<len;i++) {
        unifs[uid].valp[i]=val[i];
    }
}

void GLFWViewer::addUniforms(int did,imgv::parseShader *ps)
{
    int i;
    float vtmp[16];
    for (i=0;i<16;i++) vtmp[i]=0;
    //ps->dump();
    for(i=0;i<ps->nbUni;i++) {
        if( ps->isTexture(i) ) addUniformSampler(did,ps->uniNames[i]);
        else if( ps->isFloat(i) ) addUniformFloat(did,ps->uniNames[i],0.0f);
        else if( ps->isInt(i) ) addUniformInt(did,ps->uniNames[i],0);
        else if( ps->isVec4(i) ) addUniformVec4(did,ps->uniNames[i],vtmp);
        else if( ps->isMat4(i) ) addUniformMat4(did,ps->uniNames[i],vtmp);
        else if( ps->isMat3(i) ) addUniformMat3(did,ps->uniNames[i],vtmp);
        else cout << "[!!!!] Uniform "<<ps->uniNames[i]<<" has unknown type "<<ps->uniTypes[i]<<endl;
    }
    delete ps;
}


/*
  int GLFWViewer::addText(int wid,float x,float y,float color[3],string txt) {
    text t;
    t.wid=wid; // should check
    t.x=x;
    t.y=y;
    t.text=txt;
    for(int i=0;i<3;i++) t.color[i]=color[i];
    int tid=texts.size();
    texts.push_back(t);
    return(tid);
  }
  void GLFWViewer::setText(int tid,float x,float y,float color[3],string txt) {
    if( tid<0 || tid>=texts.size() ) return;
    texts[tid].x=x;
    texts[tid].y=y;
    texts[tid].text=txt;
    for(int i=0;i<3;i++) texts[tid].color[i]=color[i];
  }
  void GLFWViewer::setText(int tid,string txt) {
    if( tid<0 || tid>=texts.size() ) return;
    texts[tid].text=txt;
  }
  void GLFWViewer::setText(int tid,float x,float y) {
    if( tid<0 || tid>=texts.size() ) return;
    texts[tid].x=x;
    texts[tid].y=y;
  }
  void GLFWViewer::setText(int tid,float color[3]) {
    if( tid<0 || tid>=texts.size() ) return;
    for(int i=0;i<3;i++) texts[tid].color[i]=color[i];
  }
*/

void GLFWViewer::start() {
    cout << "starting viewerGLFW" <<endl;
    // rien a faire...
}
void GLFWViewer::stop() {
    cout << "stopping viewerGLFW" <<endl;
    int wid;
    for(wid=0;wid<windows.size();wid++) {
        if( windows[wid].deleted ) continue;
        glfwDestroyWindow(windows[wid].window);
    }
    glfwTerminate();
    // vide les structures de donnees
    int did;
    for(did=0;did<drawables.size();did++)
    {
        if( drawables[did].deleted ) continue;
        delDrawable(windows[drawables[did].wid].name,drawables[did].name);
    }
    windows.clear();
    drawables.clear();
    unifs.clear();
    texts.clear();

    //exit(0);

}

int GLFWViewer::bindUniforms(int did) {
    int uid;
    int unit=0;
    int uidref; // uid of redirected uniform. usually =uid when no redirect
    for(uid=0;uid<unifs.size();uid++) {
        if( unifs[uid].deleted ) continue;
        if( unifs[uid].did!=did ) continue; // pas le bon drawable!
        //cout << "  defining uniform "<<unifs[uid].name<< " texnum="<<unifs[uid].texNum<<" location="<<unifs[uid].location<<endl;
	if( (uidref=unifs[uid].redirect)<0 ) uidref=uid; // no redirection

        switch( unifs[uid].type ) {
        case UNIF_TYPE_TEX:
            glActiveTexture(GL_TEXTURE0+unit);
            glBindTexture(GL_TEXTURE_2D, unifs[uidref].texNum);
            glUniform1i(unifs[uid].location,unit);
            unit++;
            break;
        case UNIF_TYPE_FLOAT:
            glUniform1f(unifs[uid].location, unifs[uidref].valf);
            break;
        case UNIF_TYPE_INT:
            glUniform1i(unifs[uid].location, unifs[uidref].vali);
            break;
        case UNIF_TYPE_VEC4:
            glUniform4fv(unifs[uid].location, 1, unifs[uidref].valp);
            break;
        case UNIF_TYPE_MAT4:
            glUniformMatrix4fv(unifs[uid].location, 1, GL_TRUE, unifs[uidref].valp);
            break;
        case UNIF_TYPE_MAT3:
            glUniformMatrix3fv(unifs[uid].location, 1, GL_TRUE, unifs[uidref].valp);
            break;
        }
    }
    return unit;
}


int GLFWViewer::bindFbos(int did, int unit, int buffer) {
    int fid;
    int uid;
    for (uid=0;uid<unifs.size();uid++)
    {
        //if (unifs[uid].did==did && unifs[uid].type==UNIF_TYPE_TEX && unifs[uid].name.find("fbo_")==0) //uniform sampler that is a reference to a fbo texture...find this fbo
        if (unifs[uid].did==did && unifs[uid].type==UNIF_TYPE_TEX)
        {
            string uniName=getUniformView(uid);
            //cout << "FBO uniName: " << uniName << endl;
            map<string,string>::iterator ua;
            ua=aliases.find(uniName);
            if( ua!=aliases.end() ) uniName=ua->second;
            else uniName=unifs[uid].name;
            //cout << "FBO uniName alias: " << uniName << endl;
            for(fid=0;fid<drawables.size();fid++)
            {
                if( drawables[fid].deleted ) continue;
                if( drawables[fid].type!=DRAW_TYPE_FBO ) continue;
                //if (unifs[uid].name.find(drawables[fid].texname)==0)
                if (uniName.find(drawables[fid].texname)==0)
                {
                    int b=buffer;
                    string bstr=uniName.substr(drawables[fid].texname.size());
                    if (bstr!="") b=atoi(bstr.c_str());
                    //if (bstr!="") cout << "buffer (" << uniName << ") (" << drawables[fid].texname << "): " << b << endl;
                    if (b<0 || b>=drawables[fid].nbbuffers) b=0;
                    //cout << "Binding FBO to uniform " << drawables[fid].texname << " (buffer: "<< b << ")" << endl;
                    glActiveTexture(GL_TEXTURE0+unit);
                    glBindTexture(GL_TEXTURE_2D, drawables[fid].fbo_tex[b]);
                    glUniform1i(unifs[uid].location,unit);
                    unit++;
                    break;
                }
            }
        }
    }
    return 0;
}

// make uniNameFrom refer to uniNameTo, so the texture of uniNameTo will be used when drawing
// instead of uniNameFrom.
void GLFWViewer::setRedirect(string uniNameFrom,string uniNameTo)
{
	int uidFrom=getUniformFromView(uniNameFrom);
	int uidTo=getUniformFromView(uniNameTo);

	if( uidFrom<0 || uidTo<0 ) return;
	unifs[uidFrom].redirect=uidTo;
	cout << "added redirection from "<<uniNameFrom<<" to "<<uniNameTo<<endl;
}

void GLFWViewer::unsetRedirect(string uniNameFrom)
{
	int uidFrom=getUniformFromView(uniNameFrom);
	if( uidFrom<0 ) return;
	unifs[uidFrom].redirect=-1; // no more redirection!
	cout << "removed redirection from "<<uniNameFrom<<endl;
}


void GLFWViewer::addAlias(string uniName,string fboName)
{
    aliases[uniName]=fboName;
}

void GLFWViewer::setNbDraws(string fboName,int nbdraws)
{
    for(int fid=0;fid<drawables.size();fid++)
    {
        if (drawables[fid].texname==fboName)
        {
            drawables[fid].nbdraws=nbdraws;
            break;
        }
    }
}

bool GLFWViewer::drawAll() {
    int wid;
    for(wid=0;wid<windows.size();wid++) {
        if( windows[wid].deleted ) { continue; }
        if( windows[wid].hidden ) { continue; } // skip all rendering for this one
        // pour le moment, un close = ferme tout
        //cout << "DRAW wid="<<wid<<endl;
        if( !draw(wid) ) return false;
    }
    return true;
}
bool GLFWViewer::draw(int wid) {
    //cout << "--- draw " << windows[wid].name << endl;
    if( windows[wid].deleted ) return true; // on skip! PAS NORMAL!
    if( glfwWindowShouldClose(windows[wid].window) ) return false;

    glfwMakeContextCurrent(windows[wid].window);

    GLenum draw_buff;
    // pour la transparence
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    // clear back buffer for quads
    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    draw_buff=GL_BACK_LEFT;
    glDrawBuffers(1, &(draw_buff));
    glClear( GL_COLOR_BUFFER_BIT );

    int did,uid;
	float EyePitch,EyeRoll,EyeYaw;
    float yawparam,pitchparam;
    int width,height;
    bool cleared;
    int pass; //make 2 passes, first renders fbos, then quads
    for (pass=0;pass<2;pass++)
    {
	if( pass==0 ) {
        profilometre_start("profil");
		profilometre_start("pass0");
        profilometre_stop("profil");
	}else{
		profilometre_stop("pass0");
		profilometre_start("pass1");
	}
        // dessine chaque fbos dans un premier temps (un fbo peut servir de texture dans un quad)
        for(did=0;did<drawables.size();did++)
        {
            if( drawables[did].deleted ) continue;
            if( pass==0 && drawables[did].type!=DRAW_TYPE_FBO ) continue;
            if( pass==1 && drawables[did].type!=DRAW_TYPE_QUAD ) continue;
            if( drawables[did].wid != wid ) continue; // pas la bonne fenetre
            if (drawables[did].nbdraws==0 ) continue;
            if (drawables[did].nbdraws>0 ) drawables[did].nbdraws--;
            //cout << "drawing pass="<<pass<<" did="<<did<< " name="<<drawables[did].name<<endl;

            for (int b=0;b<drawables[did].nbbuffers;b++)
            {
                if (pass==0) //fbo
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, drawables[did].fbo[b]);
                    width=drawables[did].xres;
                    height=drawables[did].yres;
                    draw_buff=GL_COLOR_ATTACHMENT0+b;
                    //set fbo buffer index, if present
                    for (int uid=0;uid<unifs.size();uid++)
                    {
                        if (unifs[uid].did==did && unifs[uid].name=="fbo_buffer")
                        {
                            unifs[uid].vali=b;
                        }
                    }
                }
                else //quad
                {
                    glfwGetWindowSize(windows[drawables[did].wid].window, &width, &height);
                    //cout << "window is "<<width << " x " << height<<endl;
                    draw_buff=GL_BACK_LEFT;
                }
#ifdef HAVE_OCULUS
             	ovrGLTexture EyeTexture[2];
				if (windows[wid].hmd_window && drawables[did].hmd_texture)
	            {
                  for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
                  {
                    ovrEyeType eye = windows[wid].hmd->EyeRenderOrder[eyeIndex];
                    windows[wid].hmd_pose[eye] = ovrHmd_GetHmdPosePerEye(windows[wid].hmd, eye);
              	  }
                  profilometre_stop("begin");
                  profilometre_start("begin");
                  // le beginframe est instantane. 0ms
                  // tout jusqu'au enframe est aussi instantane.
                  ovrFrameTiming hmdFrameTiming = ovrHmd_BeginFrame(windows[wid].hmd, 0);
                  ovrPosef headPose[2];
                  // Query the HMD for the current tracking state.
                  ovrTrackingState ts = ovrHmd_GetTrackingState(windows[wid].hmd, ovr_GetTimeInSeconds());
                  if (ts.StatusFlags & ovrStatus_OrientationTracked)
                  {
                    profilometre_start("pose");
                    Posef pose = ts.HeadPose.ThePose;
                    pose.Rotation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&EyeYaw, &EyePitch, &EyeRoll);
                    //printf("%f %f %f\n",EyeYaw,EyePitch,EyeRoll);
                    //fflush(stdout);
                    yawparam=-(float)((EyeYaw+M_PI)/(2*M_PI));
                    pitchparam=1.0-(float)((EyePitch+(M_PI/2))/M_PI);
                    //printf("%f %f\n",yawparam,pitchparam);
                    //fflush(stdout);
                    for (int eyeIndex = 0; eyeIndex < 2; eyeIndex++)
                    {
                      ovrEyeType eye = windows[wid].hmd->EyeRenderOrder[eyeIndex];
                      headPose[eye] = ovrHmd_GetHmdPosePerEye(windows[wid].hmd, eye);
		            }
					uid=findUnif(did,"yaw");
					setUniformData(uid,yawparam);
					//cout << uid << " " << yawparam << endl;
					uid=findUnif(did,"pitch");
					setUniformData(uid,pitchparam);
					//cout << uid << " " << pitchparam << endl;
					
					uid=findUnif(did,"fov_cam_tan_left");
					setUniformData(uid,windows[wid].hmd_fov_tan_left);
					uid=findUnif(did,"fov_cam_tan_right");
					setUniformData(uid,windows[wid].hmd_fov_tan_right);
					uid=findUnif(did,"fov_cam_tan_up");
					setUniformData(uid,windows[wid].hmd_fov_tan_up);
                    uid=findUnif(did,"fov_cam_tan_down");
                    setUniformData(uid,windows[wid].hmd_fov_tan_down);
                    profilometre_stop("pose");
		          }		 				  
				  Sizei renderTargetSize;
				  if( drawables[did].type==DRAW_TYPE_FBO )
				  {
				    renderTargetSize.w=drawables[did].xres;
				    renderTargetSize.h=drawables[did].yres;				  
				  }
				  else
				  {
				    renderTargetSize.w=width;
				    renderTargetSize.h=height;
			      }
				  
                  ovrRecti EyeRenderViewport[2];
				  EyeRenderViewport[0].Pos = Vector2i(0,0);
                  EyeRenderViewport[0].Size = Sizei(renderTargetSize.w / 2, renderTargetSize.h);
                  EyeRenderViewport[1].Pos = Vector2i((renderTargetSize.w + 1) / 2, 0);
                  EyeRenderViewport[1].Size = EyeRenderViewport[0].Size;
				  //cout << "Pos0 " << EyeRenderViewport[0].Pos.x << " " << EyeRenderViewport[0].Pos.y << endl;
				  //cout << "Pos1 " << EyeRenderViewport[1].Pos.x << " " << EyeRenderViewport[1].Pos.y << endl;
				  //cout << "Size0 " << EyeRenderViewport[0].Size.w << " " << EyeRenderViewport[0].Size.h << endl;
				  //cout << "Size1 " << EyeRenderViewport[1].Size.w << " " << EyeRenderViewport[1].Size.h << endl;

				  for (int eyeIndex=0;eyeIndex<2;eyeIndex++)
				  {
                    EyeTexture[eyeIndex].OGL.Header.API = ovrRenderAPI_OpenGL;
                    EyeTexture[eyeIndex].OGL.Header.TextureSize = renderTargetSize;
                    EyeTexture[eyeIndex].OGL.Header.RenderViewport = EyeRenderViewport[eyeIndex];
                    EyeTexture[eyeIndex].OGL.TexId = drawables[did].fbo_tex[b];
				  }
		  			  
				  //Direct rendering from a window handle to the Hmd.
                  //Not required if ovrHmdCap_ExtendDesktop flag is set.
                  //ovrHmd_AttachToWindow(hmd, window, NULL, NULL);
                }
#endif				
                if (motion_file!=NULL)
                { 
                  int yaw_uid,pitch_uid;           
                  yaw_uid=findUnif(did,"yaw");
	     		  pitch_uid=findUnif(did,"pitch");
                  if( yaw_uid>=0 && pitch_uid>=0 ) 
                  {
                    char line[50];
                    int k = fscanf(motion_file," %[^\n]",line);
                    float yaw,pitch;
                    if (k==1 && sscanf(line,"%f %f",&yaw,&pitch)==2)
                    {
                      printf("Motion input file: %f %f\n",yaw,pitch);

			  		  setUniformData(yaw_uid,yaw);
					  setUniformData(pitch_uid,pitch);
                    }
                  }
                }
                glDrawBuffers(1, &(draw_buff));
                if (pass==0) //fbo: no transparency, clear buffer
                {
                    glBlendFunc(GL_ONE, GL_ZERO);
                    //glClear( GL_COLOR_BUFFER_BIT );
                }
                else //quad: transparency
                {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }

                glUseProgram(drawables[did].shaderProgram);
                int unit=bindUniforms(did);
                bindFbos(did,unit,b);
				
                // place au bon endroit
                float x1=/*width**/drawables[did].x1;
                float x2=/*width**/drawables[did].x2;
                float y1=/*height**/drawables[did].y1;
                float y2=/*height**/drawables[did].y2;
                float z=-10.0; //not sure that drawables[did].z is actually necessary because of glDisable(GL_DEPTH_TEST)

                glViewport(0, 0, width, height);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0.0, 1.0/*(double)(width)*/, 1.0, 0.0/*(double)(height)*/, 0.0, 1000.0);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();

                // le coord de texture du quad
                static float textices[12] = {
                    0.0f, 0.0f,
                    1.0f, 0.0f,
                    1.0f, 1.0f,
                    0.0f, 0.0f,
                    1.0f, 1.0f,
                    0.0f, 1.0f };
                float vertices[18] = {
                    x1,y1,z,
                    x2,y1,z,
                    x2,y2,z,
                    x1,y1,z,
                    x2,y2,z,
                    x1,y2,z };
					
                // stop using VBO
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // ensure the proper arrays are enabled
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);

                // setup buffer offsets
                glVertexPointer(3, GL_FLOAT, 0, vertices);
                glTexCoordPointer(2, GL_FLOAT, 0, textices);

                // drawing
                glDisable(GL_CULL_FACE);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                //glDrawElements(GL_TRIANGLES, 2, GL_UNSIGNED_BYTE, indices);

#ifdef HAVE_OCULUS
		if (windows[wid].hmd_window && drawables[did].hmd_texture)
	            {
	              //Let OVR do distortion rendering, Present and flush/sync.
  		          profilometre_start("timeend");
                  ovrHmd_EndFrame(windows[wid].hmd, windows[wid].hmd_pose, (const ovrTexture *)(EyeTexture));
		          profilometre_stop("timeend");
				  //cout << "End frame." << endl;
             	}
#endif				

                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);

                glUseProgram(0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind our frambuffer for drawing
            }
        } // drawables
    } // pass
    profilometre_stop("pass1");

    //
    // affiche du texte en position bitmap de la fenetre
    //
    // GLUT_BITMAP_9_BY_15  _8_BY_13
    // GLUT_BITMAP_TIMES_ROMAN_10 _24
    // GLUT_BITMAP_HELVETICA_10 _12 _18
    //
    /***
    int w0= 0;//glutGet((GLenum)GLUT_WINDOW_WIDTH);
    int h0= 0;//glutGet((GLenum)GLUT_WINDOW_HEIGHT);
    glUseProgram(0);
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0,w0,0,h0,1,50);
    // ajoute un peu de texte par dessus tout ca
    float color[4] = { 1.0,0.0,0.0,1.0};
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
        glLoadIdentity();
        for(int i=0;i<texts.size();i++) {
            if( texts[i].wid==wid ) RenderString(texts[i]);
        }
    glPopMatrix();
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
***/

    // Draw blue text at screen coordinates (100, 120), where (0, 0) is the top-left of the
    // screen in an 18-point Helvetica font
    //string toto="Un texte!!!";
    //glRasterPos3f(0.0f,0.0f,-10.0f);
    //glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    //glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)toto.c_str());
    //glutStrokeCharacter(GLUT_STROKE_ROMAN, 'H');

    // fini!
    if (windows[wid].hmd_window==false)
    {
	  //cout << "Swapping buffers." << endl;
	  profilometre_stop("render");
	  profilometre_start("render");
	  glfwSwapBuffers(windows[wid].window);
    }
	
    glfwPollEvents();
    return true;
}

void GLFWViewer::resize_callback( GLFWwindow *window, GLsizei width, GLsizei height ) {
    //glViewport and glOrtho and now set in draw()
}

// static
void GLFWViewer::keyboard_callback( GLFWwindow *window,  int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    GLFWViewer *V=(GLFWViewer *)glfwGetWindowUserPointer(window);
    int wid;
    for(wid=0;wid<V->windows.size();wid++) {
        if( V->windows[wid].deleted ) continue;
        if( V->windows[wid].window==window ) break;
    }
    // process dans le plugin, si souhaite
    double xpos,ypos;
    int w,h;
    glfwGetCursorPos(window,&xpos,&ypos);
    glfwGetWindowSize(window, &w, &h);

    if( V->listener!=NULL ) V->listener->key(V->windows[wid].name.c_str(),key,xpos,ypos,w,h);
    else{
        cout << "escape is "<<GLFW_KEY_ESCAPE<<endl;
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
        default:
            cout << "key "<<key<<" pressed"<<endl;
        }
    }
}

void GLFWViewer::mouse_callback( GLFWwindow *window,  double x,double y) {
    GLFWViewer *V=(GLFWViewer *)glfwGetWindowUserPointer(window);
    int wid;
    for(wid=0;wid<V->windows.size();wid++) {
        if( V->windows[wid].deleted ) continue;
        if( V->windows[wid].window==window ) break;
    }
    if( V->listener!=NULL ) {
        int width,height;
        glfwGetWindowSize(window, &width, &height);
        V->listener->mouse(V->windows[wid].name.c_str(),x,y,width,height);
    }
}

void GLFWViewer::error_callback( int code, const char *msg) {
    cout << "@@@@@ ERROR "<<code<<" : "<<msg<<endl;
}

//
// quelques fonctions de support
//

// static
void GLFWViewer::initTexture2D( int texNum, cv::Mat img,int clamping )
{
    GLubyte *image;

    //cout << "image size is "<<img.cols<<" x "<<img.rows<<endl;

    /* enable 2D texture mapping */

    glBindTexture(GL_TEXTURE_2D,texNum);
    //cout << "binded to "<<texNum<<endl;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR/*GL_NEAREST*/);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamping);

    /* load texture */
    //cout << "Image elemSize="<<img.elemSize()<<"  single element="<<img.elemSize1()<<endl;

    // as in http://stackoverflow.com/questions/7380773/glteximage2d-segfault-related-to-width-height
    // to fix the odd texture size problem
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    if( img.elemSize1()==1 ) {
        // images 8 bits!
        switch( img.elemSize() ) {
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, img.cols, img.rows,
                         0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img.data);
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols, img.rows,
                         0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.cols, img.rows,
                         0, GL_BGRA, GL_UNSIGNED_BYTE, img.data);
            break;
        }
    }else if( img.elemSize1()==2 ) {
        // images 16 bits!
        switch( img.elemSize() ) {
        case 2:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16, img.cols, img.rows,
                         0, GL_LUMINANCE, GL_UNSIGNED_SHORT, img.data);
            break;
        case 6:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, img.cols, img.rows,
                         0, GL_BGR, GL_UNSIGNED_SHORT, img.data);
            break;
        case 8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, img.cols, img.rows,
                         0, GL_BGRA, GL_UNSIGNED_SHORT, img.data);
            break;
        }
    }else{
        cout << "!!!!! impossible d'utiliser cette image (2d)"<<endl;
    }

    //cout << "done loading image into texture "<<texNum<<endl;
}

void GLFWViewer::initMipMap2D( int texNum, cv::Mat img, unsigned char lod,int clamping )
{
    GLubyte *image;

    //cout << "image size is "<<img.cols<<" x "<<img.rows<<endl;

    /* enable 2D texture mapping */

    glBindTexture(GL_TEXTURE_2D,texNum);
    //cout << "binded to "<<texNum<<endl;

    //not sure this should be set at each frame...
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR_MIPMAP_LINEAR*/GL_LINEAR_MIPMAP_NEAREST/*use nearest level*/);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamping);

    /* load texture */
    //cout << "Image elemSize="<<img.elemSize()<<"  single element="<<img.elemSize1()<<endl;

    // as in http://stackoverflow.com/questions/7380773/glteximage2d-segfault-related-to-width-height
    // to fix the odd texture size problem
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    //not supported on Intel HD4000...
    //glTexStorage2D(GL_TEXTURE_2D, lod, GL_RGBA8, img.cols, img.rows);
    //glTexSubImage2D(GL_TEXTURE_2D, 0​, 0, 0, img.cols, img.rows​, GL_BGRA, GL_UNSIGNED_BYTE, img.data);

    if( img.elemSize1()==1 ) {
        // images 8 bits!
        switch( img.elemSize() ) {
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, img.cols, img.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img.data);
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.cols, img.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, img.data);
            break;
        }
    }else if( img.elemSize1()==2 ) {
        // images 16 bits!
        switch( img.elemSize() ) {
        case 2:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16, img.cols, img.rows, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, img.data);
            break;
        case 6:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_SHORT, img.data);
            break;
        case 8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, img.cols, img.rows, 0, GL_BGRA, GL_UNSIGNED_SHORT, img.data);
            break;
        }
    }else{
        cout << "!!!!! impossible d'utiliser cette image"<<endl;
    }

    glGenerateMipmap(GL_TEXTURE_2D);  //Generate 'lod' number of mipmaps here.

    //cout << "done loading image into mipmap "<<texNum<<endl;
}

// loadFile - loads text file into char* fname
// allocates memory - so need to delete after use
// size of file returned in fSize
// static
std::string GLFWViewer::loadFile(string fname)
{
    std::ifstream file(fname.c_str());
    if(!file.is_open())
    {
        cout << "Unable to open file " << fname << endl;
        return std::string();
    }

    std::stringstream fileData;
    fileData << file.rdbuf();
    file.close();

    return fileData.str();
}

// printShaderInfoLog
// From OpenGL Shading Language 3rd Edition, p215-216
// Display (hopefully) useful error messages if shader fails to compile
// static
void GLFWViewer::printShaderInfoLog(GLint shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    if (infoLogLen > 0)
    {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
        cout << "InfoLog : " << endl << infoLog << endl;
        delete [] infoLog;
    }
}


// assume that the context is already set to the correct window. qid is for info only
// 0 if ok, sinon -1
// static
imgv::parseShader *GLFWViewer::LoadShader(string pfilePath_vs, string pfilePath_fs, GLuint &shaderProgram)
{
    GLuint vertexShader;
    GLuint fragmentShader;

    cout << "loading shader "<<pfilePath_vs<< " & " << pfilePath_fs<<endl;
    cout << "SHADER PATH is "<<IMGV_SHARE<<"shaders/"<<endl;

    shaderProgram=0;
    vertexShader=0;
    fragmentShader=0;

    // load shaders & get length of each
    int vlen;
    int flen;
    std::string vertexShaderStr = loadFile(pfilePath_vs);
    std::string fragmentShaderStr = loadFile(pfilePath_fs);

    if( vertexShaderStr.empty() ) {
        vertexShaderStr=loadFile(IMGV_SHARE+std::string("shaders/")+pfilePath_vs);
    }
    if( fragmentShaderStr.empty() ) {
        fragmentShaderStr=loadFile(IMGV_SHARE+std::string("shaders/")+pfilePath_fs);
    }

    if( vertexShaderStr.empty() ) {
        vertexShaderStr=loadFile(IMGV_INSTALL_PREFIX+pfilePath_vs);
    }
    if( fragmentShaderStr.empty() ) {
        fragmentShaderStr=loadFile(IMGV_INSTALL_PREFIX+pfilePath_fs);
    }

    if(vertexShaderStr.empty()) { return NULL; }
    if(fragmentShaderStr.empty()) { return NULL; }

    vlen = vertexShaderStr.length();
    flen = fragmentShaderStr.length();

    //cout << vertexShaderString <<endl;
    //cout << fragmentShaderString <<endl;

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    cout << "got vertexShader " << vertexShader << " and fragshader " << fragmentShader<<endl;

    const char *vertexShaderCStr = vertexShaderStr.c_str();
    const char *fragmentShaderCStr = fragmentShaderStr.c_str();
    glShaderSource(vertexShader, 1, (const GLchar **)&vertexShaderCStr, &vlen);
    glShaderSource(fragmentShader, 1, (const GLchar **)&fragmentShaderCStr, &flen);


    GLint compiled;

    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
    if(compiled==GL_FALSE) // seb
    {
        cout << "Vertex shader not compiled." << endl;
        printShaderInfoLog(vertexShader);

        glDeleteShader(vertexShader);
        vertexShader=0;
        glDeleteShader(fragmentShader);
        fragmentShader=0;
        return NULL;
    }

    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
    if(compiled==GL_FALSE) // seb
    {
        cout << "Fragment shader not compiled." << endl;
        printShaderInfoLog(fragmentShader);

        glDeleteShader(vertexShader);
        vertexShader=0;
        glDeleteShader(fragmentShader);
        fragmentShader=0;

        return NULL;
    }

    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);

    /**
        uniTex_Loc = glGetUniformLocation(shaderProgram, "myTexture");
        cout << "got unitex="<<uniTex_Loc<<endl;

        extraTex_Loc = glGetUniformLocation(shaderProgram, "extraTex");
        cout << "got extratex="<<extraTex_Loc<<endl;

        paramTex_Loc = glGetUniformLocation(shaderProgram, "param");
        cout << "got param="<<paramTex_Loc<<endl;
**/

    GLint IsLinked;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, (GLint *)&IsLinked);
    if(IsLinked==GL_FALSE) // seb
    {
        cout << "Failed to link shader." << endl;

        GLint maxLength;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
        if(maxLength>0)
        {
            char *pLinkInfoLog = new char[maxLength];
            glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, pLinkInfoLog);
            cout << pLinkInfoLog << endl;
            delete [] pLinkInfoLog;
        }

        glDetachShader(shaderProgram, vertexShader);
        glDetachShader(shaderProgram, fragmentShader);
        glDeleteShader(vertexShader);
        vertexShader=0;
        glDeleteShader(fragmentShader);
        fragmentShader=0;
        glDeleteProgram(shaderProgram);
        shaderProgram=0;

        return NULL;
    }

    //
    // parse les shader pour extraire les uniformes
    //
    imgv::parseShader *ps = new imgv::parseShader(MAX_UNIFORMS_PER_SHADER);
    ps->parse(vertexShaderCStr);
    ps->parse(fragmentShaderCStr);
    ps->dump();

    return ps;
}


//
//
//
#ifdef SKIP

// static
void GLFWViewer::RenderString(float x, float y, void *font, const char* str, float rgb[3])
{  
    char *c;

    glColor3fv(rgb);
    glRasterPos3f(x, y,-10.0f); // -10 a cause du culling
    //for(int i=0;str[i];i++) glutBitmapCharacter(font, str[i]);
    glutBitmapString(font, (const unsigned char *)str);
}
// static
void GLFWViewer::RenderString(text &t) {
    RenderString(t.x,t.y, GLUT_BITMAP_9_BY_15/* GLUT_BITMAP_HELVETICA_18*/,
                 (const char *)t.text.c_str(), t.color);
}

#endif


