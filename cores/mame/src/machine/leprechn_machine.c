#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "includes/leprechn.h"


static data8_t input_port_select;

static WRITE_HANDLER( leprechn_input_port_select_w )
{
    input_port_select = data;
}

static READ_HANDLER( leprechn_input_port_r )
{
    switch (input_port_select)
    {
    case 0x01:
        return input_port_0_r(0);
    case 0x02:
        return input_port_2_r(0);
    case 0x04:
        return input_port_3_r(0);
    case 0x08:
        return input_port_1_r(0);
    case 0x40:
        return input_port_5_r(0);
    case 0x80:
        return input_port_4_r(0);
    }

    return 0xff;
}


static WRITE_HANDLER( leprechn_coin_counter_w )
{
	coin_counter_w(offset, !data);
}


static WRITE_HANDLER( leprechn_sh_w )
{
    soundlatch_w(offset,data);
    cpu_set_irq_line(1,M6502_IRQ_LINE,HOLD_LINE);
}



static struct via6522_interface leprechn_via_0_interface =
{
	/*inputs : A/B         */ 0, leprechn_videoram_r,
	/*inputs : CA/B1,CA/B2 */ 0, 0, 0, 0,
	/*outputs: A/B,CA/B2   */ leprechn_videoram_w, leprechn_graphics_command_w, 0, 0,
	/*irq                  */ 0
};

static struct via6522_interface leprechn_via_1_interface =
{
	/*inputs : A/B         */ leprechn_input_port_r, 0,
	/*inputs : CA/B1,CA/B2 */ 0, 0, 0, 0,
	/*outputs: A/B,CA/B2   */ 0, leprechn_input_port_select_w, 0, leprechn_coin_counter_w,
	/*irq                  */ 0
};

static struct via6522_interface leprechn_via_2_interface =
{
	/*inputs : A/B         */ 0, 0,
	/*inputs : CA/B1,CA/B2 */ 0, 0, 0, 0,
	/*outputs: A/B,CA/B2   */ leprechn_sh_w, 0, 0, 0,
	/*irq                  */ 0
};


DRIVER_INIT( leprechn )
{
	via_config(0, &leprechn_via_0_interface);
	via_config(1, &leprechn_via_1_interface);
	via_config(2, &leprechn_via_2_interface);

	via_reset();
}


READ_HANDLER( leprechn_sh_0805_r )
{
    return 0xc0;
}
