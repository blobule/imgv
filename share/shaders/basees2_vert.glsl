

attribute vec4 position;                // vertex position attribute
attribute vec2 texCoord;                // vertex texture coordinate attribute
 
// uniform mat4 modelView;                 // shader modelview matrix uniform
// uniform mat4 projection;                // shader projection matrix uniform
 
varying vec2 texCoordVar;               // vertex texture coordinate varying
 
void main()
{
    //vec4 p = modelView * position;      // transform vertex position with modelview matrix
    //gl_Position = projection * p;       // project the transformed position and write it to gl_Position
	gl_Position=position;
        texCoordVar = texCoord;             // assign the texture coordinate attribute to its varying
}


