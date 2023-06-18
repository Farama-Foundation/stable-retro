/***************************************************************************

	Atari Bad Lands hardware

    driver by Aaron Giles

	Games supported:
		* Bad Lands (1989)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	MAIN CPU
	========================================================================
	000000-03FFFF   R     xxxxxxxx xxxxxxxx   Program ROM
	FC0000          R     -------x --------   Sound command buffer full
	FC0000            W   -------- --------   Sound CPU reset
	FD0000-FD1FFF   R/W   -------- xxxxxxxx   EEPROM
	FE0000            W   -------- --------   Watchdog reset
	FE2000            W   -------- --------   VBLANK IRQ acknowledge
	FE4000          R     -------- xxxx----   Switch inputs
	                R     -------- x-------      (Self test)
	                R     -------- -x------      (VBLANK)
	                R     -------- --x-----      (Player 2 button)
	                R     -------- ---x----      (Player 1 button)
	FE6000          R     -------- xxxxxxxx   Player 1 steering
	FE6002          R     -------- xxxxxxxx   Player 2 steering
	FE6004          R     -------- xxxxxxxx   Player 1 pedal
	FE6006          R     -------- xxxxxxxx   Player 2 pedal
	FE8000            W   xxxxxxxx --------   Sound command write
	FEA000          R     xxxxxxxx --------   Sound response read
	FEC000            W   -------- -------x   Playfield tile bank select
	FEE000            W   -------- --------   EEPROM enable
	FFC000-FFC0FF   R/W   xxxxxxxx xxxxxxxx   Playfield palette RAM (128 entries)
	                R/W   x------- --------      (RGB 1 LSB)
	                R/W   -xxxxx-- --------      (Red 5 MSB)
	                R/W   ------xx xxx-----      (Green 5 MSB)
	                R/W   -------- ---xxxxx      (Blue 5 MSB)
	FFC100-FFC1FF   R/W   xxxxxxxx xxxxxxxx   Motion object palette RAM (128 entries)
	FFC200-FFC3FF   R/W   xxxxxxxx xxxxxxxx   Extra palette RAM (256 entries)
	FFE000-FFEFFF   R/W   xxxxxxxx xxxxxxxx   Playfield RAM (64x32 tiles)
	                R/W   xxx----- --------      (Palette select)
	                R/W   ---x---- --------      (Tile bank select)
	                R/W   ----xxxx xxxxxxxx      (Tile index)
	FFF000-FFFFFF   R/W   xxxxxxxx xxxxxxxx   Motion object RAM (32 entries x 4 words)
	                R/W   ----xxxx xxxxxxxx      (0: Tile index)
	                R/W   xxxxxxxx x-------      (1: Y position)
	                R/W   -------- ----xxxx      (1: Number of Y tiles - 1)
	                R/W   xxxxxxxx x-------      (3: X position)
	                R/W   -------- ----x---      (3: Priority)
	                R/W   -------- -----xxx      (3: Palette select)
	========================================================================
	Interrupts:
		IRQ1 = VBLANK
		IRQ2 = sound CPU communications
	========================================================================


	========================================================================
	SOUND CPU (based on JSA II, but implemented onboard)
	========================================================================
	0000-1FFF   R/W   xxxxxxxx   Program RAM
	2000-2001   R/W   xxxxxxxx   YM2151 communications
	2802        R     xxxxxxxx   Sound command read
	2804        R     xxxx--xx   Status input
	            R     x-------      (Self test)
	            R     -x------      (Sound command buffer full)
	            R     --x-----      (Sound response buffer full)
	            R     ---x----      (Self test)
	            R     ------xx      (Coin inputs)
	2806        R/W   --------   IRQ acknowledge
	2A02          W   xxxxxxxx   Sound response write
	2A04          W   xxxx---x   Sound control
	              W   xx------      (ROM bank select)
	              W   --xx----      (Coin counters)
	              W   -------x      (YM2151 reset)
	3000-3FFF   R     xxxxxxxx   Banked ROM
	4000-FFFF   R     xxxxxxxx   Program ROM
	========================================================================
	Interrupts:
		IRQ = timed interrupt ORed with YM2151 interrupt
		NMI = latch on sound command
	========================================================================

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "badlands.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 pedal_value[2];

static UINT8 *bank_base;
static UINT8 *bank_source_data;



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 1;
	if (atarigen_sound_int_state)
		newstate = 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void scanline_update(int scanline)
{
	/* sound IRQ is on 32V */
	if (scanline & 32)
		atarigen_6502_irq_ack_r(0);
	else if (!(readinputport(0) & 0x40))
		atarigen_6502_irq_gen();
}


static MACHINE_INIT( badlands )
{
	pedal_value[0] = pedal_value[1] = 0x80;

	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(scanline_update, 32);

	atarigen_sound_io_reset(1);
	memcpy(bank_base, &bank_source_data[0x0000], 0x1000);
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static INTERRUPT_GEN( vblank_int )
{
	int pedal_state = input_port_4_r(0);
	int i;

	/* update the pedals once per frame */
    for (i = 0; i < 2; i++)
	{
		pedal_value[i]--;
		if (pedal_state & (1 << i))
			pedal_value[i]++;
	}

	atarigen_video_int_gen();
}



/*************************************
 *
 *	I/O read dispatch
 *
 *************************************/

static READ16_HANDLER( sound_busy_r )
{
	int temp = 0xfeff;
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0100;
	return temp;
}


static READ16_HANDLER( pedal_0_r )
{
	return pedal_value[0];
}


static READ16_HANDLER( pedal_1_r )
{
	return pedal_value[1];
}



/*************************************
 *
 *	Audio I/O handlers
 *
 *************************************/

static READ_HANDLER( audio_io_r )
{
	int result = 0xff;

	switch (offset & 0x206)
	{
		case 0x000:		/* n/c */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "audio_io_r: Unknown read at %04X\n", offset & 0x206);
			break;

		case 0x002:		/* /RDP */
			result = atarigen_6502_sound_r(offset);
			break;

		case 0x004:		/* /RDIO */
			/*
				0x80 = self test
				0x40 = NMI line state (active low)
				0x20 = sound output full
				0x10 = self test
				0x08 = +5V
				0x04 = +5V
				0x02 = coin 2
				0x01 = coin 1
			*/
			result = readinputport(3);
			if (!(readinputport(0) & 0x0080)) result ^= 0x90;
			if (atarigen_cpu_to_sound_ready) result ^= 0x40;
			if (atarigen_sound_to_cpu_ready) result ^= 0x20;
			result ^= 0x10;
			break;

		case 0x006:		/* /IRQACK */
			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* /VOICE */
		case 0x202:		/* /WRP */
		case 0x204:		/* /WRIO */
		case 0x206:		/* /MIX */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "audio_io_r: Unknown read at %04X\n", offset & 0x206);
			break;
	}

	return result;
}


static WRITE_HANDLER( audio_io_w )
{
	switch (offset & 0x206)
	{
		case 0x000:		/* n/c */
		case 0x002:		/* /RDP */
		case 0x004:		/* /RDIO */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "audio_io_w: Unknown write (%02X) at %04X\n", data & 0xff, offset & 0x206);
			break;

		case 0x006:		/* /IRQACK */
			atarigen_6502_irq_ack_r(0);
			break;

		case 0x200:		/* n/c */
		case 0x206:		/* n/c */
			break;

		case 0x202:		/* /WRP */
			atarigen_6502_sound_w(offset, data);
			break;

		case 0x204:		/* WRIO */
			/*
				0xc0 = bank address
				0x20 = coin counter 2
				0x10 = coin counter 1
				0x08 = n/c
				0x04 = n/c
				0x02 = n/c
				0x01 = YM2151 reset (active low)
			*/

			/* update the bank */
			memcpy(bank_base, &bank_source_data[0x1000 * ((data >> 6) & 3)], 0x1000);
			break;
	}
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xfc0000, 0xfc1fff, sound_busy_r },
	{ 0xfd0000, 0xfd1fff, atarigen_eeprom_r },
	{ 0xfe4000, 0xfe5fff, input_port_0_word_r },
	{ 0xfe6000, 0xfe6001, input_port_1_word_r },
	{ 0xfe6002, 0xfe6003, input_port_2_word_r },
	{ 0xfe6004, 0xfe6005, pedal_0_r },
	{ 0xfe6006, 0xfe6007, pedal_1_r },
	{ 0xfea000, 0xfebfff, atarigen_sound_upper_r },
	{ 0xffc000, 0xffc3ff, MRA16_RAM },
	{ 0xffe000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xfc0000, 0xfc1fff, atarigen_sound_reset_w },
	{ 0xfd0000, 0xfd1fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfe0000, 0xfe1fff, watchdog_reset16_w },
	{ 0xfe2000, 0xfe3fff, atarigen_video_int_ack_w },
	{ 0xfe8000, 0xfe9fff, atarigen_sound_upper_w },
	{ 0xfec000, 0xfedfff, badlands_pf_bank_w },
	{ 0xfee000, 0xfeffff, atarigen_eeprom_enable_w },
	{ 0xffc000, 0xffc3ff, atarigen_expanded_666_paletteram_w, &paletteram16 },
	{ 0xffe000, 0xffefff, atarigen_playfield_w, &atarigen_playfield },
	{ 0xfff000, 0xfff1ff, atarimo_0_spriteram_expanded_w, &atarimo_0_spriteram },
	{ 0xfff200, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( audio_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2800, 0x2bff, audio_io_r },
	{ 0x3000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( audio_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2bff, audio_io_w },
	{ 0x3000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( badlands )
	PORT_START		/* fe4000 */
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* fe6000 */
	PORT_ANALOG( 0x00ff, 0, IPT_DIAL | IPF_PLAYER1, 50, 10, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* fe6002 */
	PORT_ANALOG( 0x00ff, 0, IPT_DIAL | IPF_PLAYER2, 50, 10, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* audio port */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* self test */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* response buffer full */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* command buffer full */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* self test */

	PORT_START      /* fake for pedals */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};


static struct GfxLayout molayout =
{
	16,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60 },
	{ 0*8, 8*8, 16*8, 24*8, 32*8, 40*8, 48*8, 56*8 },
	64*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout,    0, 8 },
	{ REGION_GFX2, 0, &molayout,  128, 8 },
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct YM2151interface ym2151_interface =
{
	1,
	ATARI_CLOCK_14MHz/4,
	{ YM3012_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
	{ 0 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( badlands )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(vblank_int,1)

	MDRV_CPU_ADD(M6502, ATARI_CLOCK_14MHz/8)
	MDRV_CPU_MEMORY(audio_readmem,audio_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(badlands)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(badlands)
	MDRV_VIDEO_UPDATE(badlands)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( badlands )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "1008.20f",  0x00000, 0x10000, CRC(a3da5774) SHA1(5ab1eb61d25594b2d7c40400cb57e7f47a717598) )
	ROM_LOAD16_BYTE( "1006.27f",  0x00001, 0x10000, CRC(aa03b4f3) SHA1(5eda60c715ffcefd4ad34bdb90579e8671dc384a) )
	ROM_LOAD16_BYTE( "1009.17f",  0x20000, 0x10000, CRC(0e2e807f) SHA1(5b61de066dca12c44335aa68a13c821845657866) )
	ROM_LOAD16_BYTE( "1007.24f",  0x20001, 0x10000, CRC(99a20c2c) SHA1(9b0a5a5dafb8816e72330d302c60339b600b49a8) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1018.9c", 0x10000, 0x4000, CRC(a05fd146) SHA1(d97abbcf7897ca720cc18ff3a323f41cd3b23c34) )
	ROM_CONTINUE(        0x04000, 0xc000 )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1012.4n",  0x000000, 0x10000, CRC(5d124c6c) SHA1(afebaaf90b3751f5e873fc4c45f1d5385ef86a6e) )	/* playfield */
	ROM_LOAD( "1013.2n",  0x010000, 0x10000, CRC(b1ec90d6) SHA1(8d4c7db8e1bf9c050f5869eb38fa573867fdc12b) )
	ROM_LOAD( "1014.4s",  0x020000, 0x10000, CRC(248a6845) SHA1(086ef0840b889e790ce3fcd09f98589aae932456) )
	ROM_LOAD( "1015.2s",  0x030000, 0x10000, CRC(792296d8) SHA1(833cdb968064151ca77bb3dbe416ff7127a12de4) )
	ROM_LOAD( "1016.4u",  0x040000, 0x10000, CRC(878f7c66) SHA1(31159bea5d6aac8100fca8f3860220b97d63e72e) )
	ROM_LOAD( "1017.2u",  0x050000, 0x10000, CRC(ad0071a3) SHA1(472b197e5d320b3424d8a8d8c051b1023a07ae08) )

	ROM_REGION( 0x30000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1010.14r", 0x000000, 0x10000, CRC(c15f629e) SHA1(944e3479dce6e420cf9a3f4c1438c5ca66e5cb97) )	/* mo */
	ROM_LOAD( "1011.10r", 0x010000, 0x10000, CRC(fb0b6717) SHA1(694ab0f04d673682831a24027757d4b3c40a4e0e) )
	ROM_LOAD( "1019.14t", 0x020000, 0x10000, CRC(0e26bff6) SHA1(ee018dd37a27c7e7c16a57ea0d32aeb9cdf26bb4) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( badlands )
{
	atarigen_eeprom_default = NULL;
	atarigen_init_6502_speedup(1, 0x4155, 0x416d);

	/* initialize the audio system */
	bank_base = &memory_region(REGION_CPU2)[0x03000];
	bank_source_data = &memory_region(REGION_CPU2)[0x10000];
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1989, badlands, 0, badlands, badlands, badlands, ROT0, "Atari Games", "Bad Lands" )
