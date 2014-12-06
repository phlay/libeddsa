#ifndef BURN_H
#define BURN_H

#include <stddef.h>

#include "compat.h"

#ifdef USE_STACKCLEAN

void	burn(void *s, size_t n);

#else

static INLINE void burn(void *s, size_t n) { }

#endif

#endif
