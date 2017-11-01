include_directories("../lib")

#
# Selftests against dynamic library
#
add_executable(selftest-ed25519 selftest-ed25519.c)
add_executable(selftest-x25519 selftest-x25519.c)
add_executable(selftest-x25519_base selftest-x25519_base.c)
add_executable(selftest-convert selftest-convert.c)

target_link_libraries(selftest-ed25519 eddsa)
target_link_libraries(selftest-x25519 eddsa)
target_link_libraries(selftest-x25519_base eddsa)
target_link_libraries(selftest-convert eddsa)


add_test(NAME test-ed25519 COMMAND selftest-ed25519)
add_test(NAME test-x25519 COMMAND selftest-x25519)
add_test(NAME test-x25519_base COMMAND selftest-x25519_base)
add_test(NAME test-convert COMMAND selftest-convert)

#
# Build selftests against static library.
#
# Please note, that some tests could only be done here, because they
# test non-exported functions of the library.
#
if (BUILD_STATIC)
	add_executable(selftest-static-sha512 selftest-sha512.c)
	add_executable(selftest-static-ed25519 selftest-ed25519.c)
	add_executable(selftest-static-x25519 selftest-x25519.c)
	add_executable(selftest-static-x25519_base selftest-x25519_base.c)
	add_executable(selftest-static-convert selftest-convert.c)

	target_link_libraries(selftest-static-sha512 eddsa-static)
        target_link_libraries(selftest-static-ed25519 eddsa-static)
        target_link_libraries(selftest-static-x25519 eddsa-static)
	target_link_libraries(selftest-static-x25519_base eddsa-static)
	target_link_libraries(selftest-static-convert eddsa-static)

	add_test(NAME test-static-sha512 COMMAND selftest-static-sha512)
	add_test(NAME test-static-ed25519 COMMAND selftest-static-ed25519)
	add_test(NAME test-static-x25519 COMMAND selftest-static-x25519)
	add_test(NAME test-static-x25519_base COMMAND selftest-static-x25519_base)
	add_test(NAME test-static-convert COMMAND selftest-static-convert)
endif ()
