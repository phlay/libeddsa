#ifndef RIDDLER_H
#define RIDDLER_H

#include <stddef.h>	/* for size_t */
#include <stdint.h>	/* for uint8_t */

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef RIDDLER_STATIC
#  if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef RIDDLER_BUILD
#      define RIDDLER_DECL __declspec(dllexport)
#    else
#      define RIDDLER_DECL __declspec(dllimport)
#    endif
#  elif defined(RIDDLER_BUILD) && defined(__GNUC__) && __GNUC__ >= 4
#    define RIDDLER_DECL __attribute__((visibility ("default")))
#  elif defined(RIDDLER_BUILD) && defined(__CLANG__) && __has_attribute(visibility)
#    define RIDDLER_DECL __attribute__((visibility ("default")))
#  endif
#endif

#ifndef RIDDLER_DECL
#define RIDDLER_DECL
#endif


/* eddsa */
RIDDLER_DECL void	eddsa_genpub(uint8_t pub[32], const uint8_t sec[32]);
RIDDLER_DECL void	eddsa_sign(uint8_t sig[64],
				   const uint8_t sec[32],
				   const uint8_t pub[32],
				   const uint8_t *data, size_t len);
RIDDLER_DECL int	eddsa_verify(const uint8_t sig[64],
				     const uint8_t pub[32],
				     const uint8_t *data, size_t len);

/* diffie-hellman */
RIDDLER_DECL void	DH(uint8_t out[32], const uint8_t sec[32],
			   const uint8_t base[32]);


#endif
