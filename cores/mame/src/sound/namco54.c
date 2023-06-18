/***************************************************************************

Namco 54XX

This instance of the Fujitsu MB8852 MCU is programmed to act as a noise
generator (mostly for explosions).

CMD = command from CPU
OUTn = sound outputs (3 channels)

      +------+
 EXTAL|1   28|Vcc
  XTAL|2   27|CMD7
/RESET|3   26|CMD6
OUT0.0|4   25|CMD5
OUT0.1|5   24|CMD4
OUT0.2|6   23|/IRQ
OUT0.3|7   22|n.c.
OUT1.0|8   21|n.c.
OUT1.1|9   20|OUT2.3
OUT1.2|10  19|OUT2.2
OUT1.3|11  18|OUT2.1
  CMD0|12  17|OUT2.0
  CMD1|13  16|CMD3
   GND|14  15|CMD2
      +------+

TODO:
- The chip is not understood yet; this file is little more than a placeholder,
  playing samples when it receives known command sequences.

***************************************************************************/

#include "driver.h"
#include "namco54.h"



int namco_54xx_sh_start(const struct MachineSound *msound)
{
	return 0;
}


void namco_54xx_sh_stop(void)
{
}


void namcoio_54XX_write(int data)
{
	static int fetch;
	static int fetchmode;
	static UINT8 config1[4],config2[4],config3[5];


/*logerror("%04x: custom 54XX write %02x\n",activecpu_get_pc(),data); */

	if (fetch)
	{
		switch (fetchmode)
		{
			default:
			case 1:
				config1[4-(fetch--)] = data;
				break;

			case 2:
				config2[4-(fetch--)] = data;
				break;

			case 3:
				config3[5-(fetch--)] = data;
				break;
		}
	}
	else
	{
		switch (data & 0xf0)
		{
			case 0x00:	/* nop */
				break;

			case 0x10:	/* output sound on pins 4-7 only */
				if (memcmp(config1,"\x40\x00\x02\xdf",4) == 0)
					/* bosco */
					/* galaga */
					/* xevious */
					sample_start(0, 0, 0);
				else if (memcmp(config1,"\x10\x00\x80\xff",4) == 0)
					/* xevious */
					sample_start(0, 1, 0);
				else if (memcmp(config1,"\x80\x80\x01\xff",4) == 0)
					/* xevious */
					sample_start(0, 2, 0);
				break;

			case 0x20:	/* output sound on pins 8-11 only */
				if (memcmp(config2,"\x40\x40\x01\xff",4) == 0)
					/* xevious */
					sample_start(1, 3, 0);
				else if (memcmp(config2,"\x30\x30\x03\xdf",4) == 0)
					/* bosco */
					/* galaga */
					sample_start(1, 1, 0);
				else if (memcmp(config2,"\x60\x30\x03\x66",4) == 0)
					/* polepos */
					sample_start( 0, 0, 0 );
				break;

			case 0x30:
				fetch = 4;
				fetchmode = 1;
				break;

			case 0x40:
				fetch = 4;
				fetchmode = 2;
				break;

			case 0x50:	/* output sound on pins 17-20 only */
				if (memcmp(config3,"\x08\x04\x21\x00\xf1",5) == 0)
					/* bosco */
					sample_start(2, 2, 0);
				break;

			case 0x60:
				fetch = 5;
				fetchmode = 3;
				break;

			case 0x70:
				/* polepos */
				/* 0x7n = Screech sound. n = pitch (if 0 then no sound) */
				/* followed by 0x60 command? */
				if (( data & 0x0f ) == 0)
				{
					if (sample_playing(1))
						sample_stop(1);
				}
				else
				{
					int freq = (int)( ( 44100.0f / 10.0f ) * (float)(data & 0x0f) );

					if (!sample_playing(1))
						sample_start(1, 1, 1);
					sample_set_freq(1, freq);
				}
				break;
		}
	}
}
