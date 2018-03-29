
#version 130

// applique une distorsion radiale
// (assume que la texture est deja entre -1 et 1)
// 1,0.22,0.24,0 -> k0..k3
//
vec2 radialWarp(vec2 p,vec4 k,float center)
{
    p.x-=center;
        float rSq = p.x*p.x + p.y*p.y;
        vec2 rvector= p * (k.x + k.y * rSq + k.z * rSq * rSq + k.w * rSq * rSq * rSq);
    rvector.x+=center;
        return rvector;
}


uniform sampler2D tex;

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;
	// pos est de 0 a 1 normalement...
  vec2 p=(pos-vec2(0.5,0.5))*vec2(2,2/1.4);
  float r = sqrt( dot(p,p) );
  vec2 qr=p/vec2(2,2/1.4)*0.991+vec2(0.5,0.5);
  vec2 qg=p/vec2(2,2/1.4)*1.000+vec2(0.5,0.5);
  vec2 qb=p/vec2(2,2/1.4)*1.009+vec2(0.5,0.5);
  vec4 t;
  //if( r>1 ) t=vec4(1,0.3,0,1);
  //else{
	t.r = texture2D(tex,qr).r;
	t.g = texture2D(tex,qg).g;
	t.b = texture2D(tex,qb).b;
	t.a = 1.0;
  //}
  gl_FragColor  = t;
}
