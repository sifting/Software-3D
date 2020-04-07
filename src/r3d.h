#ifndef _RAGE_3D_H_
#define _RAGE_3D_H_
#include <assert.h>
#include "3dmath.h"

#define MAX_WIDTH	320
#define MAX_HEIGHT	240

/*Public API*/
enum
{
	DM_TRIANGLES,
	DM_TRIANGLE_STRIP,
	DM_TRIANGLE_FAN,
	MAX_DM
};
void r3d_draw (unsigned int mode, unsigned int num);
void r3d_perspective (float fov, float aspect, float znear, float zfar);
void r3d_clear (void);
void r3d_shutdown (void);
int r3d_init (void);

/*Public variables*/
extern Matrix r3d_pmv;

/*Private stuff*/
extern struct _3D_context
{
	float *vp;
	unsigned short *sp;
	struct _Canvas
	{
		unsigned short vp[4];
		unsigned short *zb;
		unsigned char *cb;
	}c;
	struct _Perspective
	{
		float y, w; 
		float ry, x;
		float zn, zf;
	}p;
	/*Colour state*/
	unsigned char *pal;
	unsigned char *shades;
	unsigned char col;
}_c;

static inline void
r3d_vertex_pointer (float *data)
{
	_c.vp = data;
}
static inline void
r3d_shade_pointer (unsigned short *data)
{
	_c.sp = data;
}
static inline void
r3d_viewport
(
	unsigned short x,
	unsigned short y,
	unsigned short w,
	unsigned short h
)
{
	_c.c.vp[0] = x < 0 ? 0 : x;
	_c.c.vp[1] = y < 0 ? 0 : y;
	_c.c.vp[2] = MAX_WIDTH < w ? MAX_WIDTH : w;
	_c.c.vp[3] = MAX_HEIGHT < h ? MAX_HEIGHT : h;
}
static inline void
r3d_depth_buffer (unsigned short *buf)
{
	_c.c.zb = buf;
}
static inline void
r3d_colour_buffer (unsigned char *buf)
{
	_c.c.cb = buf;
}
static inline void
r3d_colour (unsigned char col)
{
	_c.col = col;
}
#endif