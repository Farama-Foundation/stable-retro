/***************************************************************************

Namco System II

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6805/m6805.h"
#include "namcos2.h"
#include "vidhrdw/generic.h"
#include "machine/random.h"

data16_t *namcos2_68k_master_ram;
data16_t *namcos2_68k_slave_ram;

int namcos2_gametype;

static unsigned mFinalLapProtCount;

READ16_HANDLER( namcos2_flap_prot_r )
{
	const data16_t table0[8] = { 0x0000,0x0040,0x0440,0x2440,0x2480,0xa080,0x8081,0x8041 };
	const data16_t table1[8] = { 0x0040,0x0060,0x0060,0x0860,0x0864,0x08e4,0x08e5,0x08a5 };
	data16_t data;

	switch( offset )
	{
	case 0:
		data = 0x0101;
		break;

	case 1:
		data = 0x3e55;
		break;

	case 2:
		data = table1[mFinalLapProtCount&7];
		data = (data&0xff00)>>8;
		break;

	case 3:
		data = table1[mFinalLapProtCount&7];
		mFinalLapProtCount++;
		data = data&0x00ff;
		break;

	case 0x3fffc/2:
		data = table0[mFinalLapProtCount&7];
		data = data&0xff00;
		break;

	case 0x3fffe/2:
		data = table0[mFinalLapProtCount&7];
		mFinalLapProtCount++;
		data = (data&0x00ff)<<8;
		break;

	default:
		data = 0;
	}
	return data;
}

/*************************************************************/
/* Perform basic machine initialisation 					 */
/*************************************************************/

MACHINE_INIT( namcos2 ){
	int loop;

	mFinalLapProtCount = 0;

	/* Initialise the bank select in the sound CPU */
	namcos2_sound_bankselect_w(0,0); /* Page in bank 0 */

	/* Place CPU2 & CPU3 into the reset condition */
	cpu_set_reset_line(NAMCOS2_CPU3, ASSERT_LINE);
	cpu_set_reset_line(NAMCOS2_CPU2, ASSERT_LINE);
	cpu_set_reset_line(NAMCOS2_CPU4, ASSERT_LINE);

	/* Initialise interrupt handlers */
	for(loop=0;loop<20;loop++)
	{
		namcos2_68k_master_C148[loop]=0;
		namcos2_68k_slave_C148[loop]=0;
	}
}

/*************************************************************/
/* EEPROM Load/Save and read/write handling 				 */
/*************************************************************/

data16_t *namcos2_eeprom;
size_t namcos2_eeprom_size;

NVRAM_HANDLER( namcos2 ){
	if( read_or_write ){
		mame_fwrite (file, namcos2_eeprom, namcos2_eeprom_size);
	}
	else {
		if (file)
		{
			mame_fread (file, namcos2_eeprom, namcos2_eeprom_size);
		}
		else
		{
			int pat = 0xff; /* default */
			if( namcos2_gametype == NAMCOS21_STARBLADE )
			{
				pat = 0x00;
			}
			memset (namcos2_eeprom, pat, namcos2_eeprom_size);
		}
	}
}

WRITE16_HANDLER( namcos2_68k_eeprom_w ){
	COMBINE_DATA( &namcos2_eeprom[offset] );
}

READ16_HANDLER( namcos2_68k_eeprom_r ){
	return namcos2_eeprom[offset];
}

/*************************************************************/
/* 68000 Shared memory area - Data ROM area 				 */
/*************************************************************/
READ16_HANDLER( namcos2_68k_data_rom_r ){
	data16_t *ROM = (data16_t *)memory_region(REGION_USER1);
	return ROM[offset];
}



/**************************************************************/
/* 68000 Shared serial communications processor (CPU5?) 	  */
/**************************************************************/

data16_t  namcos2_68k_serial_comms_ctrl[0x8];
data16_t *namcos2_68k_serial_comms_ram;

READ16_HANDLER( namcos2_68k_serial_comms_ram_r ){
	return namcos2_68k_serial_comms_ram[offset];
}

WRITE16_HANDLER( namcos2_68k_serial_comms_ram_w ){
	COMBINE_DATA( &namcos2_68k_serial_comms_ram[offset] );
}

READ16_HANDLER( namcos2_68k_serial_comms_ctrl_r )
{
	data16_t retval = namcos2_68k_serial_comms_ctrl[offset];

	switch(offset){
	case 0x00:
		retval |= 0x0004; 	/* Set READY? status bit */
		break;

	default:
		break;
	}
	return retval;
}

WRITE16_HANDLER( namcos2_68k_serial_comms_ctrl_w )
{
	COMBINE_DATA( &namcos2_68k_serial_comms_ctrl[offset] );
}

/*************************************************************/
/* 68000 Shared protection/random key generator 			 */
/*************************************************************

Custom chip ID numbers:

Game		Year	ID (dec)	ID (hex)
--------	----	---			-----
finallap	1987
assault		1988	unused
metlhawk	1988
ordyne		1988	176			$00b0
mirninja	1988	177			$00b1
phelios		1988	178			$00b2	readme says 179
dirtfoxj	1989	180			$00b4
fourtrax	1989
valkyrie	1989
finehour	1989	188			$00bc
burnforc	1989	189			$00bd
marvland	1989	190			$00be
kyukaidk	1990	191			$00bf
dsaber		1990	192			$00c0
finalap2	1990	318			$013e
rthun2		1990	319			$013f
gollygho	1990				$0143
cosmogng	1991	330			$014a
sgunner2	1991	346			$015a	ID out of order; gfx board is not standard
finalap3	1992	318			$013e	same as finalap2
suzuka8h	1992
sws92		1992	331			$014b
sws92g		1992	332			$014c
suzuk8h2	1993
sws93		1993	334			$014e
 *************************************************************/
static int sendval = 0;
READ16_HANDLER( namcos2_68k_key_r )
{
	switch (namcos2_gametype)
	{
	case NAMCOS2_ORDYNE:
		switch(offset)
		{
		case 2: return 0x1001;
		case 3: return 0x1;
		case 4: return 0x110;
		case 5: return 0x10;
		case 6: return 0xB0;
		case 7: return 0xB0;
		}
		break;

	case NAMCOS2_STEEL_GUNNER_2:
		switch( offset )
		{
			case 4: return 0x15a;
		}
	    break;

	case NAMCOS2_MIRAI_NINJA:
		switch(offset)
		{
		case 7: return 0xB1;
		}
	    break;

	case NAMCOS2_PHELIOS:
		switch(offset)
		{
		case 0: return 0xF0;
		case 1: return 0xFF0;
		case 2: return 0xB2;
		case 3: return 0xB2;
		case 4: return 0xF;
		case 5: return 0xF00F;
		case 7: return 0xB2;
		}
	    break;

	case NAMCOS2_DIRT_FOX_JP:
		switch(offset)
		{
		case 1: return 0xB4;
		}
		break;

	case NAMCOS2_FINEST_HOUR:
		switch(offset)
		{
		case 7: return 0xBC;
		}
	    break;

	case NAMCOS2_BURNING_FORCE:
		switch(offset)
		{
		case 1: return 0xBD;
		case 7: return 0xBD;
		}
		break;

	case NAMCOS2_MARVEL_LAND:
		switch(offset)
		{
		case 0: return 0x10;
		case 1: return 0x110;
		case 4: return 0xBE;
		case 6: return 0x1001;
		case 7: return (sendval==1)?0xBE:1;
		}
	    break;

	case NAMCOS2_DRAGON_SABER:
		switch(offset)
		{
		case 2: return 0xC0;
		}
		break;

	case NAMCOS2_ROLLING_THUNDER_2:
		switch(offset)
		{
		case 4:
			if( sendval == 1 ){
		        sendval = 0;
		        return 0x13F;
			}
			break;
		case 7:
			if( sendval == 1 ){
			    sendval = 0;
			    return 0x13F;
			}
			break;
		case 2: return 0;
		}
		break;

	case NAMCOS2_COSMO_GANG:
		switch(offset)
		{
		case 3: return 0x14A;
		}
		break;

	case NAMCOS2_SUPER_WSTADIUM_92:
		switch(offset)
		{
		case 3: return 0x14B;
		}
		break;

	case NAMCOS2_SUPER_WSTADIUM_92T:
		switch(offset)
		{
		case 3: return 0x14C;
		}
		break;

	case NAMCOS2_SUPER_WSTADIUM_93:
		switch(offset)
		{
		case 3: return 0x14E;
		}
		break;

	case NAMCOS2_SUZUKA_8_HOURS_2:
		switch(offset)
		{
		case 3: return 0x14D;
		case 2: return 0;
		}
		break;

	case NAMCOS2_GOLLY_GHOST:
		switch(offset)
		{
		case 0: return 2;
		case 1: return 2;
		case 2: return 0;
		case 4: return 0x143;
		}
		break;
	}
	return mame_rand()&0xffff;
}

WRITE16_HANDLER( namcos2_68k_key_w )
{
	if ((namcos2_gametype == NAMCOS2_MARVEL_LAND) && (offset == 5))
	{
		if (data == 0x615E) sendval = 1;
	}
	if ((namcos2_gametype == NAMCOS2_ROLLING_THUNDER_2) && (offset == 4)) {
		if (data == 0x13EC) sendval = 1;
	}
	if ((namcos2_gametype == NAMCOS2_ROLLING_THUNDER_2) && (offset == 7)) {
		if (data == 0x13EC) sendval = 1;
	}
	if ((namcos2_gametype == NAMCOS2_MARVEL_LAND) && (offset == 6)) {
		if (data == 0x1001) sendval = 0;
	}
}

/*************************************************************/
/* 68000 Interrupt/IO Handlers - CUSTOM 148 - NOT SHARED	 */
/*************************************************************/

#define NO_OF_LINES 	256
#define FRAME_TIME		(1.0/60.0)
#define LINE_LENGTH 	(FRAME_TIME/NO_OF_LINES)

data16_t  namcos2_68k_master_C148[0x20];
data16_t  namcos2_68k_slave_C148[0x20];

static data16_t
ReadC148( int cpu, offs_t offset )
{
	offs_t addr = ((offset*2)+0x1c0000)&0x1fe000;
	data16_t *pC148Reg;
	if( cpu == CPU_SLAVE )
	{
		pC148Reg = namcos2_68k_slave_C148;
	}
	else /* cpu == CPU_MASTER */
	{
		pC148Reg = namcos2_68k_master_C148;
	}

	switch( addr )
	{
	case 0x1d8000: /* ack EXIRQ */
		cpu_set_irq_line(cpu, pC148Reg[NAMCOS2_C148_EXIRQ], CLEAR_LINE);
		break;

	case 0x1da000: /* ack POSIRQ */
		cpu_set_irq_line(cpu, pC148Reg[NAMCOS2_C148_POSIRQ], CLEAR_LINE);
		break;

	case 0x1dc000: /* ack NAMCOS2_C148_SERIRQ */
		cpu_set_irq_line(cpu, pC148Reg[NAMCOS2_C148_SERIRQ], CLEAR_LINE);
		break;

	case 0x1de000: /* ack VBLANK */
		cpu_set_irq_line(cpu, pC148Reg[NAMCOS2_C148_VBLANKIRQ], CLEAR_LINE);
		break;

	case 0x1e0000: /* EEPROM Status Register */
		return ~0; /* Only BIT0 used: 1=EEPROM READY 0=EEPROM BUSY */

	default:
		break;
	}
	return pC148Reg[(addr>>13)&0x1f];
}

static void
WriteC148( int cpu, offs_t offset, data16_t data )
{
	offs_t addr = ((offset*2)+0x1c0000)&0x1fe000;
	int altCPU;
	data16_t *pC148Reg;
	if( cpu == CPU_SLAVE )
	{
		altCPU = CPU_MASTER;
		pC148Reg = namcos2_68k_slave_C148;
	}
	else /* cpu == CPU_MASTER */
	{
		altCPU = CPU_SLAVE;
		pC148Reg = namcos2_68k_master_C148;
	}

	pC148Reg[(addr>>13)&0x1f] = data&0x0007;

	switch(addr){
	case 0x1c6000: /* Master/Slave IRQ level */
	case 0x1c8000: /* EXIRQ level */
	case 0x1ca000: /* POSIRQ level */
	case 0x1cc000: /* SCIRQ level */
	case 0x1ce000: /* VBLANK level */
		break;

	case 0x1d4000: /* trigger master/slave interrupt */
		if( cpu == CPU_MASTER )
		{
			cpu_set_irq_line(altCPU, namcos2_68k_slave_C148[NAMCOS2_C148_CPUIRQ], ASSERT_LINE);
		}
		else /* cpu == CPU_SLAVE */
		{
			cpu_set_irq_line(altCPU, namcos2_68k_master_C148[NAMCOS2_C148_CPUIRQ], ASSERT_LINE);
		}
		break;

	case 0x1d6000: /* ack master/slave interrupt */
	case 0x1d8000: /* ack EXIRQ */
	case 0x1da000: /* ack POSIRQ */
	case 0x1dc000: /* ack SCIRQ */
	case 0x1de000: /* ack VBLANK */
		(void)ReadC148( cpu, offset );
		break;

	case 0x1e2000: /* Sound CPU Reset control */
		if( cpu == CPU_MASTER )
		{
			if(data&0x01)
			{
				/* Resume execution */
				cpu_set_reset_line (NAMCOS2_CPU3, CLEAR_LINE);
				cpu_yield();
			}
			else
			{
				/* Suspend execution */
				cpu_set_reset_line(NAMCOS2_CPU3, ASSERT_LINE);
			}
		}
		else
		{ /* HACK! */
			cpu_set_irq_line(
				CPU_SLAVE,
				namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ],
				HOLD_LINE);
		}
		break;

	case 0x1e4000: /* Alt 68000 & IO CPU Reset */
		if( cpu == CPU_MASTER )
		{
			if(data&0x01)
			{
				/* Resume execution */
				cpu_set_reset_line(altCPU, CLEAR_LINE);
				cpu_set_reset_line(NAMCOS2_CPU4, CLEAR_LINE);
				/* Give the new CPU an immediate slice of the action */
				cpu_yield();
			}
			else
			{
				/* Suspend execution */
				cpu_set_reset_line(altCPU, ASSERT_LINE);
				cpu_set_reset_line(NAMCOS2_CPU4, ASSERT_LINE);
			}
		}
		break;

	case 0x1e6000: /* Watchdog reset kicker */
		/* watchdog_reset_w(0,0); */
		break;

	default:
		break;
	}
}


WRITE16_HANDLER( namcos2_68k_master_C148_w ){
	WriteC148( CPU_MASTER, offset, data );
}

READ16_HANDLER( namcos2_68k_master_C148_r ){
	return ReadC148( CPU_MASTER, offset );
}

void namcos2_68k_master_posirq( int scanline ){
	force_partial_update(scanline);
	cpu_set_irq_line(CPU_MASTER , namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ] , ASSERT_LINE);
}

static int
GetPosIRQScanline( void )
{
	int scanline;
	switch( namcos2_gametype )
	{
		case NAMCOS2_FOUR_TRAX:
		scanline = 160;
		break;

		case NAMCOS2_SUZUKA_8_HOURS_2:
		case NAMCOS2_SUZUKA_8_HOURS:
		scanline = 56;
		break;

		case NAMCOS2_LUCKY_AND_WILD:
		scanline = 40;
		break;

		case NAMCOS2_FINEST_HOUR:
		scanline = 192;
		break;

		case NAMCOS2_BURNING_FORCE:
		scanline = 24; /* ? */
		break;

		default: /* Final Lap */
		scanline = 64;
		break;
	}
	return scanline;
}

INTERRUPT_GEN( namcos2_68k_master_vblank )
{
	if(namcos2_68k_master_C148[NAMCOS2_C148_POSIRQ])
	{
		int scanline = GetPosIRQScanline();
		timer_set( cpu_getscanlinetime(scanline), scanline, namcos2_68k_master_posirq );
	}
	cpu_set_irq_line( CPU_MASTER, namcos2_68k_master_C148[NAMCOS2_C148_VBLANKIRQ], HOLD_LINE);
}

WRITE16_HANDLER( namcos2_68k_slave_C148_w ){
	WriteC148( CPU_SLAVE, offset, data );
}

READ16_HANDLER( namcos2_68k_slave_C148_r )
{
	return ReadC148( CPU_SLAVE, offset );
}

void namcos2_68k_slave_posirq( int scanline )
{
	force_partial_update(scanline);
	cpu_set_irq_line(CPU_SLAVE , namcos2_68k_slave_C148[NAMCOS2_C148_POSIRQ] , ASSERT_LINE);
}

INTERRUPT_GEN( namcos2_68k_slave_vblank ){
	if(namcos2_68k_slave_C148[NAMCOS2_C148_POSIRQ])
	{
		int scanline = GetPosIRQScanline();
		timer_set( cpu_getscanlinetime(scanline), scanline, namcos2_68k_slave_posirq );
	}
	cpu_set_irq_line( CPU_SLAVE, namcos2_68k_slave_C148[NAMCOS2_C148_VBLANKIRQ], HOLD_LINE);
}

/**************************************************************/
/*	Sound sub-system										  */
/**************************************************************/

WRITE_HANDLER( namcos2_sound_bankselect_w )
{
	unsigned char *RAM=memory_region(REGION_CPU3);
	unsigned long max = (memory_region_length(REGION_CPU3) - 0x10000) / 0x4000;
	int bank = ( data >> 4 ) % max;	/* 991104.CAB */
	cpu_setbank( CPU3_ROM1, &RAM[ 0x10000 + ( 0x4000 * bank ) ] );
}



/**************************************************************/
/*															  */
/*	68705 IO CPU Support functions							  */
/*															  */
/**************************************************************/

static int namcos2_mcu_analog_ctrl=0;
static int namcos2_mcu_analog_data=0xaa;
static int namcos2_mcu_analog_complete=0;

WRITE_HANDLER( namcos2_mcu_analog_ctrl_w )
{
	namcos2_mcu_analog_ctrl=data&0xff;

	/* Check if this is a start of conversion */
	/* Input ports 2 thru 9 are the analog channels */
	if(data&0x40)
	{
		/* Set the conversion complete flag */
		namcos2_mcu_analog_complete=2;
		/* We convert instantly, good eh! */
		switch((data>>2)&0x07)
		{
			case 0:
				namcos2_mcu_analog_data=input_port_2_r(0);
				break;
			case 1:
				namcos2_mcu_analog_data=input_port_3_r(0);
				break;
			case 2:
				namcos2_mcu_analog_data=input_port_4_r(0);
				break;
			case 3:
				namcos2_mcu_analog_data=input_port_5_r(0);
				break;
			case 4:
				namcos2_mcu_analog_data=input_port_6_r(0);
				break;
			case 5:
				namcos2_mcu_analog_data=input_port_7_r(0);
				break;
			case 6:
				namcos2_mcu_analog_data=input_port_8_r(0);
				break;
			case 7:
				namcos2_mcu_analog_data=input_port_9_r(0);
				break;
		}
#if 0
		/* Perform the offset handling on the input port */
		/* this converts it to a twos complement number */
		if( namcos2_gametype==NAMCOS2_DIRT_FOX ||
			namcos2_gametype==NAMCOS2_DIRT_FOX_JP )
		{
			namcos2_mcu_analog_data^=0x80;
		}
#endif
		/* If the interrupt enable bit is set trigger an A/D IRQ */
		if(data&0x20)
		{
			cpu_set_irq_line( CPU_MCU, HD63705_INT_ADCONV , PULSE_LINE);
		}
	}
}

READ_HANDLER( namcos2_mcu_analog_ctrl_r )
{
	int data=0;

	/* ADEF flag is only cleared AFTER a read from control THEN a read from DATA */
	if(namcos2_mcu_analog_complete==2) namcos2_mcu_analog_complete=1;
	if(namcos2_mcu_analog_complete) data|=0x80;

	/* Mask on the lower 6 register bits, Irq EN/Channel/Clock */
	data|=namcos2_mcu_analog_ctrl&0x3f;
	/* Return the value */
	return data;
}

WRITE_HANDLER( namcos2_mcu_analog_port_w )
{
}

READ_HANDLER( namcos2_mcu_analog_port_r )
{
	if(namcos2_mcu_analog_complete==1) namcos2_mcu_analog_complete=0;
	return namcos2_mcu_analog_data;
}

WRITE_HANDLER( namcos2_mcu_port_d_w )
{
	/* Undefined operation on write */
}

READ_HANDLER( namcos2_mcu_port_d_r )
{
	/* Provides a digital version of the analog ports */
	int threshold=0x7f;
	int data=0;

	/* Read/convert the bits one at a time */
	if(input_port_2_r(0)>threshold) data|=0x01;
	if(input_port_3_r(0)>threshold) data|=0x02;
	if(input_port_4_r(0)>threshold) data|=0x04;
	if(input_port_5_r(0)>threshold) data|=0x08;
	if(input_port_6_r(0)>threshold) data|=0x10;
	if(input_port_7_r(0)>threshold) data|=0x20;
	if(input_port_8_r(0)>threshold) data|=0x40;
	if(input_port_9_r(0)>threshold) data|=0x80;

	/* Return the result */
	return data;
}

READ_HANDLER( namcos2_input_port_0_r )
{
	int data=readinputport(0);

	int one_joy_trans0[2][10]={
        {0x05,0x01,0x09,0x08,0x0a,0x02,0x06,0x04,0x12,0x14},
        {0x00,0x20,0x20,0x20,0x08,0x08,0x00,0x08,0x02,0x02}};

	int datafake, i;

	switch(namcos2_gametype)
	{
		case NAMCOS2_ASSAULT:
		case NAMCOS2_ASSAULT_JP:
		case NAMCOS2_ASSAULT_PLUS:
			datafake=~readinputport(15) & 0xff;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "xxx=%08x\n",datafake);
			for (i=0;i<10;i++)
				if (datafake==one_joy_trans0[0][i])
				{
					data&=~one_joy_trans0[1][i];
					break;
				}
	}
	return data;
}

READ_HANDLER( namcos2_input_port_10_r )
{
	int data=readinputport(10);

	int one_joy_trans10[2][10]={
        {0x05,0x01,0x09,0x08,0x0a,0x02,0x06,0x04,0x1a,0x18},
        {0x08,0x08,0x00,0x02,0x00,0x02,0x02,0x08,0x80,0x80}};

	int datafake, i;

	switch(namcos2_gametype)
	{
		case NAMCOS2_ASSAULT:
		case NAMCOS2_ASSAULT_JP:
		case NAMCOS2_ASSAULT_PLUS:
			datafake=~readinputport(15) & 0xff;
			for (i=0;i<10;i++)
				if (datafake==one_joy_trans10[0][i])
				{
					data&=~one_joy_trans10[1][i];
					break;
				}
	}
	return data;
}

READ_HANDLER( namcos2_input_port_12_r )
{
	int data=readinputport(12);

	int one_joy_trans12[2][4]={
        {0x12,0x14,0x11,0x18},
        {0x02,0x08,0x08,0x02}};

	int datafake, i;

	switch(namcos2_gametype)
	{
		case NAMCOS2_ASSAULT:
		case NAMCOS2_ASSAULT_JP:
		case NAMCOS2_ASSAULT_PLUS:
			datafake=~readinputport(15) & 0xff;
			for (i=0;i<4;i++)
				if (datafake==one_joy_trans12[0][i])
				{
					data&=~one_joy_trans12[1][i];
					break;
				}
	}
	return data;
}
