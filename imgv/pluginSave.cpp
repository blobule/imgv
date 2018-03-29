
#include <imgv/pluginSave.hpp>

//#include <opencv2/opencv.hpp>

// ffmpeg -f rawvideo -pix_fmt bgr24 -s 640x480 -r 30 -i - -an -f avi -r 30 foo.avi


//#include <ffmpeg_encode.hpp>

//
// save plugin
//
// [0] : queue d'entree des images (et control)
// [1] : queue de sortie des images
//
/*
CV_FOURCC('P','I','M','1') = MPEG-1 codec
CV_FOURCC('M','J','P','G') = motion-jpeg codec (does not work well)
CV_FOURCC('M','P','4','2') = MPEG-4.2 codec
CV_FOURCC('D','I','V','3') = MPEG-4.3 codec
CV_FOURCC('D','I','V','X') = MPEG-4 codec
CV_FOURCC('U','2','6','3') = H263 codec
CV_FOURCC('I','2','6','3') = H263I codec
CV_FOURCC('F','L','V','1') = FLV1 codec 
*/

using namespace std;

#undef VERBOSE
//#define VERBOSE

#define SAVE_VIDEO_COMMAND "ffmpeg -y -f rawvideo -pix_fmt gray -s %dx%d -r 20 -i - -an -vcodec mjpeg -b 30000k -f avi -r 20 %s"

#define PLUGIN_SAVE_MAX_BUFFER_SIZE 16000000

pluginSave::pluginSave() {
	ports["in"]=0;
	ports["out"]=1;
	ports["log"]=2;
	ports["control"]=3; // controle seulement! on ne lit pas d'images ici

	// for dot output
	portsIN["in"]=true;
	portsIN["control"]=true;

    internal_buffer=NULL;
}

pluginSave::~pluginSave() { 
  if (internal_buffer) {
    free(internal_buffer);
    internal_buffer=NULL;
  }
}

void pluginSave::init() {
	mode=SAVE_MODE_INTERNAL; // utilise le compteur interne
	count=-1; // do not count. save everything
	//video=NULL;
	F=NULL;
	buf[0]='A';
	buf[1]='B';
	buf[2]='C';
	buf[3]=0;
	initializing=true;
	index=0; // for internal mode. Never reset.
    compression=2;
    quality=90;
	//log << "init tid=" << pthread_self() << warn;
	log << "buf at " << (long int)buf<<" and "<<buf<< info;
}

void pluginSave::uninit() {
	//if( video!=NULL ) { delete video;video=NULL; }
	if( F!=NULL ) { pclose(F);F=NULL; }
	log << "uninit"<<warn;
}

void pluginSave::save_by_type(blob *i1) 
{
    char *c=strrchr(buf,'.');
    if (c!=NULL && strcasecmp(c,".png")==0)
    {
      cv::FileStorage file_info(".xml", cv::FileStorage::WRITE + cv::FileStorage::MEMORY);;
      file_info << "n" << i1->n;
      file_info << "timestamp" << i1->timestamp;
      file_info << "yaw" << i1->yaw;
      file_info << "pitch" << i1->pitch;
      //log << "[save] " << buf << ": timestamp is " << i1->timestamp << info;
      blob::pngwrite(buf,*i1,compression,&file_info);
      file_info.release();
    }
    else if (c!=NULL && (strcasecmp(c,".ppm")==0 || strcasecmp(c,".pgm")==0))
    {
      cv::FileStorage file_info(".xml", cv::FileStorage::WRITE + cv::FileStorage::MEMORY);;
      file_info << "n" << i1->n;
      file_info << "timestamp" << i1->timestamp;
      file_info << "yaw" << i1->yaw;
      file_info << "pitch" << i1->pitch;
      //log << "[save] " << buf << ": timestamp is " << i1->timestamp << info;
      blob::pbmwrite(buf,*i1,&file_info);
      file_info.release();
    }
    else if (c!=NULL && strcasecmp(c,".lz4")==0 && internal_buffer!=NULL)
    {
      cv::FileStorage file_info(".xml", cv::FileStorage::WRITE + cv::FileStorage::MEMORY);;
      file_info << "n" << i1->n;
      file_info << "timestamp" << i1->timestamp;
      file_info << "yaw" << i1->yaw;
      file_info << "pitch" << i1->pitch;
      //log << "[save] " << buf << ": timestamp is " << i1->timestamp << info;
      blob::lz4write(buf,*i1,&file_info,internal_buffer);
      file_info.release();
    }
    else 
    {
      cv::imwrite(buf,*i1,flags);
    }
}

bool pluginSave::loop() {
	struct timeval tv;

	if( initializing ) {
		if( badpos(3) ) { pop_front_ctrl_wait(0); }
		else { pop_front_ctrl_wait(3); }
		return true;
	}

	// attend une image en entree
	pop_front_nowait(3); // au cas ou on recevrait des messages pendant qu'on save...
	blob *i1=pop_front_nowait(0);
	if( i1==NULL ) { usleepSafe(2000);return(true); }
	int sz;
	bool output=true;
	int num;

  	  switch( mode ) {
	    case SAVE_MODE_INTERNAL:
	  	  if( i1->n<0 ) {
			log << "skip bad image (n=-1)"<<warn;
		  }else{
            const char *fname;
		    if( filename.empty() ) {
					  // no filename... use mapView instead.
					  map<string,string>::iterator itr;
					  itr=mapView.find(i1->view);
					  if( itr==mapView.end() ) {
						log << "no filename for view '"<<i1->view<<"'"<<err;
						output=false;
					  }else{
						fname=itr->second.c_str();
#ifdef VERBOSE
						log << "filename for view "<<i1->view<<" is "<<fname<<warn;
#endif
						// check countView
						  map<string,int>::iterator itc;
						  itc=countView.find(i1->view);
						  if( itc==countView.end() ) {
							// no count? output all.
							log << "no count" << info;
							output=true;
						  }else{
							int c=itc->second;
							if( c<0 ) output=true;
							else if( c==0 ) output=false;
							else{
								output=true;
								countView[itc->first]=c-1;
							}
						  }
						log << "output is "<<output<<info;
						if( output ) {
							itc=indexView.find(i1->view);
							if( itc==indexView.end() ) { num=index++; }
							else {
								num=indexView[itc->first];
								indexView[itc->first]=num+1;
							}
						}
					  }
					
		    }else{
			  // we have a filename. use it
			  fname=filename.c_str();
				// check count
				if( count<0 ) output=true;
				else if( count==0 ) output=false;
				else { output=true;count--; 
                  if (count==0) checkOutgoingMessages("done",0,-1,-1);
                }
		    }
			if( output ) {
			  sprintf(buf,fname,index);
			  log << "saving as "<<buf<<warn;
              save_by_type(i1);
			  index++;
			}else log << "no output image "<<i1->n<<warn;
		  }
		  break;
	    case SAVE_MODE_ID:
		  if( i1->n<0 ) {
			log << "skip bad image (n=-1)"<<warn;
		  }else{
		    const char *fname;
		    if( filename.empty() ) {
				// no filename... use mapView instead.
				map<string,string>::iterator itr;
				itr=mapView.find(i1->view);
				if( itr==mapView.end() ) {
					log << "no filename for view "<<i1->view<<err;
					output=false;
				}else{
					fname=itr->second.c_str();
					// check countView
					  map<string,int>::iterator itc;
					  itc=countView.find(i1->view);
					  if( itc==countView.end() ) {
						// no count? output all.
						output=true;
					  }else{
						int c=itc->second;
						if( c<0 ) output=true;
						else if( c==0 ) output=false;
						else{
							output=true;
							countView[itc->first]=c-1;
						}
					  }
				}
		    }else{
				// we have a filename. use it
				fname=filename.c_str();
				// check count
				if( count<0 ) output=true;
				else if( count==0 ) output=false;
				else { output=true;count--;  
                  if (count==0) checkOutgoingMessages("done",0,-1,-1);
                }
		    }
		    // save the image!
		    // timeinfo... 1/100 of ms resolution
			if( output ) {
		      int v=(int)((int64)(i1->timestamp*100000)%100000000);
			  sprintf(buf,fname,i1->n+1,v);
              //if (i1->n%100==0) log << buf<< ": timestamp is " << i1->timestamp << " (" << Qs[0]->size() << ")" << info;
              save_by_type(i1);
			}//else log << "no output image "<<i1->n<<warn;
		  }
	 	  break;
	    case SAVE_MODE_VIDEO:
			if( count!=0 ) {
			  if( F==NULL ) {
				if( !filename.empty() ) {
					//F=fopen(filename,"wb");
					char command[300];
					sprintf(command,SAVE_VIDEO_COMMAND,i1->cols,i1->rows,filename.c_str());
					log << "starting: "<<command<<warn;
					F=popen(command,"w");
				}else{ log << "no filename!"<<err; }
				if( F==NULL ) {
					log << "unable to start ffmpeg."<<err;
					mode=SAVE_MODE_NOP;
					break;
				}
			  }
			  sz=i1->rows*i1->cols;
			  fwrite(i1->data,1,sz,F);
			  // counting images? 
			  if( count>0 )
              {
                count--;
                if (count==0) checkOutgoingMessages("done",0,-1,-1);
              }
			}
	  	  break;
	    case SAVE_MODE_NOP:		break;
	    default: break;
  	  }

    push_back(1,&i1);

	// verifier le 'now'
	//checkOutgoingMessages(EVENT_SAVED,now,-1,-1);
	return true;
}

	//
	// decodage de messages a l'initialisation (init=true)
	// ou pendant l'execution du plugin (init=false)
	// (dans messageDecoder)
	//
bool pluginSave::decode(const osc::ReceivedMessage &m) {
	// verification pour un /defevent
	if( plugin::decode(m) ) return true;

	const char *address=m.AddressPattern();
	// en tout temps...
	if( oscutils::endsWith(address,"/mode") ) {
		// si video en cours, on ferme le tout
		if( F!=NULL ) { pclose(F);F=NULL; }
		//if( video!=NULL ) { delete video; video=NULL; }

		const char *val;
		m.ArgumentStream() >> val >> osc::EndMessage;

        log << "mode set to : "<<val<<info;

		if( strcmp(val,"internal")==0 ) {
			mode=SAVE_MODE_INTERNAL;
		}else if( strcmp(val,"id")==0 ) {
			mode=SAVE_MODE_ID;
		}else if( strcmp(val,"nop")==0 ) {
			mode=SAVE_MODE_NOP;
		}else if( strcmp(val,"video")==0 ) {
			mode=SAVE_MODE_VIDEO;
		}else{
			log << "illegal mode : "<<m<<err;
		}
	}else if( oscutils::endsWith(address,"/filename") ) {
		//if( mode==SAVE_MODE_VIDEO && video!=NULL ) { delete video;video=NULL; }
		if( mode==SAVE_MODE_VIDEO && F!=NULL ) { pclose(F);F=NULL; }
		const char *str;
		m.ArgumentStream() >> str >> osc::EndMessage;
		filename=string(str);
	}else if( oscutils::endsWith(address,"/mapview") ) {
		// /mapview <image view> <associated filename>
		const char *str1,*str2;
		m.ArgumentStream() >> str1 >> str2 >> osc::EndMessage;
		string view(str1);
		string fname(str2);
		mapView[view]=fname;
		countView[view]=-1;
		indexView[view]=0;
		log << "map view "<<view<<" to filename "<<fname<<info;
	}else if( oscutils::endsWith(address,"/compression") ) {
		m.ArgumentStream() >> compression >> osc::EndMessage;
		flags.push_back(CV_IMWRITE_PNG_COMPRESSION);
		flags.push_back(compression);  // [0-9] 9 being max compression, default is 3
		log << "compression set to "<<compression<<info;
	}else if( oscutils::endsWith(address,"/quality") ) {
		m.ArgumentStream() >> quality >> osc::EndMessage;
		flags.push_back(CV_IMWRITE_JPEG_QUALITY);
		flags.push_back(quality);  // [0-100] 100 being best quality
		log << "quality set to "<<quality<<info;
	}else if( oscutils::endsWith(address,"/internal-buffer") ) {
		int32 size;
		m.ArgumentStream() >> size >> osc::EndMessage;
        if (internal_buffer!=NULL) free(internal_buffer);
        if (size>=0 && size<=PLUGIN_SAVE_MAX_BUFFER_SIZE) internal_buffer=(char *)(malloc(sizeof(char)*size));
	}else if ( oscutils::endsWith(address,"/count") ) {
		osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		int nb;
		const char *view;
		if( arg->IsString() ) {
			view=arg->AsStringUnchecked();
			arg++;
			nb=arg->AsInt32();
			// reset les counts
			if( string("*")==view ) {
			    map<std::string,std::string>::iterator itr;
				for(itr=mapView.begin();itr!=mapView.end();itr++) {
					countView[itr->first]=nb;
					log << "reset count of "<<itr->first<<" to "<<nb<<info;
				}
			}else{
				countView[string(view)]=nb;
				log << "reset count of "<<view<<" to "<<nb<<info;
			}
		}else{
			nb=arg->AsInt32();
			count=nb;
			log << "reset global count to "<<nb<<info;
		}
    }else if( oscutils::endsWith(address,"/init-done") ) {
		initializing=false;
	}else{
		log << "unknown command: "<<m<<err;
		return false;
	}
	return true;
}



