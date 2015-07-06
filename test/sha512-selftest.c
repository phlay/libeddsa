#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eddsa.h>

#include "sha512.h"

#define BUFLEN		(16*1024)

struct {
	int		len;
	uint8_t		buffer[BUFLEN];
	uint8_t		hash[64];
} table[] = {
	#include "sha512-table.h"
};

const int table_num = sizeof(table) / sizeof(table[0]);



int main()
{
	sha512ctx hctx;
	uint8_t checkhash[64];
	int i;

	for (i = 0; i < table_num; i++) {
		sha512_init(&hctx);
		sha512_add(&hctx, table[i].buffer, table[i].len);
		sha512_done(&hctx, checkhash);

		if (memcmp(checkhash, table[i].hash, 32) != 0) {
			fprintf(stderr, "sha512-selftest: can't verify hash number %d\n", i+1);
			return 1;
		}
	}

	return 0;
}
