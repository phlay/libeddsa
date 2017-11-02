/*
 * Implementing twisted edwards curve
 * 	E : -x^2 + y^2 = 1 - (121665/121666) x^2 y^2
 * over GF(2^255-19) according to [1].
 *
 * This code is public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>.
 *
 * References:
 * [1] High-speed high-security signatures, 2011/09/26,
 *     Bernstein, Duif, Lange, Schwabe, Yang
 * [2] Twisted Edwards Curves Revisited, 2008,
 *     Hisil, Wong, Carter, Dawson
 */

#include <stdint.h>
#include <string.h>

#include "bitness.h"
#include "fld.h"
#include "sc.h"
#include "ed.h"


/*
 * special pre-computed form of a point on the curve, used
 * for the lookup table and some optimizations.
 */
struct pced {
	fld_t		diff;		/* y - x */
	fld_t		sum;		/* y + x */
	fld_t		prod;		/* 2*d*t */
};



#ifdef USE_64BIT

/* lookup-table for ed_scale_base - 64bit version */
static const struct pced ed_lookup[][8] = {
  #include "ed_lookup64.h"
};

/* base point B of our group in pre-computed form */
const struct pced pced_B = {
	{62697248952638, 204681361388450, 631292143396476,
	 338455783676468, 1213667448819585},
	{1288382639258501, 245678601348599, 269427782077623,
	 1462984067271730, 137412439391563},
	{301289933810280, 1259582250014073, 1422107436869536,
	 796239922652654, 1953934009299142} };

#else

/* lookup-table for ed_scale_base - 32bit version */
static const struct pced ed_lookup[][8] = {
  #include "ed_lookup32.h"
};

/* base point B of our group in pre-computed form */
const struct pced pced_B = {
	{54563134, 934261, 64385954, 3049989, 66381436,
	 9406985, 12720692, 5043384, 19500929, 18085054},
	{25967493, 19198397, 29566455, 3660896, 54414519,
	 4014786, 27544626, 21800161, 61029707, 2047604},
	{58370664, 4489569, 9688441, 18769238, 10184608,
	 21191052, 29287918, 11864899, 42594502, 29115885} };

#endif

const struct ed ed_zero = { { 0 }, { 1 }, { 0 }, { 1 } };
const struct pced pced_zero = { { 1 }, { 1 }, { 0 } };


/*
 * memselect - helper function to select one of two buffers in a
 * time-constant way.
 */
static void
memselect(void *out, const void *pa, const void *pb, size_t max, int flag)
{
	uint8_t *a = (uint8_t*)pa;
	uint8_t *b = (uint8_t*)pb;
	uint8_t *p = (uint8_t*)out;
	uint8_t *endp = p+max;

	uint8_t mB = (flag & 1) - 1;
	uint8_t mA = ~mB;

	while (p < endp)
		*p++ = (mA & *a++) ^ (mB & *b++);
}


/*
 * ed_import - import a point P on the curve from packet 256bit encoding.
 *
 */
void
ed_import(struct ed *P, const uint8_t in[32])
{
	fld_t U, V, A, B;
	uint8_t tmp[32];
	int flag;

	/* import y */
	memcpy(tmp, in, 32);
	tmp[31] &= 0x7f;
	
	fld_import(P->y, tmp);

	
	/* U <- y^2 - 1,  V <- d*y^2 + 1 */
	fld_sq(U, P->y);
	fld_mul(V, con_d, U);
	U[0]--;
	V[0]++;

	/* A <- v^2 */
	fld_sq(A, V);
	/* B <- v^4 */
	fld_sq(B, A);
	/* A <- uv^3 */
	fld_mul(A, A, U);
	fld_mul(A, A, V);
	/* B <- (uv^7)^((q-5)/8) */
	fld_mul(B, B, A);
	fld_pow2523(B, B);
	/* B <- uv^3 * (uv^7)^((q-5)/8) */
	fld_mul(B, B, A);

	/* A <- v * B^2 */
	fld_sq(A, B);
	fld_mul(A, A, V);
	/* flag <- v*B^2 == u */
	flag = fld_eq(A, U);

	/* A <- j * B */
	fld_mul(A, con_j, B);

	memselect(P->x, B, A, sizeof(fld_t), flag);
	fld_reduce(P->x, P->x);
	fld_tinyscale(P->x, P->x, 1 - 2*((in[31] >> 7) ^ (P->x[0] & 1)));

	/* compute t and z */
	fld_mul(P->t, P->x, P->y);
	fld_set0(P->z, 1);
}


/*
 * ed_export - export point P to packed 256bit format.
 */
void
ed_export(uint8_t out[32], const struct ed *P)
{
	fld_t x, y, zinv;

	/* divide x and y by z to get affine coordinates */
	fld_inv(zinv, P->z);
	fld_mul(x, P->x, zinv);
	fld_mul(y, P->y, zinv);
	fld_export(out, y);

	/* lsb decides the sign of x */
	fld_reduce(x, x);
	out[31] |= (x[0] & 1) << 7;
}


/*
 * ed_add - add points P and Q
 */
static void
ed_add(struct ed *out, const struct ed *P, const struct ed *Q)
{
	fld_t a, b, c, d, e, f, g, h, t;
	
	fld_sub(a, P->y, P->x);
	fld_sub(t, Q->y, Q->x);
	fld_mul(a, a, t);

	fld_add(b, P->y, P->x);
	fld_add(t, Q->y, Q->x);
	fld_mul(b, b, t);

	fld_mul(c, P->t, Q->t);
	fld_mul(c, c, con_2d);

	fld_mul(d, P->z, Q->z);
	fld_scale2(d, d);

	fld_sub(e, b, a);
	fld_sub(f, d, c);
	fld_add(g, d, c);
	fld_add(h, b, a);
	
	fld_mul(out->x, e, f);
	fld_mul(out->y, g, h);
	fld_mul(out->t, e, h);
	fld_mul(out->z, f, g);
}


/*
 * ed_double - special case of ed_add for P=Q.
 *
 * little bit faster, since 4 multiplications are turned into squares.
 */
static void
ed_double(struct ed *out, const struct ed *P)
{
	fld_t a, b, c, d, e, f, g, h;
	
	fld_sub(a, P->y, P->x);
	fld_sq(a, a);

	fld_add(b, P->y, P->x);
	fld_sq(b, b);

	fld_sq(c, P->t);
	fld_mul(c, c, con_2d);

	fld_sq(d, P->z);
	fld_scale2(d, d);

	fld_sub(e, b, a);
	fld_sub(f, d, c);
	fld_add(g, d, c);
	fld_add(h, b, a);
	
	fld_mul(out->x, e, f);
	fld_mul(out->y, g, h);
	fld_mul(out->t, e, h);
	fld_mul(out->z, f, g);
}

/*
 * ed_sub - subtract two points P and Q.
 *
 * alternatively we could negate a point (which is cheap) and use
 * ed_add, but this is a little bit faster.
 */
static void
ed_sub(struct ed *out, const struct ed *P, const struct ed *Q)
{
	fld_t a, b, c, d, e, f, g, h, t;
	
	fld_sub(a, P->y, P->x);
	fld_add(t, Q->y, Q->x);
	fld_mul(a, a, t);

	fld_add(b, P->y, P->x);
	fld_sub(t, Q->y, Q->x);
	fld_mul(b, b, t);

	fld_mul(c, P->t, Q->t);
	fld_mul(c, c, con_m2d);

	fld_mul(d, P->z, Q->z);
	fld_scale2(d, d);

	fld_sub(e, b, a);
	fld_sub(f, d, c);
	fld_add(g, d, c);
	fld_add(h, b, a);
	
	fld_mul(out->x, e, f);
	fld_mul(out->y, g, h);
	fld_mul(out->t, e, h);
	fld_mul(out->z, f, g);
}


/*
 * ed_add_pc - add points P and Q where Q is in precomputed form
 *
 * the precomputed form is used for the lookup table and as an
 * optimization in ed_double_scale.
 */
static void
ed_add_pc(struct ed *out, const struct ed *P, const struct pced *Q)
{
	fld_t a, b, c, d, e, f, g, h;

	fld_sub(a, P->y, P->x);
	fld_mul(a, a, Q->diff);

	fld_add(b, P->y, P->x);
	fld_mul(b, b, Q->sum);

	fld_mul(c, P->t, Q->prod);
	fld_scale2(d, P->z);

	fld_sub(e, b, a);
	fld_sub(f, d, c);
	fld_add(g, d, c);
	fld_add(h, b, a);
	
	fld_mul(out->x, e, f);
	fld_mul(out->y, g, h);
	fld_mul(out->t, e, h);
	fld_mul(out->z, f, g);
}

/*
 * ed_sub_pc - subtract P and Q where Q is in precomputed form.
 */
static void
ed_sub_pc(struct ed *out, const struct ed *P, const struct pced *Q)
{
	fld_t a, b, c, d, e, f, g, h;

	fld_sub(a, P->y, P->x);
	fld_mul(a, a, Q->sum);

	fld_add(b, P->y, P->x);
	fld_mul(b, b, Q->diff);

	fld_mul(c, P->t, Q->prod);
	fld_neg(c, c);

	fld_scale2(d, P->z);

	fld_sub(e, b, a);
	fld_sub(f, d, c);
	fld_add(g, d, c);
	fld_add(h, b, a);
	
	fld_mul(out->x, e, f);
	fld_mul(out->y, g, h);
	fld_mul(out->t, e, h);
	fld_mul(out->z, f, g);
}


/*
 * scale16 - helper function for ed_scale_base, returns x * 16^pow * base
 *
 * assumes:
 *  -8 <= x <= 7
 *   0 <= pow <= 62
 *   2 | pow
 */
static void
scale16(struct pced *out, int pow, int x)
{
	struct pced R = { { 0 }, { 0 }, { 0 } };
	limb_t mA, mB, mask;
	int neg, sgnx, absx;
	int i, k;
	
	neg = (x >> 3) & 1;
	sgnx = 1 - 2*neg;
	absx = sgnx * x;
	pow >>= 1;

	/* handle abs(x) == 0 */
	mask = absx | (absx >> 2);
	mask |= mask >> 1;
	mask = (mask & 1) - 1;
	for (i = 0; i < FLD_LIMB_NUM; i++) {
		R.diff[i] ^= pced_zero.diff[i] & mask;
		R.sum[i] ^= pced_zero.sum[i] & mask;
		R.prod[i] ^= pced_zero.prod[i] & mask;
	}


	/* go through our table and look for abs(x) */
	for (k = 0; k < 8; k++) {
		absx--;
		mask = absx | (absx >> 2);
		mask |= mask >> 1;
		mask = (mask & 1) - 1;
		for (i = 0; i < FLD_LIMB_NUM; i++) {
			R.diff[i] ^= ed_lookup[pow][k].diff[i] & mask;
			R.sum[i] ^= ed_lookup[pow][k].sum[i] & mask;
			R.prod[i] ^= ed_lookup[pow][k].prod[i] & mask;
		}
	}

	/* conditionally negate R and write to out */
	mA = neg-1;
	mB = ~mA;
	for (i = 0; i < FLD_LIMB_NUM; i++) {
		out->diff[i] = (mA & R.diff[i]) ^ (mB & R.sum[i]);
		out->sum[i]  = (mB & R.diff[i]) ^ (mA & R.sum[i]);
		out->prod[i] = sgnx * R.prod[i];
	}
}


/*
 * ed_scale_base - calculates x * base
 */
void
ed_scale_base(struct ed *out, const sc_t x)
{
	struct ed R0, R1;
	struct pced P;
	sc_t tmp;
	uint8_t pack[32];
	int r[64];
	int i;
	
	/* s <- x + 8 * (16^64 - 1) / 15 */
	sc_add(tmp, x, con_off);
	sc_export(pack, tmp);
	for (i = 0; i < 32; i++) {
		r[2*i] = (pack[i] & 0x0f) - 8;
		r[2*i+1] = (pack[i] >> 4) - 8;
	}
	
	/*
	 * R0 <- r0*B + r2*16^2*B + ... + r62*16^62*B 
	 * R1 <- r1*B + r3*16^2*B + ... + r63*16^62*B
	 */
	memcpy(&R0, &ed_zero, sizeof(struct ed));
	memcpy(&R1, &ed_zero, sizeof(struct ed));
	for (i = 0; i < 63; i += 2) {
		scale16(&P, i, r[i]);
		ed_add_pc(&R0, &R0, &P);

		scale16(&P, i, r[i+1]);
		ed_add_pc(&R1, &R1, &P);
	}
	
	/* R1 <- 16 * R1 */
	for (i = 0; i < 4; i++)
		ed_add(&R1, &R1, &R1);

	/* out <- R0 + R1 */
	ed_add(out, &R0, &R1);
}


/*
 * helper function to speed up ed_double_scale
 */
static void
ed_precompute(struct pced *R, const struct ed *P)
{
	fld_sub(R->diff, P->y, P->x);
	fld_add(R->sum, P->y, P->x);
	fld_mul(R->prod, P->t, con_2d);
}


/*
 * ed_dual_scale - calculates R = x*base + y*Q.  (vartime)
 *
 * Note: This algorithms does NOT run in constant time! Please use this
 * only for public information like in ed25519_verify().
 *
 * assumes:
 *   Q is affine, ie has z = 1
 *   x and y must be reduced
 */
void
ed_dual_scale(struct ed *R,
	      const sc_t x,
	      const sc_t y, const struct ed *Q)
{
	struct ed QpB, QmB;
	struct pced pcQ;

	int ux[SC_BITS+1], uy[SC_BITS+1];
	int n, i;

	memcpy(R, &ed_zero, sizeof(struct ed));

	/* calculate joint sparse form of x and y */
	n = sc_jsf(ux, uy, x, y);
	if (n == -1)
		return;

	/* precompute Q, Q+B and Q-B */
	ed_add_pc(&QpB, Q, &pced_B);
	ed_sub_pc(&QmB, Q, &pced_B);
	ed_precompute(&pcQ, Q);
	
	/* now we calculate R = x * base_point + y * Q using fast shamir method */
	for (i = n; ; i--) {
		if (ux[i] == 1) {
			if (uy[i] == 1)
				ed_add(R, R, &QpB);
			else if (uy[i] == -1)
				ed_sub(R, R, &QmB);
			else
				ed_add_pc(R, R, &pced_B);
			
		} else if (ux[i] == -1) {
			if (uy[i] == 1)
				ed_add(R, R, &QmB);
			else if (uy[i] == -1)
				ed_sub(R, R, &QpB);
			else
				ed_sub_pc(R, R, &pced_B);

		} else {
			if (uy[i] == 1)
				ed_add_pc(R, R, &pcQ);
			else if (uy[i] == -1)
				ed_sub_pc(R, R, &pcQ);
		}

		if (i == 0) break;

		ed_double(R, R);
	}
}
