
#include <imgv.hpp>


int main(int argc,char *argv[]) {

	cout << "--- test blob ---" << endl;

	blob i0=blob::pngread("/usr/local/share/imgv/images/scan.png");

	cout << "image is "<<i0.cols<<" x "<<i0.rows<<endl;

	i0.dumpText();

	blob::pngwrite("blub0.png",i0,1);

	cout << "----------------------------"<<endl;
	blob i1(2,2, CV_8UC1);
	blob::pngwrite("blub1.png",i1,1);

	cout << "----------------------------"<<endl;
	blob i2(2,2, CV_8UC3);
	blob::pngwrite("blub2.png",i2,1);

	cout << "----------------------------"<<endl;
	blob i3(2,2, CV_16UC1);
	blob::pngwrite("blub3.png",i3,1);

	cv::Mat m(3,3,CV_64F);
#ifdef WIN32	
	srand(1234);
#else
	srand48(1234);
#endif
	int x,y;
	for(y=0;y<m.rows;y++) for(x=0;x<m.cols;x++)
    {
#ifdef WIN32	
  	  m.at<double>(x,y)=(double)(rand())/RAND_MAX;
#else
	  m.at<double>(x,y)=drand48();
#endif
	}

	cout << m << endl;

/****
	//==== storing data ====
	cv::FileStorage fs(".xml", cv::FileStorage::WRITE + cv::FileStorage::MEMORY);
	cout << "sizeof is "<<sizeof(fs)<<endl;
	fs << "date" << "16-dec-2013";
	fs << "rotation" << m;
	fs << "img" << i2;
	string buf = fs.releaseAndGetString();
	cout << buf <<endl;

	//==== reading it back ====
	fs.open(buf, cv::FileStorage::READ + cv::FileStorage::MEMORY);
	//cv::FileStorage fsr(buf, cv::FileStorage::READ + cv::FileStorage::MEMORY);
	//fs["date"] >> date_string;
	//fs["mymatrix"] >> mymatrix;
	cv::FileNode root=fs.root(0);
	for(cv::FileNodeIterator fni = root.begin(); fni != root.end(); fni++) {
		std::string key = (*fni).name();
		cout << "key is "<<key<<endl;
	}
	fs.release(); // not needed when reading
**/


	cout << "----------------------------"<<endl;
	blob i4=blob::pngread("/usr/local/share/imgv/images/scan8.png");
	i4.texts["bonjour"]="hello";
	i4.dumpText();

	double toto=1.23456;

	// extra data
	cv::FileStorage fs(".xml", cv::FileStorage::WRITE + cv::FileStorage::MEMORY);
	fs << "date" << "16-dec-2013";
	fs << "rotation" << m;
	fs << "img" << i2;
	fs << "toto" << toto;

	blob::pngwrite("blub4.png",i4,1,&fs);

/*
	cout << "----------------------------"<<endl;
	blob i5(100,100, CV_32SC1);
	srand48(100);
	int x,y;
	for(y=0;y<i5.rows;y++) for(x=0;x<i5.cols;x++) {
		i5.at<int>(x,y)=mrand48();
		cout << i5.at<int>(x,y)<<endl;
	}
	blob::imwrite("blub5.png",i4,1);

	cout << "----------------------------"<<endl;
	blob i6(2,2, CV_64FC3);
	blob::imwrite("blub6.png",i6,1);
*/

	blob i5=blob::pngread("blub4.png",&fs);
	cout << "got date "<<(string)fs["date"]<<endl;
	cv::Mat mm;
	fs["rotation"]>>mm;
	cout << "got rotation "<<mm<<endl;
	cv::Mat mm2;
	fs["img"]>>mm2;
	cout << "got i2 "<<mm2<<endl;
	fs["toto"]>>toto;
	cout << "goto toto "<<toto<<endl;
	fs.release();

	return 0;

}

