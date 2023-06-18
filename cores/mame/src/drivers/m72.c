/***************************************************************************

IREM M72 board

driver by Nicola Salmoria
protection information by Nao

                                   Year Board        Protected?
R-Type                             1987  M72             N
Battle Chopper / Mr. Heli          1987  M72             Y
Ninja Spirit                       1988  M72             Y
Image Fight                        1988  M72             Y
Legend of Hero Tonma               1989  M72             Y
X Multiply                         1989  M72(1)          Y
Dragon Breed                       1989  M81             Y
R-Type II                          1989  M82/M84(2)      N
Major Title                        1990  M84             N
Hammerin' Harry	/ Daiku no Gensan  1990  M82(3)          N
                  Daiku no Gensan  1990  M72(4)          Y
Pound for Pound                    1990  M85             N
Air Duel                           1990  M72?            Y
Cosmic Cop /                       1991  M84             N
  Gallop - Armed Police Unit       1991  M73?            N
Ken-Go                             1991  ?           Encrypted

(1) different addressing PALs, so different memory map
(2) rtype2j has M84 written on the board, but it's the same hardware as rtype2
(3) multiple versions supported, running on different hardware
(4) normal M72 memory map, but IRQ vectors and sprite control as in X-Multiply


TODO:
- majtitle_gfx_ctrl_w is unknown, it seems to be used to disable rowscroll,
  and maybe other things

- Maybe there is a layer enable register, e.g. nspirit shows (for an instant)
  incomplete screens with bad colors when you start a game.

- A lot of unknown I/O writes from the sound CPU in Pound for Pound.

- the sprite chip triggers IRQ1 when it has finished copying the sprite RAM to its
  private buffer. This isn't implemented (all games have an empty IRQ1 handler).
  The cpu board also has support for IRQ3 and IRQ4, coming from the external
  connectors, but I don't think they are used by any game.

IRQ controller
--------------
The initialization consists of one write to port 0x40 and multiple writes
(2 or 3) to port 0x42. The first value written to 0x42 is the IRQ vector base.
Kengo probably has a different controller.

Game      irqbase 0x40  0x42
----      ------- ----  ----------
rtype       0x20   17    20 0F
bchopper     "     "     "
nspirit      "     "     "
loht         "     "     "
rtype2       "     "     "
airduel      "     "     "
gallop       "     "     "
imgfight    0x20   17    20 0F 06
majtitle     "     "     "
poundfor    0x20   17    20 0F 0A
xmultipl    0x08   13    08 0F FA
dbreed       "     "     "
hharry       "     "     "
cosmccop	0x18   --------------
kengo       0x18   --------------

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/irem_cpu.h"
#include "sndhrdw/m72.h"


/* in vidhrdw/m72.c */
extern unsigned char *m72_videoram1,*m72_videoram2,*majtitle_rowscrollram;
MACHINE_INIT( m72 );
MACHINE_INIT( xmultipl );
MACHINE_INIT( kengo );
INTERRUPT_GEN( m72_interrupt );
VIDEO_START( m72 );
VIDEO_START( rtype2 );
VIDEO_START( majtitle );
VIDEO_START( hharry );
VIDEO_START( poundfor );
READ_HANDLER( m72_palette1_r );
READ_HANDLER( m72_palette2_r );
WRITE_HANDLER( m72_palette1_w );
WRITE_HANDLER( m72_palette2_w );
READ_HANDLER( m72_videoram1_r );
READ_HANDLER( m72_videoram2_r );
WRITE_HANDLER( m72_videoram1_w );
WRITE_HANDLER( m72_videoram2_w );
WRITE_HANDLER( m72_irq_line_w );
WRITE_HANDLER( m72_scrollx1_w );
WRITE_HANDLER( m72_scrollx2_w );
WRITE_HANDLER( m72_scrolly1_w );
WRITE_HANDLER( m72_scrolly2_w );
WRITE_HANDLER( m72_dmaon_w );
WRITE_HANDLER( m72_port02_w );
WRITE_HANDLER( rtype2_port02_w );
WRITE_HANDLER( majtitle_gfx_ctrl_w );
VIDEO_UPDATE( m72 );
VIDEO_UPDATE( majtitle );
VIDEO_EOF( m72 );


static unsigned char *protection_ram;



/***************************************************************************

Sample playback

In the later games, the sound CPU can program the start offset of the PCM
samples, but it seems the earlier games have them hardcoded somewhere (maybe
the protection MCU?).
So, here I provided some tables with the start offset precomputed.
They could be built automatically in most cases (00 marks the end of a
sample), but a couple of games (nspirit, loht) have holes in the numbering
so we would have to do them differently anyway.

Also, some games (dkgenm72, poundfor, airduel, gallop) have an empty NMI
handler, so the sample playback has to be handled entirely by external
hardware; we work around that by using (for all games, not just the ones
without a NMI handler) a NMI interrupt gen that mimics the behaviour of
the NMI handler in the other games.

***************************************************************************/


static int find_sample(int num)
{
	data8_t *rom = memory_region(REGION_SOUND1);
	int len = memory_region_length(REGION_SOUND1);
	int addr = 0;

	while (num--)
	{
		/* find end of sample */
		while (addr < len &&  rom[addr]) addr++;

		/* skip 0 filler between samples */
		while (addr < len && !rom[addr]) addr++;
	}

	return addr;
}

static INTERRUPT_GEN(fake_nmi)
{
	int sample = m72_sample_r(0);
	if (sample)
		m72_sample_w(0,sample);
}


static WRITE_HANDLER( bchopper_sample_trigger_w )
{
	int a[6] = { 0x0000, 0x0010, 0x2510, 0x6510, 0x8510, 0x9310 };
	if (data < 6) m72_set_sample_start(a[data]);
}

static WRITE_HANDLER( nspirit_sample_trigger_w )
{
	int a[9] = { 0x0000, 0x0020, 0x2020, 0, 0x5720, 0, 0x7b60, 0x9b60, 0xc360 };
	if (data < 9) m72_set_sample_start(a[data]);
}

static WRITE_HANDLER( imgfight_sample_trigger_w )
{
	int a[7] = { 0x0000, 0x0020, 0x44e0, 0x98a0, 0xc820, 0xf7a0, 0x108c0 };
	if (data < 7) m72_set_sample_start(a[data]);
}

static WRITE_HANDLER( loht_sample_trigger_w )
{
	int a[7] = { 0x0000, 0x0020, 0, 0x2c40, 0x4320, 0x7120, 0xb200 };
	if (data < 7) m72_set_sample_start(a[data]);
}

static WRITE_HANDLER( xmultipl_sample_trigger_w )
{
	int a[3] = { 0x0000, 0x0020, 0x1a40 };
	if (data < 3) m72_set_sample_start(a[data]);
}

static WRITE_HANDLER( dbreed_sample_trigger_w )
{
	int a[9] = { 0x00000, 0x00020, 0x02c40, 0x08160, 0x0c8c0, 0x0ffe0, 0x13000, 0x15820, 0x15f40 };
	if (data < 9) m72_set_sample_start(a[data]);
}

static WRITE_HANDLER( airduel_sample_trigger_w )
{
	int a[16] = { 0x00000, 0x00020, 0x03ec0, 0x05640, 0x06dc0, 0x083a0, 0x0c000, 0x0eb60,
				  0x112e0, 0x13dc0, 0x16520, 0x16d60, 0x18ae0, 0x1a5a0, 0x1bf00, 0x1c340 };
	if (data < 16) m72_set_sample_start(a[data]);
}

static WRITE_HANDLER( dkgenm72_sample_trigger_w )
{
	int a[28] = { 0x00000, 0x00020, 0x01800, 0x02da0, 0x03be0, 0x05ae0, 0x06100, 0x06de0,
			      0x07260, 0x07a60, 0x08720, 0x0a5c0, 0x0c3c0, 0x0c7a0, 0x0e140, 0x0fb00,
				  0x10fa0, 0x10fc0, 0x10fe0, 0x11f40, 0x12b20, 0x130a0, 0x13c60, 0x14740,
				  0x153c0, 0x197e0, 0x1af40, 0x1c080 };

	if (data < 28) m72_set_sample_start(a[data]);
}

static WRITE_HANDLER( gallop_sample_trigger_w )
{
	int a[31] = { 0x00000, 0x00020, 0x00040, 0x01360, 0x02580, 0x04f20, 0x06240, 0x076e0,
			      0x08660, 0x092a0, 0x09ba0, 0x0a560, 0x0cee0, 0x0de20, 0x0e620, 0x0f1c0,
				  0x10200, 0x10220, 0x10240, 0x11380, 0x12760, 0x12780, 0x127a0, 0x13c40,
				  0x140a0, 0x16760, 0x17e40, 0x18ee0, 0x19f60, 0x1bbc0, 0x1cee0 };

	if (data < 31) m72_set_sample_start(a[data]);
}



/***************************************************************************

Protection simulation

Most of the games running on this board have an 8751 protection mcu.
It is not known how it works in detail, however it's pretty clear that it
shares RAM at b0000-b0fff.
On startup, the game writes a pattern to the whole RAM, then reads it back
expecting it to be INVERTED. If it isn't, it reports a RAM error.
If the RAM passes the test, the program increments every byte up to b0ffb,
then calls a subroutine at b0000, which has to be provided by the mcu.
It seems that this routine is not supposed to RET, but instead it should
jump directly to the game entry point. The routine should also write some
bytes here and there in RAM (different in every game); those bytes are
checked at various points during the game, causing a crash if they aren't
right.
Note that the program keeps incrementing b0ffe while the game is running,
maybe this is done to keep the 8751 alive. We don't bother with that.

Finally, to do the ROM test the program asks the mcu to provide the correct
values. This is done only in service, so doesn't seem to be much of a
protection. Here we have provided the correct crcs for the available dumps,
of course there is no guarantee that they are actually good.

All the protection routines below are entirely made up. They get the games
running, but they have not been derived from the real 8751 code.

***************************************************************************/

#define CODE_LEN 96
#define CRC_LEN 18

/* Battle Chopper / Mr. Heli */
static unsigned char bchopper_code[CODE_LEN] =
{
	0x68,0x00,0xa0,				/* push 0a000h*/
	0x1f,						/* pop ds*/
	0xc6,0x06,0x38,0x38,0x53,	/* mov [3838h], byte 053h*/
	0xc6,0x06,0x3a,0x38,0x41,	/* mov [383ah], byte 041h*/
	0xc6,0x06,0x3c,0x38,0x4d,	/* mov [383ch], byte 04dh*/
	0xc6,0x06,0x3e,0x38,0x4f,	/* mov [383eh], byte 04fh*/
	0xc6,0x06,0x40,0x38,0x54,	/* mov [3840h], byte 054h*/
	0xc6,0x06,0x42,0x38,0x4f,	/* mov [3842h], byte 04fh*/
	0x68,0x00,0xb0,				/* push 0b000h*/
	0x1f,						/* pop ds*/
	0xc6,0x06,0x00,0x09,0x49^0xff,	/* mov [0900h], byte 049h*/
	0xc6,0x06,0x00,0x0a,0x49^0xff,	/* mov [0a00h], byte 049h*/
	0xc6,0x06,0x00,0x0b,0x49^0xff,	/* mov [0b00h], byte 049h*/
	0xc6,0x06,0x00,0x00,0xcb^0xff,	/* mov [0000h], byte 0cbh ; retf : bypass protection check during the game*/
	0x68,0x00,0xd0,				/* push 0d000h*/
	0x1f,						/* pop ds*/
	/* the following is for mrheli only, the game checks for*/
	/* "This game can only be played in Japan..." message in the video text buffer*/
	/* the message is nowhere to be found in the ROMs, so has to be displayed by the mcu*/
	0xc6,0x06,0x70,0x16,0x77,	/* mov [1670h], byte 077h*/
	0xea,0x68,0x01,0x40,0x00	/* jmp  0040:$0168*/
};
static unsigned char bchopper_crc[CRC_LEN] =  {	0x1a,0x12,0x5c,0x08, 0x84,0xb6,0x73,0xd1,
												0x54,0x91,0x94,0xeb, 0x00,0x00 };
static unsigned char mrheli_crc[CRC_LEN] =	  {	0x24,0x21,0x1f,0x14, 0xf9,0x28,0xfb,0x47,
												0x4c,0x77,0x9e,0xc2, 0x00,0x00 };

/* Ninja Spirit */
static unsigned char nspirit_code[CODE_LEN] =
{
	0x68,0x00,0xa0,				/* push 0a000h*/
	0x1f,						/* pop ds*/
	0xc6,0x06,0x38,0x38,0x4e,	/* mov [3838h], byte 04eh*/
	0xc6,0x06,0x3a,0x38,0x49,	/* mov [383ah], byte 049h*/
	0xc6,0x06,0x3c,0x38,0x4e,	/* mov [383ch], byte 04eh*/
	0xc6,0x06,0x3e,0x38,0x44,	/* mov [383eh], byte 044h*/
	0xc6,0x06,0x40,0x38,0x4f,	/* mov [3840h], byte 04fh*/
	0xc6,0x06,0x42,0x38,0x55,	/* mov [3842h], byte 055h*/
	0x68,0x00,0xb0,				/* push 0b000h*/
	0x1f,						/* pop ds*/
	0xc6,0x06,0x00,0x09,0x49^0xff,	/* mov [0900h], byte 049h*/
	0xc6,0x06,0x00,0x0a,0x49^0xff,	/* mov [0a00h], byte 049h*/
	0xc6,0x06,0x00,0x0b,0x49^0xff,	/* mov [0b00h], byte 049h*/
	0x68,0x00,0xd0,				/* push 0d000h*/
	0x1f,						/* pop ds*/
	/* the following is for nspiritj only, the game checks for*/
	/* "This game can only be played in Japan..." message in the video text buffer*/
	/* the message is nowhere to be found in the ROMs, so has to be displayed by the mcu*/
	0xc6,0x06,0x70,0x16,0x57,	/* mov [1670h], byte 057h*/
	0xc6,0x06,0x71,0x16,0x00,	/* mov [1671h], byte 000h*/
	0xea,0x00,0x00,0x40,0x00	/* jmp  0040:$0000*/
};
static unsigned char nspirit_crc[CRC_LEN] =   {	0xfe,0x94,0x6e,0x4e, 0xc8,0x33,0xa7,0x2d,
												0xf2,0xa3,0xf9,0xe1, 0xa9,0x6c,0x02,0x95, 0x00,0x00 };
static unsigned char nspiritj_crc[CRC_LEN] =  {	0x26,0xa3,0xa5,0xe9, 0xc8,0x33,0xa7,0x2d,
												0xf2,0xa3,0xf9,0xe1, 0xbc,0x6c,0x01,0x95, 0x00,0x00 };

/* Image Fight */
static unsigned char imgfight_code[CODE_LEN] =
{
	0x68,0x00,0xa0,				/* push 0a000h*/
	0x1f,						/* pop ds*/
	0xc6,0x06,0x38,0x38,0x50,	/* mov [3838h], byte 050h*/
	0xc6,0x06,0x3a,0x38,0x49,	/* mov [383ah], byte 049h*/
	0xc6,0x06,0x3c,0x38,0x43,	/* mov [383ch], byte 043h*/
	0xc6,0x06,0x3e,0x38,0x4b,	/* mov [383eh], byte 04bh*/
	0xc6,0x06,0x40,0x38,0x45,	/* mov [3840h], byte 045h*/
	0xc6,0x06,0x42,0x38,0x54,	/* mov [3842h], byte 054h*/
	0x68,0x00,0xb0,				/* push 0b000h*/
	0x1f,						/* pop ds*/
	0xc6,0x06,0x00,0x09,0x49^0xff,	/* mov [0900h], byte 049h*/
	0xc6,0x06,0x00,0x0a,0x49^0xff,	/* mov [0a00h], byte 049h*/
	0xc6,0x06,0x00,0x0b,0x49^0xff,	/* mov [0b00h], byte 049h*/
	0xc6,0x06,0x20,0x09,0x49^0xff,	/* mov [0920h], byte 049h*/
	0xc6,0x06,0x21,0x09,0x4d^0xff,	/* mov [0921h], byte 04dh*/
	0xc6,0x06,0x22,0x09,0x41^0xff,	/* mov [0922h], byte 041h*/
	0xc6,0x06,0x23,0x09,0x47^0xff,	/* mov [0923h], byte 047h*/
	0x68,0x00,0xd0,				/* push 0d000h*/
	0x1f,						/* pop ds*/
	/* the game checks for*/
	/* "This game can only be played in Japan..." message in the video text buffer*/
	/* the message is nowhere to be found in the ROMs, so has to be displayed by the mcu*/
	0xc6,0x06,0xb0,0x1c,0x57,	/* mov [1cb0h], byte 057h*/
	0xea,0x00,0x00,0x40,0x00	/* jmp  0040:$0000*/
};
static unsigned char imgfight_crc[CRC_LEN] =  {	0x7e,0xcc,0xec,0x03, 0x04,0x33,0xb6,0xc5,
												0xbf,0x37,0x92,0x94, 0x00,0x00 };

/* Legend of Hero Tonma */
static unsigned char loht_code[CODE_LEN] =
{
	0x68,0x00,0xa0,				/* push 0a000h*/
	0x1f,						/* pop ds*/
	0xc6,0x06,0x3c,0x38,0x47,	/* mov [383ch], byte 047h*/
	0xc6,0x06,0x3d,0x38,0x47,	/* mov [383dh], byte 047h*/
	0xc6,0x06,0x42,0x38,0x44,	/* mov [3842h], byte 044h*/
	0xc6,0x06,0x43,0x38,0x44,	/* mov [3843h], byte 044h*/
	0x68,0x00,0xb0,				/* push 0b000h*/
	0x1f,						/* pop ds*/
	0xc6,0x06,0x00,0x09,0x49^0xff,	/* mov [0900h], byte 049h*/
	0xc6,0x06,0x00,0x0a,0x49^0xff,	/* mov [0a00h], byte 049h*/
	0xc6,0x06,0x00,0x0b,0x49^0xff,	/* mov [0b00h], byte 049h*/
	0xea,0x5d,0x01,0x40,0x00	/* jmp  0040:$015d*/
};
static unsigned char loht_crc[CRC_LEN] =	  {	0x39,0x00,0x82,0xae, 0x2c,0x9d,0x4b,0x73,
												0xfb,0xac,0xd4,0x6d, 0x6d,0x5b,0x77,0xc0, 0x00,0x00 };

/* X Multiply */
static unsigned char xmultipl_code[CODE_LEN] =
{
	0xea,0x30,0x02,0x00,0x0e	/* jmp  0e00:$0230*/
};
static unsigned char xmultipl_crc[CRC_LEN] =  {	0x73,0x82,0x4e,0x3f, 0xfc,0x56,0x59,0x06,
												0x05,0x48,0xa8,0xf4, 0x00,0x00 };

/* Dragon Breed */
static unsigned char dbreed_code[CODE_LEN] =
{
	0xea,0x6c,0x00,0x00,0x00	/* jmp  0000:$006c*/
};
static unsigned char dbreed_crc[CRC_LEN] =	  {	0xa4,0x96,0x5f,0xc0, 0xab,0x49,0x9f,0x19,
												0x84,0xe6,0xd6,0xca, 0x00,0x00 };

/* Air Duel */
static unsigned char airduel_code[CODE_LEN] =
{
	0x68,0x00,0xd0,				/* push 0d000h*/
	0x1f,						/* pop ds*/
	/* the game checks for*/
	/* "This game can only be played in Japan..." message in the video text buffer*/
	/* the message is nowhere to be found in the ROMs, so has to be displayed by the mcu*/
	0xc6,0x06,0xc0,0x1c,0x57,	/* mov [1cc0h], byte 057h*/
	0xea,0x69,0x0b,0x00,0x00	/* jmp  0000:$0b69*/
};
static unsigned char airduel_crc[CRC_LEN] =	  {	0x72,0x9c,0xca,0x85, 0xc9,0x12,0xcc,0xea,
												0x00,0x00 };

/* Daiku no Gensan */
static unsigned char dkgenm72_code[CODE_LEN] =
{
	0xea,0x3d,0x00,0x00,0x10	/* jmp  1000:$003d*/
};
static unsigned char dkgenm72_crc[CRC_LEN] =  {	0xc8,0xb4,0xdc,0xf8, 0xd3,0xba,0x48,0xed,
												0x79,0x08,0x1c,0xb3, 0x00,0x00 };


unsigned char *protection_code,*protection_crc;

static READ_HANDLER( protection_r )
{
	if (offset == 0xffb)
		memcpy(protection_ram,protection_code,CODE_LEN);

	return protection_ram[offset];
}

static WRITE_HANDLER( protection_w )
{
	protection_ram[offset] = data ^ 0xff;;

	if (offset == 0x0fff && data == 0)
		memcpy(&protection_ram[0x0fe0],protection_crc,CRC_LEN);
}

static void install_protection_handler(unsigned char *code,unsigned char *crc)
{
	protection_code = code;
	protection_crc =  crc;
	install_mem_read_handler (0,0xb0000,0xb0fff,protection_r);
	install_mem_write_handler(0,0xb0000,0xb0fff,protection_w);
	protection_ram = &memory_region(REGION_CPU1)[0xb0000];
}

static DRIVER_INIT( bchopper )
{
	install_protection_handler(bchopper_code,bchopper_crc);

	install_port_write_handler(0,0xc0,0xc0,bchopper_sample_trigger_w);
}

static DRIVER_INIT( mrheli )
{
	install_protection_handler(bchopper_code,mrheli_crc);

	install_port_write_handler(0,0xc0,0xc0,bchopper_sample_trigger_w);
}

static DRIVER_INIT( nspirit )
{
	install_protection_handler(nspirit_code,nspirit_crc);

	install_port_write_handler(0,0xc0,0xc0,nspirit_sample_trigger_w);
}

static DRIVER_INIT( nspiritj )
{
	install_protection_handler(nspirit_code,nspiritj_crc);

	install_port_write_handler(0,0xc0,0xc0,nspirit_sample_trigger_w);
}

static DRIVER_INIT( imgfight )
{
	install_protection_handler(imgfight_code,imgfight_crc);

	install_port_write_handler(0,0xc0,0xc0,imgfight_sample_trigger_w);
}

static DRIVER_INIT( loht )
{
	install_protection_handler(loht_code,loht_crc);

	install_port_write_handler(0,0xc0,0xc0,loht_sample_trigger_w);

	/* since we skip the startup tests, clear video RAM to prevent garbage on title screen */
	memset(m72_videoram2,0,0x4000);
}

static DRIVER_INIT( xmultipl )
{
	install_protection_handler(xmultipl_code,xmultipl_crc);

	install_port_write_handler(0,0xc0,0xc0,xmultipl_sample_trigger_w);
}

static DRIVER_INIT( dbreed )
{
	install_protection_handler(dbreed_code,dbreed_crc);

	install_port_write_handler(0,0xc0,0xc0,dbreed_sample_trigger_w);
}

static DRIVER_INIT( airduel )
{
	install_protection_handler(airduel_code,airduel_crc);

	install_port_write_handler(0,0xc0,0xc0,airduel_sample_trigger_w);
}

static DRIVER_INIT( dkgenm72 )
{
	install_protection_handler(dkgenm72_code,dkgenm72_crc);

	install_port_write_handler(0,0xc0,0xc0,dkgenm72_sample_trigger_w);
}

static DRIVER_INIT( gallop )
{
	install_port_write_handler(0,0xc0,0xc0,gallop_sample_trigger_w);
}




static unsigned char *soundram;


static READ_HANDLER( soundram_r )
{
	return soundram[offset];
}

static WRITE_HANDLER( soundram_w )
{
	soundram[offset] = data;
}


static READ_HANDLER( poundfor_trackball_r )
{
	static int prev[4],diff[4];

	if (offset == 0)
	{
		int i,curr;

		for (i = 0;i < 4;i++)
		{
			curr = readinputport(6+i);
			diff[i] = (curr - prev[i]);
			prev[i] = curr;
		}
	}

	switch (offset)
	{
		default:
		case 0:
			return diff[0] & 0xff;
		case 1:
			return diff[2] & 0xff;
		case 2:
			return ((diff[0] >> 8) & 0x1f) | (readinputport(0) & 0xe0);
		case 3:
			return ((diff[2] >> 8) & 0x1f) | (readinputport(1) & 0xe0);
		case 4:
			return diff[1] & 0xff;
		case 5:
			return diff[3] & 0xff;
		case 6:
			return ((diff[1] >> 8) & 0x1f);
		case 7:
			return ((diff[3] >> 8) & 0x1f);
	}
}


#define CPU1_MEMORY(NAME,ROMSIZE,WORKRAM) 						\
static MEMORY_READ_START( NAME##_readmem )						\
	{ 0x00000, ROMSIZE-1, MRA_ROM },							\
	{ WORKRAM, WORKRAM+0x3fff, MRA_RAM },						\
	{ 0xc0000, 0xc03ff, MRA_RAM },								\
	{ 0xc8000, 0xc8bff, m72_palette1_r },						\
	{ 0xcc000, 0xccbff, m72_palette2_r },						\
	{ 0xd0000, 0xd3fff, m72_videoram1_r },						\
	{ 0xd8000, 0xdbfff, m72_videoram2_r },						\
	{ 0xe0000, 0xeffff, soundram_r },							\
MEMORY_END														\
																\
static MEMORY_WRITE_START( NAME##_writemem )					\
	{ 0x00000, ROMSIZE-1, MWA_ROM },							\
	{ WORKRAM, WORKRAM+0x3fff, MWA_RAM },	/* work RAM */		\
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },	\
	{ 0xc8000, 0xc8bff, m72_palette1_w, &paletteram },			\
	{ 0xcc000, 0xccbff, m72_palette2_w, &paletteram_2 },		\
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },		\
	{ 0xd8000, 0xdbfff, m72_videoram2_w, &m72_videoram2 },		\
	{ 0xe0000, 0xeffff, soundram_w },							\
MEMORY_END


/*                     ROMSIZE  WORKRAM */
CPU1_MEMORY( rtype,    0x40000, 0x40000 )
CPU1_MEMORY( m72,      0x80000, 0xa0000 )
CPU1_MEMORY( xmultipl, 0x80000, 0x80000 )
CPU1_MEMORY( dbreed,   0x80000, 0x90000 )


static MEMORY_READ_START( rtype2_readmem )
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc8bff, m72_palette1_r },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd4000, 0xd7fff, m72_videoram2_r },
	{ 0xd8000, 0xd8bff, m72_palette2_r },
	{ 0xe0000, 0xe3fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( rtype2_writemem )
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xb0000, 0xb0001, m72_irq_line_w },
	{ 0xbc000, 0xbc001, m72_dmaon_w },
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc8bff, m72_palette1_w, &paletteram },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd4000, 0xd7fff, m72_videoram2_w, &m72_videoram2 },
	{ 0xd8000, 0xd8bff, m72_palette2_w, &paletteram_2 },
	{ 0xe0000, 0xe3fff, MWA_RAM },	/* work RAM */
MEMORY_END

static MEMORY_READ_START( majtitle_readmem )
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xa0000, 0xa03ff, MRA_RAM },
	{ 0xa4000, 0xa4bff, m72_palette2_r },
	{ 0xac000, 0xaffff, m72_videoram1_r },
	{ 0xb0000, 0xbffff, m72_videoram2_r },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc83ff, MRA_RAM },
	{ 0xcc000, 0xccbff, m72_palette1_r },
	{ 0xd0000, 0xd3fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( majtitle_writemem )
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xa0000, 0xa03ff, MWA_RAM, &majtitle_rowscrollram },
	{ 0xa4000, 0xa4bff, m72_palette2_w, &paletteram_2 },
	{ 0xac000, 0xaffff, m72_videoram1_w, &m72_videoram1 },
	{ 0xb0000, 0xbffff, m72_videoram2_w, &m72_videoram2 },	/* larger than the other games */
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc83ff, MWA_RAM, &spriteram_2 },
	{ 0xcc000, 0xccbff, m72_palette1_w, &paletteram },
	{ 0xd0000, 0xd3fff, MWA_RAM },	/* work RAM */
	{ 0xe0000, 0xe0001, m72_irq_line_w },
	{ 0xe4000, 0xe4001, MWA_RAM },	/* playfield enable? 1 during screen transitions, 0 otherwise */
	{ 0xec000, 0xec001, m72_dmaon_w },
MEMORY_END

static MEMORY_READ_START( hharry_readmem )
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xa0000, 0xa3fff, MRA_RAM },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xc8000, 0xc8bff, m72_palette1_r },
	{ 0xcc000, 0xccbff, m72_palette2_r },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd8000, 0xdbfff, m72_videoram2_r },
MEMORY_END

static MEMORY_WRITE_START( hharry_writemem )
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xa0000, 0xa3fff, MWA_RAM },	/* work RAM */
	{ 0xb0ffe, 0xb0fff, MWA_RAM },	/* leftover from protection?? */
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc8000, 0xc8bff, m72_palette1_w, &paletteram },
	{ 0xcc000, 0xccbff, m72_palette2_w, &paletteram_2 },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd8000, 0xdbfff, m72_videoram2_w, &m72_videoram2 },
MEMORY_END

static MEMORY_READ_START( hharryu_readmem )
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xa0000, 0xa0bff, m72_palette1_r },
	{ 0xa8000, 0xa8bff, m72_palette2_r },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0xd0000, 0xd3fff, m72_videoram1_r },
	{ 0xd4000, 0xd7fff, m72_videoram2_r },
	{ 0xe0000, 0xe3fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( hharryu_writemem )
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xa0000, 0xa0bff, m72_palette1_w, &paletteram },
	{ 0xa8000, 0xa8bff, m72_palette2_w, &paletteram_2 },
	{ 0xb0000, 0xb0001, m72_irq_line_w },
	{ 0xbc000, 0xbc001, m72_dmaon_w },
	{ 0xb0ffe, 0xb0fff, MWA_RAM },	/* leftover from protection?? */
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd0000, 0xd3fff, m72_videoram1_w, &m72_videoram1 },
	{ 0xd4000, 0xd7fff, m72_videoram2_w, &m72_videoram2 },
	{ 0xe0000, 0xe3fff, MWA_RAM },	/* work RAM */
MEMORY_END

static MEMORY_READ_START( kengo_readmem )
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0xa0000, 0xa0bff, m72_palette1_r },
	{ 0xa8000, 0xa8bff, m72_palette2_r },
	{ 0xc0000, 0xc03ff, MRA_RAM },
	{ 0x80000, 0x83fff, m72_videoram1_r },
	{ 0x84000, 0x87fff, m72_videoram2_r },
	{ 0xe0000, 0xe3fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( kengo_writemem )
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0xa0000, 0xa0bff, m72_palette1_w, &paletteram },
	{ 0xa8000, 0xa8bff, m72_palette2_w, &paletteram_2 },
	{ 0xb0000, 0xb0001, m72_irq_line_w },
{ 0xb4000, 0xb4001, MWA_NOP },	/* ??? */
	{ 0xbc000, 0xbc001, m72_dmaon_w },
	{ 0xc0000, 0xc03ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x80000, 0x83fff, m72_videoram1_w, &m72_videoram1 },
	{ 0x84000, 0x87fff, m72_videoram2_w, &m72_videoram2 },
	{ 0xe0000, 0xe3fff, MWA_RAM },	/* work RAM */
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ 0x05, 0x05, input_port_5_r },
PORT_END

static PORT_READ_START( poundfor_readport )
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ 0x05, 0x05, input_port_5_r },
	{ 0x08, 0x0f, poundfor_trackball_r },
PORT_END


static PORT_WRITE_START( writeport )
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, m72_port02_w },	/* coin counters, reset sound cpu, other stuff? */
	{ 0x04, 0x05, m72_dmaon_w },
	{ 0x06, 0x07, m72_irq_line_w },
	{ 0x40, 0x43, MWA_NOP }, /* Interrupt controller, only written to at bootup */
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
/*	{ 0xc0, 0xc0      trigger sample, filled by init_ function */
PORT_END

static PORT_WRITE_START( xmultipl_writeport )
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, m72_port02_w },	/* coin counters, reset sound cpu, other stuff? */
	{ 0x04, 0x05, m72_dmaon_w },
	{ 0x06, 0x07, m72_irq_line_w },
	{ 0x40, 0x43, MWA_NOP }, /* Interrupt controller, only written to at bootup */
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
/*	{ 0xc0, 0xc0      trigger sample, filled by init_ function */
PORT_END

static PORT_WRITE_START( rtype2_writeport )
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, rtype2_port02_w },
	{ 0x40, 0x43, MWA_NOP }, /* Interrupt controller, only written to at bootup */
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
PORT_END

static PORT_WRITE_START( majtitle_writeport )
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, rtype2_port02_w },
	{ 0x40, 0x43, MWA_NOP }, /* Interrupt controller, only written to at bootup */
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
	{ 0x8e, 0x8f, majtitle_gfx_ctrl_w },
PORT_END

static PORT_WRITE_START( hharry_writeport )
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, rtype2_port02_w },	/* coin counters, reset sound cpu, other stuff? */
	{ 0x04, 0x05, m72_dmaon_w },
	{ 0x06, 0x07, m72_irq_line_w },
	{ 0x40, 0x43, MWA_NOP }, /* Interrupt controller, only written to at bootup */
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
PORT_END

static PORT_WRITE_START( kengo_writeport )
	{ 0x00, 0x01, m72_sound_command_w },
	{ 0x02, 0x03, rtype2_port02_w },
	{ 0x80, 0x81, m72_scrolly1_w },
	{ 0x82, 0x83, m72_scrollx1_w },
	{ 0x84, 0x85, m72_scrolly2_w },
	{ 0x86, 0x87, m72_scrollx2_w },
/*{ 0x8c, 0x8f, MWA_NOP },	 // ??? /*/
PORT_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xffff, MWA_RAM, &soundram },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x02, 0x02, soundlatch_r },
	{ 0x84, 0x84, m72_sample_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x06, 0x06, m72_sound_irq_ack_w },
	{ 0x82, 0x82, m72_sample_w },
PORT_END

static PORT_READ_START( rtype2_sound_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x80, 0x80, soundlatch_r },
	{ 0x84, 0x84, m72_sample_r },
PORT_END

static PORT_WRITE_START( rtype2_sound_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x80, 0x81, rtype2_sample_addr_w },
	{ 0x82, 0x82, m72_sample_w },
	{ 0x83, 0x83, m72_sound_irq_ack_w },
PORT_END

static PORT_READ_START( poundfor_sound_readport )
	{ 0x41, 0x41, YM2151_status_port_0_r },
	{ 0x42, 0x42, soundlatch_r },
PORT_END

static PORT_WRITE_START( poundfor_sound_writeport )
	{ 0x10, 0x13, poundfor_sample_addr_w },
	{ 0x40, 0x40, YM2151_register_port_0_w },
	{ 0x41, 0x41, YM2151_data_port_0_w },
	{ 0x42, 0x42, m72_sound_irq_ack_w },
PORT_END



#define JOYSTICK_1 \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )

#define JOYSTICK_2 \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )

#define COIN_MODE_1 \
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) ) \
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 8C_3C ) ) \
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 5C_3C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) ) \
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

#define COIN_MODE_2 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )


INPUT_PORTS_START( rtype )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50K 150K 250K 400K 600K" )
	PORT_DIPSETTING(    0x08, "100K 200K 350K 500K 700K"  )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/* identical but Demo Sounds is inverted */
INPUT_PORTS_START( rtypep )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50K 150K 250K 400K 600K" )
	PORT_DIPSETTING(    0x08, "100K 200K 350K 500K 700K"  )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( bchopper )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "80K 200K 350K" )
	PORT_DIPSETTING(    0x00, "100K 250K 400K" )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( nspirit )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( imgfight )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x04, "Hardest" )
	PORT_DIPSETTING(    0x00, "Debug Mode 2lap" )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( loht )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x08, "Hardest" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( xmultipl )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "4" )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( dbreed )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( rtype2 )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    /* Coin Mode 1 */
    COIN_MODE_1
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, "Upright (2P)" )
	PORT_DIPSETTING(    0x18, DEF_STR( Cocktail ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( hharry )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Continue Limit" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x04, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, "Upright (2P)" )
	PORT_DIPSETTING(    0x06, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit, 1 Coin/Continue" )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	/* Coin mode 2, not supported yet */
    /* COIN_MODE_2*/
INPUT_PORTS_END

INPUT_PORTS_START( poundfor )
	PORT_START
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* high bits of trackball X */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* high bits of trackball X */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Round Time" )
	PORT_DIPSETTING(    0x02, "60" )
	PORT_DIPSETTING(    0x03, "90" )
	PORT_DIPSETTING(    0x01, "120" )
	PORT_DIPSETTING(    0x00, "150" )
	PORT_DIPNAME( 0x04, 0x04, "Matches/Credit (2P)" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x08, 0x08, "Rounds/Match" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Trackball Size" )
	PORT_DIPSETTING(    0x20, "Small" )
	PORT_DIPSETTING(    0x00, "Large" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x04, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, "Upright (2P)" )
	PORT_DIPSETTING(    0x06, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )

	/* Coin Mode 1 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit, 1 Coin/Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_X | IPF_PLAYER1, 50, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER1, 50, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_X | IPF_REVERSE | IPF_PLAYER2, 50, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_Y | IPF_PLAYER2, 50, 30, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( airduel )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )

	/* Coin Mode 1 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
/*	PORT_DIPSETTING(    0x10, DEF_STR( Free-Play ) )   // another free play /*/
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
    /* Coin Mode 2, not supported yet */
    /* COIN_MODE_2*/
INPUT_PORTS_END

INPUT_PORTS_START( gallop )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, "Upright (2P)" )
	PORT_DIPSETTING(    0x06, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit, 1 Coin/Continue" )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	/* Coin mode 2, not supported yet */
    /* COIN_MODE_2*/
INPUT_PORTS_END

INPUT_PORTS_START( kengo )
	PORT_START
	JOYSTICK_1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	JOYSTICK_2
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* 0x20 is another test mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* sprite DMA complete */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty?" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )
	PORT_DIPSETTING(    0x08, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit, 1 Coin/Continue" )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	/* Coin mode 2, not supported yet */
    /* COIN_MODE_2*/
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,4),	/* NUM characters */
	4,	/* 4 bits per pixel */
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,4),	/* NUM characters */
	4,	/* 4 bits per pixel */
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo m72_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout,    0, 16 },
	{ REGION_GFX2, 0, &tilelayout,    256, 16 },
	{ REGION_GFX3, 0, &tilelayout,    256, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rtype2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout,     0, 16 },
	{ REGION_GFX2, 0, &tilelayout,     256, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo majtitle_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout,     0, 16 },
	{ REGION_GFX2, 0, &tilelayout,     256, 16 },
	{ REGION_GFX3, 0, &spritelayout,     0, 16 },
	{ -1 } /* end of array */
};



static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ m72_ym2151_irq_handler },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,	/* 1 channel */
	{ 40 }
};



static MACHINE_DRIVER_START( rtype )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(rtype_readmem,rtype_writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(m72)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(m72_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m72)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( m72 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(m72_readmem,m72_writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(fake_nmi,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(m72)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(m72_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m72)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dkgenm72 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(m72_readmem,m72_writemem)
	MDRV_CPU_PORTS(readport,xmultipl_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(fake_nmi,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(xmultipl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(m72_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m72)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( xmultipl )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(xmultipl_readmem,xmultipl_writemem)
	MDRV_CPU_PORTS(readport,xmultipl_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(xmultipl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(m72_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m72)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dbreed )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(dbreed_readmem,dbreed_writemem)
	MDRV_CPU_PORTS(readport,xmultipl_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(xmultipl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(m72_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(m72)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rtype2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(rtype2_readmem,rtype2_writemem)
	MDRV_CPU_PORTS(readport,rtype2_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(rtype2_sound_readport,rtype2_sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(m72)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(rtype2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(rtype2)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( majtitle )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(majtitle_readmem,majtitle_writemem)
	MDRV_CPU_PORTS(readport,majtitle_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(rtype2_sound_readport,rtype2_sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(m72)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(majtitle_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(majtitle)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(majtitle)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hharry )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(hharry_readmem,hharry_writemem)
	MDRV_CPU_PORTS(readport,hharry_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(rtype2_sound_readport,rtype2_sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(xmultipl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(rtype2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(hharry)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hharryu )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(hharryu_readmem,hharryu_writemem)
	MDRV_CPU_PORTS(readport,rtype2_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(rtype2_sound_readport,rtype2_sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(xmultipl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(rtype2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(rtype2)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( poundfor )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(rtype2_readmem,rtype2_writemem)
	MDRV_CPU_PORTS(poundfor_readport,rtype2_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(poundfor_sound_readport,poundfor_sound_writeport)
	MDRV_CPU_VBLANK_INT(fake_nmi,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(m72)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(rtype2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(poundfor)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kengo )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/4)	/* 16 MHz external freq (8MHz internal) */
	MDRV_CPU_MEMORY(kengo_readmem,kengo_writemem)
	MDRV_CPU_PORTS(readport,kengo_writeport)
	MDRV_CPU_VBLANK_INT(m72_interrupt,256)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(rtype2_sound_readport,rtype2_sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(kengo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 16*8, (64-16)*8-1 )
	MDRV_GFXDECODE(rtype2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(poundfor)
	MDRV_VIDEO_EOF(m72)
	MDRV_VIDEO_UPDATE(m72)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rtype )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "rt_r-h0-.bin", 0x00001, 0x10000, CRC(c2940df2) SHA1(cbccd205ef81a0e39990a34d46e3f7d52b62e385) )
	ROM_LOAD16_BYTE( "rt_r-l0-.bin", 0x00000, 0x10000, CRC(858cc0f6) SHA1(7a256fe3aa3a96e161dd485a90b18c421b61458b) )
	ROM_LOAD16_BYTE( "rt_r-h1-.bin", 0x20001, 0x10000, CRC(5bcededa) SHA1(4ada3fd207fa57751f8e3d885bc91b374e27035d) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "rt_r-l1-.bin", 0x20000, 0x10000, CRC(4821141c) SHA1(df6cf04c3ecd04b6f27a96871848904575414dae) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-00.bin",   0x00000, 0x10000, CRC(dad53bc0) SHA1(1e3bc498861946278a0b1fe24259f5d224e265d7) )	/* sprites */
	ROM_LOAD( "cpu-01.bin",   0x10000, 0x10000, CRC(b28d1a60) SHA1(26d0b5a381f5b462a223cb474067c7b5e67706df) )
	ROM_LOAD( "cpu-10.bin",   0x20000, 0x10000, CRC(d6a66298) SHA1(d2873d05aa3b257e7699c188880ac3daad672fa5) )
	ROM_LOAD( "cpu-11.bin",   0x30000, 0x10000, CRC(bb182f1a) SHA1(93b24bb0d6abd5c42c84a041c79b1dcea568e443) )
	ROM_LOAD( "cpu-20.bin",   0x40000, 0x10000, CRC(fc247c8a) SHA1(01cf0a60f47fa5e2ed430a3f075e69e6cb762a48) )
	ROM_LOAD( "cpu-21.bin",   0x50000, 0x10000, CRC(5b41f5f3) SHA1(8f47a77fb6513b8f611671d34721e33d284dc035) )
	ROM_LOAD( "cpu-30.bin",   0x60000, 0x10000, CRC(eb02a1cb) SHA1(60a394ab53afdcbbf9e88083b8dbe8c897170d77) )
	ROM_LOAD( "cpu-31.bin",   0x70000, 0x10000, CRC(2bec510a) SHA1(3455e72095832135ac006944a1209e720ec5c8bf) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-a0.bin",   0x00000, 0x08000, CRC(4e212fb0) SHA1(687061ecade2ebd0bd1343c9c4a831791853f79c) )	/* tiles #1 */
	ROM_LOAD( "cpu-a1.bin",   0x08000, 0x08000, CRC(8a65bdff) SHA1(130bf6af521f13247a739a95eab4bdaa24b2ac10) )
	ROM_LOAD( "cpu-a2.bin",   0x10000, 0x08000, CRC(5a4ae5b9) SHA1(95c3b64f50e6f673b2bf9b40642c152da5009d25) )
	ROM_LOAD( "cpu-a3.bin",   0x18000, 0x08000, CRC(73327606) SHA1(9529ecdedd30e2a0400fb1083117992cc18b5158) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-b0.bin",   0x00000, 0x08000, CRC(a7b17491) SHA1(5b390770e56ba2d35e108534d7eda8dca996fdf7) )	/* tiles #2 */
	ROM_LOAD( "cpu-b1.bin",   0x08000, 0x08000, CRC(b9709686) SHA1(700905a3e9661e0874939f54da2909e1396ce596) )
	ROM_LOAD( "cpu-b2.bin",   0x10000, 0x08000, CRC(433b229a) SHA1(14222eaa3e67e5a7f80eafcf22bac4eb2d485a9a) )
	ROM_LOAD( "cpu-b3.bin",   0x18000, 0x08000, CRC(ad89b072) SHA1(e2683d0e7415f3abd147e518bf6c87e44744cd4f) )
ROM_END

ROM_START( rtypepj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "db_b1.bin",   0x00001, 0x10000, CRC(c1865141) SHA1(3302b6529aa903d81eb2196d745eb4f7f8316857) )
	ROM_LOAD16_BYTE( "db_a1.bin",   0x00000, 0x10000, CRC(5ad2bd90) SHA1(0937dbbdf0cbce2e81cecf4d770bbd8c6bd82801) )
	ROM_LOAD16_BYTE( "db_b2.bin",   0x20001, 0x10000, CRC(b4f6407e) SHA1(4a00d8e104c580900b4feb318dd162b77b71d0a5) )
	ROM_RELOAD(                     0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "db_a2.bin",   0x20000, 0x10000, CRC(6098d86f) SHA1(c6c9c1c2c30d5f190c40e000004bd21606efb8b0) )
	ROM_RELOAD(                     0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-00.bin",   0x00000, 0x10000, CRC(dad53bc0) SHA1(1e3bc498861946278a0b1fe24259f5d224e265d7) )	/* sprites */
	ROM_LOAD( "cpu-01.bin",   0x10000, 0x10000, CRC(b28d1a60) SHA1(26d0b5a381f5b462a223cb474067c7b5e67706df) )
	ROM_LOAD( "cpu-10.bin",   0x20000, 0x10000, CRC(d6a66298) SHA1(d2873d05aa3b257e7699c188880ac3daad672fa5) )
	ROM_LOAD( "cpu-11.bin",   0x30000, 0x10000, CRC(bb182f1a) SHA1(93b24bb0d6abd5c42c84a041c79b1dcea568e443) )
	ROM_LOAD( "cpu-20.bin",   0x40000, 0x10000, CRC(fc247c8a) SHA1(01cf0a60f47fa5e2ed430a3f075e69e6cb762a48) )
	ROM_LOAD( "cpu-21.bin",   0x50000, 0x10000, CRC(5b41f5f3) SHA1(8f47a77fb6513b8f611671d34721e33d284dc035) )
	ROM_LOAD( "cpu-30.bin",   0x60000, 0x10000, CRC(eb02a1cb) SHA1(60a394ab53afdcbbf9e88083b8dbe8c897170d77) )
	ROM_LOAD( "cpu-31.bin",   0x70000, 0x10000, CRC(2bec510a) SHA1(3455e72095832135ac006944a1209e720ec5c8bf) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-a0.bin",   0x00000, 0x08000, CRC(4e212fb0) SHA1(687061ecade2ebd0bd1343c9c4a831791853f79c) )	/* tiles #1 */
	ROM_LOAD( "cpu-a1.bin",   0x08000, 0x08000, CRC(8a65bdff) SHA1(130bf6af521f13247a739a95eab4bdaa24b2ac10) )
	ROM_LOAD( "cpu-a2.bin",   0x10000, 0x08000, CRC(5a4ae5b9) SHA1(95c3b64f50e6f673b2bf9b40642c152da5009d25) )
	ROM_LOAD( "cpu-a3.bin",   0x18000, 0x08000, CRC(73327606) SHA1(9529ecdedd30e2a0400fb1083117992cc18b5158) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-b0.bin",   0x00000, 0x08000, CRC(a7b17491) SHA1(5b390770e56ba2d35e108534d7eda8dca996fdf7) )	/* tiles #2 */
	ROM_LOAD( "cpu-b1.bin",   0x08000, 0x08000, CRC(b9709686) SHA1(700905a3e9661e0874939f54da2909e1396ce596) )
	ROM_LOAD( "cpu-b2.bin",   0x10000, 0x08000, CRC(433b229a) SHA1(14222eaa3e67e5a7f80eafcf22bac4eb2d485a9a) )
	ROM_LOAD( "cpu-b3.bin",   0x18000, 0x08000, CRC(ad89b072) SHA1(e2683d0e7415f3abd147e518bf6c87e44744cd4f) )
ROM_END

ROM_START( rtypeu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "aud-h0.bin",   0x00001, 0x10000, CRC(36008a4e) SHA1(832006cb14a34e1671e305cc8ae606c3c6185a6a) )
	ROM_LOAD16_BYTE( "aud-l0.bin",   0x00000, 0x10000, CRC(4aaa668e) SHA1(87059460b59f43f2ca8cd959d76f721facd9de96) )
	ROM_LOAD16_BYTE( "aud-h1.bin",   0x20001, 0x10000, CRC(7ebb2a53) SHA1(1466df19888c3374847eb77f702060647e49d6ad) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "aud-l1.bin",   0x20000, 0x10000, CRC(c28b103b) SHA1(f294a23c3917b97812eb4c7f3a99253fd0cbb7ea) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-00.bin",   0x00000, 0x10000, CRC(dad53bc0) SHA1(1e3bc498861946278a0b1fe24259f5d224e265d7) )	/* sprites */
	ROM_LOAD( "cpu-01.bin",   0x10000, 0x10000, CRC(b28d1a60) SHA1(26d0b5a381f5b462a223cb474067c7b5e67706df) )
	ROM_LOAD( "cpu-10.bin",   0x20000, 0x10000, CRC(d6a66298) SHA1(d2873d05aa3b257e7699c188880ac3daad672fa5) )
	ROM_LOAD( "cpu-11.bin",   0x30000, 0x10000, CRC(bb182f1a) SHA1(93b24bb0d6abd5c42c84a041c79b1dcea568e443) )
	ROM_LOAD( "cpu-20.bin",   0x40000, 0x10000, CRC(fc247c8a) SHA1(01cf0a60f47fa5e2ed430a3f075e69e6cb762a48) )
	ROM_LOAD( "cpu-21.bin",   0x50000, 0x10000, CRC(5b41f5f3) SHA1(8f47a77fb6513b8f611671d34721e33d284dc035) )
	ROM_LOAD( "cpu-30.bin",   0x60000, 0x10000, CRC(eb02a1cb) SHA1(60a394ab53afdcbbf9e88083b8dbe8c897170d77) )
	ROM_LOAD( "cpu-31.bin",   0x70000, 0x10000, CRC(2bec510a) SHA1(3455e72095832135ac006944a1209e720ec5c8bf) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-a0.bin",   0x00000, 0x08000, CRC(4e212fb0) SHA1(687061ecade2ebd0bd1343c9c4a831791853f79c) )	/* tiles #1 */
	ROM_LOAD( "cpu-a1.bin",   0x08000, 0x08000, CRC(8a65bdff) SHA1(130bf6af521f13247a739a95eab4bdaa24b2ac10) )
	ROM_LOAD( "cpu-a2.bin",   0x10000, 0x08000, CRC(5a4ae5b9) SHA1(95c3b64f50e6f673b2bf9b40642c152da5009d25) )
	ROM_LOAD( "cpu-a3.bin",   0x18000, 0x08000, CRC(73327606) SHA1(9529ecdedd30e2a0400fb1083117992cc18b5158) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu-b0.bin",   0x00000, 0x08000, CRC(a7b17491) SHA1(5b390770e56ba2d35e108534d7eda8dca996fdf7) )	/* tiles #2 */
	ROM_LOAD( "cpu-b1.bin",   0x08000, 0x08000, CRC(b9709686) SHA1(700905a3e9661e0874939f54da2909e1396ce596) )
	ROM_LOAD( "cpu-b2.bin",   0x10000, 0x08000, CRC(433b229a) SHA1(14222eaa3e67e5a7f80eafcf22bac4eb2d485a9a) )
	ROM_LOAD( "cpu-b3.bin",   0x18000, 0x08000, CRC(ad89b072) SHA1(e2683d0e7415f3abd147e518bf6c87e44744cd4f) )
ROM_END

ROM_START( bchopper )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "c-h0-b.rom",   0x00001, 0x10000, CRC(f2feab16) SHA1(03ee874658e0f59957f8425e1ebf9c938737cc19) )
	ROM_LOAD16_BYTE( "c-l0-b.rom",   0x00000, 0x10000, CRC(9f887096) SHA1(4f41ef29580fc026ea91d110ec6b2e6af83dbd9a) )
	ROM_LOAD16_BYTE( "c-h1-b.rom",   0x20001, 0x10000, CRC(a995d64f) SHA1(43eb2eb11e6875298a6ef2b18f0f5e587f1bba16) )
	ROM_LOAD16_BYTE( "c-l1-b.rom",   0x20000, 0x10000, CRC(41dda999) SHA1(4d07a399aaf16bc37b5488e3e4bb60e78811a099) )
	ROM_LOAD16_BYTE( "c-h3-b.rom",   0x60001, 0x10000, CRC(ab9451ca) SHA1(ec0e0ad592d8b21bb4e6927a452e3b7964cda015) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "c-l3-b.rom",   0x60000, 0x10000, CRC(11562221) SHA1(a2f136a487fb6f30350e8d1e26c0729eb0686c7d) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c-00-a.rom",   0x00000, 0x10000, CRC(f6e6e660) SHA1(e066e5ed37719cf2b6fd36e0117f11325bb06f9c) )	/* sprites */
	ROM_LOAD( "c-01-b.rom",   0x10000, 0x10000, CRC(708cdd37) SHA1(24f3fcd381422f0d75410c2af7a56744e3b4a699) )
	ROM_LOAD( "c-10-a.rom",   0x20000, 0x10000, CRC(292c8520) SHA1(c552090d295ee1c1ca611b0cddee356e509e2045) )
	ROM_LOAD( "c-11-b.rom",   0x30000, 0x10000, CRC(20904cf3) SHA1(71fe505f2da53c2eb445b7b758d257d6af42e6f1) )
	ROM_LOAD( "c-20-a.rom",   0x40000, 0x10000, CRC(1ab50c23) SHA1(43e2f11e5bbf157c47764e04e372f40ed68bab59) )
	ROM_LOAD( "c-21-b.rom",   0x50000, 0x10000, CRC(c823d34c) SHA1(47383214b6a60e0b1b70208b00c291f8ffed36bc) )
	ROM_LOAD( "c-30-a.rom",   0x60000, 0x10000, CRC(11f6c56b) SHA1(39a2a674698b044c84fea65ae41a9e003a50b639) )
	ROM_LOAD( "c-31-b.rom",   0x70000, 0x10000, CRC(23134ec5) SHA1(43453f8a13b51310e04729dc828d391ca9c04da2) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b-a0-b.rom",   0x00000, 0x10000, CRC(e46ed7bf) SHA1(75abb5f40629f7c40a610a44e068b6c4e3a5126e) )	/* tiles #1 */
	ROM_LOAD( "b-a1-b.rom",   0x10000, 0x10000, CRC(590605ff) SHA1(fbb5c0cebd28b08d4ce39db4055d6343620e0f1c) )
	ROM_LOAD( "b-a2-b.rom",   0x20000, 0x10000, CRC(f8158226) SHA1(bb3a8686cd89bb8265b6b9e03682cc0bf6533793) )
	ROM_LOAD( "b-a3-b.rom",   0x30000, 0x10000, CRC(0f07b9b7) SHA1(63dbec17097f07eb39299372b736fbbc1b11b65e) )

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "b-b0-.rom",    0x00000, 0x10000, CRC(b5b95776) SHA1(4685b56071b916ce712c45f24da8068dd7e40ed1) )	/* tiles #2 */
	ROM_LOAD( "b-b1-.rom",    0x10000, 0x10000, CRC(74ca16ee) SHA1(7984bc9a0b46e1b4a8ecac7528d57606305aad73) )
	ROM_LOAD( "b-b2-.rom",    0x20000, 0x10000, CRC(b82cca04) SHA1(c12b95be311205181b01d15021bcf9f01ed3e0a3) )
	ROM_LOAD( "b-b3-.rom",    0x30000, 0x10000, CRC(a7afc920) SHA1(92c75463ada39184e731b82ef2883ae6f1f67482) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "c-v0-b.rom",   0x00000, 0x10000, CRC(d0c27e58) SHA1(fec76217cc0c04c723989c3ec127a2bd33d64c60) )
ROM_END

ROM_START( mrheli )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mh-c-h0.bin",  0x00001, 0x10000, CRC(e2ca5646) SHA1(9f4fe2f0a45233325bd9336cabb925a1f625453b) )
	ROM_LOAD16_BYTE( "mh-c-l0.bin",  0x00000, 0x10000, CRC(643e23cd) SHA1(66998a6dfc7ef538540986b61d2414a5ef250d0d) )
	ROM_LOAD16_BYTE( "mh-c-h1.bin",  0x20001, 0x10000, CRC(8974e84d) SHA1(39e05c80e805dde45f2fc5fc429b75f9b599089c) )
	ROM_LOAD16_BYTE( "mh-c-l1.bin",  0x20000, 0x10000, CRC(5f8bda69) SHA1(48629d617bd48c9de9c6a567fb203258a56fdbbd) )
	ROM_LOAD16_BYTE( "mh-c-h3.bin",  0x60001, 0x10000, CRC(143f596e) SHA1(f9d444eebcd53dac925d14b7a2858803b7fd9ce2) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "mh-c-l3.bin",  0x60000, 0x10000, CRC(c0982536) SHA1(45399f8d0577c6e2a277a69303954ce5d2de7c07) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mh-c-00.bin",  0x00000, 0x20000, CRC(dec4e121) SHA1(92169b523f1600e994e016dc1959a52958e1d89d) )	/* sprites */
	ROM_LOAD( "mh-c-10.bin",  0x20000, 0x20000, CRC(7aaa151e) SHA1(efd980bb2eed7084354b7a4aa2f733cd2f876741) )
	ROM_LOAD( "mh-c-20.bin",  0x40000, 0x20000, CRC(eae0de74) SHA1(3a2469c0eeb18131f989807afb50228f57ccea30) )
	ROM_LOAD( "mh-c-30.bin",  0x60000, 0x20000, CRC(01d5052f) SHA1(5d5e70913bb7af48193c70209595f27a64fa6cac) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mh-b-a0.bin",  0x00000, 0x10000, CRC(6a0db256) SHA1(fa3a2dc03da5bbe06a9c9b3d4ed4fddb47c469ac) )	/* tiles #1 */
	ROM_LOAD( "mh-b-a1.bin",  0x10000, 0x10000, CRC(14ec9795) SHA1(4842e076115efe9daf00dab8f61516d28c19baae) )
	ROM_LOAD( "mh-b-a2.bin",  0x20000, 0x10000, CRC(dfcb510e) SHA1(2387cde4ec0bae176486e1f7541103fd557fe255) )
	ROM_LOAD( "mh-b-a3.bin",  0x30000, 0x10000, CRC(957e329b) SHA1(9d48a0b84915e1cef0b0311a3581991dc83ee199) )

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "b-b0-.rom",    0x00000, 0x10000, CRC(b5b95776) SHA1(4685b56071b916ce712c45f24da8068dd7e40ed1) )	/* tiles #2 */
	ROM_LOAD( "b-b1-.rom",    0x10000, 0x10000, CRC(74ca16ee) SHA1(7984bc9a0b46e1b4a8ecac7528d57606305aad73) )
	ROM_LOAD( "b-b2-.rom",    0x20000, 0x10000, CRC(b82cca04) SHA1(c12b95be311205181b01d15021bcf9f01ed3e0a3) )
	ROM_LOAD( "b-b3-.rom",    0x30000, 0x10000, CRC(a7afc920) SHA1(92c75463ada39184e731b82ef2883ae6f1f67482) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "c-v0-b.rom",   0x00000, 0x10000, CRC(d0c27e58) SHA1(fec76217cc0c04c723989c3ec127a2bd33d64c60) )
ROM_END

ROM_START( nspirit )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "nin-c-h0.rom", 0x00001, 0x10000, CRC(035692fa) SHA1(d5ab54488344bf405063737ed55d68ff1e64b55f) )
	ROM_LOAD16_BYTE( "nin-c-l0.rom", 0x00000, 0x10000, CRC(9a405898) SHA1(b28d71c1a6410720a37e6b6518b3cc66d4c32972) )
	ROM_LOAD16_BYTE( "nin-c-h1.rom", 0x20001, 0x10000, CRC(cbc10586) SHA1(9b1935ea9ebb21fe42ee3a57d6c10f1e8516f23c) )
	ROM_LOAD16_BYTE( "nin-c-l1.rom", 0x20000, 0x10000, CRC(b75c9a4d) SHA1(03c28896cbe0c9f778c259d59d2e69796902daa8) )
	ROM_LOAD16_BYTE( "nin-c-h2.rom", 0x40001, 0x10000, CRC(8ad818fa) SHA1(dd25e79b656b7fc6c31d1f8971fd0916295ccdb0) )
	ROM_LOAD16_BYTE( "nin-c-l2.rom", 0x40000, 0x10000, CRC(c52ca78c) SHA1(2b40cce5a1f5c588b49634e7fd4bc28c9160fe43) )
	ROM_LOAD16_BYTE( "nin-c-h3.rom", 0x60001, 0x10000, CRC(501104ef) SHA1(e44e060c072affd359e52bf6606b1dd565368d44) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "nin-c-l3.rom", 0x60000, 0x10000, CRC(fd7408b8) SHA1(3cbe72835a561c50265a047f0f5cd62db48378fd) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "nin-r00.rom",  0x00000, 0x20000, CRC(5f61d30b) SHA1(7754697e43f6117fa604f50885b76014b1dc5760) )	/* sprites */
	ROM_LOAD( "nin-r10.rom",  0x20000, 0x20000, CRC(0caad107) SHA1(c4eff00327313e05ac8f7c6dbee3a0de1c83fadd) )
	ROM_LOAD( "nin-r20.rom",  0x40000, 0x20000, CRC(ef3617d3) SHA1(16c175cf45559aacdea6e4002dd8a87f16817cfb) )
	ROM_LOAD( "nin-r30.rom",  0x60000, 0x20000, CRC(175d2a24) SHA1(d1887efd4d8e74c38c53dbbc541ca8d17f29eb59) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "nin-b-a0.rom", 0x00000, 0x10000, CRC(63f8f658) SHA1(82c02d0f7a2d95dfd8d300c46312d511524775ce) )	/* tiles #1 */
	ROM_LOAD( "nin-b-a1.rom", 0x10000, 0x10000, CRC(75eb8306) SHA1(2abc359a0bb2863759a68ed60e730761b9751829) )
	ROM_LOAD( "nin-b-a2.rom", 0x20000, 0x10000, CRC(df532172) SHA1(58b5a79a57e71405b3e1abd41d54cf6a4d12873a) )
	ROM_LOAD( "nin-b-a3.rom", 0x30000, 0x10000, CRC(4dedd64c) SHA1(8a5c73a024d95e6fe3ab70daafcd5b235418ad36) )

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "nin-b0.rom",   0x00000, 0x10000, CRC(1b0e08a6) SHA1(892686594970c264babbe8673c258929a5e480f6) )	/* tiles #2 */
	ROM_LOAD( "nin-b1.rom",   0x10000, 0x10000, CRC(728727f0) SHA1(2f594c77a847ebee71c9da8a644f83ea2a1313d7) )
	ROM_LOAD( "nin-b2.rom",   0x20000, 0x10000, CRC(f87efd75) SHA1(16474c7ab57b4fbb5cb50799ea6a2326c66706b5) )
	ROM_LOAD( "nin-b3.rom",   0x30000, 0x10000, CRC(98856cb4) SHA1(aa4fbae972d2e827c75650a71ab4ef73a33cd018) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "nin-v0.rom",   0x00000, 0x10000, CRC(a32e8caf) SHA1(63d56ad3a63fb089056e4a170159120287594ea8) )
ROM_END

ROM_START( nspiritj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "c-h0",         0x00001, 0x10000, CRC(8603fab2) SHA1(2c5bc97b6c9648156969b4a9f139081dca19fa24) )
	ROM_LOAD16_BYTE( "c-l0",         0x00000, 0x10000, CRC(e520fa35) SHA1(05f7e5a1a5ada95809ffd941080fb2c2b54363b7) )
	ROM_LOAD16_BYTE( "nin-c-h1.rom", 0x20001, 0x10000, CRC(cbc10586) SHA1(9b1935ea9ebb21fe42ee3a57d6c10f1e8516f23c) )
	ROM_LOAD16_BYTE( "nin-c-l1.rom", 0x20000, 0x10000, CRC(b75c9a4d) SHA1(03c28896cbe0c9f778c259d59d2e69796902daa8) )
	ROM_LOAD16_BYTE( "nin-c-h2.rom", 0x40001, 0x10000, CRC(8ad818fa) SHA1(dd25e79b656b7fc6c31d1f8971fd0916295ccdb0) )
	ROM_LOAD16_BYTE( "nin-c-l2.rom", 0x40000, 0x10000, CRC(c52ca78c) SHA1(2b40cce5a1f5c588b49634e7fd4bc28c9160fe43) )
	ROM_LOAD16_BYTE( "c-h3",         0x60001, 0x10000, CRC(95b63a61) SHA1(bd5ec35fffe6d4898e6712eb6add7c51077b58d2) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "c-l3",         0x60000, 0x10000, CRC(e754a87a) SHA1(9951d972ed13a0415c827beff122bc7ddb078447) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "nin-r00.rom",  0x00000, 0x20000, CRC(5f61d30b) SHA1(7754697e43f6117fa604f50885b76014b1dc5760) )	/* sprites */
	ROM_LOAD( "nin-r10.rom",  0x20000, 0x20000, CRC(0caad107) SHA1(c4eff00327313e05ac8f7c6dbee3a0de1c83fadd) )
	ROM_LOAD( "nin-r20.rom",  0x40000, 0x20000, CRC(ef3617d3) SHA1(16c175cf45559aacdea6e4002dd8a87f16817cfb) )
	ROM_LOAD( "nin-r30.rom",  0x60000, 0x20000, CRC(175d2a24) SHA1(d1887efd4d8e74c38c53dbbc541ca8d17f29eb59) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "nin-b-a0.rom", 0x00000, 0x10000, CRC(63f8f658) SHA1(82c02d0f7a2d95dfd8d300c46312d511524775ce) )	/* tiles #1 */
	ROM_LOAD( "nin-b-a1.rom", 0x10000, 0x10000, CRC(75eb8306) SHA1(2abc359a0bb2863759a68ed60e730761b9751829) )
	ROM_LOAD( "nin-b-a2.rom", 0x20000, 0x10000, CRC(df532172) SHA1(58b5a79a57e71405b3e1abd41d54cf6a4d12873a) )
	ROM_LOAD( "nin-b-a3.rom", 0x30000, 0x10000, CRC(4dedd64c) SHA1(8a5c73a024d95e6fe3ab70daafcd5b235418ad36) )

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "nin-b0.rom",   0x00000, 0x10000, CRC(1b0e08a6) SHA1(892686594970c264babbe8673c258929a5e480f6) )	/* tiles #2 */
	ROM_LOAD( "nin-b1.rom",   0x10000, 0x10000, CRC(728727f0) SHA1(2f594c77a847ebee71c9da8a644f83ea2a1313d7) )
	ROM_LOAD( "nin-b2.rom",   0x20000, 0x10000, CRC(f87efd75) SHA1(16474c7ab57b4fbb5cb50799ea6a2326c66706b5) )
	ROM_LOAD( "nin-b3.rom",   0x30000, 0x10000, CRC(98856cb4) SHA1(aa4fbae972d2e827c75650a71ab4ef73a33cd018) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "nin-v0.rom",   0x00000, 0x10000, CRC(a32e8caf) SHA1(63d56ad3a63fb089056e4a170159120287594ea8) )
ROM_END

ROM_START( imgfight )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "if-c-h0.bin",  0x00001, 0x10000, CRC(592d2d80) SHA1(d54916a9bfe4b65a972b62202af706135e73518d) )
	ROM_LOAD16_BYTE( "if-c-l0.bin",  0x00000, 0x10000, CRC(61f89056) SHA1(3e0724dbc2b00a30193ea6cfac8b4331055d4fd4) )
	ROM_LOAD16_BYTE( "if-c-h3.bin",  0x40001, 0x20000, CRC(ea030541) SHA1(ee4c12773ecced2d755443ce0ca78fb2b2c04805) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "if-c-l3.bin",  0x40000, 0x20000, CRC(c66ae348) SHA1(eca5096ebd5bffc6e68f3fc9969cda9679bd921f) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "if-c-00.bin",  0x00000, 0x20000, CRC(745e6638) SHA1(43fb1f9da4190fea67eee3aee8caf4219becc21b) )	/* sprites */
	ROM_LOAD( "if-c-10.bin",  0x20000, 0x20000, CRC(b7108449) SHA1(1f41ebe7164fab86958caaf6749b99425e682657) )
	ROM_LOAD( "if-c-20.bin",  0x40000, 0x20000, CRC(aef33cba) SHA1(2d8a8458207d0c790c81b1285366463c8540d190) )
	ROM_LOAD( "if-c-30.bin",  0x60000, 0x20000, CRC(1f98e695) SHA1(5fddcfb17523f8e96f4b85f0cb15d837b81f2bd4) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "if-a-a0.bin",  0x00000, 0x10000, CRC(34ee2d77) SHA1(38826e0318aa8da893fa4c93f217288c015df606) )	/* tiles #1 */
	ROM_LOAD( "if-a-a1.bin",  0x10000, 0x10000, CRC(6bd2845b) SHA1(149cf14f919590da88b9a8e254690da010709862) )
	ROM_LOAD( "if-a-a2.bin",  0x20000, 0x10000, CRC(090d50e5) SHA1(4f2a7c76320b3f8dafae90a246187e034fe7562b) )
	ROM_LOAD( "if-a-a3.bin",  0x30000, 0x10000, CRC(3a8e3083) SHA1(8a75d556790b6bea41ead1a5f95589dd293bdf4e) )

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "if-a-b0.bin",  0x00000, 0x10000, CRC(b425c829) SHA1(0ccd487dba00bb7cb0ff5d1c67f8fee3e68df5d8) )	/* tiles #2 */
	ROM_LOAD( "if-a-b1.bin",  0x10000, 0x10000, CRC(e9bfe23e) SHA1(f97a68dbdce7e06d07faab19acf7625cdc8eeaa8) )
	ROM_LOAD( "if-a-b2.bin",  0x20000, 0x10000, CRC(256e50f2) SHA1(9e9fda4f1f1449548942c0da4478f61fe0d263d1) )
	ROM_LOAD( "if-a-b3.bin",  0x30000, 0x10000, CRC(4c682785) SHA1(f61f1227e0ad629fdfca106306b17a9f6a9959e3) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "if-c-v0.bin",  0x00000, 0x10000, CRC(cb64a194) SHA1(940fad6b9147bccc8290e112f5973f8ea062b52f) )
	ROM_LOAD( "if-c-v1.bin",  0x10000, 0x10000, CRC(45b68bf5) SHA1(2fb28793019ca85b3b6d7c4c31eedff1d71f2d83) )
ROM_END

ROM_START( loht )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "tom_c-h0.rom", 0x00001, 0x20000, CRC(a63204b6) SHA1(d217bc70650a1a1bbe0cf536ec3bb678f670718d) )
	ROM_LOAD16_BYTE( "tom_c-l0.rom", 0x00000, 0x20000, CRC(e788002f) SHA1(35f509976b342fd47e645453381faa3d86645876) )
	ROM_LOAD16_BYTE( "tom_c-h3.rom", 0x40001, 0x20000, CRC(714778b5) SHA1(e2eaa35d6b5fa5df5163fe0d7b45fa66667f9947) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "tom_c-l3.rom", 0x40000, 0x20000, CRC(2f049b03) SHA1(21047cb10912b1fc23795673af3ea7de249328b7) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tom_m53.rom",  0x00000, 0x20000, CRC(0b83265f) SHA1(b31918d6442b79c9fe4f20410189788b050a994e) )	/* sprites */
	ROM_LOAD( "tom_m51.rom",  0x20000, 0x20000, CRC(8ec5f6f3) SHA1(210f2753f5eeb06396758d21ab1778d459add247) )
	ROM_LOAD( "tom_m49.rom",  0x40000, 0x20000, CRC(a41d3bfd) SHA1(536fb7c0321dbbc1a8b73e9647fba9c53a253fcc) )
	ROM_LOAD( "tom_m47.rom",  0x60000, 0x20000, CRC(9d81a25b) SHA1(a354537c2fbba85f06485aa8487d7583a7133357) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tom_m21.rom",  0x00000, 0x10000, CRC(3ca3e771) SHA1(be052e01c5429ee89057c9d408794f2c7744047c) )	/* tiles #1 */
	ROM_LOAD( "tom_m22.rom",  0x10000, 0x10000, CRC(7a05ee2f) SHA1(7d1ca5db9a5a85610129e3bc6c640ade036fe7f9) )
	ROM_LOAD( "tom_m20.rom",  0x20000, 0x10000, CRC(79aa2335) SHA1(6b70c79d800a7b755aa7c9a368c4ea74029aaa1e) )
	ROM_LOAD( "tom_m23.rom",  0x30000, 0x10000, CRC(789e8b24) SHA1(e957cd25c3c155ca295ab1aea03d610f91562cfb) )

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tom_m26.rom",  0x00000, 0x10000, CRC(44626bf6) SHA1(571ef74d42d30a272ff0fb33f830652b4a4bad29) )	/* tiles #2 */
	ROM_LOAD( "tom_m27.rom",  0x10000, 0x10000, CRC(464952cf) SHA1(6b99360b6ba1ed5a72c257f51291f9f7a1ddf363) )
	ROM_LOAD( "tom_m25.rom",  0x20000, 0x10000, CRC(3db9b2c7) SHA1(02a318ffc459c494b7f40827eff5f89b41ac0426) )
	ROM_LOAD( "tom_m24.rom",  0x30000, 0x10000, CRC(f01fe899) SHA1(c5ab967b7af55a757638bcdc9975f4b15064022d) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "tom_m44.rom",  0x00000, 0x10000, CRC(3ed51d1f) SHA1(84f3aa17d640df91387e5f1f5b5971cf8dcd4e17) )
ROM_END

ROM_START( xmultipl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ch3.h3",       0x00001, 0x20000, CRC(20685021) SHA1(92f4216320bf525045223b9454fb5bb224c536d8) )
	ROM_LOAD16_BYTE( "cl3.l3",       0x00000, 0x20000, CRC(93fdd200) SHA1(dd4244ba0ce6c621136b0648374179da44363c01) )
	ROM_LOAD16_BYTE( "ch0.h0",       0x40001, 0x10000, CRC(9438dd8a) SHA1(dc85789c47d31a96300b4236dc43f065e1b01e4a) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "cl0.l0",       0x40000, 0x10000, CRC(06a9e213) SHA1(9831c110814642703d6e71d49848d854095b7d3a) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "t44.00",       0x00000, 0x20000, CRC(db45186e) SHA1(8c8edeb4b7e6b0516f2597823dc27eba9c5d9528) )	/* sprites */
	ROM_LOAD( "t45.01",       0x20000, 0x20000, CRC(4d0764d4) SHA1(4942333336a110b033f16ac1afa06ffef7b2dad6) )
	ROM_LOAD( "t46.10",       0x40000, 0x20000, CRC(f0c465a4) SHA1(69c107c860d4e8736431fd86b6821b70a8367eb3) )
	ROM_LOAD( "t47.11",       0x60000, 0x20000, CRC(1263b24b) SHA1(0445a5381df3a868bed6967c8e5de7169e4be6a3) )
	ROM_LOAD( "t48.20",       0x80000, 0x20000, CRC(4129944f) SHA1(988b072032d1667c3ac0731fada32fb6978505dc) )
	ROM_LOAD( "t49.21",       0xa0000, 0x20000, CRC(2346e6f9) SHA1(b3de017dd0353e04d279f57e151c47f5fcc70e9c) )
	ROM_LOAD( "t50.30",       0xc0000, 0x20000, CRC(e322543e) SHA1(b4c3a7f202d81485d5f0a7b7668ee89fc1edb215) )
	ROM_LOAD( "t51.31",       0xe0000, 0x20000, CRC(229bf7b1) SHA1(ae42c7efbb6278dd3fa56842361138391f2d49ca) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "t53.a0",       0x00000, 0x20000, CRC(1a082494) SHA1(63a3a84a262833d2cafab41e35df8f10a5e317b1) )	/* tiles #1 */
	ROM_LOAD( "t54.a1",       0x20000, 0x20000, CRC(076c16c5) SHA1(4be858806b916953d59aceee550e721eaf3996a6) )
	ROM_LOAD( "t55.a2",       0x40000, 0x20000, CRC(25d877a5) SHA1(48c948bf714c432f534c098123c8f50d5561756f) )
	ROM_LOAD( "t56.a3",       0x60000, 0x20000, CRC(5b1213f5) SHA1(87782aa0bd04d4378c4ba78b63028ae2709da2f1) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "t57.b0",       0x00000, 0x20000, CRC(0a84e0c7) SHA1(67ad181a7d2c431cb4bf45955e09754549a03576) )	/* tiles #2 */
	ROM_LOAD( "t58.b1",       0x20000, 0x20000, CRC(a874121d) SHA1(1351d5901d55059c6472a4588a2e560396903861) )
	ROM_LOAD( "t59.b2",       0x40000, 0x20000, CRC(69deb990) SHA1(1eed3183efbe576376661b45152a0a21240ecfc8) )
	ROM_LOAD( "t60.b3",       0x60000, 0x20000, CRC(14c69f99) SHA1(4bea72f8bd421ef3ca559363f7473ce2e7038699) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "t52.v0",       0x00000, 0x20000, CRC(2db1bd80) SHA1(657006d0642ec7fb949bb52821d78fe51a599415) )
ROM_END

ROM_START( dbreed )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "db_c-h3.rom",  0x00001, 0x20000, CRC(4bf3063c) SHA1(3f970c9ece2ac700738e217e0b31b3aba2848ab2) )
	ROM_LOAD16_BYTE( "db_c-l3.rom",  0x00000, 0x20000, CRC(e4b89b79) SHA1(c312925940633e60fb5d0f05044c6e73e4f7fd54) )
	ROM_LOAD16_BYTE( "db_c-h0.rom",  0x60001, 0x10000, CRC(5aa79fb2) SHA1(b7b862699ddccf90cf18d3822703078668aa1dc7) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "db_c-l0.rom",  0x60000, 0x10000, CRC(ed0f5e06) SHA1(9030840b15e83c18d59c884ed08c93c05fa70c5b) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "db_k800m.rom", 0x00000, 0x20000, CRC(c027a8cf) SHA1(534dc416b8f5587168c7f644d3f9438c8a190491) )	/* sprites */
	ROM_LOAD( "db_k801m.rom", 0x20000, 0x20000, CRC(093faf33) SHA1(2704f644cdce87daf975984f143b1d55ba731c3f) )
	ROM_LOAD( "db_k802m.rom", 0x40000, 0x20000, CRC(055b4c59) SHA1(71315dd7476612f138cb64b905648791d44eb7da) )
	ROM_LOAD( "db_k803m.rom", 0x60000, 0x20000, CRC(8ed63922) SHA1(51daa8a23e637f6b4394598ff4a1d26f65b59c8b) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "db_a0m.rom",   0x00000, 0x20000, CRC(4c83e92e) SHA1(6dade027435c48ab48bd4516d16a9961d4dd6fad) )	/* tiles #1 */
	ROM_LOAD( "db_a1m.rom",   0x20000, 0x20000, CRC(835ef268) SHA1(89d0bb15201440dffad3ef745970f95505d7ab03) )
	ROM_LOAD( "db_a2m.rom",   0x40000, 0x20000, CRC(5117f114) SHA1(a401a3e638209b32d4101a5c2e2a8b4612eaa21b) )
	ROM_LOAD( "db_a3m.rom",   0x60000, 0x20000, CRC(8eb0c978) SHA1(7fc55bbe4d0923db88492bb7160a89de34e11cd6) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "db_b0m.rom",   0x00000, 0x20000, CRC(4c83e92e) SHA1(6dade027435c48ab48bd4516d16a9961d4dd6fad) )	/* tiles #2 */
	ROM_LOAD( "db_b1m.rom",   0x20000, 0x20000, CRC(835ef268) SHA1(89d0bb15201440dffad3ef745970f95505d7ab03) )
	ROM_LOAD( "db_b2m.rom",   0x40000, 0x20000, CRC(5117f114) SHA1(a401a3e638209b32d4101a5c2e2a8b4612eaa21b) )
	ROM_LOAD( "db_b3m.rom",   0x60000, 0x20000, CRC(8eb0c978) SHA1(7fc55bbe4d0923db88492bb7160a89de34e11cd6) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "db_c-v0.rom",  0x00000, 0x20000, CRC(312f7282) SHA1(742d56980b4618180e9a0e02051c5aec4d5cdae4) )
ROM_END

ROM_START( rtype2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ic54.8d",      0x00001, 0x20000, CRC(d8ece6f4) SHA1(f7bb246fe8b75af24716d419bb3c6e7d9cd0971e) )
	ROM_LOAD16_BYTE( "ic60.9d",      0x00000, 0x20000, CRC(32cfb2e4) SHA1(d4b44a40e2933040eddb2b09de7bfe28d76c5f25) )
	ROM_LOAD16_BYTE( "ic53.8b",      0x40001, 0x20000, CRC(4f6e9b15) SHA1(ef733c2615951f54691877ad3e84d08107723324) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "ic59.9b",      0x40000, 0x20000, CRC(0fd123bf) SHA1(1133163f6716e9a4bbb437b3a471477d0bd97051) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic17.4f",      0x0000, 0x10000, CRC(73ffecb4) SHA1(4795bf0d6263060c3d3759b659bdb189a4087600) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic31.6l",      0x00000, 0x20000, CRC(2cd8f913) SHA1(a53752b35da95b420dd29a09176d265d292b3938) )	/* sprites */
	ROM_LOAD( "ic21.4l",      0x20000, 0x20000, CRC(5033066d) SHA1(e125127f0610c63f9e59a585db547be5d49ed863) )
	ROM_LOAD( "ic32.6m",      0x40000, 0x20000, CRC(ec3a0450) SHA1(632bdd397f1bc67f6970faf7d09ab8d911e105fe) )
	ROM_LOAD( "ic22.4m",      0x60000, 0x20000, CRC(db6176fc) SHA1(1eaf72af0322490c98461aded202288e387caac1) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic50.7s",      0x00000, 0x20000, CRC(f3f8736e) SHA1(37872b30459ad05b2981d4ac84983f3b52d0d2d6) )	/* tiles */
	ROM_LOAD( "ic51.7u",      0x20000, 0x20000, CRC(b4c543af) SHA1(56042eba711160fc701021c8787414dcaddcdecb) )
	ROM_LOAD( "ic56.8s",      0x40000, 0x20000, CRC(4cb80d66) SHA1(31c5496c14b277e428a2f22195fe1742d6a577d4) )
	ROM_LOAD( "ic57.8u",      0x60000, 0x20000, CRC(bee128e0) SHA1(b149dae5f8f67a329d6df033fadf50ad75c0a57a) )
	ROM_LOAD( "ic65.9r",      0x80000, 0x20000, CRC(2dc9c71a) SHA1(124e89c17f3af034d5a387ff3eab906d289c27f7) )
	ROM_LOAD( "ic66.9u",      0xa0000, 0x20000, CRC(7533c428) SHA1(ba435cfb6c3c49fcc4d716dcecf8f17545b8eec6) )
	ROM_LOAD( "ic63.9m",      0xc0000, 0x20000, CRC(a6ad67f2) SHA1(b005b037ce8b3c932089982ecfbccdc922278fe3) )
	ROM_LOAD( "ic64.9p",      0xe0000, 0x20000, CRC(3686d555) SHA1(d03754d9b8a6a3bfd4a85eeddacc35a36af197bd) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ic14.4c",      0x00000, 0x20000, CRC(637172d5) SHA1(9dd0dc409306287238826bf301e2a7a12d6cd9ce) )
ROM_END

ROM_START( rtype2j )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "rt2-a-h0.54",  0x00001, 0x20000, CRC(7857ccf6) SHA1(9f6774a8128ee2dbb5b6c42289095275337bc73e) )
	ROM_LOAD16_BYTE( "rt2-a-l0.60",  0x00000, 0x20000, CRC(cb22cd6e) SHA1(a877cffbac9f55bca8932b12540a4686ba975684) )
	ROM_LOAD16_BYTE( "rt2-a-h1.53",  0x40001, 0x20000, CRC(49e75d28) SHA1(956bafaaa6711a8a13f2bffe43e8d05d51d8a3c9) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "rt2-a-l1.59",  0x40000, 0x20000, CRC(12ec1676) SHA1(10cee9a87dd954444b0e64fad7f15a5ae529890d) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic17.4f",      0x0000, 0x10000, CRC(73ffecb4) SHA1(4795bf0d6263060c3d3759b659bdb189a4087600) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic31.6l",      0x00000, 0x20000, CRC(2cd8f913) SHA1(a53752b35da95b420dd29a09176d265d292b3938) )	/* sprites */
	ROM_LOAD( "ic21.4l",      0x20000, 0x20000, CRC(5033066d) SHA1(e125127f0610c63f9e59a585db547be5d49ed863) )
	ROM_LOAD( "ic32.6m",      0x40000, 0x20000, CRC(ec3a0450) SHA1(632bdd397f1bc67f6970faf7d09ab8d911e105fe) )
	ROM_LOAD( "ic22.4m",      0x60000, 0x20000, CRC(db6176fc) SHA1(1eaf72af0322490c98461aded202288e387caac1) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic50.7s",      0x00000, 0x20000, CRC(f3f8736e) SHA1(37872b30459ad05b2981d4ac84983f3b52d0d2d6) )	/* tiles */
	ROM_LOAD( "ic51.7u",      0x20000, 0x20000, CRC(b4c543af) SHA1(56042eba711160fc701021c8787414dcaddcdecb) )
	ROM_LOAD( "ic56.8s",      0x40000, 0x20000, CRC(4cb80d66) SHA1(31c5496c14b277e428a2f22195fe1742d6a577d4) )
	ROM_LOAD( "ic57.8u",      0x60000, 0x20000, CRC(bee128e0) SHA1(b149dae5f8f67a329d6df033fadf50ad75c0a57a) )
	ROM_LOAD( "ic65.9r",      0x80000, 0x20000, CRC(2dc9c71a) SHA1(124e89c17f3af034d5a387ff3eab906d289c27f7) )
	ROM_LOAD( "ic66.9u",      0xa0000, 0x20000, CRC(7533c428) SHA1(ba435cfb6c3c49fcc4d716dcecf8f17545b8eec6) )
	ROM_LOAD( "ic63.9m",      0xc0000, 0x20000, CRC(a6ad67f2) SHA1(b005b037ce8b3c932089982ecfbccdc922278fe3) )
	ROM_LOAD( "ic64.9p",      0xe0000, 0x20000, CRC(3686d555) SHA1(d03754d9b8a6a3bfd4a85eeddacc35a36af197bd) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ic14.4c",      0x00000, 0x20000, CRC(637172d5) SHA1(9dd0dc409306287238826bf301e2a7a12d6cd9ce) )
ROM_END

ROM_START( majtitle )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mt_m0.bin",    0x00001, 0x20000, CRC(b9682c70) SHA1(b979c0a630397f2a2eb73709cf12c5262c973782) )
	ROM_LOAD16_BYTE( "mt_l0.bin",    0x00000, 0x20000, CRC(702c9fd6) SHA1(84a5e9e64f4bf235d115f5648b4a108f710ade1d) )
	ROM_LOAD16_BYTE( "mt_m1.bin",    0x40001, 0x20000, CRC(d9e97c30) SHA1(97f59b614eeeced0a414f8a1693590525a58f788) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "mt_l1.bin",    0x40000, 0x20000, CRC(8dbd91b5) SHA1(2bd01f3fba0fa1ca4b6f8ff57e7dc4434c42ce48) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "mt_sp.bin",    0x0000, 0x10000, CRC(e44260a9) SHA1(a2512033c8cca9a8064eae1ada721202edf06e8e) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mt_n0.bin",    0x00000, 0x40000, CRC(5618cddc) SHA1(16d34b431ab9b72067fa669d694e635c88aeb261) )	/* sprites #1 */
	ROM_LOAD( "mt_n1.bin",    0x40000, 0x40000, CRC(483b873b) SHA1(654efd67b2102521e8c46cd57cefa2cc64cf4fd3) )
	ROM_LOAD( "mt_n2.bin",    0x80000, 0x40000, CRC(4f5d665b) SHA1(f539d0f5c738ffabfac16121706abe3bb3b2a1fa) )
	ROM_LOAD( "mt_n3.bin",    0xc0000, 0x40000, CRC(83571549) SHA1(ce0b89aa4b3e3e1cf6ec6136f956577267cdd9d3) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mt_c0.bin",    0x00000, 0x20000, CRC(780e7a02) SHA1(9776ecb8b5d86636061f8360464001a63bec0842) )	/* tiles */
	ROM_LOAD( "mt_c1.bin",    0x20000, 0x20000, CRC(45ad1381) SHA1(de281398dcd1c547bde9fa86f8ca409dd8d4aa6c) )
	ROM_LOAD( "mt_c2.bin",    0x40000, 0x20000, CRC(5df5856d) SHA1(f16163f672de6701b411315c9956ddb74c8464ce) )
	ROM_LOAD( "mt_c3.bin",    0x60000, 0x20000, CRC(f5316cc8) SHA1(123892d4a7e8d98582ea736afe659afdba8c5f87) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mt_f0.bin",    0x00000, 0x20000, CRC(2d5e05d5) SHA1(18bdc9c561dbf0f91642161ca985d2154bd58b5d) )	/* sprites #2 */
	ROM_LOAD( "mt_f1.bin",    0x20000, 0x20000, CRC(c68cd65f) SHA1(8999b558b4af0f453ada9e4ef705163df96844e6) )
	ROM_LOAD( "mt_f2.bin",    0x40000, 0x20000, CRC(a71feb2d) SHA1(47e366b422772bed08ee4d1c338970687d6c3b4c) )
	ROM_LOAD( "mt_f3.bin",    0x60000, 0x20000, CRC(179f7562) SHA1(6d28b199daffc62e8fa9009878ac0bb976ccbb2a) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "mt_vo.bin",    0x00000, 0x20000, CRC(eb24bb2c) SHA1(9fca04fba0249e8213dd164eb6829e1a5acbee65) )
ROM_END

ROM_START( hharry )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "a-h0-v.rom",   0x00001, 0x20000, CRC(c52802a5) SHA1(7180189c886aebe8d3e7fd38922916cecfddae32) )
	ROM_LOAD16_BYTE( "a-l0-v.rom",   0x00000, 0x20000, CRC(f463074c) SHA1(aca86345610e65848c276ab278092d35ba215916) )
	ROM_LOAD16_BYTE( "a-h1-0.rom",   0x60001, 0x10000, CRC(3ae21335) SHA1(780d7a0c5bebe4b914ea5b3741e30630f8c29a4f) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "a-l1-0.rom",   0x60000, 0x10000, CRC(bc6ac5f9) SHA1(c6afba4967a8055f6b63827697425eac743f5a75) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "a-sp-0.rom",   0x0000, 0x10000, CRC(80e210e7) SHA1(66cff58fb37c52e1d8e0567e13b774253e862585) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hh_00.rom",    0x00000, 0x20000, CRC(ec5127ef) SHA1(014ac8ad7b19cd9b475b72a0f42a4991119501c4) )	/* sprites */
	ROM_LOAD( "hh_10.rom",    0x20000, 0x20000, CRC(def65294) SHA1(23f5d99fa9f604fde37cb52113bff233d9be1d25) )
	ROM_LOAD( "hh_20.rom",    0x40000, 0x20000, CRC(bb0d6ad4) SHA1(4ab617fadfc32efad90ed7f0555513f167b0c43a) )
	ROM_LOAD( "hh_30.rom",    0x60000, 0x20000, CRC(4351044e) SHA1(0d3ce3f4f1473fd997e70de91e7b5b5a5ec60ad4) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hh_a0.rom",    0x00000, 0x20000, CRC(c577ba5f) SHA1(c882e58cf64deca8eee6f14f3df43ecc932488fc) )	/* tiles */
	ROM_LOAD( "hh_a1.rom",    0x20000, 0x20000, CRC(429d12ab) SHA1(ccba25eab981fc4e664f76e06a2964066f2ae2e8) )
	ROM_LOAD( "hh_a2.rom",    0x40000, 0x20000, CRC(b5b163b0) SHA1(82a708fea4953a7c4dcd1d4a1b07f302221ba30b) )
	ROM_LOAD( "hh_a3.rom",    0x60000, 0x20000, CRC(8ef566a1) SHA1(3afb020a7317efe89c18b2a7773894ce28499d49) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "a-v0-0.rom",   0x00000, 0x20000, CRC(faaacaff) SHA1(ea3a3920255c07aa9c0a7e0191eae257a9f7f558) )
ROM_END

ROM_START( hharryu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "a-ho-u.8d",    0x00001, 0x20000, CRC(ede7f755) SHA1(adcec83d6b936ab1a14d039792b9375e9f803a08) )
	ROM_LOAD16_BYTE( "a-lo-u.9d",    0x00000, 0x20000, CRC(df0726ae) SHA1(7ef163d2e8c14a14328d4365705bb31540bdc7cb) )
	ROM_LOAD16_BYTE( "a-h1-f.8b",    0x60001, 0x10000, CRC(31b741c5) SHA1(46c1c4cea09477cc4989f3e06e08851d02743e62) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "a-l1-f.9b",    0x60000, 0x10000, CRC(b23e966c) SHA1(f506f6d1f4f7874070e91d1df8f141cca031ce29) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "a-sp-0.rom",   0x0000, 0x10000, CRC(80e210e7) SHA1(66cff58fb37c52e1d8e0567e13b774253e862585) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hh_00.rom",    0x00000, 0x20000, CRC(ec5127ef) SHA1(014ac8ad7b19cd9b475b72a0f42a4991119501c4) )	/* sprites */
	ROM_LOAD( "hh_10.rom",    0x20000, 0x20000, CRC(def65294) SHA1(23f5d99fa9f604fde37cb52113bff233d9be1d25) )
	ROM_LOAD( "hh_20.rom",    0x40000, 0x20000, CRC(bb0d6ad4) SHA1(4ab617fadfc32efad90ed7f0555513f167b0c43a) )
	ROM_LOAD( "hh_30.rom",    0x60000, 0x20000, CRC(4351044e) SHA1(0d3ce3f4f1473fd997e70de91e7b5b5a5ec60ad4) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hh_a0.rom",    0x00000, 0x20000, CRC(c577ba5f) SHA1(c882e58cf64deca8eee6f14f3df43ecc932488fc) )	/* tiles */
	ROM_LOAD( "hh_a1.rom",    0x20000, 0x20000, CRC(429d12ab) SHA1(ccba25eab981fc4e664f76e06a2964066f2ae2e8) )
	ROM_LOAD( "hh_a2.rom",    0x40000, 0x20000, CRC(b5b163b0) SHA1(82a708fea4953a7c4dcd1d4a1b07f302221ba30b) )
	ROM_LOAD( "hh_a3.rom",    0x60000, 0x20000, CRC(8ef566a1) SHA1(3afb020a7317efe89c18b2a7773894ce28499d49) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "a-v0-0.rom",   0x00000, 0x20000, CRC(faaacaff) SHA1(ea3a3920255c07aa9c0a7e0191eae257a9f7f558) )
ROM_END

ROM_START( dkgensan )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "gen-a-h0.bin", 0x00001, 0x20000, CRC(07a45f6d) SHA1(8ffbd395aad244747d9f87062d2b062f41a4829c) )
	ROM_LOAD16_BYTE( "gen-a-l0.bin", 0x00000, 0x20000, CRC(46478fea) SHA1(fd4ff544588535333c1b98fbc08446ef49b11212) )
	ROM_LOAD16_BYTE( "gen-a-h1.bin", 0x60001, 0x10000, CRC(54e5b73c) SHA1(5664f6e0a931b1c139e82dc98fcc9e38acd14616) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "gen-a-l1.bin", 0x60000, 0x10000, CRC(894f8a9f) SHA1(57a0885c52a094def03b129a450cc891e6c075c6) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "gen-a-sp.bin", 0x0000, 0x10000, CRC(e83cfc2c) SHA1(3193bdd06a9712fc499e6fc90a33140463ef59fe) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hh_00.rom",    0x00000, 0x20000, CRC(ec5127ef) SHA1(014ac8ad7b19cd9b475b72a0f42a4991119501c4) )	/* sprites */
	ROM_LOAD( "hh_10.rom",    0x20000, 0x20000, CRC(def65294) SHA1(23f5d99fa9f604fde37cb52113bff233d9be1d25) )
	ROM_LOAD( "hh_20.rom",    0x40000, 0x20000, CRC(bb0d6ad4) SHA1(4ab617fadfc32efad90ed7f0555513f167b0c43a) )
	ROM_LOAD( "hh_30.rom",    0x60000, 0x20000, CRC(4351044e) SHA1(0d3ce3f4f1473fd997e70de91e7b5b5a5ec60ad4) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hh_a0.rom",    0x00000, 0x20000, CRC(c577ba5f) SHA1(c882e58cf64deca8eee6f14f3df43ecc932488fc) )	/* tiles */
	ROM_LOAD( "hh_a1.rom",    0x20000, 0x20000, CRC(429d12ab) SHA1(ccba25eab981fc4e664f76e06a2964066f2ae2e8) )
	ROM_LOAD( "hh_a2.rom",    0x40000, 0x20000, CRC(b5b163b0) SHA1(82a708fea4953a7c4dcd1d4a1b07f302221ba30b) )
	ROM_LOAD( "hh_a3.rom",    0x60000, 0x20000, CRC(8ef566a1) SHA1(3afb020a7317efe89c18b2a7773894ce28499d49) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "gen-vo.bin",   0x00000, 0x20000, CRC(d8595c66) SHA1(97920c9947fbac609fb901415e5471c6e4ca066c) )
ROM_END

ROM_START( dkgenm72 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ge72-h0.bin",  0x00001, 0x20000, CRC(a0ad992c) SHA1(6de4105d8454c4e4e62762fdd7e22829acc2442b) )
	ROM_LOAD16_BYTE( "ge72-l0.bin",  0x00000, 0x20000, CRC(996396f0) SHA1(1a2501ba46bcbc607f772765e8614bc442154a18) )
	ROM_LOAD16_BYTE( "ge72-h3.bin",  0x60001, 0x10000, CRC(d8b86005) SHA1(dd626cfe50a823066c54cc24d9fdaaf03d61d1e7) )
	ROM_RELOAD(                      0xe0001, 0x10000 )
	ROM_LOAD16_BYTE( "ge72-l3.bin",  0x60000, 0x10000, CRC(23d303a5) SHA1(b62010f34d71afb590deae458493454f9af38f7c) )
	ROM_RELOAD(                      0xe0000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hh_00.rom",    0x00000, 0x20000, CRC(ec5127ef) SHA1(014ac8ad7b19cd9b475b72a0f42a4991119501c4) )	/* sprites */
	ROM_LOAD( "hh_10.rom",    0x20000, 0x20000, CRC(def65294) SHA1(23f5d99fa9f604fde37cb52113bff233d9be1d25) )
	ROM_LOAD( "hh_20.rom",    0x40000, 0x20000, CRC(bb0d6ad4) SHA1(4ab617fadfc32efad90ed7f0555513f167b0c43a) )
	ROM_LOAD( "hh_30.rom",    0x60000, 0x20000, CRC(4351044e) SHA1(0d3ce3f4f1473fd997e70de91e7b5b5a5ec60ad4) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ge72b-a0.bin", 0x00000, 0x10000, CRC(f5f56b2a) SHA1(4ef6602052fa70e765d6d7747e672b7108b44f59) )	/* tiles #1 */
	ROM_LOAD( "ge72-a1.bin",  0x10000, 0x10000, CRC(d194ea08) SHA1(0270897049cd256472df42f3dda856ee707535cd) )
	ROM_LOAD( "ge72-a2.bin",  0x20000, 0x10000, CRC(2b06bcc3) SHA1(36378a4a69f3c3da96d2dc8df48916af8de50009) )
	ROM_LOAD( "ge72-a3.bin",  0x30000, 0x10000, CRC(94b96bfa) SHA1(33c1e9045e7a984097f3fe4954b20d954cffbafa) )

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ge72-b0.bin",  0x00000, 0x10000, CRC(208796b3) SHA1(38b90732c8d5c77ee84053364a8a7e3daaaabe66) )	/* tiles #2 */
	ROM_LOAD( "ge72-b1.bin",  0x10000, 0x10000, CRC(b4a7f490) SHA1(851b40650fc8920b49f43f9cc6f19e845a25e945) )
	ROM_LOAD( "ge72b-b2.bin", 0x20000, 0x10000, CRC(34fe8f7f) SHA1(fbf8839b26be55ad83ad4db538ba3e196c1ab945) )
	ROM_LOAD( "ge72b-b3.bin", 0x30000, 0x10000, CRC(4b0e92f4) SHA1(16ad9220ca6708028cea18c1c4b57e2b6eb425b4) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "gen-vo.bin",   0x00000, 0x20000, CRC(d8595c66) SHA1(97920c9947fbac609fb901415e5471c6e4ca066c) )
ROM_END

ROM_START( poundfor )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ppa-h0-b.bin", 0x00001, 0x20000, CRC(50d4a2d8) SHA1(7fd62c6613cb58b512c6c3670fa66a5b9906e6a1) )
	ROM_LOAD16_BYTE( "ppa-l0-b.bin", 0x00000, 0x20000, CRC(bd997942) SHA1(da484afe3b79e09e323c768a0b2165e6283971a7) )
	ROM_LOAD16_BYTE( "ppa-h1.9f",    0x40001, 0x20000, CRC(f6c82f48) SHA1(b38a2f9f0f6439b2cf453fec87ca11d959777ee6) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "ppa-l1.9c",    0x40000, 0x20000, CRC(5b07b087) SHA1(04a2403eb8c443cb92b880edc612542acdbcafa4) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ppa-sp.4j",    0x0000, 0x10000, CRC(3f458a5b) SHA1(d73740b2a548bf8a895909da0841f18d9ed32668) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ppb-n0.bin",   0x00000, 0x40000, CRC(951a41f8) SHA1(59b64f63ea2452c2b42ff7ebf1ff6fc4e7879ce3) )	/* sprites */
	ROM_LOAD( "ppb-n1.bin",   0x40000, 0x40000, CRC(c609b7f2) SHA1(1da3550c7e4d2a26d75d143934680d9177ba5c35) )
	ROM_LOAD( "ppb-n2.bin",   0x80000, 0x40000, CRC(318c0b5f) SHA1(1d4cd17dc2f8fc4e523eaf679f21d83e1bfade4e) )
	ROM_LOAD( "ppb-n3.bin",   0xc0000, 0x40000, CRC(93dc9490) SHA1(3df4d57a7bf19443f5aa6a416bcee968f81d9059) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ppa-g00.bin",  0x00000, 0x20000, CRC(8a88a174) SHA1(d360b9014aec31960538ee488894496248a820dc) )	/* tiles */
	ROM_LOAD( "ppa-g10.bin",  0x20000, 0x20000, CRC(e48a66ac) SHA1(49b33db6a922d6f1d1417e28714a67431b7c0217) )
	ROM_LOAD( "ppa-g20.bin",  0x40000, 0x20000, CRC(12b93e79) SHA1(f3d2b76a30874827c8998c1d13a55a3990b699b7) )
	ROM_LOAD( "ppa-g30.bin",  0x60000, 0x20000, CRC(faa39aee) SHA1(9cc1a468b304437766c04189054d3b8f7ff1f958) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ppa-v0.bin",   0x00000, 0x40000, CRC(03321664) SHA1(51f2b2b712385c1cd55fd069829efac01838d603) )
ROM_END

ROM_START( poundfou )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ppa-ho-a.9e",  0x00001, 0x20000, CRC(ff4c83a4) SHA1(1b7791c784bf7c4774e3200b76d65ab0bf0ff93b) )
	ROM_LOAD16_BYTE( "ppa-lo-a.9d",  0x00000, 0x20000, CRC(3374ce8f) SHA1(7455f8339aeed0ef3d0567baa804b62ca3615283) )
	ROM_LOAD16_BYTE( "ppa-h1.9f",    0x40001, 0x20000, CRC(f6c82f48) SHA1(b38a2f9f0f6439b2cf453fec87ca11d959777ee6) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "ppa-l1.9c",    0x40000, 0x20000, CRC(5b07b087) SHA1(04a2403eb8c443cb92b880edc612542acdbcafa4) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ppa-sp.4j",    0x0000, 0x10000, CRC(3f458a5b) SHA1(d73740b2a548bf8a895909da0841f18d9ed32668) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ppb-n0.bin",   0x00000, 0x40000, CRC(951a41f8) SHA1(59b64f63ea2452c2b42ff7ebf1ff6fc4e7879ce3) )	/* sprites */
	ROM_LOAD( "ppb-n1.bin",   0x40000, 0x40000, CRC(c609b7f2) SHA1(1da3550c7e4d2a26d75d143934680d9177ba5c35) )
	ROM_LOAD( "ppb-n2.bin",   0x80000, 0x40000, CRC(318c0b5f) SHA1(1d4cd17dc2f8fc4e523eaf679f21d83e1bfade4e) )
	ROM_LOAD( "ppb-n3.bin",   0xc0000, 0x40000, CRC(93dc9490) SHA1(3df4d57a7bf19443f5aa6a416bcee968f81d9059) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ppa-g00.bin",  0x00000, 0x20000, CRC(8a88a174) SHA1(d360b9014aec31960538ee488894496248a820dc) )	/* tiles */
	ROM_LOAD( "ppa-g10.bin",  0x20000, 0x20000, CRC(e48a66ac) SHA1(49b33db6a922d6f1d1417e28714a67431b7c0217) )
	ROM_LOAD( "ppa-g20.bin",  0x40000, 0x20000, CRC(12b93e79) SHA1(f3d2b76a30874827c8998c1d13a55a3990b699b7) )
	ROM_LOAD( "ppa-g30.bin",  0x60000, 0x20000, CRC(faa39aee) SHA1(9cc1a468b304437766c04189054d3b8f7ff1f958) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ppa-v0.bin",   0x00000, 0x40000, CRC(03321664) SHA1(51f2b2b712385c1cd55fd069829efac01838d603) )
ROM_END

ROM_START( airduel )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ad-c-h0.bin",  0x00001, 0x20000, CRC(12140276) SHA1(f218c5f2e6795b6295dea064817d7d6b1a7762b6) )
	ROM_LOAD16_BYTE( "ad-c-l0.bin",  0x00000, 0x20000, CRC(4ac0b91d) SHA1(97e2f633181cd5c25927fd0e2988af2acdb3f388) )
	ROM_LOAD16_BYTE( "ad-c-h3.bin",  0x40001, 0x20000, CRC(9f7cfca3) SHA1(becf827aa7749c54f1c435ea224e1fd9c8b3f5f9) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "ad-c-l3.bin",  0x40000, 0x20000, CRC(9dd343f7) SHA1(9f499936b6d3807aa5b5c18e9811c73c9a2c99f9) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ad-00.bin",    0x00000, 0x20000, CRC(2f0d599b) SHA1(a966f806b5e25bb98cc63c46c49e0e676a62afcf) )	/* sprites */
	ROM_LOAD( "ad-10.bin",    0x20000, 0x20000, CRC(9865856b) SHA1(b18a06899ae29d45e2351594df544220f3f4485a) )
	ROM_LOAD( "ad-20.bin",    0x40000, 0x20000, CRC(d392aef2) SHA1(0f639a07066cadddc3884eb490885a8745571567) )
	ROM_LOAD( "ad-30.bin",    0x60000, 0x20000, CRC(923240c3) SHA1(f587a83329087a715a3e42110f74f104e8c8ef1f) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ad-a0.bin",    0x00000, 0x20000, CRC(ce134b47) SHA1(841358cc222c81b8a91edc262f355310d50b4dbb) )	/* tiles #1 */
	ROM_LOAD( "ad-a1.bin",    0x20000, 0x20000, CRC(097fd853) SHA1(8e08f4f4a747c899bb8e21b347635e26af9edc2d) )
	ROM_LOAD( "ad-a2.bin",    0x40000, 0x20000, CRC(6a94c1b9) SHA1(55174acbac54236e5fc1b80d120cd6da9fe5524c) )
	ROM_LOAD( "ad-a3.bin",    0x60000, 0x20000, CRC(6637c349) SHA1(27cb7c89ab73292b43f8ae3c0d803a01ef3d3936) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ad-b0.bin",    0x00000, 0x20000, CRC(ce134b47) SHA1(841358cc222c81b8a91edc262f355310d50b4dbb) )	/* tiles #2 */
	ROM_LOAD( "ad-b1.bin",    0x20000, 0x20000, CRC(097fd853) SHA1(8e08f4f4a747c899bb8e21b347635e26af9edc2d) )
	ROM_LOAD( "ad-b2.bin",    0x40000, 0x20000, CRC(6a94c1b9) SHA1(55174acbac54236e5fc1b80d120cd6da9fe5524c) )
	ROM_LOAD( "ad-b3.bin",    0x60000, 0x20000, CRC(6637c349) SHA1(27cb7c89ab73292b43f8ae3c0d803a01ef3d3936) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ad-v0.bin",    0x00000, 0x20000, CRC(339f474d) SHA1(a81bb52598a0e31b2ed6a538755237c5d14d1844) )
ROM_END

ROM_START( cosmccop )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "cc-d-h0b.bin", 0x00001, 0x40000, CRC(38958b01) SHA1(7d7e217742e33a1fe096adf5bbc93d63ddcfb375) )
	ROM_RELOAD(                      0x80001, 0x40000 )
	ROM_LOAD16_BYTE( "cc-d-l0b.bin", 0x00000, 0x40000, CRC(eff87f70) SHA1(61f49b8738cf31546d4182680b761705274b01bf) )
	ROM_RELOAD(                      0x80000, 0x40000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "cc-d-sp.bin", 0x0000, 0x10000, CRC(3e3ace60) SHA1(d89b1b84de2887598bb7bcb17b1df1ec8d1862a9) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cc-c-00.bin", 0x00000, 0x20000, CRC(9d99deaa) SHA1(acf16bea0f482306107d2a305c568406b6c21e9a) )	/* cc-b-n0*/
	ROM_LOAD( "cc-c-10.bin", 0x20000, 0x20000, CRC(7eb083ed) SHA1(31fa7d532fd46e861c3d19d5b08661653f685a49) )	/* cc-b-n1*/
	ROM_LOAD( "cc-c-20.bin", 0x40000, 0x20000, CRC(9421489e) SHA1(e43d042bf8b4ebed93558d74ec479ec60a01ca5c) )	/* cc-b-n2*/
	ROM_LOAD( "cc-c-30.bin", 0x60000, 0x20000, CRC(920ec735) SHA1(2d0949b43dddce7317c45910d6e4868ddf010806) )	/* cc-b-n3*/

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cc-d-g00.bin", 0x00000, 0x20000, CRC(e7f3d772) SHA1(c7f0bc42e8dde7bae334c7974c3d0ddba3856144) ) /* tiles */
	ROM_LOAD( "cc-d-g10.bin", 0x20000, 0x20000, CRC(418b4e4c) SHA1(1191f12741ee7a360240f706534c9c83be8d5c2d) )
	ROM_LOAD( "cc-d-g20.bin", 0x40000, 0x20000, CRC(a4b558eb) SHA1(0babf725de0065dbeca73fa170bd33565305d129) )
	ROM_LOAD( "cc-d-g30.bin", 0x60000, 0x20000, CRC(f64a3166) SHA1(1661db2a37c76e6b4552e48c04966dbbccab8926) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "cc-c-v0.bin", 0x00000, 0x20000, CRC(6247bade) SHA1(4bc9f86acd09908c74b1ab0e7817c4ff1cad6f0b) )	/* cc-d-v0*/
ROM_END

ROM_START( gallop )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "cc-c-h0.bin",  0x00001, 0x20000, CRC(2217dcd0) SHA1(9485b6c3eec99e720439e69dcbe0e55798bbff1c) )
	ROM_LOAD16_BYTE( "cc-c-l0.bin",  0x00000, 0x20000, CRC(ff39d7fb) SHA1(fad95f76050fce04464268b5edff6622b2cb798f) )
	ROM_LOAD16_BYTE( "cc-c-h3.bin",  0x40001, 0x20000, CRC(9b2bbab9) SHA1(255d4dda55be667f5f1f4324e9e66111738e79b3) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "cc-c-l3.bin",  0x40000, 0x20000, CRC(acd3278e) SHA1(83d7ddfbdb4bc9548a179b728351a21b3b0ac134) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* no ROM, program will be copied by the main CPU */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cc-c-00.bin",  0x00000, 0x20000, CRC(9d99deaa) SHA1(acf16bea0f482306107d2a305c568406b6c21e9a) )	/* sprites */
	ROM_LOAD( "cc-c-10.bin",  0x20000, 0x20000, CRC(7eb083ed) SHA1(31fa7d532fd46e861c3d19d5b08661653f685a49) )
	ROM_LOAD( "cc-c-20.bin",  0x40000, 0x20000, CRC(9421489e) SHA1(e43d042bf8b4ebed93558d74ec479ec60a01ca5c) )
	ROM_LOAD( "cc-c-30.bin",  0x60000, 0x20000, CRC(920ec735) SHA1(2d0949b43dddce7317c45910d6e4868ddf010806) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cc-b-a0.bin",  0x00000, 0x10000, CRC(a33472bd) SHA1(962047fe3dd1fb996285ecef615a8ebdb529adef) )	/* tiles #1 */
	ROM_LOAD( "cc-b-a1.bin",  0x10000, 0x10000, CRC(118b1f2d) SHA1(7413ccc67a8aa9dae156e6ee122b1ca5beeb9a76) )
	ROM_LOAD( "cc-b-a2.bin",  0x20000, 0x10000, CRC(83cebf48) SHA1(12847827ecbf6b493eb9dbddd0a469729d87a451) )
	ROM_LOAD( "cc-b-a3.bin",  0x30000, 0x10000, CRC(572903fc) SHA1(03305301bcf939e97044e746594736b1ca1d7c0a) )

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cc-b-b0.bin",  0x00000, 0x10000, CRC(0df5b439) SHA1(0775cf92139a111542c8b5f940da0f7f43020982) )	/* tiles #2 */
	ROM_LOAD( "cc-b-b1.bin",  0x10000, 0x10000, CRC(010b778f) SHA1(cc5bfeb0fbe0ed2fe513458c5785ec0ce5b02f53) )
	ROM_LOAD( "cc-b-b2.bin",  0x20000, 0x10000, CRC(bda9f6fb) SHA1(a6b655ae5bff0568c1fb56ee8a3874fc6524052c) )
	ROM_LOAD( "cc-b-b3.bin",  0x30000, 0x10000, CRC(d361ba3f) SHA1(7348fdae03e997e05187a2726eb221edb92553df) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc-c-v0.bin",  0x00000, 0x20000, CRC(6247bade) SHA1(4bc9f86acd09908c74b1ab0e7817c4ff1cad6f0b) )
ROM_END

ROM_START( kengo )
	ROM_REGION( 0x100000 * 2, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ken_d-h0.rom", 0x00001, 0x20000, CRC(f4ddeea5) SHA1(bcf016e40886e11c171f2f50de39ac0d8cabcdd1) )
	ROM_RELOAD(                      0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "ken_d-l0.rom", 0x00000, 0x20000, CRC(04dc0f81) SHA1(b296529f0bc26d53b344449dfa5a08eca70f30d8) )
	ROM_RELOAD(                      0xc0000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ken_d-sp.rom", 0x0000, 0x10000, CRC(233ca1cf) SHA1(4ebb6162773bd586a10016ccd77998a9b880f474) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ken_m31.rom",  0x00000, 0x20000, CRC(e00b95a6) SHA1(6efcd8d58f8ebe3a42c60a0aa790b42c0e132777) )	/* sprites */
	ROM_LOAD( "ken_m21.rom",  0x20000, 0x20000, CRC(d7722f87) SHA1(8606a53b8630934d2b5dfc986bd92ac4142f67e2) )
	ROM_LOAD( "ken_m32.rom",  0x40000, 0x20000, CRC(30a844c4) SHA1(72b2caba3ee7a229ca56f004516dea8d3f0a7ba6) )
	ROM_LOAD( "ken_m22.rom",  0x60000, 0x20000, CRC(a00dac85) SHA1(0c1ed852795046926f62843f6b256cbeecf9ebcf) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ken_m51.rom",  0x00000, 0x20000, CRC(1646cf4f) SHA1(d240cb2bad3e766128e8e40aa7b1bf4f3b9a5559) )	/* tiles */
	ROM_LOAD( "ken_m57.rom",  0x20000, 0x20000, CRC(a9f88d90) SHA1(c8d4a96fe55fed4b7499550f3c74b03d10306757) )
	ROM_LOAD( "ken_m66.rom",  0x40000, 0x20000, CRC(e9d17645) SHA1(fbe18d6691686a1c458d4a91169c9850698b5ca7) )
	ROM_LOAD( "ken_m64.rom",  0x60000, 0x20000, CRC(df46709b) SHA1(e7c2cd752e765bf7b8ff24637305d61031ce0baa) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ken_m14.rom",  0x00000, 0x20000, CRC(6651e9b7) SHA1(c42009f986c9a9f35732d5cd717d548536469b1c) )
ROM_END



static DRIVER_INIT( kengo )
{
	irem_cpu_decrypt(0,gunforce_decryption_table);
}



GAMEX( 1987, rtype,    0,        rtype,    rtype,    0,        ROT0,   "Irem", "R-Type (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1987, rtypepj,  rtype,    rtype,    rtypep,   0,        ROT0,   "Irem", "R-Type (Japan prototype)", GAME_NO_COCKTAIL )
GAMEX( 1987, rtypeu,   rtype,    rtype,    rtype,    0,        ROT0,   "Irem (Nintendo of America license)", "R-Type (US)", GAME_NO_COCKTAIL )
GAMEX( 1987, bchopper, 0,        m72,      bchopper, bchopper, ROT0,   "Irem", "Battle Chopper", GAME_NO_COCKTAIL )
GAMEX( 1987, mrheli,   bchopper, m72,      bchopper, mrheli,   ROT0,   "Irem", "Mr. HELI no Dai-Bouken", GAME_NO_COCKTAIL )
GAMEX( 1988, nspirit,  0,        m72,      nspirit,  nspirit,  ROT0,   "Irem", "Ninja Spirit", GAME_NO_COCKTAIL )
GAMEX( 1988, nspiritj, nspirit,  m72,      nspirit,  nspiritj, ROT0,   "Irem", "Saigo no Nindou (Japan)", GAME_NO_COCKTAIL )
GAME(  1988, imgfight, 0,        m72,      imgfight, imgfight, ROT270, "Irem", "Image Fight (Japan)" )
GAMEX( 1989, loht,     0,        m72,      loht,     loht,     ROT0,   "Irem", "Legend of Hero Tonma", GAME_NO_COCKTAIL )
GAMEX( 1989, xmultipl, 0,        xmultipl, xmultipl, xmultipl, ROT0,   "Irem", "X Multiply (Japan)", GAME_NO_COCKTAIL )
GAMECX(1989, dbreed,   0,        dbreed,   dbreed,   dbreed,   ROT0,   "Irem", "Dragon Breed", GAME_NO_COCKTAIL, &dbreed_ctrl, NULL )
GAMEX( 1989, rtype2,   0,        rtype2,   rtype2,   0,        ROT0,   "Irem", "R-Type II", GAME_NO_COCKTAIL )
GAMEX( 1989, rtype2j,  rtype2,   rtype2,   rtype2,   0,        ROT0,   "Irem", "R-Type II (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1990, majtitle, 0,        majtitle, rtype2,   0,        ROT0,   "Irem", "Major Title (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1990, hharry,   0,        hharry,   hharry,   0,        ROT0,   "Irem", "Hammerin' Harry (World)", GAME_NO_COCKTAIL )
GAMEX( 1990, hharryu,  hharry,   hharryu,  hharry,   0,        ROT0,   "Irem America", "Hammerin' Harry (US)", GAME_NO_COCKTAIL )
GAMEX( 1990, dkgensan, hharry,   hharryu,  hharry,   0,        ROT0,   "Irem", "Daiku no Gensan (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1990, dkgenm72, hharry,   dkgenm72, hharry,   dkgenm72, ROT0,   "Irem", "Daiku no Gensan (Japan, M72)", GAME_NO_COCKTAIL )
GAMEX( 1990, poundfor, 0,        poundfor, poundfor, 0,        ROT270, "Irem", "Pound for Pound (World)", GAME_NO_COCKTAIL )
GAMEX( 1990, poundfou, poundfor, poundfor, poundfor, 0,        ROT270, "Irem America", "Pound for Pound (US)", GAME_NO_COCKTAIL )
GAME(  1990, airduel,  0,        m72,      airduel,  airduel,  ROT270, "Irem", "Air Duel (Japan)" )
GAMEX( 1991, cosmccop, 0,        kengo,    gallop,   0,        ROT0,   "Irem", "Cosmic Cop (World)", GAME_NO_COCKTAIL )
GAMEX( 1991, gallop,   cosmccop, m72,      gallop,   gallop,   ROT0,   "Irem", "Gallop - Armed police Unit (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1991, kengo,    0,        kengo,    kengo,    kengo,    ROT0,   "Irem", "Ken-Go", GAME_NO_COCKTAIL )
