/*
 * implementing ed25519-sha-512 from [1].
 *
 * This code is public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>
 *
 *
 * References:
 * [1] High-speed high-security signatures, 2011/09/26,
 *     Bernstein, Duif, Lange, Schwabe, Yang
 *
 * TODO:
 *   - batch verify
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <eddsa.h>

#include "sha512.h"
#include "sc.h"
#include "fld.h"
#include "ed.h"
#include "burnstack.h"


/*
 * genpub - derive public key from secret key
 */
static void
genpub(uint8_t pub[32], const uint8_t sec[32])
{
	sha512ctx hash;
	uint8_t h[64];
	struct ed A;
	sc_t a;

	/* hash secret-key */
	sha512_init(&hash);
	sha512_add(&hash, sec, 32);
	sha512_done(&hash, h);
	
	/* delete bit 255 and set bit 254 */
	h[31] &= 0x7f;
	h[31] |= 0x40;
	/* delete 3 lowest bits */
	h[0] &= 0xf8;
	
	/* now the first 256bit of h form our DH secret a */
	sc_import(a, h, 32);

	/* calculate public key */
	ed_scale_base(&A, a);
	ed_export(pub, &A);
}

/*
 * eddsa_genpub - stack-clearing wrapper for genpub
 */
void
eddsa_genpub(uint8_t pub[32], const uint8_t sec[32])
{
	genpub(pub, sec);
	burnstack(65536);
}


/*
 * sign - create ed25519 signature of data using secret key sec
 */
static void
sign(uint8_t sig[64], const uint8_t sec[32], const uint8_t pub[32], const uint8_t *data, size_t len)
{
	sha512ctx hash;
	uint8_t h[64];
	
	sc_t a, r, t, S;
	struct ed R;

	/* hash secret key */
	sha512_init(&hash);
	sha512_add(&hash, sec, 32);
	sha512_done(&hash, h);

	/* derive DH secret a */
	h[31] &= 0x7f;
	h[31] |= 0x40;
	h[0] &= 0xf8;
	sc_import(a, h, 32);

	/* hash next 32 bytes together with data to form r */
	sha512_init(&hash);
	sha512_add(&hash, h+32, 32);
	sha512_add(&hash, data, len);
	sha512_done(&hash, h);
	sc_import(r, h, 64);

	/* calculate R = r * B which form the first 256bit of the signature */
	ed_scale_base(&R, r);
	ed_export(sig, &R);
	
	/* calculate t := Hash(export(R), export(A), data) mod m */
	sha512_init(&hash);
	sha512_add(&hash, sig, 32);
	sha512_add(&hash, pub, 32);
	sha512_add(&hash, data, len);
	sha512_done(&hash, h);
	sc_import(t, h, 64);
	
	/* calculate S := r + t*a mod m and finish the signature */
	sc_mul(S, t, a);
	sc_add(S, r, S);
	sc_export(sig+32, S);
}

/*
 * eddsa_sign - stack-cleaning wrapper for sign
 */
void
eddsa_sign(uint8_t sig[64], const uint8_t sec[32], const uint8_t pub[32], const uint8_t *data, size_t len)
{
	sign(sig, sec, pub, data, len);
	burnstack(65536);
}


/*
 * verify - verifies an ed25519 signature of given data. (vartime)
 *
 * return 0 if signature is ok and -1 otherwise.
 */
static bool
verify(const uint8_t sig[64], const uint8_t pub[32], const uint8_t *data, size_t len)
{
	sha512ctx hash;
	uint8_t h[64];
	struct ed A, C;
	sc_t t, S;
	uint8_t check[32];

	/* import public key */
	ed_import(&A, pub);

	/* import S from second half of the signature */
	sc_import(S, sig+32, 32);

	/* calculate t := Hash(export(R), export(A), data) mod m */
	sha512_init(&hash);
	sha512_add(&hash, sig, 32);
	sha512_add(&hash, pub, 32);
	sha512_add(&hash, data, len);
	sha512_done(&hash, h);
	sc_import(t, h, 64);

	/* verify signature (vartime) */
	fld_neg(A.x, A.x);
	fld_neg(A.t, A.t);
	ed_double_scale(&C, S, &ed_BP, t, &A);
	ed_export(check, &C);
	
	/* is export(C) == export(R) (vartime) */
	if (memcmp(check, sig, 32) == 0)
		return true;

	return false;
}

/*
 * eddsa_verify - stack-burn-wrapper for verify
 */
bool
eddsa_verify(const uint8_t sig[64], const uint8_t pub[32], const uint8_t *data, size_t len)
{
	bool rval;
	rval = verify(sig, pub, data, len);
	burnstack(65536);
	return rval;
}
