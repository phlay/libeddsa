#include <stdint.h>

#include "burn.h"
#include "burnstack.h"

/*
 * burnstack - cleanup our stack
 */
void
burnstack(int len)
{
	uint8_t stack[1024];
	burn(stack, 1024);
	if (len > 0)
		burnstack(len-1024);
}
