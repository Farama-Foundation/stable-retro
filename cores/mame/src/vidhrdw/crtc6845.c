/**********************************************************************

        Motorola 6845 CRT Controller interface and emulation

        This function emulates the functionality of a single
        crtc6845.

        This is just a storage shell for crtc6845 variables due
        to the fact that the hardware implementation makes a big
        difference and that is done by the specific vidhrdw code.

**********************************************************************/

#define CRTC6845_C

#include "driver.h"
#include "crtc6845.h"


READ_HANDLER( crtc6845_register_r )
{
	int retval=0;

	switch(crtc6845_address_latch)
	{
		case 0:
			retval=crtc6845_horiz_total;
			break;
		case 1:
			retval=crtc6845_horiz_disp;
			break;
		case 2:
			retval=crtc6845_horiz_sync_pos;
			break;
		case 3:
			retval=crtc6845_sync_width;
			break;
		case 4:
			retval=crtc6845_vert_total;
			break;
		case 5:
			retval=crtc6845_vert_total_adj;
			break;
		case 6:
			retval=crtc6845_vert_disp;
			break;
		case 7:
			retval=crtc6845_vert_sync_pos;
			break;
		case 8:
			retval=crtc6845_intl_skew;
			break;
		case 9:
			retval=crtc6845_max_ras_addr;
			break;
		case 10:
			retval=crtc6845_cursor_start_ras;
			break;
		case 11:
			retval=crtc6845_cursor_end_ras;
			break;
		case 12:
			retval=(crtc6845_start_addr&0x3f)>>8;
			break;
		case 13:
			retval=crtc6845_start_addr&0xff;
			break;
		case 14:
			retval=(crtc6845_cursor&0x3f)>>8;
			break;
		case 15:
			retval=crtc6845_cursor&0xff;
			break;
		case 16:
			retval=(crtc6845_light_pen&0x3f)>>8;
			break;
		case 17:
			retval=crtc6845_light_pen&0xff;
			break;
		default:
			break;
	}
        return retval;
}


WRITE_HANDLER( crtc6845_address_w )
{
	crtc6845_address_latch=data&0x1f;
}


WRITE_HANDLER( crtc6845_register_w )
{

log_cb(RETRO_LOG_DEBUG, LOGPRE "CRT #0 PC %04x: WRITE reg 0x%02x data 0x%02x\n",activecpu_get_pc(),crtc6845_address_latch,data);

	switch(crtc6845_address_latch)
	{
		case 0:
			crtc6845_horiz_total=data;
			break;
		case 1:
			crtc6845_horiz_disp=data;
			break;
		case 2:
			crtc6845_horiz_sync_pos=data;
			break;
		case 3:
			crtc6845_sync_width=data;
			break;
		case 4:
			crtc6845_vert_total=data&0x7f;
			break;
		case 5:
			crtc6845_vert_total_adj=data&0x1f;
			break;
		case 6:
			crtc6845_vert_disp=data&0x7f;
			break;
		case 7:
			crtc6845_vert_sync_pos=data&0x7f;
			break;
		case 8:
			crtc6845_intl_skew=data;
			break;
		case 9:
			crtc6845_max_ras_addr=data&0x1f;
			break;
		case 10:
			crtc6845_cursor_start_ras=data&0x7f;
			break;
		case 11:
			crtc6845_cursor_end_ras=data&0x1f;
			break;
		case 12:
			crtc6845_start_addr&=0x00ff;
			crtc6845_start_addr|=(data&0x3f)<<8;
			crtc6845_page_flip=data&0x40;
			break;
		case 13:
			crtc6845_start_addr&=0xff00;
			crtc6845_start_addr|=data;
			break;
		case 14:
			crtc6845_cursor&=0x00ff;
			crtc6845_cursor|=(data&0x3f)<<8;
			break;
		case 15:
			crtc6845_cursor&=0xff00;
			crtc6845_cursor|=data;
			break;
		case 16:
			crtc6845_light_pen&=0x00ff;
			crtc6845_light_pen|=(data&0x3f)<<8;
			break;
		case 17:
			crtc6845_light_pen&=0xff00;
			crtc6845_light_pen|=data;
			break;
		default:
			break;
	}
}
