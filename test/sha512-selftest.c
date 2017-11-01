#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eddsa.h>

#include "sha512.h"

#define BUFLEN		(16*1024)

struct {
	int		len;
	uint8_t		buffer[BUFLEN];
	uint8_t		hash[SHA512_HASH_LENGTH];
} table[] = {
	#include "sha512-table.h"
};

const int table_num = sizeof(table) / sizeof(table[0]);



int main()
{
	struct sha512 h;
	uint8_t checkhash[SHA512_HASH_LENGTH];
	int i;

	for (i = 0; i < table_num; i++) {
		sha512_init(&h);
		sha512_add(&h, table[i].buffer, table[i].len);
		sha512_final(&h, checkhash);

		if (memcmp(checkhash, table[i].hash, SHA512_HASH_LENGTH) != 0) {
			fprintf(stderr, "sha512-selftest: can't verify hash number %d\n", i+1);
			return 1;
		}
	}

	return 0;
}
