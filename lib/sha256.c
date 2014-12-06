/*
 * sha256 - calculates sha256 hash
 *
 * This code is under public domain.
 *
 * Philipp Lay <philipp.lay@illunis.net>.
 */

#include <stdint.h>
#include <stddef.h>

#include "sha256.h"


static const uint32_t K[80] = {
	0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
	0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
	0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
	0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
	0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
	0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
	0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
	0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
	0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
	0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
	0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
	0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
	0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
	0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
	0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL,
};



static void
store_be32(uint8_t *p, uint32_t x)
{
	p[0] = (x >> 24) & 0xff;
	p[1] = (x >> 16) & 0xff;
	p[2] = (x >>  8) & 0xff;
	p[3] = (x >>  0) & 0xff;
}


static uint32_t
load_be32(const uint8_t *p)
{
	return (((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
		((uint32_t)p[2] << 8) | ((uint32_t)p[3]));
}

#define ROR(x, n)	( ((x) >> (n)) | ((x) << (32-(n))) )
#define S0(x)		(ROR(x,  2) ^ ROR(x, 13) ^ ROR(x, 22))
#define S1(x)		(ROR(x,  6) ^ ROR(x, 11) ^ ROR(x, 25))
#define G0(x)		(ROR(x,  7) ^ ROR(x, 18) ^ (x >> 3))
#define G1(x)		(ROR(x, 17) ^ ROR(x, 19) ^ (x >> 10))

#define ROUND(i, a,b,c,d,e,f,g,h)				\
	t = h + S1(e) + (g ^ (e & (f ^ g))) + K[i] + W[i];	\
	d += t;							\
	h  = t  + S0(a) + ( ((a | b) & c) | (a & b) )


static void
compress(uint32_t state[8], const uint8_t buf[64])
{
	uint32_t W[64], t;
	uint32_t a, b, c, d, e, f, g, h;
	int i;

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	f = state[5];
	g = state[6];
	h = state[7];

	for (i = 0; i < 16; i++)
		W[i] = load_be32(buf+4*i);

	for (i = 16; i < 64; i++)
		W[i] = W[i-16] + G0(W[i-15]) + W[i-7] + G1(W[i-2]);

	for (i = 0; i < 64; i += 8) {
		ROUND(i+0, a,b,c,d,e,f,g,h);
		ROUND(i+1, h,a,b,c,d,e,f,g);
		ROUND(i+2, g,h,a,b,c,d,e,f);
		ROUND(i+3, f,g,h,a,b,c,d,e);
		ROUND(i+4, e,f,g,h,a,b,c,d);
		ROUND(i+5, d,e,f,g,h,a,b,c);
		ROUND(i+6, c,d,e,f,g,h,a,b);
		ROUND(i+7, b,c,d,e,f,g,h,a);
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	state[5] += f;
	state[6] += g;
	state[7] += h;
}


void
sha256_init(sha256ctx *ctx)
{
	ctx->fill = 0;
	ctx->count = 0;

	ctx->state[0] = 0x6a09e667U;
	ctx->state[1] = 0xbb67ae85U;
	ctx->state[2] = 0x3c6ef372U;
	ctx->state[3] = 0xa54ff53aU;
	ctx->state[4] = 0x510e527fU;
	ctx->state[5] = 0x9b05688cU;
	ctx->state[6] = 0x1f83d9abU;
	ctx->state[7] = 0x5be0cd19U;
}

void
sha256_add(sha256ctx *ctx, const uint8_t *data, size_t len)
{
	int i;

	if (ctx->fill > 0) {
		/* fill internal buffer up and compress */
		while (ctx->fill < 64 && len > 0) {
			ctx->buffer[ctx->fill++] = *data++;
			len--;
		}
		if (ctx->fill < 64)
			return;

		compress(ctx->state, ctx->buffer);
		ctx->count++;
	}

	/* ctx->fill is now zero */

	while (len >= 64) {
		compress(ctx->state, data);
		ctx->count++;

		data += 64;
		len -= 64;
	}

	/* save rest for next time */
	for (i = 0; i < len; i++)
		ctx->buffer[i] = data[i];

	ctx->fill = len;
}


void
sha256_done(sha256ctx *ctx, uint8_t out[32])
{
	int rest;
	int i;

	rest = ctx->fill;

	/* append 1-bit to signal end of data */
	ctx->buffer[ctx->fill++] = 0x80;

	if (ctx->fill > 56) {
		while (ctx->fill < 64)
			ctx->buffer[ctx->fill++] = 0;

		compress(ctx->state, ctx->buffer);
		ctx->fill = 0;
	}
	while (ctx->fill < 56)
		ctx->buffer[ctx->fill++] = 0;

	/* because rest < 64 our message length is
	 *   L := 64*ctx->count + rest == (ctx->count<<6)|rest.
	 * now we have to write the message length in bit, or
	 *   8*L = ((ctx->count << 6) | rest) << 3.
	 */
	store_be32(ctx->buffer+56, ctx->count >> 23);
	store_be32(ctx->buffer+60, ((ctx->count << 6) | rest) << 3);

	compress(ctx->state, ctx->buffer);

	/* store hash */
	for (i = 0; i < 8; i++)
		store_be32(out + 4*i, ctx->state[i]);
}
