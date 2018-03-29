
#version 150 compatibility

uniform mat3 deform;

void main(void)
{
/*
 mat4 m = mat4(
1.07374, -0.666007, 0.102567, 0.0,
0.664371, 1.79577, -0.810483, 0.0,
-0.472707, -0.0935253, 1.10793, 0.0,
0.0, 0.0, 0.0, 1.0);
*/

/*
mat4 mi = mat4(0.8072755239202892, 0.3072107941196511, 0.14999981712612245, 0.0, 
-0.14888408562379088, 0.5222620631921763, 0.3958328755177312, 0.0, 
0.3318624482707368, 0.1751603521834278, 0.9999967073613825, 0.0, 0.0, 0.0, 
0.0, 1.0);
*/

/*
mat3 mi = mat3(0.8072755239202892, 0.3072107941196511, 0.14999981712612245,  
-0.14888408562379088, 0.5222620631921763, 0.3958328755177312, 
0.3318624482707368, 0.1751603521834278, 0.9999967073613825 );
*/

mat3 ideform=inverse(deform);
//mat3 ideform=deform;

//ideform=mat3(1.0,-0.25,0.0,0.0,0.5,0.0,0.0,-0.5,1.0);
//ideform=mat3(2.0,-0.25,0.0,  -0.5,2.0,0.0,   0.0,0.0,1.0);

// column major... attention...
// access is mii[row][col]
mat4 mii;
mii[0][0]=ideform[0][0];
mii[0][1]=ideform[1][0];
mii[0][2]=0.0;
mii[0][3]=ideform[2][0];

mii[1][0]=ideform[0][1];
mii[1][1]=ideform[1][1];
mii[1][2]=0.0;
mii[1][3]=ideform[2][1];

mii[2][0]=0.0;
mii[2][1]=0.0;
mii[2][2]=1.0;
mii[2][3]=0.0;

mii[3][0]=ideform[0][2];
mii[3][1]=ideform[1][2];
mii[3][2]=1.0;
mii[3][3]=ideform[2][2];


/*
mii=mat4(
	1.0, 0.0, 0.0, 0.0,
	-0.25, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0);
*/
// access is mii[row][col]
//mii[1][3]=-0.5;

//mii=inverse(mii);

 gl_TexCoord[0] = gl_MultiTexCoord0;
 gl_Position =  gl_ModelViewProjectionMatrix * mii * gl_Vertex; 
}

