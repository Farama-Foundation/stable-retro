/***************************************************************************

Namco 52XX

This instance of the Fujitsu MB8852 MCU is programmed to act as a sample player.
It is used by just two games: Bosconian and Pole Position.

A0-A15 = address to read from sample ROMs
D0-D7 = data freom sample ROMs
CMD = command from CPU (sample to play, 0 = none)
OUT = sound output

      +------+
 EXTAL|1   42|Vcc
  XTAL|2   41|CMD3
/RESET|3   40|CMD2
  /IRQ|4   39|CMD1
  n.c.|5   38|CMD0
  [2] |6   37|A7
  n.c.|7   36|A6
  [1] |8   35|A5
  OUT0|9   34|A4
  OUT1|10  33|A3
  OUT2|11  32|A2
  OUT3|12  31|A1
    A8|13  30|A0
    A9|14  29|D7
   A10|15  28|D6
   A11|16  27|D5
[3]A12|17  26|D4
[3]A13|18  25|D3
[3]A14|19  24|D2
[3]A15|20  23|D1
   GND|21  22|D0
      +------+

[1] in polepos, GND; in bosco, output from a 555 timer
[2] in polepos, +5V; in bosco, GND
[3] in polepos, these are true address lines, in bosco they are chip select lines
    (each one select one of the four ROM chips). Behaviour related to [2]?

TODO:
- the purpose of the 555 timer in bosco is unknown; maybe modulate the output?

***************************************************************************/

#include "driver.h"
#include "namco52.h"

/* macro to convert 4-bit unsigned samples to 8-bit signed samples */
#define SAMPLE_CONV4(a) (0x11*((a&0x0f))-0x80)


static INT8 *samples;	/* 4-bit samples will be unpacked to 8-bit here */
static int channel;		/* channel assigned by the mixer */
static const struct namco_52xx_interface *intf;	/* pointer to our config data */
static UINT8 *rom;		/* pointer to sample ROM */
static int rom_len;


int namco_52xx_sh_start(const struct MachineSound *msound)
{
	int i;
	unsigned char bits;

	intf = msound->sound_interface;
	rom     = memory_region(intf->region);
	rom_len = memory_region_length(intf->region);

	channel = mixer_allocate_channel(intf->mixing_level);
	mixer_set_name(channel,sound_name(msound));

	samples = auto_malloc(2*rom_len);
	if (!samples)
		return 1;

	/* decode the rom samples */
	for (i = 0;i < rom_len;i++)
	{
		bits = rom[i] & 0x0f;
		samples[2*i] = SAMPLE_CONV4(bits);

		bits = (rom[i] & 0xf0) >> 4;
		samples[2*i + 1] = SAMPLE_CONV4(bits);
	}

	return 0;
}


void namco_52xx_sh_stop(void)
{
}


void namcoio_52XX_write(int data)
{
	data &= 0x0f;

/*logerror("%04x: custom 52XX write %02x\n",activecpu_get_pc(),data); */

	if (Machine->sample_rate == 0)
		return;

	if (data != 0)
	{
		int start = rom[data-1] + (rom[data-1+0x10] << 8);
		int end   = rom[data] + (rom[data+0x10] << 8);

		if (end >= rom_len)
			end = rom_len;

		if (start < end)
			mixer_play_sample(channel, samples + start * 2, (end - start) * 2, intf->baseclock/384, 0);
	}
}
