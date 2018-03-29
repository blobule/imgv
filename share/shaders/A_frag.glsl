
#version 130

uniform sampler2D tex;
uniform float fade;
uniform float angle;
uniform float scale;
uniform float sep;

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;
  vec2 rot;
  float ang=angle*3.14159/180.0;
  pos.x = pos.x+sep;
  rot.x = scale*(cos(ang)*(pos.x-0.5) + sin(ang)*(pos.y-0.5)) +0.5;
  rot.y = scale*(-sin(ang)*(pos.x-0.5) + cos(ang)*(pos.y-0.5)) +0.5;

  vec4 tex = texture2D(tex,rot);
  tex.a=fade;
  gl_FragColor  = tex;
}
