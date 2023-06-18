#include "driver.h"
#include "machine/pd4990a.h"
#include "neogeo.h"
#include "inptport.h"
#include "state.h"
#include <time.h>


static int sram_locked;
static offs_t sram_protection_hack;
extern void *record;
extern void *playback;

extern int neogeo_rng;

data16_t *neogeo_ram16;
data16_t *neogeo_sram16;


/***************** MEMCARD GLOBAL VARIABLES ******************/
int mcd_action=0;
int mcd_number=0;
int memcard_status=0;		/* 1=Inserted 0=No card */
int memcard_number=0;		/* 000...999, -1=None */
int memcard_manager=0;		/* 0=Normal boot 1=Call memcard manager */
UINT8 *neogeo_memcard;		/* Pointer to 2kb RAM zone */

data8_t *neogeo_game_vectors;



static void neogeo_custom_memory(void);
static void neogeo_register_sub_savestate(void);


/* This function is called on every reset */
MACHINE_INIT( neogeo )
{
#if 0
	data16_t src, res, *mem16= (data16_t *)memory_region(REGION_USER1);
#endif
	time_t ltime;
	struct tm *today;


	/* Reset variables & RAM */
	memset (neogeo_ram16, 0, 0x10000);


#if 0
	/* Set up machine country */
	src = readinputport(5);
	res = src & 0x3;

	/* Console/arcade mode */
#ifndef CONSOLE
	res |= 0x8000;
#endif

	/* write the ID in the system BIOS ROM */
	mem16[0x0200] = res;

	if (memcard_manager==1)
	{
		memcard_manager=0;
		mem16[0x11b1a/2] = 0x500a;
	}
	else
	{
		mem16[0x11b1a/2] = 0x1b6a;
	}
#endif

	time(&ltime);
	today = localtime(&ltime);

	/* Disable Real Time Clock if the user selects to record or playback an .inp file   */
	/* This is needed in order to playback correctly an .inp on several games,as these  */
	/* use the RTC of the NEC pd4990a as pseudo-random number generator   -kal 8 apr 02 */
	if( record != 0 || playback != 0 )
	{
		pd4990a.seconds = 0;
		pd4990a.minutes = 0;
		pd4990a.hours = 0;
		pd4990a.days = 0;
		pd4990a.month = 0;
		pd4990a.year = 0;
		pd4990a.weekday = 0;
	}
	else
	{
		pd4990a.seconds = ((today->tm_sec/10)<<4) + (today->tm_sec%10);
		pd4990a.minutes = ((today->tm_min/10)<<4) + (today->tm_min%10);
		pd4990a.hours = ((today->tm_hour/10)<<4) + (today->tm_hour%10);
		pd4990a.days = ((today->tm_mday/10)<<4) + (today->tm_mday%10);
		pd4990a.month = (today->tm_mon + 1);
		pd4990a.year = (((today->tm_year%100)/10)<<4) + (today->tm_year%10);
		pd4990a.weekday = today->tm_wday;
	}

	neogeo_rng = 0x2345;	/* seed for the protection RNG in KOF99 onwards */
}


/* This function is only called once per game. */
DRIVER_INIT( neogeo )
{
	extern struct YM2610interface neogeo_ym2610_interface;
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	int tileno,numtiles;

	numtiles = memory_region_length(REGION_GFX3)/128;
	for (tileno = 0;tileno < numtiles;tileno++)
	{
		unsigned char swap[128];
		UINT8 *gfxdata;
		int x,y;
		unsigned int pen;

		gfxdata = &memory_region(REGION_GFX3)[128 * tileno];

		memcpy(swap,gfxdata,128);

		for (y = 0;y < 16;y++)
		{
			UINT32 dw;

			dw = 0;
			for (x = 0;x < 8;x++)
			{
				pen  = ((swap[64 + 4*y + 3] >> x) & 1) << 3;
				pen |= ((swap[64 + 4*y + 1] >> x) & 1) << 2;
				pen |= ((swap[64 + 4*y + 2] >> x) & 1) << 1;
				pen |=	(swap[64 + 4*y	  ] >> x) & 1;
				dw |= pen << 4*x;
			}
			*(gfxdata++) = dw>>0;
			*(gfxdata++) = dw>>8;
			*(gfxdata++) = dw>>16;
			*(gfxdata++) = dw>>24;

			dw = 0;
			for (x = 0;x < 8;x++)
			{
				pen  = ((swap[4*y + 3] >> x) & 1) << 3;
				pen |= ((swap[4*y + 1] >> x) & 1) << 2;
				pen |= ((swap[4*y + 2] >> x) & 1) << 1;
				pen |=	(swap[4*y	 ] >> x) & 1;
				dw |= pen << 4*x;
			}
			*(gfxdata++) = dw>>0;
			*(gfxdata++) = dw>>8;
			*(gfxdata++) = dw>>16;
			*(gfxdata++) = dw>>24;
		}
	}

	if (memory_region(REGION_SOUND2))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "using memory region %d for Delta T samples\n",REGION_SOUND2);
		neogeo_ym2610_interface.pcmromb[0] = REGION_SOUND2;
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "using memory region %d for Delta T samples\n",REGION_SOUND1);
		neogeo_ym2610_interface.pcmromb[0] = REGION_SOUND1;
	}

	/* Allocate ram banks */
	neogeo_ram16 = auto_malloc (0x10000);
	if (!neogeo_ram16)
		return;
	cpu_setbank(1, neogeo_ram16);

	/* Set the biosbank */
	cpu_setbank(3, memory_region(REGION_USER1));

	/* Set the 2nd ROM bank */
	if (memory_region_length(REGION_CPU1) > 0x100000)
		neogeo_set_cpu1_second_bank(0x100000);
	else
		neogeo_set_cpu1_second_bank(0x000000);

	/* Set the sound CPU ROM banks */
	neogeo_init_cpu2_setbank();

	/* Allocate and point to the memcard - bank 5 */
	neogeo_memcard = auto_malloc(0x800);
	if (!neogeo_memcard)
		return;
	memset(neogeo_memcard, 0, 0x800);
	memcard_status=0;
	memcard_number=0;


	mem16 = (data16_t *)memory_region(REGION_USER1);

#if 0
	if (mem16[0x11b00/2] == 0x4eba)
	{
		/* standard bios */
		neogeo_has_trackball = 0;
		/* Remove memory check for now */
		mem16[0x11b00/2] = 0x4e71;
		mem16[0x11b02/2] = 0x4e71;
		mem16[0x11b16/2] = 0x4ef9;
		mem16[0x11b18/2] = 0x00c1;
		mem16[0x11b1a/2] = 0x1b6a;

		/* Patch bios rom, for Calendar errors */
		mem16[0x11c14/2] = 0x4e71;
		mem16[0x11c16/2] = 0x4e71;
		mem16[0x11c1c/2] = 0x4e71;
		mem16[0x11c1e/2] = 0x4e71;

		/* Rom internal checksum fails for now.. */
		mem16[0x11c62/2] = 0x4e71;
		mem16[0x11c64/2] = 0x4e71;
	}
	else
	{
		/* special bios with trackball support */
		neogeo_has_trackball = 1;

		/* TODO: check the memcard manager patch in neogeo_init_machine(), */
		/* it probably has to be moved as well */
		/* Remove memory check for now */
		mem16[0x10c2a/2] = 0x4e71;
		mem16[0x10c2c/2] = 0x4e71;
		mem16[0x10c40/2] = 0x4ef9;
		mem16[0x10c42/2] = 0x00c1;
		mem16[0x10c44/2] = 0x0c94;

		/* Patch bios rom, for Calendar errors */
		mem16[0x10d3e/2] = 0x4e71;
		mem16[0x10d40/2] = 0x4e71;
		mem16[0x10d46/2] = 0x4e71;
		mem16[0x10d48/2] = 0x4e71;

		/* Rom internal checksum fails for now.. */
		mem16[0x10d8c/2] = 0x4e71;
		mem16[0x10d8e/2] = 0x4e71;
	}
#endif

	/* irritating maze uses a trackball */
	if (!strcmp(Machine->gamedrv->name,"irrmaze"))
		neogeo_has_trackball = 1;
	else
		neogeo_has_trackball = 0;


	{ /* info from elsemi, this is how nebula works, is there a better way in mame? */
		data8_t* gamerom = memory_region(REGION_CPU1);
		neogeo_game_vectors = auto_malloc (0x80);
		memcpy( neogeo_game_vectors, gamerom, 0x80 );
	}

	/* Install custom memory handlers */
	neogeo_custom_memory();

	/* register state save */
	neogeo_register_main_savestate();
	neogeo_register_sub_savestate();
}

/******************************************************************************/

WRITE16_HANDLER (neogeo_select_bios_vectors)
{
	data8_t* gamerom = memory_region(REGION_CPU1);
	data8_t* biosrom = memory_region(REGION_USER1);

	memcpy( gamerom, biosrom, 0x80 );
}

WRITE16_HANDLER (neogeo_select_game_vectors)
{
	data8_t* gamerom = memory_region(REGION_CPU1);
	memcpy( gamerom, neogeo_game_vectors, 0x80 );
}

/******************************************************************************/

static int prot_data;

static READ16_HANDLER( fatfury2_protection_16_r )
{
	data16_t res = (prot_data >> 24) & 0xff;

	switch (offset)
	{
		case 0x55550/2:
		case 0xffff0/2:
		case 0x00000/2:
		case 0xff000/2:
		case 0x36000/2:
		case 0x36008/2:
			return res;

		case 0x36004/2:
		case 0x3600c/2:
			return ((res & 0xf0) >> 4) | ((res & 0x0f) << 4);

		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "unknown protection read at pc %06x, offset %08x\n",activecpu_get_pc(),offset<<1);
			return 0;
	}
}

static WRITE16_HANDLER( fatfury2_protection_16_w )
{
	switch (offset)
	{
		case 0x55552/2:	 /* data == 0x5555; read back from 55550, ffff0, 00000, ff000 */
			prot_data = 0xff00ff00;
			break;

		case 0x56782/2:	 /* data == 0x1234; read back from 36000 *or* 36004 */
			prot_data = 0xf05a3601;
			break;

		case 0x42812/2:	 /* data == 0x1824; read back from 36008 *or* 3600c */
			prot_data = 0x81422418;
			break;

		case 0x55550/2:
		case 0xffff0/2:
		case 0xff000/2:
		case 0x36000/2:
		case 0x36004/2:
		case 0x36008/2:
		case 0x3600c/2:
			prot_data <<= 8;
			break;

		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "unknown protection write at pc %06x, offset %08x, data %02x\n",activecpu_get_pc(),offset,data);
			break;
	}
}

static READ16_HANDLER( popbounc_sfix_16_r )
{
	if (activecpu_get_pc()==0x6b10)
		return 0;
	return neogeo_ram16[0x4fbc/2];
}


static WRITE16_HANDLER( kof99_bankswitch_w )
{
	int bankaddress;
	static int bankoffset[64] =
	{
		0x000000, 0x100000, 0x200000, 0x300000,
		0x3cc000, 0x4cc000, 0x3f2000, 0x4f2000,
		0x407800, 0x507800, 0x40d000, 0x50d000,
		0x417800, 0x517800, 0x420800, 0x520800,
		0x424800, 0x524800, 0x429000, 0x529000,
		0x42e800, 0x52e800, 0x431800, 0x531800,
		0x54d000, 0x551000, 0x567000, 0x592800,
		0x588800, 0x581800, 0x599800, 0x594800,
		0x200000,	/* rest not used? */
	};

	/* unscramble bank number */
	data =
		(((data>>14)&1)<<0)+
		(((data>> 6)&1)<<1)+
		(((data>> 8)&1)<<2)+
		(((data>>10)&1)<<3)+
		(((data>>12)&1)<<4)+
		(((data>> 5)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static WRITE16_HANDLER( garou_bankswitch_w )
{
	/* thanks to Razoola and Mr K for the info */
	int bankaddress;
	static int bankoffset[64] =
	{
		0x000000, 0x100000, 0x200000, 0x300000, /* 00*/
		0x280000, 0x380000, 0x2d0000, 0x3d0000, /* 04*/
		0x2f0000, 0x3f0000, 0x400000, 0x500000, /* 08*/
		0x420000, 0x520000, 0x440000, 0x540000, /* 12*/
		0x498000, 0x598000, 0x4a0000, 0x5a0000, /* 16*/
		0x4a8000, 0x5a8000, 0x4b0000, 0x5b0000, /* 20*/
		0x4b8000, 0x5b8000, 0x4c0000, 0x5c0000, /* 24*/
		0x4c8000, 0x5c8000, 0x4d0000, 0x5d0000, /* 28*/
		0x458000, 0x558000, 0x460000, 0x560000, /* 32*/
		0x468000, 0x568000, 0x470000, 0x570000, /* 36*/
		0x478000, 0x578000, 0x480000, 0x580000, /* 40*/
		0x488000, 0x588000, 0x490000, 0x590000, /* 44*/
		0x5d0000, 0x5d8000, 0x5e0000, 0x5e8000, /* 48*/
		0x5f0000, 0x5f8000, 0x600000, /* rest not used? */
	};

	/* unscramble bank number */
	data =
		(((data>> 5)&1)<<0)+
		(((data>> 9)&1)<<1)+
		(((data>> 7)&1)<<2)+
		(((data>> 6)&1)<<3)+
		(((data>>14)&1)<<4)+
		(((data>>12)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static WRITE16_HANDLER( garouo_bankswitch_w )
{
	/* thanks to Razoola and Mr K for the info */
	int bankaddress;
	static int bankoffset[64] =
	{
		0x000000, 0x100000, 0x200000, 0x300000, /* 00*/
		0x280000, 0x380000, 0x2d0000, 0x3d0000, /* 04*/
		0x2c8000, 0x3c8000, 0x400000, 0x500000, /* 08*/
		0x420000, 0x520000, 0x440000, 0x540000, /* 12*/
		0x598000, 0x698000, 0x5a0000, 0x6a0000, /* 16*/
		0x5a8000, 0x6a8000, 0x5b0000, 0x6b0000, /* 20*/
		0x5b8000, 0x6b8000, 0x5c0000, 0x6c0000, /* 24*/
		0x5c8000, 0x6c8000, 0x5d0000, 0x6d0000, /* 28*/
		0x458000, 0x558000, 0x460000, 0x560000, /* 32*/
		0x468000, 0x568000, 0x470000, 0x570000, /* 36*/
		0x478000, 0x578000, 0x480000, 0x580000, /* 40*/
		0x488000, 0x588000, 0x490000, 0x590000, /* 44*/
		0x5d8000, 0x6d8000, 0x5e0000, 0x6e0000, /* 48*/
		0x5e8000, 0x6e8000, 0x6e8000, 0x000000, /* 52*/
		0x000000, 0x000000, 0x000000, 0x000000, /* 56*/
		0x000000, 0x000000, 0x000000, 0x000000, /* 60*/
	};

	/* unscramble bank number */
	data =
		(((data>> 4)&1)<<0)+
		(((data>> 8)&1)<<1)+
		(((data>>14)&1)<<2)+
		(((data>> 2)&1)<<3)+
		(((data>>11)&1)<<4)+
		(((data>>13)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static WRITE16_HANDLER( mslug3_bankswitch_w )
{
	/* thanks to Razoola and Mr K for the info */
	int bankaddress;
	static int bankoffset[64] =
	{
	  0x000000, 0x020000, 0x040000, 0x060000, /* 00*/
	  0x070000, 0x090000, 0x0b0000, 0x0d0000, /* 04*/
	  0x0e0000, 0x0f0000, 0x120000, 0x130000, /* 08*/
	  0x140000, 0x150000, 0x180000, 0x190000, /* 12*/
	  0x1a0000, 0x1b0000, 0x1e0000, 0x1f0000, /* 16*/
	  0x200000, 0x210000, 0x240000, 0x250000, /* 20*/
	  0x260000, 0x270000, 0x2a0000, 0x2b0000, /* 24*/
	  0x2c0000, 0x2d0000, 0x300000, 0x310000, /* 28*/
	  0x320000, 0x330000, 0x360000, 0x370000, /* 32*/
	  0x380000, 0x390000, 0x3c0000, 0x3d0000, /* 36*/
	  0x400000, 0x410000, 0x440000, 0x450000, /* 40*/
	  0x460000, 0x470000, 0x4a0000, 0x4b0000, /* 44*/
	  0x4c0000, /* rest not used? */
	};

	/* unscramble bank number */
	data =
		(((data>>14)&1)<<0)+
		(((data>>12)&1)<<1)+
		(((data>>15)&1)<<2)+
		(((data>> 6)&1)<<3)+
		(((data>> 3)&1)<<4)+
		(((data>> 9)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static WRITE16_HANDLER( kof2000_bankswitch_w )
{
	/* thanks to Razoola and Mr K for the info */
	int bankaddress;
	static int bankoffset[64] =
	{
		0x000000, 0x100000, 0x200000, 0x300000, /* 00*/
		0x3f7800, 0x4f7800, 0x3ff800, 0x4ff800, /* 04*/
		0x407800, 0x507800, 0x40f800, 0x50f800, /* 08*/
		0x416800, 0x516800, 0x41d800, 0x51d800, /* 12*/
		0x424000, 0x524000, 0x523800, 0x623800, /* 16*/
		0x526000, 0x626000, 0x528000, 0x628000, /* 20*/
		0x52a000, 0x62a000, 0x52b800, 0x62b800, /* 24*/
		0x52d000, 0x62d000, 0x52e800, 0x62e800, /* 28*/
		0x618000, 0x619000, 0x61a000, 0x61a800, /* 32*/
	};

	/* unscramble bank number */
	data =
		(((data>>15)&1)<<0)+
		(((data>>14)&1)<<1)+
		(((data>> 7)&1)<<2)+
		(((data>> 3)&1)<<3)+
		(((data>>10)&1)<<4)+
		(((data>> 5)&1)<<5);

	bankaddress = 0x100000 + bankoffset[data];

	neogeo_set_cpu1_second_bank(bankaddress);
}


static READ16_HANDLER( prot_9a37_r )
{
	return 0x9a37;
}


static void neogeo_custom_memory(void)
{
	/* Individual games can go here... */

	/* kludges */

	if (!Machine->sample_rate &&
			!strcmp(Machine->gamedrv->name,"popbounc"))
	/* the game hangs after a while without this patch */
		install_mem_read16_handler(0, 0x104fbc, 0x104fbd, popbounc_sfix_16_r);

	if (!strcmp(Machine->gamedrv->name,"kof99") || !strcmp(Machine->gamedrv->name,"kof99a") || !strcmp(Machine->gamedrv->name,"kof99e"))
	{
		/* special ROM banking handler */
		install_mem_write16_handler(0, 0x2ffff0, 0x2ffff1, kof99_bankswitch_w);

		/* additional protection */
		install_mem_read16_handler(0, 0x2fe446, 0x2fe447, prot_9a37_r);
	}

	if (!strcmp(Machine->gamedrv->name,"garou"))
	{
		/* special ROM banking handler */
		install_mem_write16_handler(0, 0x2fffc0, 0x2fffc1, garou_bankswitch_w);

		/* additional protection */
		install_mem_read16_handler(0, 0x2fe446, 0x2fe447, prot_9a37_r);
	}

	if (!strcmp(Machine->gamedrv->name,"garouo"))
	{
		/* special ROM banking handler */
		install_mem_write16_handler(0, 0x2fffc0, 0x2fffc1, garouo_bankswitch_w);

		/* additional protection */
		install_mem_read16_handler(0, 0x2fe446, 0x2fe447, prot_9a37_r);
	}

	if (!strcmp(Machine->gamedrv->name,"mslug3"))
	{
		/* special ROM banking handler */
		install_mem_write16_handler(0, 0x2fffe4, 0x2fffe5, mslug3_bankswitch_w);

		/* additional protection */
		install_mem_read16_handler(0, 0x2fe446, 0x2fe447, prot_9a37_r);
	}

	if (!strcmp(Machine->gamedrv->name,"kof2000"))
	{
		/* special ROM banking handler */
		install_mem_write16_handler(0, 0x2fffec, 0x2fffed, kof2000_bankswitch_w);

		/* additional protection */
		install_mem_read16_handler(0, 0x2fe446, 0x2fe447, prot_9a37_r);
	}

	/* hacks to make the games which do protection checks run in arcade mode */
	/* we write protect a SRAM location so it cannot be set to 1 */
	sram_protection_hack = ~0;
	if (	!strcmp(Machine->gamedrv->name,"fatfury3") ||
			!strcmp(Machine->gamedrv->name,"samsho5") ||
			!strcmp(Machine->gamedrv->name,"samsh5sp") ||
			!strcmp(Machine->gamedrv->name,"samsho3") ||
			!strcmp(Machine->gamedrv->name,"samsho4") ||
			!strcmp(Machine->gamedrv->name,"aof3") ||
			!strcmp(Machine->gamedrv->name,"rbff1") ||
			!strcmp(Machine->gamedrv->name,"rbffspec") ||
			!strcmp(Machine->gamedrv->name,"kof95") ||
			!strcmp(Machine->gamedrv->name,"kof96") ||
			!strcmp(Machine->gamedrv->name,"kof96h") ||
			!strcmp(Machine->gamedrv->name,"kof97") ||
			!strcmp(Machine->gamedrv->name,"kof97a") ||
			!strcmp(Machine->gamedrv->name,"kof97pls") ||
			!strcmp(Machine->gamedrv->name,"kog") ||
			!strcmp(Machine->gamedrv->name,"kof98") ||
			!strcmp(Machine->gamedrv->name,"kof98n") ||
			!strcmp(Machine->gamedrv->name,"kof98k") ||
			!strcmp(Machine->gamedrv->name,"kof98") ||
			!strcmp(Machine->gamedrv->name,"kof98n") ||
			!strcmp(Machine->gamedrv->name,"kof99") ||
			!strcmp(Machine->gamedrv->name,"kof99a") ||
			!strcmp(Machine->gamedrv->name,"kof99e") ||
			!strcmp(Machine->gamedrv->name,"kof99n") ||
			!strcmp(Machine->gamedrv->name,"kof99p") ||
			!strcmp(Machine->gamedrv->name,"kof2000") ||
			!strcmp(Machine->gamedrv->name,"kof2000n") ||
			!strcmp(Machine->gamedrv->name,"kizuna") ||
			!strcmp(Machine->gamedrv->name,"lastblad") ||
			!strcmp(Machine->gamedrv->name,"lastblda") ||
			!strcmp(Machine->gamedrv->name,"lastbld2") ||
			!strcmp(Machine->gamedrv->name,"rbff2") ||
			!strcmp(Machine->gamedrv->name,"rbff2a") ||
			!strcmp(Machine->gamedrv->name,"mslug2") ||
      !strcmp(Machine->gamedrv->name,"mslug2t") ||
			!strcmp(Machine->gamedrv->name,"mslug3") ||
			!strcmp(Machine->gamedrv->name,"garou") ||
			!strcmp(Machine->gamedrv->name,"garouo") ||
			!strcmp(Machine->gamedrv->name,"garoup"))
		sram_protection_hack = 0x100/2;

	if (!strcmp(Machine->gamedrv->name,"pulstar"))
		sram_protection_hack = 0x35a/2;

	if (!strcmp(Machine->gamedrv->name,"ssideki"))
	{
		/* patch out protection check */
		/* the protection routines are at 0x25dcc and involve reading and writing */
		/* addresses in the 0x2xxxxx range */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0x2240/2] = 0x4e71;
	}

	/* Hacks the program rom of Fatal Fury 2, needed either in arcade or console mode */
	/* otherwise at level 2 you cannot hit the opponent and other problems */
	if (!strcmp(Machine->gamedrv->name,"fatfury2"))
	{
		/* there seems to also be another protection check like the countless ones */
		/* patched above by protecting a SRAM location, but that trick doesn't work */
		/* here (or maybe the SRAM location to protect is different), so I patch out */
		/* the routine which trashes memory. Without this, the game goes nuts after */
		/* the first bonus stage. */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0xb820/2] = 0x4e71;
		mem16[0xb822/2] = 0x4e71;

		/* again, the protection involves reading and writing addresses in the */
		/* 0x2xxxxx range. There are several checks all around the code. */
		install_mem_read16_handler(0, 0x200000, 0x2fffff, fatfury2_protection_16_r);
		install_mem_write16_handler(0, 0x200000, 0x2fffff, fatfury2_protection_16_w);
	}

	if (!strcmp(Machine->gamedrv->name,"fatfury3"))
	{
		/* patch the first word, it must be 0x0010 not 0x0000 (initial stack pointer) */
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
		mem16[0x0000/2] = 0x0010;
	}

	if (!strcmp(Machine->gamedrv->name,"mslugx"))
	{
		/* patch out protection checks */
		int i;
		data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);

		for (i = 0;i < (0x100000/2) - 4;i++)
		{
			if (mem16[i+0] == 0x0243 &&
				mem16[i+1] == 0x0001 && 	/* andi.w  #$1, D3 */
				mem16[i+2] == 0x6600)		/* bne xxxx */
			{
				mem16[i+2] = 0x4e71;
				mem16[i+3] = 0x4e71;
			}
		}

		mem16[0x3bdc/2] = 0x4e71;
		mem16[0x3bde/2] = 0x4e71;
		mem16[0x3be0/2] = 0x4e71;
		mem16[0x3c0c/2] = 0x4e71;
		mem16[0x3c0e/2] = 0x4e71;
		mem16[0x3c10/2] = 0x4e71;

		mem16[0x3c36/2] = 0x4e71;
		mem16[0x3c38/2] = 0x4e71;
	}

	{
		/*AT: Patches a common bug in the sound codes of early ADK games where DEC's*/
		/*    should have been INC's. Magician Lord is unaffected and ADK appeared to*/
		/*    to have fixed it in World Heroes and later games.*/

/* patches no longer needed, the YM2610 emulator handles the wrong calls correctly.*/

/*		data8_t *mem8 = memory_region(REGION_CPU2);*/

/*		if (!strcmp(Machine->gamedrv->name,"ncombat")) mem8[0xeb99] = 0x0c;*/
/*		if (!strcmp(Machine->gamedrv->name,"bjourney")) mem8[0xec7a] = 0x0c;*/
/*		if (!strcmp(Machine->gamedrv->name,"crsword")) mem8[0x23db] = 0x0c;*/
/*		if (!strcmp(Machine->gamedrv->name,"trally")) mem8[0x23e4] = 0x0c;*/
/*		if (!strcmp(Machine->gamedrv->name,"ncommand")) mem8[0x2456]= mem8[0x2485] = 0x0c;*/
	}
}



WRITE16_HANDLER( neogeo_sram16_lock_w )
{
	sram_locked = 1;
}

WRITE16_HANDLER( neogeo_sram16_unlock_w )
{
	sram_locked = 0;
}

READ16_HANDLER( neogeo_sram16_r )
{
	return neogeo_sram16[offset];
}

WRITE16_HANDLER( neogeo_sram16_w )
{
	if (sram_locked)
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %06x: warning: write %02x to SRAM %04x while it was protected\n",activecpu_get_pc(),data,offset<<1);
	}
	else
	{
		if (offset == sram_protection_hack)
		{
			if (ACCESSING_LSB && (data & 0xff) == 0x01)
				return; /* fake protection pass */
		}

		COMBINE_DATA(&neogeo_sram16[offset]);
	}
}

NVRAM_HANDLER( neogeo )
{
	if (read_or_write)
	{
		/* Save the SRAM settings */
		mame_fwrite_msbfirst(file,neogeo_sram16,0x2000);

		/* save the memory card */
		neogeo_memcard_save();
	}
	else
	{
		/* Load the SRAM settings for this game */
		if (file)
			mame_fread_msbfirst(file,neogeo_sram16,0x2000);
		else
			memset(neogeo_sram16,0,0x10000);

		/* load the memory card */
		neogeo_memcard_load(memcard_number);
	}
}



/*
	INFORMATION:

	Memory card is a 2kb battery backed RAM.
	It is accessed thru 0x800000-0x800FFF.
	Even bytes are always 0xFF
	Odd bytes are memcard data (0x800 bytes)

	Status byte at 0x380000: (BITS ARE ACTIVE *LOW*)

	0 PAD1 START
	1 PAD1 SELECT
	2 PAD2 START
	3 PAD2 SELECT
	4 --\  MEMORY CARD
	5 --/  INSERTED
	6 MEMORY CARD WRITE PROTECTION
	7 UNUSED (?)
*/




/********************* MEMCARD ROUTINES **********************/
READ16_HANDLER( neogeo_memcard16_r )
{
	if (memcard_status==1)
		return neogeo_memcard[offset] | 0xff00;
	else
		return ~0;
}

WRITE16_HANDLER( neogeo_memcard16_w )
{
	if (ACCESSING_LSB)
	{
		if (memcard_status==1)
			neogeo_memcard[offset] = data & 0xff;
	}
}

int neogeo_memcard_load(int number)
{
	char name[16];
	mame_file *f;

	sprintf(name, "MEMCARD.%03d", number);
	if ((f=mame_fopen(0, name, FILETYPE_MEMCARD,0))!=0)
	{
		mame_fread(f,neogeo_memcard,0x800);
		mame_fclose(f);
		return 1;
	}
	return 0;
}

void neogeo_memcard_save(void)
{
	char name[16];
	mame_file *f;

	if (memcard_number!=-1)
	{
		sprintf(name, "MEMCARD.%03d", memcard_number);
		if ((f=mame_fopen(0, name, FILETYPE_MEMCARD,1))!=0)
		{
			mame_fwrite(f,neogeo_memcard,0x800);
			mame_fclose(f);
		}
	}
}

void neogeo_memcard_eject(void)
{
   if (memcard_number!=-1)
   {
	   neogeo_memcard_save();
	   memset(neogeo_memcard, 0, 0x800);
	   memcard_status=0;
	   memcard_number=-1;
   }
}

int neogeo_memcard_create(int number)
{
	char buf[0x800];
	char name[16];
	mame_file *f1, *f2;

	sprintf(name, "MEMCARD.%03d", number);
	if ((f1=mame_fopen(0, name, FILETYPE_MEMCARD,0))==0)
	{
		if ((f2=mame_fopen(0, name, FILETYPE_MEMCARD,1))!=0)
		{
			mame_fwrite(f2,buf,0x800);
			mame_fclose(f2);
			return 1;
		}
	}
	else
		mame_fclose(f1);

	return 0;
}

/******************************************************************************/

static void neogeo_register_sub_savestate(void)
{
	data8_t* gamevector = memory_region(REGION_CPU1);

	state_save_register_int   ("neogeo", 0, "sram_locked",             &sram_locked);
	state_save_register_UINT16("neogeo", 0, "neogeo_ram16",            neogeo_ram16,             0x10000/2);
	state_save_register_UINT8 ("neogeo", 0, "neogeo_memcard",          neogeo_memcard,           0x800);
	state_save_register_UINT8 ("neogeo", 0, "gamevector",              gamevector,               0x80);
	state_save_register_int   ("neogeo", 0, "mcd_action",              &mcd_action);
	state_save_register_int   ("neogeo", 0, "mcd_number",              &mcd_number);
	state_save_register_int   ("neogeo", 0, "memcard_status",          &memcard_status);
	state_save_register_int   ("neogeo", 0, "memcard_number",          &memcard_number);
	state_save_register_int   ("neogeo", 0, "memcard_manager",         &memcard_manager);
	state_save_register_int   ("neogeo", 0, "prot_data",               &prot_data);
}
