/*##########################################################################

	atarigen.h

	General functions for Atari raster games.

##########################################################################*/

#include "driver.h"
#include "vidhrdw/atarimo.h"
#include "vidhrdw/atarirle.h"

#ifndef __MACHINE_ATARIGEN__
#define __MACHINE_ATARIGEN__


/*##########################################################################
	CONSTANTS
##########################################################################*/

#define ATARI_CLOCK_14MHz	14318180
#define ATARI_CLOCK_20MHz	20000000
#define ATARI_CLOCK_32MHz	32000000
#define ATARI_CLOCK_50MHz	50000000



/*##########################################################################
	TYPES & STRUCTURES
##########################################################################*/

typedef void (*atarigen_int_callback)(void);

typedef void (*atarigen_scanline_callback)(int scanline);

struct atarivc_state_desc
{
	int latch1;								/* latch #1 value (-1 means disabled) */
	int latch2;								/* latch #2 value (-1 means disabled) */
	int rowscroll_enable;					/* true if row-scrolling is enabled */
	int palette_bank;						/* which palette bank is enabled */
	int pf0_xscroll;						/* playfield 1 xscroll */
	int pf0_xscroll_raw;					/* playfield 1 xscroll raw value */
	int pf0_yscroll;						/* playfield 1 yscroll */
	int pf1_xscroll;						/* playfield 2 xscroll */
	int pf1_xscroll_raw;					/* playfield 2 xscroll raw value */
	int pf1_yscroll;						/* playfield 2 yscroll */
	int mo_xscroll;							/* sprite xscroll */
	int mo_yscroll;							/* sprite xscroll */
};



/*##########################################################################
	GLOBALS
##########################################################################*/

extern UINT8			atarigen_scanline_int_state;
extern UINT8			atarigen_sound_int_state;
extern UINT8			atarigen_video_int_state;

extern const data16_t *	atarigen_eeprom_default;
extern data16_t *		atarigen_eeprom;
extern size_t 			atarigen_eeprom_size;

extern UINT8			atarigen_cpu_to_sound_ready;
extern UINT8			atarigen_sound_to_cpu_ready;

extern data16_t *		atarigen_playfield;
extern data16_t *		atarigen_playfield2;
extern data16_t *		atarigen_playfield_upper;
extern data16_t *		atarigen_alpha;
extern data16_t *		atarigen_alpha2;
extern data16_t *		atarigen_xscroll;
extern data16_t *		atarigen_yscroll;

extern data32_t *		atarigen_playfield32;
extern data32_t *		atarigen_alpha32;

extern struct tilemap *	atarigen_playfield_tilemap;
extern struct tilemap *	atarigen_playfield2_tilemap;
extern struct tilemap *	atarigen_alpha_tilemap;
extern struct tilemap *	atarigen_alpha2_tilemap;

extern data16_t *		atarivc_data;
extern data16_t *		atarivc_eof_data;
extern struct atarivc_state_desc atarivc_state;



/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

/*---------------------------------------------------------------
	INTERRUPT HANDLING
---------------------------------------------------------------*/

void atarigen_interrupt_reset(atarigen_int_callback update_int);
void atarigen_update_interrupts(void);

void atarigen_scanline_int_set(int scanline);
INTERRUPT_GEN( atarigen_scanline_int_gen );
WRITE16_HANDLER( atarigen_scanline_int_ack_w );
WRITE32_HANDLER( atarigen_scanline_int_ack32_w );

INTERRUPT_GEN( atarigen_sound_int_gen );
WRITE16_HANDLER( atarigen_sound_int_ack_w );
WRITE32_HANDLER( atarigen_sound_int_ack32_w );

INTERRUPT_GEN( atarigen_video_int_gen );
WRITE16_HANDLER( atarigen_video_int_ack_w );
WRITE32_HANDLER( atarigen_video_int_ack32_w );


/*---------------------------------------------------------------
	EEPROM HANDLING
---------------------------------------------------------------*/

void atarigen_eeprom_reset(void);

WRITE16_HANDLER( atarigen_eeprom_enable_w );
WRITE16_HANDLER( atarigen_eeprom_w );
READ16_HANDLER( atarigen_eeprom_r );
READ16_HANDLER( atarigen_eeprom_upper_r );

WRITE32_HANDLER( atarigen_eeprom_enable32_w );
WRITE32_HANDLER( atarigen_eeprom32_w );
READ32_HANDLER( atarigen_eeprom_upper32_r );

NVRAM_HANDLER( atarigen );
void atarigen_hisave(void);


/*---------------------------------------------------------------
	SLAPSTIC HANDLING
---------------------------------------------------------------*/

void atarigen_slapstic_init(int cpunum, int base, int chipnum);
void atarigen_slapstic_reset(void);

WRITE16_HANDLER( atarigen_slapstic_w );
READ16_HANDLER( atarigen_slapstic_r );


/*---------------------------------------------------------------
	SOUND I/O
---------------------------------------------------------------*/

void atarigen_sound_io_reset(int cpu_num);

INTERRUPT_GEN( atarigen_6502_irq_gen );
READ_HANDLER( atarigen_6502_irq_ack_r );
WRITE_HANDLER( atarigen_6502_irq_ack_w );

void atarigen_ym2151_irq_gen(int irq);

WRITE16_HANDLER( atarigen_sound_w );
READ16_HANDLER( atarigen_sound_r );
WRITE16_HANDLER( atarigen_sound_upper_w );
READ16_HANDLER( atarigen_sound_upper_r );

WRITE32_HANDLER( atarigen_sound_upper32_w );
READ32_HANDLER( atarigen_sound_upper32_r );

void atarigen_sound_reset(void);
WRITE16_HANDLER( atarigen_sound_reset_w );
WRITE_HANDLER( atarigen_6502_sound_w );
READ_HANDLER( atarigen_6502_sound_r );


/*---------------------------------------------------------------
	SOUND HELPERS
---------------------------------------------------------------*/

void atarigen_init_6502_speedup(int cpunum, int compare_pc1, int compare_pc2);
void atarigen_set_ym2151_vol(int volume);
void atarigen_set_ym2413_vol(int volume);
void atarigen_set_pokey_vol(int volume);
void atarigen_set_tms5220_vol(int volume);
void atarigen_set_oki6295_vol(int volume);


/*---------------------------------------------------------------
	VIDEO CONTROLLER
---------------------------------------------------------------*/

void atarivc_reset(data16_t *eof_data, int playfields);
void atarivc_update(const data16_t *data);

WRITE16_HANDLER( atarivc_w );
READ16_HANDLER( atarivc_r );

static INLINE void atarivc_update_pf_xscrolls(void)
{
	atarivc_state.pf0_xscroll = atarivc_state.pf0_xscroll_raw + ((atarivc_state.pf1_xscroll_raw) & 7);
	atarivc_state.pf1_xscroll = atarivc_state.pf1_xscroll_raw + 4;
}


/*---------------------------------------------------------------
	PLAYFIELD/ALPHA MAP HELPERS
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_alpha_w );
WRITE32_HANDLER( atarigen_alpha32_w );
WRITE16_HANDLER( atarigen_alpha2_w );
void atarigen_set_playfield_latch(int data);
void atarigen_set_playfield2_latch(int data);
WRITE16_HANDLER( atarigen_playfield_w );
WRITE32_HANDLER( atarigen_playfield32_w );
WRITE16_HANDLER( atarigen_playfield_large_w );
WRITE16_HANDLER( atarigen_playfield_upper_w );
WRITE16_HANDLER( atarigen_playfield_dual_upper_w );
WRITE16_HANDLER( atarigen_playfield_latched_lsb_w );
WRITE16_HANDLER( atarigen_playfield_latched_msb_w );
WRITE16_HANDLER( atarigen_playfield2_w );
WRITE16_HANDLER( atarigen_playfield2_latched_msb_w );


/*---------------------------------------------------------------
	VIDEO HELPERS
---------------------------------------------------------------*/

void atarigen_scanline_timer_reset(atarigen_scanline_callback update_graphics, int frequency);
int atarigen_get_hblank(void);
WRITE16_HANDLER( atarigen_halt_until_hblank_0_w );
WRITE16_HANDLER( atarigen_666_paletteram_w );
WRITE16_HANDLER( atarigen_expanded_666_paletteram_w );
WRITE32_HANDLER( atarigen_666_paletteram32_w );


/*---------------------------------------------------------------
	MISC HELPERS
---------------------------------------------------------------*/

void atarigen_swap_mem(void *ptr1, void *ptr2, int bytes);
void atarigen_blend_gfx(int gfx0, int gfx1, int mask0, int mask1);



/*##########################################################################
	GENERAL ATARI NOTES
############################################################################
	
	Atari 68000 list:
	
	Driver		Pr? Up?	VC?	PF?	P2?	MO?	AL? BM? PH?
	----------	---	---	---	---	---	---	--- ---	---
	arcadecl.c		 *				 *		 *
	atarig1.c		 *		 *		rle	 *
	atarig42.c		 *		 *		rle	 *
	atarigt.c				 *		rle	 *
	atarigx2.c				 *		rle	 *
	atarisy1.c	 *	 *		 *		 *	 *				270->260
	atarisy2.c	 *	 *		 *		 *	 *				150->120
	badlands.c		 *		 *		 *					250->260
	batman.c	 *	 *	 *	 *	 *	 *	 *		 *		200->160 ?
	blstroid.c		 *		 *		 *					240->230
	cyberbal.c		 *		 *	 	 *	 *				125->105 ?
	eprom.c			 *		 *		 *	 *				170->170
	gauntlet.c	 *	 *		 *		 *	 *		 *		220->250
	klax.c		 *	 *		 *		 *					480->440 ?
	offtwall.c		 *	 *	 *		 *					260->260
	rampart.c		 *				 *		 *			280->280
	relief.c	 *	 *	 *	 *	 *	 *					240->240
	shuuz.c			 *	 *	 *		 *					410->290 fix!
	skullxbo.c		 *	 	 *		 *	 *				150->145
	thunderj.c		 *	 *	 *	 *	 *	 *		 *		180->180
	toobin.c		 *		 *		 *	 *				140->115 fix!
	vindictr.c	 *	 *		 *		 *	 *		 *		200->210
	xybots.c	 *	 *		 *		 *	 *				235->238
	----------	---	---	---	---	---	---	--- ---	---

	Pr? - do we have verifiable proof on priorities?
	Up? - have we updated to use new MO's & tilemaps?
	VC? - does it use the video controller?
	PF? - does it have a playfield?
	P2? - does it have a dual playfield?
	MO? - does it have MO's?
	AL? - does it have an alpha layer?
	BM? - does it have a bitmap layer?
	PH? - does it use the palette hack?

##########################################################################*/


#endif
