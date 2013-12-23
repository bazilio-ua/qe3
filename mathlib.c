// mathlib.c -- math primitives

#include "cmdlib.h"
#include "mathlib.h"
#include "qedefs.h"

vec3_t vec3_origin = {0.0f,0.0f,0.0f};


float VectorLength(vec3_t v)
{
	int		i;
	float	length;
	
	length = 0.0f;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = (float)sqrt (length);

	return length;
}

qboolean VectorCompare (vec3_t v1, vec3_t v2)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
		if (fabs(v1[i]-v2[i]) > EQUAL_EPSILON)
			return false;
			
	return true;
}

vec_t Q_rint (vec_t in)
{
	return (float)floor (in + 0.5);
}

void VectorMA (vec3_t va, float scale, vec3_t vb, vec3_t vc)
{
	vc[0] = va[0] + scale*vb[0];
	vc[1] = va[1] + scale*vb[1];
	vc[2] = va[2] + scale*vb[2];
}

void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0]-vb[0];
	out[1] = va[1]-vb[1];
	out[2] = va[2]-vb[2];
}

void _VectorAdd (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0]+vb[0];
	out[1] = va[1]+vb[1];
	out[2] = va[2]+vb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

vec_t VectorNormalize (vec3_t v)
{
	int		i;
	float	length;
	
	length = 0.0f;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = (float)sqrt (length);
	if (length == 0)
		return (vec_t)0;
		
	for (i=0 ; i< 3 ; i++)
		v[i] /= length;	

	return length;
}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (vec3_t v, vec_t scale, vec3_t out)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}


void VectorRotate (vec3_t in, vec3_t rotation, vec3_t out)
{
	vec3_t	work, va;
	int		index[3][2];
	int		i;
	double	angle;
	double	c;
	double	s;
	
	VectorCopy(in, va);
	VectorCopy(va, work);
	index[0][0] = 1; index[0][1] = 2;
	index[1][0] = 2; index[1][1] = 0;
	index[2][0] = 0; index[2][1] = 1;
	
	for (i = 0; i < 3; i++)
	{
		if (rotation[i] != 0)
		{
			angle = rotation[i] * Q_PI / 180.0;
			c = cos(angle);
			s = sin(angle);
			work[index[i][0]] = va[index[i][0]] * (vec_t)c - va[index[i][1]] * (vec_t)s;
			work[index[i][1]] = va[index[i][0]] * (vec_t)s + va[index[i][1]] * (vec_t)c;
		}
		VectorCopy(work, va);
	}
	VectorCopy(work, out);
}


void VectorRotate2 (vec3_t in, vec3_t rotation, vec3_t origin, vec3_t out)
{
	vec3_t temp, temp2;
	
	VectorSubtract(in, origin, temp);
	VectorRotate(temp, rotation, temp2);
	VectorAdd(temp2, origin, out);
}


void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}


void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;
	
	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}


void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs
	
	angle = angles[YAW] * ((float)Q_PI*2 / 360.0f);
	sy = (float)sin(angle);
	cy = (float)cos(angle);
	angle = angles[PITCH] * ((float)Q_PI*2 / 360.0f);
	sp = (float)sin(angle);
	cp = (float)cos(angle);
	angle = angles[ROLL] * ((float)Q_PI*2 / 360.0f);
	sr = (float)sin(angle);
	cr = (float)cos(angle);
	
	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = -sr*sp*cy+cr*sy;
		right[1] = -sr*sp*sy-cr*cy;
		right[2] = -sr*cp;
	}
	if (up)
	{
		up[0] = cr*sp*cy+sr*sy;
		up[1] = cr*sp*sy-sr*cy;
		up[2] = cr*cp;
	}
}

void VectorToAngles( vec3_t vec, vec3_t angles )
{
	float forward;
	float yaw, pitch;
	
	if ( ( vec[ 0 ] == 0 ) && ( vec[ 1 ] == 0 ) )
	{
		yaw = 0;
		if ( vec[ 2 ] > 0 )
		{
			pitch = 90;
		}
		else
		{
			pitch = 270;
		}
	}
	else
	{
		yaw = (float)atan2( vec[ 1 ], vec[ 0 ] ) * 180.0f / (float)Q_PI;
		if ( yaw < 0 )
		{
			yaw += 360;
		}
		
		forward = (float)sqrt( vec[ 0 ] * vec[ 0 ] + vec[ 1 ] * vec[ 1 ] );
		pitch = (float)atan2( vec[ 2 ], forward ) * 180.0f / (float)Q_PI;
		if ( pitch < 0 )
		{
			pitch += 360;
		}
	}
	
	angles[ 0 ] = pitch;
	angles[ 1 ] = yaw;
	angles[ 2 ] = 0;
}
