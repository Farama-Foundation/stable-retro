#include <math.h>
/*fix for android */
#ifndef PI
#define PI 3.14159265358979323846
#endif

/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@esplexo.co.uk)                       */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DST_CRFILTER          - Simple CR filter & also highpass filter      */
/* DST_FILTER1           - Generic 1st order filter                     */
/* DST_FILTER2           - Generic 2nd order filter                     */
/* DST_OP_AMP_FILT       - Op Amp filter circuits                       */
/* DST_RCFILTER          - Simple RC filter & also lowpass filter       */
/* DST_RCDISC            - Simple discharging RC                        */
/* DST_RCDISC2           - Simple charge R1/C, discharge R0/C           */
/*                                                                      */
/************************************************************************/

struct dss_filter1_context
{
	double x1;		/* x[k-1], previous input value */
	double y1;		/* y[k-1], previous output value */
	double a1;		/* digital filter coefficients, denominator */
	double b0, b1;	/* digital filter coefficients, numerator */
};

struct dss_filter2_context
{
	double x1, x2;		/* x[k-1], x[k-2], previous 2 input values */
	double y1, y2;		/* y[k-1], y[k-2], previous 2 output values */
	double a1, a2;		/* digital filter coefficients, denominator */
	double b0, b1, b2;	/* digital filter coefficients, numerator */
};

struct dst_op_amp_filt_context
{
	int		type;		/* What kind of filter */
	int		is_norton;	/* 1 = Norton op-amps */
	double	rTotal;		/* All input resistance in parallel. */
	double	iFixed;		/* Current supplied by rP & rN if used. */
	double	exponentC1;
	double	exponentC2;
	double	rRatio;		/* divide ratio of rTotal & rF */
	double	vC1;		/* Charge on C1 */
	double	vC1b;		/* Charge on C1, part of C1 charge if needed */
	double	vC2;		/* Charge on C2 */
	double	vF_last;	/* Last output voltage relative to vRef. */
	double	gain;		/* Gain of the filter */
};

struct dst_rcdisc_context
{
        int state;
        double t;           /* time */
        double step;
	double exponent0;
	double exponent1;
};

struct dst_rcfilter_context
{
	double	exponent;
	double	vCap;
};


/************************************************************************/
/*                                                                      */
/* DST_FILTER1 - Generic 1st order filter                               */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Frequency value (initialization only)                  */
/* input[3]    - Filter type (initialization only)                      */
/*                                                                      */
/************************************************************************/

static void calculate_filter1_coefficients(double fc, double type,
                                           double *a1, double *b0, double *b1)
{
	double den, w, two_over_T;

	/* calculate digital filter coefficents */
	/*w = 2.0*M_PI*fc; no pre-warping */
	w = Machine->sample_rate*2.0*tan(M_PI*fc/Machine->sample_rate); /* pre-warping */
	two_over_T = 2.0*Machine->sample_rate;

	den = w + two_over_T;
	*a1 = (w - two_over_T)/den;
	if (type == DISC_FILTER_LOWPASS)
	{
		*b0 = *b1 = w/den;
	}
	else if (type == DISC_FILTER_HIGHPASS)
	{
		*b0 = two_over_T/den;
		*b1 = *b0;
	}
	else
	{
		discrete_log("calculate_filter1_coefficients() - Invalid filter type for 1st order filter.");
	}
}

void dst_filter1_step(struct node_description *node)
{
	struct dss_filter1_context *context = node->context;
	double gain = 1.0;

	if (node->input[0] == 0.0)
	{
		gain = 0.0;
	}

	node->output = -context->a1*context->y1 + context->b0*gain*node->input[1] + context->b1*context->x1;

	context->x1 = gain*node->input[1];
	context->y1 = node->output;
}

void dst_filter1_reset(struct node_description *node)
{
	struct dss_filter1_context *context = node->context;

	calculate_filter1_coefficients(node->input[2], node->input[3], &context->a1, &context->b0, &context->b1);
	node->output=0;
}


/************************************************************************/
/*                                                                      */
/* DST_FILTER2 - Generic 2nd order filter                               */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Frequency value (initialization only)                  */
/* input[3]    - Damping value (initialization only)                    */
/* input[4]    - Filter type (initialization only) 			*/
/*                                                                      */
/************************************************************************/

static void calculate_filter2_coefficients(double fc, double d, double type,
                                           double *a1, double *a2,
                                           double *b0, double *b1, double *b2)
{
	double w;	/* cutoff freq, in radians/sec */
	double w_squared;
	double den;	/* temp variable */
	double two_over_T = 2*Machine->sample_rate;
	double two_over_T_squared = two_over_T * two_over_T;

	/* calculate digital filter coefficents */
	/*w = 2.0*M_PI*fc; no pre-warping */
	w = Machine->sample_rate*2.0*tan(M_PI*fc/Machine->sample_rate); /* pre-warping */
	w_squared = w*w;

	den = two_over_T_squared + d*w*two_over_T + w_squared;

	*a1 = 2.0*(-two_over_T_squared + w_squared)/den;
	*a2 = (two_over_T_squared - d*w*two_over_T + w_squared)/den;

	if (type == DISC_FILTER_LOWPASS)
	{
		*b0 = *b2 = w_squared/den;
		*b1 = 2.0*(*b0);
	}
	else if (type == DISC_FILTER_BANDPASS)
	{
		*b0 = w*two_over_T/den;
		*b1 = 0.0;
		*b2 = -(*b0);
	}
	else if (type == DISC_FILTER_HIGHPASS)
	{
		*b0 = *b2 = two_over_T_squared/den;
		*b1 = -2.0*(*b0);
	}
	else
	{
		discrete_log("calculate_filter2_coefficients() - Invalid filter type for 2nd order filter.");
	}
}

void dst_filter2_step(struct node_description *node)
{
	struct dss_filter2_context *context = node->context;
	double gain = 1.0;

	if (node->input[0] == 0.0)
	{
		gain = 0.0;
	}

	node->output = -context->a1*context->y1 - context->a2*context->y2 +
	                context->b0*gain*node->input[1] + context->b1*context->x1 + context->b2*context->x2;

	context->x2 = context->x1;
	context->x1 = gain*node->input[1];
	context->y2 = context->y1;
	context->y1 = node->output;
}

void dst_filter2_reset(struct node_description *node)
{
	struct dss_filter2_context *context = node->context;

	calculate_filter2_coefficients(node->input[2], node->input[3], node->input[4],
								   &context->a1, &context->a2,
								   &context->b0, &context->b1, &context->b2);
	node->output=0;
}


/************************************************************************/
/*                                                                      */
/* DST_OP_AMP_FILT - Op Amp filter circuit RC filter                    */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - IN0 node                                               */
/* input[2]    - IN1 node                                               */
/* input[3]    - Filter Type                                            */
/*                                                                      */
/* also passed discrete_op_amp_filt_info structure                      */
/*                                                                      */
/* Mar 2004, D Renaud.                                                  */
/************************************************************************/
#define DST_OP_AMP_FILT_ENABLE	node->input[0]
#define DST_OP_AMP_FILT_INP1	node->input[1]
#define DST_OP_AMP_FILT_INP2	node->input[2]
#define DST_OP_AMP_FILT_TYPE	node->input[3]

void dst_op_amp_filt_step(struct node_description *node)
{
	const struct discrete_op_amp_filt_info *info = node->custom;
	struct dst_op_amp_filt_context *context = node->context;

	double i, v, vMid;

	if (DST_OP_AMP_FILT_ENABLE)
	{
		/* Millman the input voltages. */
		i = context->iFixed;
		i += (DST_OP_AMP_FILT_INP1 - info->vRef) / info->r1;
		if (info->r2 != 0)
			i += (DST_OP_AMP_FILT_INP2 - info->vRef) / info->r2;
		v = i * context->rTotal;

		switch (context->type)
		{
			case DISC_OP_AMP_FILTER_IS_LOW_PASS_1:
				context->vC1 += (v - context->vC1) * context->exponentC1;
				node->output = context->vC1 * context->gain + info->vRef;
				break;

			case DISC_OP_AMP_FILTER_IS_HIGH_PASS_1:
				context->vC1 += (v - context->vC1) * context->exponentC1;
				node->output = (v - context->vC1) * context->gain + info->vRef;
				break;

			case DISC_OP_AMP_FILTER_IS_BAND_PASS_1:
				context->vC2 += (v - context->vC2) * context->exponentC2;
				node->output = (v - context->vC2);
				context->vC1 += (node->output - context->vC1) * context->exponentC1;
				node->output = context->vC1 * context->gain + info->vRef;
				break;

			case DISC_OP_AMP_FILTER_IS_BAND_PASS_1M:
				/* Only the code in this case is bad.  All other filter code is good.
				 * In the restet I tell rhe module to use DISC_OP_AMP_FILTER_IS_BAND_PASS_1 for now.
				 */
				/* What a feedback nightmare, but these are facts I do know.
				 * vRef is not shown because v is assumed to be refrenced to it all ready.
				 * The arrows show relative current flow.
				 *
				 *                     c1
				 *                .----||----.------------------.
				 *                |          |                  |
				 *              v |        v |                  | ^
				 *              v |        v Z                  | ^    -i1 + i2 + i3 = 0
				 *             i2 |       i3 Z rF               | i1    i1 = i2 + i3
				 *              v |        v Z                  | ^     i2 = i1 - i3
				 *              v |        v |        |\        | ^     i3 = i1 - i2
				 *      rTotal    |    c2    |        | \       |
				 * v >--/\/\/\----+----||----+--------|- \      |
				 *     << i1 <<     << i3 <<    i=0   |   \     |
				 *                                    |    >----+---> vOut = -i3 * rF
				 *
				 * At init c1 & c2 have no charge so i1 = v / rTotal.
				 * The output needs to source or sink this total amount of current.
				 * No current is available to put a voltage across rF, so vOut = 0V.
				 * The current feed back into the (-) input must cancel out the current in to give 0 current.
				 * This means the current across rF tracks the current charge of C2.
				 * At init there is no current charge stored in c2, so no current is feed back through rF.
				 * This confirms that vOut = 0V at init.
				 * Note that i3 tracks the current stored in c2.
				 *
				 *                     c1    v1
				 *                .----||----.------------------.
				 *                |   vC1    |                  |
				 *                |          |                  |
				 *                |          Z                  |       v0 = vC2
				 *                |          Z rF               |       i1 = (v - v0) / rTotal
				 *                |          Z                  |
				 *                |          |        |\        |
				 *      rTotal    |    c2    |        | \       |
				 * v >--/\/\/\----+----||----+--------|- \      |
				 *     << i1 <<   v0  vC2    v2 = 0V  |   \     |
				 *                        virtual GND |    >----+---> vOut
				 *
				 * From here I get lost.
				 */
				 
/* Test stuff */
v=1;
context->vF_last=-3;
				/* First we need to work out the passive components. */
				/* Step 1 - Work out the voltage mid-point between out and in.
				 *
				 *         rTotal         rF
				 *    v >--/\/\/\---+---/\/\/\--< vF_last (vOut without bias)
				 *                  |
				 *                  '-----------> vMid
				 */ 
				vMid = (v - context->vF_last) * context->rRatio + context->vF_last;

				/* Step 2 - RC filter C1.
				 *          This is done in 2 parts to get the voltage on both sides of C1.
				 *          One side of the cap heads towards v.  The other heads towards vF.
				 *
				 *       rTotal + rF                       rTotal + rF
				 *    v >--/\/\/\---+---> vC1   vC1b <---+---/\/\/\--< vF_last
				 *                  |                    |
				 *                 ---                  ---
				 *                 --- c1               --- c1
				 *                  |                    |
				 *                vMid                 vMid
				 */
				context->vC1 += (v - context->vC1 - vMid) * context->exponentC1 + vMid;
				context->vC1b += (context->vF_last - context->vC1b - vMid) * context->exponentC1 + vMid;

				/* Step 3 - RC filter C2.
				 *
				 *         rTotal || rF   
				 *    vC1 >--/\/\/\---+---> vC2
				 *                    |
				 *                   ---
				 *                   --- c2
				 *                    |
				 *                  vGnd (virtual ground)
				 */
				context->vC2 += (context->vC1 - context->vC2) * context->exponentC2;

				/* Now we work out the active parts.  This is done through adding the current paths.
				 * Then the gain will be -I * rF.  Add vRef to get final amp out.
				 *
				 *                 rF      c1
				 *    vF_last >--/\/\/\----||------.
				 *             >>>>>>I1>>>>>>      |    c2
				 *                                 +----||-------< vGnd
				 *               rTotal    c1      |  >>>I=I1+I2>>>
				 *          v >--/\/\/\----||------'
				 *             >>>>>>I2>>>>>>
				 */
				node->output = -((v - context->vC2) / context->rTotal			/* I1 */
					+ (context->vF_last - context->vC2 - context->vC1b) / info->rF)	/* I2 */
					* info->rF + info->vRef;
/* Test stuff */
node->output=context->vC2;

				break;
		}

		/* Clip the output to the voltage rails.
		 * This way we get the original distortion in all it's glory.
		 */
		if (node->output > info->vP) node->output = info->vP;
		if (node->output < info->vN) node->output = info->vN;
		context->vF_last = node->output - info->vRef;
	}
	else
		node->output = 0;
}

void dst_op_amp_filt_reset(struct node_description *node)
{
	const struct discrete_op_amp_filt_info *info = node->custom;
	struct dst_op_amp_filt_context *context = node->context;

	/* Convert the passed filter type into an int for easy use. */
	context->type = (int)DST_OP_AMP_FILT_TYPE & DISC_OP_AMP_FILTER_TYPE_MASK;
	context->is_norton = (int)DST_OP_AMP_FILT_TYPE & DISC_OP_AMP_IS_NORTON;

	/* Remove this when DISC_OP_AMP_FILTER_IS_BAND_PASS_1M works. */
	if (context->type == DISC_OP_AMP_FILTER_IS_BAND_PASS_1M) context->type = DISC_OP_AMP_FILTER_IS_BAND_PASS_1;

	/* Work out the input resistance.  It is all input and bias resistors in parallel. */
	context->rTotal  = 1.0 / info->r1;			/* There has to be an R1.  Otherwise the table is wrong. */
	if (info->r2 != 0) context->rTotal += 1.0 / info->r2;
	if (info->rP != 0) context->rTotal += 1.0 / info->rP;
	if (info->rN != 0) context->rTotal += 1.0 / info->rN;
	context->rTotal = 1.0 / context->rTotal;

	/* Work out the current of the bias circuit if used. */
	context->iFixed = 0;
	if (info->rP != 0) context->iFixed  = (info->vP - info->vRef) / info->rP;
	if (info->rN != 0) context->iFixed += (info->vN - info->vRef) / info->rN;

	
	switch (context->type)
	{
		case DISC_OP_AMP_FILTER_IS_LOW_PASS_1:
			context->exponentC1 = -1.0 / (info->rF * info->c1 * Machine->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			context->exponentC2 = 0;
			break;
		case DISC_OP_AMP_FILTER_IS_HIGH_PASS_1:
			context->exponentC1 = -1.0 / (context->rTotal * info->c1 * Machine->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			context->exponentC2 = 0;
			break;
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_1:
			context->exponentC1 = -1.0 / (info->rF * info->c1 * Machine->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			context->exponentC2 = -1.0 / (context->rTotal * info->c2 * Machine->sample_rate);
			context->exponentC2 = 1.0 - exp(context->exponentC2);
			break;
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_1M:
			context->exponentC1 = -1.0 / ((context->rTotal + info->rF) * info->c1 * Machine->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			context->exponentC2 = -1.0 / ((1.0 / ( 1.0 / context->rTotal + 1.0 / info->rF)) * info->c2 * Machine->sample_rate);
			context->exponentC2 = 1.0 - exp(context->exponentC2);
			break;
	}

	/* At startup there is no charge on the caps and output is 0V in relation to vRef. */
	context->vC1 = 0;
	context->vC1b = 0;
	context->vC2 = 0;
	context->vF_last = 0;

	context->rRatio = info->rF / (context->rTotal + info->rF);
	context->gain = -info->rF / context->rTotal;
	node->output = info->vRef;
}


/************************************************************************/
/*                                                                      */
/* DST_RCFILTER - Usage of node_description values for RC filter        */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialization only)                   */
/* input[3]    - Capacitor Value (initialization only)                  */
/* input[4]    - Voltage reference. Usually 0V.                         */
/*                                                                      */
/************************************************************************/

void dst_rcfilter_step(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	/************************************************************************/
	/* Next Value = PREV + (INPUT_VALUE - PREV)*(1-(EXP(-TIMEDELTA/RC)))    */
	/************************************************************************/

	if(node->input[0])
	{
		context->vCap += ((node->input[1] - node->input[4] - context->vCap) * context->exponent);
		node->output = context->vCap + node->input[4];
	}
	else
	{
		node->output=0;
	}
}

void dst_rcfilter_reset(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	context->exponent = -1.0 / (node->input[2] * node->input[3] * Machine->sample_rate);
	context->exponent = 1.0 - exp(context->exponent);
	context->vCap = 0;
	node->output = 0;
}


/************************************************************************/
/*                                                                      */
/* DST_CRFILTER - Usage of node_description values for CR filter        */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialization only)                   */
/* input[3]    - Capacitor Value (initialization only)                  */
/* input[4]    - Voltage reference. Usually 0V.                         */
/*                                                                      */
/************************************************************************/

void dst_crfilter_step(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	if(node->input[0])
	{
		context->vCap += ((node->input[1] - node->input[4]) - context->vCap) * context->exponent;
		node->output = node->input[1] - context->vCap;
	}
	else
	{
		node->output=0;
	}
}

void dst_crfilter_reset(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	context->exponent = -1.0 / (node->input[2] * node->input[3] * Machine->sample_rate);
	context->exponent = 1.0 - exp(context->exponent);
	context->vCap = 0;
	node->output = node->input[1];
}


/************************************************************************/
/*                                                                      */
/* DST_RCDISC -   Usage of node_description values for RC discharge     */
/*                (inverse slope of DST_RCFILTER)                       */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialization only)                   */
/* input[3]    - Capacitor Value (initialization only)                  */
/*                                                                      */
/************************************************************************/

void dst_rcdisc_step(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	switch (context->state) {
		case 0:     /* waiting for trigger  */
			if(node->input[0]) {
				context->state = 1;
				context->t = 0;
			}
			node->output=0;
			break;

		case 1:
			if (node->input[0]) {
				node->output=node->input[1] * exp(context->t / context->exponent0);
				context->t += context->step;
                } else {
					context->state = 0;
			}
		}
}

void dst_rcdisc_reset(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	node->output=0;

	context->state = 0;
	context->t = 0;
	context->step = 1.0 / Machine->sample_rate;
	context->exponent0=-1.0 * node->input[2]*node->input[3];
}


/************************************************************************/
/*                                                                      */
/* DST_RCDISC2 -  Usage of node_description values for RC discharge     */
/*                Has switchable charge resistor/input                  */
/*                                                                      */
/* input[0]    - Switch input value                                     */
/* input[1]    - input[0] value                                         */
/* input[2]    - Resistor0 value (initialization only)                  */
/* input[3]    - input[1] value                                         */
/* input[4]    - Resistor1 value (initialization only)                  */
/* input[5]    - Capacitor Value (initialization only)                  */
/*                                                                      */
/************************************************************************/

void dst_rcdisc2_step(struct node_description *node)
{
	double diff;
	struct dst_rcdisc_context *context = node->context;

	/* Works differently to other as we always on, no enable */
	/* exponential based in difference between input/output   */

	diff = ((node->input[0]==0)?node->input[1]:node->input[3])-node->output;
	diff = diff -(diff * exp(context->step / ((node->input[0]==0)?context->exponent0:context->exponent1)));
	node->output+=diff;
}

void dst_rcdisc2_reset(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	node->output=0;

	context->state = 0;
	context->t = 0;
	context->step = 1.0 / Machine->sample_rate;
	context->exponent0=-1.0 * node->input[2]*node->input[5];
	context->exponent1=-1.0 * node->input[4]*node->input[5];
}

/* !!!!!!!!!!! NEW FILTERS for testing !!!!!!!!!!!!!!!!!!!!! */


/************************************************************************/
/*                                                                      */
/* DST_RCFILTERN - Usage of node_description values for RC filter       */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialization only)                   */
/* input[3]    - Capacitor Value (initialization only)                  */
/*                                                                      */
/************************************************************************/

void dst_rcfilterN_reset(struct node_description *node)
{
	double f=1.0/(2*M_PI*node->input[2]*node->input[3]);

	node->input[2] = f;
	node->input[3] = DISC_FILTER_LOWPASS;

	/* Use first order filter */
	dst_filter1_reset(node);
}


/************************************************************************/
/*                                                                      */
/* DST_RCDISCN -   Usage of node_description values for RC discharge    */
/*                (inverse slope of DST_RCFILTER)                       */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialization only)                   */
/* input[3]    - Capacitor Value (initialization only)                  */
/*                                                                      */
/************************************************************************/

void dst_rcdiscN_reset(struct node_description *node)
{
	double f=1.0/(2*M_PI*node->input[2]*node->input[3]);

	node->input[2] = f;
	node->input[3] = DISC_FILTER_LOWPASS;

	/* Use first order filter */
	dst_filter1_reset(node);
}

void dst_rcdiscN_step(struct node_description *node)
{
	struct dss_filter1_context *context = node->context;
	double gain = 1.0;

	if (node->input[0] == 0.0)
	{
		gain = 0.0;
	}

	/* A rise in the input signal results in an instant charge, */
	/* else discharge through the RC to zero */
	if (gain*node->input[1] > context->x1)
		node->output = gain*node->input[1];
	else
		node->output = -context->a1*context->y1;

	context->x1 = gain*node->input[1];
	context->y1 = node->output;
}


/************************************************************************/
/*                                                                      */
/* DST_RCDISC2N -  Usage of node_description values for RC discharge    */
/*                Has switchable charge resistor/input                  */
/*                                                                      */
/* input[0]    - Switch input value                                     */
/* input[1]    - input[0] value                                         */
/* input[2]    - Resistor0 value (initialization only)                  */
/* input[3]    - input[1] value                                         */
/* input[4]    - Resistor1 value (initialization only)                  */
/* input[5]    - Capacitor Value (initialization only)                  */
/*                                                                      */
/************************************************************************/

struct dss_rcdisc2_context
{
	double x1;		/* x[k-1], last input value */
	double y1;		/* y[k-1], last output value */
	double a1_0, b0_0, b1_0;	/* digital filter coefficients, filter #1 */
	double a1_1, b0_1, b1_1;	/* digital filter coefficients, filter #2 */
};

void dst_rcdisc2N_step(struct node_description *node)
{
	struct dss_rcdisc2_context *context = node->context;
	double input = ((node->input[0]==0)?node->input[1]:node->input[3]);

	if (node->input[0] == 0)
		node->output = -context->a1_0*context->y1 + context->b0_0*input + context->b1_0*context->x1;
	else
		node->output = -context->a1_1*context->y1 + context->b0_1*input + context->b1_1*context->x1;

	context->x1 = input;
	context->y1 = node->output;
}

void dst_rcdisc2N_reset(struct node_description *node)
{
	struct dss_rcdisc2_context *context = node->context;
	double f1,f2;

	f1=1.0/(2*M_PI*node->input[2]*node->input[5]);
	f2=1.0/(2*M_PI*node->input[4]*node->input[5]);

	calculate_filter1_coefficients(f1, DISC_FILTER_LOWPASS, &context->a1_0, &context->b0_0, &context->b1_0);
	calculate_filter1_coefficients(f2, DISC_FILTER_LOWPASS, &context->a1_1, &context->b0_1, &context->b1_1);

	/* Initialize the object */
	node->output=0;
}
