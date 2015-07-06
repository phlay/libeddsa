#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eddsa.h>

#include "sha256.h"

#define BUFLEN		(16*1024)

struct {
	int		len;
	uint8_t		buffer[BUFLEN];
	uint8_t		hash[32];
} table[] = {
	#include "sha256-table.h"
};

const int table_num = sizeof(table) / sizeof(table[0]);



int main()
{
	sha256ctx hctx;
	uint8_t checkhash[32];
	int i;

	for (i = 0; i < table_num; i++) {
		sha256_init(&hctx);
		sha256_add(&hctx, table[i].buffer, table[i].len);
		sha256_done(&hctx, checkhash);

		if (memcmp(checkhash, table[i].hash, 32) != 0) {
			fprintf(stderr, "sha256-selftest: can't verify hash number %d\n", i+1);
			return 1;
		}
	}

	return 0;
}
