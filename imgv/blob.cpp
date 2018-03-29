
#include <blob.hpp>

#include <png.h>

#include <stdio.h>

#define SIG_CHECK_SIZE 4

using namespace cv;

//
// Contructors
//

blob::blob() : cv::Mat()
{
  n=-1;
  timestamp=0.0;
  frame_last=false;
  /*cout << "blob "<< n << " created!" <<endl;*/ 
}

blob::blob(cv::Mat m) : cv::Mat(m)
{
  n=-1;
  timestamp=0.0;
  yaw=0.0;
  pitch=0.0;
  frame_last=false;
  /*cout << "blob "<< n << " created from mat!" <<endl;*/ 
}

blob::blob(const blob &b) : cv::Mat(b)
{ 
  n=b.n;
  timestamp=b.timestamp;
  yaw=b.yaw;
  pitch=b.pitch;
  frames_since_begin=b.frames_since_begin;
  frames_until_end=b.frames_until_end;view=b.view; 
  frame_last=false;
}

blob::blob(int xs,int ys,int type) : cv::Mat(ys,xs,type)
{ 
  n=-1; 
  timestamp=0.0;
  yaw=0.0;
  pitch=0.0;
  frame_last=false;
  /*cout << "blob "<< n << " created from mat!" <<endl;*/ 
}

//
// destructor
//

blob::~blob()
{
  /*cout << "blob "<< n << " destroyed!" <<endl;*/ 
}

//
// usefull stuff (read/write with tags, ...)
//


static FILE *read_sig_buf(const string &fname)
{
    unsigned char sig_buf[SIG_CHECK_SIZE];
    size_t bytesRead;

    FILE *F=fopen(fname.c_str(),"rb");
    if( F==NULL ) {
	cout << "Unable to open file "<<fname<<endl;
	return(NULL);
    }

    bytesRead = fread(sig_buf, 1, SIG_CHECK_SIZE, F);
    if (bytesRead != SIG_CHECK_SIZE) { return(NULL); }

    if (png_sig_cmp(sig_buf, (png_size_t) 0, (png_size_t) SIG_CHECK_SIZE) != 0) {
        cout << "file "<<fname<<" is not a PNG file"<<endl;
        return(NULL);
    }
    return(F);
}

// PNG is network byte order (most significant first) (little-endian)
// donc HL avec n=H*256+L
// retourne 1 si on est low-hi, et 0 si on est hi-low
static int is_big_endian(void)
{
    unsigned short v;
    unsigned char *b;
    b=(unsigned char *)&v;
    b[0]=12;
    b[1]=234;
    //printf("TEST v=%d  (normal=%d swap=%d) -> %d\n",v,b[0]*256+b[1],b[1]*256+b[0], (v==b[0]*256+b[1])?0:1);
    if( v==b[0]*256+b[1] ) return(0); /* Hi-Low */
    return(1); /* Low-Hi */
}




blob blob::pngread(const string &filename) {
	//cout << "read!" << filename << endl;
	blob ia=cv::imread(filename,-1);
	
	png_struct *png_ptr;
	png_info *info_ptr;

	FILE *F=read_sig_buf(filename);
	if( F==NULL ) return(ia); // image vide, probablement

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) { fclose(F);return(ia); }

	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(F);
		return(ia);
	}

	png_init_io (png_ptr, F);
	png_set_sig_bytes (png_ptr, SIG_CHECK_SIZE);
	png_read_info (png_ptr, info_ptr);
	//png_read_image(png_ptr, row_pointers);
	//png_read_end(png_ptr, NULL);

	/// lit le texte...
	png_textp text_ptr;
	int nbt;
	int j;
	png_get_text(png_ptr,info_ptr,&text_ptr,&nbt);
	for(j=0;j<nbt;j++) {
		//text_ptr[j].compression
		string key(text_ptr[j].key);
		string val(text_ptr[j].text);
		//cout << "text "<<j<<" "<<key<<" : "<<val<<endl;
		ia.texts[key]=val;
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(F);
	return ia;
}

// read a special tag FILE_STORAGE into fs
blob blob::pngread(const string &filename,cv::FileStorage *fs) {
	blob i0=blob::pngread(filename);
	if( i0.texts.find("FILE_STORAGE") == i0.texts.end() ) return(i0);
    //cout << i0.texts["FILE_STORAGE"] << endl;
    if (fs!=NULL) fs->open(i0.texts["FILE_STORAGE"], cv::FileStorage::READ + cv::FileStorage::MEMORY);
	return(i0);
}



bool blob::pngwrite(const string &filename,const blob &img,int compress,cv::FileStorage *fs) {
	//cout << "write! compress=" <<compress<<endl;

	FILE *F=fopen(filename.c_str(),"wb");
	if( F==NULL ) { cout << "unable to save to file "<<filename<<endl;return false; }

	png_structp png_ptr;
	png_infop info_ptr;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if( png_ptr==NULL ) { fclose(F);return false; }

	info_ptr = png_create_info_struct(png_ptr);
	if( info_ptr==NULL || setjmp(png_ptr->jmpbuf)) {
 		fclose(F);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	png_init_io(png_ptr, F);
	png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);
	if( compress<0 ) compress=0;
	if( compress>9 ) compress=9;
	png_set_compression_level(png_ptr,compress);


	// depth: CV_8U CV_8S CV_16U CV_16S CV_32S CV_32F CV_64F CV_USRTYPE1

	//cout << "channels="<<img.channels()<<endl;
	//cout << "depth="<<img.depth()<<endl;
	//cout << "elemSize="<<img.elemSize()<<endl; // elem*channels
	//cout << "elemSize1="<<img.elemSize1()<<endl; // elem only

	int ct;
	int cs=img.channels();
	switch( cs ) {
	  case 1: ct=PNG_COLOR_TYPE_GRAY;break;
	  case 2: ct=PNG_COLOR_TYPE_GRAY_ALPHA;break;
	  case 3: ct=PNG_COLOR_TYPE_RGB;break;
	  case 4: ct=PNG_COLOR_TYPE_RGB_ALPHA;break;
	  default:
		cout<<"illegal number of channels: "<<img.channels()<<endl;
        	png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	// allocate one row of output data
	// note: 8 and 16 bit are direct.
	// note: 32 and 64 bits are converted to 16bit and scaled.. eventually
	int rowsize;
	int bitdepth;
	switch( img.depth() ) {
	  case CV_8U :
	  case CV_8S : rowsize=img.cols*1*cs;bitdepth=8;break;
	  case CV_16U :
	  case CV_16S : rowsize=img.cols*2*cs;bitdepth=16;break;
	  case CV_32S :
	  case CV_32F : rowsize=img.cols*2*cs;bitdepth=16;break;
	  case CV_64F : rowsize=img.cols*2*cs;bitdepth=16;break;
	}

	png_set_IHDR(png_ptr, info_ptr,
                img.cols,img.rows,
                bitdepth,
                ct,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);

    if (fs!=NULL)
    {
  	  //
	  // add the tag FILE_STORAGE
	  //
	  string fsbuf=fs->releaseAndGetString();
	  //cout << "GOT FILE as '"<<fsbuf<<"'"<< " "<<fsbuf.empty()<<endl;

	  // texte!
	  int nbt=img.texts.size();
	  if( !fsbuf.empty() ) nbt++; // add extra tag
	  if( nbt>0 ) {
		png_text *text=(png_text *)malloc(nbt*sizeof(png_text));

		int i=0;
		std::map<std::string,std::string>::const_iterator iter;
		for(iter=img.texts.begin();iter!=img.texts.end(); iter++) {
			std::string key =  iter->first;
			std::string val = iter->second;
			text[i].key=(char *)key.c_str();
			text[i].text=(char *)val.c_str();
                        //text[i].compression = PNG_TEXT_COMPRESSION_zTXt; 
                        text[i].compression = PNG_TEXT_COMPRESSION_NONE; 
			//cout << "set text "<<i<<" to "<<key<<":"<<val<<endl;
			i++;
		}
		if( !fsbuf.empty() ) {
			text[i].key=const_cast<char *>("FILE_STORAGE");
			text[i].text=const_cast<char *>(fsbuf.c_str());
                        text[i].compression = PNG_TEXT_COMPRESSION_NONE; //zTXt; 
			i++;
		}
		png_set_text(png_ptr, info_ptr, text, nbt);
		free(text);
	  }
    }

	png_write_info(png_ptr, info_ptr);

	//cout << "row size is "<<rowsize<<endl;
	unsigned char *r=(unsigned char*)malloc(rowsize);
	int x,y,c;
	switch( img.depth() ) {
	  case CV_8U:
	  case CV_8S:
		//cout << "*** CV_8U or S ***"<<endl;
		for(y=0;y<img.rows;y++) {
			unsigned char *p=img.data+y*img.step;
			if( cs==3 ) {
				for(x=0;x<img.cols;x++) {
					r[x*cs+0]=p[x*cs+2];
					r[x*cs+1]=p[x*cs+1];
					r[x*cs+2]=p[x*cs+0];
				}
			}else{
				for(x=0;x<img.cols;x++) {
					for(c=0;c<cs;c++) {
						r[x*cs+c]=p[x*cs+c];
					}
				}
			}
			png_write_row(png_ptr, (png_bytep)r);
		}
		break;
	  case CV_16U:
	  case CV_16S:
		//cout << "*** CV_16U or S ***"<<endl;
		for(y=0;y<img.rows;y++) {
			unsigned short *p=(unsigned short *)(img.data+y*img.step);
			unsigned short *q=(unsigned short *)r;
			if( cs==3 ) {
				for(x=0;x<img.cols;x++) {
					q[x*cs+0]=p[x*cs+2];
					q[x*cs+1]=p[x*cs+1];
					q[x*cs+2]=p[x*cs+0];
				}
			}else{
				for(x=0;x<img.cols;x++) {
				for(c=0;c<cs;c++) {
					q[x*cs+c]=p[x*cs+c];
				}
				}
			}
			if( is_big_endian() ) {
				for(x=0;x<img.cols*cs;x++) {
					q[x]=(q[x]>>8)|(q[x]<<8);
				}
			}
			png_write_row(png_ptr, (png_bytep)r);
		}
		break;
	  case CV_32S:
	  case CV_32F:
	  case CV_64F:
	  default:
		cout << "unsupported pixel type"<<endl;
	}
	free(r);

	png_write_end(png_ptr, NULL);
	fclose(F);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return true;
}

bool blob::pngwrite(const string &filename,const blob &img,int compress) {
	cv::FileStorage fs;
	return(blob::pngwrite(filename,img,compress,&fs));
}

void blob::dumpText() {
	std::map<std::string,std::string>::iterator iter;
	cout << "-- texts dump nb="<<texts.size()<<" --"<<endl;
	for(iter=texts.begin(); iter!=texts.end(); iter++) {
		std::string key =  iter->first;
		std::string val = iter->second;
		cout << "key '"<<key<<"' = '"<<val<<"'"<<endl;
	}
}

blob blob::pbmread(const string &filename) {
	blob i0=blob::pbmread(filename,NULL);
	return(i0);
}

blob blob::pbmread(const string &filename, cv::FileStorage *fs)
{
    FILE *fimg;
    int max;
    int XSize,YSize,CSize;
    char TBuf[200];
    char c1,c2;
    const char *name=filename.c_str();

    blob ia;

    fimg=fopen(name,"rb");
    if( fimg==NULL ) {
        fprintf(stderr,"[PbmRead] Unable to open '%s'\n",name);return ia;
    }

    /* Read header */
    if( fgets(TBuf,100,fimg)!=TBuf ) {
        fprintf(stderr,"[PbmRead] Unable to read header of '%s'\n",name);
        fclose(fimg);
        return ia;
    }
    if( strncmp(TBuf,"P5",2)==0 ) CSize=1;      /* pgm */
    else if( strncmp(TBuf,"P6",2)==0 ) CSize=3; /* ppm */
    else {
        /* fprintf(stderr,"[PbmRead] Unsupported PBM format for '%s'\n",name); */
        fclose(fimg);
        return ia;
    }

    string fsbuf="";
    bool fs_found=false;
    if (TBuf[2]=='\n' || (TBuf[2]==' ' && TBuf[3]=='\n'))
    {
        /* Read filestorage or skip comments */
        do {
            if( fgets(TBuf,200,fimg)!=TBuf ){fclose(fimg); return ia;}
            if (strcmp(TBuf,"#FILESTORAGE\n")==0) fs_found=true;
            else if (fs_found)
            {
              if (TBuf[0]=='#') fsbuf+=string(TBuf+1);
            }
        } while( TBuf[0]=='#' || TBuf[0]=='\n');
        if (fs!=NULL && fsbuf!="") fs->open(fsbuf, cv::FileStorage::READ + cv::FileStorage::MEMORY);
        /* xres/yres */
        sscanf(TBuf,"%d %d",&XSize,&YSize);
        if (TBuf[4]==' ')
        {
            sscanf(TBuf,"%d      %d",&XSize,&YSize);
        }

        /* Number of gray , usually 255, Just skip it */
        if( fgets(TBuf,200,fimg)!=TBuf ) {fclose(fimg);return ia;}
        sscanf(TBuf,"%d",&max);        
    }
    else
    {
        sscanf(TBuf,"%c%c %d %d %d",&c1,&c2,&XSize,&YSize,&CSize);
    }
    /* print a little info about the image */
#ifdef VERBOSE
    printf("Image x and y size in data: %d %d\n",XSize,YSize);
    printf("Image zsize in channels: %d\n",CSize);
    printf("Image pixel min and max: %d %d\n",0,max);
#endif
    if (max>=256) CSize*=2;

    if (CSize==1) ia=blob(XSize,YSize,CV_8UC1);
    else if (CSize==2) ia=blob(XSize,YSize,CV_16UC1);
    else if (CSize==3) ia=blob(XSize,YSize,CV_8UC3);
    else if (CSize==6) ia=blob(XSize,YSize,CV_16UC3);
    else
    {
      fprintf(stderr,"[PbmRead] Unsupported channel size of '%d'\n",CSize);
      fclose(fimg);
      return ia;
    }

    int data_size=XSize*YSize*CSize;
    int k=fread(ia.data,1,data_size,fimg);
    if (k!=data_size)
    {
      fprintf(stderr,"[PbmRead] Unable to read all data\n");
    }    

    fclose(fimg);
    return ia;
}


bool blob::pbmwrite(const string &filename,const blob &img) {
	cv::FileStorage fs;
	return(blob::pbmwrite(filename,img,&fs));
}

bool blob::pbmwrite(const string &filename,const blob &img, cv::FileStorage *fs)
{
    FILE *fimg;
    const char *name=filename.c_str();

	int cs=img.channels();
	int bytedepth;
	switch( img.depth() ) {
	  case CV_8U :
	  case CV_8S : bytedepth=1;break;
	  case CV_16U :
	  case CV_16S : bytedepth=2;break;
      default : 
        fprintf(stderr,"[PbmWrite] Number of bytes per channel not supported\n");
        return(false);
	}

    if( cs!=1 && cs!=3 ) {
        fprintf(stderr,"[PbmWrite] Number of channels (%d) not supported\n",cs);
        return(false);
    }

#ifdef VERBOSE
    printf("Saving %d plane image '%s'\n",I->cs,Name);
#endif

    fimg=fopen(name,"wb");
    if( fimg==NULL ) {
        fprintf(stderr,"[PbmWrite] Unable to open '%s'\n",name);return(-1);
    }

    if( cs==1 ) fprintf(fimg,"P5\n"); /* pgm */
    else if( cs==3 ) fprintf(fimg,"P6\n");    /* ppm */
    else return(false); /* impossible */
    fprintf(fimg,"#FILESTORAGE\n");
    if (fs!=NULL)
    {
      string fsbuf=fs->releaseAndGetString();
      std::istringstream f(fsbuf);
      std::string line;    
      while (std::getline(f, line)) {
        fprintf(fimg,"#%s\n",line.c_str());
      }
    }
   
    fprintf(fimg,"%d %d\n",img.cols,img.rows);
    if (bytedepth==2) fprintf(fimg,"%d\n",65535);
    else fprintf(fimg,"255\n");

    int data_size=img.cols*img.rows*cs*bytedepth;
    int k=fwrite(img.data,1,data_size,fimg);
    if (k!=data_size)
    {
      fprintf(stderr,"[PbmWrite] Unable to write all data\n");
      fclose(fimg);
      return false;
    }

    fclose(fimg);

    return(true);
}



blob blob::lz4read(const string &filename,char *buf) {
	blob i0=blob::lz4read(filename,NULL,buf);
	return(i0);
}

/* same format as pgm,ppm but with data compressed with LZ4  */
/* buf needs to be large enough to store uncompresed data */
blob blob::lz4read(const string &filename, cv::FileStorage *fs,char *buf)
{
	blob ia;
#ifndef HAVE_LZ4
    fprintf(stderr,"[Lz4Read] LZ4 not available.\n");
    return ia;
#else
    FILE *fimg;
    int max;
    int XSize,YSize,CSize;
    char TBuf[200];
    char c1,c2;
    const char *name=filename.c_str();


    fimg=fopen(name,"rb");
    if( fimg==NULL ) {
        fprintf(stderr,"[Lz4Read] Unable to open '%s'\n",name);return ia;
    }

    /* Read header */
    if( fgets(TBuf,100,fimg)!=TBuf ) {
        fprintf(stderr,"[Lz4Read] Unable to read header of '%s'\n",name);
        fclose(fimg);
        return ia;
    }
    if( strncmp(TBuf,"P5",2)==0 ) CSize=1;      /* pgm */
    else if( strncmp(TBuf,"P6",2)==0 ) CSize=3; /* ppm */
    else {
        /* fprintf(stderr,"[Lz4Read] Unsupported PBM format for '%s'\n",name); */
        fclose(fimg);
        return ia;
    }

    string fsbuf="";
    bool fs_found=false;
    if (TBuf[2]=='\n' || (TBuf[2]==' ' && TBuf[3]=='\n'))
    {
        /* Read filestorage or skip comments */
        do {
            if( fgets(TBuf,200,fimg)!=TBuf ){fclose(fimg); return ia;}
            if (strcmp(TBuf,"#FILESTORAGE\n")==0) fs_found=true;
            else if (fs_found)
            {
              if (TBuf[0]=='#') fsbuf+=string(TBuf+1);
            }
        } while( TBuf[0]=='#' || TBuf[0]=='\n');
        if (fs!=NULL && fsbuf!="") fs->open(fsbuf, cv::FileStorage::READ + cv::FileStorage::MEMORY);
        /* xres/yres */
        sscanf(TBuf,"%d %d",&XSize,&YSize);
        if (TBuf[4]==' ')
        {
            sscanf(TBuf,"%d      %d",&XSize,&YSize);
        }

        /* Number of gray , usually 255, Just skip it */
        if( fgets(TBuf,200,fimg)!=TBuf ) {fclose(fimg);return ia;}
        sscanf(TBuf,"%d",&max);        
    }
    else
    {
        sscanf(TBuf,"%c%c %d %d %d",&c1,&c2,&XSize,&YSize,&CSize);
    }
    /* print a little info about the image */
#ifdef VERBOSE
    printf("Image x and y size in data: %d %d\n",XSize,YSize);
    printf("Image zsize in channels: %d\n",CSize);
    printf("Image pixel min and max: %d %d\n",0,max);
#endif
    if (max>=256) CSize*=2;

    if (CSize==1) ia=blob(XSize,YSize,CV_8UC1);
    else if (CSize==2) ia=blob(XSize,YSize,CV_16UC1);
    else if (CSize==3) ia=blob(XSize,YSize,CV_8UC3);
    else if (CSize==6) ia=blob(XSize,YSize,CV_16UC3);
    else
    {
      fprintf(stderr,"[Lz4Read] Unsupported channel size of '%d'\n",CSize);
      fclose(fimg);
      return ia;
    }

    int data_size=XSize*YSize*CSize;
    int cmp_size;
    if( fgets(TBuf,200,fimg)!=TBuf ) {fclose(fimg);return ia;}
    sscanf(TBuf,"%d",&cmp_size);        
    int k=fread(buf,1,cmp_size,fimg);
    if (k!=cmp_size)
    {
      fprintf(stderr,"[Lz4Read] Unable to read all data\n");
      fclose(fimg);return ia;
    }    
    k=LZ4_decompress_safe(buf,(char *)(ia.data),cmp_size,data_size);
    if (k!=data_size)
    {
      fprintf(stderr,"[Lz4Read] Unable to decompress all data\n");
    }    

    fclose(fimg);
    return ia;
#endif
}


bool blob::lz4write(const string &filename,const blob &img,char *buf) {
	cv::FileStorage fs;
	return(blob::lz4write(filename,img,&fs,buf));
}

/* same format as pgm,ppm but with data compressed with LZ4 */
/* buf needs to be large enough to store compresed data */
bool blob::lz4write(const string &filename,const blob &img,cv::FileStorage *fs,char *buf)
{
#ifndef HAVE_LZ4
    fprintf(stderr,"[Lz4Write] LZ4 not available.\n");
    return false;
#else

    FILE *fimg;
    const char *name=filename.c_str();

	int cs=img.channels();
	int bytedepth;
	switch( img.depth() ) {
	  case CV_8U :
	  case CV_8S : bytedepth=1;break;
	  case CV_16U :
	  case CV_16S : bytedepth=2;break;
      default : 
        fprintf(stderr,"[Lz4Write] Number of bytes per channel not supported\n");
        return(false);
	}

    if( cs!=1 && cs!=3 ) {
        fprintf(stderr,"[Lz4Write] Number of channels (%d) not supported\n",cs);
        return(false);
    }

#ifdef VERBOSE
    printf("Saving %d plane image '%s'\n",I->cs,Name);
#endif

    fimg=fopen(name,"wb");
    if( fimg==NULL ) {
        fprintf(stderr,"[Lz4Write] Unable to open '%s'\n",name);return(-1);
    }

    if( cs==1 ) fprintf(fimg,"P5\n"); /* pgm */
    else if( cs==3 ) fprintf(fimg,"P6\n");    /* ppm */
    else return(false); /* impossible */
    fprintf(fimg,"#FILESTORAGE\n");
    if (fs!=NULL)
    {
      string fsbuf=fs->releaseAndGetString();
      std::istringstream f(fsbuf);
      std::string line;    
      while (std::getline(f, line)) {
        fprintf(fimg,"#%s\n",line.c_str());
      }
    }
   
    fprintf(fimg,"%d %d\n",img.cols,img.rows);
    if (bytedepth==2) fprintf(fimg,"%d\n",65535);
    else fprintf(fimg,"255\n");

    int data_size=img.cols*img.rows*cs*bytedepth;
    int cmp_size=LZ4_compress((char *)(img.data),buf,data_size);
    //int cmp_size=LZ4_compressHC2((char *)(img.data),buf,data_size,0);
    //int cmp_size=LZ4_compress_limitedOutput((char *)(img.data),buf,data_size,data_size/2);
    //int cmp_size=LZ4_compressHC2_limitedOutput((char *)(img.data),buf,data_size,data_size/2);
    if (cmp_size==0)
    {
      fprintf(stderr,"[Lz4Write] Unable to compress data\n");
      fclose(fimg);
      return false;
    }
    fprintf(fimg,"%d\n",cmp_size);
    int k=fwrite(buf,1,cmp_size,fimg);
    if (k!=cmp_size)
    {
      fprintf(stderr,"[Lz4Write] Unable to write all data\n");
      fclose(fimg);
      return false;
    }

    fclose(fimg);

    return(true);
#endif
}

bool blob::inBound(double x,double y)
{
    int fx,fy;//,cx,cy;
    bool r;

    //if( isnanf(x) || isnanf(y) ) return true;

    fx=(int)floor(x);//cx=(int)ceil(x);
    fy=(int)floor(y);//cy=(int)ceil(y);

    if( fx<0 || fy<0 || fx>=cols || fy>=rows ) r=false;
    else r=true;

    //printf("Check (%f,%f) [%d,%d] -> %d\n",x,y,cols,rows,r);

    return(r);
}

int blob::interpolateBilinear(double x,double y,double *val)
{
    int ix,iy,ch;
    int P;
    double dx,dy;
    double a,b,c,d;

    if (val==NULL) return -1;
    if (data==NULL) return -1;

    if (inBound(x,y)==false) return -1;
    if (x>cols-1) return -1;
    if (y>rows-1) return -1;

    //value of 0.5,0.5 should return value intensity of pixel 0
    x-=0.5;
    y-=0.5;
    if (x<0) x=0;
    if (y<0) y=0;

    int cs=channels();
    //default value
    for(ch=0;ch<cs;ch++) val[ch]=0.0;

    ix=(int)floor(x);
    iy=(int)floor(y);
    dx=x-(double)ix;
    dy=y-(double)iy;
    //check for noise...
    if( dx<1e-8 ) dx=0.0;
    if( dy<1e-8 ) dy=0.0;

    P=(iy*cols+ix)*cs;

    for(ch=0;ch<cs;ch++) {
        a=data[P+ch]; // (x,y)
        if(dx==0.0) b=a;
        else
        {
          b=data[P+cs+ch];  // (x+1,y)
        }
        if(dy==0.0) c=a;
        else
        {
          c=data[P+cols*cs+ch]; // (x,y+1)
        }
        if(dx==0.0) d=c;
        else if(dy==0.0) d=b;
        else
        {
          d=data[P+(cols+1)*cs+ch]; // (x+1,y+1)
        }

        // bi-linear interpolation
        a+=(c-a)*dy;
        b+=(d-b)*dy;
        a+=(b-a)*dx;
        if (a<0.0) a=0.0;
        if (a>255.0) a=255.0;
        val[ch]=a;        
    }
    return(0);
}



