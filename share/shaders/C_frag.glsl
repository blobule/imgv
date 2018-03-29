
#version 130

uniform sampler2D tex;
uniform float fade;
uniform float angle;

varying vec3 v;
float rand(vec2 n)
{
  return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

void main (void)
{
  vec2 pos = gl_TexCoord[0].xy;
  vec2 rot;
/*
  float ang=angle*6.28;
  rot.x = cos(ang)*(pos.x-0.5) + sin(ang)*(pos.y-0.5) +0.5;
  rot.y = -sin(ang)*(pos.x-0.5) + cos(ang)*(pos.y-0.5) +0.5;
*/
  rot.x = (pos.x)/max(angle,0.01);
  rot.y = (pos.y)/max(angle,0.01);

  rot.x += rand(pos.xy)*0.05;
  rot.y += rand(1.0-pos.xy)*0.05;

  vec4 tex = texture2D(tex,rot);
  //tex.a=fade;
  tex.a=1.0;

  float lum=(tex.r+tex.g+tex.b)/3.0;

  vec4 gris = vec4(lum,lum,lum,1.0);
  vec4 final= (1.0-fade)*gris + fade*tex;

  gl_FragColor  = final;
}
