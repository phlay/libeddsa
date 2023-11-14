include(CheckCCompilerFlag)
include(CheckPrototypeDefinition)

cmake_minimum_required(VERSION 3.0.0)

project(eddsa C)

set(EDDSA_VERSION_MAJOR 0)
set(EDDSA_VERSION_MINOR 8)


option(USE_STACKCLEAN "clean all secret variables from stack" ON)
option(BUILD_STATIC "build static version of library" ON)
option(BUILD_TESTING "build test" ON)


if (UNIX)
	set(CMAKE_C_FLAGS "-std=c99 -fwrapv -Wall -Wextra -pedantic -O3")
endif ()

# check for memset_s and co
#
check_prototype_definition(memset_s
	"errno_t memset_s( void *dest, rsize_t destsz, int ch, rsize_t count )"
	"0"
	"string.h"
	HAVE_MEMSET_S)

check_prototype_definition(explicit_bzero
	"void explicit_bzero(void *b, size_t len)"
	""
	"string.h"
	HAVE_EXPLICIT_BZERO)


# does our compiler have hidden-visibility feature?
#
if (NOT (CMAKE_COMPILER_IS_GNUCC AND CMAKE_C_COMPILER_VERSION VERSION_LESS "4.2")
    AND NOT WIN32 
    AND NOT CYGWIN)
  check_c_compiler_flag("-fvisibility=hidden" HAVE_VISIBILITY)
endif ()



# add library source code
#
add_subdirectory(lib)


# add test source if requested
#
if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif ()

if (NOT BITNESS)
  set(BITNESS autodetect)
endif ()


MESSAGE("bitness: " ${BITNESS})
MESSAGE("cleanup stack: " ${USE_STACKCLEAN})
MESSAGE("build test: " ${BUILD_TESTING})
