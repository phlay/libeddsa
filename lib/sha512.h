#ifndef SHA512_H
#define SHA512_H

#include <stddef.h>
#include <stdint.h>


typedef struct {
	uint64_t	state[8];
	uint64_t	count;
	
	uint8_t		buffer[128];
	size_t		fill;
} sha512ctx;



void sha512_init(sha512ctx *ctx);
void sha512_add(sha512ctx *ctx, const uint8_t *data, size_t len);
void sha512_done(sha512ctx *ctx, uint8_t out[64]);



#endif
