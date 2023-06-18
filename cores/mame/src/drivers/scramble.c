/***************************************************************************

 Scramble hardware


Interesting tidbit:

There is a bug in Amidars and Triple Punch. Look at the loop at 0x2715.
It expects DE to be saved during the call to 0x2726, but it can be destroyed,
causing the loop to read all kinds of bogus memory locations.


To Do:

- Mariner has discrete sound circuits connected to the 8910's output ports


Notes:

- While Atlantis has a cabinet switch, it doesn't use the 2nd player controls
  in cocktail mode.

***************************************************************************/

#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "machine/8255ppi.h"
#include "galaxian.h"



static MEMORY_READ_START( scramble_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },	/* mirror */
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x7800, 0x7800, watchdog_reset_r },
	{ 0x8100, 0x8103, ppi8255_0_r },
	{ 0x8110, 0x8113, ppi8255_0_r },  /* mirror for Frog */
	{ 0x8200, 0x8203, ppi8255_1_r },
MEMORY_END

static MEMORY_WRITE_START( scramble_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x4c00, 0x4fff, galaxian_videoram_w },	/* mirror address */
	{ 0x5000, 0x503f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6801, 0x6801, galaxian_nmi_enable_w },
	{ 0x6802, 0x6802, galaxian_coin_counter_w },
	{ 0x6804, 0x6804, galaxian_stars_enable_w },
	{ 0x6806, 0x6806, galaxian_flip_screen_x_w },
	{ 0x6807, 0x6807, galaxian_flip_screen_y_w },
	{ 0x8100, 0x8103, ppi8255_0_w },
	{ 0x8200, 0x8203, ppi8255_1_w },
MEMORY_END


static MEMORY_READ_START( explorer_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },	/* mirror */
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x5100, 0x51ff, MRA_NOP },	/* test mode mirror? */
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8000, 0x8000, input_port_0_r },
	{ 0x8001, 0x8001, input_port_1_r },
	{ 0x8002, 0x8002, input_port_2_r },
	{ 0x8003, 0x8003, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( explorer_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x4c00, 0x4fff, galaxian_videoram_w },	/* mirror address */
	{ 0x5000, 0x503f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x5100, 0x51ff, MWA_NOP },	/* test mode mirror? */
	{ 0x6800, 0x6800, MWA_NOP },
	{ 0x6801, 0x6801, galaxian_nmi_enable_w },
	{ 0x6802, 0x6802, galaxian_coin_counter_w },
	{ 0x6803, 0x6803, MWA_NOP },
	{ 0x6804, 0x6804, galaxian_stars_enable_w },
	{ 0x6806, 0x6806, galaxian_flip_screen_x_w },
	{ 0x6807, 0x6807, galaxian_flip_screen_y_w },
	{ 0x7000, 0x7000, MWA_NOP },
	{ 0x8000, 0x8000, soundlatch_w },
	{ 0x9000, 0x9000, explorer_sh_irqtrigger_w },
MEMORY_END


static MEMORY_READ_START( ckongs_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6bff, MRA_RAM },
	{ 0x7000, 0x7003, ppi8255_0_r },
	{ 0x7800, 0x7803, ppi8255_1_r },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9800, 0x98ff, MRA_RAM },
	{ 0xb000, 0xb000, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( ckongs_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6bff, MWA_RAM },
	{ 0x7000, 0x7003, ppi8255_0_w },
	{ 0x7800, 0x7803, ppi8255_1_w },
	{ 0x9000, 0x93ff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x9800, 0x983f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x9860, 0x987f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x9880, 0x98ff, MWA_RAM },
	{ 0xa801, 0xa801, galaxian_nmi_enable_w },
	{ 0xa802, 0xa802, galaxian_coin_counter_w },
	{ 0xa806, 0xa806, galaxian_flip_screen_x_w },
	{ 0xa807, 0xa807, galaxian_flip_screen_y_w },
MEMORY_END


static MEMORY_READ_START( mars_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x7000, 0x7000, MRA_NOP },
	{ 0x8100, 0x810f, mars_ppi8255_0_r },
	{ 0x8200, 0x820f, mars_ppi8255_1_r },
MEMORY_END

static MEMORY_WRITE_START( mars_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x5000, 0x503f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6800, 0x6800, galaxian_coin_counter_1_w },
	{ 0x6801, 0x6801, galaxian_stars_enable_w },
	{ 0x6802, 0x6802, galaxian_nmi_enable_w },
	{ 0x6808, 0x6808, galaxian_coin_counter_0_w },
	{ 0x6809, 0x6809, galaxian_flip_screen_x_w },
	{ 0x680b, 0x680b, galaxian_flip_screen_y_w },
	{ 0x8100, 0x810f, mars_ppi8255_0_w },
	{ 0x8200, 0x820f, mars_ppi8255_1_w },
MEMORY_END


static MEMORY_READ_START( newsin7_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8200, 0x820f, mars_ppi8255_1_r },
	{ 0xa000, 0xafff, MRA_ROM },
	{ 0xc100, 0xc10f, mars_ppi8255_0_r },
MEMORY_END

static MEMORY_WRITE_START( newsin7_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x5000, 0x503f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6800, 0x6800, galaxian_coin_counter_1_w },
	{ 0x6801, 0x6801, galaxian_stars_enable_w },
	{ 0x6802, 0x6802, galaxian_nmi_enable_w },
	{ 0x6808, 0x6808, galaxian_coin_counter_0_w },
	{ 0x6809, 0x6809, galaxian_flip_screen_x_w },
	{ 0x680b, 0x680b, galaxian_flip_screen_y_w },
	{ 0x8200, 0x820f, mars_ppi8255_1_w },
	{ 0xa000, 0xafff, MWA_ROM },
	{ 0xc100, 0xc10f, mars_ppi8255_0_w },
MEMORY_END


static MEMORY_WRITE_START( mrkougar_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x4c00, 0x4fff, galaxian_videoram_w },	/* mirror address */
	{ 0x5000, 0x503f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6800, 0x6800, galaxian_coin_counter_1_w },
	{ 0x6801, 0x6801, galaxian_nmi_enable_w },
	{ 0x6808, 0x6808, galaxian_coin_counter_0_w },
	{ 0x6809, 0x6809, galaxian_flip_screen_x_w },
	{ 0x680b, 0x680b, galaxian_flip_screen_y_w },
	{ 0x8100, 0x810f, mars_ppi8255_0_w },
	{ 0x8200, 0x820f, mars_ppi8255_1_w },
MEMORY_END


static MEMORY_READ_START( hotshock_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x8000, 0x8000, input_port_0_r },
	{ 0x8001, 0x8001, input_port_1_r },
	{ 0x8002, 0x8002, input_port_2_r },
	{ 0x8003, 0x8003, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( hotshock_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x5000, 0x503f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6000, 0x6000, galaxian_coin_counter_2_w },
	{ 0x6002, 0x6002, galaxian_coin_counter_1_w },
	{ 0x6004, 0x6004, hotshock_flip_screen_w },
	{ 0x6005, 0x6005, galaxian_coin_counter_0_w },
	{ 0x6006, 0x6006, galaxian_gfxbank_w },
	{ 0x6801, 0x6801, galaxian_nmi_enable_w },
	{ 0x7000, 0x7000, watchdog_reset_w },
	{ 0x8000, 0x8000, soundlatch_w },
	{ 0x9000, 0x9000, hotshock_sh_irqtrigger_w },
MEMORY_END


static MEMORY_READ_START( hunchbks_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x1210, 0x1213, ppi8255_1_r },
	{ 0x1400, 0x14ff, MRA_RAM },
	{ 0x1500, 0x1503, ppi8255_0_r },
	{ 0x1680, 0x1680, watchdog_reset_r },
	{ 0x1800, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, MRA_ROM },
	{ 0x3000, 0x3fff, hunchbks_mirror_r },
	{ 0x4000, 0x4fff, MRA_ROM },
	{ 0x5000, 0x5fff, hunchbks_mirror_r },
	{ 0x6000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, hunchbks_mirror_r },
MEMORY_END

static MEMORY_WRITE_START( hunchbks_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x1210, 0x1213, ppi8255_1_w },
	{ 0x1400, 0x143f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x1440, 0x145f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x1460, 0x147f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x1480, 0x14ff, MWA_RAM },
	{ 0x1500, 0x1503, ppi8255_0_w },
	{ 0x1606, 0x1606, galaxian_flip_screen_x_w },
	{ 0x1607, 0x1607, galaxian_flip_screen_y_w },
	{ 0x1800, 0x1bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x1c00, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2fff, MWA_ROM },
	{ 0x3000, 0x3fff, hunchbks_mirror_w },
	{ 0x4000, 0x4fff, MWA_ROM },
	{ 0x5000, 0x5fff, hunchbks_mirror_w },
	{ 0x6000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, hunchbks_mirror_w },
MEMORY_END


static MEMORY_READ_START( sfx_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },	/* mirror */
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x7000, 0x7fff, MRA_ROM },
	{ 0x8100, 0x8103, ppi8255_0_r },
	{ 0x8200, 0x8203, ppi8255_1_r },
	{ 0xc000, 0xefff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sfx_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x4c00, 0x4fff, galaxian_videoram_w },	/* mirror address */
	{ 0x5000, 0x503f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6800, 0x6800, scramble_background_red_w },
	{ 0x6801, 0x6801, galaxian_nmi_enable_w },
	{ 0x6802, 0x6802, galaxian_coin_counter_w },
	{ 0x6803, 0x6803, scramble_background_blue_w },
	{ 0x6804, 0x6804, galaxian_stars_enable_w },
	{ 0x6805, 0x6805, scramble_background_green_w },
	{ 0x6806, 0x6806, galaxian_flip_screen_x_w },
	{ 0x6807, 0x6807, galaxian_flip_screen_y_w },
	{ 0x7000, 0x7fff, MWA_ROM },
	{ 0x8100, 0x8103, ppi8255_0_w },
	{ 0x8200, 0x8203, ppi8255_1_w },
	{ 0xc000, 0xefff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sfx_sample_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sfx_sample_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sfx_sample_readport )
	{ 0x04, 0x07, ppi8255_2_r },
MEMORY_END

static PORT_WRITE_START( sfx_sample_writeport )
	{ 0x04, 0x07, ppi8255_2_w },
	{ 0x10, 0x10, DAC_0_signed_data_w },
MEMORY_END


static MEMORY_READ_START( mimonscr_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, galaxian_videoram_r },	/* mirror address?, probably not */
	{ 0x4400, 0x4bff, MRA_RAM },
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8100, 0x8103, ppi8255_0_r },
	{ 0x8200, 0x8203, ppi8255_1_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mimonscr_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, galaxian_videoram_w },	/* mirror address?, probably not */
	{ 0x4400, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, galaxian_videoram_w, &galaxian_videoram },
	{ 0x5000, 0x503f, galaxian_attributesram_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6801, 0x6801, galaxian_nmi_enable_w },
	{ 0x6800, 0x6802, galaxian_gfxbank_w },
	{ 0x6806, 0x6806, galaxian_flip_screen_x_w },
	{ 0x6807, 0x6807, galaxian_flip_screen_y_w },
	{ 0x8100, 0x8103, ppi8255_0_w },
	{ 0x8200, 0x8203, ppi8255_1_w },
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END


static PORT_READ_START( triplep_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x02, 0x02, triplep_pip_r },
	{ 0x03, 0x03, triplep_pap_r },
PORT_END

static PORT_WRITE_START( triplep_writeport )
	{ 0x00, 0x00, AY8910_write_port_0_w },
	{ 0x01, 0x01, AY8910_control_port_0_w },
PORT_END


static PORT_READ_START( hotshock_sound_readport )
	{ 0x20, 0x20, AY8910_read_port_0_r },
	{ 0x40, 0x40, AY8910_read_port_1_r },
PORT_END

static PORT_WRITE_START( hotshock_sound_writeport )
	{ 0x10, 0x10, AY8910_control_port_0_w },
	{ 0x20, 0x20, AY8910_write_port_0_w },
	{ 0x40, 0x40, AY8910_write_port_1_w },
	{ 0x80, 0x80, AY8910_control_port_1_w },
PORT_END


static PORT_READ_START( hunchbks_readport )
    { S2650_SENSE_PORT, S2650_SENSE_PORT, input_port_3_r },
PORT_END



INPUT_PORTS_START( scramble )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1  B 2/1  C 1/1" )
	PORT_DIPSETTING(    0x02, "A 1/2  B 1/1  C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3  B 3/1  C 1/3" )
	PORT_DIPSETTING(    0x06, "A 1/4  B 4/1  C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	/* protection bit */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* protection bit */
INPUT_PORTS_END

INPUT_PORTS_START( explorer )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* pressing this disables the coins */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START	/* IN3 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x0c, "15000" )
	PORT_DIPSETTING(    0x14, "20000" )
	PORT_DIPSETTING(    0x1c, "25000" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x10, "70000" )
	PORT_DIPSETTING(    0x18, "90000" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )
INPUT_PORTS_END

INPUT_PORTS_START( atlantis )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x0e, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 1/3  B 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/6  B 1/1" )
	PORT_DIPSETTING(    0x04, "A 1/99 B 1/99")
	/* all the other combos give 99 credits */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( theend )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )		/* output bits */
INPUT_PORTS_END

INPUT_PORTS_START( froggers )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* read - function unknown */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot2 - unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 2/1 C 2/1" )
	PORT_DIPSETTING(    0x04, "A 2/1 B 1/3 C 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1 C 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 1/6 C 1/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( amidars )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( triplep )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 1/2 B 1/1 C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1 C 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 2/1 C 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1 C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ckongs )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	/* the coinage dip switch is spread across bits 0/1 of port 1 and bit 3 of port 2. */
	/* To handle that, we swap bits 0/1 of port 1 and bits 1/2 of port 2 - this is handled */
	/* by ckongs_input_port_N_r() */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	/* the coinage dip switch is spread across bits 0/1 of port 1 and bit 3 of port 2. */
	/* To handle that, we swap bits 0/1 of port 1 and bits 1/2 of port 2 - this is handled */
	/* by ckongs_input_port_N_r() */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
INPUT_PORTS_END

INPUT_PORTS_START( mars )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP     | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY | IPF_PLAYER2 )  /* this also control cocktail mode */
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP     | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )

	PORT_START	/* IN3 */
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( devilfsh )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x01, "15000" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( newsin7 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, " A 1C/1C  B 2C/1C" )
	PORT_DIPSETTING(    0x01, " A 1C/3C  B 3C/1C" )
	PORT_DIPSETTING(    0x02, " A 1C/2C  B 1C/1C" )
	PORT_DIPSETTING(    0x00, " A 1C/4C  B 4C/1C" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )  /* difficulty? */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( mrkougar )
	PORT_START	/* IN0 */
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hotshock )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* pressing this disables the coins */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START	/* IN3 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Language" )
	PORT_DIPSETTING(    0x04, "English" )
	PORT_DIPSETTING(    0x00, "Italian" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "75000" )
	PORT_DIPSETTING(    0x08, "150000" )
	PORT_DIPSETTING(    0x10, "200000" )
	PORT_DIPSETTING(    0x18, "None" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( hunchbks )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x06, "80000" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */

    PORT_START /* Sense */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( cavelon )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* force UR controls in CK mode? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
INPUT_PORTS_END

INPUT_PORTS_START( sfx )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Fire" left*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* "Fire" right*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* "Fire" left*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* "Fire" right*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused */
INPUT_PORTS_END

/* Same as 'mimonkey' (scobra.c driver) */
INPUT_PORTS_START( mimonscr )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BITX(    0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )   /* used, something to do with the bullets */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout devilfsh_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 2*256*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout devilfsh_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 2*64*16*16 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout newsin7_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 2*2*256*8*8, 0, 2*256*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout newsin7_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 2*2*64*16*16, 0, 2*64*16*16 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout mrkougar_charlayout =
{
	8,8,
	256,
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static struct GfxLayout mrkougar_spritelayout =
{
	16,16,
	64,
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3,
	  24*8+0, 24*8+1, 24*8+2, 24*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static struct GfxLayout sfx_charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,4),
	2,	/* 2 bits per pixel */
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout sfx_spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,4),
	2,	/* 2 bits per pixel */
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo devilfsh_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &devilfsh_charlayout,   0, 8 },
	{ REGION_GFX1, 0x0800, &devilfsh_spritelayout, 0, 8 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo newsin7_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &newsin7_charlayout,   0, 4 },
	{ REGION_GFX1, 0x0800, &newsin7_spritelayout, 0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo mrkougar_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &mrkougar_charlayout,   0, 8 },
	{ REGION_GFX1, 0x0000, &mrkougar_spritelayout, 0, 8 },
	{ -1 } /* end of array */
};

struct GfxDecodeInfo sfx_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0800, &sfx_charlayout,    0, 8 },
	{ REGION_GFX1, 0x0000, &sfx_spritelayout,  0, 8 },
	{ -1 } /* end of array */
};

static struct AY8910interface triplep_ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 MHz */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct AY8910interface sfx_ay8910_interface =
{
	2,	/* 2 chips */
	14318000/8,	/* 1.78975 MHz */
	/* Ant Eater clips if the volume is set higher than this */
	{ MIXERG(16,MIXER_GAIN_2x,MIXER_PAN_CENTER), MIXERG(16,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ 0, soundlatch_r },
	{ 0, scramble_portB_r },
	{ soundlatch2_w, 0 },
	{ sfx_sh_irqtrigger_w, 0 }
};

struct AY8910interface explorer_ay8910_interface =
{
	2,	/* 2 chips */
	14318000/8,	/* 1.78975 MHz */
	/* Ant Eater clips if the volume is set higher than this */
	{ MIXERG(16,MIXER_GAIN_2x,MIXER_PAN_CENTER), MIXERG(16,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ scramble_portB_r, soundlatch_r },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

static struct DACinterface sfx_dac_interface =
{
	1,	/* 1 channel */
	{ 100 },
};


static MACHINE_DRIVER_START( scramble )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(scramble_readmem,scramble_writemem)

	MDRV_CPU_ADD_TAG("audio", Z80, 14318000/8)	/* 1.78975 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(scobra_sound_readmem,scobra_sound_writemem)
	MDRV_CPU_PORTS(scobra_sound_readport,scobra_sound_writeport)

	MDRV_MACHINE_INIT(scramble)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+1)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */

	MDRV_PALETTE_INIT(scramble)
	MDRV_VIDEO_START(scramble)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("8910", AY8910, scobra_ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( explorer )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(explorer_readmem,explorer_writemem)

	MDRV_MACHINE_INIT(explorer)

	/* sound hardware */
	MDRV_SOUND_REPLACE("8910", AY8910, explorer_ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( theend )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */

	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(theend)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( froggers )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_MEMORY(frogger_sound_readmem,frogger_sound_writemem)
	MDRV_CPU_PORTS(frogger_sound_readport,frogger_sound_writeport)

	/* video hardware */
	MDRV_PALETTE_INIT(frogger)
	MDRV_VIDEO_START(froggers)

	/* sound hardware */
	MDRV_SOUND_REPLACE("8910", AY8910, frogger_ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mars )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mars_readmem,mars_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( devilfsh )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mars_readmem,mars_writemem)

	/* video hardware */
	MDRV_GFXDECODE(devilfsh_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( newsin7 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(newsin7_readmem,newsin7_writemem)

	/* video hardware */
	MDRV_GFXDECODE(newsin7_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(newsin7)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mrkougar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mars_readmem,mrkougar_writemem)

	/* video hardware */
	MDRV_GFXDECODE(mrkougar_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mrkougb )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mars_readmem,mrkougar_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ckongs )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(ckongs_readmem,ckongs_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(ckongs)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hotshock )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(hotshock_readmem,hotshock_writemem)

	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_PORTS(hotshock_sound_readport,hotshock_sound_writeport)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(pisces)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cavelon )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(ckongs)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sfx )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(sfx_readmem,sfx_writemem)

	MDRV_CPU_ADD(Z80, 14318000/8)	/* 1.78975 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sfx_sample_readmem,sfx_sample_writemem)
	MDRV_CPU_PORTS(sfx_sample_readport,sfx_sample_writeport)

	MDRV_MACHINE_INIT(sfx)

	/* video hardware */
	MDRV_VISIBLE_AREA(2*8, 30*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(32+64+2+8)	/* 32 for characters, 64 for stars, 2 for bullets, 8 for background */
	MDRV_GFXDECODE(sfx_gfxdecodeinfo)
	MDRV_PALETTE_INIT(turtles)
	MDRV_VIDEO_START(sfx)

	/* sound hardware */
	MDRV_SOUND_REPLACE("8910", AY8910, sfx_ay8910_interface)
	MDRV_SOUND_ADD(DAC, sfx_dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mimonscr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mimonscr_readmem,mimonscr_writemem)

	/* video hardware */
	MDRV_VIDEO_START(mimonkey)
MACHINE_DRIVER_END

/* Triple Punch and Mariner are different - only one CPU, one 8910 */
static MACHINE_DRIVER_START( triplep )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(triplep_readport,triplep_writeport)

	MDRV_CPU_REMOVE("audio")

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets */

	MDRV_PALETTE_INIT(galaxian)

	/* sound hardware */
	MDRV_SOUND_REPLACE("8910", AY8910, triplep_ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mariner )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(triplep)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+16)	/* 32 for characters, 64 for stars, 2 for bullets, 16 for background */

	MDRV_PALETTE_INIT(mariner)
	MDRV_VIDEO_START(mariner)
MACHINE_DRIVER_END

/* Hunchback replaces the Z80 with a S2650 CPU */
static MACHINE_DRIVER_START( hunchbks )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_REPLACE("main", S2650, 18432000/6)
	MDRV_CPU_MEMORY(hunchbks_readmem,hunchbks_writemem)
	MDRV_CPU_PORTS(hunchbks_readport,0)
	MDRV_CPU_VBLANK_INT(hunchbks_vh_interrupt,1)

	MDRV_VBLANK_DURATION(2500)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets */

	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scramble )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2d.k",         0x0000, 0x0800, CRC(ea35ccaa) SHA1(1dcb375987fe21e0483c27d485c405de53848d61) )
	ROM_LOAD( "2e.k",         0x0800, 0x0800, CRC(e7bba1b3) SHA1(240877576045fddcc9ff01d97dc78139454ac4f1) )
	ROM_LOAD( "2f.k",         0x1000, 0x0800, CRC(12d7fc3e) SHA1(a84d191c7be8700f630a83ddad798be9e83b5d55) )
	ROM_LOAD( "2h.k",         0x1800, 0x0800, CRC(b59360eb) SHA1(5d155808c19dcf2e14aa8e29c0ee41a6d3d3c43a) )
	ROM_LOAD( "2j.k",         0x2000, 0x0800, CRC(4919a91c) SHA1(9cb5861c61e4783e5fbaa3869d51195f127b1129) )
	ROM_LOAD( "2l.k",         0x2800, 0x0800, CRC(26a4547b) SHA1(67c0fa81729370631647b5d78bb5a61433facd7f) )
	ROM_LOAD( "2m.k",         0x3000, 0x0800, CRC(0bb49470) SHA1(05a6fe3010c2136284ca76352dac147797c79778) )
	ROM_LOAD( "2p.k",         0x3800, 0x0800, CRC(6a5740e5) SHA1(e3b09141cee26857d626412e9d1a0e759469b97a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f.k",         0x0000, 0x0800, CRC(4708845b) SHA1(a8b1ad19a95a9d35050a2ab7194cc96fc5afcdc9) )
	ROM_LOAD( "5h.k",         0x0800, 0x0800, CRC(11fd2887) SHA1(69844e48bb4d372cac7ae83c953df573c7ecbb7f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( scrambls )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2d",           0x0000, 0x0800, CRC(b89207a1) SHA1(5422df979e82bcc73df49f50515fe76c126c037b) )
	ROM_LOAD( "2e",           0x0800, 0x0800, CRC(e9b4b9eb) SHA1(a8ee9ddfadf5e9accedfaf81da757a88a2e55a0a) )
	ROM_LOAD( "2f",           0x1000, 0x0800, CRC(a1f14f4c) SHA1(3eae2b3e4596505a8afb5c5cfb108e823c2c4319) )
	ROM_LOAD( "2h",           0x1800, 0x0800, CRC(591bc0d9) SHA1(170f9e92f0a3bee04407be27210b4fa825367688) )
	ROM_LOAD( "2j",           0x2000, 0x0800, CRC(22f11b6b) SHA1(e426ef6a7444a39a34d59799973b07d11b89f372) )
	ROM_LOAD( "2l",           0x2800, 0x0800, CRC(705ffe49) SHA1(174df3f281068c767344f751daace646360e26d6) )
	ROM_LOAD( "2m",           0x3000, 0x0800, CRC(ea26c35c) SHA1(a2f3380982d93a022f46756f974fd16c4cd617de) )
	ROM_LOAD( "2p",           0x3800, 0x0800, CRC(94d8f5e3) SHA1(f3a9c4d1d91836476fcad87ea0d243dde7171e0a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, CRC(5f30311a) SHA1(d64134089bebd995b3a1a089411e180c8c29f32d) )
	ROM_LOAD( "5h",           0x0800, 0x0800, CRC(516e029e) SHA1(81b44eb1ce43cebde87f0a41ade2e7eb291af78d) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( explorer )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "10l.bin",      0x0000, 0x1000, CRC(d5adf626) SHA1(f362322f780c13cee73697f9158a8ca8aa943a2e) ) 
	ROM_LOAD( "9l.bin",       0x1000, 0x1000, CRC(48e32788) SHA1(7a98848d2ed8ba5b2da28c014226109af7cc9287) ) 
	ROM_LOAD( "8l.bin",       0x2000, 0x1000, CRC(c0dbdbde) SHA1(eac7444246bdf80f97962031bf900ce09b28c8b5) ) 
	ROM_LOAD( "7l.bin",       0x3000, 0x1000, CRC(9b30d227) SHA1(22764e0a2a5ce7abe862e42c84abaaf25949575f) ) 

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "3f.bin",		  0x0000, 0x1000, CRC(9faf18cf) SHA1(1b6c65472d639753cc39031750f85efe1d31ae5e) ) 
	ROM_LOAD( "4b.bin",       0x1000, 0x0800, CRC(e910b5c3) SHA1(228e8d36dd1ac8a00a396df74b80aa6616997028) ) 

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f.k",         0x0000, 0x0800, CRC(4708845b) SHA1(a8b1ad19a95a9d35050a2ab7194cc96fc5afcdc9) )
	ROM_LOAD( "5h.k",         0x0800, 0x0800, CRC(11fd2887) SHA1(69844e48bb4d372cac7ae83c953df573c7ecbb7f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( atlantis )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x0800, CRC(0e485b9a) SHA1(976f1d6f4552fbee134359a776b5688588824cbb) )
	ROM_LOAD( "2e",           0x0800, 0x0800, CRC(c1640513) SHA1(a0dfb34f401330b16e9e4d66ec4b49d120499606) )
	ROM_LOAD( "2f",           0x1000, 0x0800, CRC(eec265ee) SHA1(29b6cf6b93220414eb58cddeba591dc8813c4935) )
	ROM_LOAD( "2h",           0x1800, 0x0800, CRC(a5d2e442) SHA1(e535d1a501ebd861ad62da70b87215fb7c23de1d) )
	ROM_LOAD( "2j",           0x2000, 0x0800, CRC(45f7cf34) SHA1(d1e0e0be6dec377b684625bdfdc5a3a8af847492) )
	ROM_LOAD( "2l",           0x2800, 0x0800, CRC(f335b96b) SHA1(17daa6d9bc916081f3c6cbdfe5b4960177dc7c9b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, CRC(57f9c6b9) SHA1(ad0d09a6611998d093d676a9c9fe9e32b10f643e) )
	ROM_LOAD( "5h",           0x0800, 0x0800, CRC(e989f325) SHA1(947aee915779687deae040aeef9e9aee680aaebf) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( atlants2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "rom1",         0x0000, 0x0800, CRC(ad348089) SHA1(3548b94192c451c0126e7aaecefa7137ae074cd3) )
	ROM_LOAD( "rom2",         0x0800, 0x0800, CRC(caa705d1) SHA1(b4aefbea21fa9608e1dae2a09ae0d31270eb8c78) )
	ROM_LOAD( "rom3",         0x1000, 0x0800, CRC(e420641d) SHA1(103e7590f5acbac6991d665495f933c3a68da1c8) )
	ROM_LOAD( "rom4",         0x1800, 0x0800, CRC(04792d90) SHA1(cb477e4b8e4538def01c10b0348f8f8e3a2a9500) )
	ROM_LOAD( "rom5",         0x2000, 0x0800, CRC(6eaf510d) SHA1(381cb07ad47d1c7ad2bb547c1fd18b1db8958aff) )
	ROM_LOAD( "rom6",         0x2800, 0x0800, CRC(b297bd4b) SHA1(0c48da41d9cf2a3456df5b1e8bf27fa641bc643b) )
	ROM_LOAD( "rom7",         0x3000, 0x0800, CRC(a50bf8d5) SHA1(5bca98e1c0838d27ec66bf4b906877977b212b6d) )
	ROM_LOAD( "rom8",         0x3800, 0x0800, CRC(d2c5c984) SHA1(a9432f9aff8a2f5ca1d347443efc008a177d8ae0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom9",         0x0000, 0x0800, CRC(55cd5acd) SHA1(b3e2ce71d4e48255d44cd451ee015a7234a108c8) )
	ROM_LOAD( "rom10",        0x0800, 0x0800, CRC(72e773b8) SHA1(6ce178df3bd6a4177c68761572a13a56d222c48f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( theend )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic13_1t.bin",  0x0000, 0x0800, CRC(93e555ba) SHA1(f684927cecabfbd7544f7549a6152c0a6a436019) )
	ROM_LOAD( "ic14_2t.bin",  0x0800, 0x0800, CRC(2de7ad27) SHA1(caf369fde632652a0a5fb11d3605f0d2386d297a) )
	ROM_LOAD( "ic15_3t.bin",  0x1000, 0x0800, CRC(035f750b) SHA1(5f70518e5dbfca0ba12ba4dc4f357ce8e6b27bc8) )
	ROM_LOAD( "ic16_4t.bin",  0x1800, 0x0800, CRC(61286b5c) SHA1(14464aa5284aecc9c6046e464ab3d13da89d8dda) )
	ROM_LOAD( "ic17_5t.bin",  0x2000, 0x0800, CRC(434a8f68) SHA1(3c8c099c7865997d475c096f1b1c93d88ab21543) )
	ROM_LOAD( "ic18_6t.bin",  0x2800, 0x0800, CRC(dc4cc786) SHA1(3311361a1eb29715aa41d61fbb3563014bd9eeb1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic56_1.bin",   0x0000, 0x0800, CRC(7a141f29) SHA1(ca483943971c8fc7f5775a8a7cc6ddd331d48170) )
	ROM_LOAD( "ic55_2.bin",   0x0800, 0x0800, CRC(218497c1) SHA1(3e080621f2e83909a6f304a2d960a080bccbbdc2) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic30_2c.bin",  0x0000, 0x0800, CRC(68ccf7bf) SHA1(a8ea784a2660f855757ae0b30cb2a33ab6f2cd59) )
	ROM_LOAD( "ic31_1c.bin",  0x0800, 0x0800, CRC(4a48c999) SHA1(f1abcbfc3146a18dc3ff865e3ba278377a42a875) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, CRC(24652bc4) SHA1(d89575f3749c75dc963317fe451ffeffd9856e4d) )
ROM_END

ROM_START( theends )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic13",         0x0000, 0x0800, CRC(90e5ab14) SHA1(b926801ab1cc1e2787a76ced6c7cffd6fce753d4) )
	ROM_LOAD( "ic14",         0x0800, 0x0800, CRC(950f0a07) SHA1(bde9f3c6cf060dc6f5b7652287b94e04bed7bcf7) )
	ROM_LOAD( "ic15",         0x1000, 0x0800, CRC(6786bcf5) SHA1(7556d3dc51d6a112b6357b8a36df05fd1a4d1cc9) )
	ROM_LOAD( "ic16",         0x1800, 0x0800, CRC(380a0017) SHA1(3354eb328a32537f722fe8a0949ddcab6cf21eb8) )
	ROM_LOAD( "ic17",         0x2000, 0x0800, CRC(af067b7f) SHA1(855c6ddf29fbfea004c7143fe29064abf53801ad) )
	ROM_LOAD( "ic18",         0x2800, 0x0800, CRC(a0411b93) SHA1(d644968758a1b73d13e09b24d24bfec82276e8f4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic56",         0x0000, 0x0800, CRC(3b2c2f70) SHA1(bcccdacacfc9a3b5f1412dfba6bb0046d283bccc) )
	ROM_LOAD( "ic55",         0x0800, 0x0800, CRC(e0429e50) SHA1(27678fc3172cbca3ae1eae96e9d8a62561d5ce40) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic30",         0x0000, 0x0800, CRC(527fd384) SHA1(92a384899d5acd2c689f637da16a0e2d11a9d9c6) )
	ROM_LOAD( "ic31",         0x0800, 0x0800, CRC(af6d09b6) SHA1(f3ad51dc88aa58fd39195ead978b039e0b0b585c) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, CRC(24652bc4) SHA1(d89575f3749c75dc963317fe451ffeffd9856e4d) )
ROM_END

ROM_START( froggers )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "vid_d2.bin",   0x0000, 0x0800, CRC(c103066e) SHA1(8c2d4c825e9c4180fe70b0db18a547dc3ddc3c2c) )
	ROM_LOAD( "vid_e2.bin",   0x0800, 0x0800, CRC(f08bc094) SHA1(23ad1e57f244d6b63fd9640249dcb1eeafb8206e) )
	ROM_LOAD( "vid_f2.bin",   0x1000, 0x0800, CRC(637a2ff8) SHA1(e9b9fc692ca5d8deb9cd30d9d73ad25c8d8bafe1) )
	ROM_LOAD( "vid_h2.bin",   0x1800, 0x0800, CRC(04c027a5) SHA1(193550731513c02cad464661a1ceb230819ca70f) )
	ROM_LOAD( "vid_j2.bin",   0x2000, 0x0800, CRC(fbdfbe74) SHA1(48d5d1247d09eaea2a9a29f4ed6543d0411597aa) )
	ROM_LOAD( "vid_l2.bin",   0x2800, 0x0800, CRC(8a4389e1) SHA1(b2c74afb93927dac0d8bb24e02e0b2a069f2d3c8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, CRC(e8ab0256) SHA1(f090afcfacf5f13cdfa0dfda8e3feb868c6ce8bc) )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, CRC(7380a48f) SHA1(75582a94b696062cbdb66a4c5cf0bc0bb94f81ee) )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, CRC(31d7eb27) SHA1(2e1d34ae4da385fd7cac94707d25eeddf4604e1a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-1036.1k",  0x0000, 0x0800, CRC(658745f8) SHA1(e4e5c3e011c8a7233a36d29e10e08905873500aa) )
	ROM_LOAD( "frogger.607",  0x0800, 0x0800, CRC(05f7d883) SHA1(78831fd287da18928651a8adb7e578d291493eff) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "vid_e6.bin",   0x0000, 0x0020, CRC(0b878b54) SHA1(3667aca564ebfef5b88d7f74fabbd16dd23183b4) )
ROM_END

ROM_START( amidars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "am2d",         0x0000, 0x0800, CRC(24b79547) SHA1(eca735c6a35561a9a6ba8a20dca1e1c78ed073fc) )
	ROM_LOAD( "am2e",         0x0800, 0x0800, CRC(4c64161e) SHA1(5b2e49ff915295617671b13f15b566046a5dbc15) )
	ROM_LOAD( "am2f",         0x1000, 0x0800, CRC(b3987a72) SHA1(1d72e9ae3005029628c6f9beb6ca65afcb1f7893) )
	ROM_LOAD( "am2h",         0x1800, 0x0800, CRC(29873461) SHA1(7d0ee9a82f02163b4cc6a7097e88ae34e96ebf58) )
	ROM_LOAD( "am2j",         0x2000, 0x0800, CRC(0fdd54d8) SHA1(c32fdc8e292d91159e6c80c7033abea6404a4f2c) )
	ROM_LOAD( "am2l",         0x2800, 0x0800, CRC(5382f7ed) SHA1(425ec2c2caf404fc8ab13ee38d6567413022e1a1) )
	ROM_LOAD( "am2m",         0x3000, 0x0800, CRC(1d7109e9) SHA1(e0d24475547bbe5a94b45be6abefb84ad84d2534) )
	ROM_LOAD( "am2p",         0x3800, 0x0800, CRC(c9163ac6) SHA1(46d757180426b71c827d14a35824a248f2c787b6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "amidarus.5c",  0x0000, 0x1000, CRC(8ca7b750) SHA1(4f4c2915503b85abe141d717fd254ee10c9da99e) )
	ROM_LOAD( "amidarus.5d",  0x1000, 0x1000, CRC(9b5bdc0a) SHA1(84d953618c8bf510d23b42232a856ac55f1baff5) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2716.a6",      0x0000, 0x0800, CRC(2082ad0a) SHA1(c6014d9575e92adf09b0961c2158a779ebe940c4) )   /* Same graphics ROMs as Amigo */
	ROM_LOAD( "2716.a5",      0x0800, 0x0800, CRC(3029f94f) SHA1(3b432b42e79f8b0a7d65e197f373a04e3c92ff20) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "amidar.clr",   0x0000, 0x0020, CRC(f940dcc3) SHA1(1015e56f37c244a850a8f4bf0e36668f047fd46d) )
ROM_END

ROM_START( triplep )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "triplep.2g",   0x0000, 0x1000, CRC(c583a93d) SHA1(2bd4a02f945d64ef3ff814d0b8cbf32380d3f790) )
	ROM_LOAD( "triplep.2h",   0x1000, 0x1000, CRC(c03ddc49) SHA1(5a2fba848c4ddf2ef0bb0f00e93dbd88e939441a) )
	ROM_LOAD( "triplep.2k",   0x2000, 0x1000, CRC(e83ca6b5) SHA1(b16690cfe6fb45cf7b9a9cfa22822ba947c0e432) )
	ROM_LOAD( "triplep.2l",   0x3000, 0x1000, CRC(982cc3b9) SHA1(28bb08679126e5aa8bd0b8f387b881fe799fb009) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "triplep.5f",   0x0000, 0x0800, CRC(d51cbd6f) SHA1(c3766a69a4599e54b8d7fb893e45802ec8bf6713) )
	ROM_LOAD( "triplep.5h",   0x0800, 0x0800, CRC(f21c0059) SHA1(b1ba87f13908e3e662de8bf444f59bd5c2009720) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "tripprom.6e",  0x0000, 0x0020, CRC(624f75df) SHA1(0e9a7c48dd976af1dca1d5351236d4d5bf7a9dc8) )
ROM_END

ROM_START( knockout )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "knockout.2h",  0x0000, 0x1000, CRC(eaaa848e) SHA1(661026567db87206200ee610c3d5f5eb725aeec9) )
	ROM_LOAD( "knockout.2k",  0x1000, 0x1000, CRC(bc26d2c0) SHA1(b9934ddb2918f6c4123dafd07cc39ae31d7e28e9) )
	ROM_LOAD( "knockout.2l",  0x2000, 0x1000, CRC(02025c10) SHA1(16ffc7681d949172034b8c85dc72c1a528309abf) )
	ROM_LOAD( "knockout.2m",  0x3000, 0x1000, CRC(e9abc42b) SHA1(93b9c55a76e273b4709ee65870c0848a0d3db7cc) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "triplep.5f",   0x0000, 0x0800, CRC(d51cbd6f) SHA1(c3766a69a4599e54b8d7fb893e45802ec8bf6713) )
	ROM_LOAD( "triplep.5h",   0x0800, 0x0800, CRC(f21c0059) SHA1(b1ba87f13908e3e662de8bf444f59bd5c2009720) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "tripprom.6e",  0x0000, 0x0020, CRC(624f75df) SHA1(0e9a7c48dd976af1dca1d5351236d4d5bf7a9dc8) )
ROM_END

ROM_START( mariner )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "tp1.2h",       0x0000, 0x1000, CRC(dac1dfd0) SHA1(57b9106bb7452640544ba0ab2d2ba290cccb45f0) )
	ROM_LOAD( "tm2.2k",       0x1000, 0x1000, CRC(efe7ca28) SHA1(496f8eb2ebc9edeed5b19d87f437f23bbeb2a007) )
	ROM_LOAD( "tm3.2l",       0x2000, 0x1000, CRC(027881a6) SHA1(47953aa5140a157ade484341609d477510e8342b) )
	ROM_LOAD( "tm4.2m",       0x3000, 0x1000, CRC(a0fde7dc) SHA1(ea6700520b1bd31e6c6bfac6f067bbf652676eef) )
	ROM_LOAD( "tm5.2p",       0x6000, 0x0800, CRC(d7ebcb8e) SHA1(bddefdc5f04c2f940e08a6968fbd6f930d16b8e4) )
	ROM_CONTINUE(             0x5800, 0x0800             )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm8.5f",       0x0000, 0x1000, CRC(70ae611f) SHA1(2686dc6d3910bd58b290d6296f30c552686709f5) )
	ROM_LOAD( "tm9.5h",       0x1000, 0x1000, CRC(8e4e999e) SHA1(195e6896ca2f3175137d8c92777ba32c41e835d3) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t4.6e",        0x0000, 0x0020, CRC(ca42b6dd) SHA1(d1e224e788e3dcf57249e72f03f9fe3fd71e6c12) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "t6.6p",        0x0000, 0x0100, CRC(ad208ccc) SHA1(66a4122e46467344a7f3ddcc953a5f7f451411fa) )	/* background color prom */

	ROM_REGION( 0x0020, REGION_USER2, 0 )
	ROM_LOAD( "t5.7p",        0x0000, 0x0020, CRC(1bd88cff) SHA1(8d1620386ef654d99c51e489c822eeb2e8a4fe76) )	/* char banking and star placement */
ROM_END

ROM_START( 800fath )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "tu1.2h",       0x0000, 0x1000, CRC(5dd3d42f) SHA1(887403c385897044e1cb9709ab2b6ff5abdf9eb4) )
	ROM_LOAD( "tm2.2k",       0x1000, 0x1000, CRC(efe7ca28) SHA1(496f8eb2ebc9edeed5b19d87f437f23bbeb2a007) )
	ROM_LOAD( "tm3.2l",       0x2000, 0x1000, CRC(027881a6) SHA1(47953aa5140a157ade484341609d477510e8342b) )
	ROM_LOAD( "tm4.2m",       0x3000, 0x1000, CRC(a0fde7dc) SHA1(ea6700520b1bd31e6c6bfac6f067bbf652676eef) )
	ROM_LOAD( "tu5.2p",       0x6000, 0x0800, CRC(f864a8a6) SHA1(bd0c84284d13d099da4e139db7c9948a074d6774) )
	ROM_CONTINUE(             0x5800, 0x0800             )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm8.5f",       0x0000, 0x1000, CRC(70ae611f) SHA1(2686dc6d3910bd58b290d6296f30c552686709f5) )
	ROM_LOAD( "tm9.5h",       0x1000, 0x1000, CRC(8e4e999e) SHA1(195e6896ca2f3175137d8c92777ba32c41e835d3) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t4.6e",        0x0000, 0x0020, CRC(ca42b6dd) SHA1(d1e224e788e3dcf57249e72f03f9fe3fd71e6c12) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "t6.6p",        0x0000, 0x0100, CRC(ad208ccc) SHA1(66a4122e46467344a7f3ddcc953a5f7f451411fa) )	/* background color prom */

	ROM_REGION( 0x0020, REGION_USER2, 0 )
	ROM_LOAD( "t5.7p",        0x0000, 0x0020, CRC(1bd88cff) SHA1(8d1620386ef654d99c51e489c822eeb2e8a4fe76) )	/* char banking and star placement */
ROM_END

ROM_START( ckongs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "vid_2c.bin",   0x0000, 0x1000, CRC(49a8c234) SHA1(91d8da03a76094b6fed4bf1d9a3943dee72bf039) )
	ROM_LOAD( "vid_2e.bin",   0x1000, 0x1000, CRC(f1b667f1) SHA1(c09e0f3b70afd5a4b6ec47ac9237f278dff75783) )
	ROM_LOAD( "vid_2f.bin",   0x2000, 0x1000, CRC(b194b75d) SHA1(514b195dd02a7324e439dd63ae654af117e0c70d) )
	ROM_LOAD( "vid_2h.bin",   0x3000, 0x1000, CRC(2052ba8a) SHA1(e4200219d1a142a4aba8ef21ae1dd806400f4422) )
	ROM_LOAD( "vid_2j.bin",   0x4000, 0x1000, CRC(b377afd0) SHA1(8e42e7623a1749cea1c9861cd7dfa9b97571dc8b) )
	ROM_LOAD( "vid_2l.bin",   0x5000, 0x1000, CRC(fe65e691) SHA1(736fe70c9adc6d2c142fa876f1a1e3c6879eccd8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "turt_snd.5c",  0x0000, 0x1000, CRC(f0c30f9a) SHA1(5621f336e9be8acf986a34bbb8855ed5d45c28ef) )
	ROM_LOAD( "snd_5d.bin",   0x1000, 0x1000, CRC(892c9547) SHA1(c3ec98049b560eb0ddefdb1e1b2d551b418b9a1c) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_5f.bin",   0x0000, 0x1000, CRC(7866d2cb) SHA1(62dd8b80bc0459c7337d8a8cb83e53b999e7f4a9) )
	ROM_LOAD( "vid_5h.bin",   0x1000, 0x1000, CRC(7311a101) SHA1(49d54c8b94cae4ba81d7a7684eaa4e87815bb4da) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "vid_6e.bin",   0x0000, 0x0020, CRC(5039af97) SHA1(b1a5b32b8c944bf19d9d97aaf678726df003c194) )
ROM_END

ROM_START( mars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u26.3",        0x0000, 0x0800, CRC(2f88892c) SHA1(580c7b502321868f63d9e67286e63b5c5268827c) )
	ROM_LOAD( "u56.4",        0x0800, 0x0800, CRC(9e6bcbf7) SHA1(c3acdba073a1f3703776a7905867d81acf328f37) )
	ROM_LOAD( "u69.5",        0x1000, 0x0800, CRC(df496e6e) SHA1(b597e996ac797a239e4bc8f242f59c98a850723c) )
	ROM_LOAD( "u98.6",        0x1800, 0x0800, CRC(75f274bb) SHA1(2eb83ccc8404c69ab262bf680dce892c23c94f39) )
	ROM_LOAD( "u114.7",       0x2000, 0x0800, CRC(497fd8d0) SHA1(545aaf1d68ff727df356bbcf8ddd23df75b5ce97) )
	ROM_LOAD( "u133.8",       0x2800, 0x0800, CRC(3d4cd59f) SHA1(da96d96a40a896e1272700c50cc34f91ac9f7a23) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "u39.9",        0x0000, 0x0800, CRC(bb5968b9) SHA1(8bc57fd80da7aff294e12e991e9acf60c1ab2893) )
	ROM_LOAD( "u51.10",       0x0800, 0x0800, CRC(75fd7720) SHA1(3ee4ca0d85ffacf0388cada17581da0cdaaf83ef) )
	ROM_LOAD( "u78.11",       0x1000, 0x0800, CRC(72a492da) SHA1(a272f72378850f7ecf52498746c2015f6dad3ab9) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u72.1",        0x0000, 0x0800, CRC(279789d0) SHA1(3ccf39da252df8b3605efc26299831279d697dd8) )
	ROM_LOAD( "u101.2",       0x0800, 0x0800, CRC(c5dc627f) SHA1(529307238707d0676d1cae508f4eb66bbdd623d7) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( devilfsh )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u26.1",        0x0000, 0x0800, CRC(ec047d71) SHA1(c35555010fe239213e92946b65a54612d5a23399) )
	ROM_LOAD( "u56.2",        0x0800, 0x0800, CRC(0138ade9) SHA1(8c75572fa0a5d0665cc46b1c0080192bb0148df9) )
	ROM_LOAD( "u69.3",        0x1000, 0x0800, CRC(5dd0b3fc) SHA1(cf19193986920853d93e4ff38b70dcd46b42276b) )
	ROM_LOAD( "u98.4",        0x1800, 0x0800, CRC(ded0b745) SHA1(2a3c741f11d211b4ec8dfa2dd2b3ae0c0a2d9590) )
	ROM_LOAD( "u114.5",       0x2000, 0x0800, CRC(5fd40176) SHA1(536dd870057c48591f0bee468325c8780afb7026) )
	ROM_LOAD( "u133.6",       0x2800, 0x0800, CRC(03538336) SHA1(66cffcbc53e42c626880151a62721ef3e6bf90bc) )
	ROM_LOAD( "u143.7",       0x3000, 0x0800, CRC(64676081) SHA1(6ae628ad582680d6a825238b60d1195990dbb56f) )
	ROM_LOAD( "u163.8",       0x3800, 0x0800, CRC(bc3d6770) SHA1(ac8803520a668b1759acba23f06ba7a6c5792cbd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "u39.9",        0x0000, 0x0800, CRC(09987e2e) SHA1(6b0d8413a137726a67ceedeacfc0c48b058698f1) )
	ROM_LOAD( "u51.10",       0x0800, 0x0800, CRC(1e2b1471) SHA1(3ba33b26a5bff21b9ddc34e0a468db7b89361314) )
	ROM_LOAD( "u78.11",       0x1000, 0x0800, CRC(45279aaa) SHA1(20aca9bcfa1010311290cb3d8e4fb548182dcd4b) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u72.12",       0x0000, 0x1000, CRC(5406508e) SHA1(1d01a796c3ac22e3fddd1ec2103ef5bd8c409278) )
	ROM_LOAD( "u101.13",      0x1000, 0x1000, CRC(8c4018b6) SHA1(83c4f73a3e40fa6ecece38404fa5f64ab59d7b4e) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( newsin7 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "newsin.1",     0x0000, 0x1000, CRC(e6c23fe0) SHA1(b15c56ca7868be0f038c246f29ba54f41b5cb755) )
	ROM_LOAD( "newsin.2",     0x1000, 0x1000, CRC(3d477b5f) SHA1(9e22f7262077ce6b00b2efbba0e13dcec143d122) )
	ROM_LOAD( "newsin.3",     0x2000, 0x1000, CRC(7dfa9af0) SHA1(8cbbfff22a3c6429f7ab22d86c8d760b08871bac) )
	ROM_LOAD( "newsin.4",     0x3000, 0x1000, CRC(d1b0ba19) SHA1(66c128bc9b306aa6470de5d413f782ae17e78b14) )
	ROM_LOAD( "newsin.5",     0xa000, 0x1000, CRC(06275d59) SHA1(a5a5436c0b014af06181eeb044d8c4e3188414ea) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "newsin.13",    0x0000, 0x0800, CRC(d88489a2) SHA1(ab10fd4862129824301a0a847de298f1826aa03e) )
	ROM_LOAD( "newsin.12",    0x0800, 0x0800, CRC(b154a7af) SHA1(91ca28b2530f22786ff581e5710b40f0cf99f516) )
	ROM_LOAD( "newsin.11",    0x1000, 0x0800, CRC(7ade709b) SHA1(bda1401172139cd6e3e03424c56e4f59e5afebd5) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "newsin.7",     0x2000, 0x1000, CRC(6bc5d64f) SHA1(4f52224a4d5294a7487a1fc55eba13cf0b5fb6af) )
	ROM_LOAD( "newsin.8",     0x1000, 0x1000, CRC(0c5b895a) SHA1(994ad7f051b30a3045ffc08ac8d9d7092fbadef3) )
	ROM_LOAD( "newsin.9",     0x0000, 0x1000, CRC(6b87adff) SHA1(c0943832e498ab04978b11b163ba951d4a7e2e60) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "newsin.6",     0x0000, 0x0020, CRC(5cf2cd8d) SHA1(0c85737add75545ab11aaf64fe37c7bd078308c9) )
ROM_END

ROM_START( mrkougar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2732-7.bin",   0x0000, 0x1000, CRC(fd060ffb) SHA1(b3bee6fe879f13f3178bef3b2dff3041e698f061) )
	ROM_LOAD( "2732-6.bin",   0x1000, 0x1000, CRC(9e05d868) SHA1(802514f5de347913f0315b42b3689baa37030141) )
	ROM_LOAD( "2732-5.bin",   0x2000, 0x1000, CRC(cbc7c536) SHA1(b959c29bb7ab81ee123ba2f397eef1e32656b441) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "atw-6w-2.bin", 0x0000, 0x1000, CRC(af42a371) SHA1(edacbb29df34fdf400a5c726d851af1479a34c70) )
	ROM_LOAD( "atw-6y-3.bin", 0x1000, 0x1000, CRC(862b8902) SHA1(91dcbc634f7c7ed78dfbd0be5cf1e0631429cfbf) )
	ROM_LOAD( "atw-6z-4.bin", 0x2000, 0x1000, CRC(a0396cc8) SHA1(c8266b58b144a4bc564f3a2503d5b953c0ba6ca7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2732-1.bin",   0x0000, 0x1000, CRC(60ef1d43) SHA1(ab42fa98350051526fcc4bfe35ebed3d6daf424f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( mrkougr2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "atw-7l-7.bin", 0x0000, 0x1000, CRC(7b34b198) SHA1(c7793c49c5bd1360ef2d419bc4710b35f0a02760) )
	ROM_LOAD( "atw-7k-6.bin", 0x1000, 0x1000, CRC(fbca23c7) SHA1(da24a01d83174bad36072d4bf6764c5a3e242561) )
	ROM_LOAD( "atw-7h-5.bin", 0x2000, 0x1000, CRC(05b257a2) SHA1(728df1f1cb726d333818db8fedb27bf537be8a36) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "atw-6w-2.bin", 0x0000, 0x1000, CRC(af42a371) SHA1(edacbb29df34fdf400a5c726d851af1479a34c70) )
	ROM_LOAD( "atw-6y-3.bin", 0x1000, 0x1000, CRC(862b8902) SHA1(91dcbc634f7c7ed78dfbd0be5cf1e0631429cfbf) )
	ROM_LOAD( "atw-6z-4.bin", 0x2000, 0x1000, CRC(a0396cc8) SHA1(c8266b58b144a4bc564f3a2503d5b953c0ba6ca7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "atw-1h-1.bin", 0x0000, 0x1000, CRC(38fdfb63) SHA1(9fc4eafd6d106ffe35c179e59a234c706c489f8c) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "atw-prom.bin", 0x0000, 0x0020, CRC(c65db188) SHA1(90f0a5f22bb761693ab5895da08b20821e79ba65) )
ROM_END

ROM_START( mrkougb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "p01.bin",	  0x0000, 0x0800, CRC(dea0cde1) SHA1(aaf9c622b86d475a90f91628d033989e72dda361) )
	ROM_LOAD( "p02.bin",	  0x0800, 0x0800, CRC(c8017751) SHA1(021bd6a6efb90119767162a5847b4bbbc47f321e) )
	ROM_LOAD( "p03.bin",	  0x1000, 0x0800, CRC(b8921984) SHA1(1adccd2bad8f748995f844183cac487ad00dd71e) )
	ROM_LOAD( "p04.bin",	  0x1800, 0x0800, CRC(b3c9754c) SHA1(16a162a19079125fa01f49d90dbf8cd61b9b4833) )
	ROM_LOAD( "p05.bin",	  0x2000, 0x0800, CRC(8d94adbc) SHA1(ac5932c84864e08c6b7937ef20d5bdceb48e2d24) )
	ROM_LOAD( "p06.bin",	  0x2800, 0x0800, CRC(acc921ff) SHA1(f75158c62c6b9871ef05a6a97542469698100eb0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "atw-6w-2.bin", 0x0000, 0x1000, CRC(af42a371) SHA1(edacbb29df34fdf400a5c726d851af1479a34c70) )
	ROM_LOAD( "atw-6y-3.bin", 0x1000, 0x1000, CRC(862b8902) SHA1(91dcbc634f7c7ed78dfbd0be5cf1e0631429cfbf) )
	ROM_LOAD( "atw-6z-4.bin", 0x2000, 0x1000, CRC(a0396cc8) SHA1(c8266b58b144a4bc564f3a2503d5b953c0ba6ca7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g07.bin",      0x0000, 0x0800, CRC(0ecfd116) SHA1(0ea173c4f7f2613ef71ee5dcd52c4c6f640020b7) )
	ROM_LOAD( "g08.bin",      0x0800, 0x0800, CRC(00bfa3c6) SHA1(57a7fc48ec740b72baece96d50380dbbc826af77) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "atw-prom.bin", 0x0000, 0x0020, CRC(c65db188) SHA1(90f0a5f22bb761693ab5895da08b20821e79ba65) )
ROM_END

ROM_START( hotshock )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hotshock.l10", 0x0000, 0x1000, CRC(401078f7) SHA1(d4415a41eba1d3a2dcbb119f3136c177b02d1fb6) )
	ROM_LOAD( "hotshock.l9",  0x1000, 0x1000, CRC(af76c237) SHA1(bb54e1652a2d2e56731434ed85b40dab4aad91c9) )
	ROM_LOAD( "hotshock.l8",  0x2000, 0x1000, CRC(30486031) SHA1(bed06cb62afee6b000a0e21927559ac5d3538b38) )
	ROM_LOAD( "hotshock.l7",  0x3000, 0x1000, CRC(5bde9312) SHA1(d3ba06790c8210f41902bb8ad27a1e5abafccb33) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "hotshock.b3",  0x0000, 0x1000, CRC(0092f0e2) SHA1(d85b05370fa0ce4ba27fd331bb6a7fae067ce83b) )
	ROM_LOAD( "hotshock.b4",  0x1000, 0x1000, CRC(c2135a44) SHA1(809cf305b1f43f99f2248020c369fb5f1d7c5c44) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hotshock.h4",  0x0000, 0x1000, CRC(60bdaea9) SHA1(fd10109803661dc1ce72e1291e3721bdb2bb159f) )
	ROM_LOAD( "hotshock.h5",  0x1000, 0x1000, CRC(4ef17453) SHA1(7dc58456b2f25775c85b3ae92f605d69bb68d590) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( hunchbks )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c_hb01.bin",  0x0000, 0x0800, CRC(8bebd834) SHA1(08f2ce732d2d8754bf559260e1f656a33e2a06a5) )
	ROM_LOAD( "2e_hb02.bin",  0x0800, 0x0800, CRC(07de4229) SHA1(9f333509ae3d6c579f6d96caa172a0abe9eefb30) )
	ROM_LOAD( "2f_hb03.bin",  0x2000, 0x0800, CRC(b75a0dfc) SHA1(c60c833f28c6de027d46f5a2a54ad5646ec58453) )
	ROM_LOAD( "2h_hb04.bin",  0x2800, 0x0800, CRC(f3206264) SHA1(36a614db3fda4f97cc085d84bd13ea44969de95b) )
	ROM_LOAD( "2j_hb05.bin",  0x4000, 0x0800, CRC(1bb78728) SHA1(aebfca355d937825217d069689f9b4d7a113b10a) )
	ROM_LOAD( "2l_hb06.bin",  0x4800, 0x0800, CRC(f25ed680) SHA1(7854e4975a4f75916f60749ac24147c335927394) )
	ROM_LOAD( "2m_hb07.bin",  0x6000, 0x0800, CRC(c72e0e17) SHA1(90da1e375733873bc592e11980bdaf8168bd5aea) )
	ROM_LOAD( "2p_hb08.bin",  0x6800, 0x0800, CRC(412087b0) SHA1(4d6f343577ae73031f32cda8903c74e5a840e71d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "11d_snd.bin",  0x0000, 0x0800, CRC(88226086) SHA1(fe2da172313063e5b056fc8c8d8b2a5c64db5179) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f_hb09.bin",  0x0000, 0x0800, CRC(db489c3d) SHA1(df08607ad07222c1c1c4b3589b50b785bdeefbf2) )
	ROM_LOAD( "5h_hb10.bin",  0x0800, 0x0800, CRC(3977650e) SHA1(1de05d6ceed3f2ed0925caa8235b63a93f03f61e) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6e_prom.bin",  0x0000, 0x0020, CRC(01004d3f) SHA1(e53cbc54ea96e846481a67bbcccf6b1726e70f9c) )
ROM_END

ROM_START( cavelon )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k + 16K banked for code */
	ROM_LOAD( "2.bin",		 0x00000, 0x2000, CRC(a3b353ac) SHA1(1d5cc402f83c410f2ccd186dafb8bf16a7778fb0) )
	ROM_LOAD( "1.bin",		 0x02000, 0x2000, CRC(3f62efd6) SHA1(b03a46f8478f499812c5d9c11816ee28d67fb77b) )
	ROM_RELOAD(				 0x12000, 0x2000)
	ROM_LOAD( "3.bin",		 0x10000, 0x2000, CRC(39d74e4e) SHA1(4789eab2741555f59a97ef5a10b0500f6b64a6ce) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1c_snd.bin",	  0x0000, 0x0800, CRC(f58dcf55) SHA1(517dab8684109188d7d78c8a2cf94a4fac17d40c) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "h.bin",		  0x0000, 0x1000, CRC(d44fcd6f) SHA1(c275741bb1d876e7308e131cac2f1fee249613c7) )
	ROM_LOAD( "k.bin",		  0x1000, 0x1000, CRC(59bc7f9e) SHA1(4374955d0fdfbba57ba432da22b0d94b66832fb8) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "cavelon.clr",  0x0000, 0x0020, CRC(d133356b) SHA1(58db4013a9ad77107f0d462c96363d7c38d86fa2) )
ROM_END

ROM_START( sfx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sfx_b-0.1j",   0x0000, 0x1000, CRC(e5bc6952) SHA1(7bfb772418d738d3c49fd59c0bfc04590945977a) )
	ROM_CONTINUE(             0xe000, 0x1000             )
	ROM_LOAD( "1.1c",         0x1000, 0x1000, CRC(1b3c48e7) SHA1(2f245aaf9b4bb5d949aae18ee89a0be639e7b2df) )
	ROM_LOAD( "22.1d",        0x2000, 0x1000, CRC(ed44950d) SHA1(f8c54ff89ac461171df951d703d5571be1b8da38) )
	ROM_LOAD( "23.1e",        0x3000, 0x1000, CRC(f44a3ca0) SHA1(3917ea960329a06d3d0c447cb6a4ba710fb7ca92) )
	ROM_LOAD( "27.1a",        0x7000, 0x1000, CRC(ed86839f) SHA1(a0d8c941a6e01058eab66d5da9b49b6b5695b981) )
	ROM_LOAD( "24.1g",        0xc000, 0x1000, CRC(e6d7dc74) SHA1(c1e6d9598fb837775ee6550fea3cd4910572615e) )
	ROM_LOAD( "5.1h",         0xd000, 0x1000, CRC(d1e8d390) SHA1(f8fe9f69e6500fbcf25f8151c1070d9a1a20a38c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5.5j",         0x0000, 0x1000, CRC(59028fb6) SHA1(94105b5b03c81a948a409f7ea20312bb9c79c150) )
	ROM_LOAD( "6.6j",         0x1000, 0x1000, CRC(5427670f) SHA1(ffc3f7186d0319f0fd7ed25eb97bb0db7bc107c6) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the sample CPU */
	ROM_LOAD( "1.1j",         0x0000, 0x1000, CRC(2f172c58) SHA1(4706d55fcfad4d5a87d96a0a0187f59997ef9720) )
	ROM_LOAD( "2.2j",         0x1000, 0x1000, CRC(a6ad2f6b) SHA1(14d1a93e507c349b14a1b26408cce23f089fa33c) )
	ROM_LOAD( "3.3j",         0x2000, 0x1000, CRC(fa1274fa) SHA1(e98cb602b265b209eaa4a9b3972e47c869ff863b) )
	ROM_LOAD( "4.4j",         0x3000, 0x1000, CRC(1cd33f3a) SHA1(cf9248fd6cb56ec81d354afe032a2dea810e834b) )
	ROM_LOAD( "10.3h",        0x4000, 0x1000, CRC(b833a15b) SHA1(0d21aaa0ca5ccba89118b205a6b3b36b15663c47) )
	ROM_LOAD( "11.4h",        0x5000, 0x1000, CRC(cbd76ec2) SHA1(9434350ee93ca71efe78018b69913386353306ff) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "28.5a",        0x0000, 0x1000, CRC(d73a8252) SHA1(59d14f41f1a806f98ee33596b84fe5aefe606944) )
	ROM_LOAD( "29.5c",        0x1000, 0x1000, CRC(1401ccf2) SHA1(5762eafd9f402330e1d4ac677f46595087716c47) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331.9g",      0x0000, 0x0020, CRC(ca1d9ccd) SHA1(27124759a06497c1bc1a64b6d3faa6ba924a8447) )
ROM_END

ROM_START( mimonscr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mm1",          0x0000, 0x1000, CRC(0399a0c4) SHA1(8314124f9b535ce531663625d19cd3a76782ed3b) )
	ROM_LOAD( "mm2",          0x1000, 0x1000, CRC(2c5e971e) SHA1(39f979b99566e30a19c63115c936bb11fae4c609) )
	ROM_LOAD( "mm3",          0x2000, 0x1000, CRC(24ce1ce3) SHA1(ae5ba6913cabab2152bf48c0c0d5983ecbe5c700) )
	ROM_LOAD( "mm4",          0x3000, 0x1000, CRC(c83fb639) SHA1(38ddd80b25cc0707b9e53396c322fe731ea8bc3e) )
	ROM_LOAD( "mm5",          0xc000, 0x1000, CRC(a9f12dfc) SHA1(c279e3ac84194cc83642a2c330fd869eaae8f063) )
	ROM_LOAD( "mm6",          0xd000, 0x1000, CRC(e492a40c) SHA1(d01d6f9c18821fd8c7ed11d65d13bd0c9595881f) )
	ROM_LOAD( "mm7",          0xe000, 0x1000, CRC(5339928d) SHA1(7c28516fb7d762e2f77d0ed3dc56a57d0213dbf9) )
	ROM_LOAD( "mm8",          0xf000, 0x1000, CRC(eee7a12e) SHA1(bde6bfe98b15215c48c85a22615b0242ea4f0224) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "mmsound1",	  0x0000, 0x1000, CRC(2d14c527) SHA1(062414ce0415b6c471149319ecae22f465df3a4f) )
	ROM_LOAD( "mmsnd2a",	  0x1000, 0x1000, CRC(35ed0f96) SHA1(5aaacae5c2acf97540b72491f71ea823f5eeae1a) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mmgfx1",		  0x0000, 0x2000, CRC(4af47337) SHA1(225f7bcfbb61e3a163ecaed675d4c81b3609562f) )
	ROM_LOAD( "mmgfx2",		  0x2000, 0x2000, CRC(def47da8) SHA1(8e62e5dc5c810efaa204d0fcb3d02bc84f61ba35) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END


GAME( 1981, scramble, 0,        scramble, scramble, scramble,     ROT90, "Konami", "Scramble" )
GAME( 1981, scrambls, scramble, scramble, scramble, scrambls,     ROT90, "[Konami] (Stern license)", "Scramble (Stern)" )
GAME( 1981, explorer, scramble, explorer, explorer, 0,		      ROT90, "bootleg", "Explorer" )
GAME( 1981, atlantis, 0,        scramble, atlantis, atlantis,     ROT90, "Comsoft", "Battle of Atlantis (set 1)" )
GAME( 1981, atlants2, atlantis, scramble, atlantis, atlantis,     ROT90, "Comsoft", "Battle of Atlantis (set 2)" )
GAME( 1980, theend,   0,        theend,   theend,   theend,       ROT90, "Konami", "The End" )
GAME( 1980, theends,  theend,   theend,   theend,   theend,       ROT90, "[Konami] (Stern license)", "The End (Stern)" )
GAME( 1981, froggers, frogger,  froggers, froggers, froggers,     ROT90, "bootleg", "Frog" )
GAME( 1982, amidars,  amidar,   scramble, amidars,  atlantis,     ROT90, "Konami", "Amidar (Scramble hardware)" )
GAME( 1982, triplep,  0,        triplep,  triplep,  scramble_ppi, ROT90, "KKI", "Triple Punch" )
GAME( 1982, knockout, triplep,  triplep,  triplep,  scramble_ppi, ROT90, "KKK", "Knock Out!!" )
GAMEX(1981, mariner,  0,        mariner,  scramble, mariner,      ROT90, "Amenip", "Mariner", GAME_IMPERFECT_SOUND )
GAME( 1981, 800fath,  mariner,  mariner,  scramble, mariner,      ROT90, "Amenip (US Billiards Inc. license)", "800 Fathoms" )
GAME( 1981, ckongs,   ckong,    ckongs,   ckongs,   ckongs,       ROT90, "bootleg", "Crazy Kong (Scramble hardware)" )
GAME( 1981, mars,     0,        mars,     mars,     mars,         ROT90, "Artic", "Mars" )
GAME( 1982, devilfsh, 0,        devilfsh, devilfsh, devilfsh,     ROT90, "Artic", "Devil Fish" )
GAME( 1983, newsin7,  0,        newsin7,  newsin7,  mars,         ROT90, "ATW USA, Inc.", "New Sinbad 7" )
GAME( 1984, mrkougar, 0,        mrkougar, mrkougar, mrkougar,     ROT90, "ATW", "Mr. Kougar" )
GAME( 1983, mrkougr2, mrkougar, mrkougar, mrkougar, mrkougar,     ROT90, "ATW", "Mr. Kougar (earlier)" )
GAME( 1983, mrkougb,  mrkougar, mrkougb,  mrkougar, mrkougb,      ROT90, "bootleg", "Mr. Kougar (bootleg)" )
GAME( 1982, hotshock, 0,        hotshock, hotshock, hotshock,     ROT90, "E.G. Felaco", "Hot Shocker" )
GAME( 1983, hunchbks, hunchbak, hunchbks, hunchbks, scramble_ppi, ROT90, "Century Electronics", "Hunchback (Scramble hardware)" )
GAME( 1983, cavelon,  0,        cavelon,  cavelon,  cavelon,      ROT90, "Jetsoft", "Cavelon" )
GAME( 1983, sfx,      0,        sfx,      sfx,      sfx,          ORIENTATION_FLIP_X, "Nichibutsu", "SF-X" )
GAME( 198?, mimonscr, mimonkey, mimonscr, mimonscr, mimonscr,     ROT90, "bootleg", "Mighty Monkey (bootleg on Scramble hardware)" )
