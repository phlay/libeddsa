#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eddsa.h>


struct {
	uint8_t		sec[ED25519_KEY_LEN];
	uint8_t		pub[ED25519_KEY_LEN];
	uint8_t		sig[ED25519_SIG_LEN];
	uint8_t		msg[1024];
} table[] = {
	#include "ed25519-table.h"
};

const int table_num = sizeof(table) / sizeof(table[0]);



int main()
{
	uint8_t checkpub[ED25519_KEY_LEN];
	uint8_t checksig[ED25519_SIG_LEN];

	int i;

	/*
	 * test ed25519 functions
	 */
	for (i = 0; i < table_num; i++) {
		/* check one: do we generate the same public key? */
		ed25519_genpub(checkpub, table[i].sec);
		if (memcmp(checkpub, table[i].pub, ED25519_KEY_LEN) != 0) {
			fprintf(stderr, "eddsa-selftest: generating ed25519 public key number %d failed\n", i+1);
			return 1;
		}

		/* check two: do we make the same signature? */
		ed25519_sign(checksig, table[i].sec, table[i].pub, table[i].msg, i);
		if (memcmp(checksig, table[i].sig, ED25519_SIG_LEN) != 0) {
			fprintf(stderr, "eddsa-selftest: generating ed25519 signature number %d failed\n", i+1);
			return 1;
		}

		/* check three: does signature get verified? */
		if (!ed25519_verify(table[i].sig, table[i].pub, table[i].msg, i)) {
			fprintf(stderr, "eddsa-selftest: verifying ed25519 signature number %d failed\n", i+1);
			return 1;
		}
	}


	/*
	 * test old interface
	 */
	for (i = 0; i < table_num; i++) {
		/* check one: do we generate the same public key? */
		eddsa_genpub(checkpub, table[i].sec);
		if (memcmp(checkpub, table[i].pub, 32) != 0) {
			fprintf(stderr, "eddsa-selftest: (OLD API) generating public key number %d failed\n", i+1);
			return 1;
		}

		/* check two: do we make the same signature? */
		eddsa_sign(checksig, table[i].sec, table[i].pub, table[i].msg, i);
		if (memcmp(checksig, table[i].sig, 64) != 0) {
			fprintf(stderr, "eddsa-selftest: (OLD API) generating signature number %d failed\n", i+1);
			return 1;
		}

		/* check three: does signature get verified? */
		if (!eddsa_verify(table[i].sig, table[i].pub, table[i].msg, i)) {
			fprintf(stderr, "eddsa-selftest: (OLD API) verifying signature number %d failed\n", i+1);
			return 1;
		}
	}

	return 0;
}
