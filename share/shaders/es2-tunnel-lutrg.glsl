// from: http://glsl.heroku.com/e#3259.0
#ifdef GL_ES
precision mediump float;
#endif
//gt
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D luthi;
uniform sampler2D lutlow;

void main( void ) {

// va chercher la LUT

   vec2 lutuv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
   lutuv.y=1.0-lutuv.y; // reverse Y
   vec4 coordshi = texture2D(luthi,lutuv);
   vec4 coordslow = texture2D(lutlow,lutuv);
   vec2 coords = coordshi.bg + coordslow.bg/256.0 + sin(time)/256.0;
   coords.y=1.0-coords.y;

// on utilise coords a la place de FragCoord/resolution

	 
    vec2 p = -1.0 + 2.0 * coords; //gl_FragCoord.xy / resolution.xy;
    vec2 uv;
	 //shadertoy deform "relief tunnel"-gt
     float r = sqrt( dot(p,p) );
     float a = atan(p.y,p.x) + 0.9*sin(0.5*r-0.5*time*0.1);

	 float s = 0.5 + 0.5*cos(7.0*a);
     s = smoothstep(0.0,1.0,s);
     s = smoothstep(0.0,1.0,s);
     s = smoothstep(0.0,1.0,s);
     s = smoothstep(0.0,1.0,s);

     uv.x = time*0.18 + 1.0/( r + .2*s);
       //uv.y = 3.0*a/3.1416;
	   uv.y = 1.0*a/10.1416;

       float w = (0.5 + 0.5*s)*r*r;

   	   // vec3 col = texture2D(tex0,uv).xyz;

       float ao = 0.5 + 0.5*cos(42.0*a);//amp up the ao-g
       ao = smoothstep(0.0,0.4,ao)-smoothstep(0.4,0.7,ao);
       ao = 1.0-0.5*ao*r;
	   
	   //faux shaded texture-gt
	   float px = coords.x; //gl_FragCoord.x/resolution.x;
	   float py = coords.y; //gl_FragCoord.y/resolution.y;
	   float x = mod(uv.x*resolution.x,resolution.x/3.0);
	   float y = mod(uv.y*resolution.y+(resolution.y/2.),resolution.y/3.5);
	   float v =  (x / y) - 0.7;
	   gl_FragColor = vec4(vec3(.1-v,.9-v,1.-v)*w*ao,1.0);

}
