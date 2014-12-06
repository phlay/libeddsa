/*
 * common code (32bit and 64bit) for GF(2^255-19).
 *
 * This code is in public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>
 */

#include "fld.h"


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
