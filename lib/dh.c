/*
 * implement curve25519 diffie-hellman from [1].
 *
 * This code is public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>
 *
 * References:
 * [1] Curve25519: new Diffie-Hellman speed records, 2006/02/09, Bernstein.
 */

#include <stdint.h>
#include <string.h>

#include <eddsa.h>

#include "fld.h"
#include "burnstack.h"

/*
 * structure for a point of the elliptic curve in montgomery form
 * without it's y-coordinate.
 */
struct mg {
	fld_t	x;
	fld_t	z;
};


/*
 * ctmemswap - helper function to conditionally swap a with b
 */
static void
ctmemswap(void *a, void *b, size_t len, uint8_t mask)
{
	uint8_t *pa = (uint8_t*)a;
	uint8_t *pb = (uint8_t*)b;
	uint8_t *endp = pa + len;
	uint8_t delta;

	while (pa < endp) {
		delta = (*pa ^ *pb) & mask;
		*pa++ ^= delta;
		*pb++ ^= delta;
	}
}



/*
 * montgomery - calculate montgomery's double-and-add formula
 *
 * input: A, B, C := A-B with z=1
 * output: A <- 2*A, B <- A+B
 *
 */
static void
montgomery(struct mg *A, struct mg *B, const struct mg *C)
{
	fld_t sumA, subA, sqsumA, sqsubA;
	fld_t sumB, subB;
	fld_t T1, T2, T3;
	
	/* calculate 2*A */
	fld_add(sumA, A->x, A->z);
	fld_sq(sqsumA, sumA);
	
	fld_sub(subA, A->x, A->z);
	fld_sq(sqsubA, subA);
	
	fld_mul(A->x, sqsubA, sqsumA);
	
	fld_sub(T1, sqsumA, sqsubA);
	fld_scale(T2, T1, 121665);
	fld_add(T2, T2, sqsumA);
	fld_mul(A->z, T1, T2);

	/* calculate A + B */
	fld_add(sumB, B->x, B->z);
	fld_sub(subB, B->x, B->z);

	fld_mul(T1, subA, sumB);
	fld_mul(T2, sumA, subB);
	
	fld_add(T3, T1, T2);
	fld_sq(B->x, T3);

	fld_sub(T3, T1, T2);
	fld_sq(T3, T3);
	fld_mul(B->z, T3, C->x);
}


/*
 * mg_scale - calculates x * P with montgomery formula.
 *
 * assumes:
 *   out != P
 *   P->z must be 1
 */
static void
mg_scale(struct mg *out, const struct mg *P, const uint8_t x[32])
{
	struct mg T;
	int8_t foo;
	int i, j;

	fld_set0(out->x, 1);
	fld_set0(out->z, 0);
	memcpy(&T, P, sizeof(struct mg));
	
	for (i = 31; i >= 0; i--) {
		foo = x[i];
		for (j = 8; j > 0; j--, foo <<= 1) {
			ctmemswap(out, &T, sizeof(struct mg), foo >> 7);
			montgomery(out, &T, P);
			ctmemswap(out, &T, sizeof(struct mg), foo >> 7);
		}
	}
}

/*
 * mg_dh - calculates diffie-hellman function
 */
static void
mg_dh(uint8_t out[32], const uint8_t sec[32], const uint8_t base[32])
{
	struct mg res, P;
	uint8_t s[32];

	memcpy(s, sec, 32);
	s[0] &= 0xf8;
	s[31] &= 0x7f;
	s[31] |= 0x40;
 	
	fld_import(P.x, base);
	fld_set0(P.z, 1);
	
	mg_scale(&res, &P, s);

	fld_inv(res.z, res.z);
	fld_mul(res.x, res.x, res.z);
	fld_export(out, res.x);
}


/*
 * DH - stack-cleanup wrapper for mg_dh
 */
void
DH(uint8_t out[32], const uint8_t sec[32], const uint8_t base[32])
{
	mg_dh(out, sec, base);
	burnstack(65536);
}
