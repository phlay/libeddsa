/*
 * implementing ed25519-sha512 from [1].
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

#include "eddsa.h"

#include "sha512.h"
#include "sc.h"
#include "fld.h"
#include "ed.h"
#include "burnstack.h"


static void
ed25519_key_setup(uint8_t out[SHA512_HASH_LENGTH],
		  const uint8_t sk[ED25519_KEY_LEN])
{
	struct sha512 hash;

	/* hash secret-key */
	sha512_init(&hash);
	sha512_add(&hash, sk, ED25519_KEY_LEN);
	sha512_final(&hash, out);

	/* delete bit 255 and set bit 254 */
	out[31] &= 0x7f;
	out[31] |= 0x40;
	/* delete 3 lowest bits */
	out[0] &= 0xf8;
}


/*
 * genpub - derive public key from secret key
 */
static void
genpub(uint8_t pub[ED25519_KEY_LEN], const uint8_t sec[ED25519_KEY_LEN])
{
	uint8_t h[SHA512_HASH_LENGTH];
	struct ed A;
	sc_t a;

	/* derive secret and import it */
	ed25519_key_setup(h, sec);
	sc_import(a, h, 32);

	/* multiply with base point to calculate public key */
	ed_scale_base(&A, a);
	ed_export(pub, &A);
}


/*
 * ed25519_genpub - stack-clearing wrapper for genpub
 */
void
ed25519_genpub(uint8_t pub[ED25519_KEY_LEN], const uint8_t sec[ED25519_KEY_LEN])
{
	genpub(pub, sec);
	burnstack(2048);
}


/*
 * sign - create ed25519 signature of data using secret key sec
 */
static void
sign(uint8_t sig[ED25519_SIG_LEN],
     const uint8_t sec[ED25519_KEY_LEN],
     const uint8_t pub[ED25519_KEY_LEN],
     const uint8_t *data, size_t len)
{
	struct sha512 hash;
	uint8_t h[SHA512_HASH_LENGTH];
	
	sc_t a, r, t, S;
	struct ed R;

	/* derive secret scalar a */
	ed25519_key_setup(h, sec);
	sc_import(a, h, 32);

	/* hash next 32 bytes together with data to form r */
	sha512_init(&hash);
	sha512_add(&hash, h+32, 32);
	sha512_add(&hash, data, len);
	sha512_final(&hash, h);
	sc_import(r, h, sizeof(h));

	/* calculate R = r * B which form the first 256bit of the signature */
	ed_scale_base(&R, r);
	ed_export(sig, &R);
	
	/* calculate t := Hash(export(R), export(A), data) mod m */
	sha512_init(&hash);
	sha512_add(&hash, sig, 32);
	sha512_add(&hash, pub, 32);
	sha512_add(&hash, data, len);
	sha512_final(&hash, h);
	sc_import(t, h, sizeof(h));
	
	/* calculate S := r + t*a mod m and finish the signature */
	sc_mul(S, t, a);
	sc_add(S, r, S);
	sc_export(sig+32, S);
}


/*
 * ed25519_sign - stack-cleaning wrapper for sign
 */
void
ed25519_sign(uint8_t sig[ED25519_SIG_LEN],
	   const uint8_t sec[ED25519_KEY_LEN],
	   const uint8_t pub[ED25519_KEY_LEN],
	   const uint8_t *data, size_t len)
{
	sign(sig, sec, pub, data, len);
	burnstack(4096);
}


/*
 * ed25519_verify - verifies an ed25519-signature of given data.
 *
 * note: this functions runs in vartime and does no stack cleanup, since
 * all information are considered public.
 *
 * returns true if signature is ok and false otherwise.
 */
bool
ed25519_verify(const uint8_t sig[ED25519_SIG_LEN],
	       const uint8_t pub[ED25519_KEY_LEN],
	       const uint8_t *data, size_t len)
{
	struct sha512 hash;
	uint8_t h[SHA512_HASH_LENGTH];
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
	sha512_final(&hash, h);
	sc_import(t, h, 64);

	/* verify signature (vartime!) */
	fld_neg(A.x, A.x);
	fld_neg(A.t, A.t);
	ed_dual_scale(&C, S, t, &A);
	ed_export(check, &C);
	
	/* is export(C) == export(R) (vartime!) */
	return (memcmp(check, sig, 32) == 0);
}


/*
 * pk_ed25519_to_x25519 - convert a ed25519 public key to x25519
 */
void
pk_ed25519_to_x25519(uint8_t out[X25519_KEY_LEN], const uint8_t in[ED25519_KEY_LEN])
{
	struct ed P;
	fld_t u, t;

	/* import ed25519 public key */
	ed_import(&P, in);

	/*
	 * We now have the point P = (x,y) on the curve
	 *
	 * 	x^2 + y^2 = 1 + (121665/121666)x^2y^2
	 *
	 * and want the u-component of the corresponding point
	 * on the birationally equivalent montgomery curve
	 *
	 * 	v^2 = u^3 + 486662 u^2 + u.
	 *
	 *
	 * From the paper [1] we get
	 *
	 * 	y = (u - 1) / (u + 1),
	 *
	 * which immediately yields
	 *
	 * 	u = (1 + y) / (1 - y)
	 *
	 * or, by using projective coordinantes,
	 *
	 * 	u = (z + y) / (z - y).
	 */

	/* u <- z + y */
	fld_add(u, P.z, P.y);

	/* t <- (z - y)^-1 */
	fld_sub(t, P.z, P.y);
	fld_inv(t, t);

	/* u <- u * t = (z+y) / (z-y) */
	fld_mul(u, u, t);

	/* export curve25519 public key */
	fld_export(out, u);
}



/*
 * conv_sk_ed25519_to_x25519 - convert a ed25519 secret key to x25519 secret.
 */
static void
conv_sk_ed25519_to_x25519(uint8_t out[X25519_KEY_LEN], const uint8_t in[ED25519_KEY_LEN])
{
	uint8_t h[SHA512_HASH_LENGTH];
	ed25519_key_setup(h, in);
	memcpy(out, h, X25519_KEY_LEN);
}


/*
 * sk_ed25519_to_x25519 - stack-clearing wrapper for conv_sk_ed25519_to_x25519.
 */
void
sk_ed25519_to_x25519(uint8_t out[X25519_KEY_LEN], const uint8_t in[ED25519_KEY_LEN])
{
	conv_sk_ed25519_to_x25519(out, in);
	burnstack(1024);
}





/*
 * Obsolete Interface, this will be removed in the future.
 */


/*
 * eddsa_genpub - stack-clearing wrapper for genpub (obsolete interface!)
 */
void
eddsa_genpub(uint8_t pub[32], const uint8_t sec[32])
{
	genpub(pub, sec);
	burnstack(2048);
}


/*
 * eddsa_sign - stack-cleaning wrapper for sign (obsolete interface!)
 */
void
eddsa_sign(uint8_t sig[ED25519_SIG_LEN],
	   const uint8_t sec[ED25519_KEY_LEN],
	   const uint8_t pub[ED25519_KEY_LEN],
	   const uint8_t *data, size_t len)
{
	sign(sig, sec, pub, data, len);
	burnstack(4096);
}

/*
 * eddsa_verify - verifies an ed25519 signature of given data.
 * (obsolete interface!)
 *
 */
bool
eddsa_verify(const uint8_t sig[ED25519_SIG_LEN],
	     const uint8_t pub[ED25519_KEY_LEN],
	     const uint8_t *data, size_t len)
{
	return ed25519_verify(sig, pub, data, len);
}

/*
 * eddsa_pk_eddsa_to_dh - convert a ed25519 public key to x25519.
 * (obsolete interface!)
 */
void
eddsa_pk_eddsa_to_dh(uint8_t out[X25519_KEY_LEN], const uint8_t in[ED25519_KEY_LEN])
{
	pk_ed25519_to_x25519(out, in);
}


/*
 * eddsa_sk_eddsa_to_dh - convert a ed25519 secret key to x25519 secret.
 * (obsolete interface!)
 */
void
eddsa_sk_eddsa_to_dh(uint8_t out[X25519_KEY_LEN], const uint8_t in[ED25519_KEY_LEN])
{
	conv_sk_ed25519_to_x25519(out, in);
	burnstack(1024);
}
