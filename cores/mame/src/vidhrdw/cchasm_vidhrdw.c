/***************************************************************************

	Cinematronics Cosmic Chasm hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "cchasm.h"

#define HALT   0
#define JUMP   1
#define COLOR  2
#define SCALEY 3
#define POSY   4
#define SCALEX 5
#define POSX   6
#define LENGTH 7

data16_t *cchasm_ram;

static int xcenter, ycenter;

static void cchasm_refresh_end (int dummy)
{
    cpu_set_irq_line (0, 2, ASSERT_LINE);
}

static void cchasm_refresh (void)
{

	int pc = 0;
    int done = 0;
    int opcode, data;
    int currentx = 0, currenty = 0;
    int scalex = 0, scaley = 0;
    int color = 0;
    int total_length = 1;   /* length of all lines drawn in a frame */
    int move = 0;

	vector_clear_list();

	while (!done)
	{
        data = cchasm_ram[pc];
   		opcode = data >> 12;
        data &= 0xfff;
        if ((opcode > COLOR) && (data & 0x800))
          data |= 0xfffff000;

		pc++;

		switch (opcode)
		{
        case HALT:
            done=1;
            break;
        case JUMP:
            pc = data - 0xb00;
            log_cb(RETRO_LOG_DEBUG, LOGPRE "JUMP to %x\n", data);
            break;
        case COLOR:
            color = VECTOR_COLOR444(data ^ 0xfff);
            break;
        case SCALEY:
            scaley = data << 5;
            break;
        case POSY:
            move = 1;
            currenty = ycenter + (data << 16);
            break;
        case SCALEX:
            scalex = data << 5;
            break;
        case POSX:
            move = 1;
            currentx = xcenter - (data << 16);
            break;
        case LENGTH:
            if (move)
            {
                vector_add_point (currentx, currenty, 0, 0);
                move = 0;
            }

            currentx -= data * scalex;
            currenty += data * scaley;

            total_length += abs(data);

            if (color)
                vector_add_point (currentx, currenty, color, 0xff);
            else
                move = 1;
            break;
        default:
            log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown refresh proc opcode %x with data %x at pc = %x\n", opcode, data, pc-2);
            done = 1;
            break;
		}
	}
    /* Refresh processor runs with 6 MHz */
    timer_set (TIME_IN_NSEC(166) * total_length, 0, cchasm_refresh_end);
}


WRITE16_HANDLER( cchasm_refresh_control_w )
{
	if (ACCESSING_MSB)
	{
		switch (data >> 8)
		{
		case 0x37:
			cchasm_refresh();
			break;
		case 0xf7:
			cpu_set_irq_line (0, 2, CLEAR_LINE);
			break;
		}
	}
}

VIDEO_START( cchasm )
{
	int xmin, xmax, ymin, ymax;

	xmin=Machine->visible_area.min_x;
	ymin=Machine->visible_area.min_y;
	xmax=Machine->visible_area.max_x;
	ymax=Machine->visible_area.max_y;

	xcenter=((xmax+xmin)/2) << 16;
	ycenter=((ymax+ymin)/2) << 16;

	return video_start_vector();
}
