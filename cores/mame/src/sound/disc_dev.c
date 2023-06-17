/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@dysfunction.demon.co.uk)             */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*  (c) D.Renaud 2003-2004                                              */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DSD_555_ASTBL         - NE555 Simulation code                        */
/* DSD_555_CC            - NE555 Constant Current VCO                   */
/* DSD_566               - NE566 Simulation code                        */
/*                                                                      */
/************************************************************************/

struct dsd_555_astbl_context
{
	int		flip_flop;	/* 555 flip/flop output state */
	double	vCap;		/* voltage on cap */
	double	step;		/* time for sampling rate */
	double	threshold;
	double	trigger;
};

struct dsd_555_cc_context
{
	unsigned int	type;		/* type of 555cc circuit */
	unsigned int	state[2];	/* keeps track of excess flip_flop changes during the current step */
	int				flip_flop;	/* 555 flip/flop output state */
	double			vCap;		/* voltage on cap */
	double			step;		/* time for sampling rate */
};

struct dsd_566_context
{
	unsigned int	state[2];	/* keeps track of excess flip_flop changes during the current step */
	int			flip_flop;		/* 566 flip/flop output state */
	double		vCap;			/* voltage on cap */
	double		step;			/* time for sampling rate */
	double		vDiff;			/* voltage difference between vPlus and vNeg */
	double		vSqrLow;		/* voltage for a squarewave at low */
	double		vSqrHigh;		/* voltage for a squarewave at high */
	double		thresholdLow;	/* falling threshold */
	double		thresholdHigh;	/* rising threshold */
	double		triOffset;		/* used to shift a triangle to AC */
};




/************************************************************************/
/*                                                                      */
/* DSD_555_ASTBL - Usage of node_description values for 555 function    */
/*                                                                      */
/* input[0]    - Reset value                                            */
/* input[1]    - R1 value                                               */
/* input[2]    - R2 value                                               */
/* input[3]    - C value                                                */
/* input[4]    - Control Voltage value                                  */
/*                                                                      */
/* also passed discrete_555_astbl_desc structure                        */
/*                                                                      */
/* Jan 2004, D Renaud.                                                  */
/************************************************************************/
#define DSD_555_ASTBL_RESET	!node->input[0]
#define DSD_555_ASTBL_R1	node->input[1]
#define DSD_555_ASTBL_R2	node->input[2]
#define DSD_555_ASTBL_C		node->input[3]
#define DSD_555_ASTBL_CTRLV	node->input[4]

void dsd_555_astbl_step(struct node_description *node)
{
	const struct discrete_555_astbl_desc *info = node->custom;
	struct dsd_555_astbl_context *context = node->context;

	double dt;	/* change in time */
	double tRC;	/* RC time constant */
	double vC;	/* Current voltage on capacitor, before dt */
	double vCnext = 0;	/* Voltage on capacitor, after dt */

	if(DSD_555_ASTBL_RESET)
	{
		/* We are in RESET */
		node->output = 0;
		context->flip_flop = 1;
		context->vCap = 0;
	}
	else
	{
		/* Check: if the Control Voltage node is connected, calculate thresholds based on Control Voltage */
		if (DSD_555_ASTBL_CTRLV != NODE_NC)
		{
			context->threshold = DSD_555_ASTBL_CTRLV;
			context->trigger = DSD_555_ASTBL_CTRLV / 2.0;
		}

		/* Calculate future capacitor voltage.
		 * ref@ http://www.physics.rutgers.edu/ugrad/205/capacitance.html 
		 * The formulas from the ref pages have been modified to reflect that we are stepping the change.
		 * dt = time of sample (1/sample frequency)
		 * VC = Voltage across capacitor
		 * VC' = Future voltage across capacitor
		 * Vc = Voltage change
		 * Vr = is the voltage across the resistor.  For charging it is Vcc - VC.  Discharging it is VC - 0.
		 * R = R1+R2 (for charging)  R = R2 for discharging.
		 * Vc = Vr*(1-exp(-dt/(R*C)))
		 * VC' = VC + Vc (for charging) VC' = VC - Vc for discharging.
		 *
		 * We will also need to calculate the amount of time we overshoot the thresholds
		 * dt = amount of time we overshot
		 * Vc = voltage change overshoot
		 * dt = R*C(log(1/(1-(Vc/Vr))))
		 */

		dt = context->step;
		vC = context->vCap;

		/* Sometimes a switching network is used to setup the capacitance.
		 * These may select no capacitor, causing oscillation to stop.
		 */
		if (DSD_555_ASTBL_C == 0)
		{
			context->flip_flop = 1;
			/* The voltage goes high because the cap circuit is open. */
			vCnext = info->v555;
			vC = info->v555;
			context->vCap = 0;
		}
		else
		{
			/* Keep looping until all toggling in time sample is used up. */
			do
			{
				if (context->flip_flop)
				{
					/* Charging */
					tRC = (DSD_555_ASTBL_R1 + DSD_555_ASTBL_R2) * DSD_555_ASTBL_C;
					vCnext = vC + ((info->v555 - vC) * (1.0 - exp(-(dt / tRC))));
					dt = 0;

					/* has it charged past upper limit? */
					if (vCnext >= context->threshold)
					{
						if (vCnext > context->threshold)
						{
							/* calculate the overshoot time */
							dt = tRC * log(1.0 / (1.0 - ((vCnext - context->threshold) / (info->v555 - vC))));
						}
						vC = context->threshold;
						context->flip_flop = 0;
					}
				}
				else
				{
					/* Discharging */
					tRC = DSD_555_ASTBL_R2 * DSD_555_ASTBL_C;
					vCnext = vC - (vC * (1 - exp(-(dt / tRC))));
					dt = 0;

					/* has it discharged past lower limit? */
					if (vCnext <= context->trigger)
					{
						if (vCnext < context->trigger)
						{
							/* calculate the overshoot time */
							dt = tRC * log(1.0 / (1.0 - ((context->trigger - vCnext) / vC)));
						}
						vC = context->trigger;
						context->flip_flop = 1;
					}
				}
			} while(dt);

			context->vCap = vCnext;
		}

		switch (info->options & DISC_555_OUT_MASK)
		{
			case DISC_555_OUT_SQW:
				node->output = context->flip_flop * info->v555high;
				break;
			case DISC_555_OUT_CAP:
				node->output = vCnext;
				break;
			case DISC_555_OUT_CAP_CLAMP:
				/* vC will be at one of the thresholds if a state change happened. */
				node->output = vC;
				break;
		}
		/* Fake it to AC if needed */
		if (info->options & DISC_555_OUT_AC)
			node->output -= (info->options & DISC_555_OUT_MASK) ? context->threshold * 3.0 /4.0 : info->v555high / 2.0;
	}
}

void dsd_555_astbl_reset(struct node_description *node)
{
	const struct discrete_555_astbl_desc *info = node->custom;
	struct dsd_555_astbl_context *context = node->context;

	/* This will preset the thresholds if the Control Voltage is not used. */
	context->threshold = info->threshold555;
	context->trigger = info->trigger555;

	context->flip_flop = 1;
	context->vCap = 0;
	context->step = 1.0 / Machine->sample_rate;

	/* Step to set the output */
	dsd_555_astbl_step(node);
}


/************************************************************************/
/*                                                                      */
/* DSD_555_CC - Usage of node_description values                        */
/*                                                                      */
/* input[0]    - Reset input value                                      */
/* input[1]    - Voltage input for Constant current source.             */
/* input[2]    - R value to set CC current.                             */
/* input[3]    - C value                                                */
/* input[4]    - rBias value                                            */
/* input[5]    - rGnd value                                             */
/* input[6]    - rDischarge value                                       */
/*                                                                      */
/* also passed discrete_555_cc_desc structure                           */
/*                                                                      */
/* Mar 2004, D Renaud.                                                  */
/************************************************************************/
#define DSD_555_CC_RESET	!node->input[0]
#define DSD_555_CC_VIN		node->input[1]
#define DSD_555_CC_R		node->input[2]
#define DSD_555_CC_C		node->input[3]
#define DSD_555_CC_RBIAS	node->input[4]
#define DSD_555_CC_RGND		node->input[5]
#define DSD_555_CC_RDIS		node->input[6]

void dsd_555_cc_step(struct node_description *node)
{
	const struct discrete_555_cc_desc *info = node->custom;
	struct dsd_555_cc_context *context = node->context;

	double i;	/* Charging current created by vIn */
	double rC = 0;	/* Equivalent charging resistor */
	double rD = 0;	/* Equivalent discharging resistor */
	double vi = 0;	/* Equivalent voltage from current source */
	double vB = 0;	/* Equivalent voltage from bias voltage */
	double v  = 0;	/* Equivalent voltage total from current source and bias circuit if used */
	double dt;	/* change in time */
	double tRC;	/* RC time constant */
	double vC;	/* Current voltage on capacitor, before dt */
	double vCnext = 0;	/* Voltage on capacitor, after dt */
	double viLimit;	/* vIn and the junction voltage limit the max charging voltage from i */
	double rTemp;	/* play thing */


	if (DSD_555_CC_RESET)
	{
		/* 555 held in reset */
		node->output = 0;
		context->flip_flop = 1;
		context->vCap = 0;
		context->state[0] = 0;
		context->state[1] = 0;
	}
	else
	{
		dt = context->step;	/* Change in time */
		vC = context->vCap;	/* Set to voltage before change */
		viLimit = DSD_555_CC_VIN + info->vCCjunction;	/* the max vC can be and still be charged by i */
		/* Calculate charging current */
		i = (info->vCCsource - viLimit) / DSD_555_CC_R;

		switch (context->type)	/* see dsd_555_cc_reset for descriptions */
		{
			case 1:
				rD = DSD_555_CC_RDIS;
			case 0:
				break;
			case 3:
				rD = (DSD_555_CC_RDIS * DSD_555_CC_RGND) / (DSD_555_CC_RDIS + DSD_555_CC_RGND);
			case 2:
				rC = DSD_555_CC_RGND;
				vi = i * rC;
				break;
			case 4:
				rC = DSD_555_CC_RBIAS;
				vi = i * rC;
				vB = info->v555;
				break;
			case 5:
				rC = DSD_555_CC_RBIAS + DSD_555_CC_RDIS;
				vi = i * DSD_555_CC_RBIAS;
				vB = info->v555;
				rD = DSD_555_CC_RDIS;
				break;
			case 6:
				rC = (DSD_555_CC_RBIAS * DSD_555_CC_RGND) / (DSD_555_CC_RBIAS + DSD_555_CC_RGND);
				vi = i * rC;
				vB = info->v555 * (DSD_555_CC_RGND / (DSD_555_CC_RBIAS + DSD_555_CC_RGND));
				break;
			case 7:
				rTemp = DSD_555_CC_RBIAS + DSD_555_CC_RDIS;
				rC = (rTemp * DSD_555_CC_RGND) / (rTemp + DSD_555_CC_RGND);
				rTemp += DSD_555_CC_RGND;
				rTemp = DSD_555_CC_RGND / rTemp;	/* now has voltage divider ratio, not resistance */
				vi = i * DSD_555_CC_RBIAS * rTemp;
				vB = info->v555 * rTemp;
				rD = (DSD_555_CC_RGND * DSD_555_CC_RDIS) / (DSD_555_CC_RGND + DSD_555_CC_RDIS);
				break;
		}

		/* Keep looping until all toggling in time sample is used up. */
		do
		{
			if (context->type <= 1)
			{
				/* Standard constant current charge */
				if (context->flip_flop)
				{
					/* Charging */
					/* iC=C*dv/dt  works out to dv=iC*dt/C */
					vCnext = vC + (i * dt / DSD_555_CC_C);
					/* Yes, if the cap voltage has reached the max voltage it can,
					 * and the 555 threshold has not been reached, then oscillation stops.
					 * This is the way the actual electronics works.
					 * This is why you never play with the pots after being factory adjusted
					 * to work in the proper range. */
					if (vCnext > viLimit) vCnext = viLimit;
					dt = 0;

					/* has it charged past upper limit? */
					if (vCnext >= info->threshold555)
					{
						if (vCnext > info->threshold555)
						{
							/* calculate the overshoot time */
							dt = DSD_555_CC_C * (vCnext - info->threshold555) / i;
						}
						vC = info->threshold555;
						context->flip_flop = 0;

						/*
						 * If the sampling rate is too low and the desired frequency is too high
						 * then we will start getting too many outputs that can't catch up.  We will
						 * limit this to 3.  The output is already incorrect because of the low sampling,
						 * but at least this way it can recover.
						 */
						context->state[0] = (context->state[0] + 1) & 0x03;
					}
				}
				else if (DSD_555_CC_RDIS)
				{
					/* Discharging */
					tRC = DSD_555_CC_RDIS * DSD_555_CC_C;
					vCnext = vC - (vC * (1.0 - exp(-(dt / tRC))));
					dt = 0;

					/* has it discharged past lower limit? */
					if (vCnext <= info->trigger555)
					{
						if (vCnext < info->trigger555)
						{
							/* calculate the overshoot time */
							dt = tRC * log(1.0 / (1.0 - ((info->trigger555 - vCnext) / vC)));
						}
						vC = info->trigger555;
						context->flip_flop = 1;
						context->state[1] = (context->state[1] + 1) & 0x03;
					}
				}
				else	/* Immediate discharge. No change in dt.  */
				{
					vC = info->trigger555;
					context->flip_flop = 1;
					context->state[1] = (context->state[1] + 1) & 0x03;
				}
			}
			else
			{
				/* The constant current gets changed to a voltage due to a load resistor. */
				if (context->flip_flop)
				{
					/* Charging */
					/* If the cap voltage is past the current source charging limit
					 * then only the bias voltage will charge the cap. */
					v = vB;
					if (vC < viLimit) v += vi;
					else if (context->type <= 3) v = viLimit;
							
					tRC = rC * DSD_555_CC_C;
					vCnext = vC + ((v - vC) * (1.0 - exp(-(dt / tRC))));
					dt = 0;

					/* has it charged past upper limit? */
					if (vCnext >= info->threshold555)
					{
						if (vCnext > info->threshold555)
						{
							/* calculate the overshoot time */
							dt = tRC * log(1.0 / (1.0 - ((vCnext - info->threshold555) / (v - vC))));
						}
						vC = info->threshold555;
						context->flip_flop = 0;
						context->state[0] = (context->state[0] + 1) & 0x03;
					}
				}
				else if (rD)
				{
					/* Discharging */
					tRC = rD * DSD_555_CC_C;
					vCnext = vC - (vC * (1.0 - exp(-(dt / tRC))));
					dt = 0;

					/* has it discharged past lower limit? */
					if (vCnext <= info->trigger555)
					{
						if (vCnext < info->trigger555)
						{
							/* calculate the overshoot time */
							dt = tRC * log(1.0 / (1.0 - ((info->trigger555 - vCnext) / vC)));
						}
						vC = info->trigger555;
						context->flip_flop = 1;
						context->state[1] = (context->state[1] + 1) & 0x03;
					}
				}
				else	/* Immediate discharge. No change in dt.  */
				{
					vC = info->trigger555;
					context->flip_flop = 1;
					context->state[1] = (context->state[1] + 1) & 0x03;
				}
			}
		} while(dt);

		context->vCap = vCnext;

		switch (info->options & DISC_555_OUT_MASK)
		{
			case DISC_555_OUT_SQW:
				/* use up any output states */
				if (node->output && context->state[0])
				{
					node->output = 0;
					context->state[0]--;
				}
				else if (!node->output && context->state[1])
				{
					node->output = 1;
					context->state[1]--;
				}
				else
				{
					node->output = context->flip_flop;
				}
				node->output = node->output * info->v555high;
				break;
			case DISC_555_OUT_CAP:
				/* we can ignore any unused states when
				 * outputting the cap voltage */
				node->output = vCnext;
				break;
			case DISC_555_OUT_CAP_CLAMP:
				/* vC will be at one of the thresholds if a state change happened. */
				node->output = vC;
				break;
		}
		/* Fake it to AC if needed */
		if (info->options & DISC_555_OUT_AC)
			node->output -= (info->options & DISC_555_OUT_MASK) ? info->threshold555 * 3.0 /4.0 : info->v555high / 2.0;
	}
}

void dsd_555_cc_reset(struct node_description *node)
{
	struct dsd_555_cc_context *context = node->context;

	context->flip_flop=1;
	context->vCap = 0;
	context->step = 1.0 / Machine->sample_rate;
	context->state[0] = 0;
	context->state[1] = 0;

	/* There are 8 different types of basic oscillators
	 * depending on the resistors used.  We will determine
	 * the type of circuit at reset, because the ciruit type
	 * is constant. */
	context->type = (int)(DSD_555_CC_RDIS && DSD_555_CC_RDIS) | ((int)(DSD_555_CC_RGND && DSD_555_CC_RGND) << 1) | ((int)(DSD_555_CC_RBIAS && DSD_555_CC_RBIAS) << 2);
	/*
	 * TYPES:
	 * Note: These are equivalent circuits shown without the 555 circuitry.
	 *       See the schematic in src\sound\discrete.h for full hookup info.
	 *
	 * [0]
	 * No resistors.  Straight constant current charge of capacitor.
	 *   .------+---> vCap      CHARGING:
	 *   |      |                 dv (change in voltage) compared to dt (change in time in seconds).
	 * .---.   ---                dv = i * dt / C; where i is current in amps and C is capacitance in farads.
	 * | i |   --- C              vCap = vCap + dv
	 * '---'    |
	 *   |      |               DISCHARGING:
	 *  gnd    gnd                instantaneous
	 *
	 * [1]
	 * Same as type 1 but with rDischarge.  rDischarge has no effect on the charge rate because
	 * of the constant current source i.
	 *   .----ZZZ-----+---> vCap      CHARGING:
	 *   | rDischarge |                 dv (change in voltage) compared to dt (change in time in seconds).
	 * .---.         ---                dv = i * dt / C; where i is current in amps and C is capacitance in farads.
	 * | i |         --- C              vCap = vCap + dv
	 * '---'          |
	 *   |            |               DISCHARGING:
	 *  gnd          gnd                thru rDischarge
	 *
	 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	 * !!!!! IMPORTANT NOTE ABOUT TYPES 3 - 7 !!!!!
	 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	 *
	 * From here on in all the circuits have either an rBias or rGnd resistor.
	 * This converts the constant current into a voltage source.
	 * So all the remaining circuit types will be converted to this circuit.
	 * When discharging, rBias is out of the equation because the 555 is grounding the circuit
	 * after that point.
	 *
	 * .------------.     Rc                  Rc is the equivilent circuit resistance.
	 * |     v      |----ZZZZ---+---> vCap    v  is the equivilent circuit voltage.
	 * |            |           |
	 * '------------'          ---            Then the standard RC charging formula applies.
	 *       |                 --- C
	 *       |                  |             NOTE: All the following types are converted to Rc and v values.
	 *      gnd                gnd
	 *
	 * [2]
	 *   .-------+------+------> vCap         CHARGING:
	 *   |       |      |                       v = vi = i * rGnd
	 * .---.    ---     Z                       Rc = rGnd
	 * | i |    --- C   Z rGnd
	 * '---'     |      |                     DISCHARGING:
	 *   |       |      |                       instantaneous
	 *  gnd     gnd    gnd
	 *
	 * [3]
	 *   .----ZZZ-----+------+------> vCap    CHARGING:
	 *   | rDischarge |      |                  v = vi = i * rGnd
	 * .---.         ---     Z                  Rc = rGnd
	 * | i |         --- C   Z rGnd
	 * '---'          |      |                DISCHARGING:
	 *   |            |      |                  thru rDischarge || rGnd  ( || means in parallel)
	 *  gnd          gnd    gnd
	 *
	 * [4]
	 *     .---ZZZ---+------------+-------------> vCap      CHARGING:
	 *     |  rBias  |            |                           Rc = rBias
	 * .-------.   .---.         ---                          vi = i * rBias
	 * | vBias |   | i |         --- C                        v = vBias + vi
	 * '-------'   '---'          |
	 *     |         |            |                         DISCHARGING:
	 *    gnd       gnd          gnd                          instantaneous
	 *
	 * [5]
	 *     .---ZZZ---+----ZZZ-----+-------------> vCap      CHARGING:
	 *     |  rBias  | rDischarge |                           Rc = rBias + rDischarge
	 * .-------.   .---.         ---                          vi = i * rBias
	 * | vBias |   | i |         --- C                        v = vBias + vi
	 * '-------'   '---'          |
	 *     |         |            |                         DISCHARGING:
	 *    gnd       gnd          gnd                          thru rDischarge
	 *
	 * [6]
	 *     .---ZZZ---+------------+------+------> vCap      CHARGING:
	 *     |  rBias  |            |      |                    Rc = rBias || rGnd
	 * .-------.   .---.         ---     Z                    vi = i * Rc
	 * | vBias |   | i |         --- C   Z rGnd               v = vBias * (rGnd / (rBias + rGnd)) + vi
	 * '-------'   '---'          |      |
	 *     |         |            |      |                  DISCHARGING:
	 *    gnd       gnd          gnd    gnd                   instantaneous
	 *
	 * [7]
	 *     .---ZZZ---+----ZZZ-----+------+------> vCap      CHARGING:
	 *     |  rBias  | rDischarge |      |                    Rc = (rBias + rDischarge) || rGnd
	 * .-------.   .---.         ---     Z                    vi = i * rBias * (rGnd / (rBias + rDischarge + rGnd))
	 * | vBias |   | i |         --- C   Z rGnd               v = vBias * (rGnd / (rBias + rDischarge + rGnd)) + vi
	 * '-------'   '---'          |      |
	 *     |         |            |      |                  DISCHARGING:
	 *    gnd       gnd          gnd    gnd                   thru rDischarge || rGnd
	 */

	/* Step to set the output */
	dsd_555_cc_step(node);
}


/************************************************************************/
/*                                                                      */
/* DSD_566 - Usage of node_description values                           */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Modulation Voltage                                     */
/* input[2]    - R value                                                */
/* input[3]    - C value                                                */
/*                                                                      */
/* also passed discrete_566_desc structure                              */
/*                                                                      */
/* Mar 2004, D Renaud.                                                  */
/************************************************************************/
#define DSD_566_ENABLE	node->input[0]
#define DSD_566_VMOD	node->input[1]
#define DSD_566_R	node->input[2]
#define DSD_566_C	node->input[3]

void dsd_566_step(struct node_description *node)
{
	const struct discrete_566_desc *info = node->custom;
	struct dsd_566_context *context = node->context;

	double i;	/* Charging current created by vIn */
	double dt;	/* change in time */
	double vC;	/* Current voltage on capacitor, before dt */
	double vCnext = 0;	/* Voltage on capacitor, after dt */

	if (DSD_566_ENABLE)
	{
		dt = context->step;	/* Change in time */
		vC = context->vCap;	/* Set to voltage before change */
		/* Calculate charging current */
		i = (context->vDiff - DSD_566_VMOD) / DSD_566_R;

		/* Keep looping until all toggling in time sample is used up. */
		do
		{
			if (context->flip_flop)
			{
				/* Discharging */
				vCnext = vC - (i * dt / DSD_566_C);
				dt = 0;

				/* has it discharged past lower limit? */
				if (vCnext <= context->thresholdLow)
				{
					if (vCnext < context->thresholdLow)
					{
						/* calculate the overshoot time */
						dt = DSD_566_C * (context->thresholdLow - vCnext) / i;
					}
					vC = context->thresholdLow;
					context->flip_flop = 0;
					/*
					 * If the sampling rate is too low and the desired frequency is too high
					 * then we will start getting too many outputs that can't catch up.  We will
					 * limit this to 3.  The output is already incorrect because of the low sampling,
					 * but at least this way it can recover.
					 */
					context->state[0] = (context->state[0] + 1) & 0x03;
				}
			}
			else
			{
				/* Charging */
				/* iC=C*dv/dt  works out to dv=iC*dt/C */
				vCnext = vC + (i * dt / DSD_566_C);
				dt = 0;
				/* Yes, if the cap voltage has reached the max voltage it can,
				 * and the 566 threshold has not been reached, then oscillation stops.
				 * This is the way the actual electronics works.
				 * This is why you never play with the pots after being factory adjusted
				 * to work in the proper range. */
				if (vCnext > DSD_566_VMOD) vCnext = DSD_566_VMOD;

				/* has it charged past upper limit? */
				if (vCnext >= context->thresholdHigh)
				{
					if (vCnext > context->thresholdHigh)
					{
						/* calculate the overshoot time */
						dt = DSD_566_C * (vCnext - context->thresholdHigh) / i;
					}
					vC = context->thresholdHigh;
					context->flip_flop = 1;
					context->state[1] = (context->state[1] + 1) & 0x03;
				}
			}
		} while(dt);

		context->vCap = vCnext;

		switch (info->options & DISC_566_OUT_MASK)
		{
			case DISC_566_OUT_SQUARE:
			case DISC_566_OUT_LOGIC:
				/* use up any output states */
				if (node->output && context->state[0])
				{
					node->output = 0;
					context->state[0]--;
				}
				else if (!node->output && context->state[1])
				{
					node->output = 1;
					context->state[1]--;
				}
				else
				{
					node->output = context->flip_flop;
				}
				if ((info->options & DISC_566_OUT_MASK) != DISC_566_OUT_LOGIC)
					node->output = context->flip_flop ? context->vSqrHigh : context->vSqrLow;
				break;
			case DISC_566_OUT_TRIANGLE:
				/* we can ignore any unused states when
				 * outputting the cap voltage */
				node->output = vCnext;
				if (info->options & DISC_566_OUT_AC)
					node->output -= context->triOffset;
				break;
		}
	}
	else
		node->output = 0;
}

void dsd_566_reset(struct node_description *node)
{
	const struct discrete_566_desc *info = node->custom;
	struct dsd_566_context *context = node->context;

	double	temp;

	context->vDiff = info->vPlus - info->vNeg;
	context->flip_flop = 0;
	context->vCap = 0;
	context->step = 1.0 / Machine->sample_rate;
	context->state[0] = 0;
	context->state[1] = 0;

	/* The data sheets are crap on this IC.  I will have to get my hands on a chip
	 * to make real measurements.  Until now this should work fine for 12V. */
	context->thresholdHigh = context->vDiff / 2 + info->vNeg;
	context->thresholdLow = context->thresholdHigh - (0.2 * context->vDiff);
	context->vSqrHigh = info->vPlus - 0.6;
	context->vSqrLow = context->thresholdHigh;

	if (info->options & DISC_566_OUT_AC)
	{
		temp = (context->vSqrHigh - context->vSqrLow) / 2;
		context->vSqrHigh = temp;
		context->vSqrLow = -temp;
		context->triOffset = context->thresholdHigh - (0.1 * context->vDiff);
	}

	/* Step the output */
	dsd_566_step(node);
}
