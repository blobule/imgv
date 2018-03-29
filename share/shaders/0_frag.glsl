
#version 130

uniform sampler2D tex;
uniform float param;

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;
  vec4 tex = texture2D(tex,pos);
  tex.a = tex.a*param;
  tex.rgb = fract(tex.rgb*256);
  tex.a = 1.0;
  gl_FragColor  = tex;
}
