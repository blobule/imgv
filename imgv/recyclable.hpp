
#ifndef RECYCLABLE_HPP
#define RECYCLABLE_HPP

#include <imgv/rqueue.hpp>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <queue>


#include <stdio.h>
//#include <unistd.h> // pour le sleep


//
// plugin typique...
//
// source: -in recycle -out output
// filter: -in input -out output (ou -out null pour un sink)
//

using namespace std;


class recyclable {
    public:
	// devrait etre prive
	imgv::rqueue<recyclable> *Qrecycle;  // ou est-ce qu'on sera recycle?

	int id;
	static int refid;
	recyclable();
	virtual ~recyclable();
	void setRecycleQueue(imgv::rqueue<recyclable> *Q);
	// en static pour pouvoir remettre le pointeur a zero.
	static void recycle(recyclable **p);
};



#endif
