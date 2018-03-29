#version 150 compatibility

uniform sampler2D tex;
uniform mat3 homog;
uniform float fade;
uniform sampler2D lut; // LUT (r,g,b)=(x,y,mask)

// apply an homography to texture coords
// attention: on utilise l inverse de lhomographie

void main (void)
{
  vec3 pos;
  vec2 lutpos = gl_TexCoord[0].xy;
  pos.xyz = texture2D(lut,lutpos).rgb;
  float mask=pos.b;

  //pos.xy = gl_TexCoord[0].xy;
  pos.z=1.0;

  vec3 h = pos*inverse(homog);
  //vec3 h=pos;

  vec4 color=vec4(0.0,0.0,0.0,0.0);
  if( h.z!=0.0 ) {
	h.xy=h.xy/h.z;
	color.a=float(h.x>=0 && h.x<=1.0 && h.y>=0.0 && h.y<=1.0)*fade*mask;
        color.rgb = texture2D(tex,h.xy).rgb*color.a;
  }
  gl_FragColor  = color;
}
