#ifdef GL_ES
precision mediump float;
#endif

uniform float time; // in sec
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D video;
uniform sampler2D img;



void main( void ) {

   vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
   vec4 vid = texture2D(video,vec2(uv.x,1.0-uv.y));
   vec4 cam = texture2D(img,vec2(uv.x,1.0-uv.y));
   vec4 res = (vid+cam)/2.0;
   res.a = 1.0;
   gl_FragColor=res.rgba;

}

