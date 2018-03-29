
#version 130

uniform sampler2D tex;
uniform sampler2D homog;

// LUT for homography (r,g,b)=(x,y,mask)

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;
  vec3 h = texture2D(homog,pos).rgb;

  vec4 color = texture2D(tex,h.xy)*h.z;
  //color.a = h.z;
  gl_FragColor  = color;
}
