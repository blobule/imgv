#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "recyclable.hpp"
#include "config.hpp"

//
// un message est typiquement OSC
//

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"
#include "oscpack/osc/OscPrintReceivedElements.h"

#include "oscutils.hpp"

class messageDecoder {
	public:
	virtual bool decode(const osc::ReceivedMessage& m) {
		cout << "::messageDecoder:: "<<m<<endl;
		return false;
	}
};


class message : public recyclable {
        public:
	
	char *data; // [size+free]
	int size;  // used part of the data
	int free;  // free part of the data
	osc::OutboundPacketStream *ops; // nul si pas de data
	// pour le decodage local...
	vector<osc::ReceivedMessage> msgs; // decoded messages (un-bundled). use decode()

        message(int sz) { data=NULL;size=0;free=0;ops=NULL;reserve(sz); }
        ~message() {
		delete data;
		delete ops;
	}
	// pour faire un nouveau message
	// on ne peut pas ajouter quoi que ce soit dans le message avec ops.
	void set(const char *src,int rsize) {
		reserve(rsize);
		memcpy(data,src,rsize);
		free-=rsize;size=rsize;
		ops->Clear();
	}
	void reset() { free+=size;size=0;ops->Clear(); }
	void use(int nb) { free-=nb;size=nb; }
	void reserve(int nb) {
		free+=size;size=0;
		if( ops!=NULL && free>=nb ) { ops->Clear();return; } // recycle!
		// reallocate if needed!
		if( data!=NULL ) delete data;
		if( ops!=NULL ) delete ops;
		data=new char[nb];
		size=0;
		free=nb;
		ops=new osc::OutboundPacketStream(data,free);
	}
	void adjust() { use(ops->Size()); }

	//
	// pour l'encodage
	//
	// redefinir l'operateur <<
	message& operator<<( const osc::BundleInitiator& rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::BundleTerminator& rhs ) { *ops<<rhs;adjust();return *this; }
	message& operator<<( const osc::BeginMessage& rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::MessageTerminator& rhs ) { *ops<<rhs;adjust();return *this; }

	message& operator<<( bool rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::NilType& rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::InfinitumType& rhs ) { *ops<<rhs;return *this; }
	//message& operator<<( int32 rhs ) { *ops<<rhs;return *this; }
	message& operator<<( int rhs ) { *ops<<rhs;return *this; }

	message& operator<<( float rhs ) { *ops<<rhs;return *this; }
	message& operator<<( char rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::RgbaColor& rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::MidiMessage& rhs ) { *ops<<rhs;return *this; }
	//message& operator<<( int64_t rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::TimeTag& rhs ) { *ops<<rhs;return *this; }
	message& operator<<( double rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const char* rhs ) { *ops<<rhs;return *this; }
	message& operator<<( std::string s ) { *ops<<s.c_str();return *this; }
	message& operator<<( const osc::Symbol& rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::Blob& rhs ) { *ops<<rhs;return *this; }

	message& operator<<( const osc::ArrayInitiator& rhs ) { *ops<<rhs;return *this; }
	message& operator<<( const osc::ArrayTerminator& rhs ) { *ops<<rhs;return *this; }


	//
	// pour le decodage par un handler (messagedecoder)
	//
	void decodeMessage(const osc::ReceivedMessage& m,messageDecoder &d) {
		//cout << m <<endl;
		d.decode(m);
		// on pourrait test si ca retourne false (message pas reconnu)
	}
	void decodeBundle(const osc::ReceivedBundle& b,messageDecoder &d) {
		// ignore bundle time tag for now
		for( osc::ReceivedBundle::const_iterator i = b.ElementsBegin();
			i != b.ElementsEnd(); ++i ) {
			if( i->IsBundle() ) decodeBundle(osc::ReceivedBundle(*i),d);
			else                decodeMessage(osc::ReceivedMessage(*i),d);
		}
	}
	void decode(messageDecoder &d) {
		osc::ReceivedPacket p( data, size );
		if( p.IsBundle() )   decodeBundle(osc::ReceivedBundle(p),d);
		else                 decodeMessage(osc::ReceivedMessage(p),d);
	}

	//
	// decodage vers un vecteur local 'msgs' de received messages
	//
	void decodeBundle(const osc::ReceivedBundle& b) {
		// ignore bundle time tag for now
		for( osc::ReceivedBundle::const_iterator i = b.ElementsBegin();
			i != b.ElementsEnd(); ++i ) {
			if( i->IsBundle() ) decodeBundle(osc::ReceivedBundle(*i));
			else                msgs.push_back(osc::ReceivedMessage(*i));
		}
	}

	// local decode into a 'msgs', a vector of receivedMessages...
	//
	void decode() {
		msgs.clear(); // in case we call decode more than once
		osc::ReceivedPacket p( data, size );
		if( p.IsBundle() )	decodeBundle(osc::ReceivedBundle(p));
		else			msgs.push_back(osc::ReceivedMessage(p));
	}

};



#endif
