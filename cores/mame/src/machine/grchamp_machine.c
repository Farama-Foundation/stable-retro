#include "driver.h"

extern UINT8 grchamp_videoreg0;
extern UINT8 grchamp_vreg1[0x10];
int grchamp_cpu_irq_enable[2];

static int comm_latch;
static int comm_latch2[4];

/***************************************************************************

	Machine Init

***************************************************************************/

DRIVER_INIT( grchamp ) {
	/* clear the irq latches */
	grchamp_cpu_irq_enable[0] = grchamp_cpu_irq_enable[1] = 0;

	/* if the coin system is 1 way, lock Coin B (Page 40) */
	if ( readinputport( 1 ) & 0x10 )
		coin_lockout_w( 1, 1 );
}

/*
	A note about port signals (note the preceding asterisk):
	*OUTxx = OUT port signals from CPU1
	OUTxx = OUT port signals from CPU2
*/

/***************************************************************************

	CPU 1

***************************************************************************/

READ_HANDLER( grchamp_port_0_r ) {
	return comm_latch;
}

extern WRITE_HANDLER( PC3259_control_w );

WRITE_HANDLER( grchamp_control0_w ){
	/* *OUT0 - Page 42 */
	/* bit 0 = trigger irq on cpu1 (itself) when vblank arrives */
	/* bit 1 = enable PC3259 (10A), page 41, top-left. TODO */
	/* bit 2/3 = unused */
	/* bit 4 = HEAD LAMP (1-D5) */
	/* bit 5 = CHANGE (1-D5?) */
	/* bit 6 = FOG OUT (1-E4) */
	/* bit 7 = RADAR ON (S26) */
	grchamp_videoreg0 = data;
	grchamp_cpu_irq_enable[0] = data & 1;	/* bit 0 */
/*	osd_led_w( 0, ( ~data >> 4 ) & 1 ); 	 // bit 4 /*/
}

WRITE_HANDLER( grchamp_coinled_w ){
	/* *OUT9 - Page 40 */
	/* bit 0-3 = unused */
	/* bit 4 = Coin Lockout */
	/* bit 5 = Game Over lamp */
	/* bit 6/7 = unused */
/*	coin_lockout_global_w( 0, ( data >> 4 ) & 1 );	 // bit 4 /*/
/*	osd_led_w( 1, ( ~data >> 5 ) & 1 ); 			 // bit 5 /*/
}

WRITE_HANDLER( grchamp_sound_w ){
	/* *OUT14 - Page 42 */
	soundlatch_w( 0, data );
	cpu_set_nmi_line( 2, PULSE_LINE );
}

WRITE_HANDLER( grchamp_comm_w ){
	/* *OUT16 - Page 40 */
	comm_latch2[ offset & 3] = data;
}

/***************************************************************************

	CPU 2

***************************************************************************/

READ_HANDLER( grchamp_port_1_r ) {
	return comm_latch2[offset];
}

WRITE_HANDLER( grchamp_port_1_w ) {
	grchamp_vreg1[offset] = data;

	switch( offset ) { 	/* OUT0 - OUTF (Page 48) */
		/* OUT0 - Page 43: writes to 'Left Synk Bus' */		/* bg0 yscroll lsb */
		/* OUT1 - Page 43: writes to 'Left Synk Bus' */		/* bg0 yscroll msb */
		/* OUT2 - Page 43: writes to 'Left Synk Bus' */		/* bg0 xscroll? */

	case 3: /* OUT3 - Page 45 */
		/*	bit0-bit3 = Analog Tachometer output
			bit4 = Palette selector. (Goes to A4 on the color prom). I believe this
				select between colors 0-15 and colors 16-31.
			bit5 = Center Layer 256H line enable. This bit enables/disables the
		   		256H line (The extra higher bit for scroll) on the center (road) layer.
			bit 6 and 7 = unused.
		*/
		break;

	case 4: /* OUT4	- Page 46 */
		/* trigger irq on cpu2 when vblank arrives */
		grchamp_cpu_irq_enable[1] = data & 1;
		break;

		/* OUT5 - unused */									/* bg1 yscroll lsb */
		/* OUT6 - unused */									/* bg1 yscroll msb */
		/* OUT7 - Page 44: writes to 'Right Synk Bus' */	/* bg1 xscroll? */

	case 8: /* OUT8 - Page 47 */
		comm_latch = data;
		break;

		/* OUT9 - Page 47: writes to 'Center Synk Bus' */	/* bg2 yscroll lsb */
		/* OUTA - Page 47: writes to 'Center Synk Bus' */	/* bg2 yscroll msb? */
		/* OUTB - Page 47: writes to 'Center Synk Bus' */	/* bg2 xscroll? */

	default:
		/* OUTC - Page 48: goes to connector Q-23 */
		/* OUTD - Page 48: goes to connector Q-25 */
		/* OUTE - Page 48: goes to connector Q-27 */
		/* OUTF - unused */
		break;
	}
}
