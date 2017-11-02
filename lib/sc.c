/*
 * Implementation of the ring Z/mZ for
 *   m := 2^252 + 27742317777372353535851937790883648493.
 *
 * We use this ring (which is actually a field, but we are not
 * interested in dividing here) as scalar ring for our base point
 * B. Since B has order m the operation of Z/mZ on { xB | x in Z } is
 * well defined.
 *
 * This code is public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>.
 */

#include <stdint.h>

#include "bitness.h"
#include "compat.h"
#include "sc.h"


#define K	SC_LIMB_NUM
#define MSK	SC_LIMB_MASK
#define LB	SC_LIMB_BITS



#ifdef USE_64BIT

static const limb_t con_m[K+1] = {
	671914833335277, 3916664325105025, 1367801, 0, 17592186044416, 0 };

/* mu = floor(b^(2*k) / m) */
static const limb_t con_mu[K+1] = {
	1586638968003385, 147551898491342, 4503509987107165, 4503599627370495,
	4503599627370495, 255 };


/* off = 8 * (16^64 - 1) / 15 mod m */
const sc_t con_off = { 1530200761952544, 2593802592017535, 2401919790321849,
		      2401919801264264, 9382499223688 };

#else

static const limb_t con_m[K+1] = {
	16110573, 10012311, 30238081, 58362846, 1367801, 0, 0, 0, 0,
	262144, 0 };

static const limb_t con_mu[K+1] = {
	1252153, 23642763, 41867726, 2198694, 17178973, 67107528, 67108863,
	67108863, 67108863, 67108863, 255 };

/* off = 8 * (16^64 - 1) / 15 mod m */
const sc_t con_off = { 14280992, 22801768, 35478655, 38650670, 65114297,
		       35791393, 8947848, 35791394, 8947848, 139810 };

#endif




/*
 * sc_barrett - reduce x modulo m using barrett reduction (HAC 14.42):
 * with the notation of (14.42) we use k = K limbs and b = 2^LB as
 * (actual) limb size.
 *
 * NOTE: x must be carried and non negative.
 *
 * as long as x <= b^(k-1) * (b^(k+1) - mu), res will be fully
 * reduced. this is normally true since we have (for our choices of k
 * and b)
 *		x < m^2 < b^(k-1) * (b^(k+1) - mu)
 * if x is the result of a multiplication x = a * b with a, b < m.
 *
 * in the case of b^(k-1) * (b^(k+1) - mu) < x < b^(2k) the caller must
 * conditionally subtract m from the result.
 *
 */
static void
sc_barrett(sc_t res, const lsc_t x)
{
	llimb_t carry;
	limb_t q[K+1], r[K+1];
	limb_t mask;

	int i, j;

	/* 
	 * step 1: q <- floor( floor(x/b^(k-1)) * mu / b^(k+1) )
	 */

	/* calculate carry from the (k-1)-th and k-th position of floor(x/b^(k-1))*mu */
	carry = 0;
	for (i = 0; i <= K-1; i++)
		carry += (llimb_t)x[K-1+i] * con_mu[K-1-i];
	carry >>= LB;
	for (i = 0; i <= K; i++)
		carry += (llimb_t)x[K-1+i] * con_mu[K-i];

	
	for (j = K+1; j <= 2*K; j++) {
		carry >>= LB;
		for (i = j-K; i <= K; i++)
			carry += (llimb_t)x[K-1+i] * con_mu[j-i];
		
		q[j-K-1] = carry & MSK;
	}
	q[j-K-1] = carry >> LB;
	

	/*
	 * step 2: r <- (x - q * m) mod b^(k+1)
	 */

	/* r <- q*m mod b^(k+1) */
	for (j = 0, carry = 0; j <= K; j++) {
		carry >>= LB;
		
		for (i = 0; i <= j; i++)
			carry += (llimb_t) q[i]*con_m[j-i];
		
		r[j] = carry & MSK;
	}

	/* r <- x - r mod b^(k+1) */
	for (i = 0, carry = 0; i <= K; i++) {
		carry = (carry >> LB) + x[i] - r[i];
		r[i] = carry & MSK;
	}


	/*
	 * step 3: if (r < 0) r += b^(k+1);
	 */

	/* done by ignoring the coefficient of b^(k+1) (= carry >> LB)
	 * after the last loop above.
	 */

	/*
	 * step 4: if (r > m) r -= m;
	 */
	q[0] = r[0] - con_m[0];
	for (i = 1; i <= K; i++) {
		q[i] = (q[i-1] >> LB) + r[i] - con_m[i];
		q[i-1] &= MSK;
	}

	mask = ~(q[K] >> (8*sizeof(limb_t)-1));
	for (i = 0; i <= K; i++)
		r[i] ^= (r[i] ^ q[i]) & mask;

	/*
	 * step 5: copy out and clean up
	 */
	for (i = 0; i < K; i++)
		res[i] = r[i];
}


/*
 * sc_reduce - completely carry and reduce element e.
 */
void
sc_reduce(sc_t dst, const sc_t e)
{
	lsc_t tmp;
	limb_t carry;
	int i;

	/* carry e */
	for (carry = 0, i = 0; i < K; i++) {
		carry = (carry >> LB) + e[i];
		tmp[i] = carry & MSK;
	}
	tmp[K] = carry >> LB;
	for (i = K+1; i < 2*K; i++)
		tmp[i] = 0;
	
	/* reduce modulo m */
	sc_barrett(dst, tmp);
}

/*
 * sc_import - import packed 256bit/512bit little-endian encoded integer
 * to our internal sc_t format.
 *
 * assumes:
 *   len <= 64
 */
void
sc_import(sc_t dst, const uint8_t *src, size_t len)
{
	const uint8_t *endp = src + len;
	lsc_t tmp;
	uint64_t foo;
	int i, fill;

	fill = 0;
	foo = 0;
	for (i = 0; i < 2*K; i++) {
		while (src < endp && fill < LB) {
			foo |= (uint64_t)*src++ << fill;
			fill += 8;
		}
		
		tmp[i] = foo & MSK;

		foo >>= LB;
		fill -= LB;
	}

	sc_barrett(dst, tmp);
}


/*
 * sc_export - export internal sc_t format to an unsigned, 256bit
 * little-endian integer.
 */
void
sc_export(uint8_t dst[32], const sc_t x)
{
	const uint8_t *endp = dst+32;
	sc_t tmp;
	uint64_t foo;
	int fill, i;

	sc_reduce(tmp, x);

	for (i = 0, foo = 0, fill = 0; i < K; i++) {
		foo |= (uint64_t)tmp[i] << fill;
		for (fill += LB; fill >= 8 && dst < endp; fill -= 8, foo >>= 8)
			*dst++ = foo & 0xff;
	}
}

/*
 * sc_mul - multiply a with b and reduce modulo m
 */
void
sc_mul(sc_t res, const sc_t a, const sc_t b)
{
	int i, k;
	lsc_t tmp;
	llimb_t carry;

	carry = 0;

	for (k = 0; k < K; k++) {
		carry >>= LB;
		for (i = 0; i <= k; i++)
			carry += (llimb_t) a[i] * b[k-i];
		tmp[k] = carry & MSK;
	}
	
	for (k = K; k < 2*K-1; k++) {
		carry >>= LB;
		for (i = k-K+1; i <= K-1; i++)
			carry += (llimb_t) a[i] * b[k-i];
		tmp[k] = carry & MSK;
	}
	tmp[k] = carry >>= LB;

	sc_barrett(res, tmp);
}


/*
 * jsfdigit - helper for sc_jsf (vartime)
 */
static int
jsfdigit(unsigned int a, unsigned int b)
{
	int u = 2 - (a & 0x03);
	if (u == 2)
		return 0;
	if ( ((a & 0x07) == 3 || (a & 0x07) == 5) && (b & 0x03) == 2 )
		return -u;
	return u;
}


/*
 * sc_jsf - calculate joint sparse form of a and b. (vartime)
 *
 * assumes:
 *  a and b carried and reduced
 *
 * NOTE: this function runs in variable time and due to the nature of
 * the optimization JSF is needed for (see ed_dual_scale), there is
 * no point in creating a constant-time version.
 *
 * returns the highest index k >= 0 with max(u0[k], u1[k]) != 0
 * or -1 in case u0 and u1 are all zero.
 */
int
sc_jsf(int u0[SC_BITS+1], int u1[SC_BITS+1], const sc_t a, const sc_t b)
{
	limb_t n0, n1;
	int i, j, k;

	k = n0 = n1 = 0;

	for (i = 0; i < K; i++) {
		n0 += a[i];
		n1 += b[i];

		for (j = 0; j < LB; j++, k++) {
			u0[k] = jsfdigit(n0, n1);
			u1[k] = jsfdigit(n1, n0);

			n0 = (n0 - u0[k]) >> 1;
			n1 = (n1 - u1[k]) >> 1;
		}
	}
	u0[k] = jsfdigit(n0, n1);
	u1[k] = jsfdigit(n1, n0);

	while (k >= 0 && u0[k] == 0 && u1[k] == 0)
		k--;

	return k;
}
