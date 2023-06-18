/*
 *	Pulsar sound routines
 *
 *	TODO: change heart rate based on bit 7 of Port 1
 *
 */

#include "driver.h"


/* output port 0x01 definitions - sound effect drive outputs */
#define OUT_PORT_1_CLANG		0x01
#define OUT_PORT_1_KEY			0x02
#define OUT_PORT_1_ALIENHIT		0x04
#define OUT_PORT_1_PHIT			0x08
#define OUT_PORT_1_ASHOOT		0x10
#define OUT_PORT_1_PSHOOT		0x20
#define OUT_PORT_1_BONUS		0x40
#define OUT_PORT_1_HBEAT_RATE	0x80	/* currently not used */

/* output port 0x02 definitions - sound effect drive outputs */
#define OUT_PORT_2_SIZZLE		0x01
#define OUT_PORT_2_GATE			0x02
#define OUT_PORT_2_BIRTH		0x04
#define OUT_PORT_2_HBEAT		0x08
#define OUT_PORT_2_MOVMAZE		0x10


#define PLAY(id,loop)           sample_start( id, id, loop )
#define STOP(id)                sample_stop( id )


/* sample file names */
const char *pulsar_sample_names[] =
{
	"*pulsar",
	"clang.wav",
	"key.wav",
	"alienhit.wav",
	"phit.wav",
	"ashoot.wav",
	"pshoot.wav",
	"bonus.wav",
	"sizzle.wav",
	"gate.wav",
	"birth.wav",
	"hbeat.wav",
	"movmaze.wav",
	0
};

/* sample sound IDs - must match sample file name table above */
enum
{
	SND_CLANG = 0,
	SND_KEY,
	SND_ALIENHIT,
	SND_PHIT,
	SND_ASHOOT,
	SND_PSHOOT,
	SND_BONUS,
	SND_SIZZLE,
	SND_GATE,
	SND_BIRTH,
	SND_HBEAT,
	SND_MOVMAZE
};


static int port1State = 0;

WRITE_HANDLER( pulsar_sh_port1_w )
{
	int bitsChanged;
	int bitsGoneHigh;
	int bitsGoneLow;


	bitsChanged  = port1State ^ data;
	bitsGoneHigh = bitsChanged & data;
	bitsGoneLow  = bitsChanged & ~data;

	port1State = data;

	if ( bitsGoneLow & OUT_PORT_1_CLANG )
	{
		PLAY( SND_CLANG, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_KEY )
	{
		PLAY( SND_KEY, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_ALIENHIT )
	{
		PLAY( SND_ALIENHIT, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_PHIT )
	{
		PLAY( SND_PHIT, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_ASHOOT )
	{
		PLAY( SND_ASHOOT, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_PSHOOT )
	{
		PLAY( SND_PSHOOT, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_1_BONUS )
	{
		PLAY( SND_BONUS, 0 );
	}
}


WRITE_HANDLER( pulsar_sh_port2_w )
{
	static int port2State = 0;
	int bitsChanged;
	int bitsGoneHigh;
	int bitsGoneLow;


	bitsChanged  = port2State ^ data;
	bitsGoneHigh = bitsChanged & data;
	bitsGoneLow  = bitsChanged & ~data;

	port2State = data;

	if ( bitsGoneLow & OUT_PORT_2_SIZZLE )
	{
		PLAY( SND_SIZZLE, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_2_GATE )
	{
		sample_start( SND_CLANG, SND_GATE, 0 );
	}
	if ( bitsGoneHigh & OUT_PORT_2_GATE )
	{
		STOP( SND_CLANG );
	}

	if ( bitsGoneLow & OUT_PORT_2_BIRTH )
	{
		PLAY( SND_BIRTH, 0 );
	}

	if ( bitsGoneLow & OUT_PORT_2_HBEAT )
	{
		PLAY( SND_HBEAT, 1 );
	}
	if ( bitsGoneHigh & OUT_PORT_2_HBEAT )
	{
		STOP( SND_HBEAT );
	}

	if ( bitsGoneLow & OUT_PORT_2_MOVMAZE )
	{
		PLAY( SND_MOVMAZE, 1 );
	}
	if ( bitsGoneHigh & OUT_PORT_2_MOVMAZE )
	{
		STOP( SND_MOVMAZE );
	}
}
