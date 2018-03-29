

#include <imgv/imgv.hpp>

#include <imgv/pluginTransform.hpp>

#include <imgv/imqueue.hpp>

#include <sys/time.h>

#ifdef HAVE_PROSILICA
  #include <VmbTransform.h>
  #include <VmbCommonTypes.h>
#endif

using namespace cv;

#define Q_IN	0
#define Q_OUT	1
#define Q_LOG	2

pluginTransform::pluginTransform() {
	// ports
	ports["in"]=Q_IN;
	ports["out"]=Q_OUT;
	ports["log"]=Q_LOG;

	portsIN["in"]=true;
}

pluginTransform::~pluginTransform() {
  log << "deleted"<<warn; 
}

void pluginTransform::init() {
    unpack=false;
    bayer8planar=false;
    bayer12planar=false;
    bayer8pixel=false;
    bayer12pixel=false;
    avt=false;
    initializing=true;
}

void pluginTransform::uninit() {
	log << "uninit"<<warn;
}

bool pluginTransform::loop() {
	if( initializing ) {
		pop_front_ctrl_wait(Q_IN); // attend et process tous les messages
		return true;
	}
    
	// attend une image en entree
	blob *i1=pop_front_wait(Q_IN);

    if (bayer8planar || bayer12planar) //rearange bayer in planar format (useful for compression)
    {
      int packed=0;
      if (bayer12planar) packed=1;

      cv::Mat base;
      i1->copyTo(base);

      //image width, height
      int iwidth=base.cols;
      int iheight=base.rows;
      i1->create(iheight, iwidth, CV_8UC1);
      unsigned char *idata=(unsigned char *)(base.data);
      unsigned char *odata=(unsigned char *)(i1->data);
      //quadrant width, height
      int bwidth=iwidth/(2+packed);
      int bheight=iheight/2;
      for (int y=0;y<iheight;y+=2)
      {
        for (int x=0;x<iwidth;x+=2+packed)
        {
          int iy,ix;
          iy=y/2;
          ix=x/(2+packed);
          odata[iy*iwidth+ix]=idata[y*iwidth+x];
          odata[iy*iwidth+ix+bwidth]=idata[y*iwidth+x+1+packed];
          odata[(iy+bheight)*iwidth+ix]=idata[(y+1)*iwidth+x];
          odata[(iy+bheight)*iwidth+ix+bwidth]=idata[(y+1)*iwidth+x+1+packed];
          if (packed)
          {
            odata[iy*iwidth+ix+2*bwidth]=idata[y*iwidth+x+1];
            odata[(iy+bheight)*iwidth+ix+2*bwidth]=idata[(y+1)*iwidth+x+1];
          }
        }
      }
    }
    if (bayer8pixel || bayer12pixel) //rearange bayer in pixel order (starting from planar format)
    {
      cv::Mat base;
      i1->copyTo(base);
      int packed=0;
      if (bayer12pixel) packed=1;
      //image width, height
      int iwidth=i1->cols;
      int iheight=i1->rows;
      i1->create(iheight, iwidth, CV_8UC1);
      unsigned char *idata=(unsigned char *)(base.data);
      unsigned char *odata=(unsigned char *)(i1->data);
      //quadrant width, height
      int bwidth=iwidth/(2+packed);
      int bheight=iheight/2;
      for (int y=0;y<iheight;y+=2)
      {
        for (int x=0;x<iwidth;x+=2+packed)
        {
          int iy,ix;
          iy=y/2;
          ix=x/(2+packed);
          odata[y*iwidth+x]=idata[iy*iwidth+ix];
          odata[y*iwidth+x+1+packed]=idata[iy*iwidth+ix+bwidth];
          odata[(y+1)*iwidth+x]=idata[(iy+bheight)*iwidth+ix];
          odata[(y+1)*iwidth+x+1+packed]=idata[(iy+bheight)*iwidth+ix+bwidth];
          if (packed)
          {
            odata[y*iwidth+x+1]=idata[iy*iwidth+ix+2*bwidth];
            odata[(y+1)*iwidth+x+1]=idata[(iy+bheight)*iwidth+ix+2*bwidth];
          }
        }
      }
    }
    if (unpack)
    {
      cv::Mat base;
      i1->copyTo(base);

      //2x12bit pixels are stored in 3 bytes
      //we want to store the result in 16bit
      i1->create(base.rows, base.cols*2/3, CV_16UC1);
      int size=i1->rows*i1->cols;
      unsigned char b2; //second byte
      unsigned char *idata=(unsigned char *)(base.data);
      unsigned short *odata=(unsigned short *)(i1->data);
      for (int i=0;i<size;i+=2)
      {
        b2=idata[1];            
        odata[i]=(((unsigned short)(idata[0]))<<8)+(unsigned char)((b2&0x0f)<<4);
        odata[i+1]=(((unsigned short)(idata[2]))<<8)+(unsigned char)(b2&0xf0);
        idata+=3;
      }
    }    
#ifdef HAVE_PROSILICA
    if (avt) //convert format using AVTImageTransform
    {
      cv::Mat base;
      i1->copyTo(base);

      VmbError_t ret;
      VmbImage isrc,idest;
      isrc.Size = sizeof( isrc );
      idest.Size = sizeof( idest );
	  i1->create(base.rows, base.cols, avt_type_out);
      //log << "src dims: " << base.cols << " " << base.rows << info;
      ret=VmbSetImageInfoFromPixelFormat(avt_format_in,base.cols,base.rows,&isrc);
      //log << "src pixel format: " << ret << info;
      ret=VmbSetImageInfoFromPixelFormat(avt_format_out,base.cols,base.rows,&idest);
      //log << "dst pixel format: " << ret << info;
      isrc.Data=base.data;
      idest.Data=i1->data;

      //VmbTransformInfo info;
      //VmbSetDebayerMode( VmbDebayerMode2x2, &info );

      ret=VmbImageTransform(&isrc,&idest,NULL,0);//&info,1);
      if (ret) log << "image transform error: " << ret << err;
    }
#endif

    push_back(Q_OUT,&i1);
	return true;
}

bool pluginTransform::decode(const osc::ReceivedMessage &m) {
	//cout << "decoding " << m << endl;
	const char *address=m.AddressPattern();
    if( oscutils::endsWith(address,"/unpack") ) {
		m.ArgumentStream() >> unpack >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/bayer8planar") ) {
		m.ArgumentStream() >> bayer8planar >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/bayer12planar") ) {
		m.ArgumentStream() >> bayer12planar >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/bayer8pixel") ) {
		m.ArgumentStream() >> bayer8pixel >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/bayer12pixel") ) {
		m.ArgumentStream() >> bayer12pixel >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/avt-deactivate") ) {
      avt=false;
    }else if( oscutils::endsWith(address,"/avt-activate") ) {
      avt=true;
      m.ArgumentStream() >> avt_format_in >> osc::EndMessage;
      m.ArgumentStream() >> avt_format_out >> osc::EndMessage;
	  m.ArgumentStream() >> avt_type_out >> osc::EndMessage;
	}else if( oscutils::endsWith(address,"/init-done") ) {
		initializing=false;
	}else{
		log << "unknown command: "<<m<<warn;
		return false;
	}
	return true;
}



void pluginTransform::dump(ofstream &file) {
}



