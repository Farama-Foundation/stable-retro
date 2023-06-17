/*************************************************************************

	Sega g80 common sound hardware

*************************************************************************/

#include "driver.h"
#include "sega.h"
#include "cpu/i8039/i8039.h"

/* SP0250-based speechboard */

static UINT8 sega_speechboard_latch, sega_speechboard_t0, sega_speechboard_p2, sega_speechboard_drq;


static READ_HANDLER( speechboard_t0_r )
{
	return sega_speechboard_t0;
}

static READ_HANDLER( speechboard_t1_r )
{
	return sega_speechboard_drq;
}

static READ_HANDLER( speechboard_p1_r )
{
	return sega_speechboard_latch;
}

static READ_HANDLER( speechboard_rom_r )
{
	return memory_region(REGION_CPU2)[0x800 + 0x100*(sega_speechboard_p2 & 0x3f) + offset];
}

static WRITE_HANDLER( speechboard_p1_w )
{
	if(!(data & 0x80))
		sega_speechboard_t0 = 0;
}

static WRITE_HANDLER( speechboard_p2_w )
{
	sega_speechboard_p2 = data;
}

static void speechboard_drq_w(int level)
{
	sega_speechboard_drq = level == ASSERT_LINE;
}

WRITE_HANDLER( sega_sh_speechboard_w )
{
	sega_speechboard_latch = data & 0x7f;
	cpu_set_irq_line(1, 0, data & 0x80 ? CLEAR_LINE : ASSERT_LINE);
	if(!(data & 0x80))
		sega_speechboard_t0 = 1;
}

MEMORY_READ_START( sega_speechboard_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( sega_speechboard_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
MEMORY_END

PORT_READ_START( sega_speechboard_readport )
	{ 0x00,     0xff,     speechboard_rom_r },
	{ I8039_p1, I8039_p1, speechboard_p1_r },
	{ I8039_t0, I8039_t0, speechboard_t0_r },
	{ I8039_t1, I8039_t1, speechboard_t1_r },
PORT_END

PORT_WRITE_START( sega_speechboard_writeport )
	{ 0x00,     0xff,     sp0250_w },
	{ I8039_p1, I8039_p1, speechboard_p1_w },
	{ I8039_p2, I8039_p2, speechboard_p2_w },
PORT_END

struct sp0250_interface sega_sp0250_interface =
{
	100,
	speechboard_drq_w
};
