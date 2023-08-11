#ifndef __M37710_H__
#define __M37710_H__

/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

M37710 CPU Emulator v0.1

*/

/* ======================================================================== */
/* =============================== DEFINES ================================ */
/* ======================================================================== */
/*
   Input lines - used with cpunum_set_input_line() and the like.
   WARNING: these are in the same order as the vector table for simplicity.
   Do not alter this order!
*/

#define M37702 (1 << 0)
#define M37710 (1 << 1)

enum
{
	// these interrupts are maskable
	M37710_LINE_ADC = 0,
	M37710_LINE_UART1XMIT,
	M37710_LINE_UART1RECV,
	M37710_LINE_UART0XMIT,
	M37710_LINE_UART0RECV,
	M37710_LINE_TIMERB2,
	M37710_LINE_TIMERB1,
	M37710_LINE_TIMERB0,
	M37710_LINE_TIMERA4,
	M37710_LINE_TIMERA3,
	M37710_LINE_TIMERA2,
	M37710_LINE_TIMERA1,
	M37710_LINE_TIMERA0,  // 12 / 0xC
	M37710_LINE_IRQ2,
	M37710_LINE_IRQ1,
	M37710_LINE_IRQ0,     // 15 / 0xf
	// these interrupts are non-maskable
	M37710_LINE_WATCHDOG,
	M37710_LINE_DEBUG,
	M37710_LINE_BRK,
	M37710_LINE_ZERODIV,
	M37710_LINE_RESET,

	// these are not interrupts, they're signals external hardware can send
	M37710_LINE_TIMERA0TICK,
	M37710_LINE_TIMERA1TICK,
	M37710_LINE_TIMERA2TICK,
	M37710_LINE_TIMERA3TICK,
	M37710_LINE_TIMERA4TICK,
	M37710_LINE_TIMERB0TICK,
	M37710_LINE_TIMERB1TICK,
	M37710_LINE_TIMERB2TICK,

	M37710_LINE_MAX
};

/* Registers - used by m37710_set_reg() and m37710_get_reg() */
enum
{
	M37710_PC=1, M37710_S, M37710_P, M37710_A, M37710_B, M37710_X, M37710_Y,
	M37710_PB, M37710_DB, M37710_D, M37710_E,
	M37710_NMI_STATE, M37710_IRQ_STATE, STATE_GENPCBASE
};


/* I/O ports */
enum
{
	M37710_PORT0 = 0,
	M37710_PORT1, M37710_PORT2, M37710_PORT3, M37710_PORT4,
	M37710_PORT5, M37710_PORT6, M37710_PORT7, M37710_PORT8,

	M37710_ADC0_L = 0x10, M37710_ADC0_H,
	M37710_ADC1_L, M37710_ADC1_H, M37710_ADC2_L, M37710_ADC2_H, M37710_ADC3_L, M37710_ADC3_H,
	M37710_ADC4_L, M37710_ADC4_H, M37710_ADC5_L, M37710_ADC5_H, M37710_ADC6_L, M37710_ADC6_H,
	M37710_ADC7_L, M37710_ADC7_H,

	M37710_SER0_REC = 0x20,
	M37710_SER0_XMIT, M37710_SER1_REC, M37710_SER1_XMIT
};


#endif /* __M37710_H__ */
