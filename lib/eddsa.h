#ifndef EDDSA_H
#define EDDSA_H

#include <stddef.h>	/* for size_t */
#include <stdbool.h>	/* foor bool */
#include <stdint.h>	/* for uint8_t */

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef EDDSA_STATIC
#  if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef EDDSA_BUILD
#      define EDDSA_DECL __declspec(dllexport)
#    else
#      define EDDSA_DECL __declspec(dllimport)
#    endif
#  elif defined(EDDSA_BUILD) && defined(__GNUC__) && __GNUC__ >= 4
#    define EDDSA_DECL __attribute__((visibility ("default")))
#  elif defined(EDDSA_BUILD) && defined(__CLANG__) && __has_attribute(visibility)
#    define EDDSA_DECL __attribute__((visibility ("default")))
#  endif
#endif

#ifndef EDDSA_DECL
#define EDDSA_DECL
#endif


#ifdef __cplusplus
extern "C" {
#endif



/*
 * Ed25519 DSA
 */

#define ED25519_KEY_LEN		32
#define ED25519_SIG_LEN		64

EDDSA_DECL void	ed25519_genpub(uint8_t pub[ED25519_KEY_LEN],
			       const uint8_t sec[ED25519_KEY_LEN]);

EDDSA_DECL void	ed25519_sign(uint8_t sig[ED25519_SIG_LEN],
			     const uint8_t sec[ED25519_KEY_LEN],
			     const uint8_t pub[ED25519_KEY_LEN],
			     const uint8_t *data, size_t len);

EDDSA_DECL bool	ed25519_verify(const uint8_t sig[ED25519_SIG_LEN],
			       const uint8_t pub[ED25519_KEY_LEN],
			       const uint8_t *data, size_t len);



/*
 * X25519 Diffie-Hellman
 */

#define X25519_KEY_LEN		32

EDDSA_DECL void	x25519_base(uint8_t out[X25519_KEY_LEN],
			    const uint8_t scalar[X25519_KEY_LEN]);

EDDSA_DECL void	x25519(uint8_t out[X25519_KEY_LEN],
		       const uint8_t scalar[X25519_KEY_LEN],
		       const uint8_t point[X25519_KEY_LEN]);



/*
 * Key-conversion between ed25519 and x25519
 */

EDDSA_DECL void pk_ed25519_to_x25519(uint8_t out[X25519_KEY_LEN],
				     const uint8_t in[ED25519_KEY_LEN]);

EDDSA_DECL void sk_ed25519_to_x25519(uint8_t out[X25519_KEY_LEN],
				     const uint8_t in[ED25519_KEY_LEN]);





/*
 * Obsolete Interface
 */

/* eddsa */
EDDSA_DECL void	eddsa_genpub(uint8_t pub[32], const uint8_t sec[32]);

EDDSA_DECL void	eddsa_sign(uint8_t sig[64],
			   const uint8_t sec[32],
			   const uint8_t pub[32],
			   const uint8_t *data, size_t len);

EDDSA_DECL bool	eddsa_verify(const uint8_t sig[64],
			     const uint8_t pub[32],
			     const uint8_t *data, size_t len);


/* diffie-hellman */
EDDSA_DECL void	DH(uint8_t out[32], const uint8_t sec[32],
		   const uint8_t point[32]);


/* key conversion */
EDDSA_DECL void eddsa_pk_eddsa_to_dh(uint8_t out[32],
				     const uint8_t in[32]);

EDDSA_DECL void eddsa_sk_eddsa_to_dh(uint8_t out[32],
				     const uint8_t in[32]);


#ifdef __cplusplus
}
#endif


#endif
