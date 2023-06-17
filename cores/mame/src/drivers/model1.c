/*
**      V60 + 68k + 4xTGP
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "system16.h"
#include "vidhrdw/segaic24.h"
#include "cpu/m68000/m68k.h"

WRITE16_HANDLER( model1_paletteram_w );
VIDEO_START(model1);
VIDEO_UPDATE(model1);
VIDEO_EOF(model1);
extern UINT16 *model1_display_list0, *model1_display_list1;
extern UINT16 *model1_color_xlat;
READ16_HANDLER( model1_listctl_r );
WRITE16_HANDLER( model1_listctl_w );

READ16_HANDLER( model1_tgp_copro_r );
WRITE16_HANDLER( model1_tgp_copro_w );
READ16_HANDLER( model1_tgp_copro_adr_r );
WRITE16_HANDLER( model1_tgp_copro_adr_w );
READ16_HANDLER( model1_tgp_copro_ram_r );
WRITE16_HANDLER( model1_tgp_copro_ram_w );

static int model1_sound_irq;

#define FIFO_SIZE	(8)
static int to_68k[FIFO_SIZE], fifo_wptr, fifo_rptr;

void model1_tgp_reset(int swa);

static READ16_HANDLER( io_r )
{
	if(offset < 0x8)
		return readinputport(offset);
	if(offset < 0x10) {
		offset -= 0x8;
		if(offset < 3)
			return readinputport(offset+8) | 0xff00;
		return 0xff;
	}

	logerror("IOR: %02x\n", offset);
	return 0xffff;
}

static READ16_HANDLER( fifoin_status_r )
{
	return 0xffff;
}

static WRITE16_HANDLER( bank_w )
{
	if(ACCESSING_LSB) {
		switch(data & 0xf) {
		case 0x1: /* 100000-1fffff data roms banking */
			cpu_setbank(1, memory_region(REGION_CPU1) + 0x1000000 + 0x100000*((data >> 4) & 0xf));
			logerror("BANK %x\n", 0x1000000 + 0x100000*((data >> 4) & 0xf));
			break;
		case 0x2: /* 200000-2fffff data roms banking (unused, all known games have only one bank) */
			break;
		case 0xf: /* f00000-ffffff program roms banking (unused, all known games have only one bank) */
			break;
		}
	}
}


static int last_irq;

static void irq_raise(int level)
{
	/*	logerror("irq: raising %d\n", level); */
	/*	irq_status |= (1 << level); */
	last_irq = level;
	cpu_set_irq_line(0, 0, HOLD_LINE);
}

static int irq_callback(int irqline)
{
	return last_irq;
}
/* vf */
/* 1 = fe3ed4 */
/* 3 = fe3f5c */
/* other = fe3ec8 / fe3ebc */

/* vr */
/* 1 = fe02bc */
/* other = f302a4 / fe02b0 */

/* swa */
/* 1 = ff504 */
/* 3 = ff54c */
/* other = ff568/ff574 */

static void irq_init(void)
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	cpu_set_irq_callback(0, irq_callback);
}

extern void tgp_tick(void);
static INTERRUPT_GEN(model1_interrupt)
{
	if (cpu_getiloops())
	{
		irq_raise(1);
		tgp_tick();
	}
	else
	{
		irq_raise(model1_sound_irq);

		/* if the FIFO has something in it, signal the 68k too */
		if (fifo_rptr != fifo_wptr)
		{
			cpu_set_irq_line(1, 2, HOLD_LINE);
		}
	}
}

static MACHINE_INIT(model1)
{
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x1000000);
	irq_init();
	model1_tgp_reset(!strcmp(Machine->gamedrv->name, "swa") || !strcmp(Machine->gamedrv->name, "wingwar"));
	if (!strcmp(Machine->gamedrv->name, "swa"))
	{
		model1_sound_irq = 0;
	}
	else
	{
		model1_sound_irq = 3;
	}

	/* init the sound FIFO */
	fifo_rptr = fifo_wptr = 0;
	memset(to_68k, 0, sizeof(to_68k));
}

static READ16_HANDLER( network_ctl_r )
{
	if(offset)
		return 0x40;
	else
		return 0x00;
}

static WRITE16_HANDLER( network_ctl_w )
{
}

static WRITE16_HANDLER(md1_w)
{
	extern int model1_dump;
	COMBINE_DATA(model1_display_list1+offset);
	if(0 && offset)
		return;
	if(1 && model1_dump)
		logerror("TGP: md1_w %x, %04x @ %04x (%x)\n", offset, data, mem_mask, activecpu_get_pc());
}

static WRITE16_HANDLER(md0_w)
{
	extern int model1_dump;
	COMBINE_DATA(model1_display_list0+offset);
	if(0 && offset)
		return;
	if(1 && model1_dump)
		logerror("TGP: md0_w %x, %04x @ %04x (%x)\n", offset, data, mem_mask, activecpu_get_pc());
}

static WRITE16_HANDLER(p_w)
{
	UINT16 old = paletteram16[offset];
	paletteram16_xBBBBBGGGGGRRRRR_word_w(offset, data, mem_mask);
	if(0 && paletteram16[offset] != old)
		logerror("XVIDEO: p_w %x, %04x @ %04x (%x)\n", offset, data, mem_mask, activecpu_get_pc());
}

static UINT16 *mr;
static WRITE16_HANDLER(mr_w)
{
	COMBINE_DATA(mr+offset);
	if(0 && offset == 0x1138/2)
		logerror("MR.w %x, %04x @ %04x (%x)\n", offset*2+0x500000, data, mem_mask, activecpu_get_pc());
}

static UINT16 *mr2;
static WRITE16_HANDLER(mr2_w)
{
	COMBINE_DATA(mr2+offset);
#if 0
	if(0 && offset == 0x6e8/2) {
		logerror("MR.w %x, %04x @ %04x (%x)\n", offset*2+0x400000, data, mem_mask, activecpu_get_pc());
	}
	if(offset/2 == 0x3680/4)
		logerror("MW f80[r25], %04x%04x (%x)\n", mr2[0x3680/2+1], mr2[0x3680/2], activecpu_get_pc());
	if(offset/2 == 0x06ca/4)
		logerror("MW fca[r19], %04x%04x (%x)\n", mr2[0x06ca/2+1], mr2[0x06ca/2], activecpu_get_pc());
	if(offset/2 == 0x1eca/4)
		logerror("MW fca[r22], %04x%04x (%x)\n", mr2[0x1eca/2+1], mr2[0x1eca/2], activecpu_get_pc());
#endif

	/* wingwar scene position, pc=e1ce -> d735 */
	if(offset/2 == 0x1f08/4)
		logerror("MW  8[r10], %f (%x)\n", *(float *)(mr2+0x1f08/2), activecpu_get_pc());
	if(offset/2 == 0x1f0c/4)
		logerror("MW  c[r10], %f (%x)\n", *(float *)(mr2+0x1f0c/2), activecpu_get_pc());
	if(offset/2 == 0x1f10/4)
		logerror("MW 10[r10], %f (%x)\n", *(float *)(mr2+0x1f10/2), activecpu_get_pc());
}

static READ16_HANDLER( snd_68k_ready_r )
{
	int sr = cpunum_get_reg(1, M68K_REG_SR);

	if ((sr & 0x0700) > 0x0100)
	{
		cpu_spinuntil_time(TIME_IN_USEC(40));
		return 0;	/* not ready yet, interrupts disabled */
	}
	
	return 0xff;
}

static WRITE16_HANDLER( snd_latch_to_68k_w )
{
	to_68k[fifo_wptr] = data;
	fifo_wptr++;
	if (fifo_wptr >= FIFO_SIZE) fifo_wptr = 0;

	/* signal the 68000 that there's data waiting */
	cpu_set_irq_line(1, 2, HOLD_LINE);
	/* give the 68k time to reply */
	cpu_spinuntil_time(TIME_IN_USEC(40));
}

static MEMORY_READ16_START( model1_readmem )
    { 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x1fffff, MRA16_BANK1 },
	{ 0x200000, 0x2fffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, MRA16_RAM },
	{ 0x500000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x60ffff, MRA16_RAM },
	{ 0x610000, 0x61ffff, MRA16_RAM },
	{ 0x680000, 0x680003, model1_listctl_r },
	{ 0x700000, 0x70ffff, sys24_tile_r },
	{ 0x780000, 0x7fffff, sys24_char_r },
	{ 0x900000, 0x903fff, MRA16_RAM },
	{ 0x910000, 0x91bfff, MRA16_RAM },
	{ 0xc00000, 0xc0003f, io_r },
	{ 0xc00040, 0xc00043, network_ctl_r },
	{ 0xc00200, 0xc002ff, MRA16_RAM },
	{ 0xc40002, 0xc40003, snd_68k_ready_r },
	{ 0xd00000, 0xd00001, model1_tgp_copro_adr_r },
	{ 0xdc0000, 0xdc0003, fifoin_status_r },
	{ 0xfc0000, 0xffffff, MRA16_ROM },
MEMORY_END

static MEMORY_WRITE16_START( model1_writemem )
    { 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x1fffff, MWA16_BANK1 },
	{ 0x200000, 0x2fffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, mr2_w, &mr2 },
	{ 0x500000, 0x53ffff, mr_w, &mr },
	{ 0x600000, 0x60ffff, md0_w, &model1_display_list0 },
	{ 0x610000, 0x61ffff, md1_w, &model1_display_list1 },
	{ 0x680000, 0x680003, model1_listctl_w },
	{ 0x700000, 0x70ffff, sys24_tile_w },
	{ 0x720000, 0x720001, MWA16_NOP	   },/* Unknown, always 0 */
	{ 0x740000, 0x740001, MWA16_NOP	   },/* Horizontal synchronization register */
	{ 0x760000, 0x760001, MWA16_NOP	   },/* Vertical synchronization register */
	{ 0x770000, 0x770001, MWA16_NOP	   },/* Video synchronization switch */
	{ 0x780000, 0x7fffff, sys24_char_w },
	{ 0x900000, 0x903fff, p_w, &paletteram16 },
	{ 0x910000, 0x91bfff, MWA16_RAM, &model1_color_xlat },
	{ 0xc00040, 0xc00043, network_ctl_w },
	{ 0xc00200, 0xc002ff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ 0xc40000, 0xc40001, snd_latch_to_68k_w },
	{ 0xd00000, 0xd00001, model1_tgp_copro_adr_w },
	{ 0xd20000, 0xd20003, model1_tgp_copro_ram_w },
	{ 0xd80000, 0xd80003, model1_tgp_copro_w },
	{ 0xd80010, 0xd80013, model1_tgp_copro_w }, /* Mirror */
	{ 0xe00000, 0xe00001, MWA16_NOP          }, /* Watchdog?  IRQ ack? Always 0x20, usually on irq */
	{ 0xe00004, 0xe00005, bank_w },
	{ 0xe0000c, 0xe0000f, MWA16_NOP },
	{ 0xfc0000, 0xffffff, MWA16_ROM },
MEMORY_END

static PORT_READ16_START( model1_readport )
    { 0xd20000, 0xd20003, model1_tgp_copro_ram_r },
	{ 0xd80000, 0xd80003, model1_tgp_copro_r },
PORT_END

static READ16_HANDLER( m1_snd_68k_latch_r )
{
	UINT16 retval;

	retval = to_68k[fifo_rptr];

	fifo_rptr++;
	if (fifo_rptr >= FIFO_SIZE) fifo_rptr = 0;

	return retval;
}

static READ16_HANDLER( m1_snd_v60_ready_r )
{
	return 1;
}

static READ16_HANDLER( m1_snd_mpcm0_r )
{
	return MultiPCM_reg_0_r(0);
}

static WRITE16_HANDLER( m1_snd_mpcm0_w )
{
	MultiPCM_reg_0_w(offset, data);
}

static WRITE16_HANDLER( m1_snd_mpcm0_bnk_w )
{
	MultiPCM_bank_0_w(0, data);
}

static READ16_HANDLER( m1_snd_mpcm1_r )
{
	return MultiPCM_reg_1_r(0);
}

static WRITE16_HANDLER( m1_snd_mpcm1_w )
{
	MultiPCM_reg_1_w(offset, data);
}

static WRITE16_HANDLER( m1_snd_mpcm1_bnk_w )
{
	MultiPCM_bank_1_w(0, data);
}

static READ16_HANDLER( m1_snd_ym_r )
{
	return YM2612_status_port_0_A_r(0);
}

static WRITE16_HANDLER( m1_snd_ym_w )
{
	switch (offset)
	{
		case 0:
			YM2612_control_port_0_A_w(0, data);
			break;

		case 1:
			YM2612_data_port_0_A_w(0, data);
			break;

		case 2:
			YM2612_control_port_0_B_w(0, data);
			break;

		case 3:
			YM2612_data_port_0_B_w(0, data);
			break;
	}
}

static WRITE16_HANDLER( m1_snd_68k_latch1_w )
{
}

static WRITE16_HANDLER( m1_snd_68k_latch2_w )
{
}

static MEMORY_READ16_START( model1_readsound )
    { 0x000000, 0x0bffff, MRA16_ROM },
	{ 0xc20000, 0xc20001, m1_snd_68k_latch_r },
	{ 0xc20002, 0xc20003, m1_snd_v60_ready_r },
	{ 0xc40000, 0xc40007, m1_snd_mpcm0_r },
	{ 0xc60000, 0xc60007, m1_snd_mpcm1_r },
	{ 0xd00000, 0xd00007, m1_snd_ym_r },
	{ 0xf00000, 0xf0ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( model1_writesound )
    { 0x000000, 0x0bffff, MWA16_ROM },
	{ 0xc20000, 0xc20001, m1_snd_68k_latch1_w },
	{ 0xc20002, 0xc20003, m1_snd_68k_latch2_w },
	{ 0xc40000, 0xc40007, m1_snd_mpcm0_w },
	{ 0xc40012, 0xc40013, MWA16_NOP },
	{ 0xc50000, 0xc50001, m1_snd_mpcm0_bnk_w },
	{ 0xc60000, 0xc60007, m1_snd_mpcm1_w },
	{ 0xc70000, 0xc70001, m1_snd_mpcm1_bnk_w },
	{ 0xd00000, 0xd00007, m1_snd_ym_w },
	{ 0xf00000, 0xf0ffff, MWA16_RAM },
MEMORY_END

static struct MultiPCM_interface m1_multipcm_interface =
{
	2,
	{ 8000000, 8000000 },
	{ MULTIPCM_MODE_MODEL1, MULTIPCM_MODE_MODEL1 },
	{ (1024*1024), (1024*1024) },
	{ REGION_SOUND1, REGION_SOUND2 },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT), YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT) }
};

static struct YM2612interface m1_ym3438_interface =
{
	1,
	8000000,
	{ 60,60 },
	{ 0 },	{ 0 },	{ 0 },	{ 0 }
};

INPUT_PORTS_START( vf )
	PORT_START  /* Unused analog port 0 */
	PORT_START  /* Unused analog port 1 */
	PORT_START  /* Unused analog port 2 */
	PORT_START  /* Unused analog port 3 */
	PORT_START  /* Unused analog port 4 */
	PORT_START  /* Unused analog port 5 */
	PORT_START  /* Unused analog port 6 */
	PORT_START  /* Unused analog port 7 */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
INPUT_PORTS_END


ROM_START( vf )
	ROM_REGION( 0x1400000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD16_BYTE( "epr-16082.14", 0x200000, 0x80000, CRC(b23f22ee) SHA1(9fd5b5a5974703a60a54de3d2bce4301bfc0e533) )
	ROM_LOAD16_BYTE( "epr-16083.15", 0x200001, 0x80000, CRC(d12c77f8) SHA1(b4aeba8d5f1ab4aec024391407a2cb58ce2e94b0) )

	ROM_LOAD( "epr-16080.4", 0xfc0000, 0x20000, CRC(3662E1A5) SHA1(6bfceb1a7c1c7912679c907f2b7516ae9c7dda67) )
	ROM_LOAD( "epr-16081.5", 0xfe0000, 0x20000, CRC(6DEC06CE) SHA1(7891544456bccd2fc647bccd058945ad50466636) )

	ROM_LOAD16_BYTE( "mpr-16084.6", 0x1000000, 0x80000, CRC(483f453b) SHA1(41a5527be73f5dd1c87b2a8113235bdd247ec049) )
	ROM_LOAD16_BYTE( "mpr-16085.7", 0x1000001, 0x80000, CRC(5fa01277) SHA1(dfa7ddff0a7daf29071431f26b93dd8e8e5793b6) )
	ROM_LOAD16_BYTE( "mpr-16086.8", 0x1100000, 0x80000, CRC(deac47a1) SHA1(3a8016124e4dc579d4aae745d4af1905ad0e4fbd) )
	ROM_LOAD16_BYTE( "mpr-16087.9", 0x1100001, 0x80000, CRC(7a64daac) SHA1(da6a9cad4b0cb2af4299e664c0889f3fbdc25530) )
	ROM_LOAD16_BYTE( "mpr-16088.10", 0x1200000, 0x80000, CRC(fcda2d1e) SHA1(0f7d0f604d429a1da0d1c3f31694520bada49680) )
	ROM_LOAD16_BYTE( "mpr-16089.11", 0x1200001, 0x80000, CRC(39befbe0) SHA1(362c493092cd0536fadee7326ecc7f973e23fb58) )
	ROM_LOAD16_BYTE( "mpr-16090.12", 0x1300000, 0x80000, CRC(90c76831) SHA1(5a3c25f2a131cfbb2ad067bef1ab7b1c95645d41) )
	ROM_LOAD16_BYTE( "mpr-16091.13", 0x1300001, 0x80000, CRC(53115448) SHA1(af798d5b1fcb720d7288a5ac48839d9ace16a2f2) )

	ROM_REGION( 0xc0000, REGION_CPU2, 0 )  /* 68K code */
	ROM_LOAD16_WORD_SWAP( "epr-16120.7", 0x00000, 0x20000, CRC(2bff8378) SHA1(854b08ab983e4e98cb666f2f44de9a6829b1eb52) )
	ROM_LOAD16_WORD_SWAP( "epr-16121.8", 0x20000, 0x20000, CRC(ff6723f9) SHA1(53498b8c103745883657dfd6efe27edfd48b356f) )
	ROM_RELOAD( 0x80000, 0x20000)

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mpr-16122.32", 0x000000, 0x200000, CRC(568bc64e) SHA1(31fd0ef8319efe258011b4621adebb790b620770) )
	ROM_LOAD( "mpr-16123.33", 0x200000, 0x200000, CRC(15d78844) SHA1(37c17e38604cf7004a951408024941cd06b1d93e) )

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) /* Samples */
	ROM_LOAD( "mpr-16124.4", 0x000000, 0x200000, CRC(45520ba1) SHA1(c33e3c12639961016e5fa6b5025d0a67dff28907) )
	ROM_LOAD( "mpr-16125.5", 0x200000, 0x200000, CRC(9b4998b6) SHA1(0418d9b0acf79f35d0f7575c21f1be9a0ea343da) )

	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 ) /* TGP model roms */
	ROM_LOAD32_WORD( "mpr-16096.26", 0x000000, 0x200000, CRC(a92b0bf3) SHA1(fd3adff5f41f0b0be98df548c848eda04fc0da48) )
	ROM_LOAD32_WORD( "mpr-16097.27", 0x000002, 0x200000, CRC(0232955a) SHA1(df934fb6d022032620932571ff5ed176d5dcb017) )
	ROM_LOAD32_WORD( "mpr-16098.28", 0x400000, 0x200000, CRC(cf2e1b84) SHA1(f3d16c72344f7f218a792ce7f1dd7cad910a8c97) )
	ROM_LOAD32_WORD( "mpr-16099.29", 0x400002, 0x200000, CRC(20e46854) SHA1(423d3642bd2f14e68d29029c027b791de2c1ec53) )
	ROM_LOAD32_WORD( "mpr-16100.30", 0x800000, 0x200000, CRC(e13e983d) SHA1(120637caa2404ad4124b676fd6fcd721f30948df) )
	ROM_LOAD32_WORD( "mpr-16101.31", 0x800002, 0x200000, CRC(0dbed94d) SHA1(df1cddcc1d3976816bd786c2d6211a8563f6f690) )
	ROM_LOAD32_WORD( "mpr-16102.32", 0xc00000, 0x200000, CRC(4cb41fb6) SHA1(4a07bfad4f221508de8c931861424dcc5be3f46a) )
	ROM_LOAD32_WORD( "mpr-16103.33", 0xc00002, 0x200000, CRC(526d1c76) SHA1(edc8dafc9261cd0e970c3b50e3c1ca51a32a4cdf) )
ROM_END


static MACHINE_DRIVER_START( model1 )
    MDRV_CPU_ADD(V60, 16000000)
	MDRV_CPU_MEMORY(model1_readmem, model1_writemem)
	MDRV_CPU_PORTS(model1_readport, 0)
	MDRV_CPU_VBLANK_INT(model1_interrupt, 2)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(model1_readsound, model1_writesound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(model1)
	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(model1)
	MDRV_VIDEO_UPDATE(model1)
	MDRV_VIDEO_EOF(model1)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM3438, m1_ym3438_interface)
	MDRV_SOUND_ADD(MULTIPCM, m1_multipcm_interface)
MACHINE_DRIVER_END



GAMEX(1993, vf, 0, model1, vf, 0, ROT0, "Sega", "Virtua Fighter", GAME_IMPERFECT_GRAPHICS )
