#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eddsa.h>

struct {
	uint8_t		base[32];
	uint8_t		scale[32];
	uint8_t		result[32];
} table[] = {
	#include "dh-table.h"
};

const int table_num = sizeof(table) / sizeof(table[0]);


int
main()
{
	uint8_t check[32];
	int i;

	for (i = 0; i < table_num; i++) {
		DH(check, table[i].scale, table[i].base);
		if (memcmp(check, table[i].result, 32) != 0) {
			fprintf(stderr, "dh-selftest: test number %d failed\n", i+1);
			return 1;
		}
	}

	return 0;
}
