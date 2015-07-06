#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eddsa.h>


struct {
	uint8_t		sec[32];
	uint8_t		pub[32];
	uint8_t		sig[64];
	uint8_t		msg[1024];
} table[] = {
	#include "eddsa-table.h"
};

const int table_num = sizeof(table) / sizeof(table[0]);



int main()
{
	uint8_t checkpub[32];
	uint8_t checksig[64];

	int i;

	for (i = 0; i < table_num; i++) {
		/* check one: do we generate the same public key? */
		eddsa_genpub(checkpub, table[i].sec);
		if (memcmp(checkpub, table[i].pub, 32) != 0) {
			fprintf(stderr, "eddsa-selftest: generating public key number %d failed\n", i+1);
			return 1;
		}

		/* check two: do we make the same signature? */
		eddsa_sign(checksig, table[i].sec, table[i].pub, table[i].msg, i);
		if (memcmp(checksig, table[i].sig, 64) != 0) {
			fprintf(stderr, "eddsa-selftest: generating signature number %d failed\n", i+1);
			return 1;
		}

		/* check three: does signature get verified? */
		if (!eddsa_verify(table[i].sig, table[i].pub, table[i].msg, i)) {
			fprintf(stderr, "eddsa-selftest: verifying signature number %d failed\n", i+1);
			return 1;
		}
	}

	return 0;
}
