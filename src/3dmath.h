#ifndef _3D_MATH_H_
#define _3D_MATH_H_

#ifdef USE_SIMD
#	include <xmmintrin.h>
#endif

typedef float Matrix[4][4];

static inline void
matrix_identity (Matrix m)
{
	m[0][0] = 1; m[0][1] = 0; m[0][2] = 0; m[0][3] = 0;
	m[1][0] = 0; m[1][1] = 1; m[1][2] = 0; m[1][3] = 0;	
	m[2][0] = 0; m[2][1] = 0; m[2][2] = 1; m[2][3] = 0;
	m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;
}
static inline void
matrix_multiply (Matrix m, Matrix a, Matrix b)
{
#if USE_SIMD /*New fun SIMD code. It would be cool to rework everything to be
	16 byte aligned, but it will have to be for another day*/
	__m128 r0 = _mm_load_ps(b[0]);
	__m128 r1 = _mm_load_ps(b[1]);
	__m128 r2 = _mm_load_ps(b[2]);
	__m128 r3 = _mm_load_ps(b[3]);
	for (int i = 0; i < 4; i++)
	{
		__m128 c0 = _mm_set1_ps(a[i][0]);
		__m128 c1 = _mm_set1_ps(a[i][1]);
		__m128 c2 = _mm_set1_ps(a[i][2]);
		__m128 c3 = _mm_set1_ps(a[i][3]);
		__m128 t0 = _mm_add_ps(_mm_mul_ps(r0, c0), _mm_mul_ps(r1, c1));
		__m128 t1 = _mm_add_ps(_mm_mul_ps(r2, c2), _mm_mul_ps(r3, c3));
		_mm_store_ps(m[i], _mm_add_ps(t0, t1));
	}
#else /*Original implementation*/
	m[0][0] = a[0][0]*b[0][0] + a[0][1]*b[1][0] + a[0][2]*b[2][0] + a[0][3]*b[3][0];
	m[0][1] = a[0][0]*b[0][1] + a[0][1]*b[1][1] + a[0][2]*b[2][1] + a[0][3]*b[3][1];
	m[0][2] = a[0][0]*b[0][2] + a[0][1]*b[1][2] + a[0][2]*b[2][2] + a[0][3]*b[3][2];
	m[0][3] = a[0][0]*b[0][3] + a[0][1]*b[1][3] + a[0][2]*b[2][3] + a[0][3]*b[3][3];
	
	m[1][0] = a[1][0]*b[0][0] + a[1][1]*b[1][0] + a[1][2]*b[2][0] + a[1][3]*b[3][0];
	m[1][1] = a[1][0]*b[0][1] + a[1][1]*b[1][1] + a[1][2]*b[2][1] + a[1][3]*b[3][1];
	m[1][2] = a[1][0]*b[0][2] + a[1][1]*b[1][2] + a[1][2]*b[2][2] + a[1][3]*b[3][2];
	m[1][3] = a[1][0]*b[0][3] + a[1][1]*b[1][3] + a[1][2]*b[2][3] + a[1][3]*b[3][3];
	
	m[2][0] = a[2][0]*b[0][0] + a[2][1]*b[1][0] + a[2][2]*b[2][0] + a[2][3]*b[3][0];
	m[2][1] = a[2][0]*b[0][1] + a[2][1]*b[1][1] + a[2][2]*b[2][1] + a[2][3]*b[3][1];
	m[2][2] = a[2][0]*b[0][2] + a[2][1]*b[1][2] + a[2][2]*b[2][2] + a[2][3]*b[3][2];
	m[2][3] = a[2][0]*b[0][3] + a[2][1]*b[1][3] + a[2][2]*b[2][3] + a[2][3]*b[3][3];
#endif
}
static inline void
matrix_translate (Matrix m, float x, float y, float z)
{
	m[0][3] = x;
	m[1][3] = y;
	m[2][3] = z;
}
static inline void
vector_transform (float *dst, Matrix m, float *v)
{
	dst[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3];
	dst[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3];
	dst[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3];
}
static inline void
vector_smad (float *dst, float *a, float t, float *b)
{
	dst[0] = a[0] + t*(b[0] - a[0]);
	dst[1] = a[1] + t*(b[1] - a[1]);
	dst[2] = a[2] + t*(b[2] - a[2]);
}
static inline void
vector_copy (float *dst, float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}
static inline float
vector_dot3 (float *a, float *b)
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
#endif