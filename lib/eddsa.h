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
		   const uint8_t base[32]);


#endif
