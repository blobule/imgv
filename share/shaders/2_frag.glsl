
#version 130

uniform sampler2D extraTex;
uniform sampler2D myTexture;
uniform float param;

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;
  vec4 tex = texture2D(myTexture,pos);
  vec4 texx = texture2D(extraTex,pos);
  vec4 rgba;
  rgba.rgba=(param)*tex.rgba+(1.0-param)*texx.rgba;
  gl_FragColor  = rgba;
}
