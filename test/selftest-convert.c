#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <eddsa.h>


int main()
{
	const uint8_t B[X25519_KEY_LEN] = { 9 };
	uint8_t edsk[ED25519_KEY_LEN], edpk[ED25519_KEY_LEN];
	uint8_t dhsk[X25519_KEY_LEN], dhpk[X25519_KEY_LEN];
	uint8_t check[X25519_KEY_LEN];
	unsigned int i, j;

	/*
	 * test ed25519 to x25519 key conversion functions
	 */

	srand(0);

	for (i = 0; i < 1024; i++) {
		/* use pseudo-random for test keys (DO NOT DO THIS FOR REAL!) */
		for (j = 0; j < ED25519_KEY_LEN; j++)
			edsk[j] = (uint8_t)rand();

		/* generate ed25519 public key */
		ed25519_genpub(edpk, edsk);

		/* convert secret key from ed25519 to x25519 */
		sk_ed25519_to_x25519(dhsk, edsk);

		/* convert public key from ed25519 to x25519 */
		pk_ed25519_to_x25519(dhpk, edpk);

		/* generate x25519 public component to check */
		x25519_base(check, dhsk);

		/* now check if everything worked out */
		if (memcmp(check, dhpk, X25519_KEY_LEN) != 0) {
			fprintf(stderr, "convert-selftest: key conversion does not commutate!\n");
			return 1;
		}
	}


	/*
	 * test old API
	 */

	srand(0);

	for (i = 0; i < 1024; i++) {
		/* use pseudo-random for test keys (DO NOT DO THIS FOR REAL!) */
		for (j = 0; j < ED25519_KEY_LEN; j++)
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
		if (memcmp(check, dhpk, X25519_KEY_LEN) != 0) {
			fprintf(stderr, "convert-selftest: (OLD API) key conversion does not commutate!\n");
			return 1;
		}
	}


	return 0;
}
