/***************************************************************************

  snes.c

  Machine file to handle emulation of the Nintendo Super NES

  Anthony Kruize
  Based on the original code by Lee Hammerton (aka Savoury Snax)

***************************************************************************/
#define __MACHINE_SNES_C

#include "driver.h"
#include "includes/snes.h"
#include "cpu/g65816/g65816.h"

/* -- Globals -- */
UINT8  *snes_ram = NULL;		/* 65816 ram */
UINT8  *spc_ram = NULL;			/* spc700 ram */
UINT8  *snes_vram = NULL;		/* Video RAM (Should be 16-bit, but it's easier this way) */
UINT16 *snes_cgram = NULL;		/* Colour RAM */
UINT16 *snes_oam = NULL;		/* Object Attribute Memory */
static UINT16 cgram_address;	/* CGRAM address */
static UINT8  vram_read_offset;	/* VRAM read offset */
static UINT16 vram_fg_count;	/* Fullgraphic increase count */
static UINT16 vram_fg_incr;		/* Fullgraphic increase */
static UINT16 vram_fg_cntr;		/* Fullgraphic counter */
static INT16 vram_fg_offset;	/* Fullgraphic offset */
UINT8  spc_port_in[4];	/* Port for sending data to the SPC700 */
UINT8  spc_port_out[4];	/* Port for receiving data from the SPC700 */
static UINT8 snes_hdma_chnl;	/* channels enabled for HDMA */
static struct
{
	UINT8  mode;		/* ROM memory mode */
	UINT32 sram;		/* Amount of sram in cart */
	UINT32 sram_max;	/* Maximum amount sram in cart (based on ROM mode) */
} cart = { SNES_MODE_20, 0x40000, 0x40000 };
static struct
{
	UINT8 low;
	UINT8 high;
	UINT32 value;
	UINT8 oldrol;
} joypad[4];

static void snes_init_ram(void)
{
	/* Init VRAM */
	snes_vram = (UINT8 *)memory_region( REGION_GFX1 );
	memset( snes_vram, 0, SNES_VRAM_SIZE );

	/* Init Colour RAM */
	snes_cgram = (UINT16 *)memory_region( REGION_USER1 );
	memset( (UINT8 *)snes_cgram, 0, SNES_CGRAM_SIZE );

	/* Init oam RAM */
	snes_oam = (UINT16 *)memory_region( REGION_USER2 );
	memset( snes_oam, 0xff, SNES_OAM_SIZE );

	/* Inititialize registers/variables */
	snes_ppu.update_windows = 1;
	snes_ppu.update_palette = 1;
	snes_ppu.beam.latch_vert = 0;
	snes_ppu.beam.latch_horz = 0;
	snes_ppu.beam.current_vert = 0;
	snes_ppu.beam.current_horz = 0;
	snes_ppu.beam.last_visible_line = 240;
	snes_ppu.mode = 0;
	cgram_address = 0;
	vram_read_offset = 2;
	/* Force the use of the SPCSkipper for now.
	 * Once the two CPU's are running in sync. we should check that sound is
	 * enabled here and only use the SPCSkipper if it is. */
	spc_usefakeapu = 1;
}


MACHINE_INIT( snes )
{
	snes_init_ram();

	/* Set STAT78 to NTSC or PAL */
	if( Machine->drv->frames_per_second == 60 )
		snes_ram[STAT78] = SNES_NTSC;
	else /* if( Machine->drv->frames_per_second == 50 ) */
		snes_ram[STAT78] = SNES_PAL;
}


/* Handle reading of Mode 20 SRAM */
/* 0x700000 - 0x77ffff */
READ_HANDLER( snes_r_sram )
{
	UINT8 value = 0xff;

	if( cart.sram > 0 )
	{
		value = snes_ram[0x700000 + offset];
	}

	return value;
}

/* 0x000000 - 0x2fffff */
READ_HANDLER( snes_r_bank1 )
{
	UINT16 address = offset & 0xffff;

	if( address <= 0x1fff )								/* Mirror of Low RAM */
		return cpu_readmem24( 0x7e0000 + address );
	else if( address >= 0x2000 && address <= 0x5fff )	/* I/O */
		return snes_r_io( address );
	else if( address >= 0x6000 && address <= 0x7fff )	/* Reserved */
		return 0xff;
	else
	{
		if( cart.mode == SNES_MODE_20 )
			return snes_ram[offset];
		else	/* MODE_21 */
			return snes_ram[0xc00000 + offset];
	}

	return 0xff;
}

/* 0x300000 - 0x3fffff */
READ_HANDLER( snes_r_bank2 )
{
	UINT16 address = offset & 0xffff;

	if( address <= 0x1fff )								/* Mirror of Low RAM */
		return cpu_readmem24( 0x7e0000 + address );
	else if( address >= 0x2000 && address <= 0x5fff )	/* I/O */
		return snes_r_io( address );
	else if( address >= 0x6000 && address <= 0x7fff )
	{
		if( cart.mode == SNES_MODE_20 )
			return 0xff;						/* Reserved */
		else	/* MODE_21 */
			return snes_ram[0x300000 + offset];	/* sram */
	}
	else
	{
		if( cart.mode == SNES_MODE_20 )
			return snes_ram[0x300000 + offset];
		else	/* MODE_21 */
			return snes_ram[0xf00000 + offset];
	}

	return 0xff;
}

/* 0x400000 - 0x5fffff */
READ_HANDLER( snes_r_bank3 )
{
	UINT16 address = offset & 0xffff;

	if( cart.mode == SNES_MODE_20 )
	{
		if( address <= 0x7fff )
			return 0xff;		/* Reserved */
		else
			return snes_ram[0x400000 + offset];
	}
	else	/* MODE_21 */
	{
		return snes_ram[0x400000 + offset];
	}

	return 0xff;
}

/* 0x800000 - 0xffffff */
READ_HANDLER( snes_r_bank4 )
{
	if( cart.mode == SNES_MODE_20 )
	{
		if( offset <= 0x5fffff )
			return cpu_readmem24( offset );
		else
			return 0xff;
	}
	else	/* MODE_21 */
	{
		if( offset <= 0x3fffff )
			return cpu_readmem24( offset );
		else
			return snes_ram[offset + 0x800000];
	}

	return 0xff;
}

/* 0x000000 - 0x2fffff */
WRITE_HANDLER( snes_w_bank1 )
{
	UINT16 address = offset & 0xffff;

	if( address <= 0x1fff )								/* Mirror of Low RAM */
		cpu_writemem24( 0x7e0000 + address, data );
	else if( address >= 0x2000 && address <= 0x5fff )	/* I/O */
		snes_w_io( address, data );
	else if( address >= 0x6000 && address <= 0x7fff )	/* Reserved */
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "Attempt to write to reserved address: %X\n", offset );
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "Attempt to write to ROM address: %X\n", offset );
}

/* 0x300000 - 0x3fffff */
WRITE_HANDLER( snes_w_bank2 )
{
	UINT16 address = offset & 0xffff;

	if( address <= 0x1fff )								/* Mirror of Low RAM */
		cpu_writemem24( 0x7e0000 + address, data );
	else if( address >= 0x2000 && address <= 0x5fff )	/* I/O */
		snes_w_io( address, data );
	else if( address >= 0x6000 && address <= 0x7fff )
	{
		if( cart.mode == SNES_MODE_20 )			/* Reserved */
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "Attempt to write to reserved address: %X\n", offset );
		else /* MODE_21 */
			snes_ram[0x300000 + offset] = data;  /* sram */
	}
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "Attempt to write to ROM address: %X\n", offset );
}

/* 0x800000 - 0xffffff */
WRITE_HANDLER( snes_w_bank4 )
{
	if( cart.mode == SNES_MODE_20 )
	{
		if( offset <= 0x2fffff )
			snes_w_bank1( offset, data );
		else if( offset >= 0x300000 && offset <= 0x3fffff )
			snes_w_bank2( offset - 0x300000, data );
	}
	else /* MODE_21 */
	{
		if( offset <= 0x2fffff )
			snes_w_bank1( offset, data );
		else if( offset >= 0x300000 && offset <= 0x3fffff )
			snes_w_bank2( offset - 0x300000, data );
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "Attempt to write to ROM address: %X\n", offset );
	}
}

/*
 * DR   - Double read : address is read twice to return a 16bit value.
 * low  - This is the low byte of a 16 or 24 bit value
 * mid  - This is the middle byte of a 24 bit value
 * high - This is the high byte of a 16 or 24 bit value
 */
READ_HANDLER( snes_r_io )
{
	UINT8 value = 0;

	/* offset is from 0x000000 */
	switch( offset )
	{
		case OAMADDL:
		case OAMADDH:
		case VMADDL:
		case VMADDH:
		case VMDATAL:
		case VMDATAH:
		case CGADD:
		case CGDATA:
			return snes_ram[offset];
		case MPYL:		/* Multiplication result (low) */
		case MPYM:		/* Multiplication result (mid) */
		case MPYH:		/* Multiplication result (high) */
			{
				/* Perform 16bit * 8bit multiply */
				INT32 c = snes_ppu.mode7.matrix_a * (snes_ppu.mode7.matrix_b >> 8);
				snes_ram[MPYL] = c & 0xff;
				snes_ram[MPYM] = (c >> 8) & 0xff;
				snes_ram[MPYH] = (c >> 16) & 0xff;
				return snes_ram[offset];
			}
		case SLHV:		/* Software latch for H/V counter */
			/* FIXME: horizontal latch is a major fudge!!! */
			snes_ppu.beam.latch_vert = snes_ppu.beam.current_vert;
			snes_ppu.beam.latch_horz = snes_ppu.beam.current_horz;
			snes_ppu.beam.current_horz = 0;
			return 0x0;		/* Return value is meaningless */
		case ROAMDATA:	/* Read data from OAM (DR) */
			{
				value = (snes_oam[snes_ppu.oam.address] >> (snes_ram[OAMDATA] << 3)) & 0xff;
				snes_ram[OAMDATA] = (snes_ram[OAMDATA] + 1) % 2;
				if( snes_ram[OAMDATA] == 0 )
				{
					snes_ppu.oam.address++;
					snes_ram[OAMADDL] = snes_ppu.oam.address & 0xff;
					snes_ram[OAMADDH] = (snes_ppu.oam.address >> 8) & 0x1;
				}
				return value;
			}
		case RVMDATAL:	/* Read data from VRAM (low) */
			{
				UINT16 addr = (snes_ram[VMADDH] << 8) | snes_ram[VMADDL];
				value = snes_vram[(addr << 1) - vram_read_offset];
				if( !(snes_ram[VMAIN] & 0x80) )
				{
					if( vram_read_offset == 0 )
					{
						vram_read_offset = 2;
					}
					/* Increase the address */
					if( snes_ram[VMAIN] & 0xc )
					{
						addr++;
						vram_fg_offset += 7;	/* addr increases by 1, plus 7 = 8 */
						vram_fg_count--;
						if( vram_fg_count == 0 )
						{
							vram_fg_cntr--;
							vram_fg_count = vram_fg_incr;
							if( vram_fg_cntr == 0 )
							{
								vram_fg_cntr = 8;
								vram_fg_offset -= 7;
							}
							else
							{
								vram_fg_offset -= (vram_fg_count * 8) - 1;
							}
						}
					}
					else
					{
						switch( snes_ram[VMAIN] & 0x03 )
						{
							case 0: addr++;      break;
							case 1: addr += 32;  break;
							case 2: addr += 128; break; /* Should be 64, but a bug in the snes means it's 128 */
							case 3: addr += 128; break;
						}
					}
					snes_ram[VMADDL] = addr & 0xff;
					snes_ram[VMADDH] = (addr >> 8) & 0xff;
				}
				return value;
			}
		case RVMDATAH:	/* Read data from VRAM (high) */
			{
				UINT16 addr = (snes_ram[VMADDH] << 8) | snes_ram[VMADDL];

				value = snes_vram[(addr << 1) + 1 - vram_read_offset];
				if( snes_ram[VMAIN] & 0x80 )
				{
					if( vram_read_offset == 0 )
					{
						vram_read_offset = 2;
					}
					/* Increase the address */
					if( snes_ram[VMAIN] & 0xc )
					{
						addr++;
						vram_fg_offset += 7;	/* addr increases by 1, plus 7 = 8 */
						vram_fg_count--;
						if( vram_fg_count == 0 )
						{
							vram_fg_cntr--;
							vram_fg_count = vram_fg_incr;
							if( vram_fg_cntr == 0 )
							{
								vram_fg_cntr = 8;
								vram_fg_offset -= 7;
							}
							else
							{
								vram_fg_offset -= (vram_fg_count * 8) - 1;
							}
						}
					}
					else
					{
						switch( snes_ram[VMAIN] & 0x03 )
						{
							case 0: addr++;      break;
							case 1: addr += 32;  break;
							case 2: addr += 128; break; /* Should be 64, but a bug in the snes means it's 128 */
							case 3: addr += 128; break;
						}
					}
					snes_ram[VMADDL] = addr & 0xff;
					snes_ram[VMADDH] = (addr >> 8) & 0xff;
				}
				return value;
			}
		case RCGDATA:	/* Read data from CGRAM */
				value = ((UINT8 *)snes_cgram)[cgram_address];
				cgram_address = (cgram_address + 1) % (SNES_CGRAM_SIZE - 2);
				return value;
		case OPHCT:		/* Horizontal counter data by ext/soft latch */
			{
				/* FIXME: need to handle STAT78 reset */
				static UINT8 read_ophct = 0;
				if( read_ophct )
				{
					value = (snes_ppu.beam.latch_horz >> 8) & 0x1;
					read_ophct = 0;
				}
				else
				{
					value = snes_ppu.beam.latch_horz & 0xff;
					read_ophct = 1;
				}
				return value;
			}
		case OPVCT:		/* Vertical counter data by ext/soft latch */
			{
				/* FIXME: need to handle STAT78 reset */
				static UINT8 read_opvct = 0;
				if( read_opvct )
				{
					value = (snes_ppu.beam.latch_vert >> 8) & 0x1;
					read_opvct = 0;
				}
				else
				{
					value = snes_ppu.beam.latch_vert & 0xff;
					read_opvct = 1;
				}
				return value;
			}
		case STAT77:	/* PPU status flag and version number */
			return snes_ram[offset];
		case STAT78:	/* PPU status flag and version number */
			/* FIXME: need to reset OPHCT and OPVCT */
			return snes_ram[offset];
		case APU00:		/* Audio port register */
		case APU01:		/* Audio port register */
		case APU02:		/* Audio port register */
		case APU03:		/* Audio port register */
			if( spc_usefakeapu )
				return fakespc_port_r( offset & 0x3 );
			else
			return spc_port_out[offset & 0x3];
		case WMDATA:	/* Data to read from WRAM */
			{
				UINT32 addr = ((snes_ram[WMADDH] & 0x1) << 16) | (snes_ram[WMADDM] << 8) | snes_ram[WMADDL];

				value = cpu_readmem24(0x7e0000 + addr++);
				snes_ram[WMADDH] = (addr >> 16) & 0x1;
				snes_ram[WMADDM] = (addr >> 8) & 0xff;
				snes_ram[WMADDL] = addr & 0xff;
				return value;
			}
		case WMADDL:	/* Address to read/write to wram (low) */
		case WMADDM:	/* Address to read/write to wram (mid) */
		case WMADDH:	/* Address to read/write to wram (high) */
			return snes_ram[offset];
		case OLDJOY1:	/* Data for old NES controllers */
			{
				if( snes_ram[offset] & 0x1 )
				{
					return 0;
				}
				value = ((joypad[0].low | (joypad[0].high << 8) | 0x10000) >> (15 - (joypad[0].oldrol++ % 16))) & 0x1;
				if( !(joypad[0].oldrol % 17) )
					value = 0x1;
				return value;
			}
		case OLDJOY2:	/* Data for old NES controllers */
			{
				if( snes_ram[OLDJOY1] & 0x1 )
				{
					return 0;
				}
				value = ((joypad[1].low | (joypad[1].high << 8) | 0x10000) >> (15 - (joypad[1].oldrol++ % 16))) & 0x1;
				if( !(joypad[1].oldrol % 17) )
					value = 0x1;
				return value;
			}
		case HTIMEL:
		case HTIMEH:
		case VTIMEL:
		case VTIMEH:
			return snes_ram[offset];
		case MDMAEN:		/* GDMA channel designation and trigger */
			/* FIXME: Is this really read-only? - Villgust needs to read it */
			return snes_ram[offset];
		case RDNMI:			/* NMI flag by v-blank and version number */
			value = snes_ram[offset];
			snes_ram[offset] &= 0xf;	/* NMI flag is reset on read */
			return value;
		case TIMEUP:		/* IRQ flag by H/V count timer */
			value = snes_ram[offset];
			snes_ram[offset] = 0;	/* Register is reset on read */
			return value;
		case HVBJOY:		/* H/V blank and joypad controller enable */
			/* FIXME: JOYCONT and HBLANK are emulated wrong at present */
			value = snes_ram[offset] & 0xbe;
			snes_ram[offset] = ((snes_ram[offset]^0x41)&0x41)|value;
			return snes_ram[offset];
		case RDIO:			/* Programmable I/O port (in port ) */
			/* FIXME: do something here */
		case RDDIVL:		/* Quotient of divide result (low) */
		case RDDIVH:		/* Quotient of divide result (high) */
		case RDMPYL:		/* Product/Remainder of mult/div result (low) */
		case RDMPYH:		/* Product/Remainder of mult/div result (high) */
			return snes_ram[offset];
		case JOY1L:			/* Joypad 1 status register (low) */
			return joypad[0].low;
		case JOY1H:			/* Joypad 1 status register (high) */
			return joypad[0].high;
		case JOY2L:			/* Joypad 2 status register (low) */
			return joypad[1].low;
		case JOY2H:			/* Joypad 2 status register (high) */
			return joypad[1].high;
		case JOY3L:			/* Joypad 3 status register (low) */
			return joypad[2].low;
		case JOY3H:			/* Joypad 3 status register (high) */
			return joypad[2].high;
		case JOY4L:			/* Joypad 4 status register (low) */
			return joypad[3].low;
		case JOY4H:			/* Joypad 4 status register (high) */
			return joypad[3].high;
		case DMAP0: case BBAD0: case A1T0L: case A1T0H: case A1B0: case DAS0L:
		case DAS0H: case DSAB0: case A2A0L: case A2A0H: case NTRL0:
		case DMAP1: case BBAD1: case A1T1L: case A1T1H: case A1B1: case DAS1L:
		case DAS1H: case DSAB1: case A2A1L: case A2A1H: case NTRL1:
		case DMAP2: case BBAD2: case A1T2L: case A1T2H: case A1B2: case DAS2L:
		case DAS2H: case DSAB2: case A2A2L: case A2A2H: case NTRL2:
		case DMAP3: case BBAD3: case A1T3L: case A1T3H: case A1B3: case DAS3L:
		case DAS3H: case DSAB3: case A2A3L: case A2A3H: case NTRL3:
		case DMAP4: case BBAD4: case A1T4L: case A1T4H: case A1B4: case DAS4L:
		case DAS4H: case DSAB4: case A2A4L: case A2A4H: case NTRL4:
		case DMAP5: case BBAD5: case A1T5L: case A1T5H: case A1B5: case DAS5L:
		case DAS5H: case DSAB5: case A2A5L: case A2A5H: case NTRL5:
		case DMAP6: case BBAD6: case A1T6L: case A1T6H: case A1B6: case DAS6L:
		case DAS6H: case DSAB6: case A2A6L: case A2A6H: case NTRL6:
		case DMAP7: case BBAD7: case A1T7L: case A1T7H: case A1B7: case DAS7L:
		case DAS7H: case DSAB7: case A2A7L: case A2A7H: case NTRL7:
			return snes_ram[offset];

		case 0x4100:		/* NSS Dip-Switches */
#ifdef MAME_DEBUG
			return readinputport(12);
#else
			return readinputport(9);
#endif	/* MAME_DEBUG */
/*		case 0x4101: // PC: a104 - a10e - a12a	*/ /*only nss_actr*/
/*		case 0x420c: // PC: 9c7d - 8fab			*/ /*only nss_ssoc*/
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "offset = %x pc = %x\n",offset,activecpu_get_pc());
	}

	/* Unsupported reads return 0xff */
	return 0xff;
}

/*
 * DW   - Double write : address is written twice to set a 16bit value.
 * low  - This is the low byte of a 16 or 24 bit value
 * mid  - This is the middle byte of a 24 bit value
 * high - This is the high byte of a 16 or 24 bit value
 */
WRITE_HANDLER( snes_w_io )
{
	/* offset is from 0x000000 */
	switch( offset )
	{
		case INIDISP:	/* Initial settings for screen */
			snes_ppu.update_palette = 1;
			break;
		case OBSEL:		/* Object size and data area designation */
			snes_ppu.layer[4].data = ((data & 0x3) * 0x2000) << 1;
			snes_ppu.oam.name_select = (((data & 0x18)>>3) * 0x1000) << 1;
			/* Determine object size */
			switch( (data & 0xe0) >> 5 )
			{
				case 0:			/* 8 & 16 */
					snes_ppu.oam.size[0] = 1;
					snes_ppu.oam.size[1] = 2;
					break;
				case 1:			/* 8 & 32 */
					snes_ppu.oam.size[0] = 1;
					snes_ppu.oam.size[1] = 4;
					break;
				case 2:			/* 8 & 64 */
					snes_ppu.oam.size[0] = 1;
					snes_ppu.oam.size[1] = 8;
					break;
				case 3:			/* 16 & 32 */
					snes_ppu.oam.size[0] = 2;
					snes_ppu.oam.size[1] = 4;
					break;
				case 4:			/* 16 & 64 */
					snes_ppu.oam.size[0] = 2;
					snes_ppu.oam.size[1] = 8;
					break;
				case 5:			/* 32 & 64 */
					snes_ppu.oam.size[0] = 4;
					snes_ppu.oam.size[1] = 8;
					break;
				default:
					/* Unknown size so default to 8 & 16 */
					snes_ppu.oam.size[0] = 1;
					snes_ppu.oam.size[1] = 2;
#ifdef SNES_DBG_REG_W
					log_cb(RETRO_LOG_DEBUG, LOGPRE  "Object size unsupported: %d\n", (data & 0xe0) >> 5 );
#endif
			}
			break;
		case OAMADDL:	/* Address for accessing OAM (low) */
			snes_ppu.oam.address_low = data;
			snes_ppu.oam.address = ((snes_ppu.oam.address_high & 0x1) << 8) + data;
			snes_ram[OAMDATA] = 0;
			break;
		case OAMADDH:	/* Address for accessing OAM (high) */
			snes_ppu.oam.address_high = data & 0x1;
			snes_ppu.oam.address = ((data & 0x1) << 8) + snes_ppu.oam.address_low;
			if( data & 0x80 )
				snes_ppu.oam.high_priority = snes_ppu.oam.address;
			snes_ram[OAMDATA] = 0;
			break;
		case OAMDATA:	/* Data for OAM write (DW) */
			{
				snes_oam[snes_ppu.oam.address] = ((snes_oam[snes_ppu.oam.address] >> 8) & 0xff) + (data << 8);
				snes_ram[OAMDATA] = (snes_ram[OAMDATA] + 1) % 2;
				if( snes_ram[OAMDATA] == 0 )
				{
					snes_ram[OAMDATA] = 0;
					snes_ppu.oam.address++;
					snes_ram[OAMADDL] = snes_ppu.oam.address & 0xff;
					snes_ram[OAMADDH] = (snes_ppu.oam.address >> 8) & 0x1;
				}
				return;
			}
		case BGMODE:	/* BG mode and character size settings */
			snes_ppu.mode = data & 0x7;
#ifdef SNES_DBG_VIDHRDW
			if( snes_ppu.mode == 5 || snes_ppu.mode == 6 )
				set_visible_area(0, (SNES_SCR_WIDTH * 2 * 1.75) - 1, 0, snes_ppu.beam.last_visible_line  - 1);
			else
				set_visible_area(0, (SNES_SCR_WIDTH * 2 * 1.75) - 1, 0, snes_ppu.beam.last_visible_line - 1 );
#else
			if( snes_ppu.mode == 5 || snes_ppu.mode == 6 )
				set_visible_area(0, (SNES_SCR_WIDTH * 2) - 1, 0, snes_ppu.beam.last_visible_line - 1 );
			else
				set_visible_area(0, SNES_SCR_WIDTH - 1, 0, snes_ppu.beam.last_visible_line - 1 );
#endif

			snes_ppu.layer[0].tile_size = (data >> 4) & 0x1;
			snes_ppu.layer[1].tile_size = (data >> 5) & 0x1;
			snes_ppu.layer[2].tile_size = (data >> 6) & 0x1;
			snes_ppu.layer[3].tile_size = (data >> 7) & 0x1;
			snes_ppu.update_offsets = 1;
			break;
		case MOSAIC:	/* Size and screen designation for mosaic */
			/* FIXME: We don't support horizontal mosaic yet */
			break;
		case BG1SC:		/* Address for storing SC data BG1 SC size designation */
		case BG2SC:		/* Address for storing SC data BG2 SC size designation  */
		case BG3SC:		/* Address for storing SC data BG3 SC size designation  */
		case BG4SC:		/* Address for storing SC data BG4 SC size designation  */
			snes_ppu.layer[offset - BG1SC].map = (data & 0xfc) << 9;
			snes_ppu.layer[offset - BG1SC].map_size = data & 0x3;
			break;
		case BG12NBA:	/* Address for BG 1 and 2 character data */
			snes_ppu.layer[0].data = (data & 0xf) << 13;
			snes_ppu.layer[1].data = (data & 0xf0) << 9;
			break;
		case BG34NBA:	/* Address for BG 3 and 4 character data */
			snes_ppu.layer[2].data = (data & 0xf) << 13;
			snes_ppu.layer[3].data = (data & 0xf0) << 9;
			break;
		case BG1HOFS:	/* BG1 - horizontal scroll (DW) */
			snes_ppu.layer[0].offset.horizontal = ((snes_ppu.layer[0].offset.horizontal >> 8) & 0xff) + (data << 8);
			snes_ppu.update_offsets = 1;
			return;
		case BG1VOFS:	/* BG1 - vertical scroll (DW) */
			snes_ppu.layer[0].offset.vertical = ((snes_ppu.layer[0].offset.vertical >> 8) & 0xff) + (data << 8);
			snes_ppu.update_offsets = 1;
			return;
		case BG2HOFS:	/* BG2 - horizontal scroll (DW) */
			snes_ppu.layer[1].offset.horizontal = ((snes_ppu.layer[1].offset.horizontal >> 8) & 0xff) + (data << 8);
			snes_ppu.update_offsets = 1;
			return;
		case BG2VOFS:	/* BG2 - vertical scroll (DW) */
			snes_ppu.layer[1].offset.vertical = ((snes_ppu.layer[1].offset.vertical >> 8) & 0xff) + (data << 8);
			snes_ppu.update_offsets = 1;
			return;
		case BG3HOFS:	/* BG3 - horizontal scroll (DW) */
			snes_ppu.layer[2].offset.horizontal = ((snes_ppu.layer[2].offset.horizontal >> 8) & 0xff) + (data << 8);
			snes_ppu.update_offsets = 1;
			return;
		case BG3VOFS:	/* BG3 - vertical scroll (DW) */
			snes_ppu.layer[2].offset.vertical = ((snes_ppu.layer[2].offset.vertical >> 8) & 0xff) + (data << 8);
			snes_ppu.update_offsets = 1;
			return;
		case BG4HOFS:	/* BG4 - horizontal scroll (DW) */
			snes_ppu.layer[3].offset.horizontal = ((snes_ppu.layer[3].offset.horizontal >> 8) & 0xff) + (data << 8);
			snes_ppu.update_offsets = 1;
			return;
		case BG4VOFS:	/* BG4 - vertical scroll (DW) */
			snes_ppu.layer[3].offset.vertical = ((snes_ppu.layer[3].offset.vertical >> 8) & 0xff) + (data << 8);
			snes_ppu.update_offsets = 1;
			return;
		case VMAIN:		/* VRAM address increment value designation */
			{
				/* FIXME: We don't support full graphic properly yet */
				if( data & 0xc )
				{
					vram_fg_incr = vram_fg_count = 0x10 << ((data & 0xc) >> 2);
					vram_fg_cntr = 8;
					vram_fg_offset = 0;
				}
#ifdef SNES_DBG_REG_W
				if( (data & 0x80) != (snes_ram[VMAIN] & 0x80) )
					log_cb(RETRO_LOG_DEBUG, LOGPRE  "VRAM access: %s(%d)\n", (data & 0x80) >> 7?"Word":"Byte", (data & 0x80) >> 7 );
				if( (data & 0xc) && (data & 0xc) != (snes_ram[VMAIN] & 0xc) )
					log_cb(RETRO_LOG_DEBUG, LOGPRE  "Full graphic specified: %d, incr: %d\n", (data & 0xc) >> 2, vram_fg_incr );
#endif
			}
			break;
		case VMADDL:	/* Address for VRAM read/write (low) */
		case VMADDH:	/* Address for VRAM read/write (high) */
			vram_read_offset = 0;
			vram_fg_count = vram_fg_incr;
			vram_fg_cntr = 8;
			break;
		case VMDATAL:	/* Data for VRAM write (low) */
			{
				UINT16 addr = (snes_ram[VMADDH] << 8) | snes_ram[VMADDL];
				if( snes_ram[VMAIN] & 0xc )
					snes_vram[(addr + vram_fg_offset) << 1] = data;
				else
					snes_vram[addr << 1] = data;

				vram_read_offset = 0;
				if( !(snes_ram[VMAIN] & 0x80) )
				{
					if( snes_ram[VMAIN] & 0xc )
					{
						addr++;
						vram_fg_offset += 7;	/* addr increases by 1, plus 7 = 8 */
						vram_fg_count--;
						if( vram_fg_count == 0 )
						{
							vram_fg_cntr--;
							vram_fg_count = vram_fg_incr;
							if( vram_fg_cntr == 0 )
							{
								vram_fg_cntr = 8;
								vram_fg_offset -= 7;
							}
							else
							{
								vram_fg_offset -= (vram_fg_count * 8) - 1;
							}
						}
					}
					else
					{
						switch( snes_ram[VMAIN] & 0x03 )
						{
							case 0: addr++;      break;
							case 1: addr += 32;  break;
							case 2: addr += 128; break; /* Should be 64, but a bug in the snes means it's 128 */
							case 3: addr += 128; break;
						}
					}
					snes_ram[VMADDL] = addr & 0xff;
					snes_ram[VMADDH] = (addr >> 8) & 0xff;
				}
			} return;
		case VMDATAH:	/* Data for VRAM write (high) */
			{
				UINT16 addr = (snes_ram[VMADDH] << 8) | snes_ram[VMADDL];
				if( snes_ram[VMAIN] & 0xc )
					snes_vram[((addr + vram_fg_offset) << 1) + 1] = data;
				else
					snes_vram[(addr << 1) + 1] = data;

				vram_read_offset = 0;
				if( (snes_ram[VMAIN] & 0x80) )
				{
					if( snes_ram[VMAIN] & 0xc )
					{
						addr++;
						vram_fg_offset += 7;	/* addr increases by 1, plus 7 = 8 */
						vram_fg_count--;
						if( vram_fg_count == 0 )
						{
							vram_fg_cntr--;
							vram_fg_count = vram_fg_incr;
							if( vram_fg_cntr == 0 )
							{
								vram_fg_cntr = 8;
								vram_fg_offset -= 7;
							}
							else
							{
								vram_fg_offset -= (vram_fg_count * 8) - 1;
							}
						}
					}
					else
					{
						switch( snes_ram[VMAIN] & 0x03 )
						{
							case 0: addr++;      break;
							case 1: addr += 32;  break;
							case 2: addr += 128; break; /* Should be 64, but a bug in the snes means it's 128 */
							case 3: addr += 128; break;
						}
					}
					snes_ram[VMADDL] = addr & 0xff;
					snes_ram[VMADDH] = (addr >> 8) & 0xff;
				}
			} return;
		case M7SEL:		/* Mode 7 initial settings */
			break;
		case M7A:		/* Mode 7 COS angle/x expansion (DW) */
			snes_ppu.mode7.matrix_a = ((snes_ppu.mode7.matrix_a >> 8) & 0xff) + (data << 8);
			break;
		case M7B:		/* Mode 7 SIN angle/ x expansion (DW) */
			snes_ppu.mode7.matrix_b = ((snes_ppu.mode7.matrix_b >> 8) & 0xff) + (data << 8);
			break;
		case M7C:		/* Mode 7 SIN angle/y expansion (DW) */
			snes_ppu.mode7.matrix_c = ((snes_ppu.mode7.matrix_c >> 8) & 0xff) + (data << 8);
			break;
		case M7D:		/* Mode 7 COS angle/y expansion (DW) */
			snes_ppu.mode7.matrix_d = ((snes_ppu.mode7.matrix_d >> 8) & 0xff) + (data << 8);
			break;
		case M7X:		/* Mode 7 x center position (DW) */
			snes_ppu.mode7.origin_x = ((snes_ppu.mode7.origin_x >> 8) & 0xff) + (data << 8);
			break;
		case M7Y:		/* Mode 7 y center position (DW) */
			snes_ppu.mode7.origin_y = ((snes_ppu.mode7.origin_y >> 8) & 0xff) + (data << 8);
			break;
		case CGADD:		/* Initial address for colour RAM writing */
			/* CGRAM is 16-bit, but when reading/writing we treat it as
			 * 8-bit, so we need to double the address */
			cgram_address = data << 1;
			break;
		case CGDATA:	/* Data for colour RAM */
			((UINT8 *)snes_cgram)[cgram_address] = data;
			cgram_address = (cgram_address + 1) % (SNES_CGRAM_SIZE - 2);
			snes_ppu.update_palette = 1;
			break;
		case W12SEL:	/* Window mask settings for BG1-2 */
		case W34SEL:	/* Window mask settings for BG3-4 */
		case WOBJSEL:	/* Window mask settings for objects */
		case WH0:		/* Window 1 left position */
		case WH1:		/* Window 1 right position */
		case WH2:		/* Window 2 left position */
		case WH3:		/* Window 2 right position */
		case WBGLOG:	/* Window mask logic for BG's */
		case WOBJLOG:	/* Window mask logic for objects */
			if( data != snes_ram[offset] )
				snes_ppu.update_windows = 1;
			break;
		case TM:		/* Main screen designation */
		case TS:		/* Subscreen designation */
		case TMW:		/* Window mask for main screen designation */
		case TSW:		/* Window mask for subscreen designation */
			break;
		case CGWSEL:	/* Initial settings for Fixed colour addition or screen addition */
			/* FIXME: We don't support direct select for modes 3 & 4 or subscreen window stuff */
#ifdef SNES_DBG_REG_W
			if( (data & 0x2) != (snes_ram[CGWSEL] & 0x2) )
				log_cb(RETRO_LOG_DEBUG, LOGPRE  "Add/Sub Layer: %s\n", ((data & 0x2) >> 1) ? "Subscreen" : "Fixed colour" );
#endif
			break;
		case CGADSUB:	/* Addition/Subtraction designation for each screen */
			{
				UINT8 sub = (data & 0x80) >> 7;
				snes_ppu.layer[0].blend = (data & 0x1) << sub;
				snes_ppu.layer[1].blend = ((data & 0x2) >> 1) << sub;
				snes_ppu.layer[2].blend = ((data & 0x4) >> 2) << sub;
				snes_ppu.layer[3].blend = ((data & 0x8) >> 3) << sub;
				snes_ppu.layer[4].blend = ((data & 0x10) >> 4) << sub;
			} break;
		case COLDATA:	/* Fixed colour data for fixed colour addition/subtraction */
			{
				/* Store it in the extra space we made in the CGRAM
				 * It doesn't really go there, but it's as good a place as any. */
				UINT8 r,g,b,fade;

				/* Get existing value. */
				r = snes_cgram[FIXED_COLOUR] & 0x1f;
				g = (snes_cgram[FIXED_COLOUR] & 0x3e0) >> 5;
				b = (snes_cgram[FIXED_COLOUR] & 0x7c00) >> 10;
				/* Set new value */
				if( data & 0x20 )
					r = data & 0x1f;
				if( data & 0x40 )
					g = data & 0x1f;
				if( data & 0x80 )
					b = data & 0x1f;
				snes_cgram[FIXED_COLOUR] = (r | (g << 5) | (b << 10));

				/* set the palette entry, adjusting to the fade setting */
				fade = (snes_ram[INIDISP] & 0xf) + 1;
				r = (r * fade) >> 4;
				g = (g * fade) >> 4;
				b = (b * fade) >> 4;
				Machine->remapped_colortable[FIXED_COLOUR] = ((r & 0x1f) | ((g & 0x1f) << 5) | ((b & 0x1f) << 10));
			} break;
		case SETINI:	/* Screen mode/video select */
			/* FIXME: We only support line count here */
			snes_ppu.beam.last_visible_line = (data & 0x4) ? 240 : 225;
#ifdef SNES_DBG_REG_W
			if( (data & 0x8) != (snes_ram[SETINI] & 0x8) )
				log_cb(RETRO_LOG_DEBUG, LOGPRE  "Pseudo 512 mode: %s\n", (data & 0x8) ? "on" : "off" );
#endif
			break;
		case APU00:		/* Audio port register */
		case APU01:		/* Audio port register */
		case APU02:		/* Audio port register */
		case APU03:		/* Audio port register */
			if( spc_usefakeapu )
				fakespc_port_w( offset & 0x3, data );
			else
			{
				cpu_boost_interleave(0, TIME_IN_USEC(20));
			spc_port_in[offset & 0x3] = data;
			}
			return;
		case WMDATA:	/* Data to write to WRAM */
			{
				UINT32 addr = ((snes_ram[WMADDH] & 0x1) << 16) | (snes_ram[WMADDM] << 8) | snes_ram[WMADDL];

				cpu_writemem24( 0x7e0000 + addr++, data );
				snes_ram[WMADDH] = (addr >> 16) & 0x1;
				snes_ram[WMADDM] = (addr >> 8) & 0xff;
				snes_ram[WMADDL] = addr & 0xff;
				return;
			}
		case WMADDL:	/* Address to read/write to wram (low) */
		case WMADDM:	/* Address to read/write to wram (mid) */
		case WMADDH:	/* Address to read/write to wram (high) */
			break;
		case OLDJOY1:	/* Old NES joystick support */
			if( (data & 0x1) && !(snes_ram[offset] & 0x1) )
			{
				joypad[0].oldrol = 0;
				joypad[1].oldrol = 0;
				joypad[2].oldrol = 0;
				joypad[3].oldrol = 0;
			}
			break;
		case OLDJOY2:	/* Old NES joystick support */
		case NMITIMEN:	/* Flag for v-blank, timer int. and joy read */
		case WRIO:		/* Programmable I/O port */
		case WRMPYA:	/* Multiplier A */
			break;
		case WRMPYB:	/* Multiplier B */
			{
				UINT32 c = snes_ram[WRMPYA] * data;
				snes_ram[RDMPYL] = c & 0xff;
				snes_ram[RDMPYH] = (c >> 8) & 0xff;
			} break;
		case WRDIVL:	/* Dividend (low) */
		case WRDIVH:	/* Dividend (high) */
			break;
		case WRDVDD:	/* Divisor */
			{
				UINT16 value, dividend, remainder;
				dividend = remainder = 0;
				value = (snes_ram[WRDIVH] << 8) + snes_ram[WRDIVL];
				if( data > 0 )
				{
					dividend = value / data;
					remainder = value % data;
				}
				else
				{
					dividend = 0xffff;
					remainder = value;
				}
				snes_ram[RDDIVL] = dividend & 0xff;
				snes_ram[RDDIVH] = (dividend >> 8) & 0xff;
				snes_ram[RDMPYL] = remainder & 0xff;
				snes_ram[RDMPYH] = (remainder >> 8) & 0xff;
			} break;
		case HTIMEL:	/* H-Count timer settings (low)  */
		case HTIMEH:	/* H-Count timer settings (high) */
		case VTIMEL:	/* V-Count timer settings (low)  */
		case VTIMEH:	/* V-Count timer settings (high) */
			break;
		case MDMAEN:	/* GDMA channel designation and trigger */
			snes_gdma( data );
			data = 0;	/* Once DMA is done we need to reset all bits to 0 */
			break;
		case HDMAEN:	/* HDMA channel designation */
			break;
		case MEMSEL:	/* Access cycle designation in memory (2) area */
			/* FIXME: Need to adjust the speed only during access of banks 0x80+
			 * Currently we are just increasing it no matter what */
			cpunum_set_clockscale( 0, (data & 0x1) ? 1.335820896 : 1.0 );
#ifdef SNES_DBG_REG_W
			if( (data & 0x1) != (snes_ram[MEMSEL] & 0x1) )
				log_cb(RETRO_LOG_DEBUG, LOGPRE  "CPU speed: %f Mhz\n", (data & 0x1) ? 3.58 : 2.68 );
#endif
			break;
	/* Following are read-only */
		case HVBJOY:	/* H/V blank and joypad enable */
		case MPYL:		/* Multiplication result (low) */
		case MPYM:		/* Multiplication result (mid) */
		case MPYH:		/* Multiplication result (high) */
		case RDIO:
		case RDDIVL:
		case RDDIVH:
		case RDMPYL:
		case RDMPYH:
		case JOY1L:
		case JOY1H:
		case JOY2L:
		case JOY2H:
		case JOY3L:
		case JOY3H:
		case JOY4L:
		case JOY4H:
#ifdef MAME_DEBUG
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "Write to read-only register: %X value: %X", offset, data );
#endif /* MAME_DEBUG */
			return;
	/* Below is all DMA related */
		case DMAP0: case BBAD0: case A1T0L: case A1T0H: case A1B0: case DAS0L:
		case DAS0H: case DSAB0: case A2A0L: case A2A0H: case NTRL0:
		case DMAP1: case BBAD1: case A1T1L: case A1T1H: case A1B1: case DAS1L:
		case DAS1H: case DSAB1: case A2A1L: case A2A1H: case NTRL1:
		case DMAP2: case BBAD2: case A1T2L: case A1T2H: case A1B2: case DAS2L:
		case DAS2H: case DSAB2: case A2A2L: case A2A2H: case NTRL2:
		case DMAP3: case BBAD3: case A1T3L: case A1T3H: case A1B3: case DAS3L:
		case DAS3H: case DSAB3: case A2A3L: case A2A3H: case NTRL3:
		case DMAP4: case BBAD4: case A1T4L: case A1T4H: case A1B4: case DAS4L:
		case DAS4H: case DSAB4: case A2A4L: case A2A4H: case NTRL4:
		case DMAP5: case BBAD5: case A1T5L: case A1T5H: case A1B5: case DAS5L:
		case DAS5H: case DSAB5: case A2A5L: case A2A5H: case NTRL5:
		case DMAP6: case BBAD6: case A1T6L: case A1T6H: case A1B6: case DAS6L:
		case DAS6H: case DSAB6: case A2A6L: case A2A6H: case NTRL6:
		case DMAP7: case BBAD7: case A1T7L: case A1T7H: case A1B7: case DAS7L:
		case DAS7H: case DSAB7: case A2A7L: case A2A7H: case NTRL7:
			break;
	}

	snes_ram[offset] = data;
}

/* This function checks everything is in a valid range and returns how
 * 'valid' this section is as an information block. */
static int snes_validate_infoblock( UINT8 *infoblock, UINT16 offset )
{
	INT8 valid = 6;

	/* Check the CRC and inverse CRC */
	if( ((infoblock[offset + 0x1c] + (infoblock[offset + 0x1d] << 8)) |
		(infoblock[offset + 0x1e] + (infoblock[offset + 0x1f] << 8))) != 0xffff )
	{
		valid -= 3;
	}
	/* Check the ROM Size is in a valid range */
	if( infoblock[offset + 0x17] > 13 )
	{
		valid--;
	}
	/* Check the SRAM size */
	if( infoblock[offset + 0x18] > 8 )
	{
		valid--;
	}
	/* Check the Country is in a valid range */
	if( infoblock[offset + 0x19] > 13 )
	{
		valid--;
	}
	/* Check the game version */
	if( infoblock[offset + 0x1b] >= 128 )
	{
		valid--;
	}

	if( valid < 0 )
	{
		valid = 0;
	}

	return valid;
}

DRIVER_INIT( snes )
{
	int i;
	UINT16 totalblocks, readblocks;
	UINT8  *rom;

	rom = memory_region( REGION_USER3 );
	snes_ram = memory_region( REGION_CPU1 );
	memset( snes_ram, 0, 0x1000000 );

	/* all NSS games seem to use MODE 20 */
	cart.mode = SNES_MODE_20;
	cart.sram_max = 0x40000;

	/* Find the number of blocks in this ROM */
	/*totalblocks = ((mame_fsize(file) - offset) >> (cart.mode == MODE_20 ? 15 : 16));*/
	totalblocks = (memory_region_length(REGION_USER3) / 0x8000) - 1;

	/* FIXME: Insert crc check here */

	readblocks = 0;
	{
		/* In mode 20, all blocks are 32kb. There are upto 96 blocks, giving a
		 * total of 24mbit(3mb) of ROM.
		 * The first 48 blocks are located in banks 0x00 to 0x2f at address
		 * 0x8000.  They are mirrored in banks 0x80 to 0xaf.
		 * The next 16 blocks are located in banks 0x30 to 0x3f at address
		 * 0x8000.  They are mirrored in banks 0xb0 to 0xbf.
		 * The final 32 blocks are located in banks 0x40 - 0x5f at address
		 * 0x8000.  They are mirrored in banks 0xc0 to 0xdf.
		 */
		i = 0;
		while( i < 96 && readblocks <= totalblocks )
		{
			/*mame_fread( file, &snes_ram[(i++ * 0x10000) + 0x8000], 0x8000);*/
			memcpy(&snes_ram[(i * 0x10000) + 0x8000], &rom[i * 0x8000], 0x8000);
			i++;
			readblocks++;
		}
	}

	/* Find the amount of sram */
	cart.sram = snes_r_bank1(0x00ffd8);
	if( cart.sram > 0 )
	{
		cart.sram = ((1 << (cart.sram + 3)) / 8);
		if( cart.sram > cart.sram_max )
			cart.sram = cart.sram_max;
	}

	free_memory_region(REGION_USER3);
}


INTERRUPT_GEN(snes_scanline_interrupt)
{
	/* Start of VBlank */
	if( snes_ppu.beam.current_vert == snes_ppu.beam.last_visible_line )
	{
		snes_ram[HVBJOY] |= 0x80;		/* Set vblank bit to on */
		snes_ram[STAT77] &= 0x3f;		/* Clear Time Over and Range Over bits - done every nmi (presumably because no sprites drawn here) */
		snes_ram[RDNMI] |= 0x80;		/* Set NMI occured bit */
		if( snes_ram[NMITIMEN] & 0x80 )	/* NMI only signaled if this bit set */
		{
			cpu_set_irq_line( 0, G65816_LINE_NMI, HOLD_LINE );
		}
	}

	/* Setup HDMA on start of new frame */
	if( snes_ppu.beam.current_vert == 0 )
		snes_hdma_init();

	/* Let's draw the current line */
	if( snes_ppu.beam.current_vert < snes_ppu.beam.last_visible_line )
	{
		/* Do HDMA */
		if( snes_ram[HDMAEN] )
			snes_hdma();

		snes_refresh_scanline( snes_ppu.beam.current_vert );
	}
	else
	{
		joypad[0].low = readinputport( 0 );
		joypad[0].high = readinputport( 1 );
		joypad[1].low = readinputport( 2 );
		joypad[1].high = readinputport( 3 );
		joypad[2].low = readinputport( 4 );
		joypad[2].high = readinputport( 5 );
		joypad[3].low = readinputport( 6 );
		joypad[3].high = readinputport( 7 );
	}

	/* Vertical IRQ timer */
	if( snes_ram[NMITIMEN] & 0x20 )
	{
		if( snes_ppu.beam.current_vert == (((snes_ram[VTIMEH] << 8) | snes_ram[VTIMEL]) & 0x1ff) )
		{
			snes_ram[TIMEUP] = 0x80;	/* Indicate that irq occured */
			cpu_set_irq_line( 0, G65816_LINE_IRQ, HOLD_LINE );
		}
	}
	/* Horizontal IRQ timer */
	/* FIXME: Commented out as it causes heaps of trouble right now */
/*	if( snes_ram[NMITIMEN] & 0x10 )
	{
		if( (((snes_ram[HTIMEH] << 8) | snes_ram[HTIMEL]) & 0x1ff) )
		{
			snes_ram[TIMEUP] = 0x80;
			cpu_set_irq_line( 0, G65816_LINE_IRQ, HOLD_LINE );
		}
	} */

	/* Increase current line */
	snes_ppu.beam.current_vert = (snes_ppu.beam.current_vert + 1) % (snes_ram[STAT78] == SNES_NTSC ? SNES_MAX_LINES_NTSC : SNES_MAX_LINES_PAL);
	if( snes_ppu.beam.current_vert == 0 )
	{	/* VBlank is over, time for a new frame */
		cpu_writemem24( OAMADDL, snes_ppu.oam.address_low ); /* Reset oam address */
		cpu_writemem24( OAMADDH, snes_ppu.oam.address_high );
		snes_ram[HVBJOY] &= 0x7f;		/* Clear vblank bit */
		snes_ram[RDNMI]  &= 0x7f;		/* Clear nmi occured bit */
		cpu_set_irq_line( 0, G65816_LINE_NMI, CLEAR_LINE );
	}
}

void snes_hdma_init()
{
	UINT8 mask = 1, dma = 0, i;

	snes_hdma_chnl = snes_ram[HDMAEN];
	for( i = 0; i < 8; i++ )
	{
		if( snes_ram[HDMAEN] & mask )
		{
			snes_ram[SNES_DMA_BASE + dma + 8] = snes_ram[SNES_DMA_BASE + dma + 2];
			snes_ram[SNES_DMA_BASE + dma + 9] = snes_ram[SNES_DMA_BASE + dma + 3];
			snes_ram[SNES_DMA_BASE + dma + 0xa] = 0;
		}
		dma += 0x10;
		mask <<= 1;
	}
}

void snes_hdma()
{
	UINT8 mask = 1, dma = 0, i, contmode;
	UINT16 bbus;
	UINT32 abus;

	/* Assume priority of the 8 DMA channels is 0-7 */
	for( i = 0; i < 8; i++ )
	{
		if( snes_hdma_chnl & mask )
		{
			/* Check if we need to read a new line from the table */
			if( !(snes_ram[SNES_DMA_BASE + dma + 0xa] & 0x7f ) )
			{
				abus = (snes_ram[SNES_DMA_BASE + dma + 4] << 16) + (snes_ram[SNES_DMA_BASE + dma + 9] << 8) + snes_ram[SNES_DMA_BASE + dma + 8];

				/* Get the number of lines */
				snes_ram[SNES_DMA_BASE + dma + 0xa] = cpu_readmem24(abus);
				if( !snes_ram[SNES_DMA_BASE + dma + 0xa] )
				{
					/* No more lines so clear HDMA */
					snes_hdma_chnl &= ~mask;
					continue;
				}
				abus++;
				snes_ram[SNES_DMA_BASE + dma + 8] = abus & 0xff;
				snes_ram[SNES_DMA_BASE + dma + 9] = (abus >> 8) & 0xff;
				if( snes_ram[SNES_DMA_BASE + dma] & 0x40 )
				{
					snes_ram[SNES_DMA_BASE + dma + 5] = cpu_readmem24(abus++);
					snes_ram[SNES_DMA_BASE + dma + 6] = cpu_readmem24(abus++);
					snes_ram[SNES_DMA_BASE + dma + 8] = abus & 0xff;
					snes_ram[SNES_DMA_BASE + dma + 9] = (abus >> 8) & 0xff;
				}
			}

			contmode = (--snes_ram[SNES_DMA_BASE + dma + 0xa]) & 0x80;

			/* Transfer addresses */
			if( snes_ram[SNES_DMA_BASE + dma] & 0x40 )	/* Indirect */
				abus = (snes_ram[SNES_DMA_BASE + dma + 7] << 16) + (snes_ram[SNES_DMA_BASE + dma + 6] << 8) + snes_ram[SNES_DMA_BASE + dma + 5];
			else									/* Absolute */
				abus = (snes_ram[SNES_DMA_BASE + dma + 4] << 16) + (snes_ram[SNES_DMA_BASE + dma + 9] << 8) + snes_ram[SNES_DMA_BASE + dma + 8];
			bbus = 0x2100 + snes_ram[SNES_DMA_BASE + dma + 1];

#ifdef SNES_DBG_HDMA
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "HDMA-Ch: %d(%s) abus: %X bbus: %X type: %d(%X %X)\n", i, snes_ram[SNES_DMA_BASE + dma] & 0x40 ? "Indirect" : "Absolute", abus, bbus, snes_ram[SNES_DMA_BASE + dma] & 0x7, snes_ram[SNES_DMA_BASE + dma + 8],snes_ram[SNES_DMA_BASE + dma + 9] );
#endif

			switch( snes_ram[SNES_DMA_BASE + dma] & 0x7 )
			{
				case 0:		/* 1 address */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
				} break;
				case 1:		/* 2 addresses (l,h) */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 1, cpu_readmem24(abus++));
				} break;
				case 2:		/* Write twice (l,l) */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus, cpu_readmem24(abus++));
				} break;
				case 3:		/* 2 addresses/Write twice (l,l,h,h) */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 1, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 1, cpu_readmem24(abus++));
				} break;
				case 4:		/* 4 addresses (l,h,l,h) */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 1, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 2, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 3, cpu_readmem24(abus++));
				} break;
				default:
#ifdef MAME_DEBUG
					log_cb(RETRO_LOG_DEBUG, LOGPRE  "  HDMA of unsupported type: %d\n", snes_ram[SNES_DMA_BASE + dma] & 0x7 );
#endif
					break;
			}

			/* Update address */
			if( contmode )
			{
				if( snes_ram[SNES_DMA_BASE + dma] & 0x40 )	/* Indirect */
				{
					snes_ram[SNES_DMA_BASE + dma + 5] = abus & 0xff;
					snes_ram[SNES_DMA_BASE + dma + 6] = (abus >> 8) & 0xff;
				}
				else									/* Absolute */
				{
					snes_ram[SNES_DMA_BASE + dma + 8] = abus & 0xff;
					snes_ram[SNES_DMA_BASE + dma + 9] = (abus >> 8) & 0xff;
				}
			}

			if( !(snes_ram[SNES_DMA_BASE + dma + 0xa] & 0x7f) )
			{
				if( !(snes_ram[SNES_DMA_BASE + dma] & 0x40) )	/* Absolute */
				{
					if( !contmode )
					{
						snes_ram[SNES_DMA_BASE + dma + 8] = abus & 0xff;
						snes_ram[SNES_DMA_BASE + dma + 9] = (abus >> 8) & 0xff;
					}
				}
			}
		}
		dma += 0x10;
		mask <<= 1;
	}
}

void snes_gdma( UINT8 channels )
{
	UINT8 mask = 1, dma = 0, i;
	INT8 increment;
	UINT16 bbus;
	UINT32 abus, length;

	/* Assume priority of the 8 DMA channels is 0-7 */
	for( i = 0; i < 8; i++ )
	{
		if( channels & mask )
		{
			/* Find transfer addresses */
			abus = (snes_ram[SNES_DMA_BASE + dma + 4] << 16) + (snes_ram[SNES_DMA_BASE + dma + 3] << 8) + snes_ram[SNES_DMA_BASE + dma + 2];
			bbus = 0x2100 + snes_ram[SNES_DMA_BASE + dma + 1];

			/* Auto increment */
			if( snes_ram[SNES_DMA_BASE + dma] & 0x8 )
			{
				increment = 0;
			}
			else
			{
				if( snes_ram[SNES_DMA_BASE + dma] & 0x10 )
					increment = -1;
				else
					increment = 1;
			}

			/* Number of bytes to transfer */
			length = (snes_ram[SNES_DMA_BASE + dma + 6] << 8) + snes_ram[SNES_DMA_BASE + dma + 5];
			if( !length )
				length = 0x10000;	/* 0x0000 really means 0x10000 */

#ifdef SNES_DBG_GDMA
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "GDMA-Ch %d: len: %X, abus: %X, bbus: %X, incr: %d, dir: %s, type: %d\n", i, length, abus, bbus, increment, snes_ram[SNES_DMA_BASE + dma] & 0x80 ? "PPU->CPU" : "CPU->PPU", snes_ram[SNES_DMA_BASE + dma] & 0x7 );
#endif

			switch( snes_ram[SNES_DMA_BASE + dma] & 0x7 )
			{
				case 0:		/* 1 address */
				case 2:		/* 1 address ?? */
				{
					while( length-- )
					{
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
					}
				} break;
				case 1:		/* 2 addresses (l,h) */
				{
					while( length-- )
					{
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 1) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 1, cpu_readmem24(abus) );
						abus += increment;
					}
				} break;
				case 3:		/* 2 addresses/write twice (l,l,h,h) */
				{
					while( length-- )
					{
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 1) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 1, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 1) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 1, cpu_readmem24(abus) );
						abus += increment;
					}
				} break;
				case 4:		/* 4 addresses (l,h,l,h) */
				{
					while( length-- )
					{
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 1) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 1, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 2) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 2, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[SNES_DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 3) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 3, cpu_readmem24(abus) );
						abus += increment;
					}
				} break;
				default:
#ifdef MAME_DEBUG
					log_cb(RETRO_LOG_DEBUG, LOGPRE  "  GDMA of unsupported type: %d\n", snes_ram[SNES_DMA_BASE + dma] & 0x7 );
#endif
					break;
			}
			/* We're done so write the new abus back to the registers */
			snes_ram[SNES_DMA_BASE + dma + 2] = abus & 0xff;
			snes_ram[SNES_DMA_BASE + dma + 3] = (abus >> 8) & 0xff;
			snes_ram[SNES_DMA_BASE + dma + 5] = 0;
			snes_ram[SNES_DMA_BASE + dma + 6] = 0;
		}
		dma += 0x10;
		mask <<= 1;
	}
}
