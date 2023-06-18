/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@esplexo.co.uk)                       */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*  (c) D.Renaud 2003-2004                                              */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DST_ADDDER            - Multichannel adder                           */
/* DST_COMP_ADDER        - Selectable parallel component circuit        */
/* DST_DAC_R1            - R1 Ladder DAC with cap smoothing             */
/* DST_DIVIDER           - Division function                            */
/* DST_GAIN              - Gain Factor                                  */
/* DST_LOGIC_INV         - Logic level invertor                         */
/* DST_LOGIC_AND         - Logic AND gate 4 input                       */
/* DST_LOGIC_NAND        - Logic NAND gate 4 input                      */
/* DST_LOGIC_OR          - Logic OR gate 4 input                        */
/* DST_LOGIC_NOR         - Logic NOR gate 4 input                       */
/* DST_LOGIC_XOR         - Logic XOR gate 2 input                       */
/* DST_LOGIC_NXOR        - Logic NXOR gate 2 input                      */
/* DST_LOGIC_DFF         - Logic D-type flip/flop                       */
/* DST_MIXER             - Final Mixer Stage                            */
/* DST_ONESHOT           - One shot pulse generator                     */
/* DST_RAMP              - Ramp up/down                                 */
/* DST_SAMPHOLD          - Sample & Hold Implementation                 */
/* DST_SWITCH            - Switch implementation                        */
/* DST_TRANSFORM         - Multiple math functions                      */
/*                                                                      */
/************************************************************************/

#include <float.h>

struct dst_dac_r1_context
{
	double	iBias;		/* current of the bias circuit */
	double	exponent;	/* smoothing curve */
	double	rTotal;		/* all resistors in parallel */
};

struct dst_dflipflop_context
{
	int lastClk;
};

#define DISC_MIXER_MAX_INPS	8

struct dst_mixer_context
{
	int	type;
	double	rTotal;
	struct	node_description* rNode[DISC_MIXER_MAX_INPS];	/* Either pointer to input node OR NULL */
	double	exponent_rc[DISC_MIXER_MAX_INPS];	/* For high pass filtering cause by cIn */
	double	vCap[DISC_MIXER_MAX_INPS];		/* cap voltage of each input */
	double	exponent_cF;				/* Low pass on mixed inputs */
	double	exponent_cAmp;				/* Final high pass caused by out cap and amp input impedance */
	double	vCapF;					/* cap voltage of cF */
	double	vCapAmp;				/* cap voltage of cAmp */
	double	gain;					/* used for DISC_MIXER_IS_OP_AMP_WITH_RI */
};

struct dst_oneshot_context
{
	double countdown;
	double stepsize;
	int state;
	int lastTrig;
};

struct dss_ramp_context
{
	double step;
	int dir;		/* 1 if End is higher then Start */
	int last_en;	/* Keep track of the last enable value */
};

struct dst_samphold_context
{
	double lastinput;
	int clocktype;
};


/************************************************************************/
/*                                                                      */
/* DST_ADDER - This is a 4 channel input adder with enable function     */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Channel0 input value                                   */
/* input[2]    - Channel1 input value                                   */
/* input[3]    - Channel2 input value                                   */
/* input[4]    - Channel3 input value                                   */
/*                                                                      */
/************************************************************************/
void dst_adder_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=node->input[1] + node->input[2] + node->input[3] + node->input[4];
	}
	else
	{
		node->output=0;
	}
}


/************************************************************************/
/*                                                                      */
/* DST_COMP_ADDER  - Selectable parallel component adder                */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Bit Select                                             */
/*                                                                      */
/* Also passed discrete_comp_adder_table structure                      */
/*                                                                      */
/* Mar 2004, D Renaud.                                                  */
/************************************************************************/
void dst_comp_adder_step(struct node_description *node)
{
	const struct	discrete_comp_adder_table *info = node->custom;
	int bit;

	if(node->input[0])
	{
		switch (info->type)
		{
			case DISC_COMP_P_CAPACITOR:
				node->output = info->cDefault;
				for(bit=0; bit < info->length; bit++)
				{
					if ((int)node->input[1] & (1 << bit)) node->output += info->c[bit];
				}
				break;
			case DISC_COMP_P_RESISTOR:
				node->output = info->cDefault ? 1.0 / info->cDefault : 0;
				for(bit=0; bit < info->length; bit++)
				{
					if ((int)node->input[1] & (1 << bit)) node->output += 1.0 / info->c[bit];
				}
				if (node->output != 0) node->output = 1.0 / node->output;
				break;
		}
	}
	else
	{
		node->output = 0;
	}
}


/************************************************************************/
/*                                                                      */
/* DST_CLAMP - Simple signal clamping circuit                           */
/*                                                                      */
/* input[0]    - Enable ramp                                            */
/* input[1]    - Input value                                            */
/* input[2]    - Minimum value                                          */
/* input[3]    - Maximum value                                          */
/* input[4]    - Clamp when disabled                                    */
/*                                                                      */
/************************************************************************/
void dst_clamp_step(struct node_description *node)
{
/*	struct dss_ramp_context *context = node->context; */

	if(node->input[0])
	{
		if(node->input[1] < node->input[2]) node->output=node->input[2];
		else if(node->input[1] > node->input[3]) node->output=node->input[3];
		else node->output=node->input[1];
	}
	else
	{
		node->output=node->input[4];
	}
}


/************************************************************************/
/*                                                                      */
/* DST_DAC_R1 - R1 Ladder DAC with cap smoothing                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - Binary Data Input                                      */
/* input[2]    - Data On Voltage (3.4 for TTL)                          */
/*                                                                      */
/* also passed discrete_dac_r1_ladder structure                         */
/*                                                                      */
/* Mar 2004, D Renaud.                                                                    */
/************************************************************************/
#define DSTDACR1_ENABLE		node->input[0]
#define DSTDACR1_DATA		(int)node->input[1]
#define DSTDACR1_VON		node->input[2]

void dst_dac_r1_step(struct node_description *node)
{
	const struct discrete_dac_r1_ladder *info = node->custom;
	struct dst_dac_r1_context *context = node->context;

	int	bit;
	double	v;
	double	i;

	i = context->iBias;

	if (DSTDACR1_ENABLE)
	{
		for (bit=0; bit < info->ladderLength; bit++)
		{
			/* Add up currents of ON circuits per Millman. */
			/* Off, being 0V and having no current, can be ignored. */
			if (DSTDACR1_DATA & (1 << bit))
				i += DSTDACR1_VON / info->r[bit];
		}

		v = i * context->rTotal;

		/* Filter if needed, else just output voltage */
		node->output = info->cFilter ? node->output + ((v - node->output) * context->exponent) : v;
	}
	else
	{
		/*
		 * If module is disabled we will just leave the voltage where it was.
		 * We may want to set it to 0 in the future, but we will probably never
		 * disable this module.
		 */
	}
}

void dst_dac_r1_reset(struct node_description *node)
{
	const struct discrete_dac_r1_ladder *info = node->custom;
	struct dst_dac_r1_context *context = node->context;

	int	bit;

	/* Calculate the Millman current of the bias circuit */
	if (info->rBias)
		context->iBias = info->vBias / info->rBias;
	else
		context->iBias = 0;

	/*
	 * We will do a small amount of error checking.
	 * But if you are an idiot and pass a bad ladder table
	 * then you deserve a crash.
	 */
	if (info->ladderLength < 2)
	{
		/* You need at least 2 resistors for a ladder */
		discrete_log("dst_dac_r1_reset - Ladder length too small");
	}
	if (info->ladderLength > DISC_LADDER_MAXRES )
	{
		discrete_log("dst_dac_r1_reset - Ladder length exceeds DISC_LADDER_MAXRES");
	}

	/*
	 * Calculate the total of all resistors in parallel.
	 * This is the combined resistance of the voltage sources.
	 * This is used for the charging curve.
	 */
	context->rTotal = 0;
	for(bit=0; bit < info->ladderLength; bit++)
	{
		if (!info->r[bit])
		{
			discrete_log("dst_dac_r1_reset - Resistor can't equal 0");
		}
		context->rTotal += 1.0 / info->r[bit];
	}
	if (info->rBias) context->rTotal += 1.0 / info->rBias;
	if (info->rGnd) context->rTotal += 1.0 / info->rGnd;
	context->rTotal = 1.0 / context->rTotal;

	node->output = 0;

	if (info->cFilter)
	{
		/* Setup filter constants */
		context->exponent = -1.0 / (context->rTotal * info->cFilter  * Machine->sample_rate);
		context->exponent = 1.0 - exp(context->exponent);
	}
}


/************************************************************************/
/*                                                                      */
/* DST_DIVIDE  - Programmable divider with enable                       */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Channel0 input value                                   */
/* input[2]    - Divisor                                                */
/*                                                                      */
/************************************************************************/
void dst_divide_step(struct node_description *node)
{
	if(node->input[0])
	{
		if(node->input[2]==0)
		{
			node->output=DBL_MAX;	/* Max out but don't break */
			discrete_log("dst_divider_step() - Divide by Zero attempted.");
		}
		else
		{
			node->output=node->input[1] / node->input[2];
		}
	}
	else
	{
		node->output=0;
	}
}


/************************************************************************/
/*                                                                      */
/* DST_GAIN - This is a programmable gain module with enable function   */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Channel0 input value                                   */
/* input[2]    - Gain value                                             */
/* input[3]    - Final addition offset                                  */
/*                                                                      */
/************************************************************************/
void dst_gain_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=node->input[1] * node->input[2];
		node->output+=node->input[3];
	}
	else
	{
		node->output=0;
	}
}


/************************************************************************/
/*                                                                      */
/* DST_LOGIC_INV - Logic invertor gate implementation                   */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/*                                                                      */
/************************************************************************/
void dst_logic_inv_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1])?0.0:1.0;
	}
	else
	{
		node->output=0.0;
	}
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_AND - Logic AND gate implementation                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - input[2] value                                         */
/* input[4]    - input[3] value                                         */
/*                                                                      */
/************************************************************************/
void dst_logic_and_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1] && node->input[2] && node->input[3] && node->input[4])?1.0:0.0;
	}
	else
	{
		node->output=0.0;
	}
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_NAND - Logic NAND gate implementation                      */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - input[2] value                                         */
/* input[4]    - input[3] value                                         */
/*                                                                      */
/************************************************************************/
void dst_logic_nand_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1] && node->input[2] && node->input[3] && node->input[4])?0.0:1.0;
	}
	else
	{
		node->output=0.0;
	}
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_OR  - Logic OR  gate implementation                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - input[2] value                                         */
/* input[4]    - input[3] value                                         */
/*                                                                      */
/************************************************************************/
void dst_logic_or_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1] || node->input[2] || node->input[3] || node->input[4])?1.0:0.0;
	}
	else
	{
		node->output=0.0;
	}
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_NOR - Logic NOR gate implementation                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - input[2] value                                         */
/* input[4]    - input[3] value                                         */
/*                                                                      */
/************************************************************************/
void dst_logic_nor_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1] || node->input[2] || node->input[3] || node->input[4])?0.0:1.0;
	}
	else
	{
		node->output=0.0;
	}
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_XOR - Logic XOR gate implementation                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/*                                                                      */
/************************************************************************/
void dst_logic_xor_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=((node->input[1] && !node->input[2]) || (!node->input[1] && node->input[2]))?1.0:0.0;
	}
	else
	{
		node->output=0.0;
	}
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_NXOR - Logic NXOR gate implementation                      */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/*                                                                      */
/************************************************************************/
void dst_logic_nxor_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=((node->input[1] && !node->input[2]) || (!node->input[1] && node->input[2]))?0.0:1.0;
	}
	else
	{
		node->output=0.0;
	}
}


/************************************************************************/
/*                                                                      */
/* DST_LOGIC_DFF - Standard D-type flip-flop implementation             */
/*                                                                      */
/* input[0]    - /Reset                                                 */
/* input[1]    - /Set                                                   */
/* input[2]    - clock                                                  */
/* input[3]    - input                                                  */
/*                                                                      */
/************************************************************************/
void dst_logic_dff_step(struct node_description *node)
{
	struct dst_dflipflop_context *context = node->context;

	if (node->input[0])
	{
		if (!node->input[1])
			node->output = 0;
		else if (!node->input[2])
			node->output = 1;
		else if (!context->lastClk && node->input[3])
			node->output = node->input[4];
	}
	else
	{
		node->output=0.0;
	}
	context->lastClk = node->input[3];
}

void dst_logic_dff_reset(struct node_description *node)
{
	struct dst_dflipflop_context *context = node->context;
	context->lastClk = 0;
}


/************************************************************************/
/*                                                                      */
/* DST_MIXER  - Mixer/Gain stage                                        */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Input 1                                                */
/* input[2]    - Input 2                                                */
/* input[3]    - Input 3                                                */
/* input[4]    - Input 4                                                */
/* input[5]    - Input 5                                                */
/* input[6]    - Input 6                                                */
/* input[7]    - Input 7                                                */
/* input[8]    - Input 8                                                */
/*                                                                      */
/* Also passed discrete_mixer_info structure                            */
/*                                                                      */
/* Mar 2004, D Renaud.                                                  */
/************************************************************************/
/*
 * The input resistors can be a combination of static values and nodes.
 * If a node is used then its value is in series with the static value.
 * Also if a node is used and its value is 0, then that means the
 * input is disconnected from the circuit.
 *
 * There are 3 basic types of mixers, defined by the 2 types.  The
 * op amp mixer is further defined by the prescence of rI.  This is a
 * brief explaination.
 *
 * DISC_MIXER_IS_RESISTOR
 * The inputs are high pass filtered if needed, using (rX || rF) * cX.
 * Then Millman is used for the voltages.
 * r = (1/rF + 1/r1 + 1/r2...)
 * i = (v1/r1 + v2/r2...)
 * v = i * r
 *
 * DISC_MIXER_IS_OP_AMP - no rI
 * This is just a summing circuit.
 * The inputs are high pass filtered if needed, using rX * cX.
 * Then a modified Millman is used for the voltages.
 * i = ((vRef - v1)/r1 + (vRef - v2)/r2...)
 * v = i * rF
 *
 * DISC_MIXER_IS_OP_AMP_WITH_RI
 * The inputs are high pass filtered if needed, using (rX + rI) * cX.
 * Then Millman is used for the voltages including vRef/rI.
 * r = (1/rI + 1/r1 + 1/r2...)
 * i = (vRef/rI + v1/r1 + v2/r2...)
 * The voltage is then modified by an inverting amp formula.
 * v = vRef + (rF/rI) * (vRef - (i * r))
 */
#define DSTMIXER_ENABLE		node->input[0]

void dst_mixer_step(struct node_description *node)
{
	const struct discrete_mixer_desc *info = node->custom;
	struct dst_mixer_context *context = node->context;

	double	v, vTemp, rTotal, rTemp, rTemp2 = 0;
	double	i = 0;		/* total current of inputs */
	int	bit, connected;

	if (DSTMIXER_ENABLE)
	{
		rTotal = context->rTotal;

		for(bit=0; bit < info->mixerLength; bit++)
		{
			rTemp = info->r[bit];
			connected = 1;
			vTemp = node->input[bit + 1];

			if (info->rNode[bit])
			{
				/* a node has the posibility of being disconnected from the circuit. */
				if ((context->rNode[bit])->output == 0)
					connected = 0;
				else
				{
					rTemp += (context->rNode[bit])->output;
					rTotal += 1.0 / rTemp;
					if (info->c[bit] != 0)
					{
						switch (context->type & DISC_MIXER_TYPE_MASK)
						{
							case DISC_MIXER_IS_RESISTOR:
								rTemp2 = 1.0 / ((1.0 / rTemp) + (1.0 / info->rF));
								break;
							case DISC_MIXER_IS_OP_AMP:
								rTemp2 = rTemp;
								break;
							case DISC_MIXER_IS_OP_AMP_WITH_RI:
								rTemp2 = rTemp + info->rI;
								break;
						}
						/* Re-calculate exponent if resistor is a node */
						context->exponent_rc[bit] = -1.0 / (rTemp2 * info->c[bit]  * Machine->sample_rate);
						context->exponent_rc[bit] = 1.0 - exp(context->exponent_rc[bit]);
					}
				}
			}

			if (connected)
			{
				if (info->c[bit] != 0)
				{
					/* do input high pass filtering if needed. */
					context->vCap[bit] += (vTemp - info->vRef - context->vCap[bit]) * context->exponent_rc[bit];
					vTemp -= context->vCap[bit];
				}
				i += (((context->type & DISC_MIXER_TYPE_MASK) == DISC_MIXER_IS_OP_AMP) ? info->vRef - vTemp : vTemp) / rTemp;
			}
		}

		if ((context->type & DISC_MIXER_TYPE_MASK) == DISC_MIXER_IS_OP_AMP_WITH_RI) i += info->vRef / info->rI;
		rTotal = 1.0 / rTotal;

		/* If resistor network or has rI then Millman is used.
		 * If op-amp then summing formula is used. */
		v = i * (((context->type & DISC_MIXER_TYPE_MASK) == DISC_MIXER_IS_OP_AMP) ? info->rF : rTotal);

		if ((context->type & DISC_MIXER_TYPE_MASK) == DISC_MIXER_IS_OP_AMP_WITH_RI)
			v = info->vRef + (context->gain * (info->vRef - v));

		/* Do the low pass filtering for cF */
		if (info->cF != 0)
		{
			if (context->type & DISC_MIXER_HAS_R_NODE)
			{
				/* Re-calculate exponent if resistor nodes are used */
				context->exponent_cF = -1.0 / (rTotal * info->cF  * Machine->sample_rate);
				context->exponent_cF = 1.0 - exp(context->exponent_cF);
			}
			context->vCapF += (v -info->vRef - context->vCapF) * context->exponent_cF;
			v = context->vCapF;
		}

		/* Do the high pass filtering for cAmp */
		if (info->cAmp != 0)
		{
			context->vCapAmp += (v - context->vCapAmp) * context->exponent_cAmp;
			v -= context->vCapAmp;
		}
		node->output = v * info->gain;
	}
	else
	{
		node->output = 0;
	}
}

void dst_mixer_reset(struct node_description *node)
{
	const struct discrete_mixer_desc *info = node->custom;
	struct dst_mixer_context *context = node->context;

	int	bit;
	double	rTemp = 0;

	/*
	 * THERE IS NO ERROR CHECKING!!!!!!!!!
	 * If you are an idiot and pass a bad ladder table
	 * then you deserve a crash.
	 */

	context->type = ((info->type == DISC_MIXER_IS_OP_AMP) && info->rI) ? DISC_MIXER_IS_OP_AMP_WITH_RI : info->type;

	/*
	 * Calculate the total of all resistors in parallel.
	 * This is the combined resistance of the voltage sources.
	 * Also calculate the exponents while we are here.
	 */
	context->rTotal = 0;
	for(bit=0; bit < info->mixerLength; bit++)
	{
		if (info->rNode[bit])
		{
			context->type = context->type | DISC_MIXER_HAS_R_NODE;
			context->rNode[bit] = discrete_find_node(info->rNode[bit]);	/* get node pointers */
		}
		else
			context->rNode[bit] = NULL;

		if ((info->r[bit] != 0) && !info->rNode[bit] )
		{
			context->rTotal += 1.0 / info->r[bit];
		}

		context->vCap[bit] = 0;
		context->exponent_rc[bit] = 0;
		if ((info->c[bit] != 0)  && !info->rNode[bit])
		{
			switch (context->type)
			{
				case DISC_MIXER_IS_RESISTOR:
					rTemp = 1.0 / ((1.0 / info->r[bit]) + (1.0 / info->rF));
					break;
				case DISC_MIXER_IS_OP_AMP:
					rTemp = info->r[bit];
					break;
				case DISC_MIXER_IS_OP_AMP_WITH_RI:
					rTemp = info->r[bit] + info->rI;
					break;
			}
			/* Setup filter constants */
			context->exponent_rc[bit] = -1.0 / (rTemp * info->c[bit]  * Machine->sample_rate);
			context->exponent_rc[bit] = 1.0 - exp(context->exponent_rc[bit]);
		}
	}

	if (info->rF == 0)
	{
		/* You must have an rF */
		discrete_log("dst_mixer_reset - rF can't equal 0");
	}
	if (info->type == DISC_MIXER_IS_RESISTOR) context->rTotal += 1.0 / info->rF;
	if (context->type == DISC_MIXER_IS_OP_AMP_WITH_RI) context->rTotal += 1.0 / info->rI;

	context->vCapF = 0;
	context->exponent_cF = 0;
	if (info->cF != 0)
	{
		/* Setup filter constants */
		context->exponent_cF = -1.0 / (((info->type == DISC_MIXER_IS_OP_AMP) ? info->rF : (1.0 / context->rTotal))* info->cF  * Machine->sample_rate);
		context->exponent_cF = 1.0 - exp(context->exponent_cF);
	}

	context->vCapAmp = 0;
	context->exponent_cAmp = 0;
	if (info->cAmp != 0)
	{
		/* Setup filter constants */
		/* We will use 100000 ohms as an average final stage impedance. */
		/* Your amp/speaker system will have more effect on incorrect filtering then any value used here. */
		context->exponent_cAmp = -1.0 / (100000 * info->cAmp  * Machine->sample_rate);
		context->exponent_cAmp = 1.0 - exp(context->exponent_cAmp);
	}

	if ((context->type & DISC_MIXER_TYPE_MASK) == DISC_MIXER_IS_OP_AMP_WITH_RI) context->gain = info->rF / info->rI;

	node->output = 0;
}


/************************************************************************/
/*                                                                      */
/* DST_ONESHOT - Usage of node_description values for one shot pulse    */
/*                                                                      */
/* input[0]    - Reset value                                            */
/* input[1]    - Trigger value                                          */
/* input[2]    - Amplitude value                                        */
/* input[3]    - Width of oneshot pulse                                 */
/* input[4]    - type R/F edge, Retriggerable?                          */
/*                                                                      */
/* Complete re-write Jan 2004, D Renaud.                                */
/************************************************************************/
void dst_oneshot_step(struct node_description *node)
{
	struct dst_oneshot_context *context = node->context;
	int trigger = node->input[1] && node->input[1];

	/* If the state is triggered we will need to countdown later */
	int doCount = context->state;

	if (node->input[0])
	{
		/* Hold in Reset */
		node->output = 0;
		context->state = 0;
	}
	else
	{
		/* are we at an edge? */
		if (trigger != context->lastTrig)
		{
			/* There has been a trigger edge */
			context->lastTrig = trigger;

			/* Is it the proper edge trigger */
			if (((int)(node->input[4]) & DISC_ONESHOT_REDGE) ? trigger : !trigger)
			{
				if (!context->state)
				{
					/* We have first trigger */
					context->state = 1;
					node->output = ((int)(node->input[4]) & DISC_OUT_ACTIVE_LOW) ? 0 : node->input[2];
					context->countdown = node->input[3];
				}
				else
				{
					/* See if we retrigger */
					if ((int)(node->input[4]) & DISC_ONESHOT_RETRIG)
					{
						/* Retrigger */
						context->countdown = node->input[3];
						doCount = 0;
					}
				}
			}
		}

		if (doCount)
		{
			context->countdown -= context->stepsize;
			if(context->countdown <= 0.0)
			{
				node->output = ((int)(node->input[4]) & DISC_OUT_ACTIVE_LOW) ? node->input[2] : 0;
				context->countdown = 0;
				context->state = 0;
			}
		}
	}
}


void dst_oneshot_reset(struct node_description *node)
{
	struct dst_oneshot_context *context = node->context;
	context->countdown = 0;
	context->stepsize = 1.0 / Machine->sample_rate;
	context->state = 0;
 	/* We will make the initial last trigger equal to the current trigger.
 	 * This is so an output is not triggered on startup. */
 	context->lastTrig = node->input[1] && node->input[1]; 
 	node->output = ((int)(node->input[4]) & DISC_OUT_ACTIVE_LOW) ? node->input[2] : 0;
}


/************************************************************************/
/*                                                                      */
/* DST_RAMP - Ramp up/down model usage                                  */
/*                                                                      */
/* input[0]    - Enable ramp                                            */
/* input[1]    - Ramp Reverse/Forward switch                            */
/* input[2]    - Gradient, change/sec                                   */
/* input[3]    - Start value                                            */
/* input[4]    - End value                                              */
/* input[5]    - Clamp value when disabled                              */
/*                                                                      */
/************************************************************************/

void dst_ramp_step(struct node_description *node)
{
	struct dss_ramp_context *context = node->context;

	if(node->input[0])
	{
		if (!context->last_en)
		{
			context->last_en = 1;
			node->output = node->input[3];
		}
		if(context->dir ? node->input[1] : !node->input[1]) node->output+=context->step;
		else node->output-=context->step;
		/* Clamp to min/max */
		if(context->dir ? (node->output < node->input[3])
				: (node->output > node->input[3])) node->output=node->input[3];
		if(context->dir ? (node->output > node->input[4])
				: (node->output < node->input[4])) node->output=node->input[4];
	}
	else
	{
		context->last_en = 0;
		/* Disabled so clamp to output */
		node->output=node->input[5];
	}
}

void dst_ramp_reset(struct node_description *node)
{
	struct dss_ramp_context *context = node->context;

	node->output=node->input[5];
	context->step = node->input[2] / Machine->sample_rate;
	context->dir = ((node->input[4] - node->input[3]) == abs(node->input[4] - node->input[3]));
	context->last_en = 0;
}


/************************************************************************/
/*                                                                      */
/* DST_SAMPHOLD - Sample & Hold Implementation                          */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - clock node                                             */
/* input[3]    - clock type                                             */
/*                                                                      */
/************************************************************************/
void dst_samphold_step(struct node_description *node)
{
	struct dst_samphold_context *context = node->context;

	if(node->input[0])
	{
		switch(context->clocktype)
		{
			case DISC_SAMPHOLD_REDGE:
				/* Clock the whole time the input is rising */
				if(node->input[2] > context->lastinput) node->output=node->input[1];
				break;
			case DISC_SAMPHOLD_FEDGE:
				/* Clock the whole time the input is falling */
				if(node->input[2] < context->lastinput) node->output=node->input[1];
				break;
			case DISC_SAMPHOLD_HLATCH:
				/* Output follows input if clock != 0 */
				if(node->input[2]) node->output=node->input[1];
				break;
			case DISC_SAMPHOLD_LLATCH:
				/* Output follows input if clock == 0 */
				if(node->input[2]==0) node->output=node->input[1];
				break;
			default:
				discrete_log("dst_samphold_step - Invalid clocktype passed");
				break;
		}
	}
	else
	{
		node->output=0;
	}
	/* Save the last value */
	context->lastinput=node->input[2];
}

void dst_samphold_reset(struct node_description *node)
{
	struct dst_samphold_context *context = node->context;

	node->output=0;
	context->lastinput=-1;
	/* Only stored in here to speed up and save casting in the step function */
	context->clocktype=(int)node->input[3];
	dst_samphold_step(node);
}


/************************************************************************/
/*                                                                      */
/* DSS_SWITCH - Programmable 2 pole switch module with enable function  */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - switch position                                        */
/* input[2]    - input[0]                                               */
/* input[3]    - input[1]                                               */
/*                                                                      */
/************************************************************************/
void dst_switch_step(struct node_description *node)
{
	if(node->input[0])
	{
		/* Input 1 switches between input[0]/input[2] */
		node->output=(node->input[1])?node->input[3]:node->input[2];
	}
	else
	{
		node->output=0;
	}
}


/************************************************************************/
/*                                                                      */
/* DST_TRANSFORM - Programmable math module with enable function        */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Channel0 input value                                   */
/* input[2]    - Channel1 input value                                   */
/* input[3]    - Channel2 input value                                   */
/* input[4]    - Channel3 input value                                   */
/* input[5]    - Channel4 input value                                   */
/*                                                                      */
/************************************************************************/

#define MAX_TRANS_STACK	10

double dst_transform_pop(double *stack,int *pointer)
{
	double value;
	/*decrement THEN read */
	if(*pointer>0) (*pointer)--;
	value=stack[*pointer];
	return value;
}

double dst_transform_push(double *stack,int *pointer,double value)
{
	/*Store THEN increment */
	if(*pointer<MAX_TRANS_STACK) stack[(*pointer)++]=value;
	return value;
}

void dst_transform_step(struct node_description *node)
{
	if(node->input[0])
	{
		double trans_stack[MAX_TRANS_STACK];
		double result,number1,number2;
		int	trans_stack_ptr=0;

		const char *fPTR = node->custom;
		node->output=0;

		while(*fPTR!=0)
		{
			switch (*fPTR++)
			{
				case '*':
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1*number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '/':
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1/number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '+':
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1+number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '-':
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1-number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case 'i':	/* * -1 */
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=-number1;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '!':	/* Logical NOT of Last Value */
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=!number1;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '=':	/* Logical = */
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=(int)number1 == (int)number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '>':	/* Logical > */
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1 > number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '<':	/* Logical < */
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1 < number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '&':	/* Bitwise AND */
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=(int)number1 & (int)number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '|':	/* Bitwise OR */
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=(int)number1 | (int)number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '^':	/* Bitwise XOR */
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=(int)number1 ^ (int)number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '0':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[1]);
					break;
				case '1':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[2]);
					break;
				case '2':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[3]);
					break;
				case '3':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[4]);
					break;
				case '4':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[5]);
					break;
				default:
					discrete_log("dst_transform_step - Invalid function type/variable passed");
					node->output = 0;
					break;
			}
		}
		node->output=dst_transform_pop(trans_stack,&trans_stack_ptr);
	}
	else
	{
		node->output=0;
	}
}
