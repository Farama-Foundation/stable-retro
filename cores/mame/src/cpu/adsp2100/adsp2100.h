/*###################################################################################################
**
**
**		ADSP2100.h
**		Interface file for the portable Analog ADSP-2100 emulator.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef _ADSP2100_H
#define _ADSP2100_H

#include "memory.h"
#include "osd_cpu.h"


/*###################################################################################################
**	GLOBAL CONSTANTS
**#################################################################################################*/

#define ADSP2100_DATA_OFFSET	0x00000
#define ADSP2100_PGM_OFFSET		0x10000
#define ADSP2100_SIZE			0x20000

/* transmit and receive data callbacks types */
typedef INT32 (*RX_CALLBACK)( int port );
typedef void  (*TX_CALLBACK)( int port, INT32 data );


/*###################################################################################################
**	MEMORY MAP MACROS
**#################################################################################################*/

#define ADSP_DATA_ADDR_RANGE(start, end) (ADSP2100_DATA_OFFSET + ((start) << 1)), (ADSP2100_DATA_OFFSET + ((end) << 1) + 1)
#define ADSP_PGM_ADDR_RANGE(start, end)  (ADSP2100_PGM_OFFSET + ((start) << 2)), (ADSP2100_PGM_OFFSET + ((end) << 2) + 3)


/*###################################################################################################
**	REGISTER ENUMERATION
**#################################################################################################*/

enum
{
	ADSP2100_PC=1,
	ADSP2100_AX0, ADSP2100_AX1, ADSP2100_AY0, ADSP2100_AY1, ADSP2100_AR, ADSP2100_AF,
	ADSP2100_MX0, ADSP2100_MX1, ADSP2100_MY0, ADSP2100_MY1, ADSP2100_MR0, ADSP2100_MR1, ADSP2100_MR2, ADSP2100_MF,
	ADSP2100_SI, ADSP2100_SE, ADSP2100_SB, ADSP2100_SR0, ADSP2100_SR1,
	ADSP2100_I0, ADSP2100_I1, ADSP2100_I2, ADSP2100_I3, ADSP2100_I4, ADSP2100_I5, ADSP2100_I6, ADSP2100_I7,
	ADSP2100_L0, ADSP2100_L1, ADSP2100_L2, ADSP2100_L3, ADSP2100_L4, ADSP2100_L5, ADSP2100_L6, ADSP2100_L7,
	ADSP2100_M0, ADSP2100_M1, ADSP2100_M2, ADSP2100_M3, ADSP2100_M4, ADSP2100_M5, ADSP2100_M6, ADSP2100_M7,
	ADSP2100_PX, ADSP2100_CNTR, ADSP2100_ASTAT, ADSP2100_SSTAT, ADSP2100_MSTAT,
	ADSP2100_PCSP, ADSP2100_CNTRSP, ADSP2100_STATSP, ADSP2100_LOOPSP,
	ADSP2100_IMASK, ADSP2100_ICNTL, ADSP2100_IRQSTATE0, ADSP2100_IRQSTATE1, ADSP2100_IRQSTATE2, ADSP2100_IRQSTATE3,
	ADSP2100_FLAGIN, ADSP2100_FLAGOUT, ADSP2100_FL0, ADSP2100_FL1, ADSP2100_FL2,
	ADSP2100_AX0_SEC, ADSP2100_AX1_SEC, ADSP2100_AY0_SEC, ADSP2100_AY1_SEC, ADSP2100_AR_SEC, ADSP2100_AF_SEC,
	ADSP2100_MX0_SEC, ADSP2100_MX1_SEC, ADSP2100_MY0_SEC, ADSP2100_MY1_SEC, ADSP2100_MR0_SEC, ADSP2100_MR1_SEC, ADSP2100_MR2_SEC, ADSP2100_MF_SEC,
	ADSP2100_SI_SEC, ADSP2100_SE_SEC, ADSP2100_SB_SEC, ADSP2100_SR0_SEC, ADSP2100_SR1_SEC
};


/*###################################################################################################
**	INTERRUPT CONSTANTS
**#################################################################################################*/

#define ADSP2100_IRQ0		0		/* IRQ0 */
#define ADSP2100_SPORT1_RX	0		/* SPORT1 receive IRQ */
#define ADSP2100_IRQ1		1		/* IRQ1 */
#define ADSP2100_SPORT1_TX	1		/* SPORT1 transmit IRQ */
#define ADSP2100_IRQ2		2		/* IRQ2 */
#define ADSP2100_IRQ3		3		/* IRQ3 */


/*###################################################################################################
**	PUBLIC GLOBALS
**#################################################################################################*/

extern int adsp2100_icount;


/*###################################################################################################
**	PUBLIC FUNCTIONS
**#################################################################################################*/

extern void adsp2100_init(void);
extern void adsp2100_reset(void *param);
extern void adsp2100_exit(void);
extern int adsp2100_execute(int cycles);
extern unsigned adsp2100_get_context(void *dst);
extern void adsp2100_set_context(void *src);
extern unsigned adsp2100_get_reg(int regnum);
extern void adsp2100_set_reg(int regnum, unsigned val);
extern void adsp2100_set_irq_line(int irqline, int state);
extern void adsp2100_set_irq_callback(int (*callback)(int irqline));
extern const char *adsp2100_info(void *context, int regnum);
extern unsigned adsp2100_dasm(char *buffer, unsigned pc);


/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
#define ADSP2100_RDMEM(A) ((unsigned)cpu_readmem17lew(A))
#define ADSP2100_RDMEM_WORD(A) ((unsigned)cpu_readmem17lew_word(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define ADSP2100_WRMEM(A,V) (cpu_writemem17lew(A,V))
#define ADSP2100_WRMEM_WORD(A,V) (cpu_writemem17lew_word(A,V))

/****************************************************************************/
/* Write a 24-bit value to program memory                                   */
/****************************************************************************/
#define ADSP2100_WRPGM(A,V)	(*(UINT32 *)(A) = (V) & 0xffffff)


#ifdef MAME_DEBUG
extern unsigned DasmADSP2100(char *buffer, unsigned pc);
#endif

#if (HAS_ADSP2101)
/**************************************************************************
 * ADSP2101 section
 **************************************************************************/

#define ADSP2101_DATA_OFFSET    ADSP2100_DATA_OFFSET
#define ADSP2101_PGM_OFFSET     ADSP2100_PGM_OFFSET
#define ADSP2101_SIZE           ADSP2100_SIZE

#define adsp2101_icount adsp2100_icount

#define ADSP2101_IRQ0		0		/* IRQ0 */
#define ADSP2101_SPORT1_RX	0		/* SPORT1 receive IRQ */
#define ADSP2101_IRQ1		1		/* IRQ1 */
#define ADSP2101_SPORT1_TX	1		/* SPORT1 transmit IRQ */
#define ADSP2101_IRQ2		2		/* IRQ2 */
#define ADSP2101_SPORT0_RX	3		/* SPORT0 receive IRQ */
#define ADSP2101_SPORT0_TX	4		/* SPORT0 transmit IRQ */

extern void adsp2101_init(void);
extern void adsp2101_reset(void *param);
extern void adsp2101_exit(void);
extern int adsp2101_execute(int cycles);
extern unsigned adsp2101_get_context(void *dst);
extern void adsp2101_set_context(void *src);
extern unsigned adsp2101_get_reg(int regnum);
extern void adsp2101_set_reg(int regnum, unsigned val);
extern void adsp2101_set_irq_line(int irqline, int state);
extern void adsp2101_set_irq_callback(int (*callback)(int irqline));
extern const char *adsp2101_info(void *context, int regnum);
extern unsigned adsp2101_dasm(char *buffer, unsigned pc);
extern void adsp2101_set_rx_callback( RX_CALLBACK cb );
extern void adsp2101_set_tx_callback( TX_CALLBACK cb );
#endif

#if (HAS_ADSP2104)
/**************************************************************************
 * ADSP2104 section
 **************************************************************************/

#define ADSP2104_DATA_OFFSET    ADSP2100_DATA_OFFSET
#define ADSP2104_PGM_OFFSET     ADSP2100_PGM_OFFSET
#define ADSP2104_SIZE           ADSP2100_SIZE

#define adsp2104_icount adsp2100_icount

#define ADSP2104_IRQ0		0		/* IRQ0 */
#define ADSP2104_SPORT1_RX	0		/* SPORT1 receive IRQ */
#define ADSP2104_IRQ1		1		/* IRQ1 */
#define ADSP2104_SPORT1_TX	1		/* SPORT1 transmit IRQ */
#define ADSP2104_IRQ2		2		/* IRQ2 */
#define ADSP2104_SPORT0_RX	3		/* SPORT0 receive IRQ */
#define ADSP2104_SPORT0_TX	4		/* SPORT0 transmit IRQ */

extern void adsp2104_init(void);
extern void adsp2104_reset(void *param);
extern void adsp2104_exit(void);
extern int adsp2104_execute(int cycles);
extern unsigned adsp2104_get_context(void *dst);
extern void adsp2104_set_context(void *src);
extern unsigned adsp2104_get_reg(int regnum);
extern void adsp2104_set_reg(int regnum, unsigned val);
extern void adsp2104_set_irq_line(int irqline, int state);
extern void adsp2104_set_irq_callback(int (*callback)(int irqline));
extern const char *adsp2104_info(void *context, int regnum);
extern unsigned adsp2104_dasm(char *buffer, unsigned pc);
extern void adsp2104_set_rx_callback( RX_CALLBACK cb );
extern void adsp2104_set_tx_callback( TX_CALLBACK cb );

extern void adsp2104_load_boot_data(data8_t *srcdata, data32_t *dstdata);
#endif

#if (HAS_ADSP2105)
/**************************************************************************
 * ADSP2105 section
 **************************************************************************/

#define ADSP2105_DATA_OFFSET    ADSP2100_DATA_OFFSET
#define ADSP2105_PGM_OFFSET     ADSP2100_PGM_OFFSET
#define ADSP2105_SIZE           ADSP2100_SIZE

#define adsp2105_icount adsp2100_icount

#define ADSP2105_IRQ0		0		/* IRQ0 */
#define ADSP2105_SPORT1_RX	0		/* SPORT1 receive IRQ */
#define ADSP2105_IRQ1		1		/* IRQ1 */
#define ADSP2105_SPORT1_TX	1		/* SPORT1 transmit IRQ */
#define ADSP2105_IRQ2		2		/* IRQ2 */

extern void adsp2105_init(void);
extern void adsp2105_reset(void *param);
extern void adsp2105_exit(void);
extern int adsp2105_execute(int cycles);
extern unsigned adsp2105_get_context(void *dst);
extern void adsp2105_set_context(void *src);
extern unsigned adsp2105_get_reg(int regnum);
extern void adsp2105_set_reg(int regnum, unsigned val);
extern void adsp2105_set_irq_line(int irqline, int state);
extern void adsp2105_set_irq_callback(int (*callback)(int irqline));
extern const char *adsp2105_info(void *context, int regnum);
extern unsigned adsp2105_dasm(char *buffer, unsigned pc);
extern void adsp2105_set_rx_callback( RX_CALLBACK cb );
extern void adsp2105_set_tx_callback( TX_CALLBACK cb );

extern void adsp2105_load_boot_data(data8_t *srcdata, data32_t *dstdata);
#endif

#if (HAS_ADSP2115)
/**************************************************************************
 * ADSP2115 section
 **************************************************************************/

#define ADSP2115_DATA_OFFSET    ADSP2100_DATA_OFFSET
#define ADSP2115_PGM_OFFSET     ADSP2100_PGM_OFFSET
#define ADSP2115_SIZE           ADSP2100_SIZE

#define adsp2115_icount adsp2100_icount

#define ADSP2115_IRQ0		0		/* IRQ0 */
#define ADSP2115_SPORT1_RX	0		/* SPORT1 receive IRQ */
#define ADSP2115_IRQ1		1		/* IRQ1 */
#define ADSP2115_SPORT1_TX	1		/* SPORT1 transmit IRQ */
#define ADSP2115_IRQ2		2		/* IRQ2 */
#define ADSP2115_SPORT0_RX	3		/* SPORT0 receive IRQ */
#define ADSP2115_SPORT0_TX	4		/* SPORT0 transmit IRQ */

extern void adsp2115_init(void);
extern void adsp2115_reset(void *param);
extern void adsp2115_exit(void);
extern int adsp2115_execute(int cycles);
extern unsigned adsp2115_get_context(void *dst);
extern void adsp2115_set_context(void *src);
extern unsigned adsp2115_get_reg(int regnum);
extern void adsp2115_set_reg(int regnum, unsigned val);
extern void adsp2115_set_irq_line(int irqline, int state);
extern void adsp2115_set_irq_callback(int (*callback)(int irqline));
extern const char *adsp2115_info(void *context, int regnum);
extern unsigned adsp2115_dasm(char *buffer, unsigned pc);
extern void adsp2115_set_rx_callback( RX_CALLBACK cb );
extern void adsp2115_set_tx_callback( TX_CALLBACK cb );

extern void adsp2115_load_boot_data(data8_t *srcdata, data32_t *dstdata);
#endif

#endif /* _ADSP2100_H */
