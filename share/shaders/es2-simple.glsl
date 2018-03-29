#ifdef GL_ES
precision mediump float;
#endif

uniform float time; // in sec
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D img;


void main( void ) {

   vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
   vec4 c = texture2D(img,vec2(uv.x,1.0-uv.y));
   gl_FragColor=c.bgra;
   //gl_FragColor=vec4(uv.x,uv.y,(sin(time)+1.0)/2.0,1.0);

}

