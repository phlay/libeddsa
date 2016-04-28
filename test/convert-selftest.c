#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <eddsa.h>


int main()
{
	const uint8_t B[32] = { 9 };
	uint8_t edsk[32], edpk[32];
	uint8_t dhsk[32], dhpk[32];
	uint8_t check[32];
	unsigned int i, j;

	srand(0);

	for (i = 0; i < 1024; i++) {
		/* use pseudo-random for test keys (DO NOT DO THIS FOR REAL!) */
		for (j = 0; j < 32; j++)
			edsk[j] = (uint8_t)rand();

		/* generate eddsa public key */
		eddsa_genpub(edpk, edsk);

		/* convert secret key from eddsa to dh */
		eddsa_sk_eddsa_to_dh(dhsk, edsk);

		/* generate dh public component */
		DH(dhpk, dhsk, B);

		/* derive dh public component from eddsa public key */
		eddsa_pk_eddsa_to_dh(check, edpk);

		/* now check if everything worked out */
		if (memcmp(check, dhpk, 32) != 0) {
			fprintf(stderr, "convert-selftest: key conversion does not commutate!\n");
			return 1;
		}
	}


	return 0;
}
