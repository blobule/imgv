
#version 130

uniform sampler2D tex;
uniform mat3 homog;
uniform float fade;

// apply an homography to texture coords

void main (void)
{
  vec3 pos;
  pos.xy = gl_TexCoord[0].xy;
  pos.z=1.0;

  vec3 h = pos*homog;
  vec4 color=vec4(0.0,0.0,0.0,0.0);
  if( h.z!=0.0 ) {
	h.xy=h.xy/h.z;
	color.a=float(h.x>=0 && h.x<=1.0 && h.y>=0.0 && h.y<=1.0)*fade;
        color.rgb = texture2D(tex,h.xy).rgb*color.a;
  }
  gl_FragColor  = color;
}
