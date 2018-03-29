
/*!

\defgroup extractPattern extractPattern
@{
\ingroup examples

Analyse a video and extract N different images.

This is useful to get, for example, 50 images from a structured light scan from a video taken by a camera (like gopro) at 30 fps.

Usage
-----------

~~~
extractPattern -video toto.mp4 -n 50
~~~

To extract the most different 50 images from the video sequence<br>

Plugins used
------------
- \ref pluginFfmpeg

@}

*/


#include <imgv/imgv.hpp>

#include <imqueue.hpp>

#include <imgv/qpmanager.hpp>


#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <stdio.h>


using namespace std;
using namespace cv;

#ifdef APPLE
	#define NORMAL_SHADER "normal150"
#else
	#define NORMAL_SHADER "normal"
#endif


class info {
	public:
	int idx; // index 0 to nb-1
	int i; // image i in video
	int total; // difference between image[i] and image[i-1]

	info(int idx,int i,int total) {
		this->idx=idx;
		this->i=i;
		this->total=total;
	}
};

struct compare_total
{
    inline bool operator() (const info& A, const info& B)
    {
        return (A.total<B.total);
    }
};

struct compare_i
{
    inline bool operator() (const info& A, const info& B)
    {
        return (A.i<B.i);
    }
};


//
// plugin : clustersequence
//
/*
class pluginClusterSequence : public plugin<blob> {

	bool initializing;

	pluginClusterSequence() { }
	~pluginClusterSequence() { }
	void init() {
		initializing=true;
		pass=1;
	}

	bool loop() {
	}

	bool decode(const osc::ReceivedMessage &m) {
                const char *address=m.AddressPattern();
		if( initializing ) {
		}else{
		}
                if( oscutils::endsWith(address,"/pick") ) {
                        m.ArgumentStream() >> osc::EndMessage;
                        receiving++;
                }else{
                        log << "????? : " << m << err;
                }
        }

};
*/








//
// test plugin stream (receive)
//
void go(char *fname,int n,int start) {
	if( !qpmanager::pluginAvailable("pluginFfmpeg") ) {
		cout << "Need ffmpeg plugin!"<<endl;
		return;
	}
	if( !qpmanager::pluginAvailable("pluginViewerGLFW") ) {
		cout << "ViewerGLFW or Webcam is missing!" << endl;
		return;
	}

	//
	// les queues
	//
	qpmanager::addQueue("recycle");

        // ajoute quelques images a recycler
	qpmanager::addEmptyImages("recycle",20);

	// main queue pour faire le processing dans le main()
	qpmanager::addQueue("main");

	//
	// les plugins
	//
	qpmanager::addPlugin("V","pluginViewerGLFW");

	qpmanager::addPlugin("F","pluginFfmpeg");

	qpmanager::addQueue("display");

	qpmanager::connect("recycle","F","in");

	qpmanager::connect("main","F","out");

	qpmanager::connect("display","V","in");

	message *m;
	m=new message(1042);

	//
	// plugin ViewerGLFW
	//
	*m << osc::BeginBundleImmediate;
	*m << osc::BeginMessage("/new-window/main") << 100 << 100 << 640 << 480 << true << "Video IN" << osc::EndMessage;
	*m << osc::BeginMessage("/new-quad/main/A") << 0.0f << 1.0f << 0.0f << 1.0f << -10.0f << NORMAL_SHADER << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("display",m);

	//
	// plugin ffmpeg
	//
	m=new message(512);
	*m << osc::BeginBundleImmediate;

	*m << osc::BeginMessage("/max-fps") << -1.0f << osc::EndMessage; // normalement -1.0
	*m << osc::BeginMessage("/start") << fname << "" << "rgb" << "/main/A/tex" << 0 << osc::EndMessage;
	// on utilise defevent plutot que defevent-ctlr pour que /movie-done arrive apres la derniere image
	*m << osc::BeginMessage("/defevent") << "end" << "out" << "/movie-done" << osc::EndMessage;
	*m << osc::BeginMessage("/init-done") << osc::EndMessage;
	*m << osc::EndBundle;
	qpmanager::add("recycle",m);

	//
	// mode full automatique
	//
	qpmanager::start("V");
	qpmanager::start("F");


	//
	// main analyze loop....
	//
	bool done=false;

	blob *ilast=NULL;

	vector<info> diff;

	Mat delta;

	int idx=0; // index de l'image

	for(int i=0;!done;i++) {
		message *m;
		blob *i1;
		// get next image and/or message
		int k=qpmanager::getQueue("main")->pop_front_x(i1,m);
		if( m!=NULL ) {
		    m->decode();
		    for(int i=0;i<m->msgs.size();i++) {
			osc::ReceivedMessage rm=m->msgs[i];
			const char *address=rm.AddressPattern();
			if( oscutils::endsWith(address,"/movie-done") ) { done=true; }
			cout << "GOT "<< m->msgs[i] <<endl;
		    }
		    if( done ) break;
		}
		if( i1!=NULL ) {
			cout << "got image "<<i<<" ("<<i1->n<<")"<<endl;
			if( i1->n < start ) {
				cout << "skipping image " << i1->n << endl;
				recyclable::recycle((recyclable **)&i1); // skip
				continue;
			}
#ifdef TESTING_ONLY
			if( i1->n==390 ) {
				// tell the movie to stop
				m=new message(512);
				*m << osc::BeginMessage("/stop") << osc::EndMessage;
				qpmanager::add("recycle",m);
			}
#endif
			// compare with last image
			if( ilast==NULL ) {
				info z(idx,i1->n,0); idx++;
				diff.push_back(z); // no difference
			}else{
				cv::absdiff(*ilast,*i1,delta);
				int total=0;
				unsigned char *p=delta.data;
				int sz=delta.cols*delta.rows*delta.elemSize();
				for(int r=sz;r>0;r--) total+=*p++;
				/*
				int total=0;
				unsigned char *p=i1->data;
				unsigned char *q=ilast->data;
				int sz=i1->cols*i1->rows*i1->elemSize();
				for(int r=0;r<sz;r++,p++,q++) {
					if( *p<*q )	total+=(*q-*p);
					else		total+=(*p-*q);
				}
				//printf("image %4d size=(%d,%d) elemsize=%d\n",i1->n,i1->cols,i1->rows,i1->elemSize());
				*/
				printf("image %4d total=%10d\n",i1->n,total);
				info z(idx,i1->n,total);idx++;
				diff.push_back(z); // no difference
			}
			if( ilast!=NULL ) qpmanager::add("display",ilast);
			ilast=i1;
		}
	}
	if( ilast!=NULL ) { qpmanager::add("display",ilast);ilast=NULL; }

	//
	// non-maximum supression...
	// VERY IMPORTANT!
	// u<=v<=w -> d=v-u, v-=d, w+=d,  u>=v>=w -> d=v-w, v-=d, u+=d, sinon v
	// 
	for(int i=0;i<10;i++) {
		for(int j=1;j<diff.size()-1;j++) {
			int u=diff[j-1].total;
			int v=diff[j].total;
			int w=diff[j+1].total;
			int d;
			if( u<v && v<=w ) { d=v-u;diff[j].total-=d;diff[j+1].total+=d; }
			else if( u>=v && v>w ) { d=v-w;diff[j].total-=d;diff[j-1].total+=d; }
		}
	}
	for(int j=0;j<diff.size();j++) printf("xxx %4d idx=%04d img=%04d %8d\n",j,diff[j].idx,diff[j].i,diff[j].total);

	// sameas table
	int nb=diff.size();
	int parent[nb];
	for(int i=0;i<nb;i++) parent[i]=i;

	// find clusters
	// si [i] est bas, on peut joindre [i] et [i-1]

	// sort differences
	sort(diff.begin(),diff.end(),compare_total());

	int nbcluster=nb;
	for(int i=0;i<nb && nbcluster>n;i++) {
		// merge next!
		if( diff[i].idx==0 ) continue; // 0 cant be merged
		int j=diff[i].idx;
		int s1=j; // parent de [j]
		while( s1!=parent[s1] ) s1=parent[s1];
		int s2=j-1; // parent de [j-1]
		while( s2!=parent[s2] ) s2=parent[s2];
		printf("merge %d (parent=%d) and %d (parent=%d) (nbcluster=%d)\n",j,s1,j-1,s2,nbcluster);
		// deja dans le meme groupe?
		if( s1==s2 ) { parent[j]=s1;parent[j-1]=s2;continue; }
		// on merge!
		int s0=(s1<s2?s1:s2); // on garde le plus petit comme reference
		parent[j]=s0;
		parent[j-1]=s0;
		nbcluster--;
	}

	// ajuste tous les parents une derniere fois
	for(int i=0;i<nb;i++) {
		int s=i;
		while( s!=parent[s] ) s=parent[s];
		parent[i]=s;
	}

	for(int i=0;i<nb;i++) printf("%5d parent=%5d\n",i,parent[i]);

	// ramasse les clusters. On ajoute 1 pour indiquer la fin
	int clusterBegin[n+1];
	int i,j;
	for(i=0,j=0;i<nb;i++) {
		if( i==0 || parent[i]!=parent[i-1] ) { clusterBegin[j++]=i; }
	}
	clusterBegin[n]=nb;

	for(i=0;i<n;i++) printf("Cluster %d : start %d end %d\n",i,clusterBegin[i],clusterBegin[i+1]-1);

	// put the difference back in order of position
	sort(diff.begin(),diff.end(),compare_i());

	int clusterSelect[n];
	// for each cluster, select the image with smallest difference with previous
	for(int c=0;c<n;c++) {
		int imin=-1;
		for(int i=clusterBegin[c];i<clusterBegin[c+1];i++) {
			if( i!=0 && ( imin<0 || diff[i].total<diff[imin].total ) ) imin=i;
		}
		clusterSelect[c]=diff[imin].i; // le numero d'image
		printf("Cluster %d select %d image %d\n",c,imin,clusterSelect[c]);
	}


	// restart the movie...
	m=new message(512);
	*m << osc::BeginMessage("/rewind") << osc::EndMessage;
	qpmanager::add("recycle",m);

	//
	// main extract loop....
	//
	done=false;
	bool waitingZero=true;
	int c=0; // current cluster
	for(int i=0;!done;i++) {
		message *m;
		blob *i1;
		// get next image and/or message
		int k=qpmanager::getQueue("main")->pop_front_x(i1,m);
		if( m!=NULL ) {
		    m->decode();
		    for(int i=0;i<m->msgs.size();i++) {
			osc::ReceivedMessage rm=m->msgs[i];
			const char *address=rm.AddressPattern();
			if( oscutils::endsWith(address,"/movie-done") ) { done=true; }
			cout << "GOT "<< m->msgs[i] <<endl;
		    }
		    if( done ) break;
		}
		if( i1!=NULL ) {
			cout << "got image "<<i<<" ("<<i1->n<<")"<<endl;
			// we wait in case we stopped the playback at a specific image number instead of /movie-done
			if( waitingZero ) {
				if( i1->n==0 ) {
					waitingZero=false;
				}else{
					printf("Skip until 0\n");
					recyclable::recycle((recyclable **)&i1);
					continue;
				}
			}
			if( i1->n<clusterSelect[c] ) {
				recyclable::recycle((recyclable **)&i1); // skip
				continue;
			}
			cout << "KEEP "<<i1->n<<endl;
			char buf[100];
			sprintf(buf,"cam%03d.png",c);
			imwrite(buf,*i1);
			cout << "saved "<<buf<<endl;
			qpmanager::add("display",i1);
			c++;
			if( c==n ) done=true;
		}
	}

	sleep(1);

	m=new message(512);
	*m << osc::BeginMessage("/kill") << osc::EndMessage;
	qpmanager::add("display",m);

	qpmanager::stop("F");
	//qpmanager::stop("V");
}


int main(int argc,char *argv[]) {


	qpmanager::initialize();

	char *fname=NULL;
	int n=8; // <0 = auto
	int start=0; // premier image utilisee

	for(int i=1;i<argc;i++) {
		if( strcmp(argv[i],"-h")==0 ) {
			cout << "extractPattern -video toto.mp4 -n 50\n" << endl;
			exit(0);
		}
		if( strcmp(argv[i],"-video")==0 && i+1<argc ) {
			fname=argv[i+1];
			i++;continue;
		}
		if( strcmp(argv[i],"-n")==0 && i+1<argc ) {
			n=atoi(argv[i+1]);
                        i++;continue;
                }
		if( strcmp(argv[i],"-start")==0 && i+1<argc ) {
			start=atoi(argv[i+1]);
                        i++;continue;
                }
	}

	cout << "opencv version " << CV_VERSION << endl;

	if( fname==NULL ) exit(0);

	go(fname,n,start);
}


