#ifndef BURN_H
#define BURN_H

#include <stddef.h>

#include "compat.h"


#if defined(HAVE_MEMSET_S)

#include <string.h>
static INLINE void burn(void *dest, size_t len) { memset_s(dest, len, 0, len); }

#elif defined(HAVE_EXPLICIT_BZERO)

#include <string.h>
static INLINE void burn(void *dest, size_t len) { explicit_bzero(dest, len); }

#else

void	burn(void *dest, size_t len);

#endif


#endif
