#ifdef GL_ES
precision mediump float;
#endif

uniform float time; // in sec
uniform float fade;
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D video;
uniform sampler2D img; // camera raspicam
uniform sampler2D img2; // streaming


void main( void ) {

   vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
   vec4 vid = texture2D(video,vec2(uv.x,1.0-uv.y));
   vec4 cam = texture2D(img,vec2(uv.x*2.,1.0-uv.y));
   vec4 str = texture2D(img2,vec2((uv.x-0.5)*2.,1.0-uv.y));
   if( uv.x<0.5 ) { str.rgb=vec3(0.,0.,0.); } else { cam.rgb=vec3(0.,0.,0.); }
   vec4 res = (1.0-fade)*vid+fade*(cam+str);
   res.a = 1.0;
   gl_FragColor=res.rgba;

}

