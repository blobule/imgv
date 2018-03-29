

#include <imgv/imgv.hpp>

#include <imgv/pluginHomog.hpp>



#include <sys/time.h>

using namespace cv;

//
// homography plugin
//
// fabrique des homographie...
//
// [0] : input image (recycled image)
// [1] : output image and out matrice
//
//

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG   2

pluginHomog::pluginHomog() {
    lastDest=-1;
    // ports
    ports["in"]=Q_IN;
    ports["out"]=Q_OUT;
    ports["log"]=Q_LOG;

    // port directions
    portsIN["in"]=true;
}
pluginHomog::~pluginHomog() { log << "deleted"<<warn; }

void pluginHomog::init() {
    log << "init" << warn;

    currentTag=string("default");

    hInfo hi;

    /*
    hi.source.resize(4);
    hi.source[0]=cv::Point2d(0.0,0.0);
    hi.source[1]=cv::Point2d(1.0,0.0);
    hi.source[2]=cv::Point2d(1.0,1.0);
    hi.source[3]=cv::Point2d(0.0,1.0);

    hi.dest.resize(4);
    hi.dest[0]=cv::Point2d(0.0,0.0);
    hi.dest[1]=cv::Point2d(1.0,0.0);
    hi.dest[2]=cv::Point2d(1.0,1.0);
    hi.dest[3]=cv::Point2d(0.0,1.0);

    //pw=1;ph=1;
    hi.outw=1024;hi.outh=1024;
    hi.view="";
    */
    z[currentTag]=hi;
}


void pluginHomog::uninit() {
    log << "uninit" <<warn;
}

bool pluginHomog::loop() {
    // on process des commandes, tout simplement.
    // en attendant un /go
    pop_front_ctrl_wait(Q_IN);
    return true; // boucle infinie;
}



//
// /tag s tag  : set the current tag
//
// /set/p iff n x y         : start point n (0..3) set to (x,y) for current tag
// /set/p siff tag n x y    : start point n (0..3) set to (x,y) for specified tag
//
// /set/q iff n x y         : end point n (0..3) set to (x,y) for current tag
// /set/q siff tag n x y    : end point n (0..3) set to (x,y) for specified tag
//
// /set/out-size ii w h       : output lookup table size set to (w,h) for current tag
// /set/out-size sii tag w h  : output lookup table size set to (w,h) for specified tag
//
// /up, /down, /left, /right  : move last destination point (/set/q)
//
// /go-view s view  : output lookup for current tag and temporary view
// /go : output for current tag
// /go s tag : output for specified tag
// /go ss tag view : output for specified tag and temporary view
//
// /set/view s view  : set view for current tag
// /set/view ss tag view : set view for specified tag
//
// /set/out/lookup b use-image  : true/false send image out for this tag
// /set/out/lookup sb tag use-image  : true/false send image out for this tag
// /set/out/matrix b use-matrix : true/false send matrix out for this tag
// /set/out/matrix sb tag use-matrix : true/false send matrix out for this tag
//
//
bool pluginHomog::decode(const osc::ReceivedMessage &m) {
    log << "decoding " << m << info;
    int32 k;
    float x,y;
    string tag=currentTag;
    const char *t;

    const char *address=m.AddressPattern();

    if( oscutils::endsWith(address,"/tag") ) {
        m.ArgumentStream() >> t >> osc::EndMessage;
        tag=string(t);
        if( z.find(tag)==z.end() ) {
            // nouveau tag!!!!
            hInfo hi;
            z[tag]=hi;
        }
        // set this tag as default
        currentTag=tag;
    }else if( oscutils::endsWith(address,"/set/p") ) {
        if( m.ArgumentCount()==3 ) {
            m.ArgumentStream() >> k >> x >> y >> osc::EndMessage;
        }else{
            m.ArgumentStream() >> t >> k >> x >> y >> osc::EndMessage;
            tag=string(t);
        }
        if( k>=0 && k<=3 && z.find(tag)!=z.end() ) {
            z[tag].source[k]=cv::Point2d(x,y);
            go(tag);
        }
    }else if( oscutils::endsWith(address,"/set/q") ) {
        if( m.ArgumentCount()==3 ) {
            m.ArgumentStream() >> k >> x >> y >> osc::EndMessage;
        }else{
            m.ArgumentStream() >> t >> k >> x >> y >> osc::EndMessage;
            tag=string(t);
        }
        if( k>=0 && k<=3 && z.find(tag)!=z.end() ) {
            z[tag].dest[k]=cv::Point2d(x,y);
            lastTag=tag;lastDest=k;
            go(tag);
        }
    }else if( oscutils::endsWith(address,"/set/out-size") ) {
        int32 w,h;
        if( m.ArgumentCount()==2 ) {
            m.ArgumentStream() >> w >> h >> osc::EndMessage;
        }else{
            m.ArgumentStream() >> t >> w >> h >> osc::EndMessage;
            tag=string(t);
        }
        if( z.find(tag)!=z.end() ) {
            z[tag].outw=w;z[tag].outh=h;
        }
    }else if( oscutils::endsWith(address,"/set/out/lookup") ) {
        bool b;
        if( m.ArgumentCount()==2 ) {
            m.ArgumentStream() >> t >> b >> osc::EndMessage;
            tag=string(t);
        }else{
	    m.ArgumentStream() >> b >> osc::EndMessage;
	}
        if( z.find(tag)!=z.end() ) z[tag].useLookup=b;
    }else if( oscutils::endsWith(address,"/set/out/matrix") ) {
        bool b;
        if( m.ArgumentCount()==2 ) {
            m.ArgumentStream() >> t >> b >> osc::EndMessage;
            tag=string(t);
        }else{
	    m.ArgumentStream() >> b >> osc::EndMessage;
	}
        if( z.find(tag)!=z.end() ) z[tag].useMatrix=b;
    }else if( oscutils::endsWith(address,"/set/out/matrix-cmd") ) {
        bool b;
        ReceivedMessageArgumentStream as=m.ArgumentStream();
        if( m.ArgumentCount()==2 ) {
            as >> t;
            tag=string(t);
        }
        as >> t >> osc::EndMessage;
	log << "setting matrixcmd of tag " << tag << " to " << t << err;
        if( z.find(tag)!=z.end() ) z[tag].matrixCmd=string(t);
    }else if( oscutils::endsWith(address,"/up") ) {
        if( lastDest>=-1 ) {
            z[lastTag].dest[lastDest]=z[lastTag].dest[lastDest]+Point2d(0.0,-0.002);
            go(tag);
        }
    }else if( oscutils::endsWith(address,"/down") ) {
        if( lastDest>=-1 ) {
            z[lastTag].dest[lastDest]=z[lastTag].dest[lastDest]+Point2d(0.0,0.002);
            go(tag);
        }
    }else if( oscutils::endsWith(address,"/left") ) {
        if( lastDest>=-1 ) {
            z[lastTag].dest[lastDest]=z[lastTag].dest[lastDest]+Point2d(-0.002,0.0);
            go(tag);
        }
    }else if( oscutils::endsWith(address,"/right") ) {
        if( lastDest>=-1 ) {
            z[lastTag].dest[lastDest]=z[lastTag].dest[lastDest]+Point2d(0.002,0.0);
            go(tag);
        }
    }else if( oscutils::endsWith(address,"/go-view") ) {
        // current tag, but with temporary view
        const char *v;
        m.ArgumentStream() >> v >> osc::EndMessage;
        go(string(currentTag),v);
    }else if( oscutils::endsWith(address,"/go") ) {
        // go, go tag, go tag view
        if( m.ArgumentCount()==1 ) {
            m.ArgumentStream() >> t >> osc::EndMessage;
            go(string(t));
        }else if( m.ArgumentCount()==2 ) {
            const char *v;
            m.ArgumentStream() >> t >> v >> osc::EndMessage;
            go(string(t),v);
        }else{
            go(tag);
        }
    }else if( oscutils::endsWith(address,"/set/view") ) {
        const char *newview;
        if( m.ArgumentCount()==1 ) {
            m.ArgumentStream() >> newview >> osc::EndMessage;
        }else{
            m.ArgumentStream() >> t >> newview >> osc::EndMessage;
            tag=string(t);
        }
        if( z.find(tag)!=z.end() ) {
            z[tag].view=newview;
        }
    }else{
        log << "unknown command: "<<m<<err;
        return false;
    }
    return true;
}

void pluginHomog::go(string tag,const char *tmp) {
    log << "go! (tag="<<tag<<")"<<warn;
    if( z.find(tag)==z.end() ) return;
    blob *i1=NULL;
    if( z[tag].useLookup ) i1=pop_front_noctrl_wait(Q_IN); // esperons que ca ne sera pas long..
    Matx33d m33;
    doOnePattern(z[tag],i1,m33);
    if( z[tag].useLookup ) {
        if( tmp!=NULL ) {
            i1->view=string(tmp);
        }else if( !z[tag].view.empty() ) {
            i1->view=z[tag].view;
        }
        i1->n=n;
        push_back(Q_OUT,&i1);
        n++;
    }
    if( z[tag].useMatrix ) {
	// log does not compile on Pi
        //log << "homo is "<< m33 <<warn;
        for(int i=0;i<9;i++) log << i << ":" << m33.val[i] <<info;
        message *m=getMessage(256);
        if( z[tag].matrixCmd.empty() ) {
        	string yo="/homography/"+tag;
            *m << osc::BeginMessage(yo.c_str());
        }else{
            *m << osc::BeginMessage(z[tag].matrixCmd.c_str());
        }
    // send transposed, to make opengl happy
        *m << (float)m33.val[0];
        *m << (float)m33.val[3];
        *m << (float)m33.val[6];
        *m << (float)m33.val[1];
        *m << (float)m33.val[4];
        *m << (float)m33.val[7];
        *m << (float)m33.val[2];
        *m << (float)m33.val[5];
        *m << (float)m33.val[8];
        *m <<osc::EndMessage;
        push_back_ctrl(Q_OUT,&m);

	///// special send also /mat3/proj1/A/homog 
	/*****
	if( !z[tag].matrixCmd.empty() ) {
    		Matx33d m33i(m33.inv());
		message *m=getMessage(256);
            	*m << osc::BeginMessage("/mat3/proj1/A/homog");
		*m << (float)m33i.val[0];
		*m << (float)m33i.val[3];
		*m << (float)m33i.val[6];
		*m << (float)m33i.val[1];
		*m << (float)m33i.val[4];
		*m << (float)m33i.val[7];
		*m << (float)m33i.val[2];
		*m << (float)m33i.val[5];
		*m << (float)m33i.val[8];
        	*m <<osc::EndMessage;
        	push_back_ctrl(Q_OUT,&m);
	}
	*****/
    }
}


void pluginHomog::doOnePattern(hInfo &hi,Mat *m,Matx33d &m33) {
    //getCheckerBoard(n,m);

    for(int i=0;i<4;i++) {
        log << "src "<<hi.source[i]<<" dest "<<hi.dest[i]<<info;
    }

    std::vector<cv::Point2d> srcRef;
    srcRef.resize(4);
    srcRef[0]=cv::Point2d(0.0,0.0);
    srcRef[1]=cv::Point2d(1.0,0.0);
    srcRef[2]=cv::Point2d(1.0,1.0);
    srcRef[3]=cv::Point2d(0.0,1.0);


    Matx33d Ho(findHomography( hi.source,hi.dest ));
    Matx33d HoRef(findHomography( srcRef,hi.dest )); // pour le masque
    //Matx33d Ho(getPerspectiveTransform( source,dest ));
    Matx33d Hoi(Ho.inv());
    Matx33d HoRefi(HoRef.inv());
    //std::cout << Ho << endl;
    //std::cout << Hoi << endl;

    m33=Hoi; // ou clone?

    // LUT
    if( m!=NULL ) {
        m->create(Size(hi.outw,hi.outh), CV_16UC3);
        unsigned short *p=(unsigned short *)m->data;
        int x,y;
        Point3d from;
        Point3d fromRef;
        for(y=0;y<m->rows;y++) {
            for(x=0;x<m->cols;x++,p+=3) {
                double xx=(double)x/(m->cols-1);
                double yy=(double)y/(m->rows-1);
                from=Hoi*Point3d(xx,yy,1.0);
                fromRef=HoRefi*Point3d(xx,yy,1.0);
                // normalize from
                if( from.z!=0.0 ) { from.x/=from.z;from.y/=from.z;from.z=1.0; }
                if( fromRef.z!=0.0 ) { fromRef.x/=fromRef.z;fromRef.y/=fromRef.z;fromRef.z=1.0; }
                //log << "val("<<x<<","<<y<<") is "<<from<<info;
                if( fromRef.x<0.0  || fromRef.y<0.0
                        || fromRef.x>1.0  || fromRef.y>1.0
                        || from.x<0.0  || from.y<0.0
                        || from.x>1.0  || from.y>1.0 ) { p[0]=0; }
                else p[0]=65535;
                p[2]=from.x*65535;
                p[1]=from.y*65535;
            }
        }
    }
}


/*
void pluginHomog::dump(ofstream &file) {
        file << "P"<<name << " [shape=\"Mrecord\" label=\"{<0>0|"<<name<<"|<1>1}\"];\n";
        if( !badpos(0) ) file << "Q"<<Qs[0]->name << " -> " << "P"<<name <<":0" <<endl;
        if( !badpos(1) ) file << "P"<<name <<":1" << " -> " << "Q"<<Qs[1]->name <<endl;
    // .. a completer
}
*/


/*
void pluginHomog::getCheckerBoard(int n,Mat &m) {
    m.create(Size(outw,outh), CV_8UC1);
    int x,y;
    unsigned char *p=m.data;
    for(y=0;y<m.rows;y++) {
        int yy=((y/16)%2)^(n%2);
        for(x=0;x<m.cols;x++) {
            int xx=(x/16)%2;
            //m.at<uchar>(y,x)=(xx^yy)?255:0;
            *p++=(xx^yy)?255:0;
        }
    }
}
*/






