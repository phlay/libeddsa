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

/* j^2 = 1 (mod q) */
const fld_t con_j = {
	1718705420411056, 234908883556509, 2233514472574048, 2117202627021982,
	765476049583133 };




/*
 * fld_carry - do one carry-reduce round.
 *
 * after that all limbs but the first are carried. to completly reduce
 * x modulo 2^255-19 a second call may be needed.
 *
 * this function assumes
 *	x[i] <= 2^64 - 2^13 = 8192 * (2^51-1)
 * for i > 0.
 */
static void
fld_carry(fld_t res, const fld_t x)
{
	int i;

	/* carry loop */
	res[0] = x[0];
	for (i = 1; i < FLD_LIMB_NUM; i++) {
		res[i] = (res[i-1] >> FLD_LIMB_BITS) + x[i];
		res[i-1] &= FLD_LIMB_MASK;
	}
	
	/* reduce modulo q */
	res[0] += 19 * (res[FLD_LIMB_NUM-1] >> FLD_LIMB_BITS);
	res[FLD_LIMB_NUM-1] &= FLD_LIMB_MASK;
}



/*
 * fld_reduce - completely reduce x modulo q.
 *
 * This function carry-reduces two times (which is always enough) and
 * takes special precaution to correctly reduce values of x with
 * 			q <= x < 2^255.
 *
 * As with fld_carry all limbs x[i] are assumed to hold
 * 		     x[i] <= 2^13 * (2^51-1).
 */
void
fld_reduce(fld_t res, const fld_t x)
{
	int i;

	/* first carry round with offset 19 */
	res[0] = x[0] + 19;
	for (i = 1; i < FLD_LIMB_NUM; i++) {
		res[i] = (res[i-1] >> FLD_LIMB_BITS) + x[i];
		res[i-1] &= FLD_LIMB_MASK;
	}
	res[0] += 19*(res[FLD_LIMB_NUM-1] >> FLD_LIMB_BITS);
	res[FLD_LIMB_NUM-1] &= FLD_LIMB_MASK;
	
	/* subtract 19 again */
	res[0] -= 19;

	/* second carry round */
	fld_carry(res, res);
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
	int i;
	llimb_t carry = 0;

	for (i = 0; i < FLD_LIMB_NUM; i++) {
		carry = (carry >> FLD_LIMB_BITS) + (llimb_t) s * e[i];
		res[i] = carry & FLD_LIMB_MASK;
	}
	res[0] += 19*(carry >> FLD_LIMB_BITS);
}


/*
 * fld_mul - multiply a with b and reduce modulo q.
 */
void
fld_mul(fld_t res, const fld_t a, const fld_t b)
{
	fld_t tmp;
	llimb_t carry;

	carry = (llimb_t)a[0]*b[0] +
		19*((llimb_t)a[1]*b[4] +
		    (llimb_t)a[2]*b[3] +
		    (llimb_t)a[3]*b[2] +
		    (llimb_t)a[4]*b[1]);
	tmp[0] = carry & FLD_LIMB_MASK;

	carry >>= FLD_LIMB_BITS;
	carry += (llimb_t)a[0]*b[1] + (llimb_t)a[1]*b[0] +
		19*((llimb_t)a[2]*b[4] +
		    (llimb_t)a[3]*b[3] +
		    (llimb_t)a[4]*b[2]);
	tmp[1] = carry & FLD_LIMB_MASK;

	carry >>= FLD_LIMB_BITS;
	carry += (llimb_t)a[0]*b[2] +
		 (llimb_t)a[1]*b[1] +
		 (llimb_t)a[2]*b[0] +
		19*((llimb_t)a[3]*b[4] +
		    (llimb_t)a[4]*b[3]);
	tmp[2] = carry & FLD_LIMB_MASK;

	carry >>= FLD_LIMB_BITS;
	carry += (llimb_t)a[0]*b[3] +
		 (llimb_t)a[1]*b[2] +
		 (llimb_t)a[2]*b[1] +
		 (llimb_t)a[3]*b[0] +
		 (llimb_t)19*a[4]*b[4];
	tmp[3] = carry & FLD_LIMB_MASK;

	carry >>= FLD_LIMB_BITS;
	carry += (llimb_t)a[0]*b[4] +
		 (llimb_t)a[1]*b[3] +
		 (llimb_t)a[2]*b[2] +
		 (llimb_t)a[3]*b[1] +
		 (llimb_t)a[4]*b[0];
	tmp[4] = carry & FLD_LIMB_MASK;

	tmp[0] += 19*(carry >> FLD_LIMB_BITS);
	
	fld_carry(res, tmp);
}

/*
 * fld_sq - square x and reduce modulo q.
 */
void
fld_sq(fld_t res, const fld_t x)
{
	fld_t tmp;
	llimb_t carry;

	carry = (llimb_t)x[0]*x[0] +
		38*((llimb_t)x[1]*x[4] + (llimb_t)x[2]*x[3]);
	tmp[0] = carry & FLD_LIMB_MASK;

	carry >>= FLD_LIMB_BITS;
	carry += ( (llimb_t)x[0]*x[1] << 1 ) +
		19*(((llimb_t)x[2]*x[4] << 1) + (llimb_t)x[3]*x[3]);
	tmp[1] = carry & FLD_LIMB_MASK;

	carry >>= FLD_LIMB_BITS;
	carry += ( (llimb_t)x[0]*x[2] << 1 ) +
		(llimb_t)x[1]*x[1] + (llimb_t)38*x[3]*x[4];
	tmp[2] = carry & FLD_LIMB_MASK;

	carry >>= FLD_LIMB_BITS;
	carry += ( ((llimb_t)x[0]*x[3] + (llimb_t)x[1]*x[2]) << 1 ) +
		(llimb_t)19*x[4]*x[4];
	tmp[3] = carry & FLD_LIMB_MASK;

	carry >>= FLD_LIMB_BITS;
	carry += ( ((llimb_t)x[0]*x[4] + (llimb_t)x[1]*x[3]) << 1 ) +
		(llimb_t)x[2]*x[2];
	tmp[4] = carry & FLD_LIMB_MASK;

	tmp[0] += 19*(carry >> FLD_LIMB_BITS);

	fld_carry(res, tmp);
}
#endif
