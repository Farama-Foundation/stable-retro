/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@esplexo.co.uk)                       */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*                                                                      */
/*  Coding started in November 2000                                     */
/*  KW - Added Sawtooth waveforms  Feb2003                              */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* SEE DISCRETE.H for documentation on usage                            */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* Each sound primative DSS_xxxx or DST_xxxx has its own implementation */
/* file. All discrete sound primatives MUST implement the following     */
/* API:                                                                 */
/*                                                                      */
/* dsX_NAME_step(inputs, context,float timestep)  - Perform time step   */
/*                                                  return output value */
/* dsX_NAME_reset(context) - Reset to initial state                     */
/*                                                                      */
/* Core software takes care of traversing the netlist in the correct    */
/* order                                                                */
/*                                                                      */
/* discrete_sh_start()       - Read Node list, initialise & reset       */
/* discrete_sh_stop()        - Shutdown discrete sound system           */
/* discrete_sh_reset()       - Put sound system back to time 0          */
/* discrete_sh_update()      - Update streams to current time           */
/* discrete_stream_update()  - This does the real update to the sim     */
/*                                                                      */
/************************************************************************/

#include "driver.h"
#include "wavwrite.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>


/*************************************
 *
 *	Debugging
 *
 *************************************/

#define DISCRETE_WAVELOG			(0)
#define DISCRETE_DEBUGLOG			(0)



/*************************************
 *
 *	Global variables
 *
 *************************************/

/* internal node tracking */
static int node_count;
static struct node_description **running_order;
static struct node_description **indexed_node;
static struct node_description *node_list;

/* output node tracking */
static int discrete_outputs;
static struct node_description *output_node[DISCRETE_MAX_OUTPUTS];

/* the output stream */
static int discrete_stream;

/* debugging statics */
static void *wav_file[DISCRETE_MAX_OUTPUTS];
static FILE *disclogfile = NULL;
extern int bailing;


/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void init_nodes(struct discrete_sound_block *block_list);
static void find_input_nodes(struct discrete_sound_block *block_list);
static void setup_output_nodes(void);



/*************************************
 *
 *	Debug logging
 *
 *************************************/

void CLIB_DECL discrete_log(const char *text, ...)
{
	if (DISCRETE_DEBUGLOG)
	{
		va_list arg;
		va_start(arg, text);

		if(disclogfile)
		{
			vfprintf(disclogfile, text, arg);
			fprintf(disclogfile, "\n");
		}

		va_end(arg);
	}
}



/*************************************
 *
 *	Included simulation objects
 *
 *************************************/

#include "disc_wav.c"		/* Wave sources   - SINE/SQUARE/NOISE/etc */
#include "disc_mth.c"		/* Math Devices   - ADD/GAIN/etc */
#include "disc_inp.c"		/* Input Devices  - INPUT/CONST/etc */
#include "disc_flt.c"		/* Filter Devices - RCF/HPF/LPF */
#include "disc_dev.c"		/* Popular Devices - NE555/etc */



/*************************************
 *
 *	Master module list
 *
 *************************************/

struct discrete_module module_list[] =
{
	{ DSO_OUTPUT      ,"DSO_OUTPUT"      ,0                                      ,NULL                  ,NULL                 },

	/* from disc_inp.c */
	{ DSS_ADJUSTMENT  ,"DSS_ADJUSTMENT"  ,sizeof(struct dss_adjustment_context)  ,dss_adjustment_reset  ,dss_adjustment_step  },
	{ DSS_CONSTANT    ,"DSS_CONSTANT"    ,0                                      ,NULL                  ,dss_constant_step    },
	{ DSS_INPUT       ,"DSS_INPUT"       ,0                                      ,dss_input_reset       ,dss_input_step       },
	{ DSS_INPUT_PULSE ,"DSS_INPUT_PULSE" ,0                                      ,dss_input_reset       ,dss_input_pulse_step },

	/* from disc_wav.c */
	/* Generic modules */
	{ DSS_COUNTER     ,"DSS_COUNTER"     ,sizeof(struct dss_counter_context)     ,dss_counter_reset     ,dss_counter_step     },
	{ DSS_COUNTER_FIX ,"DSS_COUNTER_FIX" ,sizeof(struct dss_counterfix_context)  ,dss_counterfix_reset  ,dss_counterfix_step  },
	{ DSS_LFSR_NOISE  ,"DSS_LFSR_NOISE"  ,sizeof(struct dss_lfsr_context)        ,dss_lfsr_reset        ,dss_lfsr_step        },
	{ DSS_NOISE       ,"DSS_NOISE"       ,sizeof(struct dss_noise_context)       ,dss_noise_reset       ,dss_noise_step       },
	{ DSS_SAWTOOTHWAVE,"DSS_SAWTOOTHWAVE",sizeof(struct dss_sawtoothwave_context),dss_sawtoothwave_reset,dss_sawtoothwave_step},
	{ DSS_SINEWAVE    ,"DSS_SINEWAVE"    ,sizeof(struct dss_sinewave_context)    ,dss_sinewave_reset    ,dss_sinewave_step    },
	{ DSS_SQUAREWAVE  ,"DSS_SQUAREWAVE"  ,sizeof(struct dss_squarewave_context)  ,dss_squarewave_reset  ,dss_squarewave_step  },
	{ DSS_SQUAREWFIX  ,"DSS_SQUAREWFIX"  ,sizeof(struct dss_squarewfix_context)  ,dss_squarewfix_reset  ,dss_squarewfix_step  },
	{ DSS_SQUAREWAVE2 ,"DSS_SQUAREWAVE2" ,sizeof(struct dss_squarewave_context)  ,dss_squarewave2_reset ,dss_squarewave2_step },
	{ DSS_TRIANGLEWAVE,"DSS_TRIANGLEWAVE",sizeof(struct dss_trianglewave_context),dss_trianglewave_reset,dss_trianglewave_step},
	/* Component specific modules */
	{ DSS_OP_AMP_OSC  ,"DSS_OP_AMP_OSC"  ,sizeof(struct dss_op_amp_osc_context)  ,dss_op_amp_osc_reset  ,dss_op_amp_osc_step  },
	{ DSS_SCHMITT_OSC ,"DSS_SCHMITT_OSC" ,sizeof(struct dss_schmitt_osc_context) ,dss_schmitt_osc_reset ,dss_schmitt_osc_step },
	/* Not yet implemented */
	{ DSS_ADSR        ,"DSS_ADSR"        ,sizeof(struct dss_adsr_context)        ,dss_adsrenv_reset     ,dss_adsrenv_step     },

	/* from disc_mth.c */
	/* Generic modules */
	{ DST_ADDER       ,"DST_ADDER"       ,0                                      ,NULL                  ,dst_adder_step       },
	{ DST_CLAMP       ,"DST_CLAMP"       ,0                                      ,NULL                  ,dst_clamp_step       },
	{ DST_DIVIDE      ,"DST_DIVIDE"      ,0                                      ,NULL                  ,dst_divide_step      },
	{ DST_GAIN        ,"DST_GAIN"        ,0                                      ,NULL                  ,dst_gain_step        },
	{ DST_LOGIC_INV   ,"DST_LOGIC_INV"   ,0                                      ,NULL                  ,dst_logic_inv_step   },
	{ DST_LOGIC_AND   ,"DST_LOGIC_AND"   ,0                                      ,NULL                  ,dst_logic_and_step   },
	{ DST_LOGIC_NAND  ,"DST_LOGIC_NAND"  ,0                                      ,NULL                  ,dst_logic_nand_step  },
	{ DST_LOGIC_OR    ,"DST_LOGIC_OR"    ,0                                      ,NULL                  ,dst_logic_or_step    },
	{ DST_LOGIC_NOR   ,"DST_LOGIC_NOR"   ,0                                      ,NULL                  ,dst_logic_nor_step   },
	{ DST_LOGIC_XOR   ,"DST_LOGIC_XOR"   ,0                                      ,NULL                  ,dst_logic_xor_step   },
	{ DST_LOGIC_NXOR  ,"DST_LOGIC_NXOR"  ,0                                      ,NULL                  ,dst_logic_nxor_step  },
	{ DST_LOGIC_DFF   ,"DST_LOGIC_DFF"   ,sizeof(struct dst_dflipflop_context)   ,dst_logic_dff_reset   ,dst_logic_dff_step   },
	{ DST_ONESHOT     ,"DST_ONESHOT"     ,sizeof(struct dst_oneshot_context)     ,dst_oneshot_reset     ,dst_oneshot_step     },
	{ DST_RAMP        ,"DST_RAMP"        ,sizeof(struct dss_ramp_context)        ,dst_ramp_reset        ,dst_ramp_step        },
	{ DST_SAMPHOLD    ,"DST_SAMPHOLD"    ,sizeof(struct dst_samphold_context)    ,dst_samphold_reset    ,dst_samphold_step    },
	{ DST_SWITCH      ,"DST_SWITCH"      ,0                                      ,NULL                  ,dst_switch_step      },
	{ DST_TRANSFORM   ,"DST_TRANSFORM"   ,0                                      ,NULL                  ,dst_transform_step   },
	/* Component specific */
	{ DST_COMP_ADDER  ,"DST_COMP_ADDER"  ,0                                      ,NULL                  ,dst_comp_adder_step  },
	{ DST_DAC_R1      ,"DST_DAC_R1"      ,sizeof(struct dst_dac_r1_context)      ,dst_dac_r1_reset      ,dst_dac_r1_step      },
	{ DST_MIXER       ,"DST_MIXER"       ,sizeof(struct dst_mixer_context)       ,dst_mixer_reset       ,dst_mixer_step       },
	{ DST_VCA         ,"DST_VCA"         ,0                                      ,NULL                  ,NULL                 },
	{ DST_VCA_OP_AMP  ,"DST_VCA_OP_AMP"  ,0                                      ,NULL                  ,NULL                 },

	/* from disc_flt.c */
	/* Generic modules */
	{ DST_FILTER1     ,"DST_FILTER1"     ,sizeof(struct dss_filter1_context)     ,dst_filter1_reset     ,dst_filter1_step     },
	{ DST_FILTER2     ,"DST_FILTER2"     ,sizeof(struct dss_filter2_context)     ,dst_filter2_reset     ,dst_filter2_step     },
	/* Component specific modules */
	{ DST_CRFILTER    ,"DST_CRFILTER"    ,sizeof(struct dst_rcfilter_context)    ,dst_crfilter_reset    ,dst_crfilter_step    },
	{ DST_OP_AMP_FILT ,"DST_OP_AMP_FILT" ,sizeof(struct dst_op_amp_filt_context),dst_op_amp_filt_reset ,dst_op_amp_filt_step  },
	{ DST_RCDISC      ,"DST_RCDISC"      ,sizeof(struct dst_rcdisc_context)      ,dst_rcdisc_reset      ,dst_rcdisc_step      },
	{ DST_RCDISC2     ,"DST_RCDISC2"     ,sizeof(struct dst_rcdisc_context)      ,dst_rcdisc2_reset     ,dst_rcdisc2_step     },
	{ DST_RCFILTER    ,"DST_RCFILTER"    ,sizeof(struct dst_rcfilter_context)    ,dst_rcfilter_reset    ,dst_rcfilter_step    },
	/* For testing - seem to be buggered.  Use versions not ending in N. */
	{ DST_RCFILTERN   ,"DST_RCFILTERN"   ,sizeof(struct dss_filter1_context)     ,dst_rcfilterN_reset   ,dst_filter1_step     },
	{ DST_RCDISCN     ,"DST_RCDISCN"     ,sizeof(struct dss_filter1_context)     ,dst_rcdiscN_reset     ,dst_rcdiscN_step     },
	{ DST_RCDISC2N    ,"DST_RCDISC2N"    ,sizeof(struct dss_rcdisc2_context)     ,dst_rcdisc2N_reset    ,dst_rcdisc2N_step    },

	/* from disc_dev.c */
	/* Component specific modules */
	{ DSD_555_ASTBL   ,"DSD_555_ASTBL"   ,sizeof(struct dsd_555_astbl_context)   ,dsd_555_astbl_reset   ,dsd_555_astbl_step   },
	{ DSD_555_MSTBL   ,"DSD_555_MSTBL"   ,0                                      ,NULL                  ,NULL                 },
	{ DSD_555_CC      ,"DSD_555_CC"      ,sizeof(struct dsd_555_cc_context)      ,dsd_555_cc_reset      ,dsd_555_cc_step      },
	{ DSD_566         ,"DSD_566"         ,sizeof(struct dsd_566_context)         ,dsd_566_reset         ,dsd_566_step         },

	/* must be the last one */
	{ DSS_NULL        ,"DSS_NULL"        ,0                                      ,NULL                  ,NULL                 }
};



/*************************************
 *
 *	Find a given node
 *
 *************************************/

struct node_description *discrete_find_node(int node)
{
	return indexed_node[node - NODE_START];
}



/*************************************
 *
 *	Master discrete system start
 *
 *************************************/

int discrete_sh_start(const struct MachineSound *msound)
{
	struct discrete_sound_block *intf = msound->sound_interface;

	/* do nothing if sound is disabled */
	if (!Machine->sample_rate)
		return 0;

	/* create the logfile */
	if (DISCRETE_DEBUGLOG && !disclogfile)
		disclogfile = fopen("discrete.log", "w");

	/* first pass through the nodes: sanity check, fill in the indexed_nodes, and make a total count */
	discrete_log("discrete_sh_start() - Doing node list sanity check");
	for (node_count = 0; intf[node_count].type != DSS_NULL; node_count++)
	{
		/* make sure we don't have too many nodes overall */
		if (node_count > DISCRETE_MAX_NODES)
			{ log_cb(RETRO_LOG_INFO, LOGPRE "discrete_sh_start() - Upper limit of %d nodes exceeded, have you terminated the interface block.", DISCRETE_MAX_NODES);  bailing =1; }

		/* make sure the node number is in range */
		if (intf[node_count].node < NODE_START || intf[node_count].node > NODE_END)
			{ log_cb(RETRO_LOG_INFO, LOGPRE "discrete_sh_start() - Invalid node number on node %02d descriptor\n", node_count);  bailing =1; }
		
		/* make sure the node type is valid */
		if (intf[node_count].type > DSO_OUTPUT)
			{ log_cb(RETRO_LOG_INFO, LOGPRE "discrete_sh_start() - Invalid function type on NODE_%03d\n", intf[node_count].node - NODE_START);  bailing =1; }
	}
	node_count++;
	discrete_log("discrete_sh_start() - Sanity check counted %d nodes", node_count);

	/* allocate memory for the array of actual nodes */
	node_list = auto_malloc(node_count * sizeof(node_list[0]));
	if (!node_list)
		{ log_cb(RETRO_LOG_INFO, LOGPRE "discrete_sh_start() - Out of memory allocating node_list\n");  bailing =1; }
	memset(node_list, 0, node_count * sizeof(node_list[0]));
	
	/* allocate memory for the node execution order array */
	running_order = auto_malloc(node_count * sizeof(running_order[0]));
	if (!running_order)
		{ log_cb(RETRO_LOG_INFO, LOGPRE "discrete_sh_start() - Out of memory allocating running_order\n");  bailing =1; }
	memset(running_order, 0, node_count * sizeof(running_order[0]));

	/* allocate memory to hold pointers to nodes by index */
	indexed_node = auto_malloc(DISCRETE_MAX_NODES * sizeof(indexed_node[0]));
	if (!indexed_node)
		{ log_cb(RETRO_LOG_INFO, LOGPRE "discrete_sh_start() - Out of memory allocating indexed_node\n");  bailing =1; }
	memset(indexed_node, 0, DISCRETE_MAX_NODES * sizeof(indexed_node[0]));
	
	/* allocate memory to hold the input map */
	dss_input_map = auto_malloc(DSS_INPUT_SPACE * sizeof(dss_input_map[0]));
	if (!dss_input_map)
		{ log_cb(RETRO_LOG_INFO, LOGPRE "discrete_sh_start() - Out of memory allocating dss_input_map\n");  bailing =1; }
	memset(dss_input_map, 0, DSS_INPUT_SPACE * sizeof(dss_input_map[0]));

	/* initialize the node data */
	init_nodes(intf);
	
	/* now go back and find pointers to all input nodes */
	find_input_nodes(intf);
	
	/* then set up the output nodes */
	setup_output_nodes();

	/* reset the system, which in turn resets all the nodes and steps them forward one */
	discrete_sh_reset();
	return 0;
}



/*************************************
 *
 *	Master discrete system stop
 *
 *************************************/

void discrete_sh_stop (void)
{
	if (DISCRETE_WAVELOG)
	{
		int outputnum;

		/* close any wave files */
		for (outputnum = 0; outputnum < discrete_outputs; outputnum++)
			if (wav_file[outputnum])
				wav_close(wav_file[outputnum]);
	}

	if (DISCRETE_DEBUGLOG)
	{
		/* close the debug log */
	    if (disclogfile)
	    	fclose(disclogfile);
		disclogfile = NULL;
	}
}



/*************************************
 *
 *	Master discrete system update
 *
 *************************************/

void discrete_sh_update(void)
{
	stream_update(discrete_stream, 0);
}



/*************************************
 *
 *	Master reset of all nodes
 *
 *************************************/

void discrete_sh_reset(void)
{
	int nodenum, inputnum;

	/* loop over all nodes */
	for (nodenum = 0; nodenum < node_count; nodenum++)
	{
		struct node_description *node = running_order[nodenum];

		/* propogate any node inputs before resetting */
		for (inputnum = 0; inputnum < node->active_inputs; inputnum++)
		{
			struct node_description *inputnode = node->input_node[inputnum];
			if (inputnode && inputnode->node != NODE_NC)
				node->input[inputnum] = inputnode->output;
		}
		
		/* if the node has a reset function, call it */
		if (node->module.reset)
			(*node->module.reset)(node);
		
		/* otherwise, just step it */
		else if (node->module.step)
			(*node->module.step)(node);
	}
}



/*************************************
 *
 *	Stream update functions
 *
 *************************************/

static void discrete_stream_update(int ch, INT16 **buffer, int length)
{
	int samplenum, nodenum, inputnum, outputnum;

	/* Now we must do length iterations of the node list, one output for each step */
	for (samplenum = 0; samplenum < length; samplenum++)
	{
		/* loop over all nodes */
		for (nodenum = 0; nodenum < node_count; nodenum++)
		{
			struct node_description *node = running_order[nodenum];

			/* propogate any node inputs before resetting */
			for (inputnum = 0; inputnum < node->active_inputs; inputnum++)
			{
				struct node_description *inputnode = node->input_node[inputnum];
				if (inputnode && inputnode->node != NODE_NC)
					node->input[inputnum] = inputnode->output;
			}

			/* Now step the node */
			if (node->module.step)
				(*node->module.step)(node);
		}

		/* Now put the output into the buffers */
		for (outputnum = 0; outputnum < discrete_outputs; outputnum++)
		{
			double val = output_node[outputnum]->input[0];
			buffer[outputnum][samplenum] = (val < -32768) ? -32768 : (val > 32767) ? 32767 : val;
		}
	}

	/* add each stream to the logging wavfile */
	if (DISCRETE_WAVELOG)
		for (outputnum = 0; outputnum < discrete_outputs; outputnum++)
			wav_add_data_16(wav_file[outputnum], buffer[outputnum], length);
}


static void discrete_stream_update_one(int ch, INT16 *buffer, int length)
{
	discrete_stream_update(ch, &buffer, length);
}



/*************************************
 *
 *	First pass init of nodes
 *
 *************************************/

static void init_nodes(struct discrete_sound_block *block_list)
{
	int nodenum;

	/* start with no outputs */
	discrete_outputs = 0;

	/* loop over all nodes */
	for (nodenum = 0; nodenum < node_count; nodenum++)
	{
		struct discrete_sound_block *block = &block_list[nodenum];
		struct node_description *node = &node_list[nodenum];
		int inputnum, modulenum;

		/* our running order just follows the order specified */
		running_order[nodenum] = node;
		
		/* if we are an output node, track that */
		if (block->node == NODE_OP)
			output_node[discrete_outputs++] = node;
		
		/* otherwise, make sure we are not a duplicate, and put ourselves into the indexed list */
		else
		{
			if (indexed_node[block->node - NODE_START])
				{ log_cb(RETRO_LOG_INFO, LOGPRE "init_nodes() - Duplicate entries for NODE_%03d\n", block->node - NODE_START);  bailing =1; }
			indexed_node[block->node - NODE_START] = node;
		}

		/* find the requested module */	
		for (modulenum = 0; module_list[modulenum].type != DSS_NULL; modulenum++)
			if (module_list[modulenum].type == block->type)
				break;
		if (module_list[modulenum].type != block->type)
			{ log_cb(RETRO_LOG_INFO, LOGPRE "init_nodes() - Unable to find discrete module typer %d for NODE_%03d\n", block->type, block->node - NODE_START);  bailing =1; }

		/* static inits */
		node->node = block->node;
		node->module = module_list[modulenum];
		node->output = 0.0;
		
		node->active_inputs = block->active_inputs;
		for (inputnum = 0; inputnum < DISCRETE_MAX_INPUTS; inputnum++)
		{
			node->input_node[inputnum] = NULL;
			node->input[inputnum] = block->initial[inputnum];
		}
		
		node->context = NULL;
		node->name = block->name;
		node->custom = block->custom;
		
		/* allocate memory if necessary */
		if (node->module.contextsize)
		{
			node->context = auto_malloc(node->module.contextsize);
			if (!node->context)
				{ log_cb(RETRO_LOG_INFO, LOGPRE "init_nodes() - Out of memory allocating memory for NODE_%03d\n", node->node - NODE_START);  bailing =1; }
			memset(node->context, 0, node->module.contextsize);
		}
	}
	
	/* if no outputs, give an error */
	if (discrete_outputs == 0)
		{ log_cb(RETRO_LOG_INFO, LOGPRE "init_nodes() - Couldn't find an output node"); bailing =1; }
}



/*************************************
 *
 *	Find and attach all input nodes
 *
 *************************************/

static void find_input_nodes(struct discrete_sound_block *block_list)
{
	int nodenum, inputnum;
	
	/* loop over all nodes */
	for (nodenum = 0; nodenum < node_count; nodenum++)
	{
		struct node_description *node = &node_list[nodenum];
		struct discrete_sound_block *block = &block_list[nodenum];
		
		/* loop over all active inputs */
		for (inputnum = 0; inputnum < node->active_inputs; inputnum++)
		{
			int inputnode = block->input_node[inputnum];
			
			/* if this input is node-based, find the node in the indexed list */
			if (inputnode >= NODE_START && inputnode <= NODE_END)
			{
				if (!indexed_node[inputnode - NODE_START])
					{ log_cb(RETRO_LOG_INFO, LOGPRE "discrete_sh_start - Node NODE_%03d referenced a non existant node NODE_%03d\n", node->node - NODE_START, inputnode - NODE_START);  bailing =1; }
				node->input_node[inputnum] = indexed_node[inputnode - NODE_START];
			}
		}
	}
}



/*************************************
 *
 *	Set up the output nodes
 *
 *************************************/

static void setup_output_nodes(void)
{
	const char *channel_names[DISCRETE_MAX_OUTPUTS];
	char channel_name_data[DISCRETE_MAX_OUTPUTS][32];
	int channel_vol[DISCRETE_MAX_OUTPUTS];
	int outputnum;

	/* loop over output nodes */
	for (outputnum = 0; outputnum < discrete_outputs; outputnum++)
	{
		/* make up a name for this output channel */
		sprintf(&channel_name_data[outputnum][0], "Discrete CH%d", outputnum);
		channel_names[outputnum] = &channel_name_data[outputnum][0];
		
		/* set the initial volume */
		channel_vol[outputnum] = output_node[outputnum]->input[1];

		/* create a logging file */
		if (DISCRETE_WAVELOG)
		{
			char name[32];
			sprintf(name, "discrete%d.wav", outputnum);
			wav_file[outputnum] = wav_open(name, Machine->sample_rate, 1);
		}
	}

	/* initialize the stream(s) */
	if (discrete_outputs > 1)
		discrete_stream = stream_init_multi(discrete_outputs, channel_names, channel_vol, Machine->sample_rate, 0, discrete_stream_update);
	else
		discrete_stream = stream_init(channel_names[0], channel_vol[0], Machine->sample_rate, 0, discrete_stream_update_one);

	/* handle failure */
	if (discrete_stream == -1)
		{ log_cb(RETRO_LOG_INFO, LOGPRE "setup_output_nodes - Stream init returned an error\n"); bailing =1; }
}
