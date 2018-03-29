#ifndef PARSESHADER_H
#define PARSESHADER_H

#include <string>


//
// parse a shader to extract uniform type and names
//

namespace imgv {

class parseShader {

public:
	parseShader(int max=20);
	~parseShader();

	void parse(const std::string s);

	bool isTexture(int i);
	bool isFloat(int i);
	bool isInt(int i);
	bool isVec4(int i);
	bool isMat4(int i);
    bool isMat3(int i);

	void dump(void);

	int maxUni; // size of tables to store stuff
	int nbUni;
	char **uniTypes; // [_maxUni]
	char **uniNames; // [_maxUni]
};

}

#endif

