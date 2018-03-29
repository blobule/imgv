

#include "parseShader.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

imgv::parseShader::parseShader(int max) {
	maxUni=max;
	nbUni=0;
	uniTypes=(char **)malloc(maxUni*sizeof(char *));
	uniNames=(char **)malloc(maxUni*sizeof(char *));
}

//
// extraction des uniformes d'un shader
// rempli les nbUni, uniTypes et uniNames
//
void imgv::parseShader::parse(const std::string s)
{
int pos;
int pend;
char name[100];
char type[100];

        pos=pend=0;
        while( (pos=s.find("uniform ", pend))!=std::string::npos ) {
                if( (pend=s.find(";",pos))==std::string::npos || pend-pos>100 ) break;
                if( sscanf(s.c_str()+pos," uniform %[^ ] %[^ ;] ;",type,name)!=2 ) { printf("Unable to parse pos %d to %d\n",pos,pend);break;}
                //printf("Found 'uniform' at pos %d to pos %d, type='%s' name='%s'\n",pos,pend,type,name);

                if( nbUni>=maxUni ) { printf("Too many uniforms, max is %d\n",maxUni); break; }
                uniTypes[nbUni]=strdup(type);
                uniNames[nbUni]=strdup(name);
		//Qnames[nbUni]=NULL;
		//Q[nbUni]=NULL;
                nbUni++;
        }
}

imgv::parseShader::~parseShader() {
	printf("--- destroy parseShader ---\n");
	int i;
	for(i=0;i<nbUni;i++) {
		if( uniTypes[i] ) free(uniTypes[i]);
		if( uniNames[i] ) free(uniNames[i]);
	}
	free(uniTypes);
	free(uniNames);
}

bool imgv::parseShader::isTexture(int i) {
	if( i<0 || i>=nbUni ) return false;
	if( strcmp("sampler2D",uniTypes[i])==0 ) return true;
	return false;
}

bool imgv::parseShader::isFloat(int i) {
	if( i<0 || i>=nbUni ) return false;
	if( strcmp("float",uniTypes[i])==0 ) return true;
	return false;
}

bool imgv::parseShader::isInt(int i) {
	if( i<0 || i>=nbUni ) return false;
	if( strcmp("int",uniTypes[i])==0 ) return true;
	return false;
}

bool imgv::parseShader::isVec4(int i) {
	if( i<0 || i>=nbUni ) return false;
	if( strcmp("vec4",uniTypes[i])==0 ) return true;
	return false;
}

bool imgv::parseShader::isMat4(int i) {
	if( i<0 || i>=nbUni ) return false;
	if( strcmp("mat4",uniTypes[i])==0 ) return true;
	return false;
}

bool imgv::parseShader::isMat3(int i) {
    if( i<0 || i>=nbUni ) return false;
    if( strcmp("mat3",uniTypes[i])==0 ) return true;
    return false;
}

void imgv::parseShader::dump(void) {
        printf("--- uniforms (max=%d, nb=%d) ---\n",maxUni,nbUni);
        int i;
        for(i=0;i<nbUni;i++) printf("%d : t=%s n=%s (isTexture=%d)\n",i,uniTypes[i],uniNames[i],isTexture(i));
}


