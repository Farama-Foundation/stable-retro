/***********************************************************************

	DECO Cassette System machine

 ***********************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "cpu/i8x41/i8x41.h"
#include "machine/decocass.h"
#include "state.h"

/* tape direction, speed and timing (used also in vidhrdw/decocass.c) */
int tape_dir;
int tape_speed;
double tape_time0;
void *tape_timer;

static int firsttime = 1;
static int tape_present;
static int tape_blocks;
static int tape_length;
static int tape_bot_eot;
static UINT8 crc16_lsb;
static UINT8 crc16_msb;

/* pre-calculated crc16 of the tape blocks */
static UINT8 tape_crc16_lsb[256];
static UINT8 tape_crc16_msb[256];

static data8_t (*decocass_dongle_r)(offs_t offset);
static void (*decocass_dongle_w)(offs_t offset, data8_t data);

static data8_t decocass_reset;
static data8_t i8041_p1;
static data8_t i8041_p2;

/* dongle type #1: jumpers C and D assignments */
#define MAKE_MAP(m0,m1,m2,m3,m4,m5,m6,m7)		\
	((m0)<<0)|((m1)<<4)|((m2)<<8)|((m3)<<12)|	\
	((m4)<<16)|((m5)<<20)|((m6)<<24)|((m7)<<28)
#define MAP0(m) ((m>>0)&15)
#define MAP1(m) ((m>>4)&15)
#define MAP2(m) ((m>>8)&15)
#define MAP3(m) ((m>>12)&15)
#define MAP4(m) ((m>>16)&15)
#define MAP5(m) ((m>>20)&15)
#define MAP6(m) ((m>>24)&15)
#define MAP7(m) ((m>>28)&15)
static UINT32 type1_inmap;
static UINT32 type1_outmap;

/* dongle type #2: status of the latches */
static int type2_d2_latch;	/* latched 8041-STATUS D2 value */
static int type2_xx_latch;	/* latched value (D7-4 == 0xc0) ? 1 : 0 */
static int type2_promaddr;	/* latched PROM address A0-A7 */

/* dongle type #3: status and patches */
static int type3_ctrs;			/* 12 bit counter stage */
static int type3_d0_latch;		/* latched 8041-D0 value */
static int type3_pal_19;		/* latched 1 for PAL input pin-19 */
static int type3_swap;
enum {
	TYPE3_SWAP_01,
	TYPE3_SWAP_12,
	TYPE3_SWAP_13,
	TYPE3_SWAP_23_56,
	TYPE3_SWAP_24,
	TYPE3_SWAP_25,
	TYPE3_SWAP_34_0,
	TYPE3_SWAP_34_7,
	TYPE3_SWAP_45,
	TYPE3_SWAP_56,
	TYPE3_SWAP_67
};

/* dongle type #4: status */
static int type4_ctrs;			/* latched PROM address (E5x0 LSB, E5x1 MSB) */
static int type4_latch; 		/* latched enable PROM (1100xxxx written to E5x1) */

/* dongle type #5: status */
static int type5_latch; 		/* latched enable PROM (1100xxxx written to E5x1) */

/* four inputs from the quadrature decoder (H1, V1, H2, V2) */
static data8_t decocass_quadrature_decoder[4];

/* sound latches, ACK status bits and NMI timer */
static data8_t decocass_sound_ack;
static void *decocass_sound_timer;

WRITE_HANDLER( decocass_coin_counter_w )
{
}

WRITE_HANDLER( decocass_sound_command_w )
{
	LOG(2,("CPU #%d sound command -> $%02x\n", cpu_getactivecpu(), data));
	soundlatch_w(0,data);
	decocass_sound_ack |= 0x80;
	/* remove snd cpu data ack bit. i don't see it in the schems, but... */
	decocass_sound_ack &= ~0x40;
	cpu_set_irq_line(1, M6502_IRQ_LINE, ASSERT_LINE);
}

READ_HANDLER( decocass_sound_data_r )
{
	data8_t data = soundlatch2_r(0);
	LOG(2,("CPU #%d sound data    <- $%02x\n", cpu_getactivecpu(), data));
	return data;
}

READ_HANDLER( decocass_sound_ack_r )
{
	data8_t data = decocass_sound_ack;	/* D6+D7 */
	LOG(2,("CPU #%d sound ack     <- $%02x\n", cpu_getactivecpu(), data));
	return data;
}

WRITE_HANDLER( decocass_sound_data_w )
{
	LOG(2,("CPU #%d sound data    -> $%02x\n", cpu_getactivecpu(), data));
	soundlatch2_w(0, data);
	decocass_sound_ack |= 0x40;
}

READ_HANDLER( decocass_sound_command_r )
{
	data8_t data = soundlatch_r(0);
	LOG(2,("CPU #%d sound command <- $%02x\n", cpu_getactivecpu(), data));
	cpu_set_irq_line(1, M6502_IRQ_LINE, CLEAR_LINE);
	decocass_sound_ack &= ~0x80;
	return data;
}

static void decocass_sound_nmi_pulse( int param )
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

WRITE_HANDLER( decocass_sound_nmi_enable_w )
{
	LOG(2,("CPU #%d sound NMI enb -> $%02x\n", cpu_getactivecpu(), data));
	timer_adjust(decocass_sound_timer, TIME_IN_HZ(256 * 57 / 8 / 2), 0, TIME_IN_HZ(256 * 57 / 8 / 2));
}

READ_HANDLER( decocass_sound_nmi_enable_r )
{
	data8_t data = 0xff;
	LOG(2,("CPU #%d sound NMI enb <- $%02x\n", cpu_getactivecpu(), data));
	timer_adjust(decocass_sound_timer, TIME_IN_HZ(256 * 57 / 8 / 2), 0, TIME_IN_HZ(256 * 57 / 8 / 2));
	return data;
}

READ_HANDLER( decocass_sound_data_ack_reset_r )
{
	data8_t data = 0xff;
	LOG(2,("CPU #%d sound ack rst <- $%02x\n", cpu_getactivecpu(), data));
	decocass_sound_ack &= ~0x40;
	return data;
}

WRITE_HANDLER( decocass_sound_data_ack_reset_w )
{
	LOG(2,("CPU #%d sound ack rst -> $%02x\n", cpu_getactivecpu(), data));
	decocass_sound_ack &= ~0x40;
}

WRITE_HANDLER( decocass_nmi_reset_w )
{
	cpu_set_nmi_line( 0, CLEAR_LINE );
}

WRITE_HANDLER( decocass_quadrature_decoder_reset_w )
{
	/* just latch the analog controls here */
	decocass_quadrature_decoder[0] = input_port_3_r(0);
	decocass_quadrature_decoder[1] = input_port_4_r(0);
	decocass_quadrature_decoder[2] = input_port_5_r(0);
	decocass_quadrature_decoder[3] = input_port_6_r(0);
}

WRITE_HANDLER( decocass_adc_w )
{
}

/*
 * E6x0    inputs
 * E6x1    inputs
 * E6x2    coin inp
 * E6x3    quadrature decoder read
 * E6x4    ""
 * E6x5    ""
 * E6x6    ""
 * E6x7    a/d converter read
 */
READ_HANDLER( decocass_input_r )
{
	data8_t data = 0xff;
	switch (offset & 7)
	{
	case 0: case 1: case 2:
		data = readinputport(offset & 7);
		break;
	case 3: case 4: case 5: case 6:
		data = decocass_quadrature_decoder[(offset & 7) - 3];
		break;
	default:
		break;
	}

	return data;
}

/*
 * D0 - REQ/ data request	  (8041 pin 34 port 1.7)
 * D1 - FNO/ function number  (8041 pin 21 port 2.0)
 * D2 - EOT/ end-of-tape	  (8041 pin 22 port 2.1)
 * D3 - ERR/ error condition  (8041 pin 23 port 2.2)
 * D4 - BOT-EOT from tape
 * D5 -
 * D6 -
 * D7 - cassette present
 */
/* Note on a tapes leader-BOT-data-EOT-trailer format:
 * A cassette has a transparent piece of tape on both ends,
 * leader and trailer. And data tapes also have BOT and EOT
 * holes, shortly before the the leader and trailer.
 * The holes and clear tape are detected using a photo-resitor.
 * When rewinding, the BOT/EOT signal will show a short
 * pulse and if rewind continues a constant high signal later.
 * The specs say the holes are "> 2ms" in length.
 */

#define TAPE_CLOCKRATE	4800	/* clock pulses per second */

/* duration of the clear LEADER (and trailer) of the tape */
#define TAPE_LEADER 	TAPE_CLOCKRATE		/* 1s */
/* duration of the GAP between leader and BOT/EOT */
#define TAPE_GAP		TAPE_CLOCKRATE*3/2	/* 1.5s */
/* duration of BOT/EOT holes */
#define TAPE_HOLE		TAPE_CLOCKRATE/400	/* 0.0025s */

/* byte offset of the tape chunks (8 clocks per byte = 16 samples) */
/* 300 ms GAP between BOT and first data block (doesn't work.. thus /2) */
#define TAPE_PRE_GAP	34
#define TAPE_LEADIN 	(TAPE_PRE_GAP + 1)
#define TAPE_HEADER 	(TAPE_LEADIN + 1)
#define TAPE_BLOCK		(TAPE_HEADER + 256)
#define TAPE_CRC16_MSB	(TAPE_BLOCK + 1)
#define TAPE_CRC16_LSB	(TAPE_CRC16_MSB + 1)
#define TAPE_TRAILER	(TAPE_CRC16_LSB + 1)
#define TAPE_LEADOUT	(TAPE_TRAILER + 1)
#define TAPE_LONGCLOCK	(TAPE_LEADOUT + 1)
#define TAPE_POST_GAP	(TAPE_LONGCLOCK + 34)

/* size of a tape chunk (block) including gaps */
#define TAPE_CHUNK		TAPE_POST_GAP

#define E5XX_MASK	0x02	/* use 0x0e for old style board */

#define BIT0(x) ((x)&1)
#define BIT1(x) (((x)>>1)&1)
#define BIT2(x) (((x)>>2)&1)
#define BIT3(x) (((x)>>3)&1)
#define BIT4(x) (((x)>>4)&1)
#define BIT5(x) (((x)>>5)&1)
#define BIT6(x) (((x)>>6)&1)
#define BIT7(x) (((x)>>7)&1)

WRITE_HANDLER( decocass_reset_w )
{
	LOG(1,("%9.7f 6502-PC: %04x decocass_reset_w(%02x): $%02x\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
	decocass_reset = data;

	/* CPU #1 active hight reset */
	cpu_set_reset_line( 1, data & 0x01 );

	/* on reset also remove the sound timer */
	if (data & 1)
		timer_adjust(decocass_sound_timer, TIME_NEVER, 0, 0);

	/* 8041 active low reset */
	cpu_set_reset_line( 2, (data & 0x08) ^ 0x08 );
}

#ifdef MAME_DEBUG
static const char *dirnm(int speed)
{
	if (speed <  -1) return "fast rewind";
	if (speed == -1) return "rewind";
	if (speed ==  0) return "stop";
	if (speed ==  1) return "forward";
	return "fast forward";
}
#endif

static void tape_crc16(UINT8 data)
{
	UINT8 c0, c1;
	UINT8 old_lsb = crc16_lsb;
	UINT8 old_msb = crc16_msb;
	UINT8 feedback;

	feedback = ((data >> 7) ^ crc16_msb) & 1;

	/* rotate 16 bits */
	c0 = crc16_lsb & 1;
	c1 = crc16_msb & 1;
	crc16_msb = (crc16_msb >> 1) | (c0 << 7);
	crc16_lsb = (crc16_lsb >> 1) | (c1 << 7);

	/* feedback into bit 7 */
	if (feedback)
		crc16_lsb |= 0x80;
	else
		crc16_lsb &= ~0x80;

	/* feedback to bit 6 into bit 5 */
	if (((old_lsb >> 6) ^ feedback) & 1)
		crc16_lsb |= 0x20;
	else
		crc16_lsb &= ~0x20;

	/* feedback to bit 1 into bit 0 */
	if (((old_msb >> 1) ^ feedback) & 1)
		crc16_msb |= 0x01;
	else
		crc16_msb &= ~0x01;
}

static void tape_update(void)
{
	static int last_byte;
	double tape_time = tape_time0;
	int offset, rclk, rdata, tape_bit, tape_byte, tape_block;

	if (tape_timer)
		tape_time += tape_dir * timer_timeelapsed(tape_timer);

	if (tape_time < 0.0)
		tape_time = 0.0;
	else if (tape_time > 999.9)
		tape_time = 999.9;

	offset = (int)(tape_time * TAPE_CLOCKRATE + 0.499995);

	/* reset RCLK and RDATA inputs */
	rclk = 0;
	rdata = 0;

	if (offset < TAPE_LEADER)
	{
		if (offset < 0)
			offset = 0;
		/* LEADER area */
		if (0 == tape_bot_eot)
		{
			tape_bot_eot = 1;
			set_led_status(1, 1);
			LOG(5,("tape %5.4fs: %s found LEADER\n", tape_time, dirnm(tape_dir)));
		}
	}
	else
	if (offset < TAPE_LEADER + TAPE_GAP)
	{
		/* GAP between LEADER and BOT hole */
		if (1 == tape_bot_eot)
		{
			tape_bot_eot = 0;
			set_led_status(1, 0);
			LOG(5,("tape %5.4fs: %s between BOT + LEADER\n", tape_time, dirnm(tape_dir)));
		}
	}
	else
	if (offset < TAPE_LEADER + TAPE_GAP + TAPE_HOLE)
	{
		/* during BOT hole */
		if (0 == tape_bot_eot)
		{
			tape_bot_eot = 1;
			set_led_status(1, 1);
			LOG(5,("tape %5.4fs: %s found BOT\n", tape_time, dirnm(tape_dir)));
		}
	}
	else
	if (offset < tape_length - TAPE_LEADER - TAPE_GAP - TAPE_HOLE)
	{
		offset -= TAPE_LEADER + TAPE_GAP + TAPE_HOLE;

		/* data area */
		if (1 == tape_bot_eot)
		{
			tape_bot_eot = 0;
			set_led_status(1, 0);
			LOG(5,("tape %5.4fs: %s data area\n", tape_time, dirnm(tape_dir)));
		}
		rclk = (offset ^ 1) & 1;
		tape_bit = (offset / 2) % 8;
		tape_byte = (offset / 16) % TAPE_CHUNK;
		tape_block = offset / 16 / TAPE_CHUNK;

		if (tape_byte < TAPE_PRE_GAP)
		{
			rclk = 0;
			rdata = 0;
		}
		else
		if (tape_byte < TAPE_LEADIN)
		{
			rdata = (0x00 >> tape_bit) & 1;
			if (tape_byte != last_byte)
			{
				LOG(5,("tape %5.4fs: LEADIN $00\n", tape_time));
				set_led_status(2, 1);
			}
		}
		else
		if (tape_byte < TAPE_HEADER)
		{
			rdata = (0xaa >> tape_bit) & 1;
			if (tape_byte != last_byte)
				LOG(5,("tape %5.4fs: HEADER $aa\n", tape_time));
		}
		else
		if (tape_byte < TAPE_BLOCK)
		{
			data8_t *ptr = memory_region(REGION_USER2) + tape_block * 256 + tape_byte - TAPE_HEADER;
			rdata = (*ptr >> tape_bit) & 1;
			if (tape_byte != last_byte)
				LOG(4,("tape %5.4fs: DATA(%02x) $%02x\n", tape_time, tape_byte - TAPE_HEADER, *ptr));
		}
		else
		if (tape_byte < TAPE_CRC16_MSB)
		{
			rdata = (tape_crc16_msb[tape_block] >> tape_bit) & 1;
			if (tape_byte != last_byte)
				LOG(4,("tape %5.4fs: CRC16 MSB $%02x\n", tape_time, tape_crc16_msb[tape_block]));
		}
		else
		if (tape_byte < TAPE_CRC16_LSB)
		{
			rdata = (tape_crc16_lsb[tape_block] >> tape_bit) & 1;
			if (tape_byte != last_byte)
				LOG(4,("tape %5.4fs: CRC16 LSB $%02x\n", tape_time, tape_crc16_lsb[tape_block]));
		}
		else
		if (tape_byte < TAPE_TRAILER)
		{
			rdata = (0xaa >> tape_bit) & 1;
			if (tape_byte != last_byte)
				LOG(4,("tape %5.4fs: TRAILER $aa\n", tape_time));
		}
		else
		if (tape_byte < TAPE_LEADOUT)
		{
			rdata = (0x00 >> tape_bit) & 1;
			if (tape_byte != last_byte)
				LOG(4,("tape %5.4fs: LEADOUT $00\n", tape_time));
		}
		else
		if (tape_byte < TAPE_LONGCLOCK)
		{
			if (tape_byte != last_byte)
			{
				LOG(4,("tape %5.4fs: LONG CLOCK\n", tape_time));
				set_led_status(2, 0);
			}
			rclk = 1;
			rdata = 0;
		}
		last_byte = tape_byte;
	}
	else
	if (offset < tape_length - TAPE_LEADER - TAPE_GAP)
	{
		/* during EOT hole */
		if (0 == tape_bot_eot)
		{
			tape_bot_eot = 1;
			set_led_status(1, 1);
			LOG(5,("tape %5.4fs: %s found EOT\n", tape_time, dirnm(tape_dir)));
		}
	}
	else
	if (offset < tape_length - TAPE_LEADER)
	{
		/* GAP between EOT and trailer */
		if (1 == tape_bot_eot)
		{
			tape_bot_eot = 0;
			set_led_status(1, 0);
			LOG(5,("tape %5.4fs: %s EOT and TRAILER\n", tape_time, dirnm(tape_dir)));
		}
	}
	else
	{
		/* TRAILER area */
		if (0 == tape_bot_eot)
		{
			tape_bot_eot = 1;
			set_led_status(1, 1);
			LOG(5,("tape %5.4fs: %s found TRAILER\n", tape_time, dirnm(tape_dir)));
		}
		offset = tape_length - 1;
	}

	i8041_p2 = (i8041_p2 & ~0xe0) | (tape_bot_eot << 5) | (rclk << 6) | (rdata << 7);
}

#ifdef MAME_DEBUG
static void decocass_fno(offs_t offset, data8_t data)
{
		/* 8041ENA/ and is this a FNO write (function number)? */
		if (0 == (i8041_p2 & 0x01))
		{
			switch (data)
			{
			case 0x25: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 25: write_block\n"); break;
			case 0x26: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 26: rewind_block\n"); break;
			case 0x27: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 27: read_block_a\n"); break;
			case 0x28: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 28: read_block_b\n"); break;
			case 0x29: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 29: tape_rewind_fast\n"); break;
			case 0x2a: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 2a: tape_forward\n"); break;
			case 0x2b: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 2b: tape_rewind\n"); break;
			case 0x2c: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 2c: force_abort\n"); break;
			case 0x2d: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 2d: tape_erase\n"); break;
			case 0x2e: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 2e: search_tape_mark\n"); break;
			case 0x2f: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 2f: search_eot\n"); break;
			case 0x30: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 30: advance_block\n"); break;
			case 0x31: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 31: write_tape_mark\n"); break;
			case 0x32: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 32: reset_error\n"); break;
			case 0x33: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 33: flag_status_report\n"); break;
			case 0x34: log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO 34: report_status_to_main\n"); break;
			default:   log_cb(RETRO_LOG_DEBUG, LOGPRE "8041 FNO %02x: invalid\n", data);
			}
		}
}
#endif

/***************************************************************************
 *
 *	TYPE1 DONGLE (DE-0061)
 *	- Test Tape
 *	- Lock 'n Chase
 *	- Treasure Island
 *	- Super Astro Fighter
 *	- Lucky Poker
 *	- Terranian
 *	- Explorer
 *	- Pro Golf
 *
 ***************************************************************************/

READ_HANDLER( decocass_type1_r )
{
	static data8_t latch1;
	data8_t data;

	if (1 == (offset & 1))
	{
		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_STAT);
		else
			data = 0xff;

		data =
			(BIT0(data) << 0) |
			(BIT1(data) << 1) |
			(1			<< 2) |
			(1			<< 3) |
			(1			<< 4) |
			(1			<< 5) |
			(1			<< 6) |
			(0			<< 7);
		LOG(4,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x <- (%s %s)\n",
			timer_get_time(), activecpu_get_previouspc(), offset, data,
			(data & 1) ? "OBF" : "-",
			(data & 2) ? "IBF" : "-"));
	}
	else
	{
		offs_t promaddr;
		data8_t save;
		UINT8 *prom = memory_region(REGION_USER1);

		if (firsttime)
		{
			LOG(4,("prom data:\n"));
			for (promaddr = 0; promaddr < 32; promaddr++)
			{
				if (promaddr % 8 == 0)
					LOG(4,("%04x:", promaddr));
				LOG(4,(" %02x%s", prom[promaddr], (promaddr % 8) == 7 ? "\n" : ""));
			}
			firsttime = 0;
			latch1 = 0; 	 /* reset latch (??) */
		}

		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_DATA);
		else
			data = 0xff;

		save = data;	/* save the unmodifed data for the latch */

		promaddr =
			(((data >> MAP0(type1_inmap)) & 1) << 0) |
			(((data >> MAP1(type1_inmap)) & 1) << 1) |
			(((data >> MAP4(type1_inmap)) & 1) << 2) |
			(((data >> MAP5(type1_inmap)) & 1) << 3) |
			(((data >> MAP7(type1_inmap)) & 1) << 4);

		data =
			(((prom[promaddr] >> 0) & 1)			   << MAP0(type1_outmap)) |
			(((prom[promaddr] >> 1) & 1)			   << MAP1(type1_outmap)) |
			((1 - ((latch1 >> MAP2(type1_inmap)) & 1)) << MAP2(type1_outmap)) |
			(((data >> MAP3(type1_inmap)) & 1)		   << MAP3(type1_outmap)) |
			(((prom[promaddr] >> 2) & 1)			   << MAP4(type1_outmap)) |
			(((prom[promaddr] >> 3) & 1)			   << MAP5(type1_outmap)) |
			(((latch1 >> MAP6(type1_inmap)) & 1)	   << MAP6(type1_outmap)) |
			(((prom[promaddr] >> 4) & 1)			   << MAP7(type1_outmap));

		LOG(3,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x <- (%s $%02x mangled with PROM[$%02x])\n", timer_get_time(), activecpu_get_previouspc(), offset, data, 0 == (offset & E5XX_MASK) ? "8041-DATA" : "open bus", save, promaddr));

		latch1 = save;		/* latch the data for the next A0 == 0 read */
	}
	return data;
}


/***************************************************************************
 *
 *  TYPE1 DONGLE (DE-0061) with alternate PROM
 *  - Highway Chase
 *
 ***************************************************************************/

READ_HANDLER( decocass_type1_alt_r )
{
	static UINT8 latch1;
	UINT8 data;

	if (1 == (offset & 1))
	{
		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_STAT);
		else
			data = 0xff;

		data =
			(BIT0(data) << 0) |
			(BIT1(data) << 1) |
			(1			<< 2) |
			(1			<< 3) |
			(1			<< 4) |
			(1			<< 5) |
			(1			<< 6) |
			(0			<< 7);
		LOG(4,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x <- (%s %s)\n",
			timer_get_time(), activecpu_get_previouspc(), offset, data,
			(data & 1) ? "OBF" : "-",
			(data & 2) ? "IBF" : "-"));
	}
	else
	{
		offs_t promaddr;
		UINT8 save;
		UINT8 *prom = memory_region(REGION_USER1);

		if (firsttime)
		{
			LOG(4,("prom data:\n"));
			for (promaddr = 0; promaddr < 32; promaddr++)
			{
				if (promaddr % 8 == 0)
					LOG(4,("%04x:", promaddr));
				LOG(4,(" %02x%s", prom[promaddr], (promaddr % 8) == 7 ? "\n" : ""));
			}
			firsttime = 0;
			latch1 = 0; 	 /* reset latch (??) */
		}

		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_DATA);
		else
			data = 0xff;

		save = data;	/* save the unmodifed data for the latch */

		promaddr =
			(((data >> MAP0(type1_inmap)) & 1) << 0) |
			(((data >> MAP1(type1_inmap)) & 1) << 1) |
			(((data >> MAP4(type1_inmap)) & 1) << 2) |
			(((data >> MAP5(type1_inmap)) & 1) << 3) |
			(((data >> MAP6(type1_inmap)) & 1) << 4);

		data =
			(((prom[promaddr] >> 0) & 1)			   << MAP0(type1_outmap)) |
			(((prom[promaddr] >> 1) & 1)			   << MAP1(type1_outmap)) |
			((1 - ((latch1 >> MAP2(type1_inmap)) & 1)) << MAP2(type1_outmap)) |
			(((data >> MAP3(type1_inmap)) & 1)		   << MAP3(type1_outmap)) |
			(((prom[promaddr] >> 2) & 1)			   << MAP4(type1_outmap)) |
			(((prom[promaddr] >> 3) & 1)			   << MAP5(type1_outmap)) |
			(((prom[promaddr] >> 4) & 1)			   << MAP6(type1_outmap)) |
			(((latch1 >> MAP7(type1_inmap)) & 1)	   << MAP7(type1_outmap));

		LOG(3,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x <- (%s $%02x mangled with PROM[$%02x])\n", timer_get_time(), activecpu_get_previouspc(), offset, data, 0 == (offset & E5XX_MASK) ? "8041-DATA" : "open bus", save, promaddr));

		latch1 = save;		/* latch the data for the next A0 == 0 read */
	}
	return data;
}

/*
 * special handler for the test tape, because we cannot
 * look inside the dongle :-/
 * There seem to be lines 1, 3 and 6 straight through.
 * The rest could be translated with the standard Type1 dongle
 * PROM, but I don't know. For now we have found this lookup
 * table by applying data to the dongle and logging the outputs.
 */

READ_HANDLER( decocass_type1_map1_r )
{
	static data8_t map[] = {
		0x01,0x34,0x03,0x36,0xa4,0x15,0xa6,0x17,
		0x09,0x3c,0x0b,0x3e,0xac,0x1d,0xae,0x1f,
		0x90,0x14,0x92,0x16,0x85,0x00,0x87,0x02,
		0x98,0x1c,0x9a,0x1e,0x8d,0x08,0x8f,0x0a,
		0x31,0x30,0x33,0x32,0xa1,0x11,0xa3,0x13,
		0x39,0x38,0x3b,0x3a,0xa9,0x19,0xab,0x1b,
		0x84,0xb5,0x86,0xb7,0x81,0xb4,0x83,0xb6,
		0x8c,0xbd,0x8e,0xbf,0x89,0xbc,0x8b,0xbe,
		0x41,0x74,0x43,0x76,0xe4,0x55,0xe6,0x57,
		0x49,0x7c,0x4b,0x7e,0xec,0x5d,0xee,0x5f,
		0xd0,0x54,0xd2,0x56,0xc5,0x40,0xc7,0x42,
		0xd8,0x5c,0xda,0x5e,0xcd,0x48,0xcf,0x4a,
		0x71,0x70,0x73,0x72,0xe1,0x51,0xe3,0x53,
		0x79,0x78,0x7b,0x7a,0xe9,0x59,0xeb,0x5b,
		0xc4,0xf5,0xc6,0xf7,0xc1,0xf4,0xc3,0xf6,
		0xcc,0xfd,0xce,0xff,0xc9,0xfc,0xcb,0xfe,
		0x25,0xa0,0x27,0xa2,0x95,0x10,0x97,0x12,
		0x2d,0xa8,0x2f,0xaa,0x9d,0x18,0x9f,0x1a,
		0x80,0xb1,0x82,0xb3,0x24,0xb0,0x26,0xb2,
		0x88,0xb9,0x8a,0xbb,0x2c,0xb8,0x2e,0xba,
		0x21,0x94,0x23,0x96,0x05,0x04,0x07,0x06,
		0x29,0x9c,0x2b,0x9e,0x0d,0x0c,0x0f,0x0e,
		0x35,0xa5,0x37,0xa7,0x20,0x91,0x22,0x93,
		0x3d,0xad,0x3f,0xaf,0x28,0x99,0x2a,0x9b,
		0x65,0xe0,0x67,0xe2,0xd5,0x50,0xd7,0x52,
		0x6d,0xe8,0x6f,0xea,0xdd,0x58,0xdf,0x5a,
		0xc0,0xf1,0xc2,0xf3,0x64,0xf0,0x66,0xf2,
		0xc8,0xf9,0xca,0xfb,0x6c,0xf8,0x6e,0xfa,
		0x61,0xd4,0x63,0xd6,0x45,0x44,0x47,0x46,
		0x69,0xdc,0x6b,0xde,0x4d,0x4c,0x4f,0x4e,
		0x75,0xe5,0x77,0xe7,0x60,0xd1,0x62,0xd3,
		0x7d,0xed,0x7f,0xef,0x68,0xd9,0x6a,0xdb
	};
	data8_t save, data;

	if (1 == (offset & 1))
	{
		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_STAT);
		else
			data = 0xff;

		data =
			(BIT0(data) << 0) |
			(BIT1(data) << 1) |
			(1			<< 2) |
			(1			<< 3) |
			(1			<< 4) |
			(1			<< 5) |
			(1			<< 6) |
			(0			<< 7);
		LOG(4,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x <- (%s %s)\n",
			timer_get_time(), activecpu_get_previouspc(), offset, data,
			(data & 1) ? "OBF" : "-",
			(data & 2) ? "IBF" : "-"));
	}
	else
	{
		if (0 == (offset & E5XX_MASK))
			save = cpunum_get_reg(2, I8X41_DATA);
		else
			save = 0xff;

		data = map[save];

		LOG(3,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x '%c' <- map[%02x] (%s)\n", timer_get_time(), activecpu_get_previouspc(), offset, data, (data >= 32) ? data : '.', save, 0 == (offset & E5XX_MASK) ? "8041-DATA" : "open bus"));
	}
	return data;
}

READ_HANDLER( decocass_type1_map2_r )
{
	static data8_t map[] = {
/* 00 */0x06,0x1f,0x8f,0x0c,0x02,0x1b,0x8b,0x08,
		0x1e,0x1d,0x8e,0x16,0x1a,0x19,0x8a,0x12,
		0x95,0x17,0x94,0x05,0x91,0x13,0x90,0x01,
		0x87,0x04,0x86,0x9f,0x83,0x00,0x82,0x9b,
/* 20 */0x26,0x3f,0xaf,0x2c,0x22,0x3b,0xab,0x28,
		0x3e,0x3d,0xae,0x36,0x3a,0x39,0xaa,0x32,
		0xb5,0x37,0xb4,0x25,0xb1,0x33,0xb0,0x21,
		0xa7,0x24,0xa6,0xbf,0xa3,0x20,0xa2,0xbb,
/* 40 */0x46,0x5f,0xcf,0x4c,0x42,0x5b,0xcb,0x48,
		0x5e,0x5d,0xce,0x56,0x5a,0x59,0xca,0x52,
		0xd5,0x57,0xd4,0x45,0xd1,0x53,0xd0,0x41,
		0xc7,0x44,0xc6,0xdf,0xc3,0x40,0xc2,0xdb,
/* 60 */0x66,0x7f,0xef,0x6c,0x62,0x7b,0xeb,0x68,
		0x7e,0x7d,0xee,0x76,0x7a,0x79,0xea,0x72,
		0xf5,0x77,0xf4,0x65,0xf1,0x73,0xf0,0x61,
		0xe7,0x64,0xe6,0xff,0xe3,0x60,0xe2,0xfb,
/* 80 */0x1c,0x8d,0x8c,0x15,0x18,0x89,0x88,0x11,
		0x0e,0x97,0x14,0x07,0x0a,0x93,0x10,0x03,
		0x85,0x9e,0x0f,0x9d,0x81,0x9a,0x0b,0x99,
		0x84,0x9c,0x0d,0x96,0x80,0x98,0x09,0x92,
/* a0 */0x3c,0xad,0xac,0x35,0x38,0xa9,0xa8,0x31,
		0x2e,0xb7,0x34,0x27,0x2a,0xb3,0x30,0x23,
		0xa5,0xbe,0x2f,0xbd,0xa1,0xba,0x2b,0xb9,
		0xa4,0xbc,0x2d,0xb6,0xa0,0xb8,0x29,0xb2,
/* c0 */0x5c,0xcd,0xcc,0x55,0x58,0xc9,0xc8,0x51,
		0x4e,0xd7,0x54,0x47,0x4a,0xd3,0x50,0x43,
		0xc5,0xde,0x4f,0xdd,0xc1,0xda,0x4b,0xd9,
		0xc4,0xdc,0x4d,0xd6,0xc0,0xd8,0x49,0xd2,
/* e0 */0x7c,0xed,0xec,0x75,0x78,0xe9,0xe8,0x71,
		0x6e,0xf7,0x74,0x67,0x6a,0xf3,0x70,0x63,
		0xe5,0xfe,0x6f,0xfd,0xe1,0xfa,0x6b,0xf9,
		0xe4,0xfc,0x6d,0xf6,0xe0,0xf8,0x69,0xf2
	};
	static data8_t latch2;
	data8_t save, addr, data;

	/* read from tape:
	 *	7d 43 5d 4f 04 ae e3 59 57 cb d6 55 4d 15
	 * should become:
	 *	?? 48 44 52 42 30 31 44 45 43 4f 53 59 53
	 * lookup entries with above values:
	 *	?? 47 59 4f 44 ae a7 59 53 cf d2 55 4d 55
	 * difference:
	 *	   04 04 00 40 00 44 00 04 04 04 00 00 40
	 */

	if (1 == (offset & 1))
	{
		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_STAT);
		else
			data = 0xff;

		data =
			(BIT0(data) << 0) |
			(BIT1(data) << 1) |
			(1			<< 2) |
			(1			<< 3) |
			(1			<< 4) |
			(1			<< 5) |
			(1			<< 6) |
			(0			<< 7);
		LOG(4,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x <- (%s %s)\n",
			timer_get_time(), activecpu_get_previouspc(), offset, data,
			(data & 1) ? "OBF" : "-",
			(data & 2) ? "IBF" : "-"));
	}
	else
	{
		if (0 == (offset & E5XX_MASK))
			save = cpunum_get_reg(2, I8X41_DATA);
		else
			save = 0xff;

		addr = (save & ~0x44) | (latch2 & 0x44);
		data = map[addr];

		LOG(3,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x '%c' <- map[%02x = %02x^((%02x^%02x)&%02x)] (%s)\n", timer_get_time(), activecpu_get_previouspc(), offset, data, (data >= 32) ? data : '.', addr, save, latch2, save, 0x44, 0 == (offset & E5XX_MASK) ? "8041-DATA" : "open bus"));
		latch2 = save;
	}
	return data;
}

READ_HANDLER( decocass_type1_map3_r )
{
	static data8_t map[] = {
/* 00 */0x03,0x36,0x01,0x34,0xa6,0x17,0xa4,0x15,
		0x0b,0x3e,0x09,0x3c,0xae,0x1f,0xac,0x1d,
		0x92,0x16,0x90,0x14,0x87,0x02,0x85,0x00,
		0x9a,0x1e,0x98,0x1c,0x8f,0x0a,0x8d,0x08,
/* 20 */0x33,0x32,0x31,0x30,0xa3,0x13,0xa1,0x11,
		0x3b,0x3a,0x39,0x38,0xab,0x1b,0xa9,0x19,
		0x86,0xb7,0x84,0xb5,0x83,0xb6,0x81,0xb4,
		0x8e,0xbf,0x8c,0xbd,0x8b,0xbe,0x89,0xbc,
/* 40 */0x43,0x76,0x41,0x74,0xe6,0x57,0xe4,0x55,
		0x4b,0x7e,0x49,0x7c,0xee,0x5f,0xec,0x5d,
		0xd2,0x56,0xd0,0x54,0xc7,0x42,0xc5,0x40,
		0xda,0x5e,0xd8,0x5c,0xcf,0x4a,0xcd,0x48,
/* 60 */0x73,0x72,0x71,0x70,0xe3,0x53,0xe1,0x51,
		0x7b,0x7a,0x79,0x78,0xeb,0x5b,0xe9,0x59,
		0xc6,0xf7,0xc4,0xf5,0xc3,0xf6,0xc1,0xf4,
		0xce,0xff,0xcc,0xfd,0xcb,0xfe,0xc9,0xfc,
/* 80 */0x27,0xa2,0x25,0xa0,0x97,0x12,0x95,0x10,
		0x2f,0xaa,0x2d,0xa8,0x9f,0x1a,0x9d,0x18,
		0x82,0xb3,0x80,0xb1,0x26,0xb2,0x24,0xb0,
		0x8a,0xbb,0x88,0xb9,0x2e,0xba,0x2c,0xb8,
/* a0 */0x23,0x96,0x21,0x94,0x07,0x06,0x05,0x04,
		0x2b,0x9e,0x29,0x9c,0x0f,0x0e,0x0d,0x0c,
		0x37,0xa7,0x35,0xa5,0x22,0x93,0x20,0x91,
		0x3f,0xaf,0x3d,0xad,0x2a,0x9b,0x28,0x99,
/* c0 */0x67,0xe2,0x65,0xe0,0xd7,0x52,0xd5,0x50,
		0x6f,0xea,0x6d,0xe8,0xdf,0x5a,0xdd,0x58,
		0xc2,0xf3,0xc0,0xf1,0x66,0xf2,0x64,0xf0,
		0xca,0xfb,0xc8,0xf9,0x6e,0xfa,0x6c,0xf8,
/* e0 */0x63,0xd6,0x61,0xd4,0x47,0x46,0x45,0x44,
		0x6b,0xde,0x69,0xdc,0x4f,0x4e,0x4d,0x4c,
		0x77,0xe7,0x75,0xe5,0x62,0xd3,0x60,0xd1,
		0x7f,0xef,0x7d,0xed,0x6a,0xdb,0x68,0xd9
	};
	static data8_t latch3;
	data8_t save, addr, data;

	/* read from tape:
	 *	f6 5f e5 c5 17 23 62 40 67 51 c5 ee 85 23
	 * should become:
	 *	20 48 44 52 42 30 31 41 53 54 52 4f 50 32
	 * lookup entries with above values:
	 *	b6 5f e7 c5 55 23 22 42 65 53 c5 ec c7 21
	 * difference:
	 *	40 00 02 00 40 00 40 02 02 02 00 02 42 02
	 */

	if (1 == (offset & 1))
	{
		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_STAT);
		else
			data = 0xff;

		data =
			(BIT0(data) << 0) |
			(BIT1(data) << 1) |
			(1			<< 2) |
			(1			<< 3) |
			(1			<< 4) |
			(1			<< 5) |
			(1			<< 6) |
			(0			<< 7);
		LOG(4,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x <- (%s %s)\n",
			timer_get_time(), activecpu_get_previouspc(), offset, data,
			(data & 1) ? "OBF" : "-",
			(data & 2) ? "IBF" : "-"));
	}
	else
	{

		if (0 == (offset & E5XX_MASK))
			save = cpunum_get_reg(2, I8X41_DATA);
		else
			save = 0xff;

		addr = (save & ~0x42) | (latch3 & 0x42);
		data = map[addr];

		LOG(3,("%9.7f 6502-PC: %04x decocass_type1_r(%02x): $%02x '%c' <- map[%02x = %02x^((%02x^%02x)&%02x)] (%s)\n", timer_get_time(), activecpu_get_previouspc(), offset, data, data >= 0x20 ? data : '.', addr, save, latch3, save, 0x42, 0 == (offset & E5XX_MASK) ? "8041-DATA" : "open bus"));
		latch3 = save;
	}
	return data;
}

/***************************************************************************
 *
 *  TYPE1 DONGLE (DE-0061)
 *  - Manhattan
 *
 * Input bits that are passed uninverted = $54 (3 true bits)
 * Input bits that are passed inverted   = $00 (0 inverted bits)
 * Remaining bits for addressing PROM    = $AB (5 bits)
 *
 ***************************************************************************/

READ_HANDLER(decocass_type1_latch_xab_pass_x54_r)
{
	static data8_t latch1;
	data8_t data;

	if (1 == (offset & 1))
	{
		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_STAT);
		else
			data = 0xff;

		data = (BIT(data, 0) << 0) | (BIT(data, 1) << 1) | 0x7c;
		LOG(4,("%10s 6502-PC: %04x decocass_type1_latch_27_pass_3_inv_2_r(%02x): $%02x <- (%s %s)\n",
			timer_get_time(), activecpu_get_previouspc(), offset, data,
			(data & 1) ? "OBF" : "-",
			(data & 2) ? "IBF" : "-"));
	}
	else
	{
		offs_t promaddr;
		UINT8 save;
		UINT8 *prom = memory_region(REGION_USER1);

		if (firsttime)
		{
			LOG(3,("prom data:\n"));
			for (promaddr = 0; promaddr < 32; promaddr++)
			{
				if (promaddr % 8 == 0)
					LOG(3,("  %02x:", promaddr));
				LOG(3,(" %02x%s", prom[promaddr], (promaddr % 8) == 7 ? "\n" : ""));
			}
			firsttime = 0;
			latch1 = 0;	 /* reset latch (??) */
		}

		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, I8X41_DATA);
		else
			data = 0xff;

		save = data;	/* save the unmodifed data for the latch */

		/* AB 10101011 */
		promaddr =
			(((data >> MAP0(type1_inmap)) & 1) << 0) |
			(((data >> MAP1(type1_inmap)) & 1) << 1) |
			(((data >> MAP3(type1_inmap)) & 1) << 2) |
			(((data >> MAP5(type1_inmap)) & 1) << 3) |
			(((data >> MAP7(type1_inmap)) & 1) << 4);
		/* no latch, pass bit 0x54 */
		data =
			(((prom[promaddr] >> 0) & 1)			   << MAP0(type1_outmap)) |
			(((prom[promaddr] >> 1) & 1)			   << MAP1(type1_outmap)) |
			(((data >> MAP2(type1_inmap)) & 1)		   << MAP2(type1_outmap)) |
			(((prom[promaddr] >> 2) & 1)			   << MAP3(type1_outmap)) |
			(((data >> MAP4(type1_inmap)) & 1)		   << MAP4(type1_outmap))  |
			(((prom[promaddr] >> 3) & 1)			   << MAP5(type1_outmap)) |
			(((data >> MAP6(type1_inmap)) & 1)		   << MAP6(type1_outmap)) |
			(((prom[promaddr] >> 4) & 1)			   << MAP7(type1_outmap));

		LOG(3,("%10s 6502-PC: %04x decocass_type1_latch_27_pass_3_inv_2_r(%02x): $%02x\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
		latch1 = save;		/* latch the data for the next A0 == 0 read */
	}
	return data;
}

/***************************************************************************
 *
 *	TYPE2 DONGLE (CS82-007)
 *	- Mission X
 *	- Disco No 1
 *	- Pro Tennis
 *	- Tornado
 *
 ***************************************************************************/
READ_HANDLER( decocass_type2_r )
{
	data8_t data;

	if (1 == type2_xx_latch)
	{
		if (1 == (offset & 1))
		{
			data8_t *prom = memory_region(REGION_USER1);
			data = prom[256 * type2_d2_latch + type2_promaddr];
			LOG(3,("%9.7f 6502-PC: %04x decocass_type2_r(%02x): $%02x <- prom[%03x]\n", timer_get_time(), activecpu_get_previouspc(), offset, data, 256 * type2_d2_latch + type2_promaddr));
		}
		else
		{
			data = 0xff;	/* floating input? */
		}
	}
	else
	{
		if (0 == (offset & E5XX_MASK))
			data = cpunum_get_reg(2, offset & 1 ? I8X41_STAT : I8X41_DATA);
		else
			data = offset & 0xff;

		LOG(3,("%9.7f 6502-PC: %04x decocass_type2_r(%02x): $%02x <- 8041-%s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, offset & 1 ? "STATUS" : "DATA"));
	}
	return data;
}

WRITE_HANDLER( decocass_type2_w )
{
	if (1 == type2_xx_latch)
	{
		if (1 == (offset & 1))
		{
			LOG(4,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> set PROM+D2 latch", timer_get_time(), activecpu_get_previouspc(), offset, data));
		}
		else
		{
			type2_promaddr = data;
			LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> set PROM addr $%02x\n", timer_get_time(), activecpu_get_previouspc(), offset, data, type2_promaddr));
			return;
		}
	}
	else
	{
		LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s ", timer_get_time(), activecpu_get_previouspc(), offset, data, offset & 1 ? "8041-CMND" : "8041 DATA"));
	}
	if (1 == (offset & 1))
	{
		if (0xc0 == (data & 0xf0))
		{
			type2_xx_latch = 1;
			type2_d2_latch = (data & 0x04) ? 1 : 0;
			LOG(3,("PROM:%s D2:%d", type2_xx_latch ? "on" : "off", type2_d2_latch));
		}
	}
	cpunum_set_reg(2, offset & 1 ? I8X41_CMND : I8X41_DATA, data);

#ifdef MAME_DEBUG
	decocass_fno(offset, data);
#endif
}

/***************************************************************************
 *
 *	TYPE3 DONGLE
 *	- Bump 'n Jump
 *	- Burnin' Rubber
 *	- Burger Time
 *	- Graplop
 *	- Cluster Buster
 *	- LaPaPa
 *	- Fighting Ice Hockey
 *	- Pro Bowling
 *	- Night Star
 *	- Pro Soccer
 *	- Peter Pepper's Ice Cream Factory
 *
 ***************************************************************************/
READ_HANDLER( decocass_type3_r )
{
	data8_t data, save;

	if (1 == (offset & 1))
	{
		if (1 == type3_pal_19)
		{
			data8_t *prom = memory_region(REGION_USER1);
			data = prom[type3_ctrs];
			LOG(3,("%9.7f 6502-PC: %04x decocass_type3_r(%02x): $%02x <- prom[$%03x]\n", timer_get_time(), activecpu_get_previouspc(), offset, data, type3_ctrs));
			if (++type3_ctrs == 4096)
				type3_ctrs = 0;
		}
		else
		{
			if (0 == (offset & E5XX_MASK))
			{
				data = cpunum_get_reg(2, I8X41_STAT);
				LOG(4,("%9.7f 6502-PC: %04x decocass_type3_r(%02x): $%02x <- 8041 STATUS\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
			}
			else
			{
				data = 0xff;	/* open data bus? */
				LOG(4,("%9.7f 6502-PC: %04x decocass_type3_r(%02x): $%02x <- open bus\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
			}
		}
	}
	else
	{
		if (1 == type3_pal_19)
		{
			save = data = 0xff;    /* open data bus? */
			LOG(3,("%9.7f 6502-PC: %04x decocass_type3_r(%02x): $%02x <- open bus", timer_get_time(), activecpu_get_previouspc(), offset, data));
		}
		else
		{
			if (0 == (offset & E5XX_MASK))
			{
				save = cpunum_get_reg(2, I8X41_DATA);
				if (type3_swap == TYPE3_SWAP_01)
				{
					data =
						(BIT1(save) << 0) |
						(type3_d0_latch << 1) |
						(BIT2(save) << 2) |
						(BIT3(save) << 3) |
						(BIT4(save) << 4) |
						(BIT5(save) << 5) |
						(BIT6(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_12)
				{
					data =
						(type3_d0_latch << 0) |
						(BIT2(save) << 1) |
						(BIT1(save) << 2) |
						(BIT3(save) << 3) |
						(BIT4(save) << 4) |
						(BIT5(save) << 5) |
						(BIT6(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_13)
				{
					data =
						(type3_d0_latch << 0) |
						(BIT3(save) << 1) |
						(BIT2(save) << 2) |
						(BIT1(save) << 3) |
						(BIT4(save) << 4) |
						(BIT5(save) << 5) |
						(BIT6(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_23_56)
				{
					data =
					(type3_d0_latch << 0) |
						(BIT1(save) << 1) |
						(BIT3(save) << 2) |
						(BIT2(save) << 3) |
						(BIT4(save) << 4) |
						(BIT6(save) << 5) |
						(BIT5(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_24)
				{
					data =
						(type3_d0_latch << 0) |
						(BIT1(save) << 1) |
						(BIT4(save) << 2) |
						(BIT3(save) << 3) |
						(BIT2(save) << 4) |
						(BIT5(save) << 5) |
						(BIT6(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_25)
				{
					data =
						(type3_d0_latch << 0) |
						(BIT1(save) << 1) |
						(BIT5(save) << 2) |
						(BIT3(save) << 3) |
						(BIT4(save) << 4) |
						(BIT2(save) << 5) |
						(BIT6(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_34_0)
				{
					data =
						(type3_d0_latch << 0) |
						(BIT1(save) << 1) |
						(BIT2(save) << 2) |
						(BIT3(save) << 4) |
						(BIT4(save) << 3) |
						(BIT5(save) << 5) |
						(BIT6(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_34_7)
				{
					data =
						(BIT7(save) << 0) |
						(BIT1(save) << 1) |
						(BIT2(save) << 2) |
						(BIT4(save) << 3) |
						(BIT3(save) << 4) |
						(BIT5(save) << 5) |
						(BIT6(save) << 6) |
						(type3_d0_latch << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_45)
				{
					data =
						type3_d0_latch |
                        (BIT1(save) << 1) |
						(BIT2(save) << 2) |
						(BIT3(save) << 3) |
						(BIT5(save) << 4) |
						(BIT4(save) << 5) |
						(BIT6(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_56)
				{
					data =
						type3_d0_latch |
						(BIT1(save) << 1) |
						(BIT2(save) << 2) |
						(BIT3(save) << 3) |
						(BIT4(save) << 4) |
						(BIT6(save) << 5) |
						(BIT5(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				if (type3_swap == TYPE3_SWAP_67)
				{
					data =
						type3_d0_latch |
						(BIT1(save) << 1) |
						(BIT2(save) << 2) |
						(BIT3(save) << 3) |
						(BIT4(save) << 4) |
						(BIT5(save) << 5) |
						(BIT7(save) << 6) |
						(BIT6(save) << 7);
					type3_d0_latch = save & 1;
				}
				else
				{
					data =
						type3_d0_latch |
						(BIT1(save) << 1) |
						(BIT2(save) << 2) |
						(BIT3(save) << 3) |
						(BIT4(save) << 4) |
						(BIT5(save) << 5) |
						(BIT6(save) << 6) |
						(BIT7(save) << 7);
					type3_d0_latch = save & 1;
				}
				LOG(3,("%9.7f 6502-PC: %04x decocass_type3_r(%02x): $%02x '%c' <- 8041-DATA\n", timer_get_time(), activecpu_get_previouspc(), offset, data, (data >= 32) ? data : '.'));
			}
			else
			{
				save = 0xff;	/* open data bus? */
				data =
					type3_d0_latch |
					(BIT1(save) << 1) |
					(BIT2(save) << 2) |
					(BIT3(save) << 3) |
					(BIT4(save) << 4) |
					(BIT5(save) << 5) |
					(BIT6(save) << 7) |
					(BIT7(save) << 6);
				LOG(3,("%9.7f 6502-PC: %04x decocass_type3_r(%02x): $%02x '%c' <- open bus (D0 replaced with latch)\n", timer_get_time(), activecpu_get_previouspc(), offset, data, (data >= 32) ? data : '.'));
				type3_d0_latch = save & 1;
			}
		}
	}

	return data;
}

WRITE_HANDLER( decocass_type3_w )
{
	if (1 == (offset & 1))
	{
		if (1 == type3_pal_19)
		{
			type3_ctrs = data << 4;
			LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, "LDCTRS"));
			return;
		}
		else
		if (0xc0 == (data & 0xf0))
			type3_pal_19 = 1;
	}
	else
	{
		if (1 == type3_pal_19)
		{
			/* write nowhere?? */
			LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, "nowhere?"));
			return;
		}
	}
	LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, offset & 1 ? "8041-CMND" : "8041-DATA"));
	cpunum_set_reg(2, offset & 1 ? I8X41_CMND : I8X41_DATA, data);
}

/***************************************************************************
 *
 *	TYPE4 DONGLE
 *	- Scrum Try
 *	Contains a 32K (EP)ROM that can be read from any byte
 *	boundary sequentially. The EPROM is enable after writing
 *	1100xxxx to E5x1 once. Then an address is written LSB
 *	to E5x0 MSB to E5x1 and every read from E5x1 returns the
 *	next byte of the contents.
 *
 ***************************************************************************/

READ_HANDLER( decocass_type4_r )
{
	data8_t data;

	if (1 == (offset & 1))
	{
		if (0 == (offset & E5XX_MASK))
		{
			data = cpunum_get_reg(2, I8X41_STAT);
			LOG(4,("%9.7f 6502-PC: %04x decocass_type4_r(%02x): $%02x <- 8041 STATUS\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
		}
		else
		{
			data = 0xff;	/* open data bus? */
			LOG(4,("%9.7f 6502-PC: %04x decocass_type4_r(%02x): $%02x <- open bus\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
		}
	}
	else
	{
		if (type4_latch)
		{
			UINT8 *prom = memory_region(REGION_USER1);

			data = prom[type4_ctrs];
			LOG(3,("%9.7f 6502-PC: %04x decocass_type5_r(%02x): $%02x '%c' <- PROM[%04x]\n", timer_get_time(), activecpu_get_previouspc(), offset, data, (data >= 32) ? data : '.', type4_ctrs));
			type4_ctrs = (type4_ctrs+1) & 0x7fff;
		}
		else
		{
			if (0 == (offset & E5XX_MASK))
			{
				data = cpunum_get_reg(2, I8X41_DATA);
				LOG(3,("%9.7f 6502-PC: %04x decocass_type4_r(%02x): $%02x '%c' <- open bus (D0 replaced with latch)\n", timer_get_time(), activecpu_get_previouspc(), offset, data, (data >= 32) ? data : '.'));
			}
			else
			{
				data = 0xff;	/* open data bus? */
				LOG(4,("%9.7f 6502-PC: %04x decocass_type4_r(%02x): $%02x <- open bus\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
			}
		}
	}

	return data;
}

WRITE_HANDLER( decocass_type4_w )
{
	if (1 == (offset & 1))
	{
		if (1 == type4_latch)
		{
			type4_ctrs = (type4_ctrs & 0x00ff) | ((data & 0x7f) << 8);
			LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> CTRS MSB (%04x)\n", timer_get_time(), activecpu_get_previouspc(), offset, data, type4_ctrs));
			return;
		}
		else
		if (0xc0 == (data & 0xf0))
		{
			type4_latch = 1;
		}
	}
	else
	{
		if (type4_latch)
		{
			type4_ctrs = (type4_ctrs & 0xff00) | data;
			LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> CTRS LSB (%04x)\n", timer_get_time(), activecpu_get_previouspc(), offset, data, type4_ctrs));
			return;
		}
	}
	LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, offset & 1 ? "8041-CMND" : "8041-DATA"));
	cpunum_set_reg(2, offset & 1 ? I8X41_CMND : I8X41_DATA, data);
}

/***************************************************************************
 *
 *	TYPE5 DONGLE
 *	- Boulder Dash
 *	Actually a NOP dongle returning 0x55 after triggering a latch
 *	by writing 1100xxxx to E5x1
 *
 ***************************************************************************/

READ_HANDLER( decocass_type5_r )
{
	data8_t data;

	if (1 == (offset & 1))
	{
		if (0 == (offset & E5XX_MASK))
		{
			data = cpunum_get_reg(2, I8X41_STAT);
			LOG(4,("%9.7f 6502-PC: %04x decocass_type5_r(%02x): $%02x <- 8041 STATUS\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
		}
		else
		{
			data = 0xff;	/* open data bus? */
			LOG(4,("%9.7f 6502-PC: %04x decocass_type5_r(%02x): $%02x <- open bus\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
		}
	}
	else
	{
		if (type5_latch)
		{
			data = 0x55;	/* Only a fixed value? It looks like this is all we need to do */
			LOG(3,("%9.7f 6502-PC: %04x decocass_type5_r(%02x): $%02x '%c' <- fixed value???\n", timer_get_time(), activecpu_get_previouspc(), offset, data, (data >= 32) ? data : '.'));
		}
		else
		{
			if (0 == (offset & E5XX_MASK))
			{
				data = cpunum_get_reg(2, I8X41_DATA);
				LOG(3,("%9.7f 6502-PC: %04x decocass_type5_r(%02x): $%02x '%c' <- open bus (D0 replaced with latch)\n", timer_get_time(), activecpu_get_previouspc(), offset, data, (data >= 32) ? data : '.'));
			}
			else
			{
				data = 0xff;	/* open data bus? */
				LOG(4,("%9.7f 6502-PC: %04x decocass_type5_r(%02x): $%02x <- open bus\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
			}
		}
	}

	return data;
}

WRITE_HANDLER( decocass_type5_w )
{
	if (1 == (offset & 1))
	{
		if (1 == type5_latch)
		{
			LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, "latch #2??"));
			return;
		}
		else
		if (0xc0 == (data & 0xf0))
			type5_latch = 1;
	}
	else
	{
		if (type5_latch)
		{
			/* write nowhere?? */
			LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, "nowhere?"));
			return;
		}
	}
	LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, offset & 1 ? "8041-CMND" : "8041-DATA"));
	cpunum_set_reg(2, offset & 1 ? I8X41_CMND : I8X41_DATA, data);
}

/***************************************************************************
 *
 *	Main dongle and 8041 interface
 *
 ***************************************************************************/

READ_HANDLER( decocass_e5xx_r )
{
	data8_t data;

	/* E5x2-E5x3 and mirrors */
	if (2 == (offset & E5XX_MASK))
	{
		data =
			(BIT7(i8041_p1) 	  << 0) |	/* D0 = P17 - REQ/ */
			(BIT0(i8041_p2) 	  << 1) |	/* D1 = P20 - FNO/ */
			(BIT1(i8041_p2) 	  << 2) |	/* D2 = P21 - EOT/ */
			(BIT2(i8041_p2) 	  << 3) |	/* D3 = P22 - ERR/ */
			((tape_bot_eot) 	  << 4) |	/* D4 = BOT/EOT (direct from drive) */
			(1					  << 5) |	/* D5 floating input */
			(1					  << 6) |	/* D6 floating input */
			((1 - tape_present)   << 7);	/* D7 = cassette present */

		LOG(4,("%9.7f 6502-PC: %04x decocass_e5xx_r(%02x): $%02x <- STATUS (%s%s%s%s%s%s%s%s)\n",
			timer_get_time(),
			activecpu_get_previouspc(),
			offset, data,
			data & 0x01 ? "" : "REQ/",
			data & 0x02 ? "" : " FNO/",
			data & 0x04 ? "" : " EOT/",
			data & 0x08 ? "" : " ERR/",
			data & 0x10 ? " [BOT-EOT]" : "",
			data & 0x20 ? " [BIT5?]" : "",
			data & 0x40 ? " [BIT6?]" : "",
			data & 0x80 ? "" : " [CASS-PRESENT/]"));
	}
	else
	{
		if (decocass_dongle_r)
			data = (*decocass_dongle_r)(offset);
		else
			data = 0xff;
	}
	return data;
}

WRITE_HANDLER( decocass_e5xx_w )
{
	if (decocass_dongle_w)
	{
		(*decocass_dongle_w)(offset, data);
		return;
	}

	if (0 == (offset & E5XX_MASK))
	{
		LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> %s\n", timer_get_time(), activecpu_get_previouspc(), offset, data, offset & 1 ? "8041-CMND" : "8041-DATA"));
		cpunum_set_reg(2, offset & 1 ? I8X41_CMND : I8X41_DATA, data);
#ifdef MAME_DEBUG
		decocass_fno(offset, data);
#endif
	}
	else
	{
		LOG(3,("%9.7f 6502-PC: %04x decocass_e5xx_w(%02x): $%02x -> dongle\n", timer_get_time(), activecpu_get_previouspc(), offset, data));
	}
}

/***************************************************************************
 *
 *	init machine functions (select dongle and determine tape image size)
 *
 ***************************************************************************/
static void decocass_state_save_postload(void)
{
	int A;
	unsigned char *mem = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;

	memory_set_opcode_base(0, mem + diff);

	for (A = 0;A < 0x10000; A++)
		decocass_w(A, mem[A]);
	/* restart the timer if the tape was playing */
	if (0 != tape_dir)
		timer_adjust(tape_timer, TIME_NEVER, 0, 0);
}

void decocass_init_common(void)
{
	UINT8 *image = memory_region(REGION_USER2);
	int i, offs;

	tape_dir = 0;
	tape_speed = 0;
	tape_timer = timer_alloc(NULL);

	firsttime = 1;
	tape_present = 1;
	tape_blocks = 0;
	for (i = memory_region_length(REGION_USER2) / 256 - 1; !tape_blocks && i > 0; i--)
		for (offs = 256 * i; !tape_blocks && offs < 256 * i + 256; offs++)
			if (image[offs])
				tape_blocks = i+1;
	for (i = 0; i < tape_blocks; i++)
	{
		crc16_lsb = 0;
		crc16_msb = 0;
		for (offs = 256 * i; offs < 256 * i + 256; offs++)
		{
			tape_crc16(image[offs] << 7);
			tape_crc16(image[offs] << 6);
			tape_crc16(image[offs] << 5);
			tape_crc16(image[offs] << 4);
			tape_crc16(image[offs] << 3);
			tape_crc16(image[offs] << 2);
			tape_crc16(image[offs] << 1);
			tape_crc16(image[offs] << 0);
		}
		tape_crc16_lsb[i] = crc16_lsb;
		tape_crc16_msb[i] = crc16_msb;
	}

	tape_length = tape_blocks * TAPE_CHUNK * 8 * 2 + 2 * (TAPE_LEADER + TAPE_GAP + TAPE_HOLE);
	tape_time0 = (double)(TAPE_LEADER + TAPE_GAP - TAPE_HOLE) / TAPE_CLOCKRATE;
	LOG(0,("tape: %d blocks\n", tape_blocks));
	tape_bot_eot = 0;

	decocass_dongle_r = NULL;
	decocass_dongle_w = NULL;

	decocass_reset = 0;
	i8041_p1 = 0xff;
	i8041_p2 = 0xff;

	type1_inmap = MAKE_MAP(0,1,2,3,4,5,6,7);
	type1_outmap = MAKE_MAP(0,1,2,3,4,5,6,7);

	type2_d2_latch = 0;
	type2_xx_latch = 0;
	type2_promaddr = 0;

	type3_ctrs = 0;
	type3_d0_latch = 0;
	type3_pal_19 = 0;
	type3_swap = 0;

	memset(decocass_quadrature_decoder, 0, sizeof(decocass_quadrature_decoder));
	decocass_sound_ack = 0;
	decocass_sound_timer = timer_alloc(decocass_sound_nmi_pulse);

	/* state saving code */
	state_save_register_func_postload(decocass_state_save_postload);
	state_save_register_int 	("decocass", 0, "tape_dir", &tape_dir);
	state_save_register_int 	("decocass", 0, "tape_speed", &tape_speed);
	state_save_register_double	("decocass", 0, "tape_time0", &tape_time0, 1);
	state_save_register_int 	("decocass", 0, "firsttime", &firsttime);
	state_save_register_int 	("decocass", 0, "tape_present", &tape_present);
	state_save_register_int 	("decocass", 0, "tape_blocks", &tape_blocks);
	state_save_register_int 	("decocass", 0, "tape_length", &tape_length);
	state_save_register_int 	("decocass", 0, "tape_bot_eot", &tape_bot_eot);
	state_save_register_UINT8	("decocass", 0, "crc16_lsb", &crc16_lsb, 1);
	state_save_register_UINT8	("decocass", 0, "crc16_msb", &crc16_msb, 1);
	state_save_register_UINT8	("decocass", 0, "tape_crc16_lsb", tape_crc16_lsb, 256);
	state_save_register_UINT8	("decocass", 0, "tape_crc16_msb", tape_crc16_msb, 256);
	state_save_register_UINT8	("decocass", 0, "decocass_reset", &decocass_reset, 1);
	state_save_register_UINT8	("decocass", 0, "i8041_p1", &i8041_p1, 1);
	state_save_register_UINT8	("decocass", 0, "i8041_p2", &i8041_p2, 1);
	state_save_register_UINT32	("decocass", 0, "type1_inmap", &type1_inmap, 1);
	state_save_register_UINT32	("decocass", 0, "type1_outmap", &type1_outmap, 1);
	state_save_register_int 	("decocass", 0, "type2_d2_latch", &type2_d2_latch);
	state_save_register_int 	("decocass", 0, "type2_xx_latch", &type2_xx_latch);
	state_save_register_int 	("decocass", 0, "type2_promaddr", &type2_promaddr);
	state_save_register_int 	("decocass", 0, "type3_ctrs", &type3_ctrs);
	state_save_register_int 	("decocass", 0, "type3_d0_latch", &type3_d0_latch);
	state_save_register_int 	("decocass", 0, "type3_pal_19", &type3_pal_19);
	state_save_register_int 	("decocass", 0, "type3_swap", &type3_swap);
	state_save_register_int 	("decocass", 0, "type4_ctrs", &type4_ctrs);
	state_save_register_int 	("decocass", 0, "type4_latch", &type4_latch);
	state_save_register_int 	("decocass", 0, "type5_latch", &type5_latch);
	state_save_register_UINT8	("decocass", 0, "decocass_sound_ack", &decocass_sound_ack, 1);
}

MACHINE_INIT( decocass )
{
	decocass_init_common();
}

MACHINE_INIT( ctsttape )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061)\n"));
	decocass_dongle_r = decocass_type1_map1_r;
}

MACHINE_INIT( chwy )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (ALT)\n"));
	decocass_dongle_r = decocass_type1_alt_r;
}

MACHINE_INIT( clocknch )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061 flip 2-3)\n"));
	decocass_dongle_r = decocass_type1_r;
	type1_inmap = MAKE_MAP(0,1,3,2,4,5,6,7);
	type1_outmap = MAKE_MAP(0,1,3,2,4,5,6,7);
}

MACHINE_INIT( ctisland )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061 flip 0-2)\n"));
	decocass_dongle_r = decocass_type1_r;
	type1_inmap = MAKE_MAP(2,1,0,3,4,5,6,7);
	type1_outmap = MAKE_MAP(2,1,0,3,4,5,6,7);
}

MACHINE_INIT( csuperas )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061 flip 4-5)\n"));
	decocass_dongle_r = decocass_type1_r;
	type1_inmap = MAKE_MAP(0,1,2,3,5,4,6,7);
	type1_outmap = MAKE_MAP(0,1,2,3,5,4,6,7);
}

MACHINE_INIT( castfant )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061 flip 1-2)\n"));
	decocass_dongle_r = decocass_type1_map3_r;
}

MACHINE_INIT( cluckypo )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061 flip 1-3)\n"));
	decocass_dongle_r = decocass_type1_r;
	type1_inmap = MAKE_MAP(0,3,2,1,4,5,6,7);
	type1_outmap = MAKE_MAP(0,3,2,1,4,5,6,7);
}

MACHINE_INIT( cterrani )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061 straight)\n"));
	decocass_dongle_r = decocass_type1_r;
	type1_inmap = MAKE_MAP(0,1,2,3,4,5,6,7);
	type1_outmap = MAKE_MAP(0,1,2,3,4,5,6,7);
}

MACHINE_INIT( cexplore )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061)\n"));
	decocass_dongle_r = decocass_type1_map2_r;
}

MACHINE_INIT( cprogolf )
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061 flip 0-1)\n"));
	decocass_dongle_r = decocass_type1_r;
	type1_inmap = MAKE_MAP(1,0,2,3,4,5,6,7);
	type1_outmap = MAKE_MAP(1,0,2,3,4,5,6,7);
}

MACHINE_INIT(cmanhat)
{
	decocass_init_common();
	LOG(0,("dongle type #1 (DE-0061)\n"));
	decocass_dongle_r = decocass_type1_latch_xab_pass_x54_r;
}

MACHINE_INIT( cmissnx )
{
	decocass_init_common();
	LOG(0,("dongle type #2 (CS82-007)\n"));
	decocass_dongle_r = decocass_type2_r;
	decocass_dongle_w = decocass_type2_w;
}

MACHINE_INIT( cdiscon1 )
{
	decocass_init_common();
	LOG(0,("dongle type #2 (CS82-007)\n"));
	decocass_dongle_r = decocass_type2_r;
	decocass_dongle_w = decocass_type2_w;
}

MACHINE_INIT( cptennis )
{
	decocass_init_common();
	LOG(0,("dongle type #2 (CS82-007)\n"));
	decocass_dongle_r = decocass_type2_r;
	decocass_dongle_w = decocass_type2_w;
}

MACHINE_INIT( ctornado )
{
	decocass_init_common();
	LOG(0,("dongle type #2 (CS82-007)\n"));
	decocass_dongle_r = decocass_type2_r;
	decocass_dongle_w = decocass_type2_w;
}

MACHINE_INIT( cbnj )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_67;
}

MACHINE_INIT( cburnrub )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_67;
}

MACHINE_INIT( cbtime )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_12;
}

MACHINE_INIT( cgraplop )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_56;
}

MACHINE_INIT( clapapa )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_34_7;
}

MACHINE_INIT( cfghtice )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_25;
}

MACHINE_INIT( cskater )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_45;
}

MACHINE_INIT( cprobowl )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_34_0;
}

MACHINE_INIT( cnightst )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_13;
}

MACHINE_INIT( cprosocc )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_24;
}

MACHINE_INIT( cppicf )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_01;
}

MACHINE_INIT( cscrtry )
{
	decocass_init_common();
	LOG(0,("dongle type #4 (32K ROM)\n"));
	decocass_dongle_r = decocass_type4_r;
	decocass_dongle_w = decocass_type4_w;
}

MACHINE_INIT( cbdash )
{
	decocass_init_common();
	LOG(0,("dongle type #5 (NOP)\n"));
	decocass_dongle_r = decocass_type5_r;
	decocass_dongle_w = decocass_type5_w;
}

MACHINE_INIT( czeroize )
{
	UINT8 *mem = memory_region(REGION_USER1);
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_23_56;

	/*
     * FIXME: remove if the original ROM is available.
     * The Zeroize 6502 code at 0x3707 issues LODCTRS with 0x8a,
     * and expects to read 0x18 from 0x08a0 ff. within 7 bytes.
     * This hack seems to be sufficient to get around
     * the missing dongle ROM contents and play the game.
     */
	memset(mem,0x00,0x1000);
	mem[0x08a0] = 0x18;
	mem[0x08a1] = 0xf7;
}

MACHINE_INIT( csdtenis )
{
	decocass_init_common();
	LOG(0,("dongle type #3 (PAL)\n"));
	decocass_dongle_r = decocass_type3_r;
	decocass_dongle_w = decocass_type3_w;
	type3_swap = TYPE3_SWAP_23_56;
}

/***************************************************************************
 *
 *	8041 port handlers
 *
 ***************************************************************************/

static void tape_stop(void)
{
	/* remember time */
	tape_time0 += tape_dir * timer_timeelapsed(tape_timer);
	timer_adjust(tape_timer, TIME_NEVER, 0, 0);
}


WRITE_HANDLER( i8041_p1_w )
{
	static int i8041_p1_old;

	if (data != i8041_p1_old)
	{
		LOG(4,("%9.7f 8041-PC: %03x i8041_p1_w: $%02x (%s%s%s%s%s%s%s%s)\n",
			timer_get_time(),
			activecpu_get_previouspc(),
			data,
			data & 0x01 ? "" : "DATA-WRT",
			data & 0x02 ? "" : " DATA-CLK",
			data & 0x04 ? "" : " FAST",
			data & 0x08 ? "" : " BIT3",
			data & 0x10 ? "" : " REW",
			data & 0x20 ? "" : " FWD",
			data & 0x40 ? "" : " WREN",
			data & 0x80 ? "" : " REQ"));
		i8041_p1_old = data;
	}

	/* change in REW signal ? */
	if ((data ^ i8041_p1) & 0x10)
	{
		tape_stop();
		if (0 == (data & 0x10))
		{
			LOG(2,("tape %5.4fs: rewind\n", tape_time0));
			tape_dir = -1;
			timer_adjust(tape_timer, TIME_NEVER, 0, 0);
			set_led_status(0, 1);
		}
		else
		{
			tape_dir = 0;
			tape_speed = 0;
			LOG(2,("tape %5.4fs: stopped\n", tape_time0));
#if TAPE_UI_DISPLAY
			usrintf_showmessage("   [%05.1fs]   ", tape_time0);
#endif
			set_led_status(0, 0);
		}
	}

	/* change in FWD signal ? */
	if ((data ^ i8041_p1) & 0x20)
	{
		tape_stop();
		if (0 == (data & 0x20))
		{
			LOG(2,("tape %5.4fs: forward\n", tape_time0));
			tape_dir = +1;
			timer_adjust(tape_timer, TIME_NEVER, 0, 0);
			set_led_status(0, 1);
		}
		else
		{
			tape_dir = 0;
			tape_speed = 0;
			LOG(2,("tape %5.4fs: stopped\n", tape_time0));
#if TAPE_UI_DISPLAY
			usrintf_showmessage("   [%05.1fs]   ", tape_time0);
#endif
			set_led_status(0, 0);
		}
	}

	/* change in FAST signal ? */
	if (tape_timer && (data ^ i8041_p1) & 0x04)
	{
		tape_stop();
		tape_speed = (0 == (data & 0x04)) ? 1 : 0;

		if (tape_dir < 0)
		{
			LOG(2,("tape: fast rewind %s\n", (0 == (data & 0x04)) ? "on" : "off"));
			tape_dir = (tape_speed) ? -7 : -1;
			timer_adjust(tape_timer, TIME_NEVER, 0, 0);
		}
		else
		if (tape_dir > 0)
		{
			LOG(2,("tape: fast forward %s\n", (0 == (data & 0x04)) ? "on" : "off"));
			tape_dir = (tape_speed) ? +7 : +1;
			timer_adjust(tape_timer, TIME_NEVER, 0, 0);
		}
	}

	i8041_p1 = data;
}

READ_HANDLER( i8041_p1_r )
{
	data8_t data = i8041_p1;
	static int i8041_p1_old;

	if (data != i8041_p1_old)
	{
		LOG(4,("%9.7f 8041-PC: %03x i8041_p1_r: $%02x (%s%s%s%s%s%s%s%s)\n",
			timer_get_time(),
			activecpu_get_previouspc(),
			data,
			data & 0x01 ? "" : "DATA-WRT",
			data & 0x02 ? "" : " DATA-CLK",
			data & 0x04 ? "" : " FAST",
			data & 0x08 ? "" : " BIT3",
			data & 0x10 ? "" : " REW",
			data & 0x20 ? "" : " FWD",
			data & 0x40 ? "" : " WREN",
			data & 0x80 ? "" : " REQ"));
		i8041_p1_old = data;
	}
	return data;
}

WRITE_HANDLER( i8041_p2_w )
{
	static int i8041_p2_old;

	if (data != i8041_p2_old)
	{
		LOG(4,("%9.7f 8041-PC: %03x i8041_p2_w: $%02x (%s%s%s%s%s%s%s%s)\n",
			timer_get_time(),
			activecpu_get_previouspc(),
			data,
			data & 0x01 ? "" : "FNO/",
			data & 0x02 ? "" : " EOT/",
			data & 0x04 ? "" : " ERR/",
			data & 0x08 ? "" : " OUT3?/",
			data & 0x10 ? " [IN4]" : "",
			data & 0x20 ? " [BOT-EOT]" : "",
			data & 0x40 ? " [RCLK]" : "",
			data & 0x80 ? " [RDATA]" : ""));
		i8041_p2_old = data;
	}
	i8041_p2 = data;
}

READ_HANDLER( i8041_p2_r )
{
	data8_t data;
	static int i8041_p2_old;

	tape_update();

	data = i8041_p2;

	if (data != i8041_p2_old)
	{
		LOG(4,("%9.7f 8041-PC: %03x i8041_p2_r: $%02x (%s%s%s%s%s%s%s%s)\n",
			timer_get_time(),
			activecpu_get_previouspc(),
			data,
			data & 0x01 ? "" : "FNO/",
			data & 0x02 ? "" : " EOT/",
			data & 0x04 ? "" : " ERR/",
			data & 0x08 ? "" : " OUT3?/",
			data & 0x10 ? " [IN4]" : "",
			data & 0x20 ? " [BOT-EOT]" : "",
			data & 0x40 ? " [RCLK]" : "",
			data & 0x80 ? " [RDATA]" : ""));
		i8041_p2_old = data;
	}
	return data;
}
