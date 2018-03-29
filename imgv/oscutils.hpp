#ifndef OSCUTILS_HPP
#define OSCUTILS_HPP

#include <cstring>

#include <oscpack/osc/OscTypes.h>
#include <oscpack/osc/OscHostEndianness.h>

using namespace osc;

namespace oscutils {
	// return true if string s ends with pattern pat
        bool endsWith(const char *s,const char *pat);

	//
	// pour /a/bc/def
	// retourne pos,len
	// pour n=0 : pos=6,len=3 (def)
	// pour n=1 : pos=3,len=2 (bc)
	// pour n=2 : pos=1,len=1 (a)
	// pour n>2 : retourne false
	//
	int extractNbArgs(const char *s);
	bool extractArg(const char *s,int n,int &pos,int &le);
	bool matchArg(const char *s,int n,const char *ref);

	// convert to and from data type, independent of endianness
	int ToInt32( const char *p );
	void FromInt32( char *p, int32 x );
}


#endif
