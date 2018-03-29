
#version 130

// mix 3 (mono) textures as a single r,g,b image

uniform sampler2D texr;
uniform sampler2D texg;
uniform sampler2D texb;

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;
  vec4 tex;
  tex.r = texture2D(texr,pos).r;
  tex.g = texture2D(texg,pos).r;
  tex.b = texture2D(texb,pos).r;
  tex.a = 1.0;
  gl_FragColor  = tex;
}
