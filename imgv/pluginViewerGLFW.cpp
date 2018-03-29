
//
// plugin viewer GLFW
//
//

#include <iostream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include <stdio.h>

#include <pluginViewerGLFW.hpp>



// pour changer le nom du thread
//#include <sys/prctl.h>

#include <sys/time.h>

/// Q_RECYCLE is for when we output the content of fbo

#define Q_IN	0
#define Q_OUT	1
#define Q_RECYCLE	2
#define Q_LOG	3


#undef VERBOSE
//#define VERBOSE


pluginViewerGLFW::pluginViewerGLFW() {
    started=false;/*GLFWViewer::init();*/
    manual=false;rendered=false;
    // ports
    ports["in"]=Q_IN;
    ports["out"]=Q_OUT;
    ports["recycle"]=Q_RECYCLE; // in case we output stuff
    ports["log"]=Q_LOG; // "log" is auto-detected.

    // ports direction
    portsIN["in"]=true;
    portsIN["recycle"]=true;

    uidDisplayed=new blob*[NBUID];
}


pluginViewerGLFW::~pluginViewerGLFW() {
    log << "delete" <<warn;
    delete [] uidDisplayed;
}

int pluginViewerGLFW::clampDecode(const char *clamping) {
	int clamp=GL_CLAMP_TO_BORDER;
	if( strcmp(clamping,"clamp-to-border")==0 ) clamp=GL_CLAMP_TO_BORDER;
	else if( strcmp(clamping,"clamp-to-edge")==0 ) clamp=GL_CLAMP_TO_EDGE;
	else{
		log << "new-quad : unexpected clamp option " << clamping<<" . expected clamp-to-border or clamp-to-edge. Assuming clamp-to-border" << err;
		clamp=GL_CLAMP_TO_BORDER;
	}
	return clamp;
}

bool pluginViewerGLFW::decode(const osc::ReceivedMessage &m) {
    //log << "decoding: " << m <<info;
    if( plugin::decode(m) ) return true; // verifie pour le /defevent


    // tout le reste est reserve a apres le start...
    if( !started ) { log << "command not executed: plugin not started."<<err; return true; }

    // we are started. Ok to create windows and other stuff

    const char *address=m.AddressPattern();
    //
    // .../new-window/[winname] offx offy w h deco title
    // .../new-quad/[winname]/[quadname] x1 x2 y1 y2 z shader [clamping]
    // .../set/[winname]/[quadname]/[uniname] val
    //
    // .../del-window/[winname]
    // .../del-quad/[winname]/[quadname]
    //
    int nb=oscutils::extractNbArgs(address);
    int pos,len;

    // on commence par la commande la plus frequente
    if( nb>=4 && oscutils::matchArg(address,3,"set") ) {
        oscutils::extractArg(address,2,pos,len);
        string winName(address+pos,len);
        oscutils::extractArg(address,1,pos,len);
        string quadName(address+pos,len);
        oscutils::extractArg(address,0,pos,len);
        string uniName(address+pos,len);
        // check type of argument
        // for now: only float and int
        osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
        float v;
        if( arg->IsFloat() ) {
            V.setUniformData(winName,quadName,uniName,arg->AsFloat());
            changed=true;
        }else if( arg->IsInt32() ) {
            V.setUniformData(winName,quadName,uniName,(int)arg->AsInt32());
            changed=true;
        }else{
            log << "uniform param bad type:" << m<<err;
        }
    }else if( nb>=4 && oscutils::matchArg(address,3,"vec4") ) {
        oscutils::extractArg(address,2,pos,len);
        string winName(address+pos,len);
        oscutils::extractArg(address,1,pos,len);
        string quadName(address+pos,len);
        oscutils::extractArg(address,0,pos,len);
        string uniName(address+pos,len);
        // check type of argument
        // for now: only float and int
        osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
        float v[4];
        for (int i=0;i<4;i++)
        {
            v[i]=(arg++)->AsFloat();
        }
        V.setUniformData(winName,quadName,uniName,v,4);
        changed=true;
    }else if( nb>=4 && oscutils::matchArg(address,3,"mat4") ) {
        oscutils::extractArg(address,2,pos,len);
        string winName(address+pos,len);
        oscutils::extractArg(address,1,pos,len);
        string quadName(address+pos,len);
        oscutils::extractArg(address,0,pos,len);
        string uniName(address+pos,len);
        // check type of argument
        // for now: only float and int
        osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
        float v[16];
        for (int i=0;i<16;i++) v[i]=(arg++)->AsFloat();
        V.setUniformData(winName,quadName,uniName,v,16);
        changed=true;
    }else if( nb>=4 && oscutils::matchArg(address,3,"mat3") ) {
        oscutils::extractArg(address,2,pos,len);
        string winName(address+pos,len);
        oscutils::extractArg(address,1,pos,len);
        string quadName(address+pos,len);
        oscutils::extractArg(address,0,pos,len);
        string uniName(address+pos,len);
        // check type of argument
        // for now: only float and int
        osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
        float v[9];
        for (int i=0;i<9;i++) v[i]=(arg++)->AsFloat();
        V.setUniformData(winName,quadName,uniName,v,9);
        changed=true;
    }else if( nb>=2 && oscutils::matchArg(address,1,"new-window") ) {
        // /new-window/[winname] offx offy w h deco title
        oscutils::extractArg(address,0,pos,len);
        string winName(address+pos,len);
        int32 offx,offy,w,h;
        bool decor;
        const char *title;
        m.ArgumentStream() >> offx >> offy >> w >> h >> decor >> title >> osc::EndMessage;
        log << "new window '"<<winName<<"'"<<info;
        V.addWindow(winName,offx,offy,w,h,decor,title);
    }else if( nb>=2 && oscutils::matchArg(address,1,"new-monitor") ) {
        // /new-monitor/[winname] {monitorname]
        oscutils::extractArg(address,0,pos,len);
        string winName(address+pos,len);
        osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
        if( arg == m.ArgumentsEnd() ) {
            V.addMonitor(winName);
        }else{
            V.addMonitor(winName,arg->AsString());
        }
        log << "new monitor '"<<winName<<"'"<<info;
    }else if( nb>=3 && oscutils::matchArg(address,1,"hide") ) {
        // /hide/[winname] true/false
        oscutils::extractArg(address,0,pos,len);
        string winName(address+pos,len);
        bool status;
        m.ArgumentStream() >> status >> osc::EndMessage;
        log << "hide window '"<<winName<<"' = "<< status << info;
        V.hideWindow(winName,status);
    }else if( nb>=3 && oscutils::matchArg(address,2,"new-quad") ) {
        // /new-quad/[winname]/[quadname] x1 x2 y1 y2 z shader [clamping]
        oscutils::extractArg(address,1,pos,len);
        string winName(address+pos,len);
        oscutils::extractArg(address,0,pos,len);
        string quadName(address+pos,len);
        float x1,x2,y1,y2,z;
        const char *shader;
	int clamp=GL_CLAMP_TO_BORDER;
	if( m.ArgumentCount()==6 ) {
		m.ArgumentStream() >> x1 >> x2 >> y1 >> y2 >> z >> shader >> osc::EndMessage;
	}else{
        	const char *clamping; // by default
		m.ArgumentStream() >> x1 >> x2 >> y1 >> y2 >> z >> shader >> clamping >> osc::EndMessage;
		clamp=clampDecode(clamping);
	}
        log << "new quad '"<<quadName<<"' in window '"<<winName<<"'"<< info;
        V.addQuad(winName,quadName,x1,x2,y1,y2,z,shader,clamp);
    }else if( nb>=3 && oscutils::matchArg(address,2,"new-fbo") ) {
        // /new-fbo/[winname]/[fboname] xres yres texname output shader
        oscutils::extractArg(address,1,pos,len);
        string winName(address+pos,len);
        oscutils::extractArg(address,0,pos,len);
        string fboName(address+pos,len);
        int32 xres,yres,nbbytes;
        const char *texname;
        const char *shader;
	const char *clamping;
        m.ArgumentStream() >> xres >> yres >> texname >> nbbytes >> shader >> clamping >> osc::EndMessage;
        log << "new fbo '"<<fboName<<"' in window '"<<winName<<"'"<< info;
        V.addFbo(winName,fboName,xres,yres,texname,nbbytes,shader,clampDecode(clamping));
    }else if( nb>=2 && oscutils::matchArg(address,1,"del-window") ) {
        // /del-window/[winname]
        oscutils::extractArg(address,0,pos,len);
        string winName(address+pos,len);
        m.ArgumentStream() >> osc::EndMessage;
        log << "del window '"<<winName<<"'"<<info;
        V.delWindow(winName);
    }else if( nb>=3 && (oscutils::matchArg(address,2,"del-quad") || oscutils::matchArg(address,2,"del-fbo")) ) {
        // /del-quad/[winname]/[quadname]
        oscutils::extractArg(address,1,pos,len);
        string winName(address+pos,len);
        oscutils::extractArg(address,0,pos,len);
        string quadName(address+pos,len);
        m.ArgumentStream() >> osc::EndMessage;
        log << "del quad '"<<winName<<"'/'"<<quadName<<"'"<<info;
        V.delDrawable(winName,quadName);
    }else if( nb>=3 && oscutils::matchArg(address,2,"hmd") ) {
        oscutils::extractArg(address,1,pos,len);
        string winName(address+pos,len);
        oscutils::extractArg(address,0,pos,len);
        string drawName(address+pos,len);
		osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		bool b=arg->AsBool();
		int did=V.findDrawable(winName,drawName);
        log << "hmd of drawable "<<did<<" set to "<<b<<info;
		V.setHmdTexture(did,b);
    }else if ( oscutils::endsWith(address,"/manual") ) {
        m.ArgumentStream() >> manual >> osc::EndMessage;
        log << "manual mode is '"<<manual<<"'"<< info;
    }else if( oscutils::endsWith(address,"/kill") ) {
        kill=true;
    }else if( oscutils::endsWith(address,"/render") ) {
        if (manual) { V.drawAll(); }
    }else if( oscutils::endsWith(address,"/output") ) {
        const char *drawName;
        const char *portName;
        m.ArgumentStream() >> drawName >> portName >> osc::EndMessage;
        output(drawName,portName);
    }else if( oscutils::endsWith(address,"/reset-counter") ) {
        n=0;
    }else if( oscutils::endsWith(address,"/alias") ) {
        const char *uniName;
        const char *fboName;
        m.ArgumentStream() >> uniName >> fboName >> osc::EndMessage;
        V.addAlias(uniName,fboName);
    }else if( oscutils::endsWith(address,"/nbdraws") ) {
        const char *fboName;
        int32 nbdraws;
        m.ArgumentStream() >> fboName >> nbdraws >> osc::EndMessage;
        V.setNbDraws(fboName,nbdraws);
    }else if( oscutils::endsWith(address,"/no-draw-sleep") ) {
        m.ArgumentStream() >> noDrawSleep >> osc::EndMessage;
    }else if( oscutils::endsWith(address,"/redirect") ) {
        const char *uniNameFrom;
        const char *uniNameTo;
	if( m.ArgumentCount()==2 ) {
		m.ArgumentStream() >> uniNameFrom >> uniNameTo >> osc::EndMessage;
		V.setRedirect(uniNameFrom,uniNameTo);
            	changed=true;
	}else{
		m.ArgumentStream() >> uniNameFrom >>osc::EndMessage;
		V.unsetRedirect(uniNameFrom);
		changed=true;
	}
    }else{
        stringstream ss;
        log << "unknown message: "<<m<<err;
    }

    return true;
}

void pluginViewerGLFW::init() {
    // initialize glfw
    GLFWViewer::init(); // normalement pas dans le thread principal
    //std::cout << "[viewerGLFW] init (thread=" << pthread_self() << ")"<<std::endl;
    // log
    //log << "init! " << std::hex << pthread_self() <<warn;

    // set the process name!
    //prctl(PR_SET_NAME,("imgv:"+name).c_str(),0,0,0);

    // le nom de la fenetre doit correspondre au "view" de l'image
    /*
    wid0=V.addWindow(offx,offy,w,h,decor?1:0,wname);
    qid0=V.addQuad(wid0,0.0,1.0,0.0,1.0,-10.0,shader);
    uid0=V.addUniformSampler(qid0,"tex");
    float color[3] = {1,1,0};
    tid0=V.addText(wid0,0,10,color,"Bonjour, ca roule?");
    color[0]=0;
    tid1=V.addText(wid0,100,100,color,"(x,y)");
    */
    kill=false;
    V.start();
    started=true;

    noDrawSleep=8000;

    //
    // quelques callbacks
    //
    V.setListener(this); // pour les key(), mouse(), etc...

    n=0; // current frame
}

void pluginViewerGLFW::uninit() {
    log << "uninit!" << warn;
    V.stop();
}

bool pluginViewerGLFW::loop() {
    if( kill ) return false; // The End!
    static unsigned int lastusec=0;

    struct timeval tv;
    double tick1,tick2;
    gettimeofday(&tv,NULL);
    tick1=tv.tv_sec+tv.tv_usec/1000000.0;

    changed=false;
    if (manual)
    {
        for(int i=0;;i++)
        {
            blob *i1=pop_front_nowait(0); // special pas d'attente
            if( i1==NULL ) { break; }
            int uid=V.getUniformFromView(i1->view);
            if( uid<0 ) {
                recycle(i1); // uniform illegal. recycle!
                continue;
            }
            V.setUniformData(uid,*i1); //render is done in decode()
            push_back(1,&i1); // sera probablement recyclee
            glfwPollEvents();
        }
    }
    else //automatic mode
    {
        // reset l'auto-skip
        for(int i=0;i<NBUID;i++) uidDisplayed[i]=NULL;
        for(int i=0;;i++) {
            // si on met wait ici, on ne voit plus le clavier ou souris
            blob *i1=pop_front_nowait(0); // special pas d'attente
            if( i1==NULL ) { break; }
#ifdef VERBOSE
            log<<"got image view="<<i1->view<<info;
#endif

            int uid=V.getUniformFromView(i1->view);
#ifdef VERBOSE
            log << "view "<<i1->view<<" give uid="<<uid<<warn;
#endif
            if( uid<0 ) {
                push_back(1,&i1);
                continue;
            }
            // auto-skip: l'image a deja ete affichee?
            if( uid<NBUID ) {
                if( uidDisplayed[uid]!=NULL ) {
                    // flush the old image
                    log << "skip! " << uidDisplayed[uid]->view << warn;
                    push_back(1,&(uidDisplayed[uid]));
                }
                // keep latest image
                uidDisplayed[uid]=i1;
            }else{
                log << "Attention! NBUID too low for uid="<<uid<<err;
            }
        }

        for(int uid=0;uid<NBUID;uid++) {
            if( uidDisplayed[uid]==NULL ) continue;
            //log << "passed autoskip" <<warn;
#ifdef VERBOSE
	/*
            i1=uidDisplayed[uid];
            stringstream ss;
            ss << setw(20) << setprecision(15) << i1->view << " : " << i1->timestamp << " delta="<<(i1->timestamp - last)<<std::endl;
            log << ss << info;
            last=i1->timestamp;
	*/
#endif
            // affiche l'image
            // normalement, on utilise le parametre uniform dans l'image
            // pour l'instant, premier uniforme de type image
            // if empty V.setUniformData(*i1);
#ifdef VERBOSE
		log << "affiche image uid="<<uid<<info;
#endif
            V.setUniformData(uid,*(uidDisplayed[uid]));
            changed=true;
            push_back(1,&(uidDisplayed[uid])); // sera probablement recyclee
        }
    }
#ifdef VERBOSE
	log << "changed is "<<changed<<info;
#endif
    if( changed )
    //if( !manual )
    {
        V.drawAll();

        //gettimeofday(&tv,NULL);
        //tick2=tv.tv_sec+tv.tv_usec/1000000.0;
        //printf("Time to draw: %f\n",tick2-tick1);
    }
    //else usleepSafe(noDrawSleep);

    glfwPollEvents();

    return true;
}

int pluginViewerGLFW::output(string drawName,string portName) 
{
    int uid;
    int did=V.getDrawableFromView(drawName);
    int port=portMap(portName);
    if(did>=0 && port>=0)
    {
        blob *i1=pop_front_wait(Q_RECYCLE); // esperons que ca ne sera pas long...
        V.readDrawableFbo(did,i1);
        //i1->create(10,10,CV_8UC1);
        i1->n=n;
        i1->timestamp=timestamp();
        uid=V.getUniformFromView("/main/A/yaw");
        float yaw=V.getUniformFloatValue(uid);
        uid=V.getUniformFromView("/main/A/pitch");
        float pitch=V.getUniformFloatValue(uid);
        //printf("%f %f\n",yaw,pitch);
        i1->yaw=yaw;
        i1->pitch=pitch;
        i1->view=V.getDrawableView(did);
        push_back(port,&i1);
        n++; // compte les outputs
        return 0;
    }
    return -1;
}

double pluginViewerGLFW::timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec+tv.tv_usec/1000000.0;
}

//
// fonction du GLFWViewerListener
// -> x,y : mouse position, w,h : window widht/height
//
void pluginViewerGLFW::key(const char *wname,int k,double x,double y,int w,int h) {
    //log << "KEY!!! win="<<wname<<" ("<<k<<") isalnum="<<isalnum(k)<<warn;

    // ou est la souris?
    //double xpos,ypos;
    //int w,h;
    //V.getMouse(wid,xpos,ypos,w,h);
    //
    // send OSC messages
    //
    // on va avoir une heure de reference pour les message
    // ca garanti qu'on garde tout le monde en sync
    //
    struct timeval tv;
    gettimeofday(&tv,NULL);
    double now=tv.tv_sec+tv.tv_usec/1000000.0;
    string event;
    switch(k) {
    case GLFW_KEY_UP: event="up";break;
    case GLFW_KEY_DOWN: event="down";break;
    case GLFW_KEY_LEFT: event="left";break;
    case GLFW_KEY_RIGHT: event="right";break;
    case GLFW_KEY_ESCAPE: event="esc";break;
    default:
        if( isprint(k) ) event=string(1,(char)k);
        else event="unkown-key";
    }
    //log << "event is "<<event<<warn;
    checkOutgoingMessages(event,now,(int)(x+0.5),(int)(y+0.5),w,h,wname);
}


void pluginViewerGLFW::mouse(const char *wname,double x,double y,int w,int h) {
    //log << "mouse!!! win="<<wname<<"("<<x<<","<<y<<")"<<warn;
    //
    // send OS messages
    //
    struct timeval tv;
    gettimeofday(&tv,NULL);
    double now=tv.tv_sec+tv.tv_usec/1000000.0;
    checkOutgoingMessages("mouse",now,(int)(x+0.5),(int)(y+0.5),w,h,wname);
    /*
    char buf[100];
    sprintf(buf,"(%d,%d)",x,y);
    V.setText(tid1,string(buf));
    V.setText(tid1,(float)x,(float)y);
    */
}

// on doit redefinir le stop...
// attention, ceci est appelle du thread principal, pas celui du plugin
// mais leavemainLoop est threadsafe
void pluginViewerGLFW::stop() {
    log << "stop!"<<warn;
    // averti idle qu'on veut terminer...
    kill=true;
    // on attend simplement que le thread finisse naturellement.
    pthread_join(tid,NULL);
    running=0;
}


/*
void pluginViewerGLFW::dump(ofstream &file) {
        //file << "P"<<name << " [shape=\"Mrecord\" label=\"{<0>0|"<<name<<"|<1>1}\"];" << endl;
        file << "P"<<name << " [shape=\"Mrecord\" label=\"{<0>0|{"<<name<<"|{";
        bool first=true;
        for(int i=2;i<Qs.size();i++) {
            if( !badpos(i) ) { file << (first?"":"|")<<"<"<<i<<">"<<i;; first=false; }
        }
        file <<"}}|<1>1}\"];" << endl;
        if( !badpos(0) ) file << "Q"<<Qs[0]->name << " -> " << "P"<<name <<":0" <<endl;
        if( !badpos(1) ) file << "P"<<name <<":1" << " -> " << "Q"<<Qs[1]->name <<endl;
    plugin<blob>::dump(file); // pour le triage
}
*/



