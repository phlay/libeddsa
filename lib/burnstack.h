#ifndef BURNSTACK_H
#define BURNSTACK_H

#include "compat.h"

#ifdef USE_STACKCLEAN

void burnstack(int len);

#else

static INLINE void burnstack(int len) { (void)len; }

#endif

#endif
