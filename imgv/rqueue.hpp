
//
// queue multi-process
//
#ifndef RQUEUE_HPP
#define RQUEUE_HPP

#include <unistd.h>
#include <iostream>
//#include <iomanip>
//#include <string>
#include <vector>
//#include <queue>
#include <deque>

// pour verifier un typeid avant de vider la queue...
#include <typeinfo>

#include <stdio.h>
//#include <unistd.h> // pour le sleep

#ifdef WIN32
  //timespec already defined in unistd.h
  #define _TIMESPEC_DEFINED
#endif
// parallel
#include <pthread.h>
//#include <sched.h>
//#include <time.h>

// pour le dump en fichier
#include <iostream>
#include <fstream>

using namespace std;

namespace imgv {

//
// On peut mettre n'importe que dans la queue, en template
// ... a condition que ce soit un pointeur!!!!!!!
// rqueue<int> contient des pointeurs de int
//
// Two queue are used. One is regular, one is "control" (or "hi priority")
// Rule:
// - you can push message to the regular queue or control, at your choice.
// - when you pop from the queue
//   -> any message in the control Q will be returned first.
//   -> if there are no message in the control Q, then a message from the regular Q is ret.
//   -> if both Q are empty, then a wait is done.
//   -> a message arriving either in regular or control will wake up the thread.
// - you can pop from front or back of the regular queue, but control is always poped from front.
// - you can push to front or back of regular Q, but only to back of control Q
//
template <class T>
class rqueue {
	deque<T*> Q;	// regular queue -> all messages go here normally
	deque<T*> Qctrl; // control "hi priority queue" -> only "control" messages go here
	pthread_mutex_t *acces;  // pour acces general a la queue
	pthread_cond_t *not_empty; // condition pour attendre que la queue ne soit plus 
	public:
	string name;

// AJOUTER DESTRUCTEUR!
//int rc = pthread_cond_destroy(&got_request);
//if (rc == EBUSY) { /* some thread is still waiting on this condition variable */
//    /* handle this case here... */
//}

	rqueue() : name("noname") {
		initSync();
		//cout << "********** new rqueue "<<name<<endl;
	}
	rqueue(string name) : name(name) {
		initSync();
		//cout << "********** new rqueue "<<name<<endl;
	}
	~rqueue() {
		delete acces;
		delete not_empty;
	}

	// init the locking stuff */
	private:
    void initSync() {
		acces=new pthread_mutex_t;
		not_empty=new pthread_cond_t;
		pthread_mutex_init(acces,NULL);
		pthread_cond_init (not_empty,NULL);
	}
	public:

    // attention, size n'est pas fiable...
    // un autre thread pourrait ajouter/enlever des elements apres l'appel a size()
	// NOTE: on ne DOIT PAS faire d'attente qu'une queue soit d'une certaine taille
	//       si plusieurs thread peuvent ajouter/enlever des elements.
	int size() {
		pthread_mutex_lock(acces);
		int k=Q.size();
		pthread_mutex_unlock(acces);
		return k;
	}

    int size_ctrl() {
		pthread_mutex_lock(acces);
		int k=Qctrl.size();
		pthread_mutex_unlock(acces);
		return k;
	}

    // attention, empty n'est pas fiable... pour les meme raisons que size()
    bool empty() { return( size()==0 ) ; }
    bool empty_ctrl() { return( size_ctrl()==0 ) ; }

	//
	// push to the front of the main queue
	// THIS SHOULD BE EXCEPTIONAL! You should always push_back and pop_front.
	//

	// cette fonction n'est generalement jamais utilisee
    //void push_front(T x) { T *p=new T(x);push_front(&p); } // ATTENTION: fait une copie!

	// le pointeur p sera remis a NULL apres le push
	void push_front(T *&p) {
		if( p==NULL ) return;
		pthread_mutex_lock(acces);
		Q.push_front(p);p=NULL; // transfert! p ne sera plus disponible
			// on reveille tout le monde qui attend parce que dans le cond_wait
			// il est possible que d'autres elements s'ajoutent avant que le process
			// reveille recoive l'acces.
		if( Q.size()==1 ) pthread_cond_broadcast(not_empty);
		pthread_mutex_unlock(acces);
	}

	// pour utiliser en C traditionnel
    void push_front(T **p) {
		if( p==NULL || *p==NULL ) return;
		pthread_mutex_lock(acces);
		Q.push_front(*p);*p=NULL;
        if( Q.size()==1 ) pthread_cond_broadcast(not_empty);
        pthread_mutex_unlock(acces);
    }

	//
	// push to front, high priority. Not used normaly. Use push_back_ctrl
	//
	/*
	void push_front_ctrl(T *&p) {
		if( p==NULL ) return;
		pthread_mutex_lock(acces);
		Qctrl.push_front(p);p=NULL;
		if( Qctrl.size()==1 ) pthread_cond_broadcast(not_empty);
		pthread_mutex_unlock(acces);
	}
	*/

	//
	// push to the back of the main queue
	//

    //void push_back(T x) { T *p=new T(x);push_back(&p); } // copie!
	void push_back(T *&p) {
		if( p==NULL ) return;
		pthread_mutex_lock(acces);
		Q.push_back(p);p=NULL;
		if( Q.size()==1 ) { pthread_cond_broadcast(not_empty); }
		pthread_mutex_unlock(acces);
	}

    void push_back(T **p) {
		if( p==NULL || *p==NULL ) return;
		pthread_mutex_lock(acces);
		Q.push_back(*p);*p=NULL;
		if( Q.size()==1 ) { pthread_cond_broadcast(not_empty); }
		pthread_mutex_unlock(acces);
	}

	//
	// push to back, high priority.
	//

	void push_back_ctrl(T *&p) {
		if( p==NULL ) return;
		pthread_mutex_lock(acces);
		Qctrl.push_back(p);p=NULL;
		if( Qctrl.size()==1 ) { pthread_cond_broadcast(not_empty); }
		pthread_mutex_unlock(acces);
	}
	void push_back_ctrl(T **p) {
		if( p==NULL || *p==NULL ) return;
		pthread_mutex_lock(acces);
		Qctrl.push_back(*p);*p=NULL;
		if( Qctrl.size()==1 ) { pthread_cond_broadcast(not_empty); }
		pthread_mutex_unlock(acces);
	}

	//
	// Removing stuff from queues
	//

    // no wait. If the queue is empty, returns NULL
	// this is what is normaly used.
	T* pop_front() {
		T *p=NULL;
		pthread_mutex_lock(acces);
		// always Qctrl first
		if( Qctrl.size()>0 ) { p=Qctrl.front(); Qctrl.pop_front(); }
		else if( Q.size()>0 ) { p=Q.front(); Q.pop_front(); }
		pthread_mutex_unlock(acces);
		return(p);
	}

    // no wait. If the queue is empty, returns NULL
	// not normaly used, except to get the most recent element. Use pop_front.
	T *pop_back() {
		T *p=NULL;
		pthread_mutex_lock(acces);
		// always Qctrl first
		// ATTENTION: Qctrl is always emptied from the front!!!!
		if( Qctrl.size()>0 ) { p=Qctrl.front();Qctrl.pop_front(); }
		else if( Q.size()>0 ) { p=Q.back();Q.pop_back(); }
		pthread_mutex_unlock(acces);
		return(p);
	}

	// get element only from the Qctrl queue.
    // no wait. If the queue is empty, returns NULL
	T *pop_front_ctrl() {
		T *p=NULL;
		pthread_mutex_lock(acces);
		if( Qctrl.size()>0 ) { p=Qctrl.front();Qctrl.pop_front(); }
		pthread_mutex_unlock(acces);
		return(p);
	}

	// get element only from the regular queue only.
	// this should only be used when you don't want control messages.
    // no wait. If the queue is empty, returns NULL
	T *pop_front_noctrl() {
		T *p=NULL;
		pthread_mutex_lock(acces);
		if( Q.size()>0 ) { p=Q.front();Q.pop_front(); }
		pthread_mutex_unlock(acces);
		return(p);
	}
	// get element only from the regular queue only.
	// this should only be used when you don't want control messages.
    // no wait. If the queue is empty, returns NULL
	T *pop_back_noctrl() {
		T *p=NULL;
		pthread_mutex_lock(acces);
		if( Q.size()>0 ) { p=Q.back();Q.pop_back(); }
		pthread_mutex_unlock(acces);
		return(p);
	}

	// wait until an element is available in Q or Qctrl.
	// return Qctrl first. If nothing is availble, wait until Q or Qctrl is non empty
	T* const pop_front_wait() {
		T *p;
		bool done=false;
		do {
			pthread_mutex_lock(acces);
			// super important si jamais on recoit un cancel. Must match a pop()
			pthread_cleanup_push( (void (*)(void*))pthread_mutex_unlock,acces);
			// On boucle parce que cancel() ne garantie pas la condition
			// Si on recoit un signal pendant qu'on est ici, on ne voit rien passer...
			if( Q.size()==0 && Qctrl.size()==0 ) pthread_cond_wait(not_empty, acces);
			if( Qctrl.size()>0 ) { p=Qctrl.front(); Qctrl.pop_front(); done=true; }
			else if( Q.size()>0 ) { p=Q.front(); Q.pop_front(); done=true; }
			pthread_cleanup_pop(0);
			pthread_mutex_unlock(acces);
		} while( !done );
        return(p);
    }


	// note that Qctrl is always emptied from the front.
    T *pop_back_wait() {
		T *p;
		bool done=false;
		do {
			pthread_mutex_lock(acces);
			// super important si jamais on recoit un cancel. Must match a pop()
			pthread_cleanup_push( (void (*)(void*))pthread_mutex_unlock,acces);

			// On boucle parce que cancel() ne garantie pas la condition
			// Si on recoit un signal pendant qu'on est ici, on ne voit rien passer...
			if( Q.size()==0 && Qctrl.size()==0 ) pthread_cond_wait(not_empty, acces);
			// Qctrl is alwasy from the front!
			if( Qctrl.size()>0 ) { p=Qctrl.front(); Qctrl.pop_front(); done=true; }
			else if( Q.size()>0 ) { p=Q.back(); Q.pop_back(); done=true; }
			pthread_cleanup_pop(0);
			pthread_mutex_unlock(acces);
		} while( !done );
        return(p);
    }

	// wait until an element is available in Qctrl only.
	// return Qctrl first. If nothing is availble, wait until Q or Qctrl is non empty
	T* const pop_front_ctrl_wait() {
		T *p;
		bool done=false;
		do {
			pthread_mutex_lock(acces);
			// super important si jamais on recoit un cancel. Must match a pop()
			pthread_cleanup_push( (void (*)(void*))pthread_mutex_unlock,acces);
			// On boucle parce que cancel() ne garantie pas la condition
			// Si on recoit un signal pendant qu'on est ici, on ne voit rien passer...
			if( Qctrl.size()==0 ) pthread_cond_wait(not_empty, acces);
			if( Qctrl.size()>0 ) { p=Qctrl.front(); Qctrl.pop_front(); done=true; }
			pthread_cleanup_pop(0);
			pthread_mutex_unlock(acces);
		} while( !done );
        return(p);
    }

	// wait until an element is available in Q only.
	// this should be used exceptionaly hen no control messages can be processed.
	// return Qctrl first. If nothing is availble, wait until Q or Qctrl is non empty
	T* const pop_front_noctrl_wait() {
		T *p;
		bool done=false;
		do {
			pthread_mutex_lock(acces);
			// super important si jamais on recoit un cancel. Must match a pop()
			pthread_cleanup_push( (void (*)(void*))pthread_mutex_unlock,acces);
			// On boucle parce que cancel() ne garantie pas la condition
			// Si on recoit un signal pendant qu'on est ici, on ne voit rien passer...
			if( Q.size()==0 ) pthread_cond_wait(not_empty, acces);
			if( Q.size()>0 ) { p=Q.front(); Q.pop_front(); done=true; }
			pthread_cleanup_pop(0);
			pthread_mutex_unlock(acces);
		} while( !done );
        return(p);
    }


    //
    // output in graph form
    //
    void dump(ofstream &file) {
		file << "  Q"<<name << "[label=\""<<name<<"\"];" <<endl;
    }

};


}


#endif

