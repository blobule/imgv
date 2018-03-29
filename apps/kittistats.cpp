//
// kittistats
//

#include <imgv/imgv.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

using namespace std;
using namespace cv;

//#define USE_PROFILOMETRE
#ifdef USE_PROFILOMETRE
 #include <profilometre.h>
#endif

#include <vector>
#include <sys/time.h>


// les images, en global

int nb; // nb d'images (last-first+1)
Mat *disp;      // [nb]
Mat *image0;    // [nb] left
Mat *image1;    // [nb] right
int rows,cols;  // standard size of all images

//
// image0(x,y) -> image1(x-dx,y)
// dx=disp(x,y)/256;
//
//

double horloge(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec+tv.tv_usec/1000000.0;
}



void imageFilterMean(int nb,Mat img[]) {
    int rows=img[0].rows;
    int cols=img[0].cols;

    unsigned short *p,*q;

    // filtre horizontal
    unsigned short *line=(unsigned short *)malloc(cols*sizeof(unsigned short));
    unsigned short *count=(unsigned short *)malloc(cols*sizeof(unsigned short));

    for(int i=0;i<nb;i++) {
	p=(unsigned short *)img[i].data;
	for(int r=0;r<rows;r++) {
		q=p+r*cols;
		for( int c=0;c<cols;c++) {
			line[c]=q[c];
			count[c]=(q[c]==0?0:1);
			if( c>0 && q[c-1]>0 ) {line[c]+=q[c-1];count[c]+=1;}
			if( c<cols-1 && q[c+1]>0 ) {line[c]+=q[c+1];count[c]+=1; }
		}
		for( int c=0;c<cols;c++) q[c]=line[c]/(count[c]==0?1:count[c]);
	}
    }


    free(line);
    free(count);

    // filtre vertical
    line=(unsigned short *)malloc(rows*sizeof(unsigned short));
    count=(unsigned short *)malloc(rows*sizeof(unsigned short));

    for(int i=0;i<nb;i++) {
	p=(unsigned short *)img[i].data;
	for( int c=0;c<cols;c++) {
		q=p+c;
		for(int r=0;r<rows;r++) {
			line[r]=q[r*cols];
			count[r]=(q[r*cols]==0?0:1);
			if( r>0 && q[(r-1)*cols]>0 ) {line[r]+=q[(r-1)*cols];count[r]+=1;}
			if( r<rows-1 && q[(r+1)*cols]>0 ) {line[r]+=q[(r+1)*cols];count[r]+=1; }
		}
		for( int r=0;r<rows;r++) q[r*cols]=line[r]/(count[r]==0?1:count[r]);
	}
    }

    free(line);
    free(count);
}

void imageMoyenne(int nb,Mat img[]) {

    Mat moy;
    int rows=img[0].rows;
    int cols=img[0].cols;

    int *sum=(int *)malloc(rows*cols*sizeof(int));
    short *count=(short *)malloc(rows*cols*sizeof(short));

    int *p;
    unsigned short *q;

	// initialize
	for(int i=0;i<rows*cols;i++) { sum[i]=0;count[i]=0; }

	unsigned short vMin,vMax;
	vMin=65535;
	vMax=0;

    for(int i=0;i<nb;i++) {
	//cout << "   ("<<img[i].cols<<","<<img[i].rows<<","<<img[i].depth()<<","<<img[i].channels()<<",elem="<<img[i].elemSize1()<<",continu="<<img[i].isContinuous() <<")"<<endl;

	q=(unsigned short *)img[i].data;
	for(int j=0;j<rows*cols;j++,q++) {
		if( *q>0 ) { sum[j]+=*q;count[j]+=1; }
	}
    }

    moy.create(rows,cols,CV_16UC1);
    q=(unsigned short *)moy.data;
    for(int i=0;i<rows*cols;i++,q++) *q=(unsigned short)(sum[i]/(count[i]==0?1:count[i]));

    std::cout << "moyenne.png\n";
    imwrite("moyenne.png",moy);

    free(sum);
    free(count);

}

//
// statistique globale
// p(delta C | depth )
// I0[x,y]=I1[x-D[x,y],y]
// donc on calcule histogramme de |I0[x,y]-I1[x-D[x,y],y]|
// fait aussi l'histo de
// p(deltac | any valid depth ) -> depth de 0 a 
void histoDeltaCdepth() {
    int i,r,c;
    int delta;
    unsigned short *p;
    unsigned char *q0;
    unsigned char *q1;
    unsigned char c0,c1;
    int *histo; // [0..255] difference en couleurs
    histo=(int *)malloc(256*sizeof(int));
    for(i=0;i<256;i++) histo[i]=0;

    for(i=0;i<nb;i++) {
        p=(unsigned short *)disp[i].data;
        q0=(unsigned char *)image0[i].data;
        q1=(unsigned char *)image1[i].data;
        for(r=0;r<rows;r++) {
            for(c=0;c<cols;c++,p++,q0++,q1++) {
                if( *p==0 ) continue; // pas de ground truth disponible ici
                int d=(*p*2/256+1)/2;
                if( d>c ) continue; // match is outside image
                c0=*q0;
                c1=*(q1-d);
                if( c0<=c1 ) delta=c1-c0; else delta=c0-c1;
                histo[delta]+=1;
            }
        }
    }
    printf("(* histo p(delta C | depth) *)\n");
    FILE *f=fopen("histoInfo.m","w");
    fprintf(f,"histoCD={");
    for(i=0;i<256;i++) {
        fprintf(f,"{%d,%d}%s\n",i,histo[i],(i+1==256)?"};":",");
    }
    fclose(f);

    free(histo);
}


//
// distribution des profondeurs
// p(depth) , histo de D[x,y]
//
void histoDepth() {
    int i,r,c;
    int delta;
    unsigned short *p;
    unsigned char *q0;
    unsigned char *q1;
    unsigned char c0,c1;
    int *histo; // [0..255] difference en couleurs
    
    int fac=1; // precision pixel. 10 = 1/10 pixel

    // trouve le minmax
    int min=9999999;
    int max=-100000;

    for(i=0;i<nb;i++) {
        p=(unsigned short *)disp[i].data;
        for(r=0;r<rows;r++) {
            for(c=0;c<cols;c++,p++) {
                if( *p==0 ) continue; // pas de ground truth disponible ici
                int d=(*p*2/(1000/fac)+1)/2; // depth en pixel
                if( d<min ) min=d;
                if( d>max ) max=d;
            }
        }
    }

    printf("(* depth min=%f max=%f *)\n",min/(float)fac,max/(float)fac);

    int mm=max-min+1;
    histo=(int *)malloc(mm*sizeof(int));
    for(i=0;i<mm;i++) histo[i]=0;

    for(i=0;i<nb;i++) {
        p=(unsigned short *)disp[i].data;
        for(r=0;r<rows;r++) {
            for(c=0;c<cols;c++,p++) {
                if( *p==0 ) continue; // pas de ground truth disponible ici
                int d=(*p*2/(1000/fac)+1)/2; // depth en pixel
                histo[d]++;
            }
        }
    }

    printf("(* histo p(depth) *)\n");
    FILE *f=fopen("histoInfo.m","a");
    fprintf(f,"histoD={");
    for(i=0;i<mm;i++) {
        fprintf(f,"{%4.2f,%d}%s\n",(i+min)/(float)fac,histo[i],(i+1==mm)?"};":",");
    }
    fclose(f);

    free(histo);
}

//
// p(delta depth from gt,deltaC,pos)
// On prend le ground truth
// pos: r/k*(cols/k)+cols/k, avec nbx,nby defini d'avance
//

void histoCout() {
    printf("Cout\n");
    int i,r,c;
    int delta;
    unsigned short *p;
    unsigned char *q0;
    unsigned char *q1;
    unsigned char c0,c1;
    int *histo; // [0..255] difference en couleurs

    int div=8;
    int nbd=60;
    int nbc=256;
    int nbp=(rows-1)/div*(cols/div)+(cols-1)/div+1;

    int sz=nbd*nbc*nbp;
    printf("nbD=%d nbC=%d nbP=%d sz=%5.1f mb\n",nbd,nbc,nbp,(double)sz/1024/1024);
    // [(d*nbc+c)*nbp+p]


    histo=(int *)malloc(sz*sizeof(int));
    for(i=0;i<sz;i++) histo[i]=0;

    for(i=0;i<nb && i<500000;i++) {
        printf("image %d/%d\n",i+1,nb);
        p=(unsigned short *)disp[i].data;
        q0=(unsigned char *)image0[i].data;
        q1=(unsigned char *)image1[i].data;
        for(r=0;r<rows;r++) {
            for(c=0;c<cols;c++,p++,q0++,q1++) {
                if( *p==0 ) continue; // pas de ground truth disponible ici
                int d0=(*p*2/256+1)/2;
                // on genere quelques deltas autour de la vraie profondeur
                c0=*q0;
                for(int dd=-30;dd<30;dd++) {
                    int d=d0+dd; // test depth, for delta depth = dd
                    if( c-d<0 || c-d>=cols ) continue; // match is outside image
                    //if( d>nbd-1 ) { printf("d too large %d\n",d);continue; }
                    int pos=r/div*(cols/div)+c/div;
                    c1=*(q1-d);
                    if( c0<=c1 ) delta=c1-c0; else delta=c0-c1;
                    if( delta>nbc-1 ) delta=nbc-1; // maximum color difference
                    histo[((dd+30)*nbc+delta)*nbp+pos]+=1;
                }
            }
        }
    }
    int rowsd=rows/div;
    int colsd=cols/div;
    printf("saving histoCout.data\n");
    FILE *f=fopen("histoCout.data","w");
    fwrite((void *)&rowsd,sizeof(int),1,f);
    fwrite((void *)&colsd,sizeof(int),1,f);
    fwrite((void *)&nbd,sizeof(int),1,f);
    fwrite((void *)&nbc,sizeof(int),1,f);
    fwrite((void *)&nbp,sizeof(int),1,f);
    fwrite((void *)histo,sizeof(int),sz,f);
    fclose(f);

    free(histo);

}

typedef struct { short dx,dy;int norm; } delta;

//
// p(deltaDepth,deltaPos,deltaColor)
//
// pour deux points dans l'image 1
// deltaD = Abs[  D[x1,y1] - D[x2,y2] ]
// deltaP = Norm[ (x1,y1) - (x2,y2) ]
// deltaC = I[x1,y1] - I[x2,y2]
//
// nn: nombre max d'image training
//
void histoLissage(int nn) {
    int i,k,r,c,rr,cc;
    unsigned short *p0,*p1;
    unsigned char *q0;
    unsigned char *q1;
    unsigned char c0,c1;

    // une liste de positions relatives, pour aller plus vite
    int maxDeltaP=300;
    int pFac=1; // on va aller au 1/pFac de pixel
    int maxDeltaC=100; // apres 100 de difference, ca suffit...
    int maxDeltaD=60;

    int maxDeltaP2=maxDeltaP*pFac;

    int nbDeltaP=360*maxDeltaP2;
    delta *deltaT=(delta *)malloc(nbDeltaP*sizeof(delta));

    // plus tard on va choisir un angle a au hasard 0..359, et utiliser deltaT[a*maxDeltaP2+0..maxDeltaP2]
    i=0;
    int maxi=0;
    for(int a=0;a<360;a++) {
        for(int d=0;d<maxDeltaP2;d++) {
            double aa=(double)a*M_PI/180.0;
            int dx=floor(cos(aa)*d/(double)pFac+0.5);
            int dy=floor(sin(aa)*d/(double)pFac+0.5);
            deltaT[i].dx=dx;
            deltaT[i].dy=dy;
            deltaT[i].norm=d;
            if( deltaT[i].norm > maxi ) maxi=deltaT[i].norm;
            //printf("%5d : (%3d,%d) -> %d\n",i,dx,dy,deltaT[i].norm);
            i++;
        }
    }


    /**
    int nbDeltaP=0;
    int dx,dy;
    for(dy=-maxDeltaP;dy<=maxDeltaP;dy++)
    for(dx=-maxDeltaP;dx<=maxDeltaP;dx++) {
           if( dx*dx+dy*dy>maxDeltaP*maxDeltaP ) continue;
           nbDeltaP++;
    }
    delta *deltaT=(delta *)malloc(nbDeltaP*sizeof(delta));
    nbDeltaP=0;
    int maxi=0;
    for(dy=-maxDeltaP;dy<=maxDeltaP;dy++)
    for(dx=-maxDeltaP;dx<=maxDeltaP;dx++) {
           int norm2=dx*dx+dy*dy;
           if( norm2>maxDeltaP*maxDeltaP ) continue;
           deltaT[nbDeltaP].dx=dx;
           deltaT[nbDeltaP].dy=dy;
           deltaT[nbDeltaP].norm=floor(sqrt((double)norm2)*pFac+0.5);
           //printf("%5d : (%3d,%d) -> %d\n",nbDeltaP,dx,dy,deltaT[nbDeltaP].norm);
           if( deltaT[nbDeltaP].norm > maxi ) maxi=deltaT[nbDeltaP].norm;
           nbDeltaP++;
    }
    **/
    printf("(* nb positions relatives = %d, max deltap=%d *)\n",nbDeltaP,maxi);


    // on a un histo [maxDeltaP2 * maxDeltaC * maxDeltaD]
    // acces : [ ( deltaD * maxDeltaC + deltaC )* maxDeltaP2 + deltaP2 ]

    int sz=maxDeltaD*maxDeltaC*maxDeltaP2;
    printf("size is %d x %d x %d = %d\n",maxDeltaD,maxDeltaC,maxDeltaP2,sz);

    int *H=(int *)malloc(sz*sizeof(int));
    for(i=0;i<sz;i++) H[i]=0;

    int deltaC,deltaD;

    for(i=0;i<nb && i<nn;i++) {
        printf("training image %d/%d\n",i,nb-1);
        p0=(unsigned short *)disp[i].data;
        q0=(unsigned char *)image0[i].data;
        for(r=0;r<rows;r++) {
            //printf("r=%d\n",r);
            for(c=0;c<cols;c++,p0++,q0++) {
                if( *p0==0 ) continue; // pas de ground truth disponible ici
                int d0=(*p0*2/256+1)/2; // depth en pixel
                if( d0>c ) continue; // match is outside image

                // second point
                for(int kk=0;kk<1;kk++) {
                    int ang=rand()%360;
                    for(k=0;k<maxDeltaP2;k++) {
                        delta dd=deltaT[k];
                        rr=r+dd.dy;
                        if( rr<0 || rr>=rows ) continue;
                        cc=r+dd.dx;
                        if( cc<0 || cc>=cols ) continue;
                        q1=q0+dd.dy*cols+dd.dx; // relatif a q0

                        // delta depth
                        p1=p0+dd.dy*cols+dd.dx; // deplacement relatif a p0
                        if( *p1==0 ) continue; // pas de ground truth disponible ici
                        int d1=(*p1*2/256+1)/2; // depth en pixel
                        if( d1>c ) continue; // match is outside image
                        if( d0<d1 ) deltaD=d1-d0; else deltaD=d0-d1;
                        if( deltaD>=maxDeltaD ) continue;

                        // delta couleur
                        if( (*q0)<(*q1) ) deltaC=(*q1)-(*q0); else deltaC=(*q0)-(*q1);
                        if( deltaC>=maxDeltaC ) continue; // ou possiblement deltaC=maxDeltaC-1;

                        //if( (deltaD*maxDeltaC+deltaC)*maxDeltaP2+dd.norm  >=sz ) {
                            //printf("(%d+%d,%d+%d) H[d=%3d c=%3d p=%4d]\n",r,dd.dy,c,dd.dx,deltaD,deltaC,dd.norm);
                        //}else{
                            H[(deltaD*maxDeltaC+deltaC)*maxDeltaP2+dd.norm]++;
                        //}
                    }
                }
            }
        }
    }

    // sauvegarde
    printf("saving... histoLissage.data\n");
    FILE *f=fopen("histoLissage.data","w");
    fwrite((void *)&maxDeltaD,sizeof(int),1,f);
    fwrite((void *)&maxDeltaC,sizeof(int),1,f);
    fwrite((void *)&maxDeltaP2,sizeof(int),1,f);
    fwrite((void *)H,sizeof(int),sz,f);
    fclose(f);

    free(H);
}


int main(int argc,char *argv[]) {

    char kittibase[200]="/opt/datasets/KITTI/";
    char dispname[200]="training/disp_occ/%06d_10.png";
    char imgname[200]="training/image_%d/%06d_10.png";
    int first=0;
    int last=193;


    /*
	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			printf("Usage: %s [-save toto%%04d.png]\n",argv[0]);
			exit(0);
		}else if( strcmp(argv[i],"-video")==0 ) {
			video=true;
			continue;
		}else if( strcmp(argv[i],"-save")==0 && i+1<argc ) {
			strcpy(filename,argv[i+1]);
			i+=1;continue;
		}else if( strcmp(argv[i],"-in")==0 && i+1<argc ) {
			strcpy(infilename,argv[i+1]);
			i+=1;continue;
		}else{ printf("??? %s\n",argv[i]);exit(0); }
	}
    */

#ifdef USE_PROFILOMETRE
	profilometre_init();
#endif

    cout << "(* opencv version " << CV_VERSION << "*)" << endl;
    cout << "(* imgv version   " << IMGV_VERSION << "*)" << endl;


	// tous les fichiers
	nb=last-first+1;
    disp=new Mat[nb];
    image0=new Mat[nb];
    image1=new Mat[nb];


	int wMin,wMax,hMin,hMax;
	wMin=hMin=99999;
	wMax=hMax=-99999;

	// crop a 1226x370
	// on devrait ajuster le rectangle selon l'image
	Rect myROI(0, 0, 1226, 370);
	Mat tmp;

	for(int i=0;i<nb;i++) {
		char name[200];

        //
        // disparite
        // 
		strcpy(name,kittibase);
		sprintf(name+strlen(name),dispname,i+first);

        tmp = imread(name, -1); 
		// clone pour forcer la reallocation de la memoire
		disp[i] = tmp(myROI).clone();

        //
        // image 0
        // 
		strcpy(name,kittibase);
		sprintf(name+strlen(name),imgname,0,i+first);
        tmp = imread(name, -1); 
		image0[i] = tmp(myROI).clone(); // clone pour forcer la reallocation de la memoire

        //
        // image 1
        // 
		strcpy(name,kittibase);
		sprintf(name+strlen(name),imgname,1,i+first);
        tmp = imread(name, -1); 
		image1[i] = tmp(myROI).clone(); // clone pour forcer la reallocation de la memoire

        /*
		if( disp[i].cols < wMin ) wMin=disp[i].cols;
		if( disp[i].cols > wMax ) wMax=disp[i].cols;
		if( disp[i].rows < hMin ) hMin=disp[i].rows;
		if( disp[i].rows > hMax ) hMax=disp[i].rows;
        */
	}

    // standard
    rows=image0[0].rows;
    cols=image0[0].cols;

    printf("{cols,rows}={%d,%d};\n",cols,rows);


	//cout << "w : "<<wMin<<" ... "<<wMax << endl;
	//cout << "h : "<<hMin<<" ... "<<hMax << endl;

    std::cout << "disp0.png\n";
	imwrite("disp0.png",disp[0]);

	imageFilterMean(nb,disp);

    std::cout << "disp1.png\n";
	imwrite("disp1.png",disp[0]);

	imageMoyenne(nb,disp);

    std::cout << "img0-0.png\n";
    std::cout << "img1-0.png\n";
	imwrite("img0-0.png",image0[0]);
	imwrite("img1-0.png",image1[0]);

    //histoDeltaCdepth(); // reset histoInfo.m, donc faire en premier!!
    //histoDepth();
    
    //histoLissage(100000);
    
    histoCout();


#ifdef USE_PROFILOMETRE
	profilometre_dump();
#endif
}



