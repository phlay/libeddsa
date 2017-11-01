include_directories("../lib")

#
# Selftests against dynamic library
#
add_executable(eddsa-selftest eddsa-selftest.c)
add_executable(dh-selftest dh-selftest.c)
add_executable(x25519_base-selftest x25519_base-selftest.c)
add_executable(convert-selftest convert-selftest.c)

target_link_libraries(eddsa-selftest eddsa)
target_link_libraries(dh-selftest eddsa)
target_link_libraries(x25519_base-selftest eddsa)
target_link_libraries(convert-selftest eddsa)


add_test(NAME test-eddsa COMMAND eddsa-selftest)
add_test(NAME test-dh COMMAND dh-selftest)
add_test(NAME test-x25519_base COMMAND x25519_base-selftest)
add_test(NAME test-convert COMMAND convert-selftest)

#
# Build selftests against static library.
#
# Please note, that some tests could only be done here, because they
# test non-exported functions of the library.
#
if (BUILD_STATIC)
	add_executable(sha512-static-selftest sha512-selftest.c)
	add_executable(eddsa-static-selftest eddsa-selftest.c)
	add_executable(dh-static-selftest dh-selftest.c)
	add_executable(x25519_base-static-selftest x25519_base-selftest.c)
	add_executable(convert-static-selftest convert-selftest.c)

	target_link_libraries(sha512-static-selftest eddsa-static)
        target_link_libraries(eddsa-static-selftest eddsa-static)
        target_link_libraries(dh-static-selftest eddsa-static)
	target_link_libraries(x25519_base-static-selftest eddsa-static)
	target_link_libraries(convert-static-selftest eddsa-static)

	add_test(NAME test-sha512-static COMMAND sha512-static-selftest)
	add_test(NAME test-eddsa-static COMMAND eddsa-static-selftest)
	add_test(NAME test-dh-static COMMAND dh-static-selftest)
	add_test(NAME test-x25519_base-static COMMAND x25519_base-static-selftest)
	add_test(NAME test-convert-static COMMAND convert-static-selftest)
endif ()
