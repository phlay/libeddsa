#ifndef SC_H
#define SC_H

#include <stddef.h>
#include <stdint.h>

#include "bitness.h"
#include "compat.h"
#include "limb.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_64BIT

#define SC_LIMB_NUM	5
#define SC_LIMB_BITS	52

#define SC_LIMB_MASK	((1L << SC_LIMB_BITS)-1)

#else

#define SC_LIMB_NUM	10
#define SC_LIMB_BITS	26

#define SC_LIMB_MASK	((1 << SC_LIMB_BITS)-1)

#endif


#define SC_BITS		(SC_LIMB_NUM * SC_LIMB_BITS)


/* sc_t holds 260bit in reduced form */
typedef limb_t sc_t[SC_LIMB_NUM];

/* lsc_t is double in size and holds up to 520bits in reduced form */
typedef limb_t lsc_t [2*SC_LIMB_NUM];



extern const sc_t con_off;


void	sc_reduce(sc_t dst, const lsc_t src);
void	sc_import(sc_t dst, const uint8_t *src, size_t len);
void	sc_export(uint8_t dst[32], const sc_t x);
void	sc_mul(sc_t res, const sc_t a, const sc_t b);
int	sc_jsf(int u0[SC_BITS+1], int u1[SC_BITS+1], const sc_t a, const sc_t b);


static INLINE void
sc_add(sc_t res, const sc_t a, const sc_t b)
{
	int i;
	for (i = 0; i < SC_LIMB_NUM; i++)
		res[i] = a[i] + b[i];
}

#ifdef __cplusplus
}
#endif
#endif
