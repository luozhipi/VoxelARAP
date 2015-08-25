#include <math.h>
#include "trackball.h"
#include <iostream>
#include "extract_rotation_pseudo_edge.h"

void
vzero2(float *v)
{
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 0.0;
}
void
vset2(float *v, float x, float y, float z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}
void
vsub2(const float *src1, const float *src2, float *dst)
{
    dst[0] = src1[0] - src2[0];
    dst[1] = src1[1] - src2[1];
    dst[2] = src1[2] - src2[2];
}

void
vcopy2(const float *v1, float *v2)
{
    register int i;
    for (i = 0 ; i < 3 ; i++)
        v2[i] = v1[i];
}
float
vdot2(const float *v1, const float *v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}
void
vcross2(const float *v1, const float *v2, float *cross)
{
    float temp[3];

    temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
    temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
    temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
    vcopy2(temp, cross);
}
void
vscale2(float *v, float div)
{
    v[0] *= div;
    v[1] *= div;
    v[2] *= div;
}
float
vlength2(const float *v)
{
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}
void
vnormal2(float *v)
{
    vscale2(v,1.0f/vlength2(v));
}
/*shortest arc
q.w == cos(angle / 2)
q.x == sin(angle / 2) * cross.x
q.y == sin(angle / 2) * cross.y
q.z == sin(angle / 2) * cross.z

dot and cross product of two normalized vectors are:
dot     == cos(angle)
cross.x == sin(angle) * Unitperpendicular.x
cross.y == sin(angle) * Unitperpendicular.y
cross.z == sin(angle) * Unitperpendicular.z
*/
void extract_rotation_pseudo_edge(float q[4], float p1x, float p1y, float p1z, float p2x, float p2y, float p2z)
{
    float a[3]; /* Axis of rotation */
    float p1[3], p2[3];

    vset2(p1,p1x,p1y,p1z);
    vset2(p2,p2x,p2y,p2z);

    /*
     *  Figure out how much to rotate around that axis.
     */
	float d = vdot2(p1,p2);
	if(d>=1.0)
	{
		q[3] = 1.0f;
		q[0] = 0.0;
		q[1] = 0.0;
		q[2]=0.0;
		return;
	}
	if (d < (1e-6f - 1.0f))
	{
		// Generate an axis
		float v1[3] = {1,0,0};float tmp[3];
		vcross2(v1,p1,tmp);
		if (vlength2(tmp)==0) // pick another if colinear
		{
			v1[0]=0;v1[1]=1;vcross2(v1,p1,tmp);
		}
		vnormal2(tmp);
		//Half-Way Vector Solution
		axis_to_quat(tmp,180,q);
		return;
	}
	//Half-Way Quaternion Solution
	vcross2(p1,p2,a);
    //float s = sqrt( (1+d)*2 );
	//float invs = 1 / s;
    q[0] = a[0];// * invs;
    q[1] = a[1];//* invs;
    q[2] = a[2]; //* invs;
    q[3] = 1+d;//s * 0.5f;
}