#ifndef FLD_H
#define FLD_H

#include <stdint.h>

#include "bitness.h"
#include "compat.h"
#include "limb.h"


#ifdef USE_64BIT

/*
 * in 64bit mode we use 5 limbs each 51 bits long
 */

#define FLD_LIMB_NUM		5
#define FLD_LIMB_BITS		51

#define FLD_LIMB_MASK		((1L << FLD_LIMB_BITS)-1)

#else

/*
 * in 32bit mode fld_t consists of 10 limbs alternating in size
 * between 26 and 25 bits.
 * this approach is inspired from djb's curve25519-paper, where it
 * is explained in more detail.
 */

#define FLD_LIMB_NUM		10

/* macros for alternating limb sizes: use with d=1,0,1,0,... */
#define FLD_LIMB_BITS(d)	(25+(d))
#define FLD_LIMB_MASK(d)	((1 << FLD_LIMB_BITS(d))-1)

#endif


/*
 * fld_t is our datatype for all operations modulo 2^255-19.
 *
 * since we typedef an array here, all parameters of type fld_t get
 * a call-by-reference semantic!
 */

typedef limb_t fld_t[FLD_LIMB_NUM];



/*
 * exported constants
 */
extern const fld_t con_d;
extern const fld_t con_2d;
extern const fld_t con_m2d;
extern const fld_t con_j;


/*
 * prototypes for 32bit/64bit specific functions 
 */
void	fld_reduce(fld_t dst, const fld_t x);
void	fld_import(fld_t dst, const uint8_t src[32]);
void	fld_export(uint8_t dst[32], const fld_t src);
void	fld_mul(fld_t res, const fld_t a, const fld_t b);
void	fld_scale(fld_t dst, const fld_t src, limb_t x);
void	fld_sq(fld_t res, const fld_t a);


/*
 * prototypes for common code
 */
int	fld_eq(const fld_t a, const fld_t b);
void	fld_inv(fld_t res, const fld_t z);
void	fld_pow2523(fld_t res, const fld_t z);



/*
 * simple inline functions
 */

static INLINE void
fld_set0(fld_t res, limb_t x0)
{
	int i;
	res[0] = x0;
	for (i = 1; i < FLD_LIMB_NUM; i++)
		res[i] = 0;
}


static INLINE void
fld_add(fld_t res, const fld_t a, const fld_t b)
{
	int i;
	for (i = 0; i < FLD_LIMB_NUM; i++)
		res[i] = a[i] + b[i];
}

static INLINE void
fld_sub(fld_t res, const fld_t a, const fld_t b)
{
	int i;
	for (i = 0; i < FLD_LIMB_NUM; i++)
		res[i] = a[i] - b[i];
}

/*
 * fld_tinyscale scales an element a without reducing it. this could
 * be used for conditionally change sign of an element.
 */
static INLINE void
fld_tinyscale(fld_t res, const fld_t a, limb_t x)
{
	int i;
	for (i = 0; i < FLD_LIMB_NUM; i++)
		res[i] = x * a[i];
}

/*
 * fld_scale2 is a special case of fld_tinyscale with x = 2.
 */
static INLINE void
fld_scale2(fld_t res, const fld_t a)
{
	int i;
	for (i = 0; i < FLD_LIMB_NUM; i++)
		res[i] = a[i] << 1;
}

/*
 * fld_neg is a special case of fld_tinyscale with x = -1.
 */
static INLINE void
fld_neg(fld_t res, const fld_t a)
{
	int i;
	for (i = 0; i < FLD_LIMB_NUM; i++)
		res[i] = -a[i];
}

#endif
