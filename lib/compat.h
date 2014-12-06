#ifndef COMPAT_H
#define COMPAT_H

/*
 * macro for inline-functions to make visual c happy :-\
 */

#ifdef _MSC_VER
#define INLINE __inline
#else
#define INLINE inline
#endif

#endif
