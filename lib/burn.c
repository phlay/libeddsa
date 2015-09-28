#include <stdint.h>
#include <stddef.h>

/*
 * burn - simple function to zero a buffer, used to cover our tracks
 */
void
burn(void *dest, size_t len)
{
	volatile uint8_t *p = (uint8_t *)dest;
	const uint8_t *end = (uint8_t *)dest+len;

	while (p < end) *p++ = 0;
}
