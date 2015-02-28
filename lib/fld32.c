/*
 * 32bit functions implementing the field GF(2^255-19).
 *
 * This code is public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>.
 */

#include <stdint.h>

#include "fld.h"

#ifdef USE_64BIT
#error "This module must not be comiled with -DUSE_64BIT"
#else


/*
 * exported field constants
 */

/* con_d = - 121665/121666 (mod q) */
const fld_t con_d = { 56195235, 13857412, 51736253, 6949390, 114729, 24766616,
		      60832955, 30306712, 48412415, 21499315 };

/* con_2d = 2 * con_d (mod q) */
const fld_t con_2d = { 45281625, 27714825, 36363642, 13898781, 229458,
		       15978800, 54557047, 27058993, 29715967, 9444199 };
/* con_m2d = -2 * con_d (mod q) */
const fld_t con_m2d = { 21827220, 5839606, 30745221, 19655650, 66879405,
			17575631, 12551816, 6495438, 37392896, 24110232 };
/* j^2 = 1 (mod q) */
const fld_t con_j = { 34513072, 25610706, 9377949, 3500415, 12389472, 33281959,
		      41962654, 31548777, 326685, 11406482 };


/*
 * macro for doing one carry-reduce round
 *
 * tmp is being used as carry-register and could be of type
 * limb_t or llimb_t.
 *
 * off is normally zero, but could be used to wrap-around values x
 * with q <= x < 2^255 (see fld_reduce for an example).
 */
#define CARRY(dst, src, tmp, off)	      				\
do {									\
	int _ii;							\
	(tmp) = (off) << FLD_LIMB_BITS(0);				\
	for (_ii = 0; _ii < FLD_LIMB_NUM; _ii += 2) {			\
		(tmp) = ((tmp) >> FLD_LIMB_BITS(0)) + (src)[_ii];	\
		(dst)[_ii] = (tmp) & FLD_LIMB_MASK(1);			\
		(tmp) = ((tmp) >> FLD_LIMB_BITS(1)) + (src)[_ii+1];	\
		(dst)[_ii+1] = (tmp) & FLD_LIMB_MASK(0);		\
	}								\
        (dst)[0] += 19*((tmp) >> FLD_LIMB_BITS(0));			\
} while(0)


/*
 * fld_reduce - returns the smallest non-negative representation of x modulo q
 * 		with 0 <= x[i] <= 2^26 - 1 for i % 2 == 0 
 * 		and  0 <= x[i] <= 2^25 - 1 for i % 2 == 1.
 *
 * assumes:
 *   abs(x[i]) <= 2^31 - 2^5 = 32 * (2^26 - 1)
 */
void
fld_reduce(fld_t res, const fld_t x)
{
	limb_t tmp;

	CARRY(res, x, tmp, 19);
	/* we have
	 *   -2^26 - 19*2^5 <= res[0] <= 2^26 - 1 + 19*(2^5 - 1)
	 * and for i >= 1:
	 *   0 <= res[i] <= 2^26-1, i % 2 == 0,
	 *   0 <= res[i] <= 2^25-1, i % 2 == 1.
	 */

	CARRY(res, res, tmp, 0);
	/* now
	 *   -2^26 - 19 <= res[0] <= 2^26 - 1 + 19
	 * holds and, as before,
	 *   0 <= res[i] <= 2^26-1, i % 2 == 0
	 *   0 <= res[i] <= 2^25-1, i % 2 == 1,
	 * for i >= 1.
	 */

	/* next round we will first remove our offset resulting in
	 *   -2^26 - 38 <= res[0] <= 2^26 - 1,
	 * therefor only a negative carry could appear.
	 */
	CARRY(res, res, tmp, -19);
	/* now we have
	 *   0 <= res[i] <= 2^26-1, i % 2 == 0
	 *   0 <= res[i] <= 2^25-1, i % 2 == 1
	 * for all limbs as wished.
	 *
	 * if a carry had happend, we even know
	 *   2^26 - 38 - 19 <= res[0] <= 2^26 - 1.
	 */
}

/*
 * fld_import - import an 256bit, unsigned, little-endian integer into
 * our internal fld_t format.
 */
void
fld_import(fld_t dst, const uint8_t src[32])
{
	int i;
	uint32_t foo = 0;
	int fill = 0;
	int d = 1;

	for (i = 0; i < FLD_LIMB_NUM; i++) {
		for (; fill < FLD_LIMB_BITS(d); fill += 8)
			foo |= (uint32_t)*src++ << fill;
		dst[i] = foo & FLD_LIMB_MASK(d);
		
		foo >>= FLD_LIMB_BITS(d);
		fill -= FLD_LIMB_BITS(d);
		d = 1-d;
	}
	dst[0] += 19*foo;
}

/*
 * fld_export - export a field element into 256bit little-endian encoded form.
 */
void
fld_export(uint8_t dst[32], const fld_t src)
{
	uint32_t foo;
	fld_t tmp;
	int fill, i;

	fld_reduce(tmp, src);

	for (i = 0, fill = 0, foo = 0; i < FLD_LIMB_NUM; i += 2) {
		foo |= (tmp[i] & FLD_LIMB_MASK(1)) << fill;
		for (fill += FLD_LIMB_BITS(1); fill >= 8; fill -= 8, foo >>= 8)
			*dst++ = foo & 0xff;

		foo |= (tmp[i+1] & FLD_LIMB_MASK(0)) << fill;
		for (fill += FLD_LIMB_BITS(0); fill >= 8; fill -= 8, foo >>= 8)
			*dst++ = foo & 0xff;
	}
	*dst++ = foo & 0xff;
}

/*
 * fld_scale - multiply e by scalar s and reduce modulo q.
 */
void
fld_scale(fld_t dst, const fld_t e, limb_t x)
{
	llimb_t tmp;
	int i;

	for (tmp = 0, i = 0; i < FLD_LIMB_NUM; i += 2) {
		tmp = (tmp >> FLD_LIMB_BITS(0)) + (llimb_t) x*e[i];
		dst[i] = tmp & FLD_LIMB_MASK(1);
		tmp = (tmp >> FLD_LIMB_BITS(1)) + ((llimb_t) x*e[i+1]);
		dst[i+1] = tmp & FLD_LIMB_MASK(0);
	}
	dst[0] += 19*(tmp >> FLD_LIMB_BITS(0));
}

/*
 * fld_mul - multiply a with b and reduce modulo q.
 */
void
fld_mul(fld_t dst, const fld_t a, const fld_t b)
{
	llimb_t tmp;
	llimb_t c[10];

	c[0] = (llimb_t)a[0]*b[0];
	c[1] = (llimb_t)a[0]*b[1] + (llimb_t)a[1]*b[0];
	c[2] = (llimb_t)a[0]*b[2] + (llimb_t)2*a[1]*b[1] + (llimb_t)a[2]*b[0];
	c[3] = (llimb_t)a[0]*b[3] + (llimb_t)a[1]*b[2] + (llimb_t)a[2]*b[1]
		+ (llimb_t)a[3]*b[0];
	c[4] = (llimb_t)a[0]*b[4] + (llimb_t)2*a[1]*b[3] + (llimb_t)a[2]*b[2]
		+ (llimb_t)2*a[3]*b[1] + (llimb_t)a[4]*b[0];
	c[5] = (llimb_t)a[0]*b[5] + (llimb_t)a[1]*b[4] + (llimb_t)a[2]*b[3] 
		+ (llimb_t)a[3]*b[2] + (llimb_t)a[4]*b[1] + (llimb_t)a[5]*b[0];
	c[6] = (llimb_t)a[0]*b[6] + (llimb_t)2*a[1]*b[5] + (llimb_t)a[2]*b[4]
		+ (llimb_t)2*a[3]*b[3] + (llimb_t)a[4]*b[2] + (llimb_t)2*a[5]*b[1]
		+ (llimb_t)a[6]*b[0];
	c[7] = (llimb_t)a[0]*b[7] + (llimb_t)a[1]*b[6] + (llimb_t)a[2]*b[5]
		+ (llimb_t)a[3]*b[4] + (llimb_t)a[4]*b[3] + (llimb_t)a[5]*b[2]
		+ (llimb_t)a[6]*b[1] + (llimb_t)a[7]*b[0];
	c[8] = (llimb_t)a[0]*b[8] + (llimb_t)2*a[1]*b[7] + (llimb_t)a[2]*b[6]
		+ (llimb_t)2*a[3]*b[5] + (llimb_t)a[4]*b[4] + (llimb_t)2*a[5]*b[3]
		+ (llimb_t)a[6]*b[2] + (llimb_t)2*a[7]*b[1] + (llimb_t)a[8]*b[0];
	c[9] = (llimb_t)a[0]*b[9] + (llimb_t)a[1]*b[8] + (llimb_t)a[2]*b[7]
		+ (llimb_t)a[3]*b[6] + (llimb_t)a[4]*b[5] + (llimb_t)a[5]*b[4]
		+ (llimb_t)a[6]*b[3] + (llimb_t)a[7]*b[2] + (llimb_t)a[8]*b[1]
		+ (llimb_t)a[9]*b[0];

	c[0] += 19 * ((llimb_t)2*a[1]*b[9] + (llimb_t)a[2]*b[8] + (llimb_t)2*a[3]*b[7]
		      + (llimb_t)a[4]*b[6] + (llimb_t)2*a[5]*b[5] + (llimb_t)a[6]*b[4]
		      + (llimb_t)2*a[7]*b[3] + (llimb_t)a[8]*b[2] + (llimb_t)2*a[9]*b[1]);
	c[1] += 19 * ((llimb_t)a[2]*b[9] + (llimb_t)a[3]*b[8] + (llimb_t)a[4]*b[7]
		      + (llimb_t)a[5]*b[6] + (llimb_t)a[6]*b[5] + (llimb_t)a[7]*b[4]
		      + (llimb_t)a[8]*b[3] + (llimb_t)a[9]*b[2]);
	c[2] += 19 * ((llimb_t)2*a[3]*b[9] + (llimb_t)a[4]*b[8] + (llimb_t)2*a[5]*b[7]
		      + (llimb_t)a[6]*b[6] + (llimb_t)2*a[7]*b[5] + (llimb_t)a[8]*b[4]
		      + (llimb_t)2*a[9]*b[3]);
	c[3] += 19 * ((llimb_t)a[4]*b[9] + (llimb_t)a[5]*b[8] + (llimb_t)a[6]*b[7]
		      + (llimb_t)a[7]*b[6] + (llimb_t)a[8]*b[5] + (llimb_t)a[9]*b[4]);
	c[4] += 19 * ((llimb_t)2*a[5]*b[9] + (llimb_t)a[6]*b[8] + (llimb_t)2*a[7]*b[7]
		      + (llimb_t)a[8]*b[6] + (llimb_t)2*a[9]*b[5]);
	c[5] += 19 * ((llimb_t)a[6]*b[9] + (llimb_t)a[7]*b[8] + (llimb_t)a[8]*b[7]
		      + (llimb_t)a[9]*b[6]);
	c[6] += 19 * ((llimb_t)2*a[7]*b[9] + (llimb_t)a[8]*b[8] + (llimb_t)2*a[9]*b[7]);
	c[7] += 19 * ((llimb_t)a[8]*b[9] + (llimb_t)a[9]*b[8]);
	c[8] += 19 * ((llimb_t)2*a[9]*b[9]);

	CARRY(c, c, tmp, 0);
	CARRY(dst, c, tmp, 0);
}

/*
 * fld_sq - square x and reduce modulo q.
 */
void
fld_sq(fld_t dst, const fld_t a)
{
	llimb_t tmp;
	llimb_t c[10];

	c[0] = (llimb_t)a[0]*a[0];
	c[1] = (llimb_t)2*a[0]*a[1];
	c[2] = 2*((llimb_t)a[0]*a[2] + (llimb_t)a[1]*a[1]);
	c[3] = 2*((llimb_t)a[0]*a[3] + (llimb_t)a[1]*a[2]);
	c[4] = 2*((llimb_t)a[0]*a[4] + (llimb_t)2*a[1]*a[3]) + (llimb_t)a[2]*a[2];
	c[5] = 2*((llimb_t)a[0]*a[5] + (llimb_t)a[1]*a[4] + (llimb_t)a[2]*a[3]);
	c[6] = 2*((llimb_t)a[0]*a[6] + (llimb_t)2*a[1]*a[5] + (llimb_t)a[2]*a[4] + (llimb_t)a[3]*a[3]);
	c[7] = 2*((llimb_t)a[0]*a[7] + (llimb_t)a[1]*a[6] + (llimb_t)a[2]*a[5] + (llimb_t)a[3]*a[4]);
	c[8] = 2*((llimb_t)a[0]*a[8] + (llimb_t)2*a[1]*a[7] + (llimb_t)a[2]*a[6] + (llimb_t)2*a[3]*a[5]) + (llimb_t)a[4]*a[4];
	c[9] = 2*((llimb_t)a[0]*a[9] + (llimb_t)a[1]*a[8] + (llimb_t)a[2]*a[7] + (llimb_t)a[3]*a[6] + (llimb_t)a[4]*a[5]);
	
	c[0] += 19*2*((llimb_t)2*a[1]*a[9] + (llimb_t)a[2]*a[8] + (llimb_t)2*a[3]*a[7] + (llimb_t)a[4]*a[6] + (llimb_t)a[5]*a[5]);
	c[1] += 19*2*((llimb_t)a[2]*a[9] + (llimb_t)a[3]*a[8] + (llimb_t)a[4]*a[7] + (llimb_t)a[5]*a[6]);
	c[2] += 19*(2*((llimb_t)2*a[3]*a[9] + (llimb_t)a[4]*a[8] + (llimb_t)2*a[5]*a[7]) + (llimb_t)a[6]*a[6]);
	c[3] += 19*2*((llimb_t)a[4]*a[9] + (llimb_t)a[5]*a[8] + (llimb_t)a[6]*a[7]);
	c[4] += 19*(2*((llimb_t)2*a[5]*a[9] + (llimb_t)a[6]*a[8] + (llimb_t)a[7]*a[7]));
	c[5] += 19*2*((llimb_t)a[6]*a[9] + (llimb_t)a[7]*a[8]);
	c[6] += 19*((llimb_t)2*2*a[7]*a[9] + (llimb_t)a[8]*a[8]);
	c[7] += 19*2*(llimb_t)a[8]*a[9];
	c[8] += 19*2*(llimb_t)a[9]*a[9];

	CARRY(c, c, tmp, 0);
	CARRY(dst, c, tmp, 0);
}
#endif
