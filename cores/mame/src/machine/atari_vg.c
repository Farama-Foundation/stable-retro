/***************************************************************************

	Atari vector hardware

***************************************************************************/

#include "driver.h"

#define EAROM_SIZE	0x40

static int earom_offset;
static int earom_data;
static char earom[EAROM_SIZE];


READ_HANDLER( atari_vg_earom_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "read earom: %02x(%02x):%02x\n", earom_offset, offset, earom_data);
	return (earom_data);
}


WRITE_HANDLER( atari_vg_earom_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "write earom: %02x:%02x\n", offset, data);
	earom_offset = offset;
	earom_data = data;
}


/* 0,8 and 14 get written to this location, too.
 * Don't know what they do exactly
 */
WRITE_HANDLER( atari_vg_earom_ctrl_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "earom ctrl: %02x:%02x\n",offset, data);
	/*
		0x01 = clock
		0x02 = set data latch? - writes only (not always)
		0x04 = write mode? - writes only
		0x08 = set addr latch?
	*/
	if (data & 0x01)
		earom_data = earom[earom_offset];
	if ((data & 0x0c) == 0x0c)
	{
		earom[earom_offset]=earom_data;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "    written %02x:%02x\n", earom_offset, earom_data);
	}
}


NVRAM_HANDLER( atari_vg )
{
	if (read_or_write)
		mame_fwrite(file,earom,EAROM_SIZE);
	else if (file)
		mame_fread(file,earom,EAROM_SIZE);
	else
		memset(earom,0,EAROM_SIZE);
}
