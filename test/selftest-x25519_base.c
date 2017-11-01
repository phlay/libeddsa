#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <eddsa.h>

#define TESTNUM		1024


int main()
{
	const uint8_t BP[X25519_KEY_LEN] = { 9 };

	uint8_t x[X25519_KEY_LEN];

	uint8_t result[X25519_KEY_LEN];
	uint8_t check[X25519_KEY_LEN];

	unsigned int i, j;

	srand(0);

	for (i = 0; i < TESTNUM; i++) {
		/* use pseudo-random for generating test scalars.
		 * please never use something like that for key generation!
		 */
		for (j = 0; j < X25519_KEY_LEN; j++)
			x[j] = (uint8_t)rand();


		/* calculate x * BP */
		x25519_base(result, x);

		/* calculate x * BP with x25519() to check result */
		x25519(check, x, BP);

		if (memcmp(result, check, X25519_KEY_LEN) != 0) {
			fprintf(stderr, "x25519-base-selftest: x25519_base differs from x25519!\n");
			return 1;
		}
	}

	return 0;
}
