
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
uniform int eye;
uniform sampler2D homog;





//float fovomni;


// homog : LUT for homography mapping
// eye: 0=left, 1=right, 2=stereo red-cyan

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

void main(void) {
    float pi=3.141592653589793238462643383279;

    //these variables should be uniforms instead
    bool yuv=false; //true for yuv to rgb conversion in shader

    // all false : red-cyan stereo
    //bool lefteye=true; // left eye only
    //bool righteye=false; // right eye only
    //bool monoscopic=false; //true for monocular view only

    bool camview=false; //true for a camera view that can pan across the omnistereo view
    float fovomni=172.0*pi/180.0; //field of view covered by the luts, this angle is chosen when the luts are computed
    float fovcam=65.0*pi/180.0; //horizontal field of view of the camera view
    float pitch=(40.0-18.0*pitchparam)*pi/180.0; //pitch angle (i.e. up and down) of the camera view
    float yaw=-yawparam*2.0*pi; //yaw angle (i.e. 360 orientation) of the camera view
    float convergence=0.0*pi/180.0; //angle to adjust stereo convergence
    //////////////////////////

    float radius,rad;

    float alpha=1.0;

    // homographie
    vec2 pos = gl_TexCoord[0].xy;
    vec3 h = texture2D(homog,pos).rgb;
    vec3 sray = vec3( h.x, h.y, 1.0 );    

    // pas homographie
    //vec3 sray = vec3( gl_TexCoord[0].x, gl_TexCoord[0].y, 1.0 );    


    // coin (0,0) -> en haut a gauche
    //sray.xy=1.0-sray.xy;
    if (camview)
    {
      sray.x-=0.5;
      sray.y-=0.5;
      sray.x*=2.0;
      sray.y*=2.0;
      //sray now sample the image in [-1,1]

      // test de visibilite verticale
      //  if (sray.y>1.0*9.0/16.0) alpha=0.0;
      //  if (sray.y<-1.0*9.0/16.0) alpha=0.0;

      float ssize=tan(fovcam/2.0)*sray.z;
      sray.x*=ssize;
      sray.y*=ssize/1.65714;
      sray.x=-sray.x;

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
      float vangle=atan2_local(xz,sray.y);

      // remap vangle de [0..fovomni] -> [-0.5..0.5]
      radius=0.5/(fovomni/2.0)*(fovomni/2.0+vangle);
      rad=yangle;
      sray.x=cos(rad)*radius;
      sray.y=sin(rad)*radius;
      sray.x+=0.5;
      sray.y+=0.5;
    }

    vec3 l0=texture2D(lutleft0, sray.xy).rgb;
    vec3 l1=texture2D(lutleft1, sray.xy).rgb;
    vec3 l2=texture2D(lutleft2, sray.xy).rgb;
    vec3 r0,r1,r2;
    if (camview)
    {
      rad+=pi+convergence;
      sray.x=cos(rad)*radius;
      sray.y=sin(rad)*radius;
      sray.x+=0.5;
      sray.y+=0.5;
    }
    else
    {
      //adjust disparity of right eye
      rad=pi+convergence;
      sray.x-=0.5;
      sray.y-=0.5;
      float tmp;
      tmp=cos(rad)*sray.x-sin(rad)*sray.y;
      sray.y=sin(rad)*sray.x+cos(rad)*sray.y;
      sray.x=tmp;
      sray.x+=0.5;
      sray.y+=0.5;
    }
    r0=texture2D(lutright0, sray.xy).rgb;
    r1=texture2D(lutright1, sray.xy).rgb;
    r2=texture2D(lutright2, sray.xy).rgb;

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
    if( eye==0 ) {
	// left eye
      ssum.rgb+=sl0.rgb*l0.b;
      ssum.rgb+=sl1.rgb*l1.b;
      ssum.rgb+=sl2.rgb*l2.b;
    } else if( eye==1 ) {
	// right eye
      ssum.rgb+=sr0.rgb*r0.b;
      ssum.rgb+=sr1.rgb*r1.b;
      ssum.rgb+=sr2.rgb*r2.b;
    } else {
	// stereo red-cyan
      ssum.r+=sl0.r*l0.b;
      ssum.r+=sl1.r*l1.b;
      ssum.r+=sl2.r*l2.b;
      ssum.gb+=sr0.gb*r0.b;
      ssum.gb+=sr1.gb*r1.b;
      ssum.gb+=sr2.gb*r2.b;
    }

    ssum.r=pow(ssum.r,0.6);
    ssum.g=pow(ssum.g,0.6);
    ssum.b=pow(ssum.b,0.6);

    gl_FragColor.rgb = ssum;
    gl_FragColor.a = alpha*fade*h.b;
}

