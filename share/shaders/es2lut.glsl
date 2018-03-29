#ifdef GL_ES
precision mediump float;
#endif

uniform float time; // in sec
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D img;
uniform sampler2D luthi;
uniform sampler2D lutlow;


void main( void ) {
   vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
   vec4 coordshi = texture2D(luthi,uv);
   vec4 coordslow = texture2D(lutlow,uv);

   vec2 coords = coordshi.bg + /*coordslow.bg/256.0*/ + sin(mouse.xy/16.0)/4.0;
   //vec2 coords = coordshi.bg;

// la lut est en bgr, donc x=b, y=g, et mask=r
   vec4 c = texture2D(img,vec2(coords.x,1.0-coords.y));
   c.a=coordshi.r;
   gl_FragColor=c.bgra;

    //gl_FragColor=vec4(coordslow.r,coordslow.g,coordslow.b,1.0);

//	gl_FragColor=vec4(0.5,0.7,0.2,1.0);


}

