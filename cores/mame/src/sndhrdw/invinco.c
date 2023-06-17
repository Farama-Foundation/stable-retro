/*
 *	Invinco sound routines
 */

#include "driver.h"


/* output port 0x02 definitions - sound effect drive outputs */
#define OUT_PORT_2_SAUCER		0x04
#define OUT_PORT_2_MOVE1		0x08
#define OUT_PORT_2_MOVE2		0x10
#define OUT_PORT_2_FIRE			0x20
#define OUT_PORT_2_INVHIT		0x40
#define OUT_PORT_2_SHIPHIT		0x80


#define PLAY(id,loop)           sample_start( id, id, loop )
#define STOP(id)                sample_stop( id )


/* sample file names */
const char *invinco_sample_names[] =
{
	"*invinco",
	"saucer.wav",
	"move1.wav",
	"move2.wav",
	"fire.wav",
	"invhit.wav",
	"shiphit.wav",
	"move3.wav",	/* currently not used */
	"move4.wav",	/* currently not used */
	0
};

/* sample sound IDs - must match sample file name table above */
enum
{
	SND_SAUCER = 0,
	SND_MOVE1,
	SND_MOVE2,
	SND_FIRE,
	SND_INVHIT,
	SND_SHIPHIT,
	SND_MOVE3,
	SND_MOVE4
};


WRITE_HANDLER( invinco_sh_port2_w )
{
	static int port2State = 0;
	int bitsChanged;
	int bitsGoneHigh;
	int bitsGoneLow;


	bitsChanged  = port2State ^ data;
	bitsGoneHigh = bitsChanged & data;
	bitsGoneLow  = bitsChanged & ~data;

	port2State = data;

	if ( bitsGoneLow & OUT_PORT_2_SAUCER )
	{
		PLAY( SND_SAUCER, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_2_MOVE1 )
	{
		PLAY( SND_MOVE1, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_2_MOVE2 )
	{
		PLAY( SND_MOVE2, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_2_FIRE )
	{
		PLAY( SND_FIRE, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_2_INVHIT )
	{
		PLAY( SND_INVHIT, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_2_SHIPHIT )
	{
		PLAY( SND_SHIPHIT, 0 );
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Went LO: %02X  %04X\n", bitsGoneLow, activecpu_get_pc());
}
