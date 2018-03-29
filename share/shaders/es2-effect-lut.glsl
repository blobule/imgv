#ifdef GL_ES
precision mediump float;
#endif

uniform float time; // in sec
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D prev_layer;
uniform sampler2D luthi;
uniform sampler2D lutlow;


void main( void ) {
   vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
	uv.y=1.0-uv.y; // flip vertical
   vec4 coordshi = texture2D(luthi,uv);
   vec4 coordslow = texture2D(lutlow,uv);

   vec2 d = vec2(time/64.0,0.0);
   vec2 coords = coordshi.gb + coordslow.gb/256.0;
   //vec2 coords = coordshi.bg;

// la lut est en bgr, donc x=b, y=g, et mask=r
   vec4 c = texture2D(prev_layer,vec2(fract(coords.x+0.25),1.0-coords.y));

//   c.a=coordshi.r;
	c.a=1.0;
  	c.rgb=c.rgb*pow(coordshi.r,0.7);
	//c.rgb=texture2D(luthi,uv).rgb;
	//c.rgb=texture2D(luthi,uv).rgb;

   gl_FragColor=c.rgba;

    //gl_FragColor=vec4(coordslow.r,coordslow.g,coordslow.b,1.0);

//	gl_FragColor=vec4(0.5,0.7,0.2,1.0);


}

