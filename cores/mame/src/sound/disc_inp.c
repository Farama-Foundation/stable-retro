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
/* DSS_ADJUSTMENT        - UI Mapped adjustable input                   */
/* DSS_CONSTANT          - Node based constant - Do we need this ???    */
/* DSS_INPUT             - Memory Mapped input device                   */
/*                                                                      */
/************************************************************************/

#define DSS_INPUT_SPACE	0x1000

static struct node_description **dss_input_map=NULL;

struct dss_adjustment_context
{
	INT32		port;
	INT32		lastpval;
	INT32		pmin;
	double		pscale;
	double		min;
	double		scale;
	double		lastval;
};


READ_HANDLER(discrete_sound_r)
{
	int data=0;

	if (!Machine->sample_rate) return 0;

	discrete_sh_update();
	/* Mask the memory offset to stay in the space allowed */
	offset&=(DSS_INPUT_SPACE-1);
	/* Update the node input value if allowed */
	if(dss_input_map[offset])
	{
		data=dss_input_map[offset]->input[0];
	}
    return data;
}

WRITE_HANDLER(discrete_sound_w)
{
	if (!Machine->sample_rate) return;

	/* Bring the system upto now */
	discrete_sh_update();
	/* Mask the memory offset to stay in the space allowed */
	offset&=(DSS_INPUT_SPACE-1);
	/* Update the node input value if allowed */
	if(dss_input_map[offset])
	{
		dss_input_map[offset]->input[0]=data;
	}
}


/************************************************************************/
/*                                                                      */
/* DSS_ADJUSTMENT - UI Adjustable constant node to emulate trimmers     */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - Minimum value                                          */
/* input[2]    - Maximum value                                          */
/* input[3]    - Log/Linear 0=Linear !0=Log                             */
/* input[4]    - Input Port number                                      */
/* input[5]    -                                                        */
/* input[6]    -                                                        */
/*                                                                      */
/************************************************************************/
#define DSS_ADJUSTMENT_ENABLE	node->input[0]
#define DSS_ADJUSTMENT_MIN		node->input[1]
#define DSS_ADJUSTMENT_MAX		node->input[2]
#define DSS_ADJUSTMENT_LOG		node->input[3]
#define DSS_ADJUSTMENT_PORT		node->input[4]

void dss_adjustment_step(struct node_description *node)
{
	if (DSS_ADJUSTMENT_ENABLE)
	{
		struct dss_adjustment_context *context = node->context;
		INT32 rawportval = readinputport(context->port);
		
		/* only recompute if the value changed from last time */
		if (rawportval != context->lastpval)
		{
			double portval = (double)(rawportval - context->pmin) * context->pscale;
			double scaledval = portval * context->scale + context->min;

			context->lastpval = rawportval;
			if (DSS_ADJUSTMENT_LOG == 0)
				context->lastval = scaledval;
			else
				context->lastval = pow(10, scaledval);
		}
		node->output = context->lastval;
	}
	else
	{
		node->output = 0;
	}
}

void dss_adjustment_reset(struct node_description *node)
{
	struct dss_adjustment_context *context = node->context;

	context->port = DSS_ADJUSTMENT_PORT;
	context->lastpval = 0x7fffffff;
	context->pmin = node->input[5];
	context->pscale = 1.0 / (double)(node->input[6] - node->input[5]);
	
	/* linear scale */
	if (DSS_ADJUSTMENT_LOG == 0)
	{
		context->min = DSS_ADJUSTMENT_MIN;
		context->scale = DSS_ADJUSTMENT_MAX - DSS_ADJUSTMENT_MIN;
	}
	
	/* logarithmic scale */
	else
	{
		context->min = log10(DSS_ADJUSTMENT_MIN);
		context->scale = log10(DSS_ADJUSTMENT_MAX) - log10(DSS_ADJUSTMENT_MIN);
	}
	context->lastval = 0;
	
	dss_adjustment_step(node);
}


/************************************************************************/
/*                                                                      */
/* DSS_CONSTANT - This is a constant.                                   */
/*                                                                      */
/* input[0]    - Constant value                                         */
/*                                                                      */
/************************************************************************/
void dss_constant_step(struct node_description *node)
{
	node->output=node->input[0];
}


/************************************************************************/
/*                                                                      */
/* DSS_INPUT    - Receives input from discrete_sound_w                  */
/*                                                                      */
/* input[0]    - Constant value                                         */
/* input[1]    - Address value                                          */
/* input[2]    - Address mask                                           */
/* input[3]    - Gain value                                             */
/* input[4]    - Offset value                                           */
/* input[5]    - Starting Position                                      */
/*                                                                      */
/************************************************************************/
void dss_input_step(struct node_description *node)
{
	node->output=(node->input[0]*node->input[3])+node->input[4];
}

void dss_input_reset(struct node_description *node)
{
	int loop,addr,mask;

	/* Initialise the input mapping array for this particular node */
	addr=((int)node->input[1])&(DSS_INPUT_SPACE-1);
	mask=((int)node->input[2])&(DSS_INPUT_SPACE-1);
	for(loop=0;loop<DSS_INPUT_SPACE;loop++)
	{
		if((loop&mask)==addr) dss_input_map[loop]=node;
	}
	node->input[0]=node->input[5];
	dss_input_step(node);
}

void dss_input_pulse_step(struct node_description *node)
{
	/* Set a valid output */
	node->output=(node->input[0]*node->input[3])+node->input[4];
	/* Reset the input to default for the next cycle */
	/* node order is now important */
	node->input[0]=node->input[5];
}
