#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eddsa.h>

struct {
	uint8_t		point[X25519_KEY_LEN];
	uint8_t		scalar[X25519_KEY_LEN];
	uint8_t		result[X25519_KEY_LEN];
} table[] = {
	#include "x25519-table.h"
};

const int table_num = sizeof(table) / sizeof(table[0]);


int
main()
{
	uint8_t check[X25519_KEY_LEN];
	int i;

	/*
	 * run x25519 tests against table
	 */
	for (i = 0; i < table_num; i++) {
		x25519(check, table[i].scalar, table[i].point);
		if (memcmp(check, table[i].result, X25519_KEY_LEN) != 0) {
			fprintf(stderr, "dh-selftest: test number %d failed\n", i+1);
			return 1;
		}
	}


	/*
	 * test old interface
	 */
	for (i = 0; i < table_num; i++) {
		DH(check, table[i].scalar, table[i].point);
		if (memcmp(check, table[i].result, X25519_KEY_LEN) != 0) {
			fprintf(stderr, "dh-selftest: (OLD API) test number %d failed\n", i+1);
			return 1;
		}
	}

	return 0;
}
