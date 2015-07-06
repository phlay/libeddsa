#
# These tests could only be used with a static library build
# (because they test non-exported functions of the library).
#
include_directories("../lib")

if (BUILD_STATIC)
	add_executable(sha256-selftest sha256-selftest.c)
	target_link_libraries(sha256-selftest eddsa-static)
	add_test(NAME test-sha256 COMMAND sha256-selftest)

	add_executable(sha512-selftest sha512-selftest.c)
	target_link_libraries(sha512-selftest eddsa-static)
	add_test(NAME test-sha512 COMMAND sha512-selftest)
endif ()


#
# Test exported functions
#
add_executable(eddsa-selftest eddsa-selftest.c)
add_executable(dh-selftest dh-selftest.c)

if (BUILD_STATIC)
        target_link_libraries(eddsa-selftest eddsa-static)
        target_link_libraries(dh-selftest eddsa-static)
else ()
        target_link_libraries(eddsa-selftest eddsa)
        target_link_libraries(dh-selftest eddsa)
endif ()

add_test(NAME test-eddsa COMMAND eddsa-selftest)
add_test(NAME test-dh COMMAND dh-selftest)
