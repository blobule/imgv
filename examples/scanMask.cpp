
//
// lit 100 pattern de scan, fait un max-min pour generer un masque.
// lit une carte de match et ajoute le masque.
//
//

#include <opencv2/opencv.hpp>
#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

using namespace std;
int blend;

vector<cv::Mat> imgs;
cv::Mat imin,imax;
int w,h;
int es,es1;
cv::Mat diff; // difference between max and min
cv::Mat mask; // difference between max and min

cv::Mat camlut,projlut;
cv::Mat ocamlut,oprojlut;

int computeMinMax(int seuil)
{
int i,j,x,y;
	cv::Mat sum(h,w,CV_32SC1);

	for(y=0;y<h;y++)
	for(x=0;x<w;x++) {
		sum.at<int>(y,x)=0;
	}

	for(i=0;i<imgs.size();i++)
	for(y=0;y<h;y++)
	for(x=0;x<w;x++) {
		sum.at<int>(y,x)+=imgs[i].at<uchar>(y,x);
	}
	cv::Mat avg(h,w,CV_8UC1);
	for(y=0;y<h;y++)
	for(x=0;x<w;x++) {
		avg.at<uchar>(y,x)=sum.at<int>(y,x)/imgs.size();
	}

	cv::imwrite("mean.png",avg);

	diff.create(h,w,CV_8UC1);
	mask.create(h,w,CV_8UC1);
	for(y=0;y<h;y++)
	for(x=0;x<w;x++) {
		int d1=imax.at<uchar>(y,x);
		int d2=imin.at<uchar>(y,x);
		int d=d1-d2;
		if( d<0 ) d=0;
		if( d>255 ) d=255;
		diff.at<uchar>(y,x)=d;
		if( d<seuil ) d=0; else d=255;
		mask.at<uchar>(y,x)=d;
	}
	cv::imwrite("diff.png",diff);
	
}

void computeLUT()
{
int x,y;
	ocamlut=camlut.clone();
	unsigned short *p=(unsigned short *)ocamlut.data;
	unsigned char *q=mask.data;
	for(y=0;y<camlut.rows;y++)
	for(x=0;x<camlut.cols;x++,p+=3,q++) {
		//ocamlut.at<cv::Vec<unsigned short,3>>(y,x)[0]=(int)mask.at<uchar>(y,x)*256;
		p[0]=(unsigned short)q[0]*256;
	}

	oprojlut=projlut.clone();
	p=(unsigned short *)oprojlut.data;
	for(y=0;y<projlut.rows;y++)
	for(x=0;x<projlut.cols;x++,p+=3) {
		//cv::Vec<unsigned short,3> p=oprojlut.at<cv::Vec<unsigned short,3>>(y,x);
		// get coord into camlut
		int cy=(int)((double)p[1]/65535.0*camlut.rows+0.5);
		int cx=(int)((double)p[2]/65535.0*camlut.cols+0.5);
		//oprojlut.at<cv::Vec<unsigned short,3>>(y,x)[0]=(int)mask.at<uchar>(cy,cx)*256;
		p[0]=(unsigned short)mask.at<uchar>(cy,cx)*256;
	}
}

// current masks
cv::Mat ma;
cv::Mat mb;
cv::Mat maskb; // mask B

// mask and maskb are the references

void computeBlend()
{
int x,y;
	printf("----------------- computing blend ---------------\n");
	ma=mask.clone();
	mb=maskb.clone();
	int j;

	for(j=0;j<blend;j++) {
		if( j%10==0 ) printf("j=%d\n",j);

		// gros blur extreme
		cv::blur(ma,ma,cv::Size(10,10));
		cv::blur(mb,mb,cv::Size(10,10));

		// ajuste
		// si ma>maska -> reset to maska
		for(y=0;y<mask.rows;y++)
		for(x=0;x<mask.cols;x++) {
			unsigned char a=mask.at<uchar>(y,x);
			unsigned char na=ma.at<uchar>(y,x);
			unsigned char b=maskb.at<uchar>(y,x);
			unsigned char nb=mb.at<uchar>(y,x);
			if( na>a ) na=a; // limit to original mask value
			if( nb>b ) nb=b;
			// normalize (na+nb)k = a+b ->  k = (a+b)/(na+nb). si b=0: k=a/(na+0) = a/na -> na=a
			// normalize (na+nb)k = 255 ->  k = 255/(na+nb). si b=0: k=255/na -> na=255
			// donc si b=0, on a na+nb=a et nb=0
			int k=na+nb;
			if( k>0 ) {
				na=(na*255)/k;
				nb=(nb*255)/k;
			}
			ma.at<uchar>(y,x)=na;
			mb.at<uchar>(y,x)=nb;
		}
	}
	imwrite("ma.png",ma);
	imwrite("mb.png",mb);

	// replace our mask
	for(y=0;y<mask.rows;y++)
	for(x=0;x<mask.cols;x++) {
			mask.at<uchar>(y,x)=ma.at<uchar>(y,x);
	}
}

int main(int argc,char *argv[])
{
string inname,minname,maxname;
string camlutname,ocamlutname;
string projlutname,oprojlutname;
string omaskname;
string extramaskname;
bool extra;
int n;
int seuil;
	printf("-- scanMask --\n");
	inname="scan-A-%03d.png";
	minname="scan-A-100.png";
	maxname="scan-A-102.png";
	camlutname="rescam-A.png";
	ocamlutname="rescam-A-masked.png";
	projlutname="resproj-A.png";
	oprojlutname="resproj-A-masked.png";
	omaskname="mask-A.png";
	extramaskname="mask-B.png";
	extra=false;
	seuil=50;
	n=100;
	blend=100;

	int i;
	for(i=1;i<argc;i++) {
		if( strcmp(argv[i],"-i")==0 && i+1<argc ) {
			inname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-imin")==0 && i+1<argc ) {
			minname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-imax")==0 && i+1<argc ) {
			maxname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-camlut")==0 && i+1<argc ) {
			camlutname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-ocamlut")==0 && i+1<argc ) {
			ocamlutname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-projlut")==0 && i+1<argc ) {
			projlutname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-oprojlut")==0 && i+1<argc ) {
			oprojlutname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-savemask")==0 && i+1<argc ) {
			omaskname=string(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-extramask")==0 && i+1<argc ) {
			extramaskname=string(argv[i+1]);
			extra=true;
			i++;continue;
		}else if( strcmp(argv[i],"-n")==0 && i+1<argc ) {
			n=atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-seuil")==0 && i+1<argc ) {
			seuil=atoi(argv[i+1]);
			i++;continue;
		}else if( strcmp(argv[i],"-blend")==0 && i+1<argc ) {
			blend=atoi(argv[i+1]);
			i++;continue;
		}else{
			printf("option '%s' inconnue\n",argv[i]);
			exit(0);
		}
	}

	char buf[200];

	for(i=0;i<n;i++) {
		sprintf(buf,inname.c_str(),i);
		printf("loading %s\n",buf);

		cv::Mat image=cv::imread(buf,-1);
		printf("in es=%d es1=%d\n",(int)image.elemSize(),(int)image.elemSize1());
		//cv::cvtColor(image,image,CV_BGR2GRAY);
		imgs.push_back(image);

		if( i==0 ) { w=image.cols;h=image.rows;es=image.elemSize();es1=image.elemSize1(); } }

	printf("size is (%d x %d) es=%d es1=%d\n",w,h,es,es1);

	imin=cv::imread(minname,-1);
	//cv::cvtColor(imin,imin,CV_BGR2GRAY);

	imax=cv::imread(maxname,-1);
	//cv::cvtColor(imax,imax,CV_BGR2GRAY);

	camlut=cv::imread(camlutname,-1);
	projlut=cv::imread(projlutname,-1);

	//printf("%d %d ***\n",imin.elemSize(),imin.elemSize1());

	computeMinMax(seuil);
	cv::imwrite(omaskname,mask);

	if( extra ) {
		maskb=cv::imread(extramaskname,-1);
		computeBlend();
	}


	computeLUT();

	cv::imwrite(ocamlutname,ocamlut);
	cv::imwrite(oprojlutname,oprojlut);

}
