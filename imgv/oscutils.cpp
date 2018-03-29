
#include <oscutils.hpp>


// return true if string s ends with pattern pat
bool oscutils::endsWith(const char *s,const char *pat) {
	int slen=std::strlen(s);
	int plen=std::strlen(pat);
	if( plen>slen ) return false;
	return (std::strcmp(s+slen-plen,pat)==0);
}

//
// pour /a/bc/def
// retourne pos,len
// pour n=0 : pos=6,len=3 (def)
// pour n=1 : pos=3,len=2 (bc)
// pour n=2 : pos=1,len=1 (a)
// pour n>2 : retourne false
//
int oscutils::extractNbArgs(const char *s) {
	int nb=0;
	int len=std::strlen(s);
	for(int i=0;i<len;i++) if( s[i]=='/' ) nb++;
	return nb;
}

bool oscutils::extractArg(const char *s,int n,int &pos,int &le) {
	//cout << "extract arg "<<n<<" from string >"<<s<<"<"<<endl;
	int len=std::strlen(s);
	int start=len;
	int end=len;
	// ajust start
	for(int i=0;;) {
		while( start>0 && s[start-1]!='/' ) start--;
		if( end==0 ) return false; // not enough params
		//cout << "arg "<<i<<" is ("<<start<<"..."<<end<<") >";
		//for(int j=start;j<end;j++) cout << s[j]; cout << "<"<<endl;
		i++;if( i>n ) break;
		// passe au prochain arg
		end=start-1;
		start=end;
	}
	pos=start;
	le=end-start;
	return true;
}

// return true if argument #n match ref
bool oscutils::matchArg(const char *s,int n,const char *ref) {
	int pos,len;
	if( !extractArg(s,n,pos,len) ) return false; // no arg -> no match
	if( len==std::strlen(ref) && strncmp(ref,s+pos,len)==0 ) return true;
	return false;
}


int oscutils::ToInt32( const char *p )
{
#ifdef OSC_HOST_LITTLE_ENDIAN
    union{
	osc::int32 i;
	char c[4];
    } u;

    u.c[0] = p[3];
    u.c[1] = p[2];
    u.c[2] = p[1];
    u.c[3] = p[0];

    return u.i;
#else
	return *(int*)p;
#endif
}

void oscutils::FromInt32( char *p, int32 x )
{
#ifdef OSC_HOST_LITTLE_ENDIAN
    union{
        osc::int32 i;
        char c[4];
    } u;

    u.i = x;

    p[3] = u.c[0];
    p[2] = u.c[1];
    p[1] = u.c[2];
    p[0] = u.c[3];
#else
    *reinterpret_cast<int32*>(p) = x;
#endif
}



