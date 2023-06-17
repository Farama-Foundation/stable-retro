/***************************************************************************

  machine.c

  Street Heat runs on the same type board as theglobp. It features 4 decryption algorythms
that can be implemented sequentially using the IN command. See machine\theglobp.c for more
details.  The advantage of this is weak sections can be isolated.  The key that contains the
text is obviously easily broken as is the boot section.  The remaining 2 keys are not used
much which means there's not much data to look at.  Knowing the last command in a section
will be IN is enough of a clue to break it though.

  This system is fairly effective in preventing decrypted roms.  Certain key addresses are
read in all 4 states. That is easily patched, but in at least some of the games encrypted
program code is read in the wrong key as data and used as sound data.  A
plaintext version of the roms produces things like 30 second long coinup sounds.

  - David Widel (d_widel@hotmail.com)

***************************************************************************/

#include "driver.h"

/*void machine_init_pacman(void);*/

static int counter=0;


static void strtheat_decrypt_rom_8(void)
{
	int oldbyte,inverted_oldbyte,newbyte;
	int mem;
	unsigned char *RAM;

	RAM = memory_region(REGION_CPU1);


	for (mem=0;mem<0x4000;mem++)
	{
		oldbyte = RAM[mem];
		inverted_oldbyte = ~oldbyte;

		/*	Note: D2 is inverted and connected to D1, D5 is inverted and
			connected to D0.  The other six data bits are converted by a
			PAL10H8 driven by the counter. */
		newbyte = 0;

		/*  start  c  */
		newbyte  = (inverted_oldbyte & 0x80) >> 3;
		newbyte |= (inverted_oldbyte & 0x40) >> 0;
		newbyte |= (inverted_oldbyte & 0x20) >> 5;
		newbyte |= (inverted_oldbyte & 0x10) >> 2;
		newbyte |= (inverted_oldbyte & 0x08) >> 0;
		newbyte |= (inverted_oldbyte & 0x04) >> 1;
		newbyte |= (oldbyte & 0x02) << 4;
		newbyte |= (oldbyte & 0x01) << 7;

		RAM[mem + 0x10000] = newbyte;
	}

	return;
}


static void strtheat_decrypt_rom_9(void)
{
	int oldbyte,inverted_oldbyte,newbyte;
	int mem;
	unsigned char *RAM;

	RAM = memory_region(REGION_CPU1);

	for (mem=0;mem<0x4000;mem++)
	{
		oldbyte = RAM[mem];
		inverted_oldbyte = ~oldbyte;

		/*	Note: D2 is inverted and connected to D1, D5 is inverted and
			connected to D0.  The other six data bits are converted by a
			PAL10H8 driven by the counter. */
		newbyte = 0;

		/*   d  */
		newbyte  = (oldbyte & 0x80) >> 5;
		newbyte |= (inverted_oldbyte & 0x40) << 0;
		newbyte |= (inverted_oldbyte & 0x20) >> 5;
		newbyte |= (inverted_oldbyte & 0x10) << 1;

		newbyte |= (inverted_oldbyte & 0x08) << 0;
		newbyte |= (inverted_oldbyte & 0x04) >> 1;
		newbyte |= (inverted_oldbyte & 0x02) << 3;
		newbyte |= (oldbyte & 0x01) << 7;





		RAM[mem + 0x14000] = newbyte;
	}

	return;
}

static void strtheat_decrypt_rom_A(void)
{
	int oldbyte,inverted_oldbyte,newbyte;
	int mem;
	unsigned char *RAM;

	RAM = memory_region(REGION_CPU1);

	for (mem=0;mem<0x4000;mem++)
	{
		oldbyte = RAM[mem];
		inverted_oldbyte = ~oldbyte;

		/*	Note: D2 is inverted and connected to D1, D5 is inverted and
			connected to D0.  The other six data bits are converted by a
			PAL10H8 driven by the counter. */
		newbyte = 0;


		/* DAHF bGcE   dis B   */



		newbyte  = (inverted_oldbyte & 0x80) >> 3;
		newbyte |= (inverted_oldbyte & 0x40) << 1;
		newbyte |= (inverted_oldbyte & 0x20) >> 5;
		newbyte |= (inverted_oldbyte & 0x10) >> 2;

		newbyte |= (oldbyte & 0x08) << 3;
		newbyte |= (inverted_oldbyte & 0x04) >> 1;
		newbyte |= (oldbyte & 0x02) << 4;
		newbyte |= (inverted_oldbyte & 0x01) << 3;


		RAM[mem + 0x18000] = newbyte;


	}

	return;
}

static void strtheat_decrypt_rom_B(void)
{
	int oldbyte,inverted_oldbyte,newbyte;
	int mem;
	unsigned char *RAM;

	RAM = memory_region(REGION_CPU1);

	for (mem=0;mem<0x4000;mem++)
	{
		oldbyte = RAM[mem];
		inverted_oldbyte = ~oldbyte;

		/*	Note: D2 is inverted and connected to D1, D5 is inverted and
			connected to D0.  The other six data bits are converted by a
			PAL10H8 driven by the counter. */
		newbyte = 0;

			/*  a  ascii fAHCbGDE  */
		newbyte  = (oldbyte & 0x80) >> 5;
		newbyte |= (inverted_oldbyte & 0x40) << 1;
		newbyte |= (inverted_oldbyte & 0x20) >> 5;
		newbyte |= (inverted_oldbyte & 0x10) << 1;
		newbyte |= (oldbyte & 0x08) << 3;
		newbyte |= (inverted_oldbyte & 0x04) >> 1;
		newbyte |= (inverted_oldbyte & 0x02) << 3;
		newbyte |= (inverted_oldbyte & 0x01) << 3;



		RAM[mem + 0x1C000] = newbyte;
	}

	return;
}


READ_HANDLER( strtheat_decrypt_rom )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	if (offset & 0x01)
	{
		counter = counter - 1;
		if (counter < 0)
			counter = 0x0F;
	}
	else
	{
		counter = (counter + 1) & 0x0F;
	}

	switch(counter)
	{
		case 0x08:	cpu_setbank (1, &RAM[0x10000]);		break;
		case 0x09:	cpu_setbank (1, &RAM[0x14000]);		break;
		case 0x0A:	cpu_setbank (1, &RAM[0x18000]);		break;
		case 0x0B:	cpu_setbank (1, &RAM[0x1C000]);		break;
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Invalid counter = %02X\n",counter);
			break;
	}

	return 0;
}


MACHINE_INIT( strtheat )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* While the PAL supports up to 16 decryption methods, only four
		are actually used in the PAL.  Therefore, we'll take a little
		memory overhead and decrypt the ROMs using each method in advance. */
	strtheat_decrypt_rom_8();
	strtheat_decrypt_rom_9();
	strtheat_decrypt_rom_A();
	strtheat_decrypt_rom_B();

	/* The initial state of the counter is 0x0B */
	counter = 0x08;
	cpu_setbank (1, &RAM[0x10000]);

	/*machine_init_pacman();*/
}

/*
WRITE_HANDLER( strtheat_writeport)
{
log_cb(RETRO_LOG_DEBUG, LOGPRE "Port Write: pc = %4x ############## \n",activecpu_get_pc());
}
*/
