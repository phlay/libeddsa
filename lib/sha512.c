/*
 * sha512 - calculates sha512 hash
 *
 * Written by Philipp Lay <philipp.lay@illunis.net> using code from
 * LibTomCrypt by Tom St Denis as reference.
 *
 * This code is under public domain.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sha512.h"

static const uint64_t K[80] = {
	0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL,
	0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
	0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL, 0xd807aa98a3030242ULL,
	0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
	0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL,
	0xc19bf174cf692694ULL, 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
	0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL, 0x2de92c6f592b0275ULL,
	0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
	0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL,
	0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
	0x06ca6351e003826fULL, 0x142929670a0e6e70ULL, 0x27b70a8546d22ffcULL,
	0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
	0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL,
	0x92722c851482353bULL, 0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
	0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL,
	0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
	0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL,
	0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
	0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL, 0x748f82ee5defb2fcULL,
	0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
	0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL,
	0xc67178f2e372532bULL, 0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
	0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL,
	0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
	0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL,
	0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
	0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};


#define ROR(x, n)	( ((x) >> (n)) | ((x) << (64-(n))) )
#define S0(x)		(ROR(x, 28) ^ ROR(x, 34) ^ ROR(x, 39))
#define S1(x)		(ROR(x, 14) ^ ROR(x, 18) ^ ROR(x, 41))
#define G0(x)		(ROR(x, 1) ^ ROR(x, 8) ^ (x >> 7))
#define G1(x)		(ROR(x, 19) ^ ROR(x, 61) ^ (x >> 6))


#define ROUND(i, a,b,c,d,e,f,g,h)			\
     t = h + S1(e) + (g ^ (e & (f ^ g))) + K[i] + W[i];	\
     d += t;						\
     h  = t + S0(a) + ( ((a | b) & c) | (a & b) )


static void
store_be64(uint8_t *p, uint64_t x)
{
	p[0] = (x >> 56) & 0xff;
	p[1] = (x >> 48) & 0xff;
	p[2] = (x >> 40) & 0xff;
	p[3] = (x >> 32) & 0xff;
	p[4] = (x >> 24) & 0xff;
	p[5] = (x >> 16) & 0xff;
	p[6] = (x >>  8) & 0xff;
	p[7] = (x >>  0) & 0xff;
}

static uint64_t
load_be64(const uint8_t *p)
{
	return ((uint64_t)p[0] << 56) |	((uint64_t)p[1] << 48) |
		((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) |
		((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) |
		((uint64_t)p[6] << 8) | ((uint64_t)p[7]);
}


static void
compress(uint64_t state[8], const uint8_t buf[SHA512_BLOCK_SIZE])
{
	uint64_t W[80], t;
	uint64_t a, b, c, d, e, f, g, h;
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
		W[i] = load_be64(buf+8*i);

	for (i = 16; i < 80; i++)
		W[i] = W[i-16] + G0(W[i-15]) + W[i-7] + G1(W[i-2]);

	for (i = 0; i < 80; i += 8) {
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
sha512_init(struct sha512 *ctx)
{
	ctx->fill = 0;
	ctx->count = 0;

	ctx->state[0] = 0x6a09e667f3bcc908ULL;
	ctx->state[1] = 0xbb67ae8584caa73bULL;
	ctx->state[2] = 0x3c6ef372fe94f82bULL;
	ctx->state[3] = 0xa54ff53a5f1d36f1ULL;
	ctx->state[4] = 0x510e527fade682d1ULL;
	ctx->state[5] = 0x9b05688c2b3e6c1fULL;
	ctx->state[6] = 0x1f83d9abfb41bd6bULL;
	ctx->state[7] = 0x5be0cd19137e2179ULL;
}

void
sha512_add(struct sha512 *ctx, const uint8_t *data, size_t len)
{
	if (ctx->fill > 0) {
		/* fill internal buffer up and compress */
		while (ctx->fill < SHA512_BLOCK_SIZE && len > 0) {
			ctx->buffer[ctx->fill++] = *data++;
			len--;
		}
		if (ctx->fill < SHA512_BLOCK_SIZE)
			return;

		compress(ctx->state, ctx->buffer);
		ctx->count++;
	}

	/* ctx->fill is now zero */

	while (len >= SHA512_BLOCK_SIZE) {
		compress(ctx->state, data);
		ctx->count++;
		
		data += SHA512_BLOCK_SIZE;
		len -= SHA512_BLOCK_SIZE;
	}

	/* save rest for next time */
	memcpy(ctx->buffer, data, len);
	ctx->fill = len;
}


void
sha512_final(struct sha512 *ctx, uint8_t out[SHA512_HASH_LENGTH])
{
	size_t rest;
	int i;

	rest = ctx->fill;
	
	/* append 1-bit to signal end of data */
	ctx->buffer[ctx->fill++] = 0x80;
	
	if (ctx->fill > SHA512_BLOCK_SIZE - 16) {
		while (ctx->fill < SHA512_BLOCK_SIZE)
			ctx->buffer[ctx->fill++] = 0;

		compress(ctx->state, ctx->buffer);
		ctx->fill = 0;
	}
	while (ctx->fill < SHA512_BLOCK_SIZE - 16)
		ctx->buffer[ctx->fill++] = 0;

	/* because rest < 128 our message length is
	 * L := 128*ctx->count + rest == (ctx->count<<7)|rest,
	 * now convert L to number of bits and write out as 128bit big-endian.
	 */
	store_be64(ctx->buffer+SHA512_BLOCK_SIZE-16,
			ctx->count >> 54);
	store_be64(ctx->buffer+SHA512_BLOCK_SIZE-8,
			((ctx->count << 7) | rest) << 3);
	
	compress(ctx->state, ctx->buffer);


	for (i = 0; i < 8; i++)
		store_be64(out + 8*i, ctx->state[i]);
}
