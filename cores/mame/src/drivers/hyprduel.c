/***************************************************************************

Hyper Duel
(c)1993 Technosoft
TEC442-A(Japan)

TMP68000N-10 x2
YM2151,YM3012
OKI M6295
OSC:  4.0000MHz, 20.0000MHz, 26.6660MHz
Imagetek Inc 14220 071

--
driver by Eisuke Watanabe
based on driver from drivers/metro.c by Luca Elia
spthx to kikur,Cha,teioh,kokkyu,teruchu,aya,sgo

Note:
sub68k is performing not only processing of sound but assistance of main68k.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define RASTER_LINES 262
#define FIRST_VISIBLE_LINE 0
#define LAST_VISIBLE_LINE 223

/* Variables defined in vidhrdw: */
extern data16_t *hyprduel_videoregs;
extern data16_t *hyprduel_screenctrl;
extern data16_t *hyprduel_tiletable;
extern size_t hyprduel_tiletable_size;
extern data16_t *hyprduel_vram_0, *hyprduel_vram_1, *hyprduel_vram_2;
extern data16_t *hyprduel_window;
extern data16_t hyprduel_scrollx[3][RASTER_LINES+1];
extern data16_t hyprduel_scrolly[3][RASTER_LINES+1];

/* Functions defined in vidhrdw: */
WRITE16_HANDLER( hyprduel_paletteram_w );
WRITE16_HANDLER( hyprduel_window_w );
WRITE16_HANDLER( hyprduel_vram_0_w );
WRITE16_HANDLER( hyprduel_vram_1_w );
WRITE16_HANDLER( hyprduel_vram_2_w );

VIDEO_START( hyprduel_14220 );
VIDEO_UPDATE( hyprduel );

/***************************************************************************
								Interrupts
***************************************************************************/

static int blitter_bit;
static int requested_int;
static data16_t *hypr_irq_enable;
static int subcpu_resetline;

static int rastersplit;

static data16_t *hypr_sharedram1;
static data16_t *hypr_sharedram2;
static data16_t *hypr_sub_sharedram1_1;
static data16_t *hypr_sub_sharedram1_2;
static data16_t *hypr_sub_sharedram2_1;
static data16_t *hypr_sub_sharedram2_2;


static WRITE16_HANDLER( hypr_sharedram1_w )
{
	COMBINE_DATA(&hypr_sharedram1[offset]);
	COMBINE_DATA(&hypr_sub_sharedram1_1[offset]);
	if (offset < 0x4000/2)
		COMBINE_DATA(&hypr_sub_sharedram1_2[offset]);
}

static WRITE16_HANDLER( hypr_sharedram2_w )
{
	COMBINE_DATA(&hypr_sharedram2[offset]);
	COMBINE_DATA(&hypr_sub_sharedram2_1[offset]);
	if ((offset >= 0x4000/2) && (offset < 0x7fff/2))
		COMBINE_DATA(&hypr_sub_sharedram2_2[offset-0x4000/2]);
}

static void update_irq_state(void)
{
	int irq = requested_int & ~*hypr_irq_enable;

	cpu_set_irq_line(0, 3, (irq & 0x02) ? ASSERT_LINE : CLEAR_LINE);
}

static READ16_HANDLER( hyprduel_irq_cause_r )
{
	return requested_int;
}

static WRITE16_HANDLER( hyprduel_irq_cause_w )
{
	if (ACCESSING_LSB)
	{
		if (data == 0x02)
			requested_int &= ~(0x02 & ~*hypr_irq_enable);
		else
			requested_int &= ~(data & *hypr_irq_enable);

		update_irq_state();
	}
}

static WRITE16_HANDLER( hypr_subcpu_control_w )
{
	int pc = activecpu_get_pc();

	if (data & 0x01)
	{
		if (!subcpu_resetline)
		{
			if (pc != 0x95f2)
			{
				cpu_set_reset_line(1, ASSERT_LINE);
				subcpu_resetline = 1;
			} else {
				cpu_set_halt_line(1, ASSERT_LINE);
				subcpu_resetline = -1;
			}
		}
	} else {
		if (subcpu_resetline == 1 && (data != 0x0c))
		{
			cpu_set_reset_line(1, CLEAR_LINE);
			subcpu_resetline = 0;
			if (pc == 0xbb0 || pc == 0x9d30 || pc == 0xb19c)
				cpu_spinuntil_time(TIME_IN_USEC(15000));		/* sync semaphore */
		}
		else if (subcpu_resetline == -1)
		{
			cpu_set_halt_line(1, CLEAR_LINE);
			subcpu_resetline = 0;
		}
	}
}

static WRITE16_HANDLER( hypr_scrollreg_w )
{
	int i;

	if (offset & 0x01)
	{
		for (i = rastersplit; i < RASTER_LINES; i++)
			hyprduel_scrollx[offset>>1][i] = data;
	} else {
		for (i = rastersplit; i < RASTER_LINES; i++)
			hyprduel_scrolly[offset>>1][i] = data;
	}
}

static WRITE16_HANDLER( hypr_scrollreg_init_w )
{
	int i;

	for (i = 0; i < RASTER_LINES; i++)
	{
		hyprduel_scrollx[0][i] = data;
		hyprduel_scrollx[1][i] = data;
		hyprduel_scrollx[2][i] = data;
		hyprduel_scrolly[0][i] = data;
		hyprduel_scrolly[1][i] = data;
		hyprduel_scrolly[2][i] = data;
	}
}

static void vblank_end_callback(int param)
{
	requested_int &= ~param;
	cpu_set_irq_line(1, 2, HOLD_LINE);
}

INTERRUPT_GEN( hyprduel_interrupt )
{
	int line = RASTER_LINES - cpu_getiloops();

	if (line == RASTER_LINES)
	{
		requested_int |= 0x01;		/* vblank */
		requested_int |= 0x20;
		cpu_set_irq_line(0, 2, HOLD_LINE);
		cpu_set_irq_line(1, 1, HOLD_LINE);
		timer_set(TIME_IN_USEC(DEFAULT_REAL_60HZ_VBLANK_DURATION), 0x20, vblank_end_callback);
		rastersplit = 0;
	} else {
		requested_int |= 0x12;		/* hsync */
		rastersplit = line + 1;
	}

	update_irq_state();
}

MACHINE_INIT( hyprduel )
{
	/* start with cpu2 halted */
	cpu_set_reset_line(1, ASSERT_LINE);
	subcpu_resetline = 1;
}


/***************************************************************************
								Banked ROM access
***************************************************************************/

/*
	The main CPU has access to the ROMs that hold the graphics through
	a banked window of 64k. Those ROMs also usually store the tables for
	the virtual tiles set. The tile codes to be written to the tilemap
	memory to render the backgrounds are also stored here, in a format
	that the blitter can readily use (which is a form of compression)
*/

static data16_t *hyprduel_rombank;

READ16_HANDLER( hyprduel_bankedrom_r )
{
	const int region = REGION_GFX1;

	data8_t *ROM = memory_region( region );
	size_t  len  = memory_region_length( region );

	offset = offset * 2 + 0x10000 * (*hyprduel_rombank);

	if ( offset < len )	return ((ROM[offset+0]<<8)+ROM[offset+1])^0xffff;
	else				return 0xffff;
}

/***************************************************************************

									Blitter

	[ Registers ]

		Offset:		Value:

		0.l			Destination Tilemap      (1,2,3)
		4.l			Blitter Data Address     (byte offset into the gfx ROMs)
		8.l			Destination Address << 7 (byte offset into the tilemap)

		The Blitter reads a byte and looks at the most significative
		bits for the opcode, while the remaining bits define a value
		(usually how many bytes to write). The opcode byte may be
		followed by a number of other bytes:

			76------			Opcode
			--543210			N
			(at most N+1 bytes follow)


		The blitter is designed to write every other byte (e.g. it
		writes a byte and skips the next). Hence 2 blits are needed
		to fill a tilemap (first even, then odd addresses)

	[ Opcodes ]

			0		Copy the following N+1 bytes. If the whole byte
					is $00:	stop and generate an IRQ

			1		Fill N+1 bytes with a sequence, starting with
					the  value in the following byte

			2		Fill N+1 bytes with the value in the following
					byte

			3		Skip N+1 bytes. If the whole byte is $C0:
					skip to the next row of the tilemap (+0x200 bytes)
					but preserve the column passed at the start of the
					blit (destination address % 0x200)

***************************************************************************/

data16_t *hyprduel_blitter_regs;

void hyprduel_blit_done(int param)
{
	requested_int |= 1 << blitter_bit;
	update_irq_state();
}

static INLINE int blt_read(const data8_t *ROM, const int offs)
{
	return ROM[offs] ^ 0xff;
}

static INLINE void blt_write(const int tmap, const offs_t offs, const data16_t data, const data16_t mask)
{
	switch( tmap )
	{
		case 1:	hyprduel_vram_0_w(offs,data,mask);	break;
		case 2:	hyprduel_vram_1_w(offs,data,mask);	break;
		case 3:	hyprduel_vram_2_w(offs,data,mask);	break;
	}
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter %X] %04X <- %04X & %04X\n",activecpu_get_pc(),tmap,offs,data,mask);*/
}


static WRITE16_HANDLER( hyprduel_blitter_w )
{
	COMBINE_DATA( &hyprduel_blitter_regs[offset] );

	if (offset == 0xC/2)
	{
		const int region = REGION_GFX1;

		data8_t *src	=	memory_region(region);
		size_t  src_len	=	memory_region_length(region);

		UINT32 tmap		=	(hyprduel_blitter_regs[ 0x00 / 2 ] << 16 ) +
							 hyprduel_blitter_regs[ 0x02 / 2 ];
		UINT32 src_offs	=	(hyprduel_blitter_regs[ 0x04 / 2 ] << 16 ) +
							 hyprduel_blitter_regs[ 0x06 / 2 ];
		UINT32 dst_offs	=	(hyprduel_blitter_regs[ 0x08 / 2 ] << 16 ) +
							 hyprduel_blitter_regs[ 0x0a / 2 ];

		int shift			=	(dst_offs & 0x80) ? 0 : 8;
		data16_t mask		=	(dst_offs & 0x80) ? 0xff00 : 0x00ff;

/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter regs %08X, %08X, %08X\n",activecpu_get_pc(),tmap,src_offs,dst_offs);*/

		dst_offs >>= 7+1;
		switch( tmap )
		{
			case 1:
			case 2:
			case 3:
				break;
			default:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter unknown destination: %08X\n",activecpu_get_pc(),tmap);
				return;
		}

		while (1)
		{
			data16_t b1,b2,count;

			src_offs %= src_len;
			b1 = blt_read(src,src_offs);
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter opcode %02X at %06X\n",activecpu_get_pc(),b1,src_offs);*/
			src_offs++;

			count = ((~b1) & 0x3f) + 1;

			switch( (b1 & 0xc0) >> 6 )
			{
				case 0:

					/* Stop and Generate an IRQ. We can't generate it now
					   both because it's unlikely that the blitter is so
					   fast and because some games (e.g. lastfort) need to
					   complete the blitter irq service routine before doing
					   another blit. */
					if (b1 == 0)
					{
						timer_set(TIME_IN_USEC(500),0,hyprduel_blit_done);
						return;
					}

					/* Copy */
					while (count--)
					{
						src_offs %= src_len;
						b2 = blt_read(src,src_offs) << shift;
						src_offs++;

						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
					}
					break;


				case 1:

					/* Fill with an increasing value */
					src_offs %= src_len;
					b2 = blt_read(src,src_offs);
					src_offs++;

					while (count--)
					{
						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2<<shift,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
						b2++;
					}
					break;


				case 2:

					/* Fill with a fixed value */
					src_offs %= src_len;
					b2 = blt_read(src,src_offs) << shift;
					src_offs++;

					while (count--)
					{
						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
					}
					break;


				case 3:

					/* Skip to the next line ?? */
					if (b1 == 0xC0)
					{
						dst_offs +=   0x100;
						dst_offs &= ~(0x100-1);
						dst_offs |=  (0x100-1) & (hyprduel_blitter_regs[ 0x0a / 2 ] >> (7+1));
					}
					else
					{
						dst_offs += count;
					}
					break;


				default:
					log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter unknown opcode %02X at %06X\n",activecpu_get_pc(),b1,src_offs-1);
					return;
			}

		}
	}

}

/***************************************************************************
								Memory Maps
***************************************************************************/

static MEMORY_READ16_START( hyprduel_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x400000, 0x41ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x420000, 0x43ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x440000, 0x45ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x460000, 0x46ffff, hyprduel_bankedrom_r	},	/* Banked ROM*/
	{ 0x470000, 0x473fff, MRA16_RAM				},	/* Palette*/
	{ 0x474000, 0x474fff, MRA16_RAM				},	/* Sprites*/
	{ 0x475000, 0x477fff, MRA16_RAM				},	/* (only used memory test)*/
	{ 0x478000, 0x4787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x4788a2, 0x4788a3, hyprduel_irq_cause_r	},	/* IRQ Cause*/
	{ 0xc00000, 0xc07fff, MRA16_RAM				},	/* (sound driver controls)*/
	{ 0xe00000, 0xe00001, input_port_0_word_r	},	/* Inputs*/
	{ 0xe00002, 0xe00003, input_port_1_word_r	},	/**/
	{ 0xe00004, 0xe00005, input_port_2_word_r	},	/**/
	{ 0xe00006, 0xe00007, input_port_3_word_r	},	/**/
	{ 0xfe0000, 0xffffff, MRA16_RAM				},
MEMORY_END

static MEMORY_WRITE16_START( hyprduel_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM 					},
	{ 0x400000, 0x41ffff, hyprduel_vram_0_w, &hyprduel_vram_0	},	/* Layer 0*/
	{ 0x420000, 0x43ffff, hyprduel_vram_1_w, &hyprduel_vram_1	},	/* Layer 1*/
	{ 0x440000, 0x45ffff, hyprduel_vram_2_w, &hyprduel_vram_2	},	/* Layer 2*/
	{ 0x470000, 0x473fff, hyprduel_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x474000, 0x474fff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites*/
	{ 0x475000, 0x477fff, MWA16_RAM 					},
	{ 0x478000, 0x4787ff, MWA16_RAM, &hyprduel_tiletable, &hyprduel_tiletable_size	},	/* Tiles Set*/
	{ 0x478840, 0x47884d, hyprduel_blitter_w, &hyprduel_blitter_regs	},	/* Tiles Blitter*/
	{ 0x478860, 0x47886b, hyprduel_window_w, &hyprduel_window	},	/* Tilemap Window*/
	{ 0x478870, 0x47887b, hypr_scrollreg_w				},	/* Scroll Regs*/
	{ 0x47887c, 0x47887d, hypr_scrollreg_init_w			},	/* scroll regs all sets*/
	{ 0x478880, 0x478881, MWA16_NOP						},
	{ 0x478890, 0x478891, MWA16_NOP						},
	{ 0x4788a0, 0x4788a1, MWA16_NOP						},
	{ 0x4788a2, 0x4788a3, hyprduel_irq_cause_w			},	/* IRQ Acknowledge*/
	{ 0x4788a4, 0x4788a5, MWA16_RAM, &hypr_irq_enable	},	/* IRQ Enable*/
	{ 0x4788aa, 0x4788ab, MWA16_RAM, &hyprduel_rombank		},	/* Rom Bank*/
	{ 0x4788ac, 0x4788ad, MWA16_RAM, &hyprduel_screenctrl	},	/* Screen Control*/
	{ 0x479700, 0x479713, MWA16_RAM, &hyprduel_videoregs	},	/* Video Registers*/
	{ 0x800000, 0x800001, hypr_subcpu_control_w			},
	{ 0xc00000, 0xc07fff, MWA16_RAM, &hypr_sharedram1	},
	{ 0xe00000, 0xe00001, MWA16_NOP						},
	{ 0xfe0000, 0xffffff, MWA16_RAM, &hypr_sharedram2	},
MEMORY_END

static MEMORY_READ16_START( hyprduel_readmem2 )
	{ 0x000000, 0x003fff, MRA16_RAM						},
	{ 0x004000, 0x007fff, MRA16_RAM						},
	{ 0x400002, 0x400003, YM2151_status_port_0_lsb_r	},
	{ 0x400004, 0x400005, OKIM6295_status_0_lsb_r		},
	{ 0xc00000, 0xc07fff, MRA16_RAM						},
	{ 0xfe0000, 0xffffff, MRA16_RAM						},
MEMORY_END

static MEMORY_WRITE16_START( hyprduel_writemem2 )
	{ 0x000000, 0x003fff, MWA16_RAM, &hypr_sub_sharedram1_2	},	/* shadow ($c00000 - $c03fff : vector, write ok)*/
	{ 0x004000, 0x007fff, MWA16_RAM, &hypr_sub_sharedram2_2	},	/* shadow ($fe4000 - $fe7fff : read only)*/
	{ 0x400000, 0x400001, YM2151_register_port_0_lsb_w	},
	{ 0x400002, 0x400003, YM2151_data_port_0_lsb_w		},
	{ 0x400004, 0x400005, OKIM6295_data_0_lsb_w			},
	{ 0x800000, 0x800001, MWA16_NOP						},
	{ 0xc00000, 0xc07fff, MWA16_RAM, &hypr_sub_sharedram1_1	},	/* (sound driver)*/
	{ 0xfe0000, 0xffffff, MWA16_RAM, &hypr_sub_sharedram2_1	},
MEMORY_END

/***************************************************************************
								Input Ports
***************************************************************************/

#define JOY_LSB(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \

#define JOY_MSB(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \


INPUT_PORTS_START( hyprduel )
	PORT_START
	PORT_BITX(    0x8000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x4000, 0x0000, "Show Warning" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x3fff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Start Up Mode" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT(     0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x1000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_BIT(     0xc000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	JOY_LSB(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)
	JOY_MSB(2, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START
	PORT_BIT_IMPULSE(  0x0001, IP_ACTIVE_LOW,  IPT_COIN1, 2 )
	PORT_BIT_IMPULSE(  0x0002, IP_ACTIVE_LOW,  IPT_COIN2, 2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0xffc0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************
							Graphics Layouts
***************************************************************************/

/* 8x8x4 tiles */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 4*8 },	/* line modulo */
	32*8		/* char modulo */
};

/* 8x8x8 tiles for later games */
static struct GfxLayout layout_8x8x8h =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	32*8		/* char modulo (half char step) */
};

static struct GfxDecodeInfo gfxdecodeinfo_14220[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4,    0x0, 0x200 }, /* [0] 4 Bit Tiles*/
	{ REGION_GFX1, 0, &layout_8x8x8h,   0x0,  0x20 }, /* [1] 8 Bit Tiles*/
	{ -1 }
};

/***************************************************************************
							Sound Communication
***************************************************************************/

static void sound_irq(int state)
{
	cpu_set_irq_line(1, 1, HOLD_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ 0 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 4000000/16/16 },	/* 15625 Hz */
	{ REGION_SOUND1 },
	{ 57 }
};

/***************************************************************************
								Machine Drivers
***************************************************************************/

static MACHINE_DRIVER_START( hyprduel )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)		/* 10MHz */
	MDRV_CPU_MEMORY(hyprduel_readmem,hyprduel_writemem)
	MDRV_CPU_VBLANK_INT(hyprduel_interrupt,RASTER_LINES)

	MDRV_CPU_ADD(M68000,20000000/2)		/* 10MHz */
	MDRV_CPU_MEMORY(hyprduel_readmem2,hyprduel_writemem2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(hyprduel)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, FIRST_VISIBLE_LINE, LAST_VISIBLE_LINE)
	MDRV_GFXDECODE(gfxdecodeinfo_14220)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(hyprduel_14220)
	MDRV_VIDEO_UPDATE(hyprduel)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


/***************************************************************************
								ROMs Loading
***************************************************************************/

static DRIVER_INIT( hyprduel )
{
	int i;
	data8_t *ROM = memory_region(REGION_GFX1);

	/*
	  Tiles can be either 4-bit or 8-bit, and both depths can be used at the same
	  time. The transparent pen is the last one, that is 15 or 255. To make
	  tilemap.c handle that, we invert gfx data so the transparent pen becomes 0
	  for both tile depths.
	*/
	for (i = 0;i < memory_region_length(REGION_GFX1);i++)
		ROM[i] ^= 0xff;

/*	ROM[(0x174b9*0x20)+0x1f] |= 0x0e; I */
/*	ROM[(0x174e9*0x20)+0x1f] |= 0x0e;   */

	install_mem_write16_handler(0, 0xc00000, 0xc07fff, hypr_sharedram1_w);
	install_mem_write16_handler(1, 0xc00000, 0xc07fff, hypr_sharedram1_w);
	install_mem_write16_handler(1, 0x000000, 0x003fff, hypr_sharedram1_w);

	install_mem_write16_handler(0, 0xfe0000, 0xffffff, hypr_sharedram2_w);
	install_mem_write16_handler(1, 0xfe0000, 0xffffff, hypr_sharedram2_w);

	requested_int = 0x00;
	blitter_bit = 2;
	*hypr_irq_enable = 0xff;
}


ROM_START( hyprduel )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "hd_prg2.rom", 0x000000, 0x40000, CRC(c7402722) SHA1(e385676cdcee65a3ddf07791d82a1fe83ba1b3e2) )
	ROM_LOAD16_BYTE( "hd_prg1.rom", 0x000001, 0x40000, CRC(d8297c2b) SHA1(2e23c5b1784d0a465c0c0dc3ca28505689a8b16c) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Prg + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "hyper-1.4", 0x000000, 0x100000, CRC(4b3b2d3c) SHA1(5e9e8ec853f71aeff3910b93dadbaeae2b61717b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "hyper-2.6", 0x000002, 0x100000, CRC(dc230116) SHA1(a3c447657d8499764f52c81382961f425c56037b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "hyper-3.3", 0x000004, 0x100000, CRC(2d770dd0) SHA1(27f9e7f67e96210d3710ab4f940c5d7ae13f8bbf) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "hyper-4.5", 0x000006, 0x100000, CRC(f88c6d33) SHA1(277b56df40a17d7dd9f1071b0d498635a5b783cd) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "97.11", 0x00000, 0x40000, CRC(bf3f8574) SHA1(9e743f05e53256c886d43e1f0c43d7417134b9b3) )
ROM_END

ROM_START( hyprdelj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "u24a.10", 0x000000, 0x40000, CRC(2458f91d) SHA1(c75c7bccc84738e29b35667793491a1213aea1da) )
	ROM_LOAD16_BYTE( "u23a.9",  0x000001, 0x40000, CRC(98aedfca) SHA1(42028e57ac79473cde683be2100b953ff3b2b345) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Prg + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "hyper-1.4", 0x000000, 0x100000, CRC(4b3b2d3c) SHA1(5e9e8ec853f71aeff3910b93dadbaeae2b61717b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "hyper-2.6", 0x000002, 0x100000, CRC(dc230116) SHA1(a3c447657d8499764f52c81382961f425c56037b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "hyper-3.3", 0x000004, 0x100000, CRC(2d770dd0) SHA1(27f9e7f67e96210d3710ab4f940c5d7ae13f8bbf) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "hyper-4.5", 0x000006, 0x100000, CRC(f88c6d33) SHA1(277b56df40a17d7dd9f1071b0d498635a5b783cd) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "97.11", 0x00000, 0x40000, CRC(bf3f8574) SHA1(9e743f05e53256c886d43e1f0c43d7417134b9b3) )
ROM_END


GAME( 1993, hyprduel, 0,        hyprduel, hyprduel, hyprduel, ROT0, "Technosoft", "Hyper Duel (World)" )
GAME( 1993, hyprdelj, hyprduel, hyprduel, hyprduel, hyprduel, ROT0, "Technosoft", "Hyper Duel (Japan)" )
