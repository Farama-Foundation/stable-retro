#include "driver.h"
#include "state.h"
#include "tatsumi.h"

/*static*/ data16_t tatsumi_control_word=0;
static data16_t tatsumi_last_control=0;
static data16_t tatsumi_last_irq=0;
static data8_t apache3_adc;

data16_t *tatsumi_68k_ram;
data8_t *apache3_z80_ram;

/******************************************************************************/

void tatsumi_reset(void)
{
	tatsumi_last_irq=0;
	tatsumi_last_control=0;
	tatsumi_control_word=0;
	apache3_adc=0;

	state_save_register_UINT16("tatsumi", 0, "lastirq", &tatsumi_last_irq, 1);
	state_save_register_UINT16("tatsumi", 0, "lastcontrol", &tatsumi_last_control, 1);
	state_save_register_UINT16("tatsumi", 0, "control", &tatsumi_control_word, 1);
	state_save_register_UINT8("tatsumi", 0, "adc", &apache3_adc, 1);
}

/******************************************************************************/

READ_HANDLER( apache3_bank_r )
{
	if (offset)
		return tatsumi_control_word>>8;
	return tatsumi_control_word&0xff;
}

WRITE_HANDLER( apache3_bank_w )
{
	/*
		0x8000  - Set when accessing palette ram (not implemented, perhaps blank screen?)
		0x0080	- Set when accessing IO cpu RAM/ROM (implemented as halt cpu)
		0x0060	- IOP bank to access from main cpu (0x0 = RAM, 0x20 = lower ROM, 0x60 = upper ROM)
		0x0010	- Set when accessing OBJ cpu RAM/ROM (implemented as halt cpu)
		0x000f	- OBJ bank to access from main cpu (0x8 = RAM, 0x0 to 0x7 = ROM)
	*/

	if (offset==1) tatsumi_control_word=(tatsumi_control_word&0xff)|(data<<8);
	else tatsumi_control_word=(tatsumi_control_word&0xff00)|(data&0xff);

	if (tatsumi_control_word&0x7f00)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown control Word: %04x\n",tatsumi_control_word);
		cpu_set_halt_line(3, CLEAR_LINE); /* ? */
	}
	if ((tatsumi_control_word&0x8)==0 && !(tatsumi_last_control&0x8))
		cpu_set_irq_line(1, 4, ASSERT_LINE);

	if (tatsumi_control_word&0x10)
		cpu_set_halt_line(1, ASSERT_LINE);
	else
		cpu_set_halt_line(1, CLEAR_LINE);

	if (tatsumi_control_word&0x80)
		cpu_set_halt_line(2, ASSERT_LINE);
	else
		cpu_set_halt_line(2, CLEAR_LINE);

	tatsumi_last_control=tatsumi_control_word;
}

WRITE16_HANDLER( apache3_irq_ack_w )
{
	cpu_set_irq_line(1, 4, CLEAR_LINE);

	if ((data&2) && (tatsumi_last_irq&2)==0)
	{
		cpu_set_halt_line(3, ASSERT_LINE);
	}

	if ((tatsumi_last_irq&2) && (data&2)==0)
	{
		cpu_set_halt_line(3, CLEAR_LINE);
		cpu_set_irq_line_and_vector(3, 0, PULSE_LINE, 0xc7 | 0x10);
	}

	tatsumi_last_irq=data;
}

READ_HANDLER( apache3_v30_v20_r )
{
	const data8_t* rom=(data8_t*)memory_region(REGION_CPU3);

	/* Each V20 byte maps to a V30 word */
	if ((tatsumi_control_word&0xe0)==0xe0) 
		rom+=0xf8000; /* Upper half */
	else if ((tatsumi_control_word&0xe0)==0xc0) 
		rom+=0xf0000;
	else if ((tatsumi_control_word&0xe0)==0x80) 
		rom+=0x00000; /* main ram */
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: unmapped read z80 rom %08x\n",activecpu_get_pc(),offset);

	return rom[offset/2];
}

WRITE_HANDLER( apache3_v30_v20_w )
{
	data8_t* ram=(data8_t*)memory_region(REGION_CPU3);
	
	if ((tatsumi_control_word&0xe0)!=0x80)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: write unmapped v30 rom %08x\n",activecpu_get_pc(),offset);

	/* Only 8 bits of the V30 data bus are connected - ignore writes to the other half */
	if (offset&1)
		return;

	ram[offset/2]=data&0xff;
}

READ16_HANDLER(apache3_z80_r)
{
	return apache3_z80_ram[offset];
}

WRITE16_HANDLER(apache3_z80_w)
{
	apache3_z80_ram[offset]=data&0xff;
}

READ_HANDLER( apache3_adc_r )
{
	if (apache3_adc==0)
		return readinputport(1);
	return readinputport(2);
}

WRITE_HANDLER( apache3_adc_w )
{
	apache3_adc=offset;
}

data16_t apache3_a0000[16]; /* TODO */
int a3counter=0; /* TODO */

WRITE16_HANDLER( apache3_a0000_w )
{
	if (a3counter>15) /* TODO */
		return;
	apache3_a0000[a3counter++]=data;
}

/******************************************************************************/

READ_HANDLER( roundup_v30_z80_r )
{
	const data8_t* rom=(data8_t*)memory_region(REGION_CPU3);

	/* Each Z80 byte maps to a V30 word */
	if (tatsumi_control_word&0x20) 
		rom+=0x8000; /* Upper half */

	return rom[offset/2];
}

WRITE_HANDLER( roundup_v30_z80_w )
{
	data8_t* ram=(data8_t*)memory_region(REGION_CPU3);
	
	/* Only 8 bits of the V30 data bus are connected - ignore writes to the other half */
	if (offset&1)
		return;

	if (tatsumi_control_word&0x20) 
		offset+=0x10000; /* Upper half of Z80 address space */

	ram[offset/2]=data&0xff;
}


WRITE_HANDLER( roundup5_control_w )
{
	if (offset==1) tatsumi_control_word=(tatsumi_control_word&0xff)|(data<<8);
	else tatsumi_control_word=(tatsumi_control_word&0xff00)|(data&0xff);
		
	if (tatsumi_control_word&0x10)
		cpu_set_halt_line(1, ASSERT_LINE);
	else
		cpu_set_halt_line(1, CLEAR_LINE);

	if (tatsumi_control_word&0x4)
		cpu_set_halt_line(2, ASSERT_LINE);
	else
		cpu_set_halt_line(2, CLEAR_LINE);

/*	if (offset==1 && (tatsumi_control_w&0xfeff)!=(last_bank&0xfeff)) */
/*		logerror("%08x:  Changed bank to %04x (%d)\n",activecpu_get_pc(),tatsumi_control_w,offset); */

/*todo - watchdog */

	/* Bank:

		0x0017	:	OBJ banks
		0x0018	:	68000 RAM		mask 0x0380 used to save bits when writing
		0x0c10	:	68000 ROM

		0x0040	:	Z80 rom (lower half) mapped to 0x10000
		0x0060	:	Z80 rom (upper half) mapped to 0x10000

		0x0100  :	watchdog.

		0x0c00	:	vram bank (bits 0x7000 also set when writing vram)

		0x8000	:	set whenever writing to palette ram?

		Changed bank to 0060 (0)
	*/

	if ((tatsumi_control_word&0x8)==0 && !(tatsumi_last_control&0x8))
		cpu_set_irq_line(1, 4, ASSERT_LINE);
/*	if (tatsumi_control_w&0x200) */
/*		cpu_set_reset_line(1, CLEAR_LINE); */
/*	else */
/*		cpu_set_reset_line(1, ASSERT_LINE); */

/*	if ((tatsumi_control_w&0x200) && (last_bank&0x200)==0) */
/*		logerror("68k irq\n"); */
/*	if ((tatsumi_control_w&0x200)==0 && (last_bank&0x200)==0x200) */
/*		logerror("68k reset\n"); */

	if (tatsumi_control_word==0x3a00) {
/*		cpu_set_reset_line(1, CLEAR_LINE); */
	/*	logerror("68k on\n"); */

	}

	tatsumi_last_control=tatsumi_control_word;
}

WRITE16_HANDLER( roundup5_d0000_w )
{
	COMBINE_DATA(&roundup5_d0000_ram[offset]);
/*	logerror("d_68k_d0000_w %06x %04x\n",activecpu_get_pc(),data); */
}

WRITE16_HANDLER( roundup5_e0000_w )
{
	/*	Bit 0x10 is road bank select,
		Bit 0x100 is used, but unknown 
	*/

	COMBINE_DATA(&roundup5_e0000_ram[offset]);
	cpu_set_irq_line(1, 4, CLEAR_LINE); /* guess, probably wrong */

/*	logerror("d_68k_e0000_w %06x %04x\n",activecpu_get_pc(),data); */
}

/******************************************************************************/

READ16_HANDLER(cyclwarr_control_r)
{
/*	logerror("%08x:  control_r\n",activecpu_get_pc()); */
	return tatsumi_control_word;
}

WRITE16_HANDLER(cyclwarr_control_w)
{
	COMBINE_DATA(&tatsumi_control_word);

/*	if ((tatsumi_control_word&0xfe)!=(tatsumi_last_control&0xfe)) */
/*		logerror("%08x:  control_w %04x\n",activecpu_get_pc(),data); */

/*

0x1 - watchdog
0x4 - cpu bus lock



*/

	if ((tatsumi_control_word&4)==4 && (tatsumi_last_control&4)==0) {
/*		logerror("68k 2 halt\n"); */
		cpu_set_halt_line(1, ASSERT_LINE);
	} 

	if ((tatsumi_control_word&4)==0 && (tatsumi_last_control&4)==4) {
/*		logerror("68k 2 irq go\n"); */
		cpu_set_halt_line(1, CLEAR_LINE);
	}


	/* hack */
	if (activecpu_get_pc()==0x2c3c34) {
/*		cpu_set_reset_line(1, CLEAR_LINE); */
	/*	logerror("hack 68k2 on\n"); */
	}

	tatsumi_last_control=tatsumi_control_word;
}

/******************************************************************************/

READ_HANDLER( tatsumi_v30_68000_r )
{
	const data16_t* rom=(data16_t*)memory_region(REGION_CPU2);

	/* Read from 68k RAM */
	if ((tatsumi_control_word&0x1f)==0x18) {

		/* hack to make roundup 5 boot */
		if (activecpu_get_pc()==0xec575) {
			UINT8 *dst = memory_region(REGION_CPU1);
			dst[0xec57a]=0x46;
			dst[0xec57b]=0x46;

			dst[0xfc520]=0x46; /*code that stops cpu after coin counter goes mad.. */
			dst[0xfc521]=0x46;
			dst[0xfc522]=0x46;
			dst[0xfc523]=0x46;
			dst[0xfc524]=0x46;
			dst[0xfc525]=0x46;
		}

		if ((offset&1)==0) return tatsumi_68k_ram[offset/2]&0xff;
		return (tatsumi_68k_ram[offset/2]>>8)&0xff;	
	}

	/* Read from 68k ROM */
	offset+=(tatsumi_control_word&0x7)*0x10000;

	if ((offset&1)==0) return rom[offset/2]&0xff;
	return (rom[offset/2]>>8)&0xff;
}

WRITE_HANDLER( tatsumi_v30_68000_w ) 
{ 
	data16_t d=tatsumi_68k_ram[offset/2];
	if ((offset&1)==0) d=(d&0xff00)|data;
	else d=(d&0xff)|(data<<8);
	
	if ((tatsumi_control_word&0x1f)!=0x18)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "68k write in bank %05x\n",tatsumi_control_word);

	tatsumi_68k_ram[offset/2]=d;
}

/***********************************************************************************/

/* Todo:  Current YM2151 doesn't seem to raise the busy flag quickly enough for the */
/* self-test in Tatsumi games.  Needs fixed, but hack it here for now. */
READ_HANDLER(tatsumi_hack_ym2151_r)
{
	int r=YM2151_status_port_0_r(0);

	if (activecpu_get_pc()==0x2aca || activecpu_get_pc()==0x29fe
		|| activecpu_get_pc()==0xf9721
		|| activecpu_get_pc()==0x1b96 || activecpu_get_pc()==0x1c65) /* BigFight */
		return 0x80;
	return r;
}

/* Todo:  Tatsumi self-test fails if OKI doesn't respond (when sound off). */
/* Mame really should emulate the OKI status reads even with Mame sound off. */
READ_HANDLER(tatsumi_hack_oki_r)
{
	int r=OKIM6295_status_0_r(0);

    /* Cycle Warriors and Big Fight access this with reversed activeness. */
	/* this is particularly noticeable with the "We got em" sample played in CW at stage clear: */
    /* gets cut too early with the old hack below. */
	/* fwiw returning normal oki status doesn't work at all, both games don't make any sound. */
	/* TODO: verify with HW */
	return (r ^ 0xff);


	/*if (activecpu_get_pc()==0x2b70 || activecpu_get_pc()==0x2bb5  */
	/*	|| activecpu_get_pc()==0x2acc */
	/*    || activecpu_get_pc()==0x1c79 */ /* BigFight */
	/*	|| activecpu_get_pc()==0x1cbe*/ /* BigFight */
	/*	|| activecpu_get_pc()==0xf9881) */
	/*	return 0xf; */
	/*if (activecpu_get_pc()==0x2ba3 || activecpu_get_pc()==0x2a9b || activecpu_get_pc()==0x2adc */
	/*	|| activecpu_get_pc()==0x1cac) // BigFight */
	/*	return 0; */
	/*return r; */
}
