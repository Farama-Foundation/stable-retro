/***************************************************************************

  The Konami_1 CPU is a 6809 with opcodes scrambled. Here is how to
  descramble them.

***************************************************************************/

#include "driver.h"



unsigned char decodebyte( unsigned char opcode, unsigned short address )
{
/*
>
> CPU_D7 = (EPROM_D7 & ~ADDRESS_1) | (~EPROM_D7 & ADDRESS_1)  >
> CPU_D6 = EPROM_D6
>
> CPU_D5 = (EPROM_D5 & ADDRESS_1) | (~EPROM_D5 & ~ADDRESS_1) >
> CPU_D4 = EPROM_D4
>
> CPU_D3 = (EPROM_D3 & ~ADDRESS_3) | (~EPROM_D3 & ADDRESS_3) >
> CPU_D2 = EPROM_D2
>
> CPU_D1 = (EPROM_D1 & ADDRESS_3) | (~EPROM_D1 & ~ADDRESS_3) >
> CPU_D0 = EPROM_D0
>
*/
	unsigned char xormask;


	xormask = 0;
	if (address & 0x02) xormask |= 0x80;
	else xormask |= 0x20;
	if (address & 0x08) xormask |= 0x08;
	else xormask |= 0x02;

	return opcode ^ xormask;
}



static void decode(int cpu)
{
	unsigned char *rom = memory_region(REGION_CPU1+cpu);
	int diff = memory_region_length(REGION_CPU1+cpu) / 2;
	int A;


	memory_set_opcode_base(cpu,rom+diff);

	for (A = 0;A < diff;A++)
	{
		rom[A+diff] = decodebyte(rom[A],A);
	}
}

void konami1_decode(void)
{
	decode(0);
}

void konami1_decode_cpu2(void)
{
	decode(1);
}
