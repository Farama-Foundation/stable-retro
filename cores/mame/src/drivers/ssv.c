/***************************************************************************

					-= Seta, Sammy, Visco (SSV) System =-

					driver by	Luca Elia (l.elia@tin.it)


CPU          :		NEC V60

Sound Chip   :		Ensoniq ES5506 (OTTOR2)

Custom Chips :		ST-0004		(Video DAC)
					ST-0005		(Parallel I/O)
					ST-0006		(Video controller)
					ST-0007		(System controller)

Others       :		Battery + MB3790 + LH5168D-10L (NVRAM)
					DX-102				(I/O)
					M62X42B				(RTC)
					ST010
					TA8210				(Audio AMP)
					uPD71051/7001C		(UART)

-----------------------------------------------------------------------------------
Main Board	ROM Board	Year + Game									By
-----------------------------------------------------------------------------------
STA-0001	STS-0001	93	Super Real Mahjong PIV					Seta
STA-0001	STS-0001	93	Dramatic Adventure Quiz Keith & Lucy	Visco
STA-0001	SAM-5127	93	Survival Arts							Sammy
STA-0001    SAM-5127    93  DynaGears                               Sammy
STA-0001B	VISCO-001B	94	Drift Out '94							Visco
STA-0001B	GOLF ROM	94	Eagle Shot Golf 					    Sammy
STA-0001B	?			94	Twin Eagle II - The Rescue Mission   	Seta
STA-0001B	P1-102A		95	Mahjong Hyper Reaction					Sammy
?			?			95	Ultra X Weapons / Ultra Keibitai 	Banpresto + Tsuburaya Prod.
STA-0001B	VISCO-JJ1	96	Lovely Pop Mahjong Jan Jan Shimasyo		Visco
STA-0001B	VISCO-001B	96	Storm Blade								Visco
STA-0001B	P1-105A		96?	Meosis Magic							Sammy
STA-0001B	?			97	Joryuu Syougi Kyoushitsu (1)			Visco
STA-0001B	VISCO-JJ1	97	Koi Koi Shimasyo 2						Visco
STA-0001B	P1-112A		97	Mahjong Hyper Reaction 2				Sammy
STA-0001B	?			97	Monster Slider							Visco / Datt
STA-0001	?			97	Super Real Mahjong P7					Seta
STA-0001B	?			98	Gourmet Battle Quiz Ryorioh CooKing		Visco
STA-0001B	P1-112C		98	Pachinko Sexy Reaction					Sammy
STA-0001B	P1-112C		99  Change Air Blade						Visco
STA-0001B	SSV_SUB     00  Vasara									Visco
STA-0001B	SSV_SUB		01  Vasara 2								Visco
-----------------------------------------------------------------------------------

(1) Uses an unemulated NEC V810 CPU instead of the V60.


Games not yet dumped:
						?	Kidou Senshi Gundam Final Shooting		Visco / Banpresto
						?	Pachinko Sexy Reaction 2				Sammy


STA-0001 & STA-0001B should be fully interchangable, its reported that STA-0001 runs at
12mhz whereas STA-0001B runs at 16.

To Do:

- all games	:	CRT controller (resolution+visible area+flip screen?)
				Proper "shadows" support: the low 4 bits of the pens from a "shadowing"
				tile (regardless of color code) substitute the top 4 bits of the
				color index (0-7fff) in the frame buffer.

- hypreac2	:	communication with other units
				tilemap sprites use the yoffset specified in the sprites-list?
				(see the 8 pixel gap between the backgrounds and the black rows)

- mslider   :   V60 issues?  the correct tiles don't always vanish, depending on
                the layout of the 3 tiles you put together sometimes only 2 vanish
                and a blank are off the bottom of the game board flashes instead
                of the correct 3rd tile

- srmp4		:	Backgrounds are offset by $60 pixels, so they're kludged to work

- srmp7		:	Needs interrupts by the sound chip (unsupported yet). Kludged to work.

- ultrax    :   bad gfx offsets and wrong visible area
- twineag2  :   bad gfx offsets on some scenes

- dynagear  :   Requires 2 kludges for the video emulation and has some bad shadow sprites
                on the left side of the screen

	code @ $e75cdc

	 W:
	 		0x482000 - 0x482007 - values taken from obj table
	 		0x482040 - 0x482043 - write latch ?

	 R:
	 		0x482022 - 0x482023 - result = direction ,
	 													probably : 00 = down, 40 = left, 80 = up, c0 = right
	 		0x482042 - 0x482043 - protection status bits ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/random.h"
#include "machine/eeprom.h"
#include "seta.h"

#include <math.h>

/***************************************************************************


								Interrupts


***************************************************************************/

static UINT8 requested_int;
static data16_t *ssv_irq_vectors;
static data16_t irq_enable;
static data16_t *ssv_mainram;

/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	cpu_set_irq_line(0, 0, (requested_int & irq_enable)? ASSERT_LINE : CLEAR_LINE);
}

int ssv_irq_callback(int level)
{
	int i;
	for ( i = 0; i <= 7; i++ )
	{
		if (requested_int & (1 << i))
		{
			data16_t vector = ssv_irq_vectors[i * (16/2)] & 7;
			return vector;
		}
	}
	return 0;
}

WRITE16_HANDLER( ssv_irq_ack_w )
{
	int level = ((offset * 2) & 0x70) >> 4;
	requested_int &= ~(1 << level);

	update_irq_state();
}

/*
	IRQ Enable Register:

	drifto94:	c at the start
	hypreact:	ff at the start
	hypreac2:	ff at the start
	janjans1:	0,6c,60
	keithlcy:	c at the start
	meosism:	ff at the start
	mslider:	c at the start
	ryorioh:	0,c at the start
	srmp4:		8 at the start
	srmp7:		8 at the start, 28, 40 (seems related to 21000e writes)
	survarts:	0,8 at the start
	sxyreact:	ff at the start
	ultrax:		40,00 at the start then 42,4a
	twineag2:	40,00 at the start then 42,4a
*/
WRITE16_HANDLER( ssv_irq_enable_w )
{
	COMBINE_DATA(&irq_enable);
}

static int interrupt_ultrax;

INTERRUPT_GEN( ssv_interrupt )
{
	if (cpu_getiloops())
	{
		if(interrupt_ultrax)
		{
			requested_int |= 1 << 1;	/* needed by ultrax to coin up, breaks cairblad*/
			update_irq_state();
		}
	}
	else
	{
		requested_int |= 1 << 3;	/* vblank*/
		update_irq_state();
	}
}

INTERRUPT_GEN( gdfs_interrupt )
{
	if (cpu_getiloops())
	{
		requested_int |= 1 << 6;	/* reads lightgun (4 times for 4 axis) */
		update_irq_state();
	}
	else
	{
		requested_int |= 1 << 3;	/* vblank */
		update_irq_state();
	}
}


/***************************************************************************


							Coins Lockout / Counter


***************************************************************************/

/*
	drifto94:	c3
	janjans1:	c3
	keithlcy:	c3
	mslider:	c3, 83 in test mode
	ryorioh:	c3

	hypreac2:	80
	hypreact:	80
	meosism:	83
	srmp4:		83, c0 in test mode (where only tilemap sprites are used)
	srmp7:		80
	survarts:	83
	sxyreact:	80
*/
static WRITE16_HANDLER( ssv_lockout_w )
{
/*	usrintf_showmessage("%02X",data & 0xff);*/
	if (ACCESSING_LSB)
	{
		coin_lockout_w(1,~data & 0x01);
		coin_lockout_w(0,~data & 0x02);
		coin_counter_w(1, data & 0x04);
		coin_counter_w(0, data & 0x08);
/*		                  data & 0x40?*/
		ssv_enable_video( data & 0x80);
	}
}

/* Same as above but with inverted lockout lines */
static WRITE16_HANDLER( ssv_lockout_inv_w )
{
/*	usrintf_showmessage("%02X",data & 0xff);*/
	if (ACCESSING_LSB)
	{
		coin_lockout_w(1, data & 0x01);
		coin_lockout_w(0, data & 0x02);
		coin_counter_w(1, data & 0x04);
		coin_counter_w(0, data & 0x08);
/*		                  data & 0x40?*/
		ssv_enable_video( data & 0x80);
	}
}

MACHINE_INIT( ssv )
{
	requested_int = 0;
	cpu_set_irq_callback(0, ssv_irq_callback);
	cpu_setbank(1, memory_region(REGION_USER1));
}


/***************************************************************************


							Non-Volatile RAM


***************************************************************************/

static data16_t *ssv_nvram;
static size_t    ssv_nvram_size;

NVRAM_HANDLER( ssv )
{
	if (read_or_write)
		mame_fwrite(file, ssv_nvram, ssv_nvram_size);
	else
		if (file)
			mame_fread(file, ssv_nvram, ssv_nvram_size);
}

NVRAM_HANDLER( gdfs )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file) EEPROM_load(file);
		else
		{
			/* Set the EEPROM to Factory Defaults */
		}
	}
}

/***************************************************************************


								DSP


***************************************************************************/


static UINT16 *dsp_ram;

static WRITE16_HANDLER( dsp_w )
{
	COMBINE_DATA(dsp_ram+offset);
	if(offset == 0x21 && dsp_ram[0x21]) {
		switch(dsp_ram[0x20]) {
		case 0x0001:
			dsp_ram[0x11] = (UINT8)(128*atan2(dsp_ram[0] - dsp_ram[1], dsp_ram[2] - dsp_ram[3])/M_PI) ^ 0x80;
			dsp_ram[0x21] = 0;
			break;
		default:
			dsp_ram[0x21] = 0;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "SSV DSP: unknown function %x (%x)\n", dsp_ram[0x20], activecpu_get_pc());
			break;
		}
	}
}



/***************************************************************************


								Memory Maps


***************************************************************************/

/*static READ16_HANDLER( fake_r )	{	return ssv_scroll[offset];	}*/

#define SSV_READMEM( _ROM  )										\
	{ 0x000000, 0x00ffff, MRA16_RAM				},	/*	RAM		*/	\
	{ 0x100000, 0x13ffff, MRA16_RAM				},	/*	Sprites	*/	\
	{ 0x140000, 0x15ffff, MRA16_RAM				},	/*	Palette	*/	\
	{ 0x160000, 0x17ffff, MRA16_RAM				},	/*			*/	\
	{ 0x1c0000, 0x1c0001, ssv_vblank_r			},	/*	Vblank?	*/	\
/**/{ 0x1c0002, 0x1c007f, MRA16_RAM				},	/*	Scroll	*/	\
	{ 0x210002, 0x210003, input_port_0_word_r	},	/*	DSW		*/	\
	{ 0x210004, 0x210005, input_port_1_word_r	},	/*	DSW		*/	\
	{ 0x210008, 0x210009, input_port_2_word_r	},	/*	P1		*/	\
	{ 0x21000a, 0x21000b, input_port_3_word_r	},	/*	P2		*/	\
	{ 0x21000c, 0x21000d, input_port_4_word_r	},	/*	Coins	*/	\
	{ 0x21000e, 0x21000f, MRA16_NOP				},	/*			*/	\
	{ 0x300000, 0x30007f, ES5506_data_0_word_r	},	/*	Sound	*/	\
    { 0x482000, 0x482fff, MRA16_RAM             },  /*	DSP	    */	\
	{ _ROM,     0xffffff, MRA16_BANK1			},	/*	ROM		*/	\
	/*{ 0x990000, 0x99007f, fake_r	},*/

#define SSV_WRITEMEM														\
	{ 0x000000, 0x00ffff, MWA16_RAM, &ssv_mainram	    },	/*	RAM			*/	\
	{ 0x100000, 0x13ffff, MWA16_RAM, &spriteram16		},	/*	Sprites		*/	\
	{ 0x140000, 0x15ffff, paletteram16_xrgb_swap_word_w, &paletteram16	},		\
	{ 0x160000, 0x17ffff, MWA16_RAM						},	/*				*/	\
	{ 0x1c0000, 0x1c007f, ssv_scroll_w, &ssv_scroll		},	/*	Scroll		*/	\
	{ 0x21000e, 0x21000f, ssv_lockout_w					},	/*	Lockout		*/	\
	{ 0x210010, 0x210011, MWA16_NOP						},	/*				*/	\
	{ 0x230000, 0x230071, MWA16_RAM, &ssv_irq_vectors	},	/*	IRQ Vectors	*/	\
	{ 0x240000, 0x240071, ssv_irq_ack_w					},	/*	IRQ Ack.	*/	\
	{ 0x260000, 0x260001, ssv_irq_enable_w				},	/*	IRQ Enable	*/	\
	{ 0x300000, 0x30007f, ES5506_data_0_word_w			},	/*	Sound		*/  \
    { 0x482000, 0x482fff, dsp_w, &dsp_ram               },  /*	DSP	        */	\
	/*{ 0x990000, 0x99007f, ssv_scroll_w	},*/


static data16_t *ssv_input_sel;

/***************************************************************************
								Drift Out '94
***************************************************************************/

static READ16_HANDLER( drifto94_rand_r )
{
	return mame_rand() & 0xffff;
}

static MEMORY_READ16_START( drifto94_readmem )
	{ 0x480000, 0x480001, MRA16_NOP				},	/* ?*/
	{ 0x510000, 0x510001, drifto94_rand_r		},	/* ??*/
	{ 0x520000, 0x520001, drifto94_rand_r		},	/* ??*/
	{ 0x580000, 0x5807ff, MRA16_RAM				},	/* NVRAM*/
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( drifto94_writemem )
/*	{ 0x210002, 0x210003, MWA16_NOP				},	*/ /* ? 1 at the start*/
	{ 0x400000, 0x47ffff, MWA16_RAM				},	/* ?*/
	{ 0x480000, 0x480001, MWA16_NOP				},	/* ?*/
	{ 0x483000, 0x485fff, MWA16_NOP				},	/* ?*/
	{ 0x500000, 0x500001, MWA16_NOP				},	/* ??*/
	{ 0x580000, 0x5807ff, MWA16_RAM, &ssv_nvram, &ssv_nvram_size	},	/* NVRAM*/
	SSV_WRITEMEM
MEMORY_END

/***************************************************************************
  Eagle Shot Golf
***************************************************************************/

static data8_t trackball_select, gfxrom_select;


static READ16_HANDLER( eaglshot_gfxrom_r )
{
	UINT8 *rom	=	memory_region(REGION_GFX1);
	size_t size	=	memory_region_length(REGION_GFX1);

	offset = offset * 2 + gfxrom_select * 0x200000;

	if (offset > size)
		return 0xffff;

	return rom[offset] + (rom[offset+1]<<8);
}

static WRITE16_HANDLER( eaglshot_gfxrom_w )
{
	if (ACCESSING_LSB)
		gfxrom_select = data;
}

static READ16_HANDLER( eaglshot_trackball_r )
{
	switch(trackball_select)
	{
		case 0x60:	return (readinputport(5) >> 8) & 0xff;
		case 0x40:	return (readinputport(5) >> 0) & 0xff;

		case 0x70:	return (readinputport(6) >> 8) & 0xff;
		case 0x50:	return (readinputport(6) >> 0) & 0xff;
	}
	return 0;
}

static WRITE16_HANDLER( eaglshot_trackball_w )
{
	if (ACCESSING_LSB)
		trackball_select = data;
}



static READ16_HANDLER( eaglshot_gfxram_r )
{
	return eaglshot_gfxram[offset + (ssv_scroll[0x76/2] & 0xf) * 0x40000/2];
}

static WRITE16_HANDLER( eaglshot_gfxram_w )
{
	offset += (ssv_scroll[0x76/2] & 0xf) * 0x40000/2;
	COMBINE_DATA(&eaglshot_gfxram[offset]);

	eaglshot_dirty = 1;
	eaglshot_dirty_tile[offset / (16*8/2)] = 1;
}


static MEMORY_READ16_START( eaglshot_readmem )
    { 0x180000, 0x1bffff, eaglshot_gfxram_r		},
	{ 0x210000, 0x210001, MRA16_NOP		},	/* Watchdog */
	{ 0xa00000, 0xbfffff, eaglshot_gfxrom_r		},
	{ 0xc00000, 0xc007ff, MRA16_RAM				},	/* NVRAM */
	{ 0xd00000, 0xd00001, eaglshot_trackball_r	},
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( eaglshot_writemem )
    { 0x180000, 0x1bffff, eaglshot_gfxram_w		},
/*  { 0x210002, 0x210003, MWA16_NOP             },    ? 0,4 at the start */
    { 0x21000e, 0x21000f, ssv_lockout_inv_w		},	/* Inverted lockout lines */
	{ 0x800000, 0x800001, eaglshot_gfxrom_w		},
	{ 0x900000, 0x900001, eaglshot_trackball_w	},
	{ 0xc00000, 0xc007ff, MWA16_RAM, &ssv_nvram, &ssv_nvram_size },	/* NVRAM */
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
                     Mobile Suit Gundam Final Shooting
***************************************************************************/

static int gdfs_gfxram_bank, gdfs_lightgun_select;
static data16_t *gdfs_blitram;

READ16_HANDLER( gdfs_eeprom_r )
{
	return (((gdfs_lightgun_select & 1) ? 0 : 0xff) ^ readinputport(5 + gdfs_lightgun_select)) | (EEPROM_read_bit() << 8);
}

WRITE16_HANDLER( gdfs_eeprom_w )
{
	static data16_t data_old;

	if (data & ~0x7b00)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC: %06X - Unknown EEPROM bit written %04X\n",activecpu_get_pc(),data);

	if ( ACCESSING_MSB )
	{
/*      data & 0x8000 ? (near palette writes) */
/*      data & 0x0001 ? */

		/* latch the bit */
		EEPROM_write_bit(data & 0x4000);

		/* reset line asserted: reset. */
		EEPROM_set_cs_line((data & 0x1000) ? CLEAR_LINE : ASSERT_LINE );

		/* clock line asserted: write latch or select next bit to read */
		EEPROM_set_clock_line((data & 0x2000) ? ASSERT_LINE : CLEAR_LINE );

		if (!(data_old & 0x0800) && (data & 0x0800))	/* rising clock */
			gdfs_lightgun_select = (data & 0x0300) >> 8;
	}

	COMBINE_DATA(&data_old);
}


static READ16_HANDLER( gdfs_gfxram_r )
{
	return eaglshot_gfxram[offset + gdfs_gfxram_bank * 0x100000/2];
}

static WRITE16_HANDLER( gdfs_gfxram_w )
{
	offset += gdfs_gfxram_bank * 0x100000/2;
	COMBINE_DATA(&eaglshot_gfxram[offset]);

	eaglshot_dirty = 1;
	eaglshot_dirty_tile[offset / (16*8/2)] = 1;
}

static READ16_HANDLER( gdfs_blitram_r )
{
	switch (offset)
	{
		case 0x00/2:
			/* blitter status? (bit C, bit A) */
			return 0;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC: %06X - Blit reg read: %02X\n",activecpu_get_pc(),offset*2);
	return 0;
}

static WRITE16_HANDLER( gdfs_blitram_w )
{
	COMBINE_DATA(&gdfs_blitram[offset]);

	switch (offset)
	{
		case 0x8a/2:
		{
			if (data & ~0x43)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC: %06X - Unknown gdfs_gfxram_bank bit written %04X\n",activecpu_get_pc(),data);

			if (ACCESSING_LSB)
				gdfs_gfxram_bank = data & 3;
		}
		break;

		case 0xc0/2:
		case 0xc2/2:
		case 0xc4/2:
		case 0xc6/2:
		case 0xc8/2:
		break;

		case 0xca/2:
		{
			UINT32 src	=	(gdfs_blitram[0xc0/2] + (gdfs_blitram[0xc2/2] << 16)) << 1;
			UINT32 dst	=	(gdfs_blitram[0xc4/2] + (gdfs_blitram[0xc6/2] << 16)) << 4;
			UINT32 len	=	(gdfs_blitram[0xc8/2]) << 4;

			UINT8 *rom	=	memory_region(REGION_GFX2);
			size_t size	=	memory_region_length(REGION_GFX2);

			if ( (src+len <= size) && (dst+len <= 4 * 0x100000) )
			{
				eaglshot_dirty = 1;

				memcpy( &eaglshot_gfxram[dst/2], &rom[src], len );

				if (len % (16*8))	len = len / (16*8) + 1;
				else				len = len / (16*8);

				memset( &eaglshot_dirty_tile[dst / (16*8)], 1, len );
			}
			else
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC: %06X - Blit out of range: src %x, dst %x, len %x\n",activecpu_get_pc(),src,dst,len);
			}
		}
		break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC: %06X - Blit reg written: %02X <- %04X\n",activecpu_get_pc(),offset*2,data);
	}
}

static MEMORY_READ16_START( gdfs_readmem )
    { 0x540000, 0x540001, gdfs_eeprom_r },
	{ 0x600000, 0x600fff, MRA16_RAM },
	{ 0x400000, 0x43ffff, MRA16_RAM },
	{ 0x440000, 0x44003f, MRA16_RAM },
	{ 0x800000, 0x87ffff, MRA16_RAM },
	{ 0x8c0000, 0x8c00ff, gdfs_blitram_r },
	{ 0x900000, 0x9fffff, gdfs_gfxram_r },
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( gdfs_writemem )
    { 0x400000, 0x41ffff, gdfs_tmapram_w, &gdfs_tmapram },
	{ 0x420000, 0x43ffff, MWA16_RAM },
	{ 0x440000, 0x44003f, MWA16_RAM, &gdfs_tmapscroll },
	{ 0x500000, 0x500001, gdfs_eeprom_w },
	{ 0x600000, 0x600fff, MWA16_RAM },
	{ 0x800000, 0x87ffff, MWA16_RAM, &spriteram16_2 },
	{ 0x8c0000, 0x8c00ff, gdfs_blitram_w, &gdfs_blitram },
	{ 0x900000, 0x9fffff, gdfs_gfxram_w },
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Hyper Reaction
***************************************************************************/

/*
	The game prints "backup ram ok" and there is code to test some ram
	at 0x580000-0x5bffff. The test is skipped and this ram isn't used
	though. I guess it's either a left-over or there are different
	version with some battery backed RAM (which would indeed be on the
	rom-board, AFAIK)
*/

static READ16_HANDLER( hypreact_input_r )
{
	data16_t input_sel = *ssv_input_sel;
	if (input_sel & 0x0001)	return readinputport(5);
	if (input_sel & 0x0002)	return readinputport(6);
	if (input_sel & 0x0004)	return readinputport(7);
	if (input_sel & 0x0008)	return readinputport(8);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X: unknown input read: %04X\n",activecpu_get_pc(),input_sel);
	return 0xffff;
}

static MEMORY_READ16_START( hypreact_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r		},	/* Watchdog*/
/*	{ 0x280000, 0x280001, MRA16_NOP					},	*/ /* ? read at the start, value not used*/
	{ 0xc00000, 0xc00001, hypreact_input_r			},	/* Inputs*/
	{ 0xc00006, 0xc00007, MRA16_RAM					},	/**/
	{ 0xc00008, 0xc00009, MRA16_NOP					},	/**/
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( hypreact_writemem )
/*	{ 0x210002, 0x210003, MWA16_NOP					},	*/ /* ? 5 at the start*/
	{ 0x21000e, 0x21000f, ssv_lockout_inv_w			},	/* Inverted lockout lines*/
	{ 0xc00006, 0xc00007, MWA16_RAM, &ssv_input_sel	},	/* Inputs*/
	{ 0xc00008, 0xc00009, MWA16_NOP					},	/**/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Hyper Reaction 2
***************************************************************************/

static MEMORY_READ16_START( hypreac2_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r		},	/* Watchdog*/
/*	{ 0x280000, 0x280001, MRA16_NOP					},	*/ /* ? read at the start, value not used*/
	{ 0x500000, 0x500001, hypreact_input_r			},	/* Inputs*/
	{ 0x500002, 0x500003, hypreact_input_r			},	/* (again?)*/
/*	  0x540000, 0x540003  communication with another unit*/
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( hypreac2_writemem )
/*	{ 0x210002, 0x210003, MWA16_NOP					},	*/ /* ? 5 at the start*/
	{ 0x21000e, 0x21000f, ssv_lockout_inv_w			},	/* Inverted lockout lines*/
	{ 0x520000, 0x520001, MWA16_RAM, &ssv_input_sel	},	/* Inputs*/
/*	  0x540000, 0x540003  communication with other units*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Jan Jan Simasyo
***************************************************************************/

static READ16_HANDLER( srmp4_input_r );

static MEMORY_READ16_START( janjans1_readmem )
	{ 0x210006, 0x210007, MRA16_NOP					},
	{ 0x800002, 0x800003, srmp4_input_r				},	/* Inputs*/
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( janjans1_writemem )
	{ 0x210000, 0x210001, MWA16_NOP					},	/* koikois2 but not janjans1*/
/*	{ 0x210002, 0x210003, MWA16_NOP					},	*/ /* ? 1 at the start*/
	{ 0x800000, 0x800001, MWA16_RAM, &ssv_input_sel	},	/* Inputs*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Keith & Lucy
***************************************************************************/

static MEMORY_READ16_START( keithlcy_readmem )
	{ 0x21000e, 0x21000f, MRA16_NOP			},	/**/
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( keithlcy_writemem )
/*	{ 0x210002, 0x210003, MWA16_NOP			},	*/ /* ? 1 at the start*/
	{ 0x210010, 0x210011, MWA16_NOP			},	/**/
	{ 0x400000, 0x47ffff, MWA16_RAM			},	/* ?*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Meosis Magic
***************************************************************************/

static MEMORY_READ16_START( meosism_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r	},	/* Watchdog*/
/*	{ 0x280000, 0x280001, MRA16_NOP				},	*/ /* ? read once, value not used*/
	{ 0x580000, 0x58ffff, MRA16_RAM				},	/* NVRAM*/
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( meosism_writemem )
/*	{ 0x210002, 0x210003, MWA16_NOP				},	*/ /* ? 5 at the start*/
/*	{ 0x500004, 0x500005, MWA16_NOP				},	*/ /* ? 0,58,18*/
	{ 0x580000, 0x58ffff, MWA16_RAM, &ssv_nvram, &ssv_nvram_size	},	/* NVRAM*/
	SSV_WRITEMEM
MEMORY_END

/***************************************************************************
								Monster Slider
***************************************************************************/

/* Monster Slider needs the RAM mirrored for the gameplay logic to work correctly */

static READ16_HANDLER( ssv_mainram_r )
{
	return ssv_mainram[offset];
}

static WRITE16_HANDLER( ssv_mainram_w )
{
	COMBINE_DATA(&ssv_mainram[offset]);
}

static MEMORY_READ16_START( mslider_readmem )
	{ 0x010000, 0x01ffff, ssv_mainram_r	   },	/* RAM Mirror*/
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( mslider_writemem )
	{ 0x010000, 0x01ffff, ssv_mainram_w		},	/* RAM Mirror*/
/*	{ 0x210002, 0x210003, MWA16_NOP			},	*/ /* ? 1 at the start*/
	{ 0x400000, 0x47ffff, MWA16_RAM			},	/* ?*/
/*	{ 0x500000, 0x500001, MWA16_NOP			},	*/ /* ? ff at the start*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
					Gourmet Battle Quiz Ryohrioh CooKing
***************************************************************************/

static MEMORY_READ16_START( ryorioh_readmem )
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( ryorioh_writemem )
	{ 0x210000, 0x210001, watchdog_reset16_w	},	/* Watchdog*/
/*	{ 0x210002, 0x210003, MWA16_NOP				},	*/ /* ? 1 at the start*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
							Super Real Mahjong PIV
***************************************************************************/

static READ16_HANDLER( srmp4_input_r )
{
	data16_t input_sel = *ssv_input_sel;
	if (input_sel & 0x0002)	return readinputport(5);
	if (input_sel & 0x0004)	return readinputport(6);
	if (input_sel & 0x0008)	return readinputport(7);
	if (input_sel & 0x0010)	return readinputport(8);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X: unknown input read: %04X\n",activecpu_get_pc(),input_sel);
	return 0xffff;
}

static MEMORY_READ16_START( srmp4_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r		},	/* Watchdog*/
	{ 0xc0000a, 0xc0000b, srmp4_input_r				},	/* Inputs*/
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( srmp4_writemem )
/*	{ 0x210002, 0x210003, MWA16_NOP					},	*/ /* ? 1,5 at the start*/
	{ 0xc0000e, 0xc0000f, MWA16_RAM, &ssv_input_sel	},	/* Inputs*/
	{ 0xc00010, 0xc00011, MWA16_NOP					},	/**/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
							Super Real Mahjong P7
***************************************************************************/

/*
	Interrupts aren't supported by the chip emulator yet
	(lev 5 in this case, I guess)
*/
static READ16_HANDLER( srmp7_irqv_r )
{
	return 0x0080;
}

static WRITE16_HANDLER( srmp7_sound_bank_w )
{
	if (ACCESSING_LSB)
	{
		int bank = 0x400000 * (data & 1);
		ES5506_voice_bank_0_w(2, bank);
		ES5506_voice_bank_0_w(3, bank);
	}
/*	usrintf_showmessage("%04X",data);*/
}

static READ16_HANDLER( srmp7_input_r )
{
	data16_t input_sel = *ssv_input_sel;
	if (input_sel & 0x0002)	return readinputport(5);
	if (input_sel & 0x0004)	return readinputport(6);
	if (input_sel & 0x0008)	return readinputport(7);
	if (input_sel & 0x0010)	return readinputport(8);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X: unknown input read: %04X\n",activecpu_get_pc(),input_sel);
	return 0xffff;
}

static MEMORY_READ16_START( srmp7_readmem )
	{ 0x010000, 0x050faf, MRA16_RAM				},	/* More RAM*/
	{ 0x210000, 0x210001, watchdog_reset16_r	},	/* Watchdog*/
	{ 0x300076, 0x300077, srmp7_irqv_r			},	/* Sound*/
/*	  0x540000, 0x540003, related to lev 5 irq?*/
	{ 0x600000, 0x600001, srmp7_input_r			},	/* Inputs*/
	SSV_READMEM( 0xc00000 )
MEMORY_END
static MEMORY_WRITE16_START( srmp7_writemem )
	{ 0x010000, 0x050faf, MWA16_RAM					},	/* More RAM*/
/*	{ 0x210002, 0x210003, MWA16_NOP					},	*/ /* ? 0,4 at the start*/
	{ 0x21000e, 0x21000f, ssv_lockout_inv_w			},	/* Coin Counters / Lockouts*/
/*	  0x540000, 0x540003, related to lev 5 irq?*/
	{ 0x580000, 0x580001, srmp7_sound_bank_w		},	/* Sound Bank*/
	{ 0x680000, 0x680001, MWA16_RAM, &ssv_input_sel	},	/* Inputs*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Survival Arts
***************************************************************************/

static MEMORY_READ16_START( survarts_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r	},	/* Watchdog*/
/*	{ 0x290000, 0x290001, MRA16_NOP				},	*/ /* ?*/
/*	{ 0x2a0000, 0x2a0001, MRA16_NOP				},	*/ /* ?*/
    { 0x400000, 0x43ffff, MRA16_RAM             },  /* dyna*/
	{ 0x500008, 0x500009, input_port_5_word_r	},	/* Extra Buttons*/
	SSV_READMEM( 0xf00000 )
MEMORY_END
static MEMORY_WRITE16_START( survarts_writemem )
/*	{ 0x210002, 0x210003, MWA16_NOP				},	*/ /* ? 0,4 at the start*/
    { 0x400000, 0x43ffff, MWA16_RAM             },  /* dyna*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
							Pachinko Sexy Reaction
***************************************************************************/

static data16_t serial;

static READ16_HANDLER( sxyreact_ballswitch_r )
{
	return readinputport(5);
}

static READ16_HANDLER( sxyreact_dial_r )
{
	return ((serial >> 1) & 0x80);
}

static WRITE16_HANDLER( sxyreact_dial_w )
{
	if (ACCESSING_LSB)
	{
		static int old;

		if (data & 0x20)
			serial = readinputport(6) & 0xff;

		if ( (old & 0x40) && !(data & 0x40) )	/* $40 -> $00*/
			serial <<= 1;						/* shift 1 bit*/

		old = data;
	}
}

static WRITE16_HANDLER( sxyreact_motor_w )
{
/*	usrintf_showmessage("%04X",data);	*/ /* 8 = motor on; 0 = motor off*/
}

static MEMORY_READ16_START( sxyreact_readmem )
	{ 0x210000, 0x210001, watchdog_reset16_r	},	/* Watchdog*/
	{ 0x500002, 0x500003, sxyreact_ballswitch_r	},	/* ?*/
	{ 0x500004, 0x500005, sxyreact_dial_r		},	/* Dial Value (serial)*/
	{ 0x580000, 0x58ffff, MRA16_RAM				},	/* NVRAM*/
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( sxyreact_writemem )
/*	{ 0x210002, 0x210003, MWA16_NOP				},	*/ /* ? 1 at the start*/
	{ 0x21000e, 0x21000f, ssv_lockout_inv_w		},	/* Inverted lockout lines*/
	{ 0x520000, 0x520001, sxyreact_dial_w		},	/* Dial Value (advance 1 bit)*/
	{ 0x520004, 0x520005, sxyreact_motor_w		},	/* Dial Motor?*/
	{ 0x580000, 0x58ffff, MWA16_RAM, &ssv_nvram, &ssv_nvram_size	},	/* NVRAM*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
								Twin Eagle II
***************************************************************************/

static MEMORY_READ16_START( twineag2_readmem )
	{ 0x010000, 0x03ffff, MRA16_RAM				},	/* More RAM*/
	{ 0x210000, 0x210001, watchdog_reset16_r	},	/* Watchdog (also value is cmp.b with mem 8)*/
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( twineag2_writemem )
	{ 0x010000, 0x03ffff, MWA16_RAM				},	/* More RAM*/
	SSV_WRITEMEM
MEMORY_END


/***************************************************************************
									Ultra X
***************************************************************************/

static MEMORY_READ16_START( ultrax_readmem )
	{ 0x010000, 0x03ffff, MRA16_RAM				},	/* More RAM*/
	{ 0x210000, 0x210001, watchdog_reset16_r	},	/* Watchdog (also value is cmp.b with memory address 8)*/
	SSV_READMEM( 0xe00000 )
MEMORY_END
static MEMORY_WRITE16_START( ultrax_writemem )
	{ 0x010000, 0x03ffff, MWA16_RAM			},	/* More RAM*/
/*	{ 0x210002, 0x210003, MWA16_NOP			},	*/ /* ? 2,6 at the start*/
	SSV_WRITEMEM
MEMORY_END

/***************************************************************************


								Input Ports


***************************************************************************/


/***************************************************************************
	                       Change Air Blade
***************************************************************************/

INPUT_PORTS_START( cairblad )
	PORT_START	/* IN0*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0040, "Every 2 Mil" )
	PORT_DIPSETTING(      0x0060, "2 Mil/6 Mil" )
	PORT_DIPSETTING(      0x0020, "4 Million" )
	PORT_DIPSETTING(      0x0000, "None" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN2*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Drift Out '94
***************************************************************************/

INPUT_PORTS_START( drifto94 )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, "Sound Test" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, "Unknown 2-2&3*" )
	PORT_DIPSETTING(      0x000c, "11 (0)" )
	PORT_DIPSETTING(      0x0008, "10 (1)" )
	PORT_DIPSETTING(      0x0004, "01 (0)" )
	PORT_DIPSETTING(      0x0000, "00 (2)" )
	PORT_DIPNAME( 0x0010, 0x0010, "Music Volume" )
	PORT_DIPSETTING(      0x0000, "Quiet" )
	PORT_DIPSETTING(      0x0010, "Loud" )
	PORT_DIPNAME( 0x0020, 0x0020, "Sound Volume" )
	PORT_DIPSETTING(      0x0000, "Quiet" )
	PORT_DIPSETTING(      0x0020, "Loud" )
	PORT_DIPNAME( 0x0040, 0x0040, "Save Best Time" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Eagle Shot Golf

				Place holder for corrected dip switch settings
***************************************************************************/

INPUT_PORTS_START( eaglshot )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x000f, 0x0009, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
/* "** ADDED MULTIPLE COIN FEATURE **"*/
	PORT_DIPSETTING(      0x0005, "Multiple Coin Feature A" )
/* 2c-1c, 4c-2c, 5c-3c & 6c-4c*/
	PORT_DIPSETTING(      0x0004, "Multiple Coin Feature B" )
/* 2c-1c, 4c-3c*/
	PORT_DIPSETTING(      0x0003, "Multiple Coin Feature C" )
/* 1c-1c, 2c-2c, 3c-3c, 4c-4c, 5c-6c*/
	PORT_DIPSETTING(      0x0002, "Multiple Coin Feature D" )
/* 1c-1c, 2c-2c, 3c-3c & 4c-5c*/
	PORT_DIPSETTING(      0x0001, "Multiple Coin Feature E" )
/* 1c-1c, 2c-3c*/
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Discount to Continue" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) ) /* 2 Coins to start, 1 to continue*/
	PORT_DIPNAME( 0x0020, 0x0020, "Controls" )
	PORT_DIPSETTING(      0x0020, "Trackball" )
	PORT_DIPSETTING(      0x0000, "Joystick" )
	PORT_DIPNAME( 0x0040, 0x0040, "Trackball Type" )
	PORT_DIPSETTING(      0x0040, "24 Counts (USA)" )
	PORT_DIPSETTING(      0x0000, "12 Counts (Japan)" )
	PORT_DIPNAME( 0x0080, 0x0080, "Unused/Unknown" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0003, 0x0003, "Number of Holes" )
	PORT_DIPSETTING(      0x0002, "2 Holes" )
	PORT_DIPSETTING(      0x0003, "3 Holes" )
	PORT_DIPSETTING(      0x0001, "4 Holes" )
	PORT_DIPSETTING(      0x0000, "5 Holes" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) ) /* No listed value for ON & ON*/
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	/* IN5 - trackball x ($d00000) */
    PORT_ANALOG( 0x0fff, 0x0000, IPT_TRACKBALL_X | IPF_PLAYER1, 30, 30, 0, 0x0fff)

	PORT_START	/* IN6 - trackball y ($d00000) */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_TRACKBALL_Y | IPF_PLAYER1, 30, 30, 0, 0x0fff)

INPUT_PORTS_END


/***************************************************************************
								Hyper Reaction
***************************************************************************/

INPUT_PORTS_START( hypreact )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Half Coins To Continue" )
	PORT_DIPSETTING(      0x0040, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy"    )
	PORT_DIPSETTING(      0x000c, "Normal"  )
	PORT_DIPSETTING(      0x0004, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, "Controls" )
	PORT_DIPSETTING(      0x0010, "Keyboard" )
	PORT_DIPSETTING(      0x0000, "Joystick" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN2 - $210008 (used in joystick mode)*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )

	PORT_START	/* IN3 - $21000a (used in joystick mode)*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_LOW, 0, "Ron",   KEYCODE_Z,        IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, 0, "Tsumo", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN        )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN        )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )	/* service coin & bet*/
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	/* IN5 - $c00000(0)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1                              )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 - $c00000(1)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN7 - $c00000(2)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN8 - $c00000(3)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Hyper Reaction 2
***************************************************************************/

INPUT_PORTS_START( hypreac2 )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Half Coins To Continue" )
	PORT_DIPSETTING(      0x0040, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, "Controls" )
	PORT_DIPSETTING(      0x0010, "Keyboard" )
	PORT_DIPSETTING(      0x0000, "Joystick" )
	PORT_DIPNAME( 0x0020, 0x0020, "Communication 1" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Communication 2" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	/* IN5 - $500000(0)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1                              )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 - $500000(1)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN7 - $500000(2)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN8 - $500000(3)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Jan Jan Simasyo
***************************************************************************/

INPUT_PORTS_START( janjans1 )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, "Nudity" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Mini Game" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, "Initial Score" )
	PORT_DIPSETTING(      0x0020, "1000" )
	PORT_DIPSETTING(      0x0030, "1500" )
	PORT_DIPSETTING(      0x0010, "2000" )
	PORT_DIPSETTING(      0x0000, "3000" )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Communication" )
/*	PORT_DIPSETTING(      0x0080, "unused" )*/
	PORT_DIPSETTING(      0x00c0, "None" )
	PORT_DIPSETTING(      0x0040, "Board 1 (Main)" )
	PORT_DIPSETTING(      0x0000, "Board 2 (Sub)" )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	/* IN5 - $800002(0)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 - $800002(1)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN7 - $800002(2)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN8 - $800002(3)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Keith & Lucy
***************************************************************************/

INPUT_PORTS_START( keithlcy )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3*" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy"	)	/* 15 sec*/
	PORT_DIPSETTING(      0x0003, "Normal"	)	/* 12*/
	PORT_DIPSETTING(      0x0001, "Hard"	)	/* 10*/
	PORT_DIPSETTING(      0x0000, "Hardest"	)	/* 8*/
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0004, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0030, "100k" )	/*100*/
	PORT_DIPSETTING(      0x0020, "150k" )	/*150*/
	PORT_DIPSETTING(      0x0010, "200k" )	/*100*/
	PORT_DIPSETTING(      0x0000, "200k?" )	/*200*/
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
							Koi Koi Simasyo 2
***************************************************************************/

INPUT_PORTS_START( koikois2 )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Controls" )
	PORT_DIPSETTING(      0x0080, "Joystick" )
	PORT_DIPSETTING(      0x0000, "Keyboard" )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, "Nudity" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Communication" )
/*	PORT_DIPSETTING(      0x0080, "unused" )*/
	PORT_DIPSETTING(      0x00c0, "None" )
	PORT_DIPSETTING(      0x0040, "Board 1 (Main)" )
	PORT_DIPSETTING(      0x0000, "Board 2 (Sub)" )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	/* IN5 - $800002(0)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 - $800002(1)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN7 - $800002(2)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN8 - $800002(3)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Meosis Magic
***************************************************************************/

INPUT_PORTS_START( meosism )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0003, "1 Medal/1 Credit" )
	PORT_DIPSETTING(      0x0001, "1 Medal/5 Credits" )
	PORT_DIPSETTING(      0x0002, "1 Medal/10 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Medal/20 Credits" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Attendant Pay" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Medals Payout" )
	PORT_DIPSETTING(      0x0010, "400" )
	PORT_DIPSETTING(      0x0000, "800" )
	PORT_DIPNAME( 0x0020, 0x0020, "Max Credits" )
	PORT_DIPSETTING(      0x0020, "5000" )
	PORT_DIPSETTING(      0x0000, "9999" )
	PORT_DIPNAME( 0x0040, 0x0040, "Hopper" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Reel Speed" )
	PORT_DIPSETTING(      0x0080, "Low" )
	PORT_DIPSETTING(      0x0000, "High" )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0003, 0x0003, "Game Rate" )
	PORT_DIPSETTING(      0x0000, "80%" )
	PORT_DIPSETTING(      0x0002, "85%" )
	PORT_DIPSETTING(      0x0003, "90%" )
	PORT_DIPSETTING(      0x0001, "95%" )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Controls" )
/*	PORT_DIPSETTING(      0x0020, "Simple") )*/
	PORT_DIPSETTING(      0x0000, "Complex" )
	PORT_DIPNAME( 0x0040, 0x0000, "Coin Sensor" )
	PORT_DIPSETTING(      0x0040, "Active High" )
	PORT_DIPSETTING(      0x0000, "Active Low" )
	PORT_DIPNAME( 0x0080, 0x0080, "Hopper Sensor" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON4        )	/*bet*/
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        )	/*stop/r*/
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        )	/*stop/c*/
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        )	/*stop/l*/
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	/*no*/
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )	/*yes*/
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_START1         )	/*start*/
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN        )	/*-*/

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )	/*-*/
	PORT_BITX( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	/*test*/
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )	/*-*/
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_SERVICE3 )	/*payout*/
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )	/*-*/
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_TILT     )	/*reset*/
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )	/*-*/
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )	/*-*/

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )	/*service coin*/
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )	/*analyzer*/
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON5  )	/*max bet*/
	PORT_BIT(  0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Monster Slider
***************************************************************************/

INPUT_PORTS_START( mslider )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0030, 0x0030, "Rounds (Vs Mode)" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0030, "2" )
	PORT_DIPSETTING(      0x0020, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
					Gourmet Battle Quiz Ryohrioh CooKing
***************************************************************************/

INPUT_PORTS_START( ryorioh )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7*" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0003, 0x0003, "Unknown 2-0&1*" )
	PORT_DIPSETTING(      0x0002, "0" )
	PORT_DIPSETTING(      0x0003, "1" )
	PORT_DIPSETTING(      0x0001, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
							Super Real Mahjong PIV
***************************************************************************/

INPUT_PORTS_START( srmp4 )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0006, "Easiest" )
	PORT_DIPSETTING(      0x0005, "Easier" )
	PORT_DIPSETTING(      0x0004, "Easy" )
	PORT_DIPSETTING(      0x0007, "Normal" )
	PORT_DIPSETTING(      0x0003, "Medium" )
	PORT_DIPSETTING(      0x0002, "Hard" )
	PORT_DIPSETTING(      0x0001, "Harder" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	/* IN5 - $c0000a(0)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 - $c0000a(1)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN7 - $c0000a(2)*/
	PORT_BITX(0x0001, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN8 - $c0000a(3)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Super Real Mahjong P7
***************************************************************************/

INPUT_PORTS_START( srmp7 )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Re-cloth" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Nudity" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0006, "Easiest" )
	PORT_DIPSETTING(      0x0005, "Easier" )
	PORT_DIPSETTING(      0x0004, "Easy" )
	PORT_DIPSETTING(      0x0007, "Normal" )
	PORT_DIPSETTING(      0x0003, "Medium" )
	PORT_DIPSETTING(      0x0002, "Hard" )
	PORT_DIPSETTING(      0x0001, "Harder" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0008, 0x0008, "Kuitan" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN  )	/* tested*/
	PORT_BIT(  0x00e0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	/* IN6 - $600000(0)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN7 - $600000(1)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN8 - $600000(2)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN5 - $600000(3)*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Storm Blade
***************************************************************************/

INPUT_PORTS_START( stmblade )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Rapid Fire" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0040, "600000" )
	PORT_DIPSETTING(      0x0000, "800000" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END


/***************************************************************************
								Survival Arts
***************************************************************************/

INPUT_PORTS_START( survarts )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) ) /* Verified Defualt is 2 coins 1 Credit*/
	PORT_DIPSETTING(      0x0007, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) ) */ /* 2 Credits Start, 1 to continue*/
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
/* "** ADDED MULTIPLE COIN FEATURE **"*/
	PORT_DIPSETTING(      0x0005, "Multiple Coin Feature A" )
/* 2c-1c, 4c-2c, 5c-3c & 6c-4c*/
	PORT_DIPSETTING(      0x0004, "Multiple Coin Feature B" )
/* 2c-1c, 4c-3c*/
	PORT_DIPSETTING(      0x0003, "Multiple Coin Feature C" )
/* 1c-1c, 2c-2c, 3c-3c, 4c-4c, 5c-6c*/
	PORT_DIPSETTING(      0x0002, "Multiple Coin Feature D" )
/* 1c-1c, 2c-2c, 3c-3c & 4c-5c*/
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) ) /* Verified Defualt is 2 coins 1 Credit*/
	PORT_DIPSETTING(      0x0070, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) ) */ /* 2 Credits Start, 1 to continue*/
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
/* "** ADDED MULTIPLE COIN FEATURE **"*/
	PORT_DIPSETTING(      0x0050, "Multiple Coin Feature A" )
/* 2c-1c, 4c-2c, 5c-3c & 6c-4c*/
	PORT_DIPSETTING(      0x0040, "Multiple Coin Feature B" )
/* 2c-1c, 4c-3c*/
	PORT_DIPSETTING(      0x0030, "Multiple Coin Feature C" )
/* 1c-1c, 2c-2c, 3c-3c, 4c-4c, 5c-6c*/
	PORT_DIPSETTING(      0x0020, "Multiple Coin Feature D" )
/* 1c-1c, 2c-2c, 3c-3c & 4c-5c*/

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Timer Speed" )
	PORT_DIPSETTING(      0x0004, "Normal" )
	PORT_DIPSETTING(      0x0000, "Fast" )
	PORT_DIPNAME( 0x0008, 0x0008, "Damage Level" )
	PORT_DIPSETTING(      0x0008, "Normal" )
	PORT_DIPSETTING(      0x0000, "High" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0020, "Easy" )
	PORT_DIPSETTING(      0x0030, "Normal" )
	PORT_DIPSETTING(      0x0010, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x00c0, 0x0000, "Fatal Damage" )
	PORT_DIPSETTING(      0x00c0, "Light" )
	PORT_DIPSETTING(      0x0040, "Normal" )
	PORT_DIPSETTING(      0x0080, "Heavy" )
	PORT_DIPSETTING(      0x0000, "Heaviest" )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START	/* IN5 - $500008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************
  Dyna Gears
***************************************************************************/

INPUT_PORTS_START( dynagear )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
INPUT_PORTS_END

/***************************************************************************
							Pachinko Sexy Reaction
***************************************************************************/

INPUT_PORTS_START( sxyreact )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_BIT(     0x0038, IP_ACTIVE_LOW, IPT_UNUSED )
/*	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )*/
/*	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )*/
/*	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )*/
/*	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )*/
/*	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_3C ) )*/
/*	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )*/
/*	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )*/
/*	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )*/
	PORT_DIPNAME( 0x0040, 0x0040, "Credits To Play" )
	PORT_DIPSETTING(      0x0040, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0080, 0x0080, "Buy Balls With Credits" )	/* press start*/
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, "Difficulty?" )
	PORT_DIPSETTING(      0x0008, "Easy?" )
	PORT_DIPSETTING(      0x000c, "Normal?" )
	PORT_DIPSETTING(      0x0004, "Hard?" )
	PORT_DIPSETTING(      0x0000, "Hardest?" )
	PORT_DIPNAME( 0x0010, 0x0010, "Controls" )
	PORT_DIPSETTING(      0x0010, "Dial" )
	PORT_DIPSETTING(      0x0000, "Joystick" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )	/* -> ball sensor on*/
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* (player 2, only shown in test mode)*/

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNUSED   )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, "Test Advance", KEYCODE_F1, IP_JOY_DEFAULT )
	PORT_BIT(  0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN5 - $500002*/
	PORT_BIT(  0x0001, IP_ACTIVE_HIGH,  IPT_SERVICE2 )	/* ball switch on -> handle motor off*/

	PORT_START	/* IN6 - $500004*/
	PORT_ANALOGX( 0xff, 0x00, IPT_PADDLE, 15, 15, 0, 0xcf, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END


/***************************************************************************
								Twin Eagle II
***************************************************************************/

INPUT_PORTS_START( twineag2 )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) ) /* No values listed for all "ON"*/
	PORT_DIPSETTING(      0x0007, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
/* "** ADDED MULTIPLE COIN FEATURE **"*/
	PORT_DIPSETTING(      0x0005, "Multiple Coin Feature A" )
/* 2c-1c, 4c-2c, 5c-3c & 6c-4c*/
	PORT_DIPSETTING(      0x0004, "Multiple Coin Feature B" )
/* 2c-1c, 4c-3c*/
	PORT_DIPSETTING(      0x0003, "Multiple Coin Feature C" )
/* 1c-1c, 2c-2c, 3c-3c, 4c-4c, 5c-6c*/
	PORT_DIPSETTING(      0x0002, "Multiple Coin Feature D" )
/* 1c-1c, 2c-2c, 3c-3c & 4c-5c*/
	PORT_DIPSETTING(      0x0001, "Multiple Coin Feature E" )
/* 1c-1c, 2c-3c*/
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) ) /* No values listed for all "ON"*/
	PORT_DIPSETTING(      0x0070, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
/* "** ADDED MULTIPLE COIN FEATURE **"*/
	PORT_DIPSETTING(      0x0050, "Multiple Coin Feature A" )
/* 2c-1c, 4c-2c, 5c-3c & 6c-4c*/
	PORT_DIPSETTING(      0x0040, "Multiple Coin Feature B" )
/* 2c-1c, 4c-3c*/
	PORT_DIPSETTING(      0x0030, "Multiple Coin Feature C" )
/* 1c-1c, 2c-2c, 3c-3c, 4c-4c, 5c-6c*/
	PORT_DIPSETTING(      0x0020, "Multiple Coin Feature D" )
/* 1c-1c, 2c-2c, 3c-3c & 4c-5c*/
	PORT_DIPSETTING(      0x0010, "Multiple Coin Feature E" )
/* 1c-1c, 2c-3c*/

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0006, "Easiest" )
	PORT_DIPSETTING(      0x0005, "Easier" )
	PORT_DIPSETTING(      0x0004, "Easy" )
	PORT_DIPSETTING(      0x0007, "Normal" )
	PORT_DIPSETTING(      0x0003, "Medium" )
	PORT_DIPSETTING(      0x0002, "Hard" )
	PORT_DIPSETTING(      0x0001, "Harder" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0010, "3" )
	PORT_DIPNAME( 0x0020, 0x0020, "Freeze" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1 )

	PORT_BIT(  0x0008, IP_ACTIVE_LOW,  IPT_SERVICE2 )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, "Test Advance", KEYCODE_F1, IP_JOY_DEFAULT )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_SERVICE4 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_START3 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_START4 )
INPUT_PORTS_END


/***************************************************************************
								Ultra X
***************************************************************************/

INPUT_PORTS_START( ultrax )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )

	PORT_START	/* IN1 - $210004*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy"	)	/*$140*/
	PORT_DIPSETTING(      0x0003, "Normal"	)	/*$190*/
	PORT_DIPSETTING(      0x0001, "Hard"	)	/*$200*/
	PORT_DIPSETTING(      0x0000, "Hardest"	)	/*$300*/
	PORT_DIPNAME( 0x0014, 0x0004, "Country" )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPSETTING(      0x0014, "Japan" )
/*	PORT_DIPSETTING(      0x0010, "Japan" )*/
	PORT_DIPSETTING(      0x0004, "World" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	/* country            0x0010*/
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, "Test Advance", KEYCODE_F1, IP_JOY_DEFAULT )
	PORT_BIT(  0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
		                       Vasara
***************************************************************************/

INPUT_PORTS_START( vasara )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Free_Play )  )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_START	/* IN1*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, "Bomber Stock" )
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0004, "1" )
	PORT_DIPSETTING(      0x000c, "2" )
	PORT_DIPSETTING(      0x0008, "3" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ))
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, "Game Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "English Subtitles" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END

/***************************************************************************
		                       Vasara 2
***************************************************************************/

INPUT_PORTS_START( vasara2 )
	PORT_START	/* IN0 - $210002*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Free_Play )  )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

	PORT_START	/* IN1*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0008, "5" )
	PORT_DIPNAME( 0x0010, 0x0010, "Game Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Vasara Stock" )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0040, 0x0040, "English Subtitles" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN2 - $210008*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN  )
INPUT_PORTS_END

INPUT_PORTS_START( gdfs )
	PORT_START	/* IN0 - $210002 */
	PORT_DIPNAME( 0x0001, 0x0000, "Controls" )
	PORT_DIPSETTING(      0x0001, "Joystick" )
	PORT_DIPSETTING(      0x0000, "Light_Gun" )
	PORT_DIPNAME( 0x0002, 0x0002, "Light Gun Calibration" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Stage Select" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Coinage ) )
/*  PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) )  2 Coins to Start, 1 Coin to Continue??? */
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Save Scores" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )	/* Clear NVRAM on boot */
	PORT_DIPSETTING(      0x0020, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $210004 */
	PORT_DIPNAME( 0x0001, 0x0001, "Invert X Axis" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 2-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Language" )
	PORT_DIPSETTING(      0x0000, "English" )
	PORT_DIPSETTING(      0x0008, "Japanese" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Damage From Machine Gun" )	/* F76E34 */
	PORT_DIPSETTING(      0x0020, "Light" )
	PORT_DIPSETTING(      0x0000, "Heavy" )
	PORT_DIPNAME( 0x0040, 0x0040, "Damage From Beam Cannon" )	/* F77487 */
	PORT_DIPSETTING(      0x0040, "Light" )
	PORT_DIPSETTING(      0x0000, "Heavy" )
	PORT_DIPNAME( 0x0080, 0x0080, "Damage From Missle" )	/* F77255 */
	PORT_DIPSETTING(      0x0080, "Light" )
	PORT_DIPSETTING(      0x0000, "Heavy" )
	
	PORT_START	/* IN2 - $210008 */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START1  )					/* press at boot for service mode */
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* used in test mode */
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN3 - $21000a */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN4 - $21000c */
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT_IMPULSE( 0x0002, IP_ACTIVE_LOW, IPT_COIN2, 10 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN5 - $540000(0) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 35, 10, 0, 0xff )

	PORT_START	/* IN6 - $540000(1) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 35, 10, 0, 0xff )

	PORT_START	/* IN7 - $540000(2) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 35, 10, 0, 0xff )

	PORT_START	/* IN8 - $540000(3) */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 35, 10, 0, 0xff )
INPUT_PORTS_END

/***************************************************************************


							Graphics Layouts


***************************************************************************/

/*	16 x 8 tiles. Depth is 8 bits, but can be decreased to 6 (and maybe
	less) at runtime.	*/

static struct GfxLayout layout_16x8x8 =
{
	16,8,
	RGN_FRAC(1,4),
	8,
	{	RGN_FRAC(3,4)+8, RGN_FRAC(3,4)+0,
		RGN_FRAC(2,4)+8, RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+8, RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8, RGN_FRAC(0,4)+0	},
	{	STEP8(0,1), STEP8(16,1)	},
	{	STEP8(0,16*2)	},
	16*8*2
};

static struct GfxLayout layout_16x8x6 =
{
	16,8,
	RGN_FRAC(1,4),
	6,
	{
		RGN_FRAC(2,4)+8, RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+8, RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8, RGN_FRAC(0,4)+0	},
	{	STEP8(0,1), STEP8(16,1)	},
	{	STEP8(0,16*2)	},
	16*8*2
};

static struct GfxDecodeInfo ssv_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x8x8, 0, 0x8000/64 }, /* [0] Sprites (256 colors)*/
	{ REGION_GFX1, 0, &layout_16x8x6, 0, 0x8000/64 }, /* [1] Sprites (64 colors)*/
	{ -1 }
};

static struct GfxLayout layout_16x8x8_2 =
{
	16,8,
	RGN_FRAC(1,1),
	8,
	{	STEP8(0,1)		},
	{	STEP16(0,8)		},
	{	STEP8(0,16*8)	},
	16*8*8
};

static struct GfxLayout layout_16x8x6_2 =
{
	16,8,
	RGN_FRAC(1,1),
	6,
	{	2,3,4,5,6,7		},
	{	STEP16(0,8)		},
	{	STEP8(0,16*8)	},
	16*8*8
};

static struct GfxDecodeInfo eaglshot_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x8x8_2, 0, 0x8000/64 }, /* [0] Sprites (256 colors)*/
	{ REGION_GFX1, 0, &layout_16x8x6_2, 0, 0x8000/64 }, /* [1] Sprites (64 colors)*/
	{ -1 }
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{	STEP8(0,1)		},
	{	STEP16(0,8)		},
	{	STEP16(0,16*8)	},
	16*16*8
};

static struct GfxDecodeInfo gdfs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x8x8,   0, 0x8000/64  }, /* [0] Sprites (256 colors) */
	{ REGION_GFX1, 0, &layout_16x8x6,   0, 0x8000/64  }, /* [1] Sprites (64 colors) */
	{ REGION_GFX2, 0, &layout_16x8x8_2, 0, 0x8000/64  }, /* [2] Zooming Sprites (256 colors, decoded from ram) */
	{ REGION_GFX3, 0, &layout_16x16x8,  0, 0x8000/256 }, /* [3] Tilemap */
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static struct ES5506interface es5506_interface =
{
	1,
	{ 16000000 },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ REGION_SOUND3 },
	{ REGION_SOUND4 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
	{ 0 }
};


/***************************************************************************

	Some games (e.g. hypreac2) oddly map the high bits of the tile code
	to the gfx roms: arranging the roms accordingly would waste tens of
	megabytes. So we use a look-up table.

	We also need to set up game	specific offsets for sprites and layers
	(at least until the CRT	controlled will be emulated).

***************************************************************************/

void init_ssv(void)
{
	int i;
	for (i = 0; i < 16; i++)
		ssv_tile_code[i]	=	( (i & 8) ? (1 << 16) : 0 ) +
								( (i & 4) ? (2 << 16) : 0 ) +
								( (i & 2) ? (4 << 16) : 0 ) +
								( (i & 1) ? (8 << 16) : 0 ) ;
	ssv_enable_video(1);
	ssv_special = 0;
	interrupt_ultrax=0;
	eaglshot_dirty = 0;
}

void hypreac2_init(void)
{
	int i;

	init_ssv();

	for (i = 0; i < 16; i++)
		ssv_tile_code[i]	=	(i << 16);
}


DRIVER_INIT( drifto94 )		{	init_ssv();
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( eaglshot )		{	hypreac2_init();
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = 0;	ssv_tilemap_offsy = -0xef; }								
DRIVER_INIT( gdfs )			{	init_ssv();
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = 1;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = 0;	}								
DRIVER_INIT( hypreact )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf7;	}
DRIVER_INIT( hypreac2 )		{	hypreac2_init();	/* different*/
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf8;	}
DRIVER_INIT( janjans1 )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( keithlcy )		{	init_ssv();
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = +0xf1;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( meosism )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xef;	}
DRIVER_INIT( mslider )		{	init_ssv();
								ssv_sprites_offsx =-16;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +8;	ssv_tilemap_offsy = -0xf1;	}
DRIVER_INIT( ryorioh )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( srmp4 )		{	init_ssv();
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;
/*	((data16_t *)memory_region(REGION_USER1))[0x2b38/2] = 0x037a;	 // patch to see gal test mode /*/
							}
DRIVER_INIT( srmp7 )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = -0xf;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( stmblade )		{	init_ssv();
								ssv_sprites_offsx = -8; ssv_sprites_offsy = +0xef;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf0;	}
DRIVER_INIT( survarts )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xef;	}
DRIVER_INIT( dynagear )		{	init_ssv(); ssv_special = 3;
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = +0xec;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xec;	}
DRIVER_INIT( sxyreact )		{	hypreac2_init();	/* different*/
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xe8;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xef;	}
DRIVER_INIT( twineag2 )		{	init_ssv();interrupt_ultrax=1;
								ssv_sprites_offsx = -6;	ssv_sprites_offsy = -7;
								ssv_tilemap_offsx = -10;ssv_tilemap_offsy = -8;	}
DRIVER_INIT( ultrax )		{	init_ssv();interrupt_ultrax=1;
								ssv_sprites_offsx = -8;	ssv_sprites_offsy = 0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = 0;	}
DRIVER_INIT( vasara )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf8;	}
DRIVER_INIT( vasara2 )		{	init_ssv();
								ssv_sprites_offsx = +0;	ssv_sprites_offsy = +0xf0;
								ssv_tilemap_offsx = +0;	ssv_tilemap_offsy = -0xf8;	}

static MACHINE_DRIVER_START( ssv )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", V60, 16000000) /* Based on STA-0001 & STA-0001B System boards */
	MDRV_CPU_VBLANK_INT(ssv_interrupt,2)	/* Vblank */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	/* we use cpu_getvblank */

	MDRV_MACHINE_INIT(ssv)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)

	MDRV_SCREEN_SIZE(0x180, 0x100)
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
	MDRV_GFXDECODE(ssv_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_VIDEO_START(ssv)
	MDRV_VIDEO_UPDATE(ssv)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(ES5506, es5506_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( drifto94 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(drifto94_readmem, drifto94_writemem)

	MDRV_NVRAM_HANDLER(ssv)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 4, 0xf0-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( eaglshot )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(eaglshot_readmem, eaglshot_writemem)

	MDRV_NVRAM_HANDLER(ssv)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x140-1, 4, 0xe8-1)

	MDRV_GFXDECODE(eaglshot_gfxdecodeinfo)
	MDRV_VIDEO_START(eaglshot)
	MDRV_VIDEO_UPDATE(eaglshot)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( gdfs )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(gdfs_readmem, gdfs_writemem)
	MDRV_CPU_VBLANK_INT(gdfs_interrupt,1+4)

	MDRV_NVRAM_HANDLER(gdfs)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)

	MDRV_GFXDECODE(gdfs_gfxdecodeinfo)
	MDRV_VIDEO_START(gdfs)
	MDRV_VIDEO_UPDATE(gdfs)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hypreact )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(hypreact_readmem, hypreact_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(8, 0x148-1, 16, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hypreac2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(hypreac2_readmem, hypreac2_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 8, 0xf8-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( janjans1 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(janjans1_readmem, janjans1_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( keithlcy )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(keithlcy_readmem, keithlcy_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 4, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( meosism )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(meosism_readmem, meosism_writemem)

	MDRV_NVRAM_HANDLER(ssv)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mslider )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mslider_readmem, mslider_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ryorioh )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(ryorioh_readmem, ryorioh_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( srmp4 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(srmp4_readmem, srmp4_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 4, 0xf4-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( srmp7 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(srmp7_readmem, srmp7_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( stmblade )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(drifto94_readmem, drifto94_writemem)

	MDRV_NVRAM_HANDLER(ssv)
	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x158-1, 1, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( survarts )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(survarts_readmem, survarts_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 4, 0xf4-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dynagear )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(survarts)
	/* video hardware */
	MDRV_VISIBLE_AREA(8, 0x158-16-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sxyreact )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(sxyreact_readmem, sxyreact_writemem)

	MDRV_NVRAM_HANDLER(ssv)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( twineag2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(twineag2_readmem, twineag2_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ultrax )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ssv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(ultrax_readmem, ultrax_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 0x150-1, 0, 0xf0-1)
MACHINE_DRIVER_END



/***************************************************************************


								ROMs Loading


***************************************************************************/


/***************************************************************************

						Change Air Blade (Japan)

Change Air Blade
Sammy, 1999

ROM board for use with System SSV Main Board
PCB No: P1-112C

Fairly sparsely populated board containing not much except....

RAM   : 6262 (x1)
OTHER : 3.6V Ni-Cd Battery
PALs  : (x1, labelled AC412G00)

ROMs  : (Filename  = ROM Label)
        (Extension = PCB Location)
------------------------------
AC1801M01.U6    32M Mask
AC1802M01.U9    32M Mask

AC1805M01.U8    32M Mask
AC1806M01.U11   32M Mask

AC1803M01.U7    32M Mask
AC1804M01.U10   32M Mask

AC1807M01.U41   32M Mask
AC1810E01.U32   27C160


***************************************************************************/

ROM_START( cairblad )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "ac1810e0.u32",  0x000000, 0x200000, CRC(13a0b4c2) SHA1(3498303e9b186ab329ee761cee9d4cb8ed552455) ) /* AC1810E01.U32    27C160*/

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ac1801m0.u6",  0x0000000, 0x400000, CRC(1b2b6943) SHA1(95c5dc0ed1d533b2285452c8546346d96a90d097) ) /* AC1801M01.U6    32M Mask*/
	ROM_LOAD( "ac1802m0.u9",  0x0400000, 0x400000, CRC(e053b087) SHA1(9569e79c6363e8f97c27aacaa29d25cf32c4b4c1) ) /* AC1802M01.U9    32M Mask*/

	ROM_LOAD( "ac1803m0.u7",  0x0800000, 0x400000, CRC(45484866) SHA1(5e2f06743906be298202eafc233b76762d60d8aa) ) /* AC1803M01.U7    32M Mask*/
	ROM_LOAD( "ac1804m0.u10", 0x0c00000, 0x400000, CRC(5e0b2285) SHA1(b3b8f249c1b1b2e9438ebc3a669f3ebfb5aa5feb) ) /* AC1804M01.U10   32M Mask*/

	ROM_LOAD( "ac1805m0.u8",  0x1000000, 0x400000, CRC(19771f43) SHA1(d6a05392c58d3f60d666e08b3a82f06fa2c8e3a3) ) /* AC1805M01.U8    32M Mask*/
	ROM_LOAD( "ac1806m0.u11", 0x1400000, 0x400000, CRC(816b97dc) SHA1(3737cb37a4db720901661fa9b4e30c44181efb94) ) /* AC1806M01.U11   32M Mask*/

	ROM_FILL(                 0x1800000, 0x800000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "ac1410m0.u41", 0x000000, 0x400000, CRC(ecf1f255) SHA1(984b1529b8f0c7d94ea713c85d71df00f54eba79) ) /* AC1807M01.U41   32M Mask*/
ROM_END


/***************************************************************************

						Drift Out '94 - The hard order

----------------------
System SSV (STA-0001B)
----------------------
CPU  : NEC D70615GD-16-S (V60)
Sound: Ensoniq ES5506 (OTTOR2)
OSC  : 42.9545MHz(X2) 48.0000MHz(X3)

Custom chips:
ST-0004 (Video DAC?)
ST-0005 (Parallel I/O?)
ST-0006 (Video controller)
ST-0007 (System controller)

Program Work RAM  : 256Kbitx2 (expandable to 1Mx2)
Object Work RAM   : 1Mbitx2
Color Palette RAM : 256Kbitx3 (expandable to 1Mx3)

-------------------------
SSV Subboard (VISCO-001B)
-------------------------
ROMs:
visco-33.bin - Main programs (27c4000)
visco-37.bin /

vg003-19.u26 - Data? (mask, read as 27c160)

vg003-17.u22 - Samples (mask, read as 27c160)
vg003-18.u15 /

vg003-01.a0 - Graphics (mask, read as 27c160)
vg003-05.a1 |
vg003-09.a2 |
vg009-13.a3 |
vg009-02.b0 |
vg003-06.b1 |
vg003-10.b2 |
vg003-14.b3 |
vg003-03.c0 |
vg003-07.c1 |
vg003-11.c2 |
vg003-15.c3 |
vg003-04.d0 |
vg003-08.d1 |
vg003-12.d2 |
vg003-16.d3 /

GAL:
vg003-22.u29 (16V8)

Custom chip:
ST010 (maybe D78C10?)

Others:
Lithium battery + MB3790 + LH5168D-10L

***************************************************************************/

ROM_START( drifto94 )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "vg003-19.u26", 0x000000, 0x200000, CRC(238e5e2b) SHA1(fe58f571857804263642d7d089df962327a007b6) )	/* "SoundDriverV1.1a"*/
	ROM_LOAD16_BYTE( "visco-37.bin", 0x200000, 0x080000, CRC(78fa3ccb) SHA1(0c79ff1aa31e7ca1eeb14fbef7774278fa83ba44) )
	ROM_RELOAD(                      0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "visco-33.bin", 0x200001, 0x080000, CRC(88351146) SHA1(1decce44b5d244b57676177f417e4937d7088124) )
	ROM_RELOAD(                      0x300001, 0x080000             )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "vg003-01.a0", 0x0000000, 0x200000, CRC(2812aa1a) SHA1(5046fe51a4ea50051a19cfeeb091c87f0f217fb8) )
	ROM_LOAD( "vg003-05.a1", 0x0200000, 0x200000, CRC(1a1dd910) SHA1(f2252e4cd1b6269036ed02cec9d5a224736c1bce) )
	ROM_LOAD( "vg003-09.a2", 0x0400000, 0x200000, CRC(198f1c06) SHA1(7df5d51aa62f0b609cd1d296a3cfeeb38fbcd9d0) )
	ROM_LOAD( "vg003-13.a3", 0x0600000, 0x200000, CRC(b45b2267) SHA1(66828efcca2050bc1cdca6bbf2e8cf015ff937a8) )

	ROM_LOAD( "vg003-02.b0", 0x0800000, 0x200000, CRC(d7402027) SHA1(32af6d611ea277a860ee10e98f4eee5c4458ef7a) )
	ROM_LOAD( "vg003-06.b1", 0x0a00000, 0x200000, CRC(518c509f) SHA1(e4bcbe1d8644490a58670add40d2908c7acdf989) )
	ROM_LOAD( "vg003-10.b2", 0x0c00000, 0x200000, CRC(c1ee9d8b) SHA1(7425cf92225cd1c8d764aa47db6219a6d8b090a5) )
	ROM_LOAD( "vg003-14.b3", 0x0e00000, 0x200000, CRC(645b672b) SHA1(26dfde289679cd780bc65f4f6783a6a0f8b87818) )

	ROM_LOAD( "vg003-03.c0", 0x1000000, 0x200000, CRC(1ca7163d) SHA1(d8c5fd0054a1bc1fbad5866216f6d83c42436ecd) )
	ROM_LOAD( "vg003-07.c1", 0x1200000, 0x200000, CRC(2ff113bb) SHA1(a482ddd3c86633a79d18a03193d70fc8f0f157d8) )
	ROM_LOAD( "vg003-11.c2", 0x1400000, 0x200000, CRC(f924b105) SHA1(079ad0fc3b34c31a67dd88a442088237a2c03fdd) )
	ROM_LOAD( "vg003-15.c3", 0x1600000, 0x200000, CRC(83623b01) SHA1(026654303fb58958bc7f7be86aeb4fdd08e8be7b) )

	ROM_LOAD( "vg003-04.d0", 0x1800000, 0x200000, CRC(6be9bc62) SHA1(c0b49a558786b50f04c1cd87a11e111ad31b85d9) )
	ROM_LOAD( "vg003-08.d1", 0x1a00000, 0x200000, CRC(a7113cdb) SHA1(74d8bfee7e816d53e60d4e54a2584643562a1ee5) )
	ROM_LOAD( "vg003-12.d2", 0x1c00000, 0x200000, CRC(ac0fd855) SHA1(992ae0d02bcefaa2fad7462b211a49fbd1338b62) )
	ROM_LOAD( "vg003-16.d3", 0x1e00000, 0x200000, CRC(1a5fd312) SHA1(1e67ffa51408de107be75c9c63df6fd1bb6ce6b1) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "vg003-17.u22", 0x000000, 0x200000, CRC(6f9294ce) SHA1(b097defd95eb1d8f00e107d7669f9d33148e75c1) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "vg003-18.u15", 0x000000, 0x200000, CRC(511b3e93) SHA1(09eda175c8f1b21c18645519cc6e89c6ca1fc5de) )
ROM_END


/***************************************************************************

						Eagle Shot Golf
Eagle Shot Golf
Sammy, 1994

Lower PCB
PCB Number: GOLF ROM PCB
RAM       : HM514400 (x8)
PALs      : GAL16V8 (x2) labelled SI3-11 & SI3-12
OTHER     : NEC D4701AC
            Controls probably trackball, has 6 pin connector hooked up to a
            mc14584b Logic IC. Joystick appears to be used also for selecting
            stance, club and direction.

ROMs      : U18 & U20 are used for main program.
            All rest are 16M Mask
            U23 & U24 are sound related, all others for GFX.

Loc  ROMs           Use & eprom type
-----------------------------------------
U18  si003-09.prl - V60 Program (27C4001)
U20  si003-10.prh /

U23  si003-07.s0 - Samples (16M-Mask)
U24  si003-08.s1 /

U13  si003-01.d0 - Graphics (16M-Mask)
U12  si003-02.d1 |
U11  si003-03.d2 |
U10  si003-04.d3 |
U30  si003-05.d4 |
U31  si003-06.d5 /

NOTE: The "s" and "d" designations above are silk-screened on the rom PCB.

Chips of note:  mc14584b - Motorola HEX Schmitt Trigger

This chip is used for the trackball trigger / reading / converting values

***************************************************************************/

ROM_START( eaglshot )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "si003-09.u18",  0x000000, 0x080000, CRC(219c71ce) SHA1(4f8996b4c5b267a90073d67857358147732f8c0d) )
	ROM_LOAD16_BYTE( "si003-10.u20",  0x000001, 0x080000, CRC(c8872e48) SHA1(c8e1e712d5fa380f8fc1447502f21d2ae592811a) )

	ROM_REGION( 0x0c00000, REGION_GFX1, /*ROMREGION_DISPOSE*/0 )	/* Sprites - Read by the CPU */
	ROM_LOAD( "si003-01.u13", 0x0000000, 0x200000, CRC(d7df0d52) SHA1(d7b79a186f4272334c2297666c52f32c05787c29) )
	ROM_LOAD( "si003-02.u12", 0x0200000, 0x200000, CRC(92b4d50d) SHA1(9dc2f2961b088824d8370ac83dff796345fe4158) )
	ROM_LOAD( "si003-03.u11", 0x0400000, 0x200000, CRC(6ede4012) SHA1(6663990c6ee8e500cb8c51ad2102761ee0b3351d) )
	ROM_LOAD( "si003-04.u10", 0x0600000, 0x200000, CRC(4c65d1a1) SHA1(165f16d08813d2c989ddce4bb23b3a3652003bd5) )
	ROM_LOAD( "si003-05.u30", 0x0800000, 0x200000, CRC(daf52d56) SHA1(108419ef7d3716a3890b0d8bcbfddc1585daaae8) )
	ROM_LOAD( "si003-06.u31", 0x0a00000, 0x200000, CRC(449f9ae5) SHA1(b3e664eb88d14d1e25a0cfc8dcccc8270ca778c9) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si003-07.u23", 0x000000, 0x200000, CRC(81679fd6) SHA1(ca3b07a87781278b5c7c85951728bbe5dfcbe042) )
	ROM_LOAD16_WORD_SWAP( "si003-08.u24", 0x200000, 0x200000, CRC(d0122ba2) SHA1(96230fb690cf144cd873f7d51c0304736a698316) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )
ROM_END

ROM_START( eaglshta )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "si003-09.prl",  0x000000, 0x080000, CRC(36989004) SHA1(115a8dd4d7c4b4e042d51f886a93613b1405603b) )
	ROM_LOAD16_BYTE( "si003-10.prh",  0x000001, 0x080000, CRC(2060c304) SHA1(2ecd178ea6459b8aaac1fa499e7c91809cd22649) )

	ROM_REGION( 0x0c00000, REGION_GFX1, /*ROMREGION_DISPOSE*/0 )	/* Sprites - Read by the CPU */
	ROM_LOAD( "si003-01.u13", 0x0000000, 0x200000, CRC(d7df0d52) SHA1(d7b79a186f4272334c2297666c52f32c05787c29) )
	ROM_LOAD( "si003-02.u12", 0x0200000, 0x200000, CRC(92b4d50d) SHA1(9dc2f2961b088824d8370ac83dff796345fe4158) )
	ROM_LOAD( "si003-03.u11", 0x0400000, 0x200000, CRC(6ede4012) SHA1(6663990c6ee8e500cb8c51ad2102761ee0b3351d) )
	ROM_LOAD( "si003-04.u10", 0x0600000, 0x200000, CRC(4c65d1a1) SHA1(165f16d08813d2c989ddce4bb23b3a3652003bd5) )
	ROM_LOAD( "si003-05.u30", 0x0800000, 0x200000, CRC(daf52d56) SHA1(108419ef7d3716a3890b0d8bcbfddc1585daaae8) )
	ROM_LOAD( "si003-06.u31", 0x0a00000, 0x200000, CRC(449f9ae5) SHA1(b3e664eb88d14d1e25a0cfc8dcccc8270ca778c9) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si003-07.u23", 0x000000, 0x200000, CRC(81679fd6) SHA1(ca3b07a87781278b5c7c85951728bbe5dfcbe042) )
	ROM_LOAD16_WORD_SWAP( "si003-08.u24", 0x200000, 0x200000, CRC(d0122ba2) SHA1(96230fb690cf144cd873f7d51c0304736a698316) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )
ROM_END


/***************************************************************************

					(Mahjong) Hyper Reaction (Japan)

(c)1995 Sammy, SSV system

P1-102A (ROM board)

***************************************************************************/

ROM_START( hypreact )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "s14-1-02.u2", 0x000000, 0x080000, CRC(d90a383c) SHA1(9945f60ce6e1f50c24c2ae3c2c5d0df9ec3b8926) )
	ROM_LOAD16_BYTE( "s14-1-01.u1", 0x000001, 0x080000, CRC(80481401) SHA1(4b1b7050893b6659762297d0f6496c7193ea6c4e) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "s14-1-07.u7",  0x0000000, 0x200000, CRC(6c429fd0) SHA1(de1bbcd4a20410328d88a3b246afa8e1a6a6f232) )
	ROM_LOAD( "s14-1-05.u13", 0x0200000, 0x200000, CRC(2ff72f98) SHA1(92bd5042e19e1dae1252305413684f9cff4bd0ac) )
	ROM_LOAD( "s14-1-06.u10", 0x0400000, 0x200000, CRC(f470ec42) SHA1(f31e9c3f3daa212226b9eea14aa1d01367fa348f) )

	ROM_LOAD( "s14-1-10.u6",  0x0600000, 0x200000, CRC(fdd706ba) SHA1(893ead529c1ef62002dcff97092ff9fa51ced938) )
	ROM_LOAD( "s14-1-08.u12", 0x0800000, 0x200000, CRC(5bb9bb0d) SHA1(1874375cbe79663ff1b5181a1c16fa597a6b55f7) )
	ROM_LOAD( "s14-1-09.u9",  0x0a00000, 0x200000, CRC(d1dda65f) SHA1(b4bbd5c9da08b4d4fedb48cfe2dea4f27895c2fd) )

	ROM_LOAD( "s14-1-13.u8",  0x0c00000, 0x200000, CRC(971caf11) SHA1(7cfc8bed4431467da53e19056402aa2409be5d88) )
	ROM_LOAD( "s14-1-11.u14", 0x0e00000, 0x200000, CRC(6d8e7bae) SHA1(93258663ceb6174917560bb66d72a42ba0f96c0e) )
	ROM_LOAD( "s14-1-12.u11", 0x1000000, 0x200000, CRC(233a8e23) SHA1(0c813ec80ac63aa342c8ea57d9e38cada74456d9) )

	ROM_FILL(                 0x1200000, 0x600000, 0          )

/*	The chip seems to use REGION1 too, but produces no sound from there.*/

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "s14-1-04.u4", 0x000000, 0x200000, CRC(a5955336) SHA1(1ac0f5d27224e93acfe449d8ca5c3ab3b7f5dd8c) )
	ROM_LOAD16_WORD_SWAP( "s14-1-03.u5", 0x200000, 0x200000, CRC(283a6ec2) SHA1(766c685384ea8d801c53a2ae36b4980318aff06b) )
ROM_END


/***************************************************************************

					(Mahjong) Hyper Reaction 2 (Japan)

(c)1997 Sammy,SSV system

P1-112A (ROM board)

***************************************************************************/

ROM_START( hypreac2 )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "u2.bin",  0x000000, 0x080000, CRC(05c93266) SHA1(0833e80f67ccb4ac17e771fa04dc6f433554a34f) )
	ROM_LOAD16_BYTE( "u1.bin",  0x000001, 0x080000, CRC(80cf9e59) SHA1(7025321539891e1a3354ca233255f5395d716933) )
	ROM_LOAD16_BYTE( "u47.bin", 0x100000, 0x080000, CRC(a3e9bfee) SHA1(1e897646bafd07ab48eda2883926506c6bedab87) )
	ROM_LOAD16_BYTE( "u46.bin", 0x100001, 0x080000, CRC(68c41235) SHA1(6ec32aa6ab6074a8db63a76a3d1a0ec2dc8f8aae) )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "s16-1-16.u6",  0x0000000, 0x400000, CRC(b308ac34) SHA1(409652bc5a537650cab1f3709a2c2be206f72a78) )
	ROM_LOAD( "s16-1-15.u9",  0x0400000, 0x400000, CRC(2c8e381e) SHA1(a8681620809d3d9dc62b443232b6e4c4c4209248) )
	ROM_LOAD( "s16-1-14.u12", 0x0800000, 0x200000, CRC(afe9d187) SHA1(802df8b1bbb94e4451a6b97c852fa555a6cf5837) )

	ROM_LOAD( "s16-1-10.u7",  0x0a00000, 0x400000, CRC(86a10cbd) SHA1(7c15da7c3ffebff058e78439c64f6c0386e4d55b) )
	ROM_LOAD( "s16-1-09.u10", 0x0e00000, 0x400000, CRC(6b8e4d92) SHA1(a58c02d3fe595ab654b267cebcb1c6e8ec0b20c4) )
	ROM_LOAD( "s16-1-08.u13", 0x1200000, 0x200000, CRC(b355f45d) SHA1(7e0fe81825745555b9627716bfdf1132f20e88f0) )

	ROM_LOAD( "s16-1-13.u8",  0x1400000, 0x400000, CRC(89869af2) SHA1(46a8eec18327b515a33c6e01d35fb9b947fcab1f) )
	ROM_LOAD( "s16-1-12.u11", 0x1800000, 0x400000, CRC(87d9c748) SHA1(1332db901e50e2fd25d3323920f99e0ef0b0533d) )
	ROM_LOAD( "s16-1-11.u14", 0x1c00000, 0x200000, CRC(70b3c0a0) SHA1(009e2f2f292ed6f10a9d54557861294156664e72) )

	ROM_FILL(                 0x1e00000,0x0a00000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "s16-1-06.u41", 0x000000, 0x400000, CRC(626e8a81) SHA1(45ef5b630aed575acd160ede1413e0370f4f9761) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "s16-1-07.u42", 0x000000, 0x400000, CRC(42bcb41b) SHA1(060312b19bd52770410cec1f77e5d8d6478d80eb) )
ROM_END


/***************************************************************************

							Jan Jan Simasyo (Japan)

(c)1996 Visco, SSV System

***************************************************************************/

ROM_START( janjans1 )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "jj1-data.bin", 0x000000, 0x200000, CRC(6734537e) SHA1(a40f84479141a6f33ce465e66ba9313b54915002) )
	ROM_LOAD16_BYTE( "jj1-prol.bin", 0x200000, 0x080000, CRC(4231d928) SHA1(820d1233cd1a8d0c4ece15b94bd9be976b383fe2) )
	ROM_RELOAD(                      0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "jj1-proh.bin", 0x200001, 0x080000, CRC(651383c6) SHA1(8291f86b230eee3a2ebcc926a8370777ee21ec47) )
	ROM_RELOAD(                      0x300001, 0x080000             )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "jj1-a0.bin", 0x0000000, 0x400000, CRC(39bbbc46) SHA1(77c6b5e9d4315671ea79ec838baa7ae043bcd8c4) )
	ROM_LOAD( "jj1-a1.bin", 0x0400000, 0x400000, CRC(26020133) SHA1(32c834655d885431d466f25a729aee2d589ade1b) )
	ROM_LOAD( "jj1-a2.bin", 0x0800000, 0x200000, CRC(e993251e) SHA1(6cea12bbfc170ad4ecdc09c1728f88ec7534270a) )

	ROM_LOAD( "jj1-b0.bin", 0x0a00000, 0x400000, CRC(8ee66b0a) SHA1(f5a641d54a3040b67d9b6e9533c4e1ed3dbc9e12) )
	ROM_LOAD( "jj1-b1.bin", 0x0e00000, 0x400000, CRC(048719b3) SHA1(b81198d58afbc7ef2f7dc71cfef11d269bc1608f) )
	ROM_LOAD( "jj1-b2.bin", 0x1200000, 0x200000, CRC(6e95af3f) SHA1(c4336a3f169143d5ab828ea527c08dcac27654c3) )

	ROM_LOAD( "jj1-c0.bin", 0x1400000, 0x400000, CRC(9df28afc) SHA1(98ee75b028257614c3354d5ec7b3d4b27be75595) )
	ROM_LOAD( "jj1-c1.bin", 0x1800000, 0x400000, CRC(eb470ed3) SHA1(ac0601eb57283c3ebb2daf20d07a2c350804b8e6) )
	ROM_LOAD( "jj1-c2.bin", 0x1c00000, 0x200000, CRC(aaf72c2d) SHA1(774e713bbc4c2ed7ff7c9fb49a06246d97c33bad) )

	ROM_LOAD( "jj1-d0.bin", 0x1e00000, 0x400000, CRC(2b3bd591) SHA1(0619b2779bd4bc19a5259040ccce0fdbefecf1d0) )
	ROM_LOAD( "jj1-d1.bin", 0x2200000, 0x400000, CRC(f24c0d36) SHA1(212969b456bfd7cc00081f65c03c1e167186891a) )
	ROM_LOAD( "jj1-d2.bin", 0x2600000, 0x200000, CRC(481b3be8) SHA1(cd1bcaca8c236cebba72d315e759b2e9d243aca8) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "jj1-snd0.bin", 0x000000, 0x200000, CRC(4f7d620a) SHA1(edded130ce7bb0f37e1f59b2771ae6a10a061f9e) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "jj1-snd1.bin", 0x000000, 0x200000, CRC(9b3a7ae5) SHA1(193743fcce779c4a8a73a44c54b5391d08116331) )
ROM_END


/***************************************************************************

							Joryuu Syougi Kyoushitsu

(c)1997 Visco, System SSV ROM board

CPU : NEC JAPAN D70732GD-25 (C)NEC1991 V810 9651MK007
OSC : 48.0000MHz

ROMs:
JSK-U4 .BIN [ec22fb41] - (27c1001)
JSK-U24.BIN [1fa6e156]  |
JSK-U38.BIN [8e5c0de3]  |
JSK-U52.BIN [19cc585f] /

JSK-U71.BIN [b529f331] - (27c1001)
JSK-U72.BIN [41ed8a9f] /

JSK-S0.U65  [8d1a9aeb] - (16M mask)

JSK-A0.BIN  [4bac3196] - (16M mask)
JSK-B0.BIN  [40664e5a]  |
JSK-C0.BIN  [2a230e64]  |
JSK-D0.BIN  [911e53a6] /

GAL (not dumped):
U53.BIN     [--------] - GAL16V8B
U70.BIN     [--------] /

***************************************************************************/

ROM_START( jsk )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* !! V810 Code !! */
	ROM_LOAD32_BYTE( "jsk-u4.bin",  0x00000, 0x20000, CRC(ec22fb41) SHA1(c0d6b0a92075214a91da78be52d273771cb9f646) )	/* order?*/
	ROM_LOAD32_BYTE( "jsk-u24.bin", 0x00001, 0x20000, CRC(1fa6e156) SHA1(4daedf660d89c185c945d4a526312f6528fe7b17) )
	ROM_LOAD32_BYTE( "jsk-u38.bin", 0x00002, 0x20000, CRC(8e5c0de3) SHA1(54c5dfd858086b0eb7ffa82c19fb1dfd7752d50e) )
	ROM_LOAD32_BYTE( "jsk-u52.bin", 0x00003, 0x20000, CRC(19cc585f) SHA1(b53138e93d40c0cf03aee838d7653f5665d9cf35) )

	ROM_LOAD16_BYTE( "jsk-u72.bin", 0x80000, 0x20000, CRC(41ed8a9f) SHA1(a81efc91fbf1abc70ed64f4fa9b828a8b6f554ec) )
	ROM_LOAD16_BYTE( "jsk-u71.bin", 0x80001, 0x20000, CRC(b529f331) SHA1(be6259f5c3a6106280a4b519f83ee008d761be4e) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "jsk-a0.bin", 0x0000000, 0x200000, CRC(4bac3196) SHA1(e9530278f60e789074411c6b2f20695c3af87b87) )
	ROM_LOAD( "jsk-b0.bin", 0x0400000, 0x200000, CRC(40664e5a) SHA1(ba0c24f727d502688a37ed67dad0e3aaad33afc4) )
	ROM_LOAD( "jsk-c0.bin", 0x0800000, 0x200000, CRC(2a230e64) SHA1(2af8144b84703f896971a2819d7b7a1f662cec92) )
	ROM_LOAD( "jsk-d0.bin", 0x0c00000, 0x200000, CRC(911e53a6) SHA1(9657180b4371172babcb21ad93c6253ac5d58f86) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "jsk-s0.u65", 0x000000, 0x200000, CRC(8d1a9aeb) SHA1(37316bd3e8cbe2a84239e1a11a56d4fe4723ae1a) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )
ROM_END


/***************************************************************************

				Dramatic Adventure Quiz Keith & Lucy (Japan)

(c)1993 Visco, SSV system

STS-0001 (ROM board)

***************************************************************************/

ROM_START( keithlcy )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "vg002-07.u28", 0x000000, 0x100000, CRC(57f80ff5) SHA1(9dcc35a79d3799407190d113e0f1b57864d6c56a) )	/* "SETA SoundDriver"*/
	ROM_LOAD16_BYTE( "kl-p0l.u26",   0x100000, 0x080000, CRC(d7b177fb) SHA1(2a3533b952a7b2404720916662743c144e870c0b) )
	ROM_LOAD16_BYTE( "kl-p0h.u27",   0x100001, 0x080000, CRC(9de7add4) SHA1(16f4405b12734cb6a83cff8be21d03bb3c2e2266) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "vg002-01.u13", 0x000000, 0x200000, CRC(b44d85b2) SHA1(cf78d46f9f2594a23af08a898afbf5dd609abcec) )
	ROM_LOAD( "vg002-02.u16", 0x200000, 0x200000, CRC(aa05fd14) SHA1(9144e9668788fcd45bd6c8464f9b4f865397f783) )
	ROM_LOAD( "vg002-03.u21", 0x400000, 0x200000, CRC(299a8a7d) SHA1(b24d8ffba01d345f48f47f92e58e9b2a9ec62526) )
	ROM_LOAD( "vg002-04.u34", 0x600000, 0x200000, CRC(d3633f9b) SHA1(250a25b75a4810a676a02c390bb597b6f1cd7494) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "vg002-05.u29", 0x000000, 0x200000, CRC(66aecd79) SHA1(7735034b8fb35ad5e7916acd0c2e224a7c62e195) )
	ROM_LOAD16_WORD_SWAP( "vg002-06.u33", 0x200000, 0x200000, CRC(75d8c8ea) SHA1(545768ac6d8953cd3044680953476276337a94b9) )
ROM_END


/***************************************************************************

							Koi Koi Simasyo 2 (Japan)

(c)1997 Visco, SSV System

VISCO-JJ1 (same board as janjans1)

OSC  :8.00000MHz

KK2_A0.BIN   [0c89a9ae] GFX (32M mask)
KK2_A1.BIN   [3f85ff19]  |
KK2_B0.BIN   [4d028972]  |
KK2_B1.BIN   [16e085f7]  |
KK2_C0.BIN   [34b699d9]  |
KK2_C1.BIN   [896fbb6f]  |
KK2_D0.BIN   [0e3005a4]  |
KK2_D1.BIN   [0116a5fb] /

U26.BIN      [4be937a1] Programs (27c4001) (PROL)
U27.BIN      [25f39d93] /                  (PROH)

KK2_SND0.BIN [b27eaa94] Sound (16M mask)
KK2_SND1.BIN [e5a963e1] /

(socket for DATA ROM is empty)

***************************************************************************/

ROM_START( koikois2 )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
/*	socket for DATA ROM is empty*/
	ROM_LOAD16_BYTE( "u26.bin", 0x200000, 0x080000, CRC(4be937a1) SHA1(b2c22ec12fc110984bd1914f8e3e16a8cb866816) )
	ROM_RELOAD(                 0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "u27.bin", 0x200001, 0x080000, CRC(25f39d93) SHA1(a36bc2fe5657f6ceada724fd42843e19408b39b8) )
	ROM_RELOAD(                 0x300001, 0x080000             )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
 	ROM_LOAD( "kk2-a0.bin", 0x0000000, 0x400000, CRC(b94b76c2) SHA1(07ce3e3946669c1bd2f022da9861164625be9c1b) )
 	ROM_LOAD( "kk2-a1.bin", 0x0400000, 0x200000, CRC(a7c99f56) SHA1(de341e99f76446fab4d7f09c2d8a6f18554b5d2f) )

 	ROM_LOAD( "kk2-b0.bin", 0x0800000, 0x400000, CRC(4d028972) SHA1(732c874d3511c7bce006436d557ec24e54df0166) )
 	ROM_LOAD( "kk2-b1.bin", 0x0c00000, 0x200000, CRC(778ec9fb) SHA1(5983f0292e274e3da098b461355e2c001f4881b3) )

 	ROM_LOAD( "kk2-c0.bin", 0x1000000, 0x400000, CRC(34b699d9) SHA1(b5208d5f70f21725e54c9dc59de73f1a5646a72c) )
 	ROM_LOAD( "kk2-c1.bin", 0x1400000, 0x200000, CRC(ab451e88) SHA1(0c4d6c0c758f2ab4210c201605dd573661b6c553) )

 	ROM_LOAD( "kk2-d0.bin", 0x1800000, 0x400000, CRC(0e3005a4) SHA1(fa8da58308d58bb6b2e8beb8ee8f7ea08b18f4d9) )
 	ROM_LOAD( "kk2-d1.bin", 0x1c00000, 0x200000, CRC(17a02252) SHA1(c7aa61e27f197b3c497a65a9369e3a6a20c9f82a) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "kk2_snd0.bin", 0x000000, 0x200000, CRC(b27eaa94) SHA1(05baaef683a1fcd9eb8a7cfd5b280c05108e832f) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "kk2_snd1.bin", 0x000000, 0x200000, CRC(e5a963e1) SHA1(464ffd53ac2e6db62225b18d12bfea93160771ec) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND2, 0x000000, 0x000000, 0x400000 )
ROM_END


/***************************************************************************

						Meosis Magic (Japan, BET?)

(c)1996 Sammy, SSV System

P1-105A

Custom:		DX-102 (I/O)
Others:		M62X42B (RTC?)
			64k SRAM (Back up)
			Ni-Cd Battery

***************************************************************************/

ROM_START( meosism )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "s15-2-2.u47", 0x000000, 0x080000, CRC(2ab0373f) SHA1(826aec3b9698ec5db5d7a72c3a24b1ef779fb227) )
	ROM_LOAD16_BYTE( "s15-2-1.u46", 0x000001, 0x080000, CRC(a4bce148) SHA1(17ec4d91e215bd38258329b1a71e7f135c5733ad) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "s15-1-7.u7", 0x000000, 0x200000, CRC(ec5023cb) SHA1(3406f5143a40c8dcd2d45b44ea91c737810ab05b) )
	ROM_LOAD( "s15-1-8.u6", 0x200000, 0x200000, CRC(f04b0836) SHA1(83678427cd0ed0d68ff770baa2693226b391f6c8) )
	ROM_LOAD( "s15-1-5.u9", 0x400000, 0x200000, CRC(c0414b97) SHA1(3ca8423e04f606981d158065e38431f2509e1daa) )
	ROM_LOAD( "s15-1-6.u8", 0x600000, 0x200000, CRC(d721aeb6) SHA1(3bef7e027a0e14fbf589aee32a6d9cab779da7d4) )

/*	The chip seems to use REGION1 too, but produces no sound from there.*/

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "s15-1-4.u45", 0x000000, 0x200000, CRC(0c6738a7) SHA1(acf9056bb052db7a11cf903d77ab16425d813835) )
	ROM_LOAD16_WORD_SWAP( "s15-1-3.u43", 0x200000, 0x200000, CRC(d7e83178) SHA1(74e5c09f6d3b2c8e1c1cc2b0eab0490b5bbc9099) )
ROM_END


/***************************************************************************

							Monster Slider (Japan)

(c)1997 Visco/PATT, System SSV

ms-pl.bin - V60 main program (27c4000, low)
ms-ph.bin - V60 main program (27c4000, high)

ms-snd0.bin \
            |- sound data (read as 27c160)
ms-snd1.bin /

ms-a0.bin \
ms-b0.bin |- Graphics (read as 27c160)
ms-c0.bin /

ms-a1.bin \
ms-b1.bin |- Graphics (27c4100)
ms-c1.bin /

vg001-14 \
         |- (GAL16V8. not dumped)
vg001-15 /

Other parts:	uPD71051
				OSC 8.0000MHz

***************************************************************************/

ROM_START( mslider )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "ms-pl.bin", 0x000000, 0x080000, CRC(70b2a05d) SHA1(387cf67e3e505c4cc1b5cd0b6c9fb3bc27d07e24) )
	ROM_LOAD16_BYTE( "ms-ph.bin", 0x000001, 0x080000, CRC(34a64e9f) SHA1(acf3d8490f3ec99b6171e71328a991fcc9c5a8b1) )

	ROM_REGION( 0xa00000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ms-a0.bin", 0x000000, 0x200000, CRC(7ed38ccc) SHA1(9c584a5f6b3aad8646d155a56e4070cfed4af540) )
	ROM_LOAD( "ms-a1.bin", 0x200000, 0x080000, CRC(83f5995f) SHA1(33ae99a96702d4aba422eaf454b86c96aaf88426) )

	ROM_LOAD( "ms-b0.bin", 0x280000, 0x200000, CRC(faa076e1) SHA1(cca583c617e5d4ab995605dd16280931893991c7) )
	ROM_LOAD( "ms-b1.bin", 0x480000, 0x080000, CRC(ef9748db) SHA1(34ab4524ec81b81ae2540f7d69e0f8254fd1b8f4) )

	ROM_LOAD( "ms-c0.bin", 0x500000, 0x200000, CRC(f9d3e052) SHA1(4cdde756b24ee980f3c79a35a1fe071861fdeef9) )
	ROM_LOAD( "ms-c1.bin", 0x700000, 0x080000, CRC(7f910c5a) SHA1(23ea13b6c07d3d31a25c21704d6a3e506578b199) )

	ROM_FILL(              0x780000, 0x280000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "ms-snd0.bin", 0x000000, 0x200000, CRC(cda6e3a5) SHA1(28ad8f34bc4f907654582f3522b377b97234eba8) )
	ROM_LOAD16_WORD_SWAP( "ms-snd1.bin", 0x200000, 0x200000, CRC(8f484b35) SHA1(cbf3ee7ec6337915f9d90a5b43d2de1eaa5537d0) )
ROM_END


/***************************************************************************

					Gourmet Battle Quiz Ryohrioh CooKing (Japan)

(c)1998 Visco, SSV System

***************************************************************************/

ROM_START( ryorioh )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD( "ryorioh.dat",      0x000000, 0x200000, CRC(d1335a6a) SHA1(a5670ab3c399736232baaabc59573bdb3bf762da) )
	ROM_LOAD16_BYTE( "ryorioh.l", 0x200000, 0x080000, CRC(9ad60e7d) SHA1(572b84bab08eb8293d93e03182d9871d8973b7dd) )
	ROM_RELOAD(                   0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "ryorioh.h", 0x200001, 0x080000, CRC(0655fcff) SHA1(2c088e42323f87e01b65f9f523e258f881d4e773) )
	ROM_RELOAD(                   0x300001, 0x080000             )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ryorioh.a0", 0x0000000, 0x400000, CRC(f76ee003) SHA1(04022238dcfd5cf0e4f97c3c3b24df574ec6b609) )
	ROM_LOAD( "ryorioh.a1", 0x0400000, 0x400000, CRC(ca44d66d) SHA1(d5ed2bbc9831182b212533bd67bb3831f655110a) )

	ROM_LOAD( "ryorioh.b0", 0x0800000, 0x400000, CRC(daa134f4) SHA1(c3dce66b2c67061ae980374f7559b2bb8ef2502d) )
	ROM_LOAD( "ryorioh.b1", 0x0c00000, 0x400000, CRC(7611697c) SHA1(febb0bc5f3bc8766be4377092c8443a489379bca) )

	ROM_LOAD( "ryorioh.c0", 0x1000000, 0x400000, CRC(20eb49cf) SHA1(13c201e28be17cdfc7e6266a6d1fb41cfbe04b53) )
	ROM_LOAD( "ryorioh.c1", 0x1400000, 0x400000, CRC(1370c75e) SHA1(30dfe37f3fab0e3e94df4a6d45f1291ad41e0147) )

	ROM_LOAD( "ryorioh.d0", 0x1800000, 0x400000, CRC(ffa14ef1) SHA1(22a6992f6217d8ef2140e72063290fa34cb45683) )
	ROM_LOAD( "ryorioh.d1", 0x1c00000, 0x400000, CRC(ae6055e8) SHA1(ee20a7b3c4f899404ca259991509728d3a0f96b9) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "ryorioh.snd", 0x000000, 0x200000, CRC(7bd38b76) SHA1(d8490b4af839ef0802b8b2a47277fcd4091e4d37) )
ROM_END


/***************************************************************************

							Super Real Mahjong PIV

(c)SETA 1993, System SSV

CPU        : V60 (12MHz)
Sound      : Ensoniq OTTO
Work RAM   : 256Kbit (expandable to 1Mbitx2. SRMP7 requires this)
Object RAM : 1Mbitx2
Palette RAM: 256Kbitx3 (expandable to 1Mbitx3)

sx001-01.a0 \
sx001-02.b0 |
sx001-03.c0 |
sx001-04.a1 |
sx001-05.b1 |- Graphics (16M Mask)
sx001-06.c1 |
sx001-07.a2 |
sx001-08.b2 |
sx001-09.c2 /

sx001-10.sd0 - Sound - 16M Mask

sx001-11.prl - Main program (low)  - 27c040
sx001-12.prh - Main program (high) - 27c040

Custom chips
ST-0004 (Video DAC)
ST-0005 (Parallel I/O)
ST-0006 (Video controller - 32768 palettes from 24bit color)
ST-0007 (System controller)

***************************************************************************/

ROM_START( srmp4 )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "sx001-14.prl", 0x000000, 0x080000, CRC(19aaf46e) SHA1(0c0f5acc1880971c56e7e2c2e3ad7c2932b82d4b) )
	ROM_LOAD16_BYTE( "sx001-15.prh", 0x000001, 0x080000, CRC(dbd31399) SHA1(a77dc85f481454b10223d7f4e0395e07d2f8d4f3) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sx001-01.a0", 0x0000000, 0x200000, CRC(94ee9203) SHA1(a0e944a375f94e9dd668b06f15580384902d0fe1) )
	ROM_LOAD( "sx001-04.a1", 0x0200000, 0x200000, CRC(38c9c49a) SHA1(c392d1cf5d16a348bdaa7222f2420a61a831a50a) )
	ROM_LOAD( "sx001-07.a2", 0x0400000, 0x200000, CRC(ee66021e) SHA1(f4df2bdf8100a3bd39bb61f9bb4807ca9e13537a) )

	ROM_LOAD( "sx001-02.b0", 0x0600000, 0x200000, CRC(adffb598) SHA1(fab372aebfbb12feaf7a7716a780cf2e5cc60731) )
	ROM_LOAD( "sx001-05.b1", 0x0800000, 0x200000, CRC(4c400a38) SHA1(42623d6134fb6d8ce9059f7774c6bf4d2ea5d2d9) )
	ROM_LOAD( "sx001-08.b2", 0x0a00000, 0x200000, CRC(36efd52c) SHA1(f51c6d9ceff02b9ee3f8e4ffa17f6c00ee9de905) )

	ROM_LOAD( "sx001-03.c0", 0x0c00000, 0x200000, CRC(4336b037) SHA1(f42c5622e141e384efb52955f7f6a58a8ba8fc2c) )
	ROM_LOAD( "sx001-06.c1", 0x0e00000, 0x200000, CRC(6fe7229e) SHA1(e1432aa500460f79b5b78ee4b249d8fc9f566ce1) )
	ROM_LOAD( "sx001-09.c2", 0x1000000, 0x200000, CRC(91dd8218) SHA1(a500dca9eefbf93187b1dfde7ddff1d22b886d44) )

	ROM_FILL(                0x1200000, 0x600000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "sx001-10.sd0", 0x000000, 0x200000, CRC(45409ef1) SHA1(327d0a63deac6f0f8b9a408a321c03dd4e965569) )
	ROM_RELOAD(                           0x200000, 0x200000             )
ROM_END

ROM_START( srmp4o )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "sx001-11.prl", 0x000000, 0x080000, CRC(dede3e64) SHA1(6fe998babfd2ad8f268c59bd365115a2d7cfc8f9) )
	ROM_LOAD16_BYTE( "sx001-12.prh", 0x000001, 0x080000, CRC(739c53c3) SHA1(68f12cf42177df208ff6499ccc7ccc1423e3ad5f) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sx001-01.a0", 0x0000000, 0x200000, CRC(94ee9203) SHA1(a0e944a375f94e9dd668b06f15580384902d0fe1) )
	ROM_LOAD( "sx001-04.a1", 0x0200000, 0x200000, CRC(38c9c49a) SHA1(c392d1cf5d16a348bdaa7222f2420a61a831a50a) )
	ROM_LOAD( "sx001-07.a2", 0x0400000, 0x200000, CRC(ee66021e) SHA1(f4df2bdf8100a3bd39bb61f9bb4807ca9e13537a) )

	ROM_LOAD( "sx001-02.b0", 0x0600000, 0x200000, CRC(adffb598) SHA1(fab372aebfbb12feaf7a7716a780cf2e5cc60731) )
	ROM_LOAD( "sx001-05.b1", 0x0800000, 0x200000, CRC(4c400a38) SHA1(42623d6134fb6d8ce9059f7774c6bf4d2ea5d2d9) )
	ROM_LOAD( "sx001-08.b2", 0x0a00000, 0x200000, CRC(36efd52c) SHA1(f51c6d9ceff02b9ee3f8e4ffa17f6c00ee9de905) )

	ROM_LOAD( "sx001-03.c0", 0x0c00000, 0x200000, CRC(4336b037) SHA1(f42c5622e141e384efb52955f7f6a58a8ba8fc2c) )
	ROM_LOAD( "sx001-06.c1", 0x0e00000, 0x200000, CRC(6fe7229e) SHA1(e1432aa500460f79b5b78ee4b249d8fc9f566ce1) )
	ROM_LOAD( "sx001-09.c2", 0x1000000, 0x200000, CRC(91dd8218) SHA1(a500dca9eefbf93187b1dfde7ddff1d22b886d44) )

	ROM_FILL(                0x1200000, 0x600000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "sx001-10.sd0", 0x000000, 0x200000, CRC(45409ef1) SHA1(327d0a63deac6f0f8b9a408a321c03dd4e965569) )
	ROM_RELOAD(                           0x200000, 0x200000             )
ROM_END


/***************************************************************************

							Super Real Mahjong P7 (Japan)

(c)1997 Seta, SSV system

***************************************************************************/

ROM_START( srmp7 )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "sx015-10.dat", 0x000000, 0x200000, CRC(fad3ac6a) SHA1(9a4695c06bc74ca4de0c1a83bdf38f6651c0e2a1) )
	ROM_LOAD16_BYTE( "sx015-07.pr0", 0x200000, 0x080000, CRC(08d7f841) SHA1(67567acff0ce278576290a896005de0397605eef) )
	ROM_RELOAD(                      0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "sx015-08.pr1", 0x200001, 0x080000, CRC(90307825) SHA1(13b3f82c8854808684bd41deb0bbd442efe7b685) )
	ROM_RELOAD(                      0x300001, 0x080000             )

	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sx015-26.a0", 0x0000000, 0x400000, CRC(a997be9d) SHA1(37470af24531557113f953f727f6b8cab602a7d3) )
	ROM_LOAD( "sx015-25.a1", 0x0400000, 0x400000, CRC(29ac4211) SHA1(32edf3982b0e27077cc17cd38b67a27d36dc3ad8) )
	ROM_LOAD( "sx015-24.a2", 0x0800000, 0x400000, CRC(b8fea3da) SHA1(9c3a53348f72f39d84d078068c62b10920854cd0) )
	ROM_LOAD( "sx015-23.a3", 0x0c00000, 0x400000, CRC(9ec0b81e) SHA1(fe9550592852db8a0fc38f8af444c4c137b803eb) )

	ROM_LOAD( "sx015-22.b0", 0x1000000, 0x400000, CRC(62c3df07) SHA1(2c2b7ccc53d0ccc78f599d0789d38296935c0316) )
	ROM_LOAD( "sx015-21.b1", 0x1400000, 0x400000, CRC(55b8a431) SHA1(ccdc70b27c7fc9efe2c20df23f01b96f3b542d72) )
	ROM_LOAD( "sx015-20.b2", 0x1800000, 0x400000, CRC(e84a64d7) SHA1(af7a04cebb1ccbbd76812f7b6f7bb79023aff291) )
	ROM_LOAD( "sx015-19.b3", 0x1c00000, 0x400000, CRC(994b5063) SHA1(0c44e94773160e75ef03f7ceb95ab1b123ae3ecf) )

	ROM_LOAD( "sx015-18.c0", 0x2000000, 0x400000, CRC(72d43fd4) SHA1(96582adae0bf1cc8359dd1ecc0d00a42d306c565) )
	ROM_LOAD( "sx015-17.c1", 0x2400000, 0x400000, CRC(fdfd82f1) SHA1(005b60fd7bf9f61ecd16daa6e6bb213ed6a9875b) )
	ROM_LOAD( "sx015-16.c2", 0x2800000, 0x400000, CRC(86aa314b) SHA1(5cd238785f683d3a33f36c5a326d350805ef21ff) )
	ROM_LOAD( "sx015-15.c3", 0x2c00000, 0x400000, CRC(11f50e16) SHA1(4e26aa84bea8b7e73056b0cc70661332fa7d9473) )

	ROM_LOAD( "sx015-14.d0", 0x3000000, 0x400000, CRC(186f83fa) SHA1(6f03056b766c223fef639627706c2476f9af378d) )
	ROM_LOAD( "sx015-13.d1", 0x3400000, 0x400000, CRC(ea6e5329) SHA1(614dd8d36d94a4c8b2b0c30ec96c6d183065561e) )
	ROM_LOAD( "sx015-12.d2", 0x3800000, 0x400000, CRC(80336523) SHA1(ec66e21fe1401fdb438e03657542a7b6b0cbc5ce) )
	ROM_LOAD( "sx015-11.d3", 0x3c00000, 0x400000, CRC(134c8e28) SHA1(669118b58f27d5e2e08052debe904f95d9ab32a3) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sx015-06.s0", 0x000000, 0x200000, CRC(0d5a206c) SHA1(2fdaf2a56b6608f20a788eb79a8426102ff33e14) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sx015-05.s1", 0x000000, 0x200000, CRC(bb8cebe2) SHA1(3691e5fb4e963f69c1fe01cb5d968433029c4833) )

	ROM_REGION16_BE( 0x800000, REGION_SOUND3, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sx015-04.s2", 0x000000, 0x200000, CRC(f6e933df) SHA1(7cb69515a0ffc62fbac2be3a5fb322538560bd38) )
	ROM_LOAD16_BYTE( "sx015-02.s4", 0x400000, 0x200000, CRC(6567bc3e) SHA1(e902f22f1499edc6a0e2c8b6cc26460d66a3bdbe) )

	ROM_REGION16_BE( 0x800000, REGION_SOUND4, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sx015-03.s3", 0x000000, 0x200000, CRC(5b51ab21) SHA1(cf3e86e41f7984208984d6486b04cec117dadc18) )
	ROM_LOAD16_BYTE( "sx015-01.s5", 0x400000, 0x200000, CRC(481b00ed) SHA1(2c3d158dd5be9af0ee57fd5dd94d2ec75e28b182) )
ROM_END


/***************************************************************************

         Survival Arts

 Manufacturer: Sammy USA
 System Type: System SSV

 ----------------------
 System SSV (STA-0001)
 ----------------------
 CPU  : NEC D70615GD-16 (V60)
 Sound: Ensoniq ES5506 (OTTOR2)
 OSC  : 42.9545MHz(X2) 48.0000MHz(X3)

 Custom chips:
 ST-0004 (Video DAC)
 ST-0005 (Parallel I/O)
 ST-0006 (Video controller)
 ST-0007 (System controller)

 Program Work RAM  : 256Kbitx2 (expandable to 1Mx2)
 Object Work RAM   : 1Mbitx2
 Color Palette RAM : 256Kbitx3 (expandable to 1Mx3)

 -------------------------
 SSV Subboard (SAM-5127)
 -------------------------
 ROMs:
 USA-PR-H.u3 - V60 Program (27C4001)
 USA-PR-L.u4 /

 si001-10.s0 - Samples (16M-Mask)
 si001-12.s2 /

 si001-11.s1 - Samples (8M-Mask)
 si001-13.s3 /

 si001-01.a0 - Graphics (16M-Mask)
 si001-04.a1 |
 si001-05.a2 |
 si001-02.b0 |
 si001-05.b1 |
 si001-07.b2 |
 si001-03.c0 |
 si001-06.c1 |
 si001-09.c2 /

 Empty Sockets:
 DATA --- 16M-Mask
 A3     |
 B3     |
 C3     |
 D0-D3 /

 GAL:
 si003-14.u5 (16V8B)

 MISC:
 Pin Header for P3 (used)
 Pin Header for P4 (unused)

***************************************************************************/

ROM_START( survarts )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "usa-pr-l.u4", 0x000000, 0x080000, CRC(fa328673) SHA1(f7217eaa2a8d3fb7f706fa1aecaaa5b1b8d5e32c) )
	ROM_LOAD16_BYTE( "usa-pr-h.u3", 0x000001, 0x080000, CRC(6bee2635) SHA1(a2d0517bf599331ef47beb8a902589039e4502e0) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "si001-01.a0", 0x0000000, 0x200000, CRC(8b38fbab) SHA1(c4a67b24b33d4eef7b0f885bd69cae6c67bd3981) )
	ROM_LOAD( "si001-04.a1", 0x0200000, 0x200000, CRC(34248b54) SHA1(077198f8de1622b71c580e34d5ad1b6bf3229fe9) )
	ROM_LOAD( "si001-07.a2", 0x0400000, 0x200000, CRC(497d6151) SHA1(a9860c75943c0fd2991660ce2a9505edc6c2fa46) )

	ROM_LOAD( "si001-02.b0", 0x0600000, 0x200000, CRC(cb4a2dbd) SHA1(26cdd1b54a3fa1dc3c3a8945d1a3562e9c62ace6) )
	ROM_LOAD( "si001-05.b1", 0x0800000, 0x200000, CRC(8f092381) SHA1(6c49f1f5b3c31bd7c6a93ba0450d9f64fd512633) )
	ROM_LOAD( "si001-08.b2", 0x0a00000, 0x200000, CRC(182b88c4) SHA1(a5b6a3e1fd67f036b1255385e81b6a3eb69f9f3f) )

	ROM_LOAD( "si001-03.c0", 0x0c00000, 0x200000, CRC(92fdf652) SHA1(cf7aeb3a1e8ffe34cf24cb919a0ab3cc90202fa9) )
	ROM_LOAD( "si001-06.c1", 0x0e00000, 0x200000, CRC(9a62f532) SHA1(7e7ba1224e52b33a9bd14058230efc871178c4f8) )
	ROM_LOAD( "si001-09.c2", 0x1000000, 0x200000, CRC(0955e393) SHA1(0be9134190706eaee49177034b0536b05c4bc7ac) )

	ROM_FILL(                0x1200000, 0x600000, 0          )

/*	The chip seems to use REGION1 too, but produces no sound from there.*/

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si001-10.s0", 0x000000, 0x100000, CRC(5642b333) SHA1(84936af8b3882e116b279e422075f35aabdd232f) )
	ROM_LOAD16_WORD_SWAP( "si001-11.s1", 0x100000, 0x100000, CRC(a81e6ea6) SHA1(499f070500895ed7b6785b42fb6bbf973fc6dc04) )
	ROM_LOAD16_WORD_SWAP( "si001-12.s2", 0x200000, 0x100000, CRC(e9b2b45b) SHA1(17fd27cdb8a0b9932cb1e71e0547c0d9d6fc7d06) )
	ROM_LOAD16_WORD_SWAP( "si001-13.s3", 0x300000, 0x100000, CRC(d66a7e26) SHA1(57b659daef00421b6742963f792bd5e020f625c9) )
ROM_END

/*

Dynagears
Sammy, 1993

This game runs on SSV hardware.

Game PCB Layout
---------------

SAM-5127
|----------------------------------------|
| SI002-10.U6                            |
|                                        |
| SI002-09.U7                            |
|                                        |
| SI002-08.U8                            |
|                                        |
| SI002-07.U9                            |
|                                        |
|                                        |
|                                        |
|                                        |
|                                        |
|                                        |
|                                        |
|                                        |
|                           SI002-05.U22 |
|                                        |
|                           SI002-02.U23 |
|                                        |
|                                        |
|SI002-PRH.U3                            |
|                                        |
|SI002-PRL.U4                            |
|                                        |
|             SI002-06.U16  SI002-04.U26 |
|PAL                                     |
|(SI002-14)   SI002-03.U17  SI002-01.U27 |
|                                        |
|----------------------------------------|

*/

ROM_START( dynagear )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "si002-prl.u4", 0x000000, 0x080000, CRC(71ba29c6) SHA1(ef43ab665daa4fc9ee01996d03f2f0b4c74c8435) )
	ROM_LOAD16_BYTE( "si002-prh.u3", 0x000001, 0x080000, CRC(d0947a12) SHA1(95b54ed9dc51c952ad123103b8633a821cde05e9) )
	ROM_LOAD16_BYTE( "si002-prl.u4", 0x100000, 0x080000, CRC(71ba29c6) SHA1(ef43ab665daa4fc9ee01996d03f2f0b4c74c8435) )
	ROM_LOAD16_BYTE( "si002-prh.u3", 0x100001, 0x080000, CRC(d0947a12) SHA1(95b54ed9dc51c952ad123103b8633a821cde05e9) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "si002-01.u27", 0x0000000, 0x200000, CRC(0060a521) SHA1(10cdb967e6cb4fc7c23c1ac40b24e35262060f5c) )
	ROM_LOAD( "si002-04.u26", 0x0200000, 0x200000, CRC(6140f47d) SHA1(49dcebe724990acdac76746886efe88b68ce956f) )

	ROM_LOAD( "si002-02.u23", 0x0400000, 0x200000, CRC(c22f2a41) SHA1(969affc8bac9a6024e7e5103384a40a6a2acf653) )
	ROM_LOAD( "si002-05.u22", 0x0600000, 0x200000, CRC(482412fd) SHA1(dfb896631b6999ce8ac6aeef84ff44150d67739a) )

	ROM_LOAD( "si002-03.u17", 0x0800000, 0x200000, CRC(4261a6b8) SHA1(df163faa84a86f126d5d405aef316ff9dd3c05eb) )
	ROM_LOAD( "si002-06.u16", 0x0a00000, 0x200000, CRC(0e1f23f6) SHA1(ea35c75776b75131ef9133a16a36d95132dc6776) )

	ROM_FILL(                0xc00000, 0x400000, 0          )

/*	The chip seems to use REGION1 too, but produces no sound from there.*/

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "si002-07.u9", 0x000000, 0x100000, CRC(30d2bf11) SHA1(263e9a4e6a77aa451daf6d1225071cc1147a6541) )
	ROM_LOAD16_WORD_SWAP( "si002-08.u8", 0x100000, 0x100000, CRC(253704ee) SHA1(887ebca2af497fc59b274838cdf284223cc92c97) )
	ROM_LOAD16_WORD_SWAP( "si002-09.u7", 0x200000, 0x100000, CRC(1ea86db7) SHA1(e887ea5be99f753e73355a45e37dfddb2a1d6cf6) )
	ROM_LOAD16_WORD_SWAP( "si002-10.u6", 0x300000, 0x100000, CRC(e369c177) SHA1(646aad00a8f9eda847e9a51fb0a511bf49eb9fe2) )
ROM_END


/***************************************************************************

						Pachinko Sexy Reaction (Japan)

(c)1998 Sammy, SSV system

P1-112C (ROM board)

Chips:	DX-102 x2
		uPD7001C (ADC?)
		64k NVRAM

***************************************************************************/

ROM_START( sxyreact )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_BYTE( "ac414e00.u2",  0x000000, 0x080000, CRC(d5dd7593) SHA1(ad1c7c2f27e0423ab346172a5c91316c9c0b3620) )
	ROM_LOAD16_BYTE( "ac413e00.u1",  0x000001, 0x080000, CRC(f46aee4a) SHA1(8336304797987321903977373dec027cfca2e211) )
	ROM_LOAD16_BYTE( "ac416e00.u47", 0x100000, 0x080000, CRC(e0f7bba9) SHA1(5eafd72c9fa4588f18fa02113a93abdcaf8d8693) )
	ROM_LOAD16_BYTE( "ac415e00.u46", 0x100001, 0x080000, CRC(92de1b5f) SHA1(69e30ffc0c59e7dafe3f9c76bfee782028dab042) )

	ROM_REGION( 0x2800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "ac1401m0.u6",  0x0000000, 0x400000, CRC(0b7b693c) SHA1(1e65c3f55cf3aa63d4229d30b5894c89b83cdf3e) )
	ROM_LOAD( "ac1402m0.u9",  0x0400000, 0x400000, CRC(9d593303) SHA1(c02037fabe8a74f01a25357ffdd3ce01b930008b) )
	ROM_LOAD( "ac1403m0.u12", 0x0800000, 0x200000, CRC(af433eca) SHA1(dfd83eba390171d93bc6888cc1d24a9a38d900bd) )

	ROM_LOAD( "ac1404m0.u7",  0x0a00000, 0x400000, CRC(cdda2ccb) SHA1(c22ff59e1cf621e0288537be567e0b42bf8e9bcf) )
	ROM_LOAD( "ac1405m0.u10", 0x0e00000, 0x400000, CRC(e5e7a5df) SHA1(9ab32f2a1ef055825b6cd3f643af1bd62c53f46a) )
	ROM_LOAD( "ac1406m0.u13", 0x1200000, 0x200000, CRC(c7053409) SHA1(468527a24ea592dbd884431ca57d43790cbc2456) )

	ROM_LOAD( "ac1407m0.u8",  0x1400000, 0x400000, CRC(28c83d5e) SHA1(abf4f0c1e2caa5cfa5a18fc95c025d73d6a8bc95) )
	ROM_LOAD( "ac1408m0.u11", 0x1800000, 0x400000, CRC(c45bab47) SHA1(d00802005e091088eabeb672a6428417db43cb66) )
	ROM_LOAD( "ac1409m0.u14", 0x1c00000, 0x200000, CRC(be1c66c2) SHA1(6d7b60d3b4286a768eac122c3d163e6e5287adc3) )

	ROM_FILL(                 0x1e00000, 0xa00000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_WORD_SWAP( "ac1410m0.u41", 0x000000, 0x400000, CRC(2a880afc) SHA1(193235bccde28a7d693a1a1f0159260a3a63a7d5) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD16_WORD_SWAP( "ac1411m0.u42", 0x200000, 0x200000, CRC(2ba4ca43) SHA1(9cddf57094e68d3840a37f44fbdf2f43f539ba11) )
	ROM_CONTINUE( 0x000000, 0x200000 )	/* this will go in region 3*/

	/* a few sparse samples are played from here*/
	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_COPY( REGION_SOUND2, 0x000000,    0x200000, 0x200000 )
ROM_END


/***************************************************************************

								Storm Blade
CPU  : NEC D70615GD-16-S (V60)
Sound: Ensoniq ES5506 (OTTOR2)


Rom board  001B
SSV mother board

U37, U33 = 27c040
U22, U41, U35, U25, U21, U11, U7  = 16 MEG MASK ROMS
U32, U18, U4 = 4 MEG MASK ROMS
U26 = 8 MEG MASK ROM

There is a battery on the rom board @ BT1 (battery # CR2032 - 3 volts)

***************************************************************************/

ROM_START( stmblade )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "sb-pd0.u26",  0x000000, 0x100000, CRC(91c4fbf7) SHA1(68e57ea2a9756a95a81c6688905352d631e9f2de) )
	ROM_LOAD16_BYTE( "s-blade.u37", 0x200000, 0x080000, CRC(a6a42cc7) SHA1(4bff79ff03b81a7ed96d3ad285242580146976be) )
	ROM_RELOAD(                     0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "s-blade.u33", 0x200001, 0x080000, CRC(16104ca6) SHA1(63835051c358dce33d92974d1de911b98835a3d9) )
	ROM_RELOAD(                     0x300001, 0x080000             )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "sb-a0.u41", 0x0000000, 0x200000, CRC(2a327b51) SHA1(fb1e92b7f740a80cb0c977e106d0c4bfee092dad) )
	ROM_LOAD( "sb-a1.u35", 0x0200000, 0x200000, CRC(246f6f28) SHA1(09171f04452fbcf9e3333c135288fd6e5b8244f7) )
	ROM_LOAD( "sb-a2.u32", 0x0400000, 0x080000, CRC(2049acf3) SHA1(3982b4650921da0563336060887767627f8679ab) )
	ROM_LOAD( "sb-b0.u25", 0x0600000, 0x200000, CRC(b3aa3e68) SHA1(990be5925b6c8c0d0e83ca9064425d93853fe206) )
	ROM_LOAD( "sb-b1.u21", 0x0800000, 0x200000, CRC(e95b38e7) SHA1(9256f027e4c496e3bf96ecb65c0f3e69791e2755) )
	ROM_LOAD( "sb-b2.u18", 0x0a00000, 0x080000, CRC(d080e620) SHA1(a262b42214c09fccb8f4878d8566e2acd87dbd23) )
	ROM_LOAD( "sb-c0.u11", 0x0c00000, 0x200000, CRC(825dd8f1) SHA1(39d32f54c97e21f92598442f05fd91ae2403a0d2) )
	ROM_LOAD( "sb-c1.u7",  0x0e00000, 0x200000, CRC(744afcd7) SHA1(db716a1a2ad5864ebdb4865430cb637fb94ed34f) )
	ROM_LOAD( "sb-c2.u4",  0x1000000, 0x080000, CRC(fd1d2a92) SHA1(957a8a52b79e252c7f1a4b6383107ae609dce5ef) )
	ROM_FILL(              0x1200000, 0x600000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "sb-snd0.u22", 0x000000, 0x200000, CRC(4efd605b) SHA1(9c97be105c923c7db847d9b9aea37025edb685a0) )
ROM_END


/***************************************************************************

								Twin Eagle II

***************************************************************************/

ROM_START( twineag2 )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "sx002_12", 0x000000, 0x200000, CRC(846044dc) SHA1(c1c85de1c466fb7c3580824baa1571cd0fed6ec6) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "sx002_01", 0x0000000, 0x200000, CRC(6d6896b5) SHA1(e8efd29b9f951bff6664e47cb5fd67f1d8f40608) )
	ROM_LOAD( "sx002_02", 0x0200000, 0x200000, CRC(3f47e97a) SHA1(5b0fdc762cf704c8bd92c4a4a42dba4a127b3d49) )
	ROM_LOAD( "sx002_03", 0x0400000, 0x200000, CRC(544f18bf) SHA1(539e6df1ded4e9ac8974c697215cc1e5c5a40cda) )

	ROM_LOAD( "sx002_04", 0x0600000, 0x200000, CRC(58c270e2) SHA1(7629ba978b18252f375bdc16ed62388d64a35ca1) )
	ROM_LOAD( "sx002_05", 0x0800000, 0x200000, CRC(3c310229) SHA1(9a8b81d5f17ce3078627a697aaf07f1b3ba6e08c) )
	ROM_LOAD( "sx002_06", 0x0a00000, 0x200000, CRC(46d5b1f3) SHA1(3ec03eddb159eb391ccdce5a0a867a54b3350150) )

	ROM_LOAD( "sx002_07", 0x0c00000, 0x200000, CRC(c30fa397) SHA1(d4575868c1b63f9e94bf24539a3fd8a85df93d0b) )
	ROM_LOAD( "sx002_08", 0x0e00000, 0x200000, CRC(64edcefa) SHA1(55a71afe87da93e35c5ba291e970bdcd91b52a7a) )
	ROM_LOAD( "sx002_09", 0x1000000, 0x200000, CRC(51527c56) SHA1(378155a585e5b847bd8ae1f17cb651138d844e33) )

	ROM_FILL(             0x1200000, 0x600000, 0          )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD16_BYTE( "sx002_10", 0x000000, 0x200000, CRC(b0669dfa) SHA1(ff805f59864ac4ccee3e249c06804d844d3df59c) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD16_BYTE( "sx002_11", 0x000000, 0x200000, CRC(b8dd621a) SHA1(f9b43e018f2bb121e4f4e9554419cd32b870556b) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND2, 0x000000, 0x000000, 0x400000 )
ROM_END


/***************************************************************************

				Ultra Keibitai / Ultra X Weapon

(c)1995 Banpresto (developed by Seta)
Hardware is almost identical to SSV system

****************************************************************************/

ROM_START( ultrax )
	ROM_REGION16_LE( 0x200000, REGION_USER1, 0 )  /* V60 Code */
	ROM_LOAD16_BYTE( "71047-11.u64", 0x000000, 0x080000, CRC(593b2678) SHA1(3b24b59a21386a4688502c5f0a2dd4eb0ec92544) )
	ROM_LOAD16_BYTE( "71047-09.u65", 0x000001, 0x080000, CRC(08ea8d91) SHA1(5d2672f6c96fbbe9d80bd6539c1400b62745892a) )
	ROM_LOAD16_BYTE( "71047-12.u62", 0x100000, 0x080000, CRC(76a77ab2) SHA1(0cf2f293defc23c807556ff92ea99f963fafed40) )
	ROM_LOAD16_BYTE( "71047-10.u63", 0x100001, 0x080000, CRC(7c79faf9) SHA1(40c1420eeae355efa628bbcfd69e0dd18d343fd9) )

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "71047-01.u73", 0x0000000, 0x200000, CRC(66662b08) SHA1(0cb683e5f85ffe21bd3367af4d3e48a484dbd4c3) )
	ROM_LOAD( "71047-02.u74", 0x0200000, 0x100000, CRC(6b00dc0c) SHA1(6af8ceed72d13f9979175c0d907a4a8c127ca1ad) )
	ROM_LOAD( "71047-03.u76", 0x0300000, 0x200000, CRC(00fcd6c2) SHA1(61d13cbafbc0fd6ff62cd08aa88591ed0fd0b182) )
	ROM_LOAD( "71047-04.u77", 0x0500000, 0x100000, CRC(d9e710d1) SHA1(063459a247f9ff81cb558802e9943b3ea8a2ea3a) )
	ROM_LOAD( "71047-05.u75", 0x0600000, 0x200000, CRC(10848193) SHA1(40b7ebb6011dc703bbf620cd22cd678c10ec67a4) )
	ROM_LOAD( "71047-06.u88", 0x0800000, 0x100000, CRC(b8ac2942) SHA1(3e85e8f5669d469dd3114455248546d32a642315) )
	ROM_FILL(                 0x0900000, 0x300000, 0 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD16_BYTE( "71047-07.u59", 0x000000, 0x200000, CRC(d9828b62) SHA1(f66a388d7a00b3a45d386671c061f5b840453451) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD16_BYTE( "71047-08.u60", 0x000000, 0x200000, CRC(30ebff6d) SHA1(53824c1fc37e22b545fd68b59722f7968f0ca1e2) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND2, 0x000000, 0x000000, 0x400000 )
ROM_END


/***************************************************************************

							Vasara / Vasara 2
Visco Games, 2000 / 2001
(info from the vasara 2 board but vasara should be the same)

This is a sub board that fits any standard SSV main board.

There's very little on the ROM board. Just 2x 27C040 EPROMs,
4x 64Mbit SOP44 MASK ROMs, 3x 16Mbit SOP44 MASK ROMs,
some logic and 2 PALs near the PROG & DATA ROMs.

The actual ROM PCB has the capability to accept SOP44 and
TSOP48 type 1 SMT ROMs though many of the positions are unpopulated.
It's likely this same ROM board could be used for other Visco games.

ROM PCB (PCB Number: SSV_SUB)

File Name     Labeled as        Loc. Printed*      ROM Type
-----------------------------------------------------------------------
prg-h.u31     PRG-H U31         U31  PRG H       | 27C040
prg-l.u30     PRG-L U20         U30  PRG L      /
s1.u37        C DAT VASARA-1    U37  S1         \
s0.u36        B DAT VASARA-1    U36  S0          | Surface Mounted 16Mbit SOP44 MASK ROMs
data.u34      A SND 1 VASARA-1  U34  DATA ROM   /
d0.u4         VASARA-2-D0       U4   D0.D1      \
c0.u3         VASARA-2-C0       U3   C0.C1       | Surface Mounted 64Mbit SOP44 MASK ROMs
b0.u2         VASARA-2-B0       U2   B0.B1       |
a0.u1         VASARA-2-A0       U1   A0.A1      /

2x GAL16V8D

Printed = Info silk-screened/printed on the actual PCB

****************************************************************************/

ROM_START( vasara )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "data.u34",  0x000000, 0x200000, CRC(7704cc7e) SHA1(62bb018b7f0c7ee67fee37de17bb22a73bb9e420) )
	ROM_LOAD16_BYTE( "prg-l.u30", 0x200000, 0x080000, CRC(f0547886) SHA1(6a3717f8b89575d3cb4c7d56dd9df5052faa3c7f) )
	ROM_RELOAD(                   0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "prg-h.u31", 0x200001, 0x080000, CRC(6a39bba9) SHA1(05ede167150307d7bf59037f264b1d140f6646da) )
	ROM_RELOAD(                   0x300001, 0x080000             )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "a0.u1", 0x0000000, 0x800000, CRC(673230a6) SHA1(a9d1a108c0737b709854bae199499577f5ae359e) )
	ROM_LOAD( "b0.u2", 0x0800000, 0x800000, CRC(31a2da7f) SHA1(5efec60affb2ed2b73a6694ac794d41375220609) )
	ROM_LOAD( "c0.u3", 0x1000000, 0x800000, CRC(d110dacf) SHA1(6f33bf6ce8c06f0b823b5478a56dc95095385181) )
	ROM_LOAD( "d0.u4", 0x1800000, 0x800000, CRC(82d0ca55) SHA1(5ac07df713504329fbc8e8b5374c04e53745230e) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "s0.u36", 0x000000, 0x200000, CRC(754fca02) SHA1(5b2810a36183e0d4f42f0fb4a09be033ad0db40d) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "s1.u37", 0x000000, 0x200000, CRC(5f303698) SHA1(bd6495f912aa9d761d245ef0a1566d9d7bdbb2ad) )
ROM_END

ROM_START( vasara2 )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "data.u34",  0x000000, 0x200000, CRC(493d0103) SHA1(fda68fb089328cabb3bbd52f8703b445a9509bf1) )
	ROM_LOAD16_BYTE( "prg-l.u30", 0x200000, 0x080000, CRC(40e6f5f6) SHA1(05fee4535ffe8403e86ba92a58e5f2d040489c8e) )
	ROM_RELOAD(                   0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "prg-h.u31", 0x200001, 0x080000, CRC(c958e146) SHA1(568878526cef76ac0ce4feeaa46e7039291e5f77) )
	ROM_RELOAD(                   0x300001, 0x080000             )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "a0.u1", 0x0000000, 0x800000, CRC(a6306c75) SHA1(bad715e53426a295d3571c025e0539d5f81ce5ab) )
	ROM_LOAD( "b0.u2", 0x0800000, 0x800000, CRC(227cbd9f) SHA1(a02787943b659508ce1589cdc7a372cc02826a10) )
	ROM_LOAD( "c0.u3", 0x1000000, 0x800000, CRC(54ede017) SHA1(4a7ff7ff8ec5843837016f35a588983b5ace06ff) )
	ROM_LOAD( "d0.u4", 0x1800000, 0x800000, CRC(4be8479d) SHA1(cbb5943dfae86f4d571459263199a63399dedc20) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "s0.u36", 0x000000, 0x200000, CRC(2b381b33) SHA1(b9dd13651e4b8d0b9e3bc4c592022f31ea634d19) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "s1.u37", 0x000000, 0x200000, CRC(11cd7098) SHA1(f75288b5c89df039dfb41d66bd275cda8605e75a) )
ROM_END

ROM_START( vasara2a )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "data.u34",     0x000000, 0x200000, CRC(493d0103) SHA1(fda68fb089328cabb3bbd52f8703b445a9509bf1) )
	ROM_LOAD16_BYTE( "basara-l.u30", 0x200000, 0x080000, CRC(fd88b068) SHA1(a86e3ffc870e6f6f7f18273428b24d938d6b9c3d) )
	ROM_RELOAD(                      0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "basara-h.u31", 0x200001, 0x080000, CRC(91d641e6) SHA1(4987d1771a90c9f1ce45c2dd2de5b2922d5d19c5) )
	ROM_RELOAD(                      0x300001, 0x080000             )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD( "a0.u1", 0x0000000, 0x800000, CRC(a6306c75) SHA1(bad715e53426a295d3571c025e0539d5f81ce5ab) )
	ROM_LOAD( "b0.u2", 0x0800000, 0x800000, CRC(227cbd9f) SHA1(a02787943b659508ce1589cdc7a372cc02826a10) )
	ROM_LOAD( "c0.u3", 0x1000000, 0x800000, CRC(54ede017) SHA1(4a7ff7ff8ec5843837016f35a588983b5ace06ff) )
	ROM_LOAD( "d0.u4", 0x1800000, 0x800000, CRC(4be8479d) SHA1(cbb5943dfae86f4d571459263199a63399dedc20) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "s0.u36", 0x000000, 0x200000, CRC(2b381b33) SHA1(b9dd13651e4b8d0b9e3bc4c592022f31ea634d19) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_ERASE | ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "s1.u37", 0x000000, 0x200000, CRC(11cd7098) SHA1(f75288b5c89df039dfb41d66bd275cda8605e75a) )
ROM_END

/***************************************************************************

Mobile Suit Gundam Final Shooting
Banpresto, 1995

Uses main board STA-0001B SYSTEM SSV

Game Board Layout
-----------------

VISCO (no other PCB numbers)
|---------------------------------------------------------|
|                 GAL16V8(2)                              |
|   VG004-01.U33            VG004-07.U39                  |
|      VG004-02.U34           VG004-08.U40                |
|        VG004-03.U35           VG004-10.U45      TC514400|
|          VG004-04.U36           VG004-09.U43            |
|            VG004-05.U37           VG004-11.U48  TC514400|
|D1            VG004-06.U38                               |
|                                       100MHz    TC514400|
|                                                         |
|                                |--------------| TC514400|
|      TC551001                  |              |         |
|P           |-------| GAL20V8   |              | TC514400|
| S2914      | SETA  | GAL20V8   |     SETA     |         |
|            |ST-0009| GAL20V8   |              | TC514400|
|            |       |           |   ST-0020    |         |
|            |-------|           |              | TC514400|
|      TC551001                  |              |         |
|           SSVV7.U16            |--------------| TC514400|
|               VG004-12.U4                               |
|C1                 VG004-13.U5  TC514260      VG004-14.U3|
| ADC0809             TC55257    GAL16V8(1) SSV2SET0.U1   |
|                     TC55257           SSVSET1.U2        |
|---------------------------------------------------------|
Notes:
      D1         - 4 pin connector for power
      P          - 10 pin connector
      C1         - 10 pin connector for analog controls
      ADC0809    - National Semiconductor ADC0809 8-Bit Microprocessor Compatible A/D Converter with 8-Channel Multiplexer (DIP28)
      S2914      - Seiko S2914 EEPROM (DIP8)
      TC551001   - Toshiba TC551001BFL-70L 128K x8 SRAM (SOP32)
      TC55257    - Toshiba TC55257N-70L 32K x8 SRAM (SOP28)
      TC514260   - Toshiba TC514260BJ-70 256K x16 DRAM (SOJ40)
      GAL16V8(1) - Lattice GAL16V8B stamped 'VG004-18' (DIP20)
      GAL16V8(2) - Lattice GAL16V8B stamped 'VG004-19' (DIP20)
      GAL20V8    - Lattice GAL20V8B all 3 stamped 'VG004-20' and have identical contents (DIP24)
      U16,U2,U1  - 4M MaskROM (DIP32)
      All other ROMs are 8M/16M MaskROM (DIP42)
      Custom Seta ICs -
                       ST-0009 (QFP176)
                       ST-0020 (QFP304, heatsinked)

****************************************************************************/

ROM_START( gdfs )
	ROM_REGION16_LE( 0x400000, REGION_USER1, 0 )		/* V60 Code */
	ROM_LOAD16_WORD( "vg004-14.u3",   0x000000, 0x100000, CRC(d88254df) SHA1(ccdfd42e4ce3941018f83e300da8bf7a5950f65c) )
	ROM_RELOAD(0x100000,0x100000)
	ROM_LOAD16_BYTE( "ssv2set0.u1",   0x200000, 0x080000, CRC(c23b9e2c) SHA1(9026e065252981fb403255ddc5782359c0088e8a) )
	ROM_RELOAD(0x300000,0x80000)
	ROM_LOAD16_BYTE( "ssv2set1.u2",   0x200001, 0x080000, CRC(d7d52570) SHA1(12e7531519a0a4331e409991265908fb518286ef) )
	ROM_RELOAD(0x300001,0x80000)

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vg004-09.u43", 0x000000, 0x200000, CRC(b7382cfa) SHA1(df735470181c16f8aac0e3be76e1ed53a32dbb9c) )
	ROM_LOAD( "vg004-10.u45", 0x200000, 0x200000, CRC(b3c6b1cb) SHA1(c601213e35d8dfd1244921da5c093f82145706d2) )
	ROM_LOAD( "vg004-11.u48", 0x400000, 0x200000, CRC(1491def1) SHA1(344043302c81b4118cac4f692375b8af7ea68570) )

	ROM_REGION( 0x1000000, REGION_GFX2, /*ROMREGION_DISPOSE*/ 0)	/* Zooming Sprites, read by a blitter */
	ROM_LOAD( "vg004-01.u33", 0x0000000, 0x200000, CRC(aa9a81c2) SHA1(a7d005f9be199e317aa4c6aed8a2ab322fe82119) )
	ROM_LOAD( "vg004-02.u34", 0x0200000, 0x200000, CRC(fa40ecb4) SHA1(0513f3b6879dc7d207646d949d6ddb7251f77bcc) )
	ROM_LOAD( "vg004-03.u35", 0x0400000, 0x200000, CRC(90004023) SHA1(041edb77b34e6677ac5b85ce542d87a9bb1baf31) )
	ROM_LOAD( "vg004-04.u36", 0x0600000, 0x200000, CRC(fdafd289) SHA1(3ff1969a176d13bfa68a48c9ed582f5789b1047f) )
	ROM_LOAD( "vg004-06.u38", 0x0a00000, 0x200000, CRC(3402325f) SHA1(7ea169c1f8b01a37bd7dbb4d486d38bdac62be5b) )
	ROM_LOAD( "vg004-05.u37", 0x0800000, 0x200000, CRC(9ae488b0) SHA1(7823cc689c588f3dbcafe9bdc94c094d6e9cd605) )
	ROM_LOAD( "vg004-07.u39", 0x0c00000, 0x200000, CRC(5e89fcf9) SHA1(db727ec8117e84c98037c756715e28fd5e39972a) )
	ROM_LOAD( "vg004-08.u40", 0x0e00000, 0x200000, CRC(6b1746dc) SHA1(35e5ee02975474985a4a611dcc439fc3050b7f94) )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )	/* Tilemap */
	ROM_LOAD( "ssvv7.u16",    0x0000000, 0x080000, CRC(f1c3ab6f) SHA1(b7f54f7ae60650fee7570aa4dd4266c629149673) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD16_BYTE( "vg004-12.u4", 0x000000, 0x200000, CRC(eb41a4ef) SHA1(f4d0844a3c00cf90faa59ae982744b7f0bcbe218) )
	ROM_LOAD16_BYTE( "vg004-13.u5", 0x000001, 0x200000, CRC(a4ed3977) SHA1(5843d56f69789e70ce0201a693ffae322b628459) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND3, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_COPY( REGION_SOUND1, 0x000000, 0x000000, 0x400000 )
ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

/*     year   rom       clone     machine   inputs    init      monitor manufacturer          title                                               flags*/

GAMEX( 1993,  keithlcy, 0,        keithlcy, keithlcy, keithlcy, ROT0,   "Visco",              "Dramatic Adventure Quiz Keith and Lucy (Japan)",     GAME_NO_COCKTAIL )
GAMEX( 1993,  srmp4,    0,        srmp4,    srmp4,    srmp4,    ROT0,   "Seta",               "Super Real Mahjong PIV (Japan)",                   GAME_NO_COCKTAIL )
GAMEX( 1993,  srmp4o,   srmp4,    srmp4,    srmp4,    srmp4,    ROT0,   "Seta",               "Super Real Mahjong PIV (Japan, older set)",        GAME_NO_COCKTAIL ) /* by the numbering of the program roms this should be older*/
GAMEX( 1993,  survarts, 0,        survarts, survarts, survarts, ROT0,   "American Sammy",     "Survival Arts (USA)",                              GAME_NO_COCKTAIL )
GAMEX( 1994,  dynagear, 0,        dynagear, dynagear, dynagear, ROT0,   "Sammy"         ,     "Dyna Gears",                                       GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994,  drifto94, 0,        drifto94, drifto94, drifto94, ROT0,   "Visco",              "Drift Out '94 - The Hard Order (Japan)",           GAME_NO_COCKTAIL )
GAMEX( 1995,  hypreact, 0,        hypreact, hypreact, hypreact, ROT0,   "Sammy",              "Mahjong Hyper Reaction (Japan)",                   GAME_NO_COCKTAIL | GAME_NOT_WORKING )
GAMEX( 1994,  twineag2, 0,        twineag2, twineag2, twineag2, ROT270, "Seta",               "Twin Eagle II - The Rescue Mission",               GAME_NO_COCKTAIL )
GAMEX( 1996,  janjans1, 0,        janjans1, janjans1, janjans1, ROT0,   "Visco",              "Lovely Pop Mahjong Jan Jan Shimasyo (Japan)",      GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996,  meosism,  0,        meosism,  meosism,  meosism,  ROT0,   "Sammy",              "Meosis Magic (Japan)",                             GAME_NO_COCKTAIL )
GAMEX( 1997,  mslider,  0,        mslider,  mslider,  mslider,  ROT0,   "Visco / Datt Japan", "Monster Slider (Japan)",                           GAME_NO_COCKTAIL )
GAMEX( 1996,  stmblade, 0,        stmblade, stmblade, stmblade, ROT270, "Visco",              "Storm Blade (US)",                                 GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997,  hypreac2, 0,        hypreac2, hypreac2, hypreac2, ROT0,   "Sammy",              "Mahjong Hyper Reaction 2 (Japan)",                 GAME_NO_COCKTAIL )
GAMEX( 1997,  koikois2, 0,        janjans1, koikois2, janjans1, ROT0,   "Visco",              "Koi Koi Shimasyo 2 - Super Real Hanafuda (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1997,  srmp7,    0,        srmp7,    srmp7,    srmp7,    ROT0,   "Seta",               "Super Real Mahjong P7 (Japan)",                    GAME_NO_COCKTAIL | GAME_IMPERFECT_SOUND )
GAMEX( 1998,  ryorioh,  0,        ryorioh,  ryorioh,  ryorioh,  ROT0,   "Visco",              "Gourmet Battle Quiz Ryohrioh CooKing (Japan)",     GAME_NO_COCKTAIL )
GAMEX( 1998,  sxyreact, 0,        sxyreact, sxyreact, sxyreact, ROT0,   "Sammy",              "Pachinko Sexy Reaction (Japan)",                   GAME_NO_COCKTAIL )
GAMEX( 1999,  cairblad, 0,        sxyreact, cairblad, sxyreact, ROT270, "Sammy",              "Change Air Blade (Japan)",                         GAME_NO_COCKTAIL )
GAMEX( 2000,  vasara,   0,        ryorioh,  vasara,   vasara,   ROT270, "Visco",              "Vasara",                                           GAME_NO_COCKTAIL )
GAMEX( 2001,  vasara2,  0,        ryorioh,  vasara2,  vasara2,  ROT270, "Visco",              "Vasara 2 (set 1)",                                 GAME_NO_COCKTAIL )
GAMEX( 2001,  vasara2a, vasara2,  ryorioh,  vasara2,  vasara2,  ROT270, "Visco",              "Vasara 2 (set 2)",                                 GAME_NO_COCKTAIL )
GAMEX( 1995,  ultrax,   0,        ultrax,   ultrax,   ultrax,   ROT270,	"Banpresto + Tsuburaya Prod.", "Ultra X Weapons - Ultra Keibitai",        GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995,  gdfs,     0,        gdfs,     gdfs,     gdfs,     ROT0,   "Banpresto",          "Mobile Suit Gundam Final Shooting (Japan)",                 GAME_NO_COCKTAIL ) /* title screen spells the title "Mobil" but standardized spelling is "Mobile" it also lists the company name as "Banprest" instead of "Banpresto" */
GAMEX( 1994,  eaglshot, 0,        eaglshot, eaglshot, eaglshot, ROT0,   "Sammy",              "Eagle Shot Golf",                                  GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994,  eaglshta, eaglshot, eaglshot, eaglshot, eaglshot, ROT0,   "Sammy",              "Eagle Shot Golf (alt)",                            GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS )


/*	Games not working at all:*/
GAMEX( 1997,  jsk,      0,        janjans1, janjans1, janjans1, ROT0,   "Visco",              "Joryuu Syougi Kyoushitsu (Japan)",                 GAME_NO_COCKTAIL | GAME_NOT_WORKING )
