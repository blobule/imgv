
#ifndef IMQUEUE_HPP
#define IMQUEUE_HPP

#include <imgv/imgv.hpp>
#include <imgv/rqueue.hpp>

//
// Juste pour remplacer:
//   blob *i1;...;Q.push_back((recyclable *&)i1);
// par
//   blob *i1;...;Q.push_back(i1);
//
//
// On peut donc faire:
//
// blob *i1; ... ;Q.push_front(i1);
// message *m1; ... ; Q.push_front(m1);
// toto *t1; ... ; Q.push_front((recyclable *&)t1); // pour tous les autres
//
// pour lire un message, on peut faire:
// message *m1=Q.pop_front_nowait_msg(); // peut retourner null. recycle les images
// blob *b=Q.pop_front_nowait_image(); // peut retourner null. recycle les messages
//
//

class imqueue : public imgv::rqueue<recyclable> {

	public:
	imqueue(string name) : rqueue(name) { }
	~imqueue() { }

	//
	// This is simply to simplify the queue operations on images and messages
	// push to ctrl queue is only available for messages, and to the back of the queue
	//
	//
inline void push_back(message *&p) { imgv::rqueue<recyclable>::push_back((recyclable *&)p); }
inline void push_back_ctrl(message *&p) { imgv::rqueue<recyclable>::push_back_ctrl((recyclable *&)p); }
inline void push_back(blob *&p) { imgv::rqueue<recyclable>::push_back((recyclable *&)p); }
inline void push_back(recyclable *&p) { imgv::rqueue<recyclable>::push_back(p); }

inline void push_front(message *&p) { imgv::rqueue<recyclable>::push_front((recyclable *&)p); }
inline void push_front(blob *&p) { imgv::rqueue<recyclable>::push_front((recyclable *&)p); }
inline void push_front(recyclable *&p) { imgv::rqueue<recyclable>::push_front(p); }

//
// specialement pour ceux qui veulent utiliser une imqueue directement...
// (comme pour obtenir une reponse ou un status, ex: playMovie)
//
inline message *pop_front_ctrl_nowait() {
	recyclable *p=this->pop_front_ctrl();
	if( p==NULL ) return(NULL);
	message *q=dynamic_cast<message*>(p);
	if( q!=NULL ) return(q);
	recyclable::recycle(&p); // on recycle ce qui n'est pas un message
}

//
// specialement pour ceux qui veulent utiliser une imqueue directement...
// (comme pour obtenir une reponse ou un status, ex: playMovie)
//
inline blob *pop_front_nowait() {
	recyclable *p=this->pop_front();
	if( p==NULL ) return(NULL);
	blob *q=dynamic_cast<blob*>(p);
	if( q!=NULL ) return(q);
	recyclable::recycle(&p); // on recycle ce qui n'est pas une image
}

//
// specialement pour ceux qui veulent utiliser une imqueue directement...
// retourne une image OU un message, retourne 0 si rien lu, 1 si lu quelques chose
//
//
inline int pop_front_nowait_x(blob *&img,message *&msg) {
	img=NULL;
	msg=NULL;
	recyclable *p=this->pop_front();
	if( p==NULL ) return(0);
	msg=dynamic_cast<message*>(p);
	if( msg!=NULL ) return(1);
	img=dynamic_cast<blob*>(p);
	if( img!=NULL ) return(1);
	recyclable::recycle(&p); // on recycle ce qui n'est pas une image ou un message
	return(0);
}
inline int pop_front_x(blob *&img,message *&msg) {
	img=NULL;
	msg=NULL;
	recyclable *p=this->pop_front_wait();
	if( p==NULL ) return(0);
	msg=dynamic_cast<message*>(p);
	if( msg!=NULL ) return(1);
	img=dynamic_cast<blob*>(p);
	if( img!=NULL ) return(1);
	recyclable::recycle(&p); // on recycle ce qui n'est pas une image ou un message
	return(0);
}

};


#endif
