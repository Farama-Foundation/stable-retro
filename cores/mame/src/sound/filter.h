#ifndef __FILTER_H
#define __FILTER_H

#include "osd_cpu.h"

/* Max filter order */
#define FILTER_ORDER_MAX 51

/* Define to use interger calculation */
#define FILTER_USE_INT

#ifdef FILTER_USE_INT
typedef int filter_real;
#define FILTER_INT_FRACT 15 /* fractional bits */
#else
typedef double filter_real;
#endif

typedef struct filter_struct {
	filter_real xcoeffs[(FILTER_ORDER_MAX+1)/2];
	unsigned order;
} filter;

typedef struct filter_state_struct {
	unsigned prev_mac;
	filter_real xprev[FILTER_ORDER_MAX];
} filter_state;

/* Allocate a FIR Low Pass filter */
filter* filter_lp_fir_alloc(double freq, int order);
void filter_free(filter* f);

/* Allocate a filter state */
filter_state* filter_state_alloc(void);

/* Free the filter state */
void filter_state_free(filter_state* s);

/* Clear the filter state */
void filter_state_reset(filter* f, filter_state* s);

/* Insert a value in the filter state */
static INLINE void filter_insert(filter* f, filter_state* s, filter_real x) {
	/* next state */
	++s->prev_mac;
	if (s->prev_mac >= f->order)
		s->prev_mac = 0;

	/* set x[0] */
	s->xprev[s->prev_mac] = x;
}

/* Compute the filter output */
filter_real filter_compute(filter* f, filter_state* s);

#endif
