
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
  //rgba.r=pos.x;
  //rgba.g=pos.y;
  //rgba.b=1.0;
  rgba.bgra=(1.0-param)*tex.rgba+param*texx.rgba;
  //rgba.rgb=tex.rgb;
  //rgba.a=1.0;
  gl_FragColor  = rgba;
}
