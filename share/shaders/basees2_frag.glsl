

uniform sampler2D tex;


precision mediump float;        // set default precision for floats to medium
varying vec2 texCoordVar;       // fragment texture coordinate varying
 
void main()
{
    // sample the texture at the interpolated texture coordinate
    // and write it to gl_FragColor
    gl_FragColor = texture2D( tex, texCoordVar);
	gl_FragColor=vec3(1,1,0);
}
