
#version 150 compatibility

uniform sampler2D tex;

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;
  vec4 tex = texture2D(tex,pos);
  gl_FragColor  = tex;
}

