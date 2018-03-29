
#version 130

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D lutleft0;
uniform sampler2D lutleft1;
uniform sampler2D lutleft2;
uniform sampler2D lutright0;
uniform sampler2D lutright1;
uniform sampler2D lutright2;
uniform float yawparam;
uniform float pitchparam;
uniform float fade;

//uniform int eye;
//float fovomni;

//eye: 0=left, 1=right, 2=stereo red-cyan

// YUV offset
const vec3 offset = vec3(-0.0625, -0.5, -0.5);

// RGB coefficients
const vec3 Rcoeff = vec3(1.164,  0.000,  1.596);
const vec3 Gcoeff = vec3(1.164, -0.391, -0.813);
const vec3 Bcoeff = vec3(1.164,  2.018,  0.000);

vec3 yuv2rgb(sampler2D ttt,in vec2 p) 
{
  //compute YUV sample locations from p.xy
  vec2 py = vec2( p.x, p.y * 0.666666666);
  vec2 pu = vec2( p.x/2.0, (p.y+2.0)/3.0);
  vec2 pv = vec2( (p.x+1.0)/2.0, (p.y+2.0)/3.0 );
  //YUV to RGB conversion
  vec3 pyuv;
  pyuv.x = texture2D( ttt, py ).r;
  pyuv.y = texture2D( ttt, pu ).r;
  pyuv.z = texture2D( ttt, pv ).r;
  vec3 prgb;
  pyuv += offset;
  prgb.r = dot(pyuv, Rcoeff);
  prgb.g = dot(pyuv, Gcoeff);
  prgb.b = dot(pyuv, Bcoeff);
  return prgb;
}

float atan2_local(in float px,in float py) 
{
  return(2.0*atan(py/(sqrt(px*px+py*py)+px)));
}

// applique une distorsion radiale
// (assume que la texture est deja entre -1 et 1)
// 1,0.22,0.24,0 -> k0..k3
//
vec2 radialWarp(vec2 p,vec4 k,float center)
{
    p.x-=center;
	float rSq = p.x*p.x + p.y*p.y;
	vec2 rvector= p * (k.x + k.y * rSq + k.z * rSq * rSq + k.w * rSq * rSq * rSq);
    rvector.x+=center;
	return rvector;
}

vec4 map2fisheye(vec4 ray,float fovomni,float fovcam,float pitch,float yaw,float convergence,float tex_x_offset,int sign)
{
    // ray goes from -1 to 1 in x and y
    vec4 k=vec4(1.0,0.22,0.24,0.0);
    vec4 sray=ray;
    sray.xy=radialWarp(ray.xy,k,sign*0.1453);

    sray.x-=sign*tex_x_offset;

    float ssize=tan(fovcam/2)*sray.z;
    sray.x*=ssize;
    sray.y*=ssize*1.25; //OculusRift aspect ratio
    sray.x=-sray.x;
    sray.y=-sray.y;

    //apply pitch
    float tmp=cos(pitch)*sray.y-sin(pitch)*sray.z;
    sray.z=sin(pitch)*sray.y+cos(pitch)*sray.z;
    sray.y=tmp;

    //apply yaw
    tmp=cos(yaw)*sray.x+sin(yaw)*sray.z;
    sray.z=-sin(yaw)*sray.x+cos(yaw)*sray.z;
    sray.x=tmp;
    
    float yangle=atan2_local(sray.z,sray.x);
    float xz=sqrt(sray.x*sray.x+sray.z*sray.z);
    float vangle=atan2_local(sray.y,xz); //vertical angle (top is 0 degree)

    if (vangle>fovomni/2) sray.w=0; //trick to return alpha

    // remap vangle de [0..fovomni] -> [-0.5..0.5]
    float radius=0.5/(fovomni/2)*vangle;
    float rad=yangle+convergence;
    sray.x=cos(rad)*radius;
    sray.y=sin(rad)*radius;
    sray.x+=0.5;
    sray.y+=0.5;

    return sray;
}

void main(void) {
    float pi=3.141592653589793238462643383279;

    //these variables should be uniforms instead
    bool yuv=false; //true for yuv to rgb conversion in shader
    int eye=0;

    bool camview=true; //true for a camera view that can pan across the omnistereo view
    float fovomni=210.0*pi/180.0; //field of view covered by the luts, this angle is chosen when the luts are computed
    float fovcam=90.0*pi/180.0; //horizontal field of view of the camera view, 90 should be used for the OculusRift
    float pitch=pi/2-pitchparam*pi; //pitch angle (i.e. up and down) of the camera view
    float yaw=yawparam*2.0*pi; //yaw angle (i.e. 360 orientation) of the camera view
    float convergence=-1.5*pi/180.0; //angle to adjust stereo convergence
    float tex_x_offset=0.2; //to adjust fusion on the OculusRift (this number was adjusted experimentally)
    //////////////////////////

    float alpha=1.0;

    vec4 sray;
    vec4 sray_left;
    vec4 sray_right;
    //sray = vec4( gl_TexCoord[0].x, gl_TexCoord[0].y, 1.0, 1.0 );    

    //pos: 0..0.5 = left eye, 0.5..1 = right eye
    vec2 pos = gl_TexCoord[0].xy;
    if( pos.x < 0.5 )
    {
      eye=0;
      sray = vec4( pos.x*2, pos.y, 1.0, 1.0 );    
    }
    else
    {
      eye=1;
      sray = vec4( (pos.x-0.5)*2, pos.y, 1.0, 1.0 );    
    }

    if (camview)
    {
      sray.x-=0.5;
      sray.y-=0.5;
      sray.x*=2.0;
      sray.y*=2.0;
      //sray now sample the image in [-1,1]

      sray_left=map2fisheye(sray,fovomni,fovcam,pitch,yaw,0,tex_x_offset,1);
      alpha=sray_left.w;
    }
    else
    {
      sray_left=sray;
    }

    vec3 l0=texture2D(lutleft0, sray_left.xy).rgb;
    vec3 l1=texture2D(lutleft1, sray_left.xy).rgb;
    vec3 l2=texture2D(lutleft2, sray_left.xy).rgb;
    vec3 r0,r1,r2;
    if (camview)
    {
      sray_right=map2fisheye(sray,fovomni,fovcam,pitch,yaw,convergence,tex_x_offset,-1);
    }
    else
    {
      //adjust disparity of right eye
      float rad=convergence;
      sray.x-=0.5;
      sray.y-=0.5;
      float tmp;
      tmp=cos(rad)*sray.x-sin(rad)*sray.y;
      sray.y=sin(rad)*sray.x+cos(rad)*sray.y;
      sray.x=tmp;
      sray.x+=0.5;
      sray.y+=0.5; 
      sray_right=sray;
    }
    r0=texture2D(lutright0, sray_right.xy).rgb;
    r1=texture2D(lutright1, sray_right.xy).rgb;
    r2=texture2D(lutright2, sray_right.xy).rgb;
    vec3 sl0,sl1,sl2,sr0,sr1,sr2;
    if (yuv)
    {
      sl0=yuv2rgb(tex1,l0.xy);
      sl1=yuv2rgb(tex2,l1.xy);
      sl2=yuv2rgb(tex3,l2.xy);
      sr0=yuv2rgb(tex1,r0.xy);
      sr1=yuv2rgb(tex2,r1.xy);
      sr2=yuv2rgb(tex3,r2.xy);  
    }
    else
    {
      sl0=texture2D(tex1, l0.xy).rgb;
      sl1=texture2D(tex2, l1.xy).rgb;
      sl2=texture2D(tex3, l2.xy).rgb;
      sr0=texture2D(tex1, r0.xy).rgb;
      sr1=texture2D(tex2, r1.xy).rgb;
      sr2=texture2D(tex3, r2.xy).rgb; 
    }

    vec3 ssum = vec3( 0, 0, 0 );
    if( eye==0 ) // left eye
    {
      ssum.rgb+=sl0.rgb*l0.b;
      ssum.rgb+=sl1.rgb*l1.b;
      ssum.rgb+=sl2.rgb*l2.b;
    }
    else if( eye==1 ) // right eye
    {
      ssum.rgb+=sr0.rgb*r0.b;
      ssum.rgb+=sr1.rgb*r1.b;
      ssum.rgb+=sr2.rgb*r2.b;
    }
    else // stereo red-cyan
    {
      ssum.r+=sl0.r*l0.b;
      ssum.r+=sl1.r*l1.b;
      ssum.r+=sl2.r*l2.b;
      ssum.gb+=sr0.gb*r0.b;
      ssum.gb+=sr1.gb*r1.b;
      ssum.gb+=sr2.gb*r2.b;
    }

    ssum.r=pow(ssum.r,0.7);
    ssum.g=pow(ssum.g,0.7);
    ssum.b=pow(ssum.b,0.7);

    gl_FragColor.rgb = ssum;
    gl_FragColor.a = alpha;
}

