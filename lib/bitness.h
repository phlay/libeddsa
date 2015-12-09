#ifndef BITNESS_H
#define BITNESS_H

#ifndef NO_AUTO_BITNESS
  #ifdef __LP64__
    #define USE_64BIT	1
  #else
    #undef USE_64BIT
  #endif
#endif

#endif
