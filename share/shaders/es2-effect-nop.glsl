#ifdef GL_ES
precision mediump float;
#endif

uniform float time; // in sec
//uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D prev_layer;
//uniform sampler2D luthi;
//uniform sampler2D lutlow;


void main( void ) {
   vec2 pos = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
//	pos.x = pos.x + sin (pos.x*100.0 + sin(time * 4.0)*2.0)* 0.01;
   vec2 delta = vec2(1.0, 1.0)/resolution.xy;
   //vec4 c = texture2D(prev_layer,pos);
   vec4 c = vec4(0.0, 0.0, 0.0, 0.0);
   for(int i = -2; i <= 2; i++){
	for(int j = -2; j <=2 ; j++){
		
   c += texture2D(prev_layer,pos+delta*vec2(float(i*2), float(j*2)))*sign(float(i))*5.0;
	}
   }
   c = c/25.0;
   c.a = 1.0;
   gl_FragColor=c;




}

