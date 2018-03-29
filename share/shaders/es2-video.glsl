#ifdef GL_ES
precision mediump float;
#endif

uniform float time; // in sec
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D video;



void main( void ) {

   vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
   vec4 c = texture2D(video,vec2(uv.x,1.0-uv.y));
   c.a = 1.0;
   gl_FragColor=c.rgba;

}

