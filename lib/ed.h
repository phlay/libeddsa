#ifndef ED_H
#define ED_H

#include <stdint.h>

#include "fld.h"


/* (x,y,z) are the projective points on our curve E and t = x*y is an
 * auxiliary coordinate (see [2], ch. 3).
 */
struct ed {
	fld_t		x;
	fld_t		y;
	fld_t		t;
	fld_t		z;
};


/* export base point */
extern const struct ed ed_BP;


void	ed_export(uint8_t out[32], const struct ed *P);
void	ed_import(struct ed *P, const uint8_t in[32]);

void	ed_scale_base(struct ed *res, const sc_t x);
void	ed_scale(struct ed *res, const sc_t x, const struct ed *P);

void	ed_double_scale(struct ed *res,
			const sc_t x, const struct ed *P,
			const sc_t y, const struct ed *Q);

#endif
