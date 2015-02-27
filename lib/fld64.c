/*
 * 64bit functions implementing the field GF(q) with q := 2^255-19.
 *
 * This code is public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>
 */

#include <stdint.h>

#include "fld.h"


#ifndef USE_64BIT
#error "This module must be compiled with -DUSE_64BIT"
#else



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
	 *   0 <= res[i] <= 2^51-1 for 1 <= i < 5.
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
	 *   0 <= res[i] <= 2^51 - 1 for 1 <= i < 5
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

#endif
