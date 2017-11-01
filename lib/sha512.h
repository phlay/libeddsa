#ifndef SHA512_H
#define SHA512_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA512_BLOCK_SIZE	128
#define SHA512_HASH_LENGTH	64


struct sha512 {
	uint64_t	state[8];
	uint64_t	count;

	uint8_t		buffer[SHA512_BLOCK_SIZE];
	size_t		fill;
};

void sha512_init(struct sha512 *ctx);
void sha512_add(struct sha512 *ctx, const uint8_t *data, size_t len);
void sha512_final(struct sha512 *ctx, uint8_t out[SHA512_HASH_LENGTH]);

#ifdef __cplusplus
}
#endif

#endif
