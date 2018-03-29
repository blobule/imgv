
#version 130

uniform sampler2D tex;
uniform int blub;

// blub: 0..10000 position sur la ligne

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;

  vec2 rot;
  float ang=float(blub)/10000.0*6.2831853;
  rot.x = cos(ang)*(pos.x-0.5) + sin(ang)*(pos.y-0.5) +0.5;
  rot.y = -sin(ang)*(pos.x-0.5) + cos(ang)*(pos.y-0.5) +0.5;
  vec4 tex = texture2D(tex,rot);

/*
  vec4 tex = texture2D(tex,pos);
  tex.r=blub/10000.0;
*/

  //tex.a=fade;
  tex.a=1.0;
  gl_FragColor  = tex;
}
