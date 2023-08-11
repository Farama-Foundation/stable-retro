// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

        ADSP2100.c
        Core implementation for the portable Analog ADSP-2100 emulator.
        Written by Aaron Giles

****************************************************************************

    For ADSP-2101, ADSP-2111
    ------------------------

        MMAP = 0                                        MMAP = 1

        Automatic boot loading                          No auto boot loading

        Program Space:                                  Program Space:
            0000-07ff = 2k Internal RAM (booted)            0000-37ff = 14k External access
            0800-3fff = 14k External access                 3800-3fff = 2k Internal RAM

        Data Space:                                     Data Space:
            0000-03ff = 1k External DWAIT0                  0000-03ff = 1k External DWAIT0
            0400-07ff = 1k External DWAIT1                  0400-07ff = 1k External DWAIT1
            0800-2fff = 10k External DWAIT2                 0800-2fff = 10k External DWAIT2
            3000-33ff = 1k External DWAIT3                  3000-33ff = 1k External DWAIT3
            3400-37ff = 1k External DWAIT4                  3400-37ff = 1k External DWAIT4
            3800-3bff = 1k Internal RAM                     3800-3bff = 1k Internal RAM
            3c00-3fff = 1k Internal Control regs            3c00-3fff = 1k Internal Control regs


    For ADSP-2105, ADSP-2115
    ------------------------

        MMAP = 0                                        MMAP = 1

        Automatic boot loading                          No auto boot loading

        Program Space:                                  Program Space:
            0000-03ff = 1k Internal RAM (booted)            0000-37ff = 14k External access
            0400-07ff = 1k Reserved                         3800-3bff = 1k Internal RAM
            0800-3fff = 14k External access                 3c00-3fff = 1k Reserved

        Data Space:                                     Data Space:
            0000-03ff = 1k External DWAIT0                  0000-03ff = 1k External DWAIT0
            0400-07ff = 1k External DWAIT1                  0400-07ff = 1k External DWAIT1
            0800-2fff = 10k External DWAIT2                 0800-2fff = 10k External DWAIT2
            3000-33ff = 1k External DWAIT3                  3000-33ff = 1k External DWAIT3
            3400-37ff = 1k External DWAIT4                  3400-37ff = 1k External DWAIT4
            3800-39ff = 512 Internal RAM                    3800-39ff = 512 Internal RAM
            3a00-3bff = 512 Reserved                        3a00-3bff = 512 Reserved
            3c00-3fff = 1k Internal Control regs            3c00-3fff = 1k Internal Control regs


    For ADSP-2104
    -------------

        MMAP = 0                                        MMAP = 1

        Automatic boot loading                          No auto boot loading

        Program Space:                                  Program Space:
            0000-01ff = 512 Internal RAM (booted)           0000-37ff = 14k External access
            0400-07ff = 1k Reserved                         3800-3bff = 1k Internal RAM
            0800-3fff = 14k External access                 3c00-3fff = 1k Reserved

        Data Space:                                     Data Space:
            0000-03ff = 1k External DWAIT0                  0000-03ff = 1k External DWAIT0
            0400-07ff = 1k External DWAIT1                  0400-07ff = 1k External DWAIT1
            0800-2fff = 10k External DWAIT2                 0800-2fff = 10k External DWAIT2
            3000-33ff = 1k External DWAIT3                  3000-33ff = 1k External DWAIT3
            3400-37ff = 1k External DWAIT4                  3400-37ff = 1k External DWAIT4
            3800-38ff = 256 Internal RAM                    3800-38ff = 256 Internal RAM
            3a00-3bff = 512 Reserved                        3a00-3bff = 512 Reserved
            3c00-3fff = 1k Internal Control regs            3c00-3fff = 1k Internal Control regs


    For ADSP-2181
    -------------

        MMAP = 0                                        MMAP = 1

        Program Space:                                  Program Space:
            0000-1fff = 8k Internal RAM                     0000-1fff = 8k External access
            2000-3fff = 8k Internal RAM or Overlay          2000-3fff = 8k Internal

        Data Space:                                     Data Space:
            0000-1fff = 8k Internal RAM or Overlay          0000-1fff = 8k Internal RAM or Overlay
            2000-3fdf = 8k-32 Internal RAM                  2000-3fdf = 8k-32 Internal RAM
            3fe0-3fff = 32 Internal Control regs            3fe0-3fff = 32 Internal Control regs

        I/O Space:                                      I/O Space:
            0000-01ff = 512 External IOWAIT0                0000-01ff = 512 External IOWAIT0
            0200-03ff = 512 External IOWAIT1                0200-03ff = 512 External IOWAIT1
            0400-05ff = 512 External IOWAIT2                0400-05ff = 512 External IOWAIT2
            0600-07ff = 512 External IOWAIT3                0600-07ff = 512 External IOWAIT3

***************************************************************************/

#include "adsp2100.h"
#include <stddef.h>
#include <stdlib.h>

/***************************************************************************
    PRIVATE GLOBAL VARIABLES
***************************************************************************/

static UINT16 *reverse_table = 0;
static UINT16 *mask_table = 0;
static UINT8 *condition_table = 0;

#if TRACK_HOTSPOTS
static UINT32 pcbucket[0x4000];
#endif



#if 0
/***************************************************************************
    CPU STATE DESCRIPTION
***************************************************************************/

#define ADSP21XX_STATE_ENTRY(_name, _format, _member, _datamask, _flags) \
	CPU_STATE_ENTRY(ADSP2100_##_name, #_name, _format, adsp2100_state, _member, _datamask, ~0, _flags)

#define ADSP21XX_STATE_ENTRY_MASK(_name, _format, _member, _datamask, _flags, _validmask) \
	CPU_STATE_ENTRY(ADSP2100_##_name, #_name, _format, adsp2100_state, _member, _datamask, _validmask, _flags)

static const cpu_state_entry state_array[] =
{
	ADSP21XX_STATE_ENTRY(PC,  "%04X", pc, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(GENPC, "%04X", pc, 0xffff, CPUSTATE_NOSHOW)
	ADSP21XX_STATE_ENTRY(GENPCBASE, "%04X", ppc, 0xffff, CPUSTATE_NOSHOW)

	ADSP21XX_STATE_ENTRY(AX0, "%04X", core.ax0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AX1, "%04X", core.ax1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AY0, "%04X", core.ay0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AY1, "%04X", core.ay1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AR,  "%04X", core.ar.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AF,  "%04X", core.af.u, 0xffff, 0)

	ADSP21XX_STATE_ENTRY(MX0, "%04X", core.mx0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MX1, "%04X", core.mx1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MY0, "%04X", core.my0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MY1, "%04X", core.my1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MR0, "%04X", core.mr.mrx.mr0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MR1, "%04X", core.mr.mrx.mr1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MR2, "%02X", core.mr.mrx.mr2.u, 0xff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(MF,  "%04X", core.mf.u, 0xffff, 0)

	ADSP21XX_STATE_ENTRY(SI,  "%04X", core.si.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(SE,  "%02X", core.se.u, 0xff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(SB,  "%02X", core.sb.u, 0x1f, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(SR0, "%04X", core.sr.srx.sr0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(SR1, "%04X", core.sr.srx.sr0.u, 0xffff, 0)

	ADSP21XX_STATE_ENTRY(AX0_SEC, "%04X", alt.ax0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AX1_SEC, "%04X", alt.ax1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AY0_SEC, "%04X", alt.ay0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AY1_SEC, "%04X", alt.ay1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AR_SEC,  "%04X", alt.ar.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(AF_SEC,  "%04X", alt.af.u, 0xffff, 0)

	ADSP21XX_STATE_ENTRY(MX0_SEC, "%04X", alt.mx0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MX1_SEC, "%04X", alt.mx1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MY0_SEC, "%04X", alt.my0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MY1_SEC, "%04X", alt.my1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MR0_SEC, "%04X", alt.mr.mrx.mr0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MR1_SEC, "%04X", alt.mr.mrx.mr1.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(MR2_SEC, "%02X", alt.mr.mrx.mr2.u, 0xff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(MF_SEC,  "%04X", alt.mf.u, 0xffff, 0)

	ADSP21XX_STATE_ENTRY(SI_SEC,  "%04X", alt.si.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(SE_SEC,  "%02X", alt.se.u, 0xff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(SB_SEC,  "%02X", alt.sb.u, 0x1f, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(SR0_SEC, "%04X", alt.sr.srx.sr0.u, 0xffff, 0)
	ADSP21XX_STATE_ENTRY(SR1_SEC, "%04X", alt.sr.srx.sr0.u, 0xffff, 0)

	ADSP21XX_STATE_ENTRY(I0, "%04X", i[0], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(I1, "%04X", i[1], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(I2, "%04X", i[2], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(I3, "%04X", i[3], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(I4, "%04X", i[4], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(I5, "%04X", i[5], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(I6, "%04X", i[6], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(I7, "%04X", i[7], 0x3fff, CPUSTATE_IMPORT)

	ADSP21XX_STATE_ENTRY(L0, "%04X", l[0], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(L1, "%04X", l[1], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(L2, "%04X", l[2], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(L3, "%04X", l[3], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(L4, "%04X", l[4], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(L5, "%04X", l[5], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(L6, "%04X", l[6], 0x3fff, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(L7, "%04X", l[7], 0x3fff, CPUSTATE_IMPORT)

	ADSP21XX_STATE_ENTRY(M0, "%04X", m[0], 0x3fff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(M1, "%04X", m[1], 0x3fff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(M2, "%04X", m[2], 0x3fff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(M3, "%04X", m[3], 0x3fff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(M4, "%04X", m[4], 0x3fff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(M5, "%04X", m[5], 0x3fff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(M6, "%04X", m[6], 0x3fff, CPUSTATE_IMPORT_SEXT)
	ADSP21XX_STATE_ENTRY(M7, "%04X", m[7], 0x3fff, CPUSTATE_IMPORT_SEXT)

	ADSP21XX_STATE_ENTRY(PX, "%02X", px, 0xff, 0)
	ADSP21XX_STATE_ENTRY(CNTR, "%04X", cntr, 0x3fff, 0)
	ADSP21XX_STATE_ENTRY(ASTAT, "%02X", astat, 0xff, 0)
	ADSP21XX_STATE_ENTRY(SSTAT, "%02X", sstat, 0xff, 0)
	ADSP21XX_STATE_ENTRY_MASK(MSTAT, "%01X", mstat, 0x0f, CPUSTATE_IMPORT,  (1 << CHIP_TYPE_ADSP2100))
	ADSP21XX_STATE_ENTRY_MASK(MSTAT, "%02X", mstat, 0x7f, CPUSTATE_IMPORT, ~(1 << CHIP_TYPE_ADSP2100))

	ADSP21XX_STATE_ENTRY(PCSP,   "%02X", pc_sp, 0xff, 0)
	ADSP21XX_STATE_ENTRY(GENSP,  "%02X", pc_sp, 0xff, CPUSTATE_NOSHOW)
	ADSP21XX_STATE_ENTRY(CNTRSP, "%01X", cntr_sp, 0xf, 0)
	ADSP21XX_STATE_ENTRY(STATSP, "%01X", stat_sp, 0xf, 0)
	ADSP21XX_STATE_ENTRY(LOOPSP, "%01X", loop_sp, 0xf, 0)

	ADSP21XX_STATE_ENTRY_MASK(IMASK, "%01X", imask, 0x00f, CPUSTATE_IMPORT,  (1 << CHIP_TYPE_ADSP2100))
	ADSP21XX_STATE_ENTRY_MASK(IMASK, "%02X", imask, 0x03f, CPUSTATE_IMPORT, ~((1 << CHIP_TYPE_ADSP2100) | (1 << CHIP_TYPE_ADSP2181)))
	ADSP21XX_STATE_ENTRY_MASK(IMASK, "%03X", imask, 0x3ff, CPUSTATE_IMPORT,  (1 << CHIP_TYPE_ADSP2181))
	ADSP21XX_STATE_ENTRY(ICNTL, "%02X", icntl, 0x1f, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(IRQSTATE0, "%1u", irq_state[0], 0x1, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(IRQSTATE1, "%1u", irq_state[1], 0x1, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY(IRQSTATE2, "%1u", irq_state[2], 0x1, CPUSTATE_IMPORT)
	ADSP21XX_STATE_ENTRY_MASK(IRQSTATE3, "%1u", irq_state[3], 0x1, CPUSTATE_IMPORT, (1 << CHIP_TYPE_ADSP2100))

	ADSP21XX_STATE_ENTRY(FLAGIN, "%1u", flagin, 0x1, 0)
	ADSP21XX_STATE_ENTRY(FLAGOUT, "%1u", flagout, 0x1, 0)
	ADSP21XX_STATE_ENTRY(FL0, "%1u", fl0, 0x1, 0)
	ADSP21XX_STATE_ENTRY(FL1, "%1u", fl1, 0x1, 0)
	ADSP21XX_STATE_ENTRY(FL2, "%1u", fl2, 0x1, 0)
};
static const cpu_state_table state_table_template =
{
	NULL,						/* pointer to the base of state (offsets are relative to this) */
	0,							/* subtype this table refers to */
	ARRAY_LENGTH(state_array),	/* number of entries */
	state_array					/* array of entries */
};

#endif


/***************************************************************************
    PRIVATE FUNCTION PROTOTYPES
***************************************************************************/

static int create_tables(void);
static void check_irqs(adsp2100_state *adsp);



/***************************************************************************
    STATE ACCESSORS
***************************************************************************/

#if 0
INLINE adsp2100_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == CPU);
	assert(cpu_get_type(device) == CPU_ADSP2100 ||
		   cpu_get_type(device) == CPU_ADSP2101 ||
		   cpu_get_type(device) == CPU_ADSP2104 ||
		   cpu_get_type(device) == CPU_ADSP2105 ||
		   cpu_get_type(device) == CPU_ADSP2115 ||
		   cpu_get_type(device) == CPU_ADSP2181);
	return (adsp2100_state *)device->token;   
}
#endif


/***************************************************************************
    MEMORY ACCESSORS
***************************************************************************/
INLINE UINT16 RWORD_DATA(adsp2100_state *adsp, UINT32 addr)
{
    //return memory_read_word_16le(adsp->data, addr << 1);
    return adsp21xx_data_read_word_16le(addr << 1);
}

INLINE void WWORD_DATA(adsp2100_state *adsp, UINT32 addr, UINT16 data)
{
    //memory_write_word_16le(adsp->data, addr << 1, data);
    adsp21xx_data_write_word_16le(addr << 1, data);
}

INLINE UINT16 RWORD_IO(adsp2100_state *adsp, UINT32 addr)
{
    //return memory_read_word_16le(adsp->io, addr << 1);
    return adsp21xx_data_read_word_16le(addr << 1);
}

INLINE void WWORD_IO(adsp2100_state *adsp, UINT32 addr, UINT16 data)
{
    //memory_write_word_16le(adsp->io, addr << 1, data);
    adsp21xx_data_write_word_16le(addr << 1, data);
}

INLINE UINT32 RWORD_PGM(adsp2100_state *adsp, UINT32 addr)
{
    //return memory_read_dword_32le(adsp->program, addr << 2);
    return adsp21xx_read_dword_32le(addr << 2);
}

INLINE void WWORD_PGM(adsp2100_state *adsp, UINT32 addr, UINT32 data)
{
    //memory_write_dword_32le(adsp->program, addr << 2, data & 0xffffff);
    adsp21xx_write_dword_32le(addr << 2, data & 0xffffff);
}

//#define ROPCODE(a) memory_decrypted_read_dword((a)->program, (a)->pc << 2)
#define ROPCODE(a)  adsp21xx_read_dword_32le((a)->pc << 2)


/***************************************************************************
    IMPORT CORE UTILITIES
***************************************************************************/

#include "2100ops.c"



/***************************************************************************
    IRQ HANDLING
***************************************************************************/

INLINE int adsp2100_generate_irq(adsp2100_state *adsp, int which)
{
	/* skip if masked */
	if (!(adsp->imask & (1 << which)))
		return 0;

	/* clear the latch */
	adsp->irq_latch[which] = 0;

	/* push the PC and the status */
	pc_stack_push(adsp);
	stat_stack_push(adsp);

	/* vector to location & stop idling */
	adsp->pc = which;
	adsp->idle = 0;

	/* mask other interrupts based on the nesting bit */
	if (adsp->icntl & 0x10) adsp->imask &= ~((2 << which) - 1);
	else adsp->imask &= ~0xf;

	return 1;
}


INLINE int adsp2101_generate_irq(adsp2100_state *adsp, int which, int indx)
{
	/* skip if masked */
	if (!(adsp->imask & (0x20 >> indx)))
		return 0;

	/* clear the latch */
	adsp->irq_latch[which] = 0;

	/* push the PC and the status */
	pc_stack_push(adsp);
	stat_stack_push(adsp);

	/* vector to location & stop idling */
	adsp->pc = 0x04 + indx * 4;
	adsp->idle = 0;

	/* mask other interrupts based on the nesting bit */
	if (adsp->icntl & 0x10) adsp->imask &= ~(0x3f >> indx);
	else adsp->imask &= ~0x3f;

	return 1;
}


INLINE int adsp2181_generate_irq(adsp2100_state *adsp, int which, int indx)
{
	/* skip if masked */
	if (!(adsp->imask & (0x200 >> indx)))
		return 0;

	/* clear the latch */
	adsp->irq_latch[which] = 0;

	/* push the PC and the status */
	pc_stack_push(adsp);
	stat_stack_push(adsp);

	/* vector to location & stop idling */
	adsp->pc = 0x04 + indx * 4;
	adsp->idle = 0;

	/* mask other interrupts based on the nesting bit */
	if (adsp->icntl & 0x10) adsp->imask &= ~(0x3ff >> indx);
	else adsp->imask &= ~0x3ff;

	return 1;
}


static void check_irqs(adsp2100_state *adsp)
{
	UINT8 check;

	if (adsp->chip_type >= CHIP_TYPE_ADSP2181)
	{
		/* check IRQ2 */
		check = (adsp->icntl & 4) ? adsp->irq_latch[ADSP2181_IRQ2] : adsp->irq_state[ADSP2181_IRQ2];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_IRQ2, 0))
			return;

		/* check IRQL1 */
		check = adsp->irq_state[ADSP2181_IRQL1];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_IRQL1, 1))
			return;

		/* check IRQL2 */
		check = adsp->irq_state[ADSP2181_IRQL2];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_IRQL2, 2))
			return;

		/* check SPORT0 transmit */
		check = adsp->irq_latch[ADSP2181_SPORT0_TX];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_SPORT0_TX, 3))
			return;

		/* check SPORT0 receive */
		check = adsp->irq_latch[ADSP2181_SPORT0_RX];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_SPORT0_RX, 4))
			return;

		/* check IRQE */
		check = adsp->irq_latch[ADSP2181_IRQE];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_IRQE, 5))
			return;

		/* check BDMA interrupt */

		/* check IRQ1/SPORT1 transmit */
		check = (adsp->icntl & 2) ? adsp->irq_latch[ADSP2181_IRQ1] : adsp->irq_state[ADSP2181_IRQ1];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_IRQ1, 7))
			return;

		/* check IRQ0/SPORT1 receive */
		check = (adsp->icntl & 1) ? adsp->irq_latch[ADSP2181_IRQ0] : adsp->irq_state[ADSP2181_IRQ0];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_IRQ0, 8))
			return;

		/* check timer */
		check = adsp->irq_latch[ADSP2181_TIMER];
		if (check && adsp2181_generate_irq(adsp, ADSP2181_TIMER, 9))
			return;
	}
	else if (adsp->chip_type >= CHIP_TYPE_ADSP2101)
	{
		/* check IRQ2 */
		check = (adsp->icntl & 4) ? adsp->irq_latch[ADSP2101_IRQ2] : adsp->irq_state[ADSP2101_IRQ2];
		if (check && adsp2101_generate_irq(adsp, ADSP2101_IRQ2, 0))
			return;

		/* check SPORT0 transmit */
		check = adsp->irq_latch[ADSP2101_SPORT0_TX];
		if (check && adsp2101_generate_irq(adsp, ADSP2101_SPORT0_TX, 1))
			return;

		/* check SPORT0 receive */
		check = adsp->irq_latch[ADSP2101_SPORT0_RX];
		if (check && adsp2101_generate_irq(adsp, ADSP2101_SPORT0_RX, 2))
			return;

		/* check IRQ1/SPORT1 transmit */
		check = (adsp->icntl & 2) ? adsp->irq_latch[ADSP2101_IRQ1] : adsp->irq_state[ADSP2101_IRQ1];
		if (check && adsp2101_generate_irq(adsp, ADSP2101_IRQ1, 3))
			return;

		/* check IRQ0/SPORT1 receive */
		check = (adsp->icntl & 1) ? adsp->irq_latch[ADSP2101_IRQ0] : adsp->irq_state[ADSP2101_IRQ0];
		if (check && adsp2101_generate_irq(adsp, ADSP2101_IRQ0, 4))
			return;

		/* check timer */
		check = adsp->irq_latch[ADSP2101_TIMER];
		if (check && adsp2101_generate_irq(adsp, ADSP2101_TIMER, 5))
			return;
	}
	else
	{
		/* check IRQ3 */
		check = (adsp->icntl & 8) ? adsp->irq_latch[ADSP2100_IRQ3] : adsp->irq_state[ADSP2100_IRQ3];
		if (check && adsp2100_generate_irq(adsp, ADSP2100_IRQ3))
			return;

		/* check IRQ2 */
		check = (adsp->icntl & 4) ? adsp->irq_latch[ADSP2100_IRQ2] : adsp->irq_state[ADSP2100_IRQ2];
		if (check && adsp2100_generate_irq(adsp, ADSP2100_IRQ2))
			return;

		/* check IRQ1 */
		check = (adsp->icntl & 2) ? adsp->irq_latch[ADSP2100_IRQ1] : adsp->irq_state[ADSP2100_IRQ1];
		if (check && adsp2100_generate_irq(adsp, ADSP2100_IRQ1))
			return;

		/* check IRQ0 */
		check = (adsp->icntl & 1) ? adsp->irq_latch[ADSP2100_IRQ0] : adsp->irq_state[ADSP2100_IRQ0];
		if (check && adsp2100_generate_irq(adsp, ADSP2100_IRQ0))
			return;
	}
}


static void set_irq_line(adsp2100_state *adsp, int irqline, int state)
{
	/* update the latched state */
	if (state != CLEAR_LINE && adsp->irq_state[irqline] == CLEAR_LINE)
    	adsp->irq_latch[irqline] = 1;

    /* update the absolute state */
    adsp->irq_state[irqline] = state;
}

void adsp21xx_set_irq_line(adsp2100_state *adsp, int irqline, int state)
{
    set_irq_line(adsp, irqline, state);
}
/***************************************************************************
    INITIALIZATION AND SHUTDOWN
***************************************************************************/

static adsp2100_state *adsp21xx_init(adsp2100_state *adsp, cpu_irq_callback irqcallback, int chiptype)
{
//	const adsp21xx_config *config = (const adsp21xx_config *)device->static_config;
//	adsp2100_state *adsp = get_safe_token(device);

	/* create the tables */
	if (!create_tables())
		fatalerror("creating adsp2100 tables failed");

	/* set the IRQ callback */
	adsp->chip_type = chiptype;
	adsp->irq_callback = irqcallback;

	/* fetch device parameters */
//	adsp->device = device;
//	adsp->program = memory_find_address_space(device, ADDRESS_SPACE_PROGRAM);
//	adsp->data = memory_find_address_space(device, ADDRESS_SPACE_DATA);
//	adsp->io = memory_find_address_space(device, ADDRESS_SPACE_IO);

	/* copy function pointers from the config */
//	if (config != NULL)
//	{
//		adsp->sport_rx_callback = config->rx;
//		adsp->sport_tx_callback = config->tx;
//		adsp->timer_fired = config->timer;
//	}

	/* set up the state table */
//	adsp->state = state_table_template;
//	adsp->state.baseptr = adsp;
//	adsp->state.subtypemask = 1 << chiptype;

	/* set up ALU register pointers */
	adsp->alu_xregs[0] = &adsp->core.ax0;
	adsp->alu_xregs[1] = &adsp->core.ax1;
	adsp->alu_xregs[2] = &adsp->core.ar;
	adsp->alu_xregs[3] = &adsp->core.mr.mrx.mr0;
	adsp->alu_xregs[4] = &adsp->core.mr.mrx.mr1;
	adsp->alu_xregs[5] = &adsp->core.mr.mrx.mr2;
	adsp->alu_xregs[6] = &adsp->core.sr.srx.sr0;
	adsp->alu_xregs[7] = &adsp->core.sr.srx.sr1;
	adsp->alu_yregs[0] = &adsp->core.ay0;
	adsp->alu_yregs[1] = &adsp->core.ay1;
	adsp->alu_yregs[2] = &adsp->core.af;
	adsp->alu_yregs[3] = &adsp->core.zero;

	/* set up MAC register pointers */
	adsp->mac_xregs[0] = &adsp->core.mx0;
	adsp->mac_xregs[1] = &adsp->core.mx1;
	adsp->mac_xregs[2] = &adsp->core.ar;
	adsp->mac_xregs[3] = &adsp->core.mr.mrx.mr0;
	adsp->mac_xregs[4] = &adsp->core.mr.mrx.mr1;
	adsp->mac_xregs[5] = &adsp->core.mr.mrx.mr2;
	adsp->mac_xregs[6] = &adsp->core.sr.srx.sr0;
	adsp->mac_xregs[7] = &adsp->core.sr.srx.sr1;
	adsp->mac_yregs[0] = &adsp->core.my0;
	adsp->mac_yregs[1] = &adsp->core.my1;
	adsp->mac_yregs[2] = &adsp->core.mf;
	adsp->mac_yregs[3] = &adsp->core.zero;

	/* set up shift register pointers */
	adsp->shift_xregs[0] = &adsp->core.si;
	adsp->shift_xregs[1] = &adsp->core.si;
	adsp->shift_xregs[2] = &adsp->core.ar;
	adsp->shift_xregs[3] = &adsp->core.mr.mrx.mr0;
	adsp->shift_xregs[4] = &adsp->core.mr.mrx.mr1;
	adsp->shift_xregs[5] = &adsp->core.mr.mrx.mr2;
	adsp->shift_xregs[6] = &adsp->core.sr.srx.sr0;
	adsp->shift_xregs[7] = &adsp->core.sr.srx.sr1;

#if 0
	/* "core" */
	state_save_register_device_item(device, 0, adsp->core.ax0.u);
	state_save_register_device_item(device, 0, adsp->core.ax1.u);
	state_save_register_device_item(device, 0, adsp->core.ay0.u);
	state_save_register_device_item(device, 0, adsp->core.ay1.u);
	state_save_register_device_item(device, 0, adsp->core.ar.u);
	state_save_register_device_item(device, 0, adsp->core.af.u);
	state_save_register_device_item(device, 0, adsp->core.mx0.u);
	state_save_register_device_item(device, 0, adsp->core.mx1.u);
	state_save_register_device_item(device, 0, adsp->core.my0.u);
	state_save_register_device_item(device, 0, adsp->core.my1.u);
	state_save_register_device_item(device, 0, adsp->core.mr.mr);
	state_save_register_device_item(device, 0, adsp->core.mf.u);
	state_save_register_device_item(device, 0, adsp->core.si.u);
	state_save_register_device_item(device, 0, adsp->core.se.u);
	state_save_register_device_item(device, 0, adsp->core.sb.u);
	state_save_register_device_item(device, 0, adsp->core.sr.sr);
	state_save_register_device_item(device, 0, adsp->core.zero.u);

	/* "alt" */
	state_save_register_device_item(device, 0, adsp->alt.ax0.u);
	state_save_register_device_item(device, 0, adsp->alt.ax1.u);
	state_save_register_device_item(device, 0, adsp->alt.ay0.u);
	state_save_register_device_item(device, 0, adsp->alt.ay1.u);
	state_save_register_device_item(device, 0, adsp->alt.ar.u);
	state_save_register_device_item(device, 0, adsp->alt.af.u);
	state_save_register_device_item(device, 0, adsp->alt.mx0.u);
	state_save_register_device_item(device, 0, adsp->alt.mx1.u);
	state_save_register_device_item(device, 0, adsp->alt.my0.u);
	state_save_register_device_item(device, 0, adsp->alt.my1.u);
	state_save_register_device_item(device, 0, adsp->alt.mr.mr);
	state_save_register_device_item(device, 0, adsp->alt.mf.u);
	state_save_register_device_item(device, 0, adsp->alt.si.u);
	state_save_register_device_item(device, 0, adsp->alt.se.u);
	state_save_register_device_item(device, 0, adsp->alt.sb.u);
	state_save_register_device_item(device, 0, adsp->alt.sr.sr);
	state_save_register_device_item(device, 0, adsp->alt.zero.u);

	state_save_register_device_item_array(device, 0, adsp->i);
	state_save_register_device_item_array(device, 0, adsp->m);
	state_save_register_device_item_array(device, 0, adsp->l);
	state_save_register_device_item_array(device, 0, adsp->lmask);
	state_save_register_device_item_array(device, 0, adsp->base);
	state_save_register_device_item(device, 0, adsp->px);

	state_save_register_device_item(device, 0, adsp->pc);
	state_save_register_device_item(device, 0, adsp->ppc);
	state_save_register_device_item(device, 0, adsp->loop);
	state_save_register_device_item(device, 0, adsp->loop_condition);
	state_save_register_device_item(device, 0, adsp->cntr);
	state_save_register_device_item(device, 0, adsp->astat);
	state_save_register_device_item(device, 0, adsp->sstat);
	state_save_register_device_item(device, 0, adsp->mstat);
	state_save_register_device_item(device, 0, adsp->mstat_prev);
	state_save_register_device_item(device, 0, adsp->astat_clear);
	state_save_register_device_item(device, 0, adsp->idle);

	state_save_register_device_item_array(device, 0, adsp->loop_stack);
	state_save_register_device_item_array(device, 0, adsp->cntr_stack);
	state_save_register_device_item_array(device, 0, adsp->pc_stack);
	state_save_register_device_item_2d_array(device, 0, adsp->stat_stack);

	state_save_register_device_item(device, 0, adsp->pc_sp);
	state_save_register_device_item(device, 0, adsp->cntr_sp);
	state_save_register_device_item(device, 0, adsp->stat_sp);
	state_save_register_device_item(device, 0, adsp->loop_sp);

	state_save_register_device_item(device, 0, adsp->flagout);
	state_save_register_device_item(device, 0, adsp->flagin);
	state_save_register_device_item(device, 0, adsp->fl0);
	state_save_register_device_item(device, 0, adsp->fl1);
	state_save_register_device_item(device, 0, adsp->fl2);
	state_save_register_device_item(device, 0, adsp->idma_addr);
	state_save_register_device_item(device, 0, adsp->idma_cache);
	state_save_register_device_item(device, 0, adsp->idma_offs);

	state_save_register_device_item(device, 0, adsp->imask);
	state_save_register_device_item(device, 0, adsp->icntl);
	state_save_register_device_item(device, 0, adsp->ifc);
	state_save_register_device_item_array(device, 0, adsp->irq_state);
	state_save_register_device_item_array(device, 0, adsp->irq_latch);
#endif
	return adsp;
}


//static CPU_RESET( adsp21xx )
void adsp21xx_reset(adsp2100_state *adsp)
{
	int irq;

	/* ensure that zero is zero */
	adsp->core.zero.u = adsp->alt.zero.u = 0;

	/* recompute the memory registers with their current values */
	wr_l0(adsp, adsp->l[0]);  wr_i0(adsp, adsp->i[0]);
	wr_l1(adsp, adsp->l[1]);  wr_i1(adsp, adsp->i[1]);
	wr_l2(adsp, adsp->l[2]);  wr_i2(adsp, adsp->i[2]);
	wr_l3(adsp, adsp->l[3]);  wr_i3(adsp, adsp->i[3]);
	wr_l4(adsp, adsp->l[4]);  wr_i4(adsp, adsp->i[4]);
	wr_l5(adsp, adsp->l[5]);  wr_i5(adsp, adsp->i[5]);
	wr_l6(adsp, adsp->l[6]);  wr_i6(adsp, adsp->i[6]);
	wr_l7(adsp, adsp->l[7]);  wr_i7(adsp, adsp->i[7]);

	/* reset PC and loops */
	switch (adsp->chip_type)
	{
		case CHIP_TYPE_ADSP2100:
			adsp->pc = 4;
			break;

		case CHIP_TYPE_ADSP2101:
		case CHIP_TYPE_ADSP2104:
		case CHIP_TYPE_ADSP2105:
		case CHIP_TYPE_ADSP2115:
		case CHIP_TYPE_ADSP2181:
			adsp->pc = 0;
			break;

		default:
			logerror( "ADSP2100 core: Unknown chip type!. Defaulting to adsp->\n" );
			adsp->pc = 4;
			adsp->chip_type = CHIP_TYPE_ADSP2100;
			break;
	}

	adsp->ppc = -1;
	adsp->loop = 0xffff;
	adsp->loop_condition = 0;

	/* reset status registers */
	adsp->astat_clear = ~(CFLAG | VFLAG | NFLAG | ZFLAG);
	adsp->mstat = 0;
	adsp->sstat = 0x55;
	adsp->idle = 0;
	update_mstat(adsp);

	/* reset stacks */
	adsp->pc_sp = 0;
	adsp->cntr_sp = 0;
	adsp->stat_sp = 0;
	adsp->loop_sp = 0;

	/* reset external I/O */
	adsp->flagout = 0;
	adsp->flagin = 0;
	adsp->fl0 = 0;
	adsp->fl1 = 0;
	adsp->fl2 = 0;

	/* reset interrupts */
	adsp->imask = 0;
	for (irq = 0; irq < 8; irq++)
		adsp->irq_state[irq] = adsp->irq_latch[irq] = CLEAR_LINE;
}


static int create_tables(void)
{
	int i;

	/* allocate the tables */
	if (!reverse_table)
		reverse_table = (UINT16 *)malloc(0x4000 * sizeof(UINT16));
	if (!mask_table)
		mask_table = (UINT16 *)malloc(0x4000 * sizeof(UINT16));
	if (!condition_table)
		condition_table = (UINT8 *)malloc(0x1000 * sizeof(UINT8));

	/* handle errors */
	if (reverse_table == NULL || mask_table == NULL || condition_table == NULL)
		return 0;

	/* initialize the bit reversing table */
	for (i = 0; i < 0x4000; i++)
	{
		UINT16 data = 0;

		data |= (i >> 13) & 0x0001;
		data |= (i >> 11) & 0x0002;
		data |= (i >> 9)  & 0x0004;
		data |= (i >> 7)  & 0x0008;
		data |= (i >> 5)  & 0x0010;
		data |= (i >> 3)  & 0x0020;
		data |= (i >> 1)  & 0x0040;
		data |= (i << 1)  & 0x0080;
		data |= (i << 3)  & 0x0100;
		data |= (i << 5)  & 0x0200;
		data |= (i << 7)  & 0x0400;
		data |= (i << 9)  & 0x0800;
		data |= (i << 11) & 0x1000;
		data |= (i << 13) & 0x2000;

		reverse_table[i] = data;
	}

	/* initialize the mask table */
	for (i = 0; i < 0x4000; i++)
	{
		     if (i > 0x2000) mask_table[i] = 0x0000;
		else if (i > 0x1000) mask_table[i] = 0x2000;
		else if (i > 0x0800) mask_table[i] = 0x3000;
		else if (i > 0x0400) mask_table[i] = 0x3800;
		else if (i > 0x0200) mask_table[i] = 0x3c00;
		else if (i > 0x0100) mask_table[i] = 0x3e00;
		else if (i > 0x0080) mask_table[i] = 0x3f00;
		else if (i > 0x0040) mask_table[i] = 0x3f80;
		else if (i > 0x0020) mask_table[i] = 0x3fc0;
		else if (i > 0x0010) mask_table[i] = 0x3fe0;
		else if (i > 0x0008) mask_table[i] = 0x3ff0;
		else if (i > 0x0004) mask_table[i] = 0x3ff8;
		else if (i > 0x0002) mask_table[i] = 0x3ffc;
		else if (i > 0x0001) mask_table[i] = 0x3ffe;
		else                 mask_table[i] = 0x3fff;
	}

	/* initialize the condition table */
	for (i = 0; i < 0x100; i++)
	{
		int az = ((i & ZFLAG) != 0);
		int an = ((i & NFLAG) != 0);
		int av = ((i & VFLAG) != 0);
		int ac = ((i & CFLAG) != 0);
		int mv = ((i & MVFLAG) != 0);
		int as = ((i & SFLAG) != 0);

		condition_table[i | 0x000] = az;
		condition_table[i | 0x100] = !az;
		condition_table[i | 0x200] = !((an ^ av) | az);
		condition_table[i | 0x300] = (an ^ av) | az;
		condition_table[i | 0x400] = an ^ av;
		condition_table[i | 0x500] = !(an ^ av);
		condition_table[i | 0x600] = av;
		condition_table[i | 0x700] = !av;
		condition_table[i | 0x800] = ac;
		condition_table[i | 0x900] = !ac;
		condition_table[i | 0xa00] = as;
		condition_table[i | 0xb00] = !as;
		condition_table[i | 0xc00] = mv;
		condition_table[i | 0xd00] = !mv;
		condition_table[i | 0xf00] = 1;
	}
	return 1;
}


//static CPU_EXIT( adsp21xx )
void adsp21xx_exit(adsp2100_state *adsp)
{
	if (reverse_table != NULL)
		free(reverse_table);
	reverse_table = NULL;

	if (mask_table != NULL)
		free(mask_table);
	mask_table = NULL;

	if (condition_table != NULL)
		free(condition_table);
	condition_table = NULL;

#if TRACK_HOTSPOTS
	{
		FILE *log = fopen("adsp.hot", "w");
		while (1)
		{
			int maxindex = 0, i;
			for (i = 1; i < 0x4000; i++)
				if (pcbucket[i] > pcbucket[maxindex])
					maxindex = i;
			if (pcbucket[maxindex] == 0)
				break;
			fprintf(log, "PC=%04X  (%10d hits)\n", maxindex, pcbucket[maxindex]);
			pcbucket[maxindex] = 0;
		}
		fclose(log);
	}
#endif

}

// States
void adsp21xx_scan(adsp2100_state *adsp, INT32 nAction)
{
	if (nAction & ACB_DRIVER_DATA) {
		struct BurnArea ba;
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = adsp;
		ba.nLen	  = STRUCT_SIZE_HELPER(adsp2100_state, irq_latch);
		ba.szName = "Adsp21xx Regs";
		BurnAcb(&ba);
	}
}

/***************************************************************************
    CORE EXECUTION LOOP
***************************************************************************/
void adsp21xx_stop_execute(adsp2100_state *adsp)
{
	adsp->end_run = 1;
}

int adsp21xx_total_cycles(adsp2100_state *adsp)
{
	return adsp->total_cycles + (adsp->cycles_start - adsp->icount);
}

void adsp21xx_new_frame(adsp2100_state *adsp)
{
	adsp->total_cycles = 0;
}

/* execute instructions on this CPU until icount expires */
//static CPU_EXECUTE( adsp21xx )
int adsp21xx_execute(adsp2100_state *adsp, int cycles)
{
//	int check_debugger = ((device->machine->debug_flags & DEBUG_FLAG_ENABLED) != 0);
//	adsp2100_state *adsp = get_safe_token(device);

	adsp->end_run = 0;
	adsp->cycles_start = cycles;

	check_irqs(adsp);

	/* count cycles and interrupt cycles */
	adsp->icount = cycles;

	do
	{
		UINT32 temp;
		UINT32 op;

		/* debugging */
		adsp->ppc = adsp->pc;	/* copy PC to previous PC */
//		if (check_debugger)
//			debugger_instruction_hook(device, adsp->pc);

#if TRACK_HOTSPOTS
		pcbucket[adsp->pc & 0x3fff]++;
#endif

		/* instruction fetch */
		op = ROPCODE(adsp);

		/* advance to the next instruction */
		if (adsp->pc != adsp->loop)
			adsp->pc++;

		/* handle looping */
		else
		{
			/* condition not met, keep looping */
			if (CONDITION(adsp, adsp->loop_condition))
				adsp->pc = pc_stack_top(adsp);

			/* condition met; pop the PC and loop stacks and fall through */
			else
			{
				loop_stack_pop(adsp);
				pc_stack_pop_val(adsp);
				adsp->pc++;
			}
		}

		/* parse the instruction */
		switch ((op >> 16) & 0xff)
		{
			case 0x00:
				/* 00000000 00000000 00000000  NOP */
				break;
			case 0x01:
				/* 00000001 0xxxxxxx xxxxxxxx  dst = IO(x) */
				/* 00000001 1xxxxxxx xxxxxxxx  IO(x) = dst */
				/* ADSP-218x only */
				if (adsp->chip_type >= CHIP_TYPE_ADSP2181)
				{
					if ((op & 0x008000) == 0x000000)
						WRITE_REG(adsp, 0, op & 15, RWORD_IO(adsp, (op >> 4) & 0x7ff));
					else
						WWORD_IO(adsp, (op >> 4) & 0x7ff, READ_REG(adsp, 0, op & 15));
				}
				break;
			case 0x02:
				/* 00000010 0000xxxx xxxxxxxx  modify flag out */
				/* 00000010 10000000 00000000  idle */
				/* 00000010 10000000 0000xxxx  idle (n) */
				if (op & 0x008000)
				{
					adsp->idle = 1;
					adsp->icount = 0;
				}
				else
				{
					if (CONDITION(adsp, op & 15))
					{
						if (op & 0x020) adsp->flagout = 0;
						if (op & 0x010) adsp->flagout ^= 1;
						if (adsp->chip_type >= CHIP_TYPE_ADSP2101)
						{
							if (op & 0x080) adsp->fl0 = 0;
							if (op & 0x040) adsp->fl0 ^= 1;
							if (op & 0x200) adsp->fl1 = 0;
							if (op & 0x100) adsp->fl1 ^= 1;
							if (op & 0x800) adsp->fl2 = 0;
							if (op & 0x400) adsp->fl2 ^= 1;
						}
					}
				}
				break;
			case 0x03:
				/* 00000011 xxxxxxxx xxxxxxxx  call or jump on flag in */
				if (op & 0x000002)
				{
					if (adsp->flagin)
					{
						if (op & 0x000001)
							pc_stack_push(adsp);
						adsp->pc = ((op >> 4) & 0x0fff) | ((op << 10) & 0x3000);
					}
				}
				else
				{
					if (!adsp->flagin)
					{
						if (op & 0x000001)
							pc_stack_push(adsp);
						adsp->pc = ((op >> 4) & 0x0fff) | ((op << 10) & 0x3000);
					}
				}
				break;
			case 0x04:
				/* 00000100 00000000 000xxxxx  stack control */
				if (op & 0x000010) pc_stack_pop_val(adsp);
				if (op & 0x000008) loop_stack_pop(adsp);
				if (op & 0x000004) cntr_stack_pop(adsp);
				if (op & 0x000002)
				{
					if (op & 0x000001) stat_stack_pop(adsp);
					else stat_stack_push(adsp);
				}
				break;
			case 0x05:
				/* 00000101 00000000 00000000  saturate MR */
				if (GET_MV)
				{
					if (adsp->core.mr.mrx.mr2.u & 0x80)
						adsp->core.mr.mrx.mr2.u = 0xffff, adsp->core.mr.mrx.mr1.u = 0x8000, adsp->core.mr.mrx.mr0.u = 0x0000;
					else
						adsp->core.mr.mrx.mr2.u = 0x0000, adsp->core.mr.mrx.mr1.u = 0x7fff, adsp->core.mr.mrx.mr0.u = 0xffff;
				}
				break;
			case 0x06:
				/* 00000110 000xxxxx 00000000  DIVS */
				{
					int xop = (op >> 8) & 7;
					int yop = (op >> 11) & 3;

					xop = ALU_GETXREG_UNSIGNED(adsp, xop);
					yop = ALU_GETYREG_UNSIGNED(adsp, yop);

					temp = xop ^ yop;
					adsp->astat = (adsp->astat & ~QFLAG) | ((temp >> 10) & QFLAG);
					adsp->core.af.u = (yop << 1) | (adsp->core.ay0.u >> 15);
					adsp->core.ay0.u = (adsp->core.ay0.u << 1) | (temp >> 15);
				}
				break;
			case 0x07:
				/* 00000111 00010xxx 00000000  DIVQ */
				{
					int xop = (op >> 8) & 7;
					int res;

					xop = ALU_GETXREG_UNSIGNED(adsp, xop);

					if (GET_Q)
						res = adsp->core.af.u + xop;
					else
						res = adsp->core.af.u - xop;

					temp = res ^ xop;
					adsp->astat = (adsp->astat & ~QFLAG) | ((temp >> 10) & QFLAG);
					adsp->core.af.u = (res << 1) | (adsp->core.ay0.u >> 15);
					adsp->core.ay0.u = (adsp->core.ay0.u << 1) | ((~temp >> 15) & 0x0001);
				}
				break;
			case 0x08:
				/* 00001000 00000000 0000xxxx  reserved */
				break;
			case 0x09:
				/* 00001001 00000000 000xxxxx  modify address register */
				temp = (op >> 2) & 4;
				modify_address(adsp, temp + ((op >> 2) & 3), temp + (op & 3));
				break;
			case 0x0a:
				/* 00001010 00000000 000xxxxx  conditional return */
				if (CONDITION(adsp, op & 15))
				{
					pc_stack_pop(adsp);

					/* RTI case */
					if (op & 0x000010)
						stat_stack_pop(adsp);
				}
				break;
			case 0x0b:
				/* 00001011 00000000 xxxxxxxx  conditional jump (indirect address) */
				if (CONDITION(adsp, op & 15))
				{
					if (op & 0x000010)
						pc_stack_push(adsp);
					adsp->pc = adsp->i[4 + ((op >> 6) & 3)] & 0x3fff;
				}
				break;
			case 0x0c:
				/* 00001100 xxxxxxxx xxxxxxxx  mode control */
				if (adsp->chip_type >= CHIP_TYPE_ADSP2101)
				{
					if (op & 0x000008) adsp->mstat = (adsp->mstat & ~MSTAT_GOMODE) | ((op << 5) & MSTAT_GOMODE);
					if (op & 0x002000) adsp->mstat = (adsp->mstat & ~MSTAT_INTEGER) | ((op >> 8) & MSTAT_INTEGER);
					if (op & 0x008000) adsp->mstat = (adsp->mstat & ~MSTAT_TIMER) | ((op >> 9) & MSTAT_TIMER);
				}
				if (op & 0x000020) adsp->mstat = (adsp->mstat & ~MSTAT_BANK) | ((op >> 4) & MSTAT_BANK);
				if (op & 0x000080) adsp->mstat = (adsp->mstat & ~MSTAT_REVERSE) | ((op >> 5) & MSTAT_REVERSE);
				if (op & 0x000200) adsp->mstat = (adsp->mstat & ~MSTAT_STICKYV) | ((op >> 6) & MSTAT_STICKYV);
				if (op & 0x000800) adsp->mstat = (adsp->mstat & ~MSTAT_SATURATE) | ((op >> 7) & MSTAT_SATURATE);
				update_mstat(adsp);
				break;
			case 0x0d:
				/* 00001101 0000xxxx xxxxxxxx  internal data move */
				WRITE_REG(adsp, (op >> 10) & 3, (op >> 4) & 15, READ_REG(adsp, (op >> 8) & 3, op & 15));
				break;
			case 0x0e:
				/* 00001110 0xxxxxxx xxxxxxxx  conditional shift */
				if (CONDITION(adsp, op & 15)) shift_op(adsp, op);
				break;
			case 0x0f:
				/* 00001111 0xxxxxxx xxxxxxxx  shift immediate */
				shift_op_imm(adsp, op);
				break;
			case 0x10:
				/* 00010000 0xxxxxxx xxxxxxxx  shift with internal data register move */
				shift_op(adsp, op);
				temp = READ_REG(adsp, 0, op & 15);
				WRITE_REG(adsp, 0, (op >> 4) & 15, temp);
				break;
			case 0x11:
				/* 00010001 xxxxxxxx xxxxxxxx  shift with pgm memory read/write */
				if (op & 0x8000)
				{
					pgm_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
					shift_op(adsp, op);
				}
				else
				{
					shift_op(adsp, op);
					WRITE_REG(adsp, 0, (op >> 4) & 15, pgm_read_dag2(adsp, op));
				}
				break;
			case 0x12:
				/* 00010010 xxxxxxxx xxxxxxxx  shift with data memory read/write DAG1 */
				if (op & 0x8000)
				{
					data_write_dag1(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
					shift_op(adsp, op);
				}
				else
				{
					shift_op(adsp, op);
					WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag1(adsp, op));
				}
				break;
			case 0x13:
				/* 00010011 xxxxxxxx xxxxxxxx  shift with data memory read/write DAG2 */
				if (op & 0x8000)
				{
					data_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
					shift_op(adsp, op);
				}
				else
				{
					shift_op(adsp, op);
					WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag2(adsp, op));
				}
				break;
			case 0x14: case 0x15: case 0x16: case 0x17:
				/* 000101xx xxxxxxxx xxxxxxxx  do until */
				loop_stack_push(adsp, op & 0x3ffff);
				pc_stack_push(adsp);
				break;
			case 0x18: case 0x19: case 0x1a: case 0x1b:
				/* 000110xx xxxxxxxx xxxxxxxx  conditional jump (immediate addr) */
				if (CONDITION(adsp, op & 15))
				{
					adsp->pc = (op >> 4) & 0x3fff;
					/* check for a busy loop */
					if (adsp->pc == adsp->ppc)
						adsp->icount = 0;
				}
				break;
			case 0x1c: case 0x1d: case 0x1e: case 0x1f:
				/* 000111xx xxxxxxxx xxxxxxxx  conditional call (immediate addr) */
				if (CONDITION(adsp, op & 15))
				{
					pc_stack_push(adsp);
					adsp->pc = (op >> 4) & 0x3fff;
				}
				break;
			case 0x20: case 0x21:
				/* 0010000x xxxxxxxx xxxxxxxx  conditional MAC to MR */
				if (CONDITION(adsp, op & 15))
				{
					if (adsp->chip_type >= CHIP_TYPE_ADSP2181 && (op & 0x0018f0) == 0x000010)
						mac_op_mr_xop(adsp, op);
					else
						mac_op_mr(adsp, op);
				}
				break;
			case 0x22: case 0x23:
				/* 0010001x xxxxxxxx xxxxxxxx  conditional ALU to AR */
				if (CONDITION(adsp, op & 15))
				{
					if (adsp->chip_type >= CHIP_TYPE_ADSP2181 && (op & 0x000010) == 0x000010)
						alu_op_ar_const(adsp, op);
					else
						alu_op_ar(adsp, op);
				}
				break;
			case 0x24: case 0x25:
				/* 0010010x xxxxxxxx xxxxxxxx  conditional MAC to MF */
				if (CONDITION(adsp, op & 15))
				{
					if (adsp->chip_type >= CHIP_TYPE_ADSP2181 && (op & 0x0018f0) == 0x000010)
						mac_op_mf_xop(adsp, op);
					else
						mac_op_mf(adsp, op);
				}
				break;
			case 0x26: case 0x27:
				/* 0010011x xxxxxxxx xxxxxxxx  conditional ALU to AF */
				if (CONDITION(adsp, op & 15))
				{
					if (adsp->chip_type >= CHIP_TYPE_ADSP2181 && (op & 0x000010) == 0x000010)
						alu_op_af_const(adsp, op);
					else
						alu_op_af(adsp, op);
				}
				break;
			case 0x28: case 0x29:
				/* 0010100x xxxxxxxx xxxxxxxx  MAC to MR with internal data register move */
				temp = READ_REG(adsp, 0, op & 15);
				mac_op_mr(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, temp);
				break;
			case 0x2a: case 0x2b:
				/* 0010101x xxxxxxxx xxxxxxxx  ALU to AR with internal data register move */
				if (adsp->chip_type >= CHIP_TYPE_ADSP2181 && (op & 0x0000ff) == 0x0000aa)
					alu_op_none(adsp, op);
				else
				{
					temp = READ_REG(adsp, 0, op & 15);
					alu_op_ar(adsp, op);
					WRITE_REG(adsp, 0, (op >> 4) & 15, temp);
				}
				break;
			case 0x2c: case 0x2d:
				/* 0010110x xxxxxxxx xxxxxxxx  MAC to MF with internal data register move */
				temp = READ_REG(adsp, 0, op & 15);
				mac_op_mf(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, temp);
				break;
			case 0x2e: case 0x2f:
				/* 0010111x xxxxxxxx xxxxxxxx  ALU to AF with internal data register move */
				temp = READ_REG(adsp, 0, op & 15);
				alu_op_af(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, temp);
				break;
			case 0x30: case 0x31: case 0x32: case 0x33:
				/* 001100xx xxxxxxxx xxxxxxxx  load non-data register immediate (group 0) */
				WRITE_REG(adsp, 0, op & 15, (INT32)(op << 14) >> 18);
				break;
			case 0x34: case 0x35: case 0x36: case 0x37:
				/* 001101xx xxxxxxxx xxxxxxxx  load non-data register immediate (group 1) */
				WRITE_REG(adsp, 1, op & 15, (INT32)(op << 14) >> 18);
				break;
			case 0x38: case 0x39: case 0x3a: case 0x3b:
				/* 001110xx xxxxxxxx xxxxxxxx  load non-data register immediate (group 2) */
				WRITE_REG(adsp, 2, op & 15, (INT32)(op << 14) >> 18);
				break;
			case 0x3c: case 0x3d: case 0x3e: case 0x3f:
				/* 001111xx xxxxxxxx xxxxxxxx  load non-data register immediate (group 3) */
				WRITE_REG(adsp, 3, op & 15, (INT32)(op << 14) >> 18);
				break;
			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
				/* 0100xxxx xxxxxxxx xxxxxxxx  load data register immediate */
				WRITE_REG(adsp, 0, op & 15, (op >> 4) & 0xffff);
				break;
			case 0x50: case 0x51:
				/* 0101000x xxxxxxxx xxxxxxxx  MAC to MR with pgm memory read */
				mac_op_mr(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, pgm_read_dag2(adsp, op));
				break;
			case 0x52: case 0x53:
				/* 0101001x xxxxxxxx xxxxxxxx  ALU to AR with pgm memory read */
				alu_op_ar(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, pgm_read_dag2(adsp, op));
				break;
			case 0x54: case 0x55:
				/* 0101010x xxxxxxxx xxxxxxxx  MAC to MF with pgm memory read */
				mac_op_mf(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, pgm_read_dag2(adsp, op));
				break;
			case 0x56: case 0x57:
				/* 0101011x xxxxxxxx xxxxxxxx  ALU to AF with pgm memory read */
				alu_op_af(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, pgm_read_dag2(adsp, op));
				break;
			case 0x58: case 0x59:
				/* 0101100x xxxxxxxx xxxxxxxx  MAC to MR with pgm memory write */
				pgm_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				mac_op_mr(adsp, op);
				break;
			case 0x5a: case 0x5b:
				/* 0101101x xxxxxxxx xxxxxxxx  ALU to AR with pgm memory write */
				pgm_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				alu_op_ar(adsp, op);
				break;
			case 0x5c: case 0x5d:
				/* 0101110x xxxxxxxx xxxxxxxx  ALU to MR with pgm memory write */
				pgm_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				mac_op_mf(adsp, op);
				break;
			case 0x5e: case 0x5f:
				/* 0101111x xxxxxxxx xxxxxxxx  ALU to MF with pgm memory write */
				pgm_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				alu_op_af(adsp, op);
				break;
			case 0x60: case 0x61:
				/* 0110000x xxxxxxxx xxxxxxxx  MAC to MR with data memory read DAG1 */
				mac_op_mr(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag1(adsp, op));
				break;
			case 0x62: case 0x63:
				/* 0110001x xxxxxxxx xxxxxxxx  ALU to AR with data memory read DAG1 */
				alu_op_ar(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag1(adsp, op));
				break;
			case 0x64: case 0x65:
				/* 0110010x xxxxxxxx xxxxxxxx  MAC to MF with data memory read DAG1 */
				mac_op_mf(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag1(adsp, op));
				break;
			case 0x66: case 0x67:
				/* 0110011x xxxxxxxx xxxxxxxx  ALU to AF with data memory read DAG1 */
				alu_op_af(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag1(adsp, op));
				break;
			case 0x68: case 0x69:
				/* 0110100x xxxxxxxx xxxxxxxx  MAC to MR with data memory write DAG1 */
				data_write_dag1(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				mac_op_mr(adsp, op);
				break;
			case 0x6a: case 0x6b:
				/* 0110101x xxxxxxxx xxxxxxxx  ALU to AR with data memory write DAG1 */
				data_write_dag1(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				alu_op_ar(adsp, op);
				break;
			case 0x6c: case 0x6d:
				/* 0111110x xxxxxxxx xxxxxxxx  MAC to MF with data memory write DAG1 */
				data_write_dag1(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				mac_op_mf(adsp, op);
				break;
			case 0x6e: case 0x6f:
				/* 0111111x xxxxxxxx xxxxxxxx  ALU to AF with data memory write DAG1 */
				data_write_dag1(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				alu_op_af(adsp, op);
				break;
			case 0x70: case 0x71:
				/* 0111000x xxxxxxxx xxxxxxxx  MAC to MR with data memory read DAG2 */
				mac_op_mr(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag2(adsp, op));
				break;
			case 0x72: case 0x73:
				/* 0111001x xxxxxxxx xxxxxxxx  ALU to AR with data memory read DAG2 */
				alu_op_ar(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag2(adsp, op));
				break;
			case 0x74: case 0x75:
				/* 0111010x xxxxxxxx xxxxxxxx  MAC to MF with data memory read DAG2 */
				mac_op_mf(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag2(adsp, op));
				break;
			case 0x76: case 0x77:
				/* 0111011x xxxxxxxx xxxxxxxx  ALU to AF with data memory read DAG2 */
				alu_op_af(adsp, op);
				WRITE_REG(adsp, 0, (op >> 4) & 15, data_read_dag2(adsp, op));
				break;
			case 0x78: case 0x79:
				/* 0111100x xxxxxxxx xxxxxxxx  MAC to MR with data memory write DAG2 */
				data_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				mac_op_mr(adsp, op);
				break;
			case 0x7a: case 0x7b:
				/* 0111101x xxxxxxxx xxxxxxxx  ALU to AR with data memory write DAG2 */
				data_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				alu_op_ar(adsp, op);
				break;
			case 0x7c: case 0x7d:
				/* 0111110x xxxxxxxx xxxxxxxx  MAC to MF with data memory write DAG2 */
				data_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				mac_op_mf(adsp, op);
				break;
			case 0x7e: case 0x7f:
				/* 0111111x xxxxxxxx xxxxxxxx  ALU to AF with data memory write DAG2 */
				data_write_dag2(adsp, op, READ_REG(adsp, 0, (op >> 4) & 15));
				alu_op_af(adsp, op);
				break;
			case 0x80: case 0x81: case 0x82: case 0x83:
				/* 100000xx xxxxxxxx xxxxxxxx  read data memory (immediate addr) to reg group 0 */
				WRITE_REG(adsp, 0, op & 15, RWORD_DATA(adsp, (op >> 4) & 0x3fff));
				break;
			case 0x84: case 0x85: case 0x86: case 0x87:
				/* 100001xx xxxxxxxx xxxxxxxx  read data memory (immediate addr) to reg group 1 */
				WRITE_REG(adsp, 1, op & 15, RWORD_DATA(adsp, (op >> 4) & 0x3fff));
				break;
			case 0x88: case 0x89: case 0x8a: case 0x8b:
				/* 100010xx xxxxxxxx xxxxxxxx  read data memory (immediate addr) to reg group 2 */
				WRITE_REG(adsp, 2, op & 15, RWORD_DATA(adsp, (op >> 4) & 0x3fff));
				break;
			case 0x8c: case 0x8d: case 0x8e: case 0x8f:
				/* 100011xx xxxxxxxx xxxxxxxx  read data memory (immediate addr) to reg group 3 */
				WRITE_REG(adsp, 3, op & 15, RWORD_DATA(adsp, (op >> 4) & 0x3fff));
				break;
			case 0x90: case 0x91: case 0x92: case 0x93:
				/* 1001xxxx xxxxxxxx xxxxxxxx  write data memory (immediate addr) from reg group 0 */
				WWORD_DATA(adsp, (op >> 4) & 0x3fff, READ_REG(adsp, 0, op & 15));
				break;
			case 0x94: case 0x95: case 0x96: case 0x97:
				/* 1001xxxx xxxxxxxx xxxxxxxx  write data memory (immediate addr) from reg group 1 */
				WWORD_DATA(adsp, (op >> 4) & 0x3fff, READ_REG(adsp, 1, op & 15));
				break;
			case 0x98: case 0x99: case 0x9a: case 0x9b:
				/* 1001xxxx xxxxxxxx xxxxxxxx  write data memory (immediate addr) from reg group 2 */
				WWORD_DATA(adsp, (op >> 4) & 0x3fff, READ_REG(adsp, 2, op & 15));
				break;
			case 0x9c: case 0x9d: case 0x9e: case 0x9f:
				/* 1001xxxx xxxxxxxx xxxxxxxx  write data memory (immediate addr) from reg group 3 */
				WWORD_DATA(adsp, (op >> 4) & 0x3fff, READ_REG(adsp, 3, op & 15));
				break;
			case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
			case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
				/* 1010xxxx xxxxxxxx xxxxxxxx  data memory write (immediate) DAG1 */
				data_write_dag1(adsp, op, (op >> 4) & 0xffff);
				break;
			case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
			case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
				/* 1011xxxx xxxxxxxx xxxxxxxx  data memory write (immediate) DAG2 */
				data_write_dag2(adsp, op, (op >> 4) & 0xffff);
				break;
			case 0xc0: case 0xc1:
				/* 1100000x xxxxxxxx xxxxxxxx  MAC to MR with data read to AX0 & pgm read to AY0 */
				mac_op_mr(adsp, op);
				adsp->core.ax0.u = data_read_dag1(adsp, op);
				adsp->core.ay0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xc2: case 0xc3:
				/* 1100001x xxxxxxxx xxxxxxxx  ALU to AR with data read to AX0 & pgm read to AY0 */
				alu_op_ar(adsp, op);
				adsp->core.ax0.u = data_read_dag1(adsp, op);
				adsp->core.ay0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xc4: case 0xc5:
				/* 1100010x xxxxxxxx xxxxxxxx  MAC to MR with data read to AX1 & pgm read to AY0 */
				mac_op_mr(adsp, op);
				adsp->core.ax1.u = data_read_dag1(adsp, op);
				adsp->core.ay0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xc6: case 0xc7:
				/* 1100011x xxxxxxxx xxxxxxxx  ALU to AR with data read to AX1 & pgm read to AY0 */
				alu_op_ar(adsp, op);
				adsp->core.ax1.u = data_read_dag1(adsp, op);
				adsp->core.ay0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xc8: case 0xc9:
				/* 1100100x xxxxxxxx xxxxxxxx  MAC to MR with data read to MX0 & pgm read to AY0 */
				mac_op_mr(adsp, op);
				adsp->core.mx0.u = data_read_dag1(adsp, op);
				adsp->core.ay0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xca: case 0xcb:
				/* 1100101x xxxxxxxx xxxxxxxx  ALU to AR with data read to MX0 & pgm read to AY0 */
				alu_op_ar(adsp, op);
				adsp->core.mx0.u = data_read_dag1(adsp, op);
				adsp->core.ay0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xcc: case 0xcd:
				/* 1100110x xxxxxxxx xxxxxxxx  MAC to MR with data read to MX1 & pgm read to AY0 */
				mac_op_mr(adsp, op);
				adsp->core.mx1.u = data_read_dag1(adsp, op);
				adsp->core.ay0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xce: case 0xcf:
				/* 1100111x xxxxxxxx xxxxxxxx  ALU to AR with data read to MX1 & pgm read to AY0 */
				alu_op_ar(adsp, op);
				adsp->core.mx1.u = data_read_dag1(adsp, op);
				adsp->core.ay0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xd0: case 0xd1:
				/* 1101000x xxxxxxxx xxxxxxxx  MAC to MR with data read to AX0 & pgm read to AY1 */
				mac_op_mr(adsp, op);
				adsp->core.ax0.u = data_read_dag1(adsp, op);
				adsp->core.ay1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xd2: case 0xd3:
				/* 1101001x xxxxxxxx xxxxxxxx  ALU to AR with data read to AX0 & pgm read to AY1 */
				alu_op_ar(adsp, op);
				adsp->core.ax0.u = data_read_dag1(adsp, op);
				adsp->core.ay1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xd4: case 0xd5:
				/* 1101010x xxxxxxxx xxxxxxxx  MAC to MR with data read to AX1 & pgm read to AY1 */
				mac_op_mr(adsp, op);
				adsp->core.ax1.u = data_read_dag1(adsp, op);
				adsp->core.ay1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xd6: case 0xd7:
				/* 1101011x xxxxxxxx xxxxxxxx  ALU to AR with data read to AX1 & pgm read to AY1 */
				alu_op_ar(adsp, op);
				adsp->core.ax1.u = data_read_dag1(adsp, op);
				adsp->core.ay1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xd8: case 0xd9:
				/* 1101100x xxxxxxxx xxxxxxxx  MAC to MR with data read to MX0 & pgm read to AY1 */
				mac_op_mr(adsp, op);
				adsp->core.mx0.u = data_read_dag1(adsp, op);
				adsp->core.ay1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xda: case 0xdb:
				/* 1101101x xxxxxxxx xxxxxxxx  ALU to AR with data read to MX0 & pgm read to AY1 */
				alu_op_ar(adsp, op);
				adsp->core.mx0.u = data_read_dag1(adsp, op);
				adsp->core.ay1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xdc: case 0xdd:
				/* 1101110x xxxxxxxx xxxxxxxx  MAC to MR with data read to MX1 & pgm read to AY1 */
				mac_op_mr(adsp, op);
				adsp->core.mx1.u = data_read_dag1(adsp, op);
				adsp->core.ay1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xde: case 0xdf:
				/* 1101111x xxxxxxxx xxxxxxxx  ALU to AR with data read to MX1 & pgm read to AY1 */
				alu_op_ar(adsp, op);
				adsp->core.mx1.u = data_read_dag1(adsp, op);
				adsp->core.ay1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xe0: case 0xe1:
				/* 1110000x xxxxxxxx xxxxxxxx  MAC to MR with data read to AX0 & pgm read to MY0 */
				mac_op_mr(adsp, op);
				adsp->core.ax0.u = data_read_dag1(adsp, op);
				adsp->core.my0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xe2: case 0xe3:
				/* 1110001x xxxxxxxx xxxxxxxx  ALU to AR with data read to AX0 & pgm read to MY0 */
				alu_op_ar(adsp, op);
				adsp->core.ax0.u = data_read_dag1(adsp, op);
				adsp->core.my0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xe4: case 0xe5:
				/* 1110010x xxxxxxxx xxxxxxxx  MAC to MR with data read to AX1 & pgm read to MY0 */
				mac_op_mr(adsp, op);
				adsp->core.ax1.u = data_read_dag1(adsp, op);
				adsp->core.my0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xe6: case 0xe7:
				/* 1110011x xxxxxxxx xxxxxxxx  ALU to AR with data read to AX1 & pgm read to MY0 */
				alu_op_ar(adsp, op);
				adsp->core.ax1.u = data_read_dag1(adsp, op);
				adsp->core.my0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xe8: case 0xe9:
				/* 1110100x xxxxxxxx xxxxxxxx  MAC to MR with data read to MX0 & pgm read to MY0 */
				mac_op_mr(adsp, op);
				adsp->core.mx0.u = data_read_dag1(adsp, op);
				adsp->core.my0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xea: case 0xeb:
				/* 1110101x xxxxxxxx xxxxxxxx  ALU to AR with data read to MX0 & pgm read to MY0 */
				alu_op_ar(adsp, op);
				adsp->core.mx0.u = data_read_dag1(adsp, op);
				adsp->core.my0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xec: case 0xed:
				/* 1110110x xxxxxxxx xxxxxxxx  MAC to MR with data read to MX1 & pgm read to MY0 */
				mac_op_mr(adsp, op);
				adsp->core.mx1.u = data_read_dag1(adsp, op);
				adsp->core.my0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xee: case 0xef:
				/* 1110111x xxxxxxxx xxxxxxxx  ALU to AR with data read to MX1 & pgm read to MY0 */
				alu_op_ar(adsp, op);
				adsp->core.mx1.u = data_read_dag1(adsp, op);
				adsp->core.my0.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xf0: case 0xf1:
				/* 1111000x xxxxxxxx xxxxxxxx  MAC to MR with data read to AX0 & pgm read to MY1 */
				mac_op_mr(adsp, op);
				adsp->core.ax0.u = data_read_dag1(adsp, op);
				adsp->core.my1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xf2: case 0xf3:
				/* 1111001x xxxxxxxx xxxxxxxx  ALU to AR with data read to AX0 & pgm read to MY1 */
				alu_op_ar(adsp, op);
				adsp->core.ax0.u = data_read_dag1(adsp, op);
				adsp->core.my1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xf4: case 0xf5:
				/* 1111010x xxxxxxxx xxxxxxxx  MAC to MR with data read to AX1 & pgm read to MY1 */
				mac_op_mr(adsp, op);
				adsp->core.ax1.u = data_read_dag1(adsp, op);
				adsp->core.my1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xf6: case 0xf7:
				/* 1111011x xxxxxxxx xxxxxxxx  ALU to AR with data read to AX1 & pgm read to MY1 */
				alu_op_ar(adsp, op);
				adsp->core.ax1.u = data_read_dag1(adsp, op);
				adsp->core.my1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xf8: case 0xf9:
				/* 1111100x xxxxxxxx xxxxxxxx  MAC to MR with data read to MX0 & pgm read to MY1 */
				mac_op_mr(adsp, op);
				adsp->core.mx0.u = data_read_dag1(adsp, op);
				adsp->core.my1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xfa: case 0xfb:
				/* 1111101x xxxxxxxx xxxxxxxx  ALU to AR with data read to MX0 & pgm read to MY1 */
				alu_op_ar(adsp, op);
				adsp->core.mx0.u = data_read_dag1(adsp, op);
				adsp->core.my1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xfc: case 0xfd:
				/* 1111110x xxxxxxxx xxxxxxxx  MAC to MR with data read to MX1 & pgm read to MY1 */
				mac_op_mr(adsp, op);
				adsp->core.mx1.u = data_read_dag1(adsp, op);
				adsp->core.my1.u = pgm_read_dag2(adsp, op >> 4);
				break;
			case 0xfe: case 0xff:
				/* 1111111x xxxxxxxx xxxxxxxx  ALU to AR with data read to MX1 & pgm read to MY1 */
				alu_op_ar(adsp, op);
				adsp->core.mx1.u = data_read_dag1(adsp, op);
				adsp->core.my1.u = pgm_read_dag2(adsp, op >> 4);
				break;
		}

		adsp->icount--;
	} while (adsp->icount > 0 && !adsp->end_run);

	cycles = cycles - adsp->icount;

	adsp->total_cycles += cycles;

	adsp->cycles_start = adsp->icount = 0;

	return cycles;
}



/***************************************************************************
    DEBUGGER DEFINITIONS
***************************************************************************/

//extern CPU_DISASSEMBLE( adsp21xx );



/***************************************************************************
    STATE HANDLING CALLBACKS
***************************************************************************/

#if 0
static CPU_IMPORT_STATE( adsp21xx )
{
    adsp2100_state *adsp = get_safe_token(device);
    switch (entry->index)
    {
		case ADSP2100_MSTAT:
			update_mstat(adsp);
			break;

		case ADSP2100_IMASK:
		case ADSP2100_ICNTL:
		case ADSP2100_IRQSTATE0:
		case ADSP2100_IRQSTATE1:
		case ADSP2100_IRQSTATE2:
		case ADSP2100_IRQSTATE3:
			check_irqs(adsp);
			break;

		case ADSP2100_I0:
		case ADSP2100_I1:
		case ADSP2100_I2:
		case ADSP2100_I3:
		case ADSP2100_I4:
		case ADSP2100_I5:
		case ADSP2100_I6:
		case ADSP2100_I7:
            update_i(adsp, entry->index - ADSP2100_I0);
			break;

		case ADSP2100_L0:
		case ADSP2100_L1:
		case ADSP2100_L2:
		case ADSP2100_L3:
		case ADSP2100_L4:
		case ADSP2100_L5:
		case ADSP2100_L6:
		case ADSP2100_L7:
          update_l(adsp, entry->index - ADSP2100_L0);
			break;

		default:
            fatalerror("CPU_IMPORT_STATE(adsp21xx) called for unexpected value\n");
			break;
	}
}



/**************************************************************************
 * Generic set_info
 **************************************************************************/
static CPU_SET_INFO( adsp21xx )
static void adsp21xx_set_info(UINT32 state, void *value)
{
	adsp2100_state *adsp = get_safe_token(device);
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + 0:
		case CPUINFO_INT_INPUT_STATE + 1:
		case CPUINFO_INT_INPUT_STATE + 2:
		case CPUINFO_INT_INPUT_STATE + 3:
		case CPUINFO_INT_INPUT_STATE + 4:
		case CPUINFO_INT_INPUT_STATE + 5:
		case CPUINFO_INT_INPUT_STATE + 6:
		case CPUINFO_INT_INPUT_STATE + 7:
		case CPUINFO_INT_INPUT_STATE + 8:
		case CPUINFO_INT_INPUT_STATE + 9:
			set_irq_line(adsp, state - CPUINFO_INT_INPUT_STATE, info->i);
			break;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static CPU_GET_INFO( adsp21xx )
{
	adsp2100_state *adsp = (device != NULL && device->token != NULL) ? get_safe_token(device) : NULL;
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(adsp2100_state);		break;
		case CPUINFO_INT_INPUT_LINES:					/* set per CPU */						break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case DEVINFO_INT_ENDIANNESS:					info->i = ENDIANNESS_LITTLE;			break;
		case CPUINFO_INT_CLOCK_MULTIPLIER:				info->i = 1;							break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 1;							break;

		case CPUINFO_INT_DATABUS_WIDTH_PROGRAM:			info->i = 32;							break;
		case CPUINFO_INT_ADDRBUS_WIDTH_PROGRAM: 		info->i = 14;							break;
		case CPUINFO_INT_ADDRBUS_SHIFT_PROGRAM: 		info->i = -2;							break;
		case CPUINFO_INT_DATABUS_WIDTH_DATA:			info->i = 16;							break;
		case CPUINFO_INT_ADDRBUS_WIDTH_DATA: 			info->i = 14;							break;
		case CPUINFO_INT_ADDRBUS_SHIFT_DATA: 			info->i = -1;							break;

		case CPUINFO_INT_INPUT_STATE + 0:
		case CPUINFO_INT_INPUT_STATE + 1:
		case CPUINFO_INT_INPUT_STATE + 2:
		case CPUINFO_INT_INPUT_STATE + 3:
		case CPUINFO_INT_INPUT_STATE + 4:
		case CPUINFO_INT_INPUT_STATE + 5:
		case CPUINFO_INT_INPUT_STATE + 6:
		case CPUINFO_INT_INPUT_STATE + 7:
		case CPUINFO_INT_INPUT_STATE + 8:
		case CPUINFO_INT_INPUT_STATE + 9:
			info->i = adsp->irq_state[state - CPUINFO_INT_INPUT_STATE];
			break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case CPUINFO_FCT_SET_INFO:		info->setinfo = CPU_SET_INFO_NAME(adsp21xx);			break;
		case CPUINFO_FCT_INIT:			/* set per CPU */										break;
		case CPUINFO_FCT_RESET:			info->reset = CPU_RESET_NAME(adsp21xx);					break;
		case CPUINFO_FCT_EXIT:			info->exit = CPU_EXIT_NAME(adsp21xx);					break;
		case CPUINFO_FCT_EXECUTE:		info->execute = CPU_EXECUTE_NAME(adsp21xx);				break;
		case CPUINFO_FCT_DISASSEMBLE:	info->disassemble = CPU_DISASSEMBLE_NAME(adsp21xx);		break;
		case CPUINFO_FCT_IMPORT_STATE:	info->import_state = CPU_IMPORT_STATE_NAME(adsp21xx);	break;

		/* --- the following bits of info are returned as pointers --- */
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &adsp->icount;			break;
		case CPUINFO_PTR_STATE_TABLE:					info->state_table = &adsp->state;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							/* set per CPU */						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "ADSP21xx");			break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "2.0");					break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);				break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Aaron Giles"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s, "%c%c%c%c%c%c%c%c",
				adsp->astat & 0x80 ? 'X':'.',
				adsp->astat & 0x40 ? 'M':'.',
				adsp->astat & 0x20 ? 'Q':'.',
				adsp->astat & 0x10 ? 'S':'.',
				adsp->astat & 0x08 ? 'C':'.',
				adsp->astat & 0x04 ? 'V':'.',
				adsp->astat & 0x02 ? 'N':'.',
				adsp->astat & 0x01 ? 'Z':'.');
			break;
	}
}
#endif

static void adsp21xx_load_boot_data(UINT8 *srcdata, UINT32 *dstdata)
{
	/* see how many words we need to copy */
	int pagelen = (srcdata[(3)] + 1) * 8;
	int i;
	for (i = 0; i < pagelen; i++)
	{
		UINT32 opcode = (srcdata[(i*4+0)] << 16) | (srcdata[(i*4+1)] << 8) | srcdata[(i*4+2)];
		dstdata[i] = opcode;
	}
}


/**************************************************************************
 * ADSP2100 section
 **************************************************************************/

//static CPU_INIT( adsp2100 )
void adsp2100_init(adsp2100_state *adsp, cpu_irq_callback irqcallback)
{
    adsp21xx_init(adsp, irqcallback, CHIP_TYPE_ADSP2100);
	adsp->mstat_mask = 0x0f;
	adsp->imask_mask = 0x0f;
}

#if 0
CPU_GET_INFO( adsp2100 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_LINES:					info->i = 4;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_FCT_INIT:			info->init = CPU_INIT_NAME(adsp2100);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "ADSP2100");			break;

		default:
			CPU_GET_INFO_CALL(adsp21xx);
			break;
	}
}
#endif
/**************************************************************************
 * ADSP2101 section
 **************************************************************************/

//static CPU_INIT( adsp2101 )
void adsp2101_init(adsp2100_state *adsp, cpu_irq_callback irqcallback)
{
    adsp21xx_init(adsp, irqcallback, CHIP_TYPE_ADSP2101);
	adsp->mstat_mask = 0x7f;
	adsp->imask_mask = 0x3f;
}

#if 0
CPU_GET_INFO( adsp2101 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_LINES:					info->i = 5;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_FCT_INIT:			info->init = CPU_INIT_NAME(adsp2101);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "ADSP2101");			break;

		default:
			CPU_GET_INFO_CALL(adsp21xx);
			break;
	}
}
#endif

/**************************************************************************
 * ADSP2104 section
 **************************************************************************/

//static CPU_INIT( adsp2104 )
void adsp2104_init(adsp2100_state *adsp, cpu_irq_callback irqcallback)
{
    adsp21xx_init(adsp, irqcallback, CHIP_TYPE_ADSP2104);
	adsp->mstat_mask = 0x7f;
	adsp->imask_mask = 0x3f;
}

void adsp2104_load_boot_data(UINT8 *srcdata, UINT32 *dstdata)
{
	adsp21xx_load_boot_data(srcdata, dstdata);
}

#if 0
CPU_GET_INFO( adsp2104 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_LINES:					info->i = 6;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_FCT_INIT:			info->init = CPU_INIT_NAME(adsp2104);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "ADSP2104");			break;

		default:
			CPU_GET_INFO_CALL(adsp21xx);
			break;
	}
}

#endif

/**************************************************************************
 * ADSP2105 section
 **************************************************************************/

//static CPU_INIT( adsp2105 )
void adsp2105_init(adsp2100_state *adsp, cpu_irq_callback irqcallback)
{
    adsp21xx_init(adsp, irqcallback, CHIP_TYPE_ADSP2105);
	adsp->mstat_mask = 0x7f;
	adsp->imask_mask = 0x3f;
}

void adsp2105_load_boot_data(UINT8 *srcdata, UINT32 *dstdata)
{
	adsp21xx_load_boot_data(srcdata, dstdata);
}

#if 0
CPU_GET_INFO( adsp2105 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_LINES:					info->i = 3;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_FCT_INIT:			info->init = CPU_INIT_NAME(adsp2105);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "ADSP2105");			break;

		default:
			CPU_GET_INFO_CALL(adsp21xx);
			break;
	}
}

#endif

/**************************************************************************
 * ADSP2115 section
 **************************************************************************/

//static CPU_INIT( adsp2115 )
void adsp2115_init(adsp2100_state *adsp, cpu_irq_callback irqcallback)
{
    adsp21xx_init(adsp, irqcallback, CHIP_TYPE_ADSP2115);
	adsp->mstat_mask = 0x7f;
	adsp->imask_mask = 0x3f;
}

void adsp2115_load_boot_data(UINT8 *srcdata, UINT32 *dstdata)
{
	adsp21xx_load_boot_data(srcdata, dstdata);
}

#if 0
CPU_GET_INFO( adsp2115 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_LINES:					info->i = 6;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_FCT_INIT:			info->init = CPU_INIT_NAME(adsp2115);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "ADSP2115");			break;

		default:
			CPU_GET_INFO_CALL(adsp21xx);
			break;
	}
}
#endif

/**************************************************************************
 * ADSP2181 section
 **************************************************************************/

//static CPU_INIT( adsp2181 )
void adsp2181_init(adsp2100_state *adsp, cpu_irq_callback irqcallback)
{
    adsp21xx_init(adsp, irqcallback, CHIP_TYPE_ADSP2181);
	adsp->mstat_mask = 0x7f;
	adsp->imask_mask = 0x3ff;
}

void adsp2181_load_boot_data(UINT8 *srcdata, UINT32 *dstdata)
{
	adsp21xx_load_boot_data(srcdata, dstdata);
}

#if 0
CPU_GET_INFO( adsp2181 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_LINES:					info->i = 9;							break;

		case CPUINFO_INT_DATABUS_WIDTH_IO:				info->i = 16;							break;
		case CPUINFO_INT_ADDRBUS_WIDTH_IO: 				info->i = 11;							break;
		case CPUINFO_INT_ADDRBUS_SHIFT_IO: 				info->i = -1;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_FCT_INIT:			info->init = CPU_INIT_NAME(adsp2181);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "ADSP2181");					break;

		default:
			CPU_GET_INFO_CALL(adsp21xx);
			break;
	}
}
#endif

void adsp2181_idma_addr_w(adsp2100_state *adsp, UINT16 data)
{
//	adsp2100_state *adsp = get_safe_token(device);
	adsp->idma_addr = data;
	adsp->idma_offs = 0;
}

UINT16 adsp2181_idma_addr_r(adsp2100_state *adsp)
{
//	adsp2100_state *adsp = get_safe_token(device);
	return adsp->idma_addr;
}

void adsp2181_idma_data_w(adsp2100_state *adsp, UINT16 data)
{
//	adsp2100_state *adsp = get_safe_token(device);

	/* program memory? */
	if (!(adsp->idma_addr & 0x4000))
	{
		/* upper 16 bits */
		if (adsp->idma_offs == 0)
		{
			adsp->idma_cache = data;
			adsp->idma_offs = 1;
		}

		/* lower 8 bits */
		else
		{
			WWORD_PGM(adsp, adsp->idma_addr++ & 0x3fff, (adsp->idma_cache << 8) | (data & 0xff));
			adsp->idma_offs = 0;
		}
	}

	/* data memory */
	else
		WWORD_DATA(adsp, adsp->idma_addr++ & 0x3fff, data);
}

UINT16 adsp2181_idma_data_r(adsp2100_state *adsp)
{
	UINT16 result = 0xffff;

	/* program memory? */
	if (!(adsp->idma_addr & 0x4000))
	{
		/* upper 16 bits */
		if (adsp->idma_offs == 0)
		{
			result = RWORD_PGM(adsp, adsp->idma_addr & 0x3fff) >> 8;
			adsp->idma_offs = 1;
		}

		/* lower 8 bits */
		else
		{
			result = RWORD_PGM(adsp, adsp->idma_addr++ & 0x3fff) & 0xff;
			adsp->idma_offs = 0;
		}
	}

	/* data memory */
	else
		result = RWORD_DATA(adsp, adsp->idma_addr++ & 0x3fff);

	return result;
}
