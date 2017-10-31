/*
 * code for the field GF(2^255-19).
 *
 * This code is in public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>
 */

#include "bitness.h"
#include "fld.h"


#ifdef USE_64BIT

/*
 * 64bit implementation
 */

/*
 * exported field constants
 */

/* con_d = - 121665/121666 (mod q) */
const fld_t con_d = {
	929955233495203, 466365720129213, 1662059464998953, 2033849074728123,
	1442794654840575 };

/* con_2d = 2 * con_d (mod q) */
const fld_t con_2d = {
	1859910466990425, 932731440258426, 1072319116312658, 1815898335770999,
	633789495995903 };

/* con_m2d = -2 * con_d (mod q) */
const fld_t con_m2d = {
	391889346694804, 1319068373426821, 1179480697372589, 435901477914248,
	1618010317689344 };

/* con_j^2 = 1 (mod q) */
const fld_t con_j = {
	1718705420411056, 234908883556509, 2233514472574048, 2117202627021982,
	765476049583133 };



/*
 * fld_reduce - returns the smallest non-negative representation of x modulo q
 * 		with 0 <= x[i] <= 2^51 - 1 for 0 <= i <= 4.
 *
 * This function assumes
 *   abs(x[i]) <= 2^63 - 2^12 = 2^12 * (2^51 - 1), 
 * for 0 <= i <= 4 to work properly.
 */
void
fld_reduce(fld_t res, const fld_t x)
{
	/* first carry round with offset 19 */
	res[0] = x[0] + 19;
	res[1] = x[1] + (res[0] >> FLD_LIMB_BITS);
	res[2] = x[2] + (res[1] >> FLD_LIMB_BITS);
	res[3] = x[3] + (res[2] >> FLD_LIMB_BITS);
	res[4] = x[4] + (res[3] >> FLD_LIMB_BITS);

	/* -2^12 <= (res[4] >> FLD_LIMB_BITS) <= 2^12-1 */
	res[0] = (res[0] & FLD_LIMB_MASK) + 19*(res[4] >> FLD_LIMB_BITS);
	res[1] &= FLD_LIMB_MASK;
	res[2] &= FLD_LIMB_MASK;
	res[3] &= FLD_LIMB_MASK;
	res[4] &= FLD_LIMB_MASK;

	/* now we have
	 *   -19*2^12 <= res[0] <= 2^51-1 + 19*(2^12-1),
	 * and
	 *   0 <= res[i] <= 2^51-1 for 1 <= i <= 4.
	 */

	/* second round */
	res[1] += (res[0] >> FLD_LIMB_BITS);
	res[2] += (res[1] >> FLD_LIMB_BITS);
	res[3] += (res[2] >> FLD_LIMB_BITS);
	res[4] += (res[3] >> FLD_LIMB_BITS);

	/* -1 <= (res[4] >> FLD_LIMB_BITS) <= 1 */
	res[0] = (res[0] & FLD_LIMB_MASK) + 19*(res[4] >> FLD_LIMB_BITS);

	res[1] &= FLD_LIMB_MASK;
	res[2] &= FLD_LIMB_MASK;
	res[3] &= FLD_LIMB_MASK;
	res[4] &= FLD_LIMB_MASK;

	/* the second round yields 
	 *   -19 <= res[0] <= 2^51-1 + 19
	 * and
	 *   0 <= res[i] <= 2^51-1 for 1 <= i <= 4.
	 */
	res[0] -= 19;

	/* with the offset removed we have
	 *   -38 <= res[0] <= 2^51-1
	 * and need a third round to assure that res[0] is non-negative.
	 *
	 * please note, that no positive carry is possible at this point.
	 */

	res[1] += (res[0] >> FLD_LIMB_BITS);
	res[2] += (res[1] >> FLD_LIMB_BITS);
	res[3] += (res[2] >> FLD_LIMB_BITS);
	res[4] += (res[3] >> FLD_LIMB_BITS);

	/* -1 <= (res[4] >> FLD_LIMB_BITS) <= 0, because of non-positive carry */
	res[0] = (res[0] & FLD_LIMB_MASK) + 19*(res[4] >> FLD_LIMB_BITS);

	res[1] &= FLD_LIMB_MASK;
	res[2] &= FLD_LIMB_MASK;
	res[3] &= FLD_LIMB_MASK;
	res[4] &= FLD_LIMB_MASK;


	/* if res[0] was negative before this round we had an negative carry and
	 * have now
	 *   2^51 - 38 - 19 <= res[0] <= 2^51 - 1.
	 *
	 * so in any case it is
	 *   0 <= res[0] <= 2^51 - 1
	 * and
	 *   0 <= res[i] <= 2^51 - 1 for 1 <= i <= 4
	 * as wished.
	 *
	 * moreover res is the smallest non-negative representant of x modulo q.
	 */
}

/*
 * fld_import - import an 256bit, unsigned, little-endian integer into
 * our signed 51-bit limb format and reduce modulo 2^255-19.
 */
void
fld_import(fld_t dst, const uint8_t src[32])
{
	int i;
	uint64_t tmp = 0;
	int fill = 0;

	for (i = 0; i < FLD_LIMB_NUM; i++) {
		for (;fill < FLD_LIMB_BITS; fill += 8)
			tmp |= (uint64_t)*src++ << fill;

		dst[i] = tmp & FLD_LIMB_MASK;
		tmp >>= FLD_LIMB_BITS;
		fill -= FLD_LIMB_BITS;
	}
	dst[0] += 19*tmp;

	/* dst is now reduced and partially carried (first limb may
	 * use 52bits instead of 51).
	 */
}

/*
 * fld_export - export our internal format to a 256bit, unsigned,
 * little-endian packed format.
 */
void
fld_export(uint8_t dst[32], const fld_t src)
{
	int i;
	fld_t tmp;
	uint64_t foo = 0;
	int fill = 0;
	
	fld_reduce(tmp, src);

	for (i = 0; i < FLD_LIMB_NUM; i++) {
		foo |= (uint64_t)tmp[i] << fill;
		for (fill += FLD_LIMB_BITS; fill >= 8; fill -= 8, foo >>= 8)
			*dst++ = foo & 0xff;
	}
	*dst++ = foo & 0xff;
}

/*
 * fld_scale - multiply e by scalar s and reduce modulo q.
 */
void
fld_scale(fld_t res, const fld_t e, limb_t s)
{
	llimb_t carry;

	carry = (llimb_t)s*e[0];
	res[0] = carry & FLD_LIMB_MASK;

	carry = (carry >> FLD_LIMB_BITS) + (llimb_t)s*e[1];
	res[1] = carry & FLD_LIMB_MASK;

	carry = (carry >> FLD_LIMB_BITS) + (llimb_t)s*e[2];
	res[2] = carry & FLD_LIMB_MASK;

	carry = (carry >> FLD_LIMB_BITS) + (llimb_t)s*e[3];
	res[3] = carry & FLD_LIMB_MASK;

	carry = (carry >> FLD_LIMB_BITS) + (llimb_t)s*e[4];
	res[4] = carry & FLD_LIMB_MASK;

	res[0] += 19*(carry >> FLD_LIMB_BITS);
}

/*
 * fld_mul - multiply a with b and reduce modulo q.
 */
void
fld_mul(fld_t res, const fld_t a, const fld_t b)
{
	limb_t a19_1, a19_2, a19_3, a19_4, tmp;
	llimb_t c[5];

	a19_1 = 19*a[1];
	a19_2 = 19*a[2];
	a19_3 = 19*a[3];
	a19_4 = 19*a[4];

	c[0] = (llimb_t)a[0]*b[0] + (llimb_t)a19_1*b[4] + (llimb_t)a19_2*b[3]
		+ (llimb_t)a19_3*b[2] + (llimb_t)a19_4*b[1];
	c[1] = (llimb_t)a[0]*b[1] + (llimb_t)a[1]*b[0] + (llimb_t)a19_2*b[4]
		+ (llimb_t)a19_3*b[3] + (llimb_t)a19_4*b[2];
	c[2] = (llimb_t)a[0]*b[2] + (llimb_t)a[1]*b[1] + (llimb_t)a[2]*b[0]
		+ (llimb_t)a19_3*b[4] + (llimb_t)a19_4*b[3];
	c[3] = (llimb_t)a[0]*b[3] + (llimb_t)a[1]*b[2] + (llimb_t)a[2]*b[1]
		+ (llimb_t)a[3]*b[0] + (llimb_t)a19_4*b[4];
	c[4] = (llimb_t)a[0]*b[4] + (llimb_t)a[1]*b[3] + (llimb_t)a[2]*b[2]
		+ (llimb_t)a[3]*b[1] + (llimb_t)a[4]*b[0];


	c[1] += c[0] >> FLD_LIMB_BITS;
	c[2] += c[1] >> FLD_LIMB_BITS;
	c[3] += c[2] >> FLD_LIMB_BITS;
	c[4] += c[3] >> FLD_LIMB_BITS;

	tmp = (c[0] & FLD_LIMB_MASK) + 19*(c[4] >> FLD_LIMB_BITS);
	res[1] = (c[1] & FLD_LIMB_MASK) + (tmp >> FLD_LIMB_BITS);

	res[0] = tmp & FLD_LIMB_MASK;
	res[2] = c[2] & FLD_LIMB_MASK;
	res[3] = c[3] & FLD_LIMB_MASK;
	res[4] = c[4] & FLD_LIMB_MASK;
}

/*
 * fld_sq - square x and reduce modulo q.
 */
void
fld_sq(fld_t res, const fld_t x)
{
	limb_t x2_1, x2_2, x2_3, x2_4, x19_3, x19_4, tmp;
	llimb_t c[5];

	x2_1 = 2*x[1];
	x2_2 = 2*x[2];
	x2_3 = 2*x[3];
	x2_4 = 2*x[4];
	x19_3 = 19*x[3];
	x19_4 = 19*x[4];

	c[0] = (llimb_t)x[0]*x[0] + (llimb_t)x2_1*x19_4 + (llimb_t)x2_2*x19_3;
	c[1] = (llimb_t)x[0]*x2_1 + (llimb_t)x2_2*x19_4 + (llimb_t)x19_3*x[3];
	c[2] = (llimb_t)x[0]*x2_2 + (llimb_t)x[1]*x[1] + (llimb_t)x2_3*x19_4;
	c[3] = (llimb_t)x[0]*x2_3 + (llimb_t)x2_1*x[2] + (llimb_t)x19_4*x[4];
	c[4] = (llimb_t)x[0]*x2_4 + (llimb_t)x2_1*x[3] + (llimb_t)x[2]*x[2];

	c[1] += c[0] >> FLD_LIMB_BITS;
	c[2] += c[1] >> FLD_LIMB_BITS;
	c[3] += c[2] >> FLD_LIMB_BITS;
	c[4] += c[3] >> FLD_LIMB_BITS;

	tmp = (c[0] & FLD_LIMB_MASK) + 19*(c[4] >> FLD_LIMB_BITS);
	res[1] = (c[1] & FLD_LIMB_MASK) + (tmp >> FLD_LIMB_BITS);

	res[0] = tmp & FLD_LIMB_MASK;
	res[2] = c[2] & FLD_LIMB_MASK;
	res[3] = c[3] & FLD_LIMB_MASK;
	res[4] = c[4] & FLD_LIMB_MASK;
}

#else		/* USE_64BIT */


/*
 * 32bit implementation
 */


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
	(tmp) = (off);							\
	(tmp) <<= FLD_LIMB_BITS(0);					\
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

#endif		/* USE_64BIT */


/*
 * common code
 */


/*
 * fld_eq - compares two field elements in a time-constant manner.
 *
 * returns 1 if a == b and 0 otherwise.
 */
int
fld_eq(const fld_t a, const fld_t b)
{
	fld_t tmp;
	limb_t res;
	int i;

	/* tmp <- a - b */
	fld_sub(tmp, a, b);
	fld_reduce(tmp, tmp);

	/* and check tmp for zero */
	res = tmp[0];
	for (i = 1; i < FLD_LIMB_NUM; i++)
		res |= tmp[i];
	for (i = 4*sizeof(limb_t); i > 0; i >>= 1)
                res |= res >> i;

	/* now (res & 1) is zero iff tmp is zero */
	res = ~res & 1;

	return res;
}


/*
 * fld_inv - inverts z modulo q.
 *
 * this code is taken from nacl. it works by taking z to the q-2
 * power. by lagrange's theorem (aka 'fermat's little theorem' in this
 * special case) this gives us z^-1 modulo q.
 */
void
fld_inv(fld_t res, const fld_t z)
{
	fld_t z2;
	fld_t z9;
	fld_t z11;
	fld_t z2_5_0;
	fld_t z2_10_0;
	fld_t z2_20_0;
	fld_t z2_50_0;
	fld_t z2_100_0;
	fld_t t0;
	fld_t t1;
	int i;

	/* 2 */ fld_sq(z2, z);
	/* 4 */ fld_sq(t1, z2);
	/* 8 */ fld_sq(t0, t1);
	/* 9 */ fld_mul(z9,t0, z);
	/* 11 */ fld_mul(z11, z9, z2);
	/* 22 */ fld_sq(t0, z11);
	/* 2^5 - 2^0 = 31 */ fld_mul(z2_5_0, t0, z9);

	/* 2^6 - 2^1 */ fld_sq(t0, z2_5_0);
	/* 2^7 - 2^2 */ fld_sq(t1, t0);
	/* 2^8 - 2^3 */ fld_sq(t0, t1);
	/* 2^9 - 2^4 */ fld_sq(t1, t0);
	/* 2^10 - 2^5 */ fld_sq(t0, t1);
	/* 2^10 - 2^0 */ fld_mul(z2_10_0, t0, z2_5_0);

	/* 2^11 - 2^1 */ fld_sq(t0, z2_10_0);
	/* 2^12 - 2^2 */ fld_sq(t1, t0);
	/* 2^20 - 2^10 */ for (i = 2; i < 10; i += 2) { fld_sq(t0, t1); fld_sq(t1, t0); }
	/* 2^20 - 2^0 */ fld_mul(z2_20_0, t1, z2_10_0);

	/* 2^21 - 2^1 */ fld_sq(t0, z2_20_0);
	/* 2^22 - 2^2 */ fld_sq(t1, t0);
	/* 2^40 - 2^20 */ for (i = 2;i < 20;i += 2) { fld_sq(t0, t1); fld_sq(t1, t0); }
	/* 2^40 - 2^0 */ fld_mul(t0, t1, z2_20_0);

	/* 2^41 - 2^1 */ fld_sq(t1, t0);
	/* 2^42 - 2^2 */ fld_sq(t0, t1);
	/* 2^50 - 2^10 */ for (i = 2; i < 10; i += 2) { fld_sq(t1, t0); fld_sq(t0, t1); }
	/* 2^50 - 2^0 */ fld_mul(z2_50_0, t0, z2_10_0);

	/* 2^51 - 2^1 */ fld_sq(t0, z2_50_0);
	/* 2^52 - 2^2 */ fld_sq(t1, t0);
	/* 2^100 - 2^50 */ for (i = 2; i < 50; i += 2) { fld_sq(t0, t1); fld_sq(t1, t0); }
	/* 2^100 - 2^0 */ fld_mul(z2_100_0, t1, z2_50_0);

	/* 2^101 - 2^1 */ fld_sq(t1, z2_100_0);
	/* 2^102 - 2^2 */ fld_sq(t0, t1);
	/* 2^200 - 2^100 */ for (i = 2; i < 100; i += 2) { fld_sq(t1, t0); fld_sq(t0, t1); }
	/* 2^200 - 2^0 */ fld_mul(t1, t0, z2_100_0);

	/* 2^201 - 2^1 */ fld_sq(t0, t1);
	/* 2^202 - 2^2 */ fld_sq(t1, t0);
	/* 2^250 - 2^50 */ for (i = 2; i < 50; i += 2) { fld_sq(t0, t1); fld_sq(t1, t0); }
	/* 2^250 - 2^0 */ fld_mul(t0, t1, z2_50_0);

	/* 2^251 - 2^1 */ fld_sq(t1, t0);
	/* 2^252 - 2^2 */ fld_sq(t0, t1);
	
	/* 2^253 - 2^3 */ fld_sq(t1, t0);
	/* 2^254 - 2^4 */ fld_sq(t0, t1);
	/* 2^255 - 2^5 */ fld_sq(t1, t0);
	/* 2^255 - 21 */ fld_mul(res, t1, z11);
}


/*
 * fld_pow2523 - compute z^((q-5)/8) modulo q, ie (z*res)^2 is either z
 * or -z modulo q.
 *
 * this function is used to mix a square-root modulo q with an invertation in
 * ed_import. see the ed25519 paper for an explanation.
 *
 * this code is, like fld_inv, taken from nacl.
 */
void
fld_pow2523(fld_t res, const fld_t z)
{
        fld_t z2;
        fld_t z9;
        fld_t z2_5_0;
        fld_t z2_10_0;
        fld_t z2_20_0;
        fld_t z2_50_0;
        fld_t z2_100_0;
        fld_t t;
        int i;
                
        /* 2 */ fld_sq(z2, z);
        /* 4 */ fld_sq(t, z2);
        /* 8 */ fld_sq(t, t);
        /* 9 */ fld_mul(z9, t, z);
        /* 11 */ fld_mul(t, z9, z2);
        /* 22 */ fld_sq(t, t);
        /* 2^5 - 2^0 = 31 */ fld_mul(z2_5_0, t, z9);

        /* 2^6 - 2^1 */ fld_sq(t, z2_5_0);
        /* 2^10 - 2^5 */ for (i = 1;i < 5;i++) { fld_sq(t, t); }
        /* 2^10 - 2^0 */ fld_mul(z2_10_0, t, z2_5_0);

        /* 2^11 - 2^1 */ fld_sq(t, z2_10_0);
        /* 2^20 - 2^10 */ for (i = 1;i < 10;i++) { fld_sq(t, t); }
        /* 2^20 - 2^0 */ fld_mul(z2_20_0, t, z2_10_0);

        /* 2^21 - 2^1 */ fld_sq(t, z2_20_0);
        /* 2^40 - 2^20 */ for (i = 1;i < 20;i++) { fld_sq(t, t); }
        /* 2^40 - 2^0 */ fld_mul(t, t, z2_20_0);

        /* 2^41 - 2^1 */ fld_sq(t, t);
        /* 2^50 - 2^10 */ for (i = 1;i < 10;i++) { fld_sq(t, t); }
        /* 2^50 - 2^0 */ fld_mul(z2_50_0, t, z2_10_0);

        /* 2^51 - 2^1 */ fld_sq(t, z2_50_0);
        /* 2^100 - 2^50 */ for (i = 1;i < 50;i++) { fld_sq(t, t); }
        /* 2^100 - 2^0 */ fld_mul(z2_100_0, t, z2_50_0);

        /* 2^101 - 2^1 */ fld_sq(t, z2_100_0);
        /* 2^200 - 2^100 */ for (i = 1;i < 100;i++) { fld_sq(t, t); }
        /* 2^200 - 2^0 */ fld_mul(t, t, z2_100_0);

        /* 2^201 - 2^1 */ fld_sq(t, t);
        /* 2^250 - 2^50 */ for (i = 1;i < 50;i++) { fld_sq(t, t); }
        /* 2^250 - 2^0 */ fld_mul(t, t, z2_50_0);

        /* 2^251 - 2^1 */ fld_sq(t, t);
        /* 2^252 - 2^2 */ fld_sq(t, t);
        /* 2^252 - 3 */ fld_mul(res, t, z);
}
