include(CheckCCompilerFlag)

cmake_minimum_required(VERSION 2.8)

project(eddsa C)

set(EDDSA_VERSION_MAJOR 0)
set(EDDSA_VERSION_MINOR 2)


option(USE_STACKCLEAN "clean all secret variables from stack" ON)
option(BUILD_STATIC "build static version of library" ON)
option(BUILD_TESTING "build test" OFF)


if (UNIX)
	set(CMAKE_C_FLAGS "-std=c99 -fwrapv -Wall -pedantic -O3")
endif ()


# does our compiler have hidden-visibility feature?
#
if (NOT (CMAKE_COMPILER_IS_GNUCC AND CMAKE_C_COMPILER_VERSION VERSION_LESS "4.2")
    AND NOT WIN32 
    AND NOT CYGWIN)
  check_c_compiler_flag("-fvisibility=hidden" HAVE_VISIBILITY)
endif ()



# check platform bitness
#
set(BITNESS 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(BITNESS 64)
endif()


# add library source code
#
add_subdirectory(lib)


# add test source if requested
#
if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif ()


MESSAGE("bitness: " ${BITNESS})
MESSAGE("cleanup stack: " ${USE_STACKCLEAN})
MESSAGE("build test: " ${BUILD_TESTING})
