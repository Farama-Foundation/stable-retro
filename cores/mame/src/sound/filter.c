#include "filter.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

static filter* filter_alloc(void) {
	filter* f = malloc(sizeof(filter));
	return f;
}

void filter_free(filter* f) {
	free(f);
}

void filter_state_reset(filter* f, filter_state* s) {
	int i;
	s->prev_mac = 0;
	for(i=0;i<f->order;++i) {
		s->xprev[i] = 0;
	}
}

filter_state* filter_state_alloc(void) {
	int i;
        filter_state* s = malloc(sizeof(filter_state));
	s->prev_mac = 0;
	for(i=0;i<FILTER_ORDER_MAX;++i)
		s->xprev[i] = 0;
	return s;
}

void filter_state_free(filter_state* s) {
	free(s);
}

/****************************************************************************/
/* FIR */

filter_real filter_compute(filter* f, filter_state* s) {
	unsigned order = f->order;
	unsigned midorder = f->order / 2;
	filter_real y = 0;
	unsigned i,j,k;

	/* i == [0] */
	/* j == [-2*midorder] */
	i = s->prev_mac;
	j = i + 1;
	if (j == order)
		j = 0;

	/* x */
	for(k=0;k<midorder;++k) {
		y += f->xcoeffs[midorder-k] * (s->xprev[i] + s->xprev[j]);
		++j;
		if (j == order)
			j = 0;
		if (i == 0)
			i = order - 1;
		else
			--i;
	}
	y += f->xcoeffs[0] * s->xprev[i];

#ifdef FILTER_USE_INT
	return y >> FILTER_INT_FRACT;
#else
	return y;
#endif
}

filter* filter_lp_fir_alloc(double freq, int order) {
	filter* f = filter_alloc();
	unsigned midorder = (order - 1) / 2;
	unsigned i;
	double gain;

	assert( order <= FILTER_ORDER_MAX );
	assert( order % 2 == 1 );
	assert( 0 < freq && freq <= 0.5 );

	/* Compute the antitrasform of the perfect low pass filter */
	gain = 2*freq;
#ifdef FILTER_USE_INT
	f->xcoeffs[0] = gain * (1 << FILTER_INT_FRACT);
#else
	f->xcoeffs[0] = gain;
#endif
	for(i=1;i<=midorder;++i) {
		/* number of the sample starting from 0 to (order-1) included */
		unsigned n = i + midorder;

		/* sample value */
		double c = sin(2*M_PI*freq*i) / (M_PI*i);

		/* apply only one window or none */
		/* double w = 2 - 2*n/(order-1); */ /* Bartlett (triangular) */
		/* double w = 0.5 * (1 - cos(2*M_PI*n/(order-1))); */ /* Hanning */
		double w = 0.54 - 0.46 * cos(2*M_PI*n/(order-1)); /* Hamming */
		/* double w = 0.42 - 0.5 * cos(2*M_PI*n/(order-1)) + 0.08 * cos(4*M_PI*n/(order-1)); */ /* Blackman */

		/* apply the window */
		c *= w;

		/* update the gain */
		gain += 2*c;

		/* insert the coeff */
#ifdef FILTER_USE_INT
		f->xcoeffs[i] = c * (1 << FILTER_INT_FRACT);
#else
		f->xcoeffs[i] = c;
#endif
	}

	/* adjust the gain to be exact 1.0 */
	for(i=0;i<=midorder;++i) {
#ifdef FILTER_USE_INT
		f->xcoeffs[i] /= gain;
#else
		f->xcoeffs[i] = f->xcoeffs[i] * (double)(1 << FILTER_INT_FRAC) / gain;
#endif
	}

	/* decrease the order if the last coeffs are 0 */
	i = midorder;
	while (i > 0 && f->xcoeffs[i] == 0.0)
		--i;

	f->order = i * 2 + 1;

	return f;
}
