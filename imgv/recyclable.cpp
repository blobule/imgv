
#include <recyclable.hpp>

// le static compteur. juste pour info.
int recyclable::refid=0;

recyclable::recyclable() {
	id=refid++;
	Qrecycle=NULL;
	//cout << "recyclable "<<id<<" created"<<endl;
}

recyclable::~recyclable() {
	//cout << "recyclable "<<id<<" deleted" << endl;
}

void recyclable::setRecycleQueue(imgv::rqueue<recyclable> *Q) { Qrecycle=Q; }

void recyclable::recycle(recyclable **p) {
	if( p==NULL ) return;
	if( *p==NULL ) return;
	if( (*p)->Qrecycle==NULL ) { delete(*p);*p=NULL; }
	else (*p)->Qrecycle->push_front(p); // reuse as fast as possible
}



