/***************************************************************************

	Cinematronics Cosmic Chasm hardware

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/z80fmly.h"
#include "cchasm.h"

static int sound_flags;

READ_HANDLER( cchasm_snd_io_r )
{
    int coin;

    switch (offset & 0x61 )
    {
    case 0x00:
        coin = (input_port_3_r (offset) >> 4) & 0x7;
        if (coin != 0x7) coin |= 0x8;
        return sound_flags | coin;

    case 0x01:
        return AY8910_read_port_0_r (offset);

    case 0x21:
        return AY8910_read_port_1_r (offset);

    case 0x40:
        return soundlatch_r (offset);

    case 0x41:
        sound_flags &= ~0x80;
        z80ctc_0_trg2_w (0, 0);
        return soundlatch2_r (offset);
    default:
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Read from unmapped internal IO device at 0x%x\n", offset + 0x6000);
        return 0;
    }
}

WRITE_HANDLER( cchasm_snd_io_w )
{
    switch (offset & 0x61 )
    {
    case 0x00:
        AY8910_control_port_0_w (offset, data);
        break;

    case 0x01:
        AY8910_write_port_0_w (offset, data);
        break;

    case 0x20:
        AY8910_control_port_1_w (offset, data);
        break;

    case 0x21:
        AY8910_write_port_1_w (offset, data);
        break;

    case 0x40:
        soundlatch3_w (offset, data);
        break;

    case 0x41:
        sound_flags |= 0x40;
        soundlatch4_w (offset, data);
        cpu_set_irq_line(0, 1, HOLD_LINE);
        break;

    case 0x61:
        z80ctc_0_trg0_w (0, 0);
        break;

    default:
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Write %x to unmapped internal IO device at 0x%x\n", data, offset + 0x6000);
    }
}

WRITE16_HANDLER( cchasm_io_w )
{
    static int led;

	if (ACCESSING_MSB)
	{
		data >>= 8;
		switch (offset & 0xf)
		{
		case 0:
			soundlatch_w (offset, data);
			break;
		case 1:
			sound_flags |= 0x80;
			soundlatch2_w (offset, data);
			z80ctc_0_trg2_w (0, 1);
			cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
			break;
		case 2:
			led = data;
			break;
		}
	}
}

READ16_HANDLER( cchasm_io_r )
{
	switch (offset & 0xf)
	{
	case 0x0:
		return soundlatch3_r (offset) << 8;
	case 0x1:
		sound_flags &= ~0x40;
		return soundlatch4_r (offset) << 8;
	case 0x2:
		return (sound_flags| (input_port_3_r (offset) & 0x07) | 0x08) << 8;
	case 0x5:
		return input_port_2_r (offset) << 8;
	case 0x8:
		return input_port_1_r (offset) << 8;
	default:
		return 0xff << 8;
	}
}

static int channel[2], channel_active[2];
static int output[2];

static void ctc_interrupt (int state)
{
	cpu_set_irq_line_and_vector(1, 0, HOLD_LINE, Z80_VECTOR(0,state));
}

static WRITE_HANDLER( ctc_timer_1_w )
{

    if (data) /* rising edge */
    {
        output[0] ^= 0x7f;
        channel_active[0] = 1;
        stream_update(channel[0], 0);
    }
}

static WRITE_HANDLER( ctc_timer_2_w )
{

    if (data) /* rising edge */
    {
        output[1] ^= 0x7f;
        channel_active[1] = 1;
        stream_update(channel[1], 0);
    }
}

static z80ctc_interface ctc_intf =
{
	1,                   /* 1 chip */
	{ 0 },               /* clock (filled in from the CPU 0 clock */
	{ 0 },               /* timer disables */
	{ ctc_interrupt },   /* interrupt handler */
	{ 0 },               /* ZC/TO0 callback */
	{ ctc_timer_1_w },     /* ZC/TO1 callback */
	{ ctc_timer_2_w }      /* ZC/TO2 callback */
};

static void tone_update(int num,INT16 *buffer,int length)
{
	INT16 out = 0;

	if (channel_active[num])
		out = output[num] << 8;

	while (length--) *(buffer++) = out;
	channel_active[num] = 0;
}

int cchasm_sh_start(const struct MachineSound *msound)
{
    sound_flags = 0;
    output[0] = 0; output[1] = 0;

    channel[0] = stream_init("CTC sound 1", 50, Machine->sample_rate, 0, tone_update);
    channel[1] = stream_init("CTC sound 2", 50, Machine->sample_rate, 1, tone_update);

	ctc_intf.baseclock[0] = Machine->drv->cpu[1].cpu_clock;
	z80ctc_init (&ctc_intf);

	return 0;
}

void cchasm_sh_update(void)
{
    if ((input_port_3_r (0) & 0x70) != 0x70)
        z80ctc_0_trg0_w (0, 1);
}
