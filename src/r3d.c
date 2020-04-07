#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "r3d.h"

#define MAX_POINTS 8
typedef struct _Vertex
{
	float xyz[3];
	unsigned shade;
}Vertex;
typedef struct _Clip_vertex
{
	float rz;
	float xyz[3];
}Clip_vertex;
Matrix r3d_pmv;
struct _3D_context _c;
static unsigned int _ytbl[MAX_HEIGHT];

/*
**Scan converting and painting
*/
static void
drawspan (int y, int x1, int x2)
{
	int xyz = 0;
	while (x1 < x2)
	{
		unsigned int px = _ytbl[y] + (x1<<2);
		unsigned int co = (_c.col<<1) + _c.col;
		_c.c.cb[px + 0] = _c.pal[co + 2];
		_c.c.cb[px + 1] = _c.pal[co + 1];
		_c.c.cb[px + 2] = _c.pal[co + 0];
		x1++;
	}
	y = 0;
}
static void
tridraw (float *a, float *b, float *c)
{
	static unsigned short _eb[2][MAX_HEIGHT];
	float *sorted[3];
	float e, f, g, h;
	unsigned w;
	int i, j;
#if 1
	/*Cull reversed triangles*/
	float A = b[0] - a[0];
	float B = b[1] - a[1];
	float C = c[0] - a[0];
	float D = c[1] - a[1];
	if (A*D - B*C < 0)
	{
		return;
	}
#endif
	/*Sort points by most vertical Y (lowest) coordinate...
	cmov instructions would be great here*/
	if (a[1] < b[1])
	{
		if (a[1] < c[1])
		{
			sorted[0] = a;
			if (b[1] < c[1])
			{
				sorted[1] = b;
				sorted[2] = c;
			}
			else
			{
				sorted[1] = c;
				sorted[2] = b;
			}
		}
		else
		{/*We know a >= b already*/
			sorted[0] = c;
			sorted[1] = a;
			sorted[2] = b;
		}
	}
	else
	{
		if (b[1] < c[1])
		{
			sorted[0] = b;
			if (a[1] < c[1])
			{
				sorted[1] = a;
				sorted[2] = c;
			}
			else
			{
				sorted[1] = c;
				sorted[2] = a;
			}
		}
		else
		{/*We know b >= a already*/
			sorted[0] = c;
			sorted[1] = b;
			sorted[2] = a;
		}
	}
	/*Determine winding so we know how to scan the polygon edges*/
	e = sorted[1][0] - sorted[0][0];
	f = sorted[1][1] - sorted[0][1];
	g = sorted[2][0] - sorted[0][0];
	h = sorted[2][1] - sorted[0][1];
	w = e*h - f*g < 0;
	{/*Scan convert the lead edge*/
		float dy = sorted[2][1] - sorted[0][1];
		if (fabs (dy) < 1e-2)
		{/*TRIVIAL REJECTION: Triangle has no height*/
			return;
		}
		float m = (sorted[2][0] - sorted[0][0])/dy;
		float y = floor (sorted[0][1] + 1);
		float e = m*(y - sorted[0][1]);
		float x = sorted[0][0] + e;
		unsigned short *eb = _eb[w];
		i = (int)sorted[0][1];
		j = (int)sorted[2][1];
		while (i < j)
		{
			eb[i] = (unsigned short)x;
			x = x + m;
			i++;
		}
		i = 0;
	}
	/*Remaining edges will be on the opposite side of the lead*/
	w ^= 1;
	{
		float dy = sorted[1][1] - sorted[0][1];
		if (fabs (dy) < 1e-2)
		{
			goto Bottom;
		}
		float m = (sorted[1][0] - sorted[0][0])/dy;
		float y = floor (sorted[0][1] + 1);
		float e = m*(y - sorted[0][1]);
		float x = sorted[0][0] + e;
		unsigned short *eb = _eb[w];
		i = (int)sorted[0][1];
		j = (int)sorted[1][1];
		while (i < j)
		{
			eb[i] = (unsigned short)x;
			x = x + m;
			i++;
		}
		i = 0;
	}
Bottom:
	{
		float dy = sorted[2][1] - sorted[1][1];
		if (fabs (dy) < 1e-2)
		{
			goto Fill;
		}
		float m = (sorted[2][0] - sorted[1][0])/dy;
		float y = floor (sorted[1][1] + 1);
		float e = m*(y - sorted[1][1]);
		float x = sorted[1][0] + e;
		unsigned short *eb = _eb[w];
		i = (int)sorted[1][1];
		j = (int)sorted[2][1];
		while (i < j)
		{
			eb[i] = (unsigned short)x;
			x = x + m;
			i++;
		}
		i = 0;
	}
Fill:
	i = (int)sorted[0][1];
	j = (int)sorted[2][1];
	while (i < j)
	{
		int x1 = _eb[0][i];
		int x2 = _eb[1][i];
		drawspan (i, x1, x2);
		i++;
	}
}
/*
**Clipping
*/
static inline void
clip
(
	Vertex dst[], 
	int *n, 
	Vertex pts[], 
	int npts, 
	short *sides, 
	float *dots,
	float epsilon
)
{
	int ndst = 0;
	int i, j;
	for (i = 0, j = 1; j < npts; i++, j++)
	{
		Vertex *p1 = &pts[i];
		Vertex *p2 = &pts[j];
		/*Save this point if it's in or on the plane*/
		if (sides[i] != -1)
		{
			dst[ndst] = *p1;
			ndst++;
		}
		if (sides[j] != 0 && sides[i] == sides[j])
		{/*No split necessary since this edge is entirely on one side*/
			continue;
		}
		/*Edge crosses plane, so add a split point*/
		{
			float t = (dots[i] + epsilon)/(dots[i] - dots[j]);
			const float *xyz1 = p1->xyz;
			const float *xyz2 = p2->xyz;
			dst[ndst].xyz[0] = xyz1[0] + t*(xyz2[0] - xyz1[0]);
			dst[ndst].xyz[1] = xyz1[1] + t*(xyz2[1] - xyz1[1]);
			dst[ndst].xyz[2] = xyz1[2] + t*(xyz2[2] - xyz1[2]);
			{/*Recompute shade*/
				unsigned short s1 = p1->shade;
				unsigned short s2 = p2->shade;
				dst[ndst].shade = s1 + (unsigned short)(t*(s2 - s1));
			}
			ndst++;
		}
	}
	*n = ndst;
}
static void
triclip (unsigned int a, unsigned int b, unsigned int c)
{
	Vertex pt1[MAX_POINTS];
	unsigned int index;
	int npt1 = 3;
	/*Transform points by pmv
	NB: (x<<1) + x = 2x + x = 3x
	*/
	vector_transform (pt1[0].xyz, r3d_pmv, &_c.vp[(a<<1) + a]);
	pt1[0].shade = _c.sp[a];
	vector_transform (pt1[1].xyz, r3d_pmv, &_c.vp[(b<<1) + b]);
	pt1[1].shade = _c.sp[b];
	vector_transform (pt1[2].xyz, r3d_pmv, &_c.vp[(c<<1) + c]);
	pt1[2].shade = _c.sp[c];
	if (1) {/*Clip to frustum*/
		const float epsilon = 1e-3;
		const float fudge = 1.005; /*Expand frustum for culling a little*/
		Vertex pt2[MAX_POINTS];
		float dots[MAX_POINTS];
		float y = _c.p.y;
		float x = _c.p.w;
		short sides[MAX_POINTS];
		int npt2;
		int i;
		/*Near*/
		for (i = 0; i < npt1; i++)
		{
			float d = -pt1[i].xyz[2];
			dots[i] = d - _c.p.zn;
			sides[i] = (dots[i] > epsilon) - (dots[i] < -epsilon);
		}
		sides[npt1] = sides[0]; dots[npt1] = dots[0];
		vector_copy (pt1[npt1++].xyz, pt1[0].xyz);
		clip (pt2, &npt2, pt1, npt1, sides, dots, 0);
		/*Far*/
		for (i = 0; i < npt2; i++)
		{
			float d = -pt2[i].xyz[2];
			dots[i] =-d + _c.p.zf;
			sides[i] = (dots[i] > epsilon) - (dots[i] < -epsilon);
		}
		sides[npt2] = sides[0]; dots[npt2] = dots[0];
		vector_copy (pt2[npt2++].xyz, pt2[0].xyz);
		clip (pt1, &npt1, pt2, npt2, sides, dots, 0);
		/*Top*/
		for (i = 0; i < npt1; i++)
		{
			float h = -pt1[i].xyz[2]*y*fudge;
			float d = pt1[i].xyz[1];
			dots[i] =-d + h;
			sides[i] = (dots[i] > epsilon) - (dots[i] < -epsilon);
		}
		sides[npt1] = sides[0]; dots[npt1] = dots[0];
		vector_copy (pt1[npt1++].xyz, pt1[0].xyz);
		clip (pt2, &npt2, pt1, npt1, sides, dots, 0);
		/*Bottom*/
		for (i = 0; i < npt2; i++)
		{
			float h = -pt2[i].xyz[2]*y*fudge;
			float d = pt2[i].xyz[1];
			dots[i] = d + h;
			sides[i] = (dots[i] > epsilon) - (dots[i] < -epsilon);
		}
		sides[npt2] = sides[0]; dots[npt2] = dots[0];
		vector_copy (pt2[npt2++].xyz, pt2[0].xyz);
		clip (pt1, &npt1, pt2, npt2, sides, dots, 0);
		/*Left*/
		for (i = 0; i < npt1; i++)
		{
			float w = -pt1[i].xyz[2]*x*fudge;
			float d = pt1[i].xyz[0];
			dots[i] =-d + w;
			sides[i] = (dots[i] > epsilon) - (dots[i] < -epsilon);
		}
		sides[npt1] = sides[0]; dots[npt1] = dots[0];
		vector_copy (pt1[npt1++].xyz, pt1[0].xyz);
		clip (pt2, &npt2, pt1, npt1, sides, dots, 0);
		/*Right*/
		for (i = 0; i < npt2; i++)
		{
			float w = -pt2[i].xyz[2]*x*fudge;
			float d = pt2[i].xyz[0];
			dots[i] = d + w;
			sides[i] = (dots[i] > epsilon) - (dots[i] < -epsilon);
		}
		sides[npt2] = sides[0]; dots[npt2] = dots[0];
		vector_copy (pt2[npt2++].xyz, pt2[0].xyz);
		clip (pt1, &npt1, pt2, npt2, sides, dots, 0);
	}
	/*Don't draw anything degenerate*/
	if (npt1 < 3)
	{
		return;
	}
	{/*The result of the triangle after clipping is a triangle fan.
	Transform each triangle in the fan to screen coords and draw it.*/
		Clip_vertex cv[MAX_POINTS];
		Clip_vertex *o = &cv[0];
		float y = _c.p.ry;
		float x = _c.p.x;
		float ox = (_c.c.vp[0]) + 0.5;
		float oy = (_c.c.vp[1]) + 0.5;
		float w = _c.c.vp[2]>>1;
		float h = _c.c.vp[3]>>1;
		int i;
		cv[0].rz = 1.0/(pt1[0].xyz[2] - 1);
		cv[0].xyz[0] = w*(1 + x*pt1[0].xyz[0]*cv[0].rz) + ox;
		cv[0].xyz[1] = h*(1 + y*pt1[0].xyz[1]*cv[0].rz) + oy;
		cv[0].xyz[2] = pt1[0].xyz[2];
		
		cv[1].rz = 1.0/(pt1[1].xyz[2] - 1);
		cv[1].xyz[0] = w*(1 + x*pt1[1].xyz[0]*cv[1].rz) + ox;
		cv[1].xyz[1] = h*(1 + y*pt1[1].xyz[1]*cv[1].rz) + oy;
		cv[1].xyz[2] = pt1[1].xyz[2];
		for (i = 2; i < npt1; i++)
		{
			Clip_vertex *p = &cv[i-1];
			Clip_vertex *q = &cv[i];
			Vertex *v = &pt1[i];
			q->rz = 1.0/(v->xyz[2] - 1);
			q->xyz[0] = w*(1 + x*v->xyz[0]*q->rz) + ox;
			q->xyz[1] = h*(1 + y*v->xyz[1]*q->rz) + oy;
			q->xyz[2] = v->xyz[2];
			tridraw (o->xyz, p->xyz, q->xyz);
		}
	}
}
/*
**Public API
*/
void
r3d_draw (unsigned int mode, unsigned int num)
{
	unsigned int i;
	switch (mode)
	{
	case DM_TRIANGLES:
		for (i = 2; i < num; i+=3)
		{
			triclip (i-2, i-1, i-0);
		}
		break;
	case DM_TRIANGLE_STRIP: /*Each new vertex shares previous two*/
		if (num <= 3)
		{/*Handle single triangle edge case*/
			if (3 == num)
			{
				triclip (i-2, i-1, i-0);
			}
			break;
		}
		/*Perserve winding order*/
		for (i = 3; i < num; i+=2)
		{
			triclip (i-3, i-2, i-1);
			triclip (i-2, i-0, i-1);
		}
		if (i != num)
		{
			triclip (i-3, i-2, i-1);
		}
		break;
	case DM_TRIANGLE_FAN: /*Shared central point*/
		if (num < 3)
		{
			break;
		}
		triclip (0, 1, 2);
		for (i = 3; i < num; i+=2)
		{
			triclip (0, i-1, i-0);
		}
		break;
	default:
		break;
	}
}
void
r3d_clear (void)
{
	int i;
	for (i = 0; i < _c.c.vp[3]; i++)
	{
		unsigned int px = _ytbl[_c.c.vp[1] + i] + (_c.c.vp[0]<<2);
		memset (&_c.c.cb[px], 0x50, _c.c.vp[2]<<2);
	}
}
void
r3d_perspective (float fov, float aspect, float znear, float zfar)
{
	_c.p.y = tan ((fov*3.14159265)/360.0);
	_c.p.ry = 1.0/_c.p.y;
	_c.p.w = _c.p.y*aspect;
	_c.p.x = _c.p.ry/aspect;
	_c.p.zn = znear;
	_c.p.zf = zfar;
}
void
r3d_shutdown (void)
{
	if (_c.pal)
	{
		free (_c.pal);
		_c.pal = NULL;
	}
}
int
r3d_init (void)
{
	matrix_identity (r3d_pmv);
	memset (&_c, 0, sizeof (_c));
	_c.col = 128;
	{/*Load palette and shade tables*/
		FILE *fp = fopen ("pal.dat", "rb");
		size_t len;
		if (NULL == fp)
		{
			return -1;
		}
		/*Load whole file from disk*/
		fseek (fp, 0, SEEK_END);
		len = ftell (fp);
		fseek (fp, 0, SEEK_SET);
		_c.pal = malloc (len);
		if (NULL == _c.pal)
		{
			return -2;
		}
		fread (_c.pal, 1, len, fp);
		fclose (fp);
		/*Set shade table pointer*/
		_c.shades = _c.pal + 256*3;
	}
	{/*Init y lut*/
		int i;
		for (i = 0; i < MAX_HEIGHT; i++)
		{
			_ytbl[i] = i*MAX_WIDTH*4;
		}
		i = 0;
	}
	return 0;
}
