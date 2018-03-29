

#include <imgv/imgv.hpp>

#include <imgv/pluginRead.hpp>

#include <imgv/imqueue.hpp>

#include <sys/time.h>

using namespace cv;

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2

#define PLUGIN_READ_MAX_BUFFER_SIZE 16000000

pluginRead::pluginRead() {
	// ports
	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=Q_LOG;

	portsIN["in"]=true;
    internal_buffer=NULL;
}

pluginRead::~pluginRead() {
  log << "deleted"<<warn; 
  if (internal_buffer) {
    free(internal_buffer);
    internal_buffer=NULL;
  }
}

void pluginRead::init() {
    pause=false;
    read_one_frame=false;
    count=0;
    apply_at=-1;
    apply_offset=0;
    noffset=0;
    nmin=0;
    nmax=-1;
    file_pattern="";
    view="";
    initializing=true;
    convertToGray=false;
}

void pluginRead::uninit() {
	log << "uninit"<<warn;
}

bool pluginRead::loop() {
    int ncount=count+noffset;
    if (apply_at>=0) {
      if (ncount>=apply_at) {
        if (apply_offset==0) {
          pause=true;
          read_one_frame=true; // play this frame
          //cout << "[read] pause!" << endl;
        }else{
          count+=apply_offset;
          if (count<0) count=0;      
          read_one_frame=true;
          //cout << "[read] offset!" << endl;
        }
        apply_at=-1;
        apply_offset=0;
      }else{
        read_one_frame=true;
      }
    }

    if( initializing || (pause && !read_one_frame) ) {
        log << "[read] " << "waiting for ctrl (" << count << ")" << info;
		pop_front_ctrl_wait(Q_IN); // attend et process tous les messages
		return true;
    }
    int n=nmin+count;
    if (nmax>=0 && n>=nmax)
    {
	  // we are done.
	  // return to initialization, in case we want to restart the sequence
	  initializing=true;
	  // and reset counter
	  count=0;
      return true;
      //return false;
    }
    if (read_one_frame) log << "[read] " << n << " (" << count << ", " << apply_at << ")" << info;

    char filename[1024];
	// attend une image en entree
	blob *i1=pop_front_wait(Q_IN);

    blob img_in;
    sprintf(filename,file_pattern.c_str(),n);
    char *c=strrchr(filename,'.');
    if (c!=NULL && strcasecmp(c,".png")==0)
    {
      cv::FileStorage file_info;
      img_in=blob::pngread(filename,&file_info);
      file_info["timestamp"]>>img_in.timestamp;
      file_info["yaw"]>>img_in.yaw;
      file_info["pitch"]>>img_in.pitch;
      //log << "[read] " << filename << ": timestamp is " << img_in.timestamp << info;
      file_info.release();
    }
    else if (c!=NULL && (strcasecmp(c,".ppm")==0 || strcasecmp(c,".pgm")==0))
    {
      cv::FileStorage file_info;
      img_in=blob::pbmread(filename,&file_info);
      file_info["timestamp"]>>img_in.timestamp;
      file_info["yaw"]>>img_in.yaw;
      file_info["pitch"]>>img_in.pitch;
      //log << "[read] " << filename << ": timestamp is " << img_in.timestamp << info;
      file_info.release();
    }
    else if (c!=NULL && strcasecmp(c,".lz4")==0 && internal_buffer!=NULL)
    {
      cv::FileStorage file_info;
      img_in=blob::lz4read(filename,&file_info,internal_buffer);
      file_info["timestamp"]>>img_in.timestamp;
      file_info["yaw"]>>img_in.yaw;
      file_info["pitch"]>>img_in.pitch;
      if (n%100==0) log << "[read] " << filename << ": timestamp is " << img_in.timestamp << info;
      file_info.release();
    }
    else
    {
      img_in=cv::imread(filename,-1);
      img_in.timestamp=0;
    }
    if(img_in.empty())
    {
      log << "[Read] Unable to load "<<filename<<err;
      push_back(-1,&i1); //recycle...
      initializing=true;
	  // and reset counter
	  count=0;
      return true;
      //return false;
    }
    if( convertToGray && img_in.channels()>1 ) {
	cv::cvtColor(img_in,*i1, CV_BGR2GRAY);
    }else{
	    img_in.copyTo(*i1);
    }
    i1->timestamp=img_in.timestamp;
    //log << "Loading " << filename << " count="<<count<<info;
    i1->view=view;
    i1->n=n;
    push_back(Q_OUT,&i1);

    read_one_frame=false;
    count++;
    //log << "[read] count is now " << count << info;

	return true;
}

int pluginRead::getCounter() {
  return count+noffset;
}

bool pluginRead::isPaused() {
  return pause;
}

bool pluginRead::decode(const osc::ReceivedMessage &m) {
	//cout << "decoding " << m << endl;
	const char *address=m.AddressPattern();
	if( oscutils::endsWith(address,"/view") ) {
		const char *v;
		m.ArgumentStream() >> v >> osc::EndMessage;
        view=string(v);
	}else if( oscutils::endsWith(address,"/file") ) {
		const char *f;
		m.ArgumentStream() >> f >> osc::EndMessage;
        file_pattern=string(f);
	}else if( oscutils::endsWith(address,"/offset") ) {
		m.ArgumentStream() >> noffset >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/min") ) {
		m.ArgumentStream() >> nmin >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/max") ) {
		m.ArgumentStream() >> nmax >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/internal-buffer") ) {
		int32 size;
		m.ArgumentStream() >> size >> osc::EndMessage;
		if (internal_buffer!=NULL) free(internal_buffer);
		if (size>=0 && size<=PLUGIN_READ_MAX_BUFFER_SIZE) internal_buffer=(char *)(malloc(sizeof(char)*size));
	}else if( oscutils::endsWith(address,"/reinit") ) {
        count=0;
        apply_at=-1;
        apply_offset=0;
		initializing=true;
	}else if( oscutils::endsWith(address,"/init-done") ) {
		initializing=false;

	}else if( oscutils::endsWith(address,"/play") ) {
        	apply_at=-1;
        	apply_offset=0;
        	pause=false;        
	}else if( oscutils::endsWith(address,"/pause") ) {
		m.ArgumentStream() >> apply_at >> osc::EndMessage;
		apply_offset=0;
	}else if( oscutils::endsWith(address,"/next") ) {
		m.ArgumentStream() >> osc::EndMessage;
		if( !pause ) { log << "/next has no effect when not in pause" << err; }
		else{
			// add 1 to the count, regardless of the current count
			apply_at=0;
			apply_offset=1;
		}
	}else if( oscutils::endsWith(address,"/count-add") ) {
		m.ArgumentStream() >> apply_at >> apply_offset >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/convert-to-gray") ) {
		m.ArgumentStream() >>  osc::EndMessage;
		convertToGray=true;
	}else if( oscutils::endsWith(address,"/no-conversion") ) {
		m.ArgumentStream() >>  osc::EndMessage;
		convertToGray=false;
	}else{
		log << "unknown command: "<<m<<warn;
		return false;
	}
	return true;
}



void pluginRead::dump(ofstream &file) {
}



