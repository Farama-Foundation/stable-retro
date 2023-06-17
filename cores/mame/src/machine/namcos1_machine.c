#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/random.h"

#define NEW_TIMER 0 /* CPU slice optimize with new timer system */

#define NAMCOS1_MAX_BANK 0x400

#define USE_MTRANDOM 1

/* from vidhrdw */
READ_HANDLER( namcos1_videoram_r );
WRITE_HANDLER( namcos1_videoram_w );
READ_HANDLER( namcos1_paletteram_r );
WRITE_HANDLER( namcos1_paletteram_w );
WRITE_HANDLER( namcos1_videocontrol_w );
extern void namcos1_set_scroll_offsets( const int *bgx, const int *bgy, int negative, int optimize );
extern void namcos1_set_sprite_offsets( int x, int y );

#define NAMCOS1_MAX_KEY 0x100
static unsigned char key[NAMCOS1_MAX_KEY];

static unsigned char *s1ram;

static int namcos1_cpu1_banklatch;
static int namcos1_reset = 0;

static int berabohm_input_counter;
int namcos1_game_id = 0;

/*******************************************************************************
*                                                                              *
*   BANK area handling                                                          *
*                                                                              *
*******************************************************************************/

/* Bank handler definitions */
typedef struct {
	mem_read_handler bank_handler_r;
	mem_write_handler bank_handler_w;
	int           bank_offset;
	unsigned char *bank_pointer;
} bankhandler;

/* hardware elements of 1Mbytes physical memory space */
static bankhandler namcos1_bank_element[NAMCOS1_MAX_BANK];

static const mem_read_handler org_bank_handler_r[16] =
{
	MRA_BANK1 ,MRA_BANK2 ,MRA_BANK3 ,MRA_BANK4 ,
	MRA_BANK5 ,MRA_BANK6 ,MRA_BANK7 ,MRA_BANK8 ,
	MRA_BANK9 ,MRA_BANK10,MRA_BANK11,MRA_BANK12,
	MRA_BANK13,MRA_BANK14,MRA_BANK15,MRA_BANK16
};

static const mem_write_handler org_bank_handler_w[16] =
{
	MWA_BANK1 ,MWA_BANK2 ,MWA_BANK3 ,MWA_BANK4 ,
	MWA_BANK5 ,MWA_BANK6 ,MWA_BANK7 ,MWA_BANK8 ,
	MWA_BANK9 ,MWA_BANK10,MWA_BANK11,MWA_BANK12,
	MWA_BANK13,MWA_BANK14,MWA_BANK15,MWA_BANK16
};



/*******************************************************************************
*                                                                              *
*   Key emulation (CUS136) Rev1 (Pacmania & Galaga 88)                         *
*                                                                              *
*******************************************************************************/

static int key_id;
static int key_id_query;

static READ_HANDLER( rev1_key_r ) {
	/*logerror("CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,key[offset]);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);
		return 0;
	}
	return key[offset];
}

static WRITE_HANDLER( rev1_key_w ) {
	static unsigned short divider, divide_32 = 0;
	/*logerror("CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip write %04x=%04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);
		return;
	}

	key[offset] = data;

	switch ( offset )
	{
	case 0x01:
		divider = ( key[0] << 8 ) + key[1];
		break;
	case 0x03:
		{
			static unsigned short d;
			unsigned short  v1, v2;
			unsigned long   l=0;

			if ( divide_32 )
				l = d << 16;

			d = ( key[2] << 8 ) + key[3];

			if ( divider == 0 ) {
				v1 = 0xffff;
				v2 = 0;
			} else {
				if ( divide_32 ) {
					l |= d;

					v1 = l / divider;
					v2 = l % divider;
				} else {
					v1 = d / divider;
					v2 = d % divider;
				}
			}

			key[2] = v1 >> 8;
			key[3] = v1;
			key[0] = v2 >> 8;
			key[1] = v2;
		}
		break;
	case 0x04:
		if ( key[4] == key_id_query ) /* get key number */
			key[4] = key_id;

		if ( key[4] == 0x0c )
			divide_32 = 1;
		else
			divide_32 = 0;
		break;
	}
}

/*******************************************************************************
*                                                                              *
*   Key emulation (CUS136) Rev2 (Dragon Spirit, Blazer, World Court)           *
*                                                                              *
*******************************************************************************/

static READ_HANDLER( rev2_key_r )
{
	/*logerror("CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,key[offset]);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);
		return 0;
	}
	return key[offset];
}

static WRITE_HANDLER( rev2_key_w )
{
	/*logerror("CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip write %04x=%04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);
		return;
	}
	key[offset] = data;

	switch(offset)
	{
	case 0x00:
		if ( data == 1 )
		{
			/* fetch key ID */
			key[3] = key_id;
			return;
		}
		break;
	case 0x02:
		/* $f2 = Dragon Spirit, $b7 = Blazer , $35($d9) = worldcourt */
		if ( key[3] == 0xf2 || key[3] == 0xb7 || key[3] == 0x35 )
		{
			switch( key[0] )
			{
				case 0x10: key[0] = 0x05; key[1] = 0x00; key[2] = 0xc6; break;
				case 0x12: key[0] = 0x09; key[1] = 0x00; key[2] = 0x96; break;
				case 0x15: key[0] = 0x0a; key[1] = 0x00; key[2] = 0x8f; break;
				case 0x22: key[0] = 0x14; key[1] = 0x00; key[2] = 0x39; break;
				case 0x32: key[0] = 0x31; key[1] = 0x00; key[2] = 0x12; break;
				case 0x3d: key[0] = 0x35; key[1] = 0x00; key[2] = 0x27; break;
				case 0x54: key[0] = 0x10; key[1] = 0x00; key[2] = 0x03; break;
				case 0x58: key[0] = 0x49; key[1] = 0x00; key[2] = 0x23; break;
				case 0x7b: key[0] = 0x48; key[1] = 0x00; key[2] = 0xd4; break;
				case 0xc7: key[0] = 0xbf; key[1] = 0x00; key[2] = 0xe8; break;
			}
			return;
		}
		break;
	case 0x03:
		/* $c2 = Dragon Spirit, $b6 = Blazer */
		if ( key[3] == 0xc2 || key[3] == 0xb6 ) {
			key[3] = 0x36;
			return;
		}
		/* $d9 = World court */
		if ( key[3] == 0xd9 )
		{
			key[3] = 0x35;
			return;
		}
		break;
	case 0x3f:  /* Splatter House */
		key[0x3f] = 0xb5;
		key[0x36] = 0xb5;
		return;
	}
	/* ?? */
	if ( key[3] == 0x01 ) {
		if ( key[0] == 0x40 && key[1] == 0x04 && key[2] == 0x00 ) {
			key[1] = 0x00; key[2] = 0x10;
			return;
		}
	}
}

/*******************************************************************************
*                                                                              *
*   Key emulation (CUS136) for Dangerous Seed                                  *
*                                                                              *
*******************************************************************************/

static READ_HANDLER( dangseed_key_r ) {
	/*logerror("CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,key[offset]);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);
		return 0;
	}
	return key[offset];
}

static WRITE_HANDLER( dangseed_key_w ) {
	int i;
	/*logerror("CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip write %04x=%04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);
		return;
	}

	key[offset] = data;

	switch ( offset )
	{
		case 0x50:
			for ( i = 0; i < 0x50; i++ ) {
				key[i] = ( data >> ( ( i >> 4 ) & 0x0f ) ) & 0x0f;
				key[i] |= ( i & 0x0f ) << 4;
			}
			break;

		case 0x57:
			key[3] = key_id;
			break;
	}
}

/*******************************************************************************
*                                                                              *
*   Key emulation (CUS136) for Dragon Spirit                                   *
*                                                                              *
*******************************************************************************/

static READ_HANDLER( dspirit_key_r )
{
	/*logerror("CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,key[offset]);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);
		return 0;
	}
	return key[offset];
}

static WRITE_HANDLER( dspirit_key_w )
{
	static unsigned short divisor;
	/*logerror("CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip write %04x=%04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);
		return;
	}
	key[offset] = data;

	switch(offset)
	{
	case 0x00:
		if ( data == 1 )
		{
			/* fetch key ID */
			key[3] = key_id;
		} else
			divisor = data;
		break;

	case 0x01:
		if ( key[3] == 0x01 ) { /* division gets resolved on latch to $1 */
			unsigned short d, v1, v2;

			d = ( key[1] << 8 ) + key[2];

			if ( divisor == 0 ) {
				v1 = 0xffff;
				v2 = 0;
			} else {
				v1 = d / divisor;
				v2 = d % divisor;
			}

			key[0] = v2 & 0xff;
			key[1] = v1 >> 8;
			key[2] = v1 & 0xff;

			return;
		}

		if ( key[3] != 0xf2 ) { /* if its an invalid mode, clear regs */
			key[0] = 0;
			key[1] = 0;
			key[2] = 0;
		}
		break;
	case 0x02:
		if ( key[3] == 0xf2 ) { /* division gets resolved on latch to $2 */
			unsigned short d, v1, v2;

			d = ( key[1] << 8 ) + key[2];

			if ( divisor == 0 ) {
				v1 = 0xffff;
				v2 = 0;
			} else {
				v1 = d / divisor;
				v2 = d % divisor;
			}

			key[0] = v2 & 0xff;
			key[1] = v1 >> 8;
			key[2] = v1 & 0xff;

			return;
		}

		if ( key[3] != 0x01 ) { /* if its an invalid mode, clear regs */
			key[0] = 0;
			key[1] = 0;
			key[2] = 0;
		}
		break;
	case 0x03:
		if ( key[3] != 0xf2 && key[3] != 0x01 ) /* if the mode is unknown return the id on $3 */
			key[3] = key_id;
		break;
	}
}

/*******************************************************************************
*                                                                              *
*   Key emulation (CUS136) for Blazer                                          *
*                                                                              *
*******************************************************************************/

static READ_HANDLER( blazer_key_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,key[offset]);
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);
		return 0;
	}
	return key[offset];
}

static WRITE_HANDLER( blazer_key_w )
{
	static unsigned short divisor;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip write %04x=%04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);
		return;
	}
	key[offset] = data;

	switch(offset)
	{
	case 0x00:
		if ( data == 1 )
		{
			/* fetch key ID */
			key[3] = key_id;
		} else
			divisor = data;
		break;

	case 0x01:
		if ( key[3] != 0xb7 ) { /* if its an invalid mode, clear regs */
			key[0] = 0;
			key[1] = 0;
			key[2] = 0;
		}
		break;

	case 0x02:
		if ( key[3] == 0xb7 ) { /* division gets resolved on latch to $2 */
			unsigned short d, v1, v2;

			d = ( key[1] << 8 ) + key[2];

			if ( divisor == 0 ) {
				v1 = 0xffff;
				v2 = 0;
			} else {
				v1 = d / divisor;
				v2 = d % divisor;
			}

			key[0] = v2 & 0xff;
			key[1] = v1 >> 8;
			key[2] = v1 & 0xff;

			return;
		}

		/* if its an invalid mode, clear regs */
		key[0] = 0;
		key[1] = 0;
		key[2] = 0;
		break;
	case 0x03:
		if ( key[3] != 0xb7 ) { /* if the mode is unknown return the id on $3 */
			key[3] = key_id;
		}
		break;
	}
}

/*******************************************************************************
*                                                                              *
*   Key emulation (CUS136) for World Stadium                                   *
*                                                                              *
*******************************************************************************/

static READ_HANDLER( ws_key_r ) {
	/*logerror("CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,key[offset]);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);
		return 0;
	}
	return key[offset];
}

static WRITE_HANDLER( ws_key_w ) {
	static unsigned short divider;
	/*logerror("CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);*/
	if(offset >= NAMCOS1_MAX_KEY)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %08x: unmapped keychip write %04x=%04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);
		return;
	}

	key[offset] = data;

	switch ( offset )
	{
	case 0x01:
		divider = ( key[0] << 8 ) + key[1];
		break;
	case 0x03:
		{
			static unsigned short d;
			unsigned short  v1, v2;

			d = ( key[2] << 8 ) + key[3];

			if ( divider == 0 ) {
				v1 = 0xffff;
				v2 = 0;
			} else {
				v1 = d / divider;
				v2 = d % divider;
			}

			key[2] = v1 >> 8;
			key[3] = v1;
			key[0] = v2 >> 8;
			key[1] = v2;
		}
		break;
	case 0x04:
		key[4] = key_id;
		break;
	}
}

/*******************************************************************************
*                                                                              *
*   Key emulation (CUS181) for SplatterHouse                                   *
*                                                                              *
*******************************************************************************/

#ifdef USE_MTRANDOM

/* Courtesy of Shawn J. Cokus, University of Washington*/

typedef unsigned long uint32;

#define N              (624)                 /* length of state vector*/
#define M              (397)                 /* a period parameter*/
#define K              (0x9908B0DFU)         /* a magic constant*/
#define hiBit(u)       ((u) & 0x80000000U)   /* mask all but highest   bit of u*/
#define loBit(u)       ((u) & 0x00000001U)   /* mask all but lowest    bit of u*/
#define loBits(u)      ((u) & 0x7FFFFFFFU)   /* mask     the highest   bit of u*/
#define mixBits(u, v)  (hiBit(u)|loBits(v))  /* move hi bit of u to hi bit of v*/

static uint32   state[N+1];     /* state vector + 1 extra to not violate ANSI C*/
static uint32   *next;          /* next random value is computed from here*/
static int      left = -1;      /* can *next++ this many times before reloading*/

static void seedMT(uint32 seed)
{
	register uint32 x = (seed | 1U) & 0xFFFFFFFFU, *s = state;
	register int    j;

	for(left=0, *s++=x, j=N; --j;
		*s++ = (x*=69069U) & 0xFFFFFFFFU);
}

static uint32 reloadMT(void)
{
	register uint32 *p0=state, *p2=state+2, *pM=state+M, s0, s1;
	register int    j;

	if(left < -1)
		seedMT(4357U);

	left=N-1, next=state+1;

	for(s0=state[0], s1=state[1], j=N-M+1; --j; s0=s1, s1=*p2++)
		*p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

	for(pM=state, j=M; --j; s0=s1, s1=*p2++)
		*p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

	s1=state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
	s1 ^= (s1 >> 11);
	s1 ^= (s1 <<  7) & 0x9D2C5680U;
	s1 ^= (s1 << 15) & 0xEFC60000U;
	return(s1 ^ (s1 >> 18));
}

static INLINE uint32 randomMT(void)
{
	uint32 y;

	if(--left < 0)
		return(reloadMT());

	y  = *next++;
	y ^= (y >> 11);
	y ^= (y <<  7) & 0x9D2C5680U;
	y ^= (y << 15) & 0xEFC60000U;
	return(y ^ (y >> 18));
}

#endif

static READ_HANDLER( splatter_key_r ) {

	unsigned long data;

	/*logerror("CPU #%d PC %08x: keychip read %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,key[offset]);*/

	switch( ( offset >> 4 ) & 0x07 ) {
		case 0x00:
		case 0x06:
			return 0xff;
			break;

		case 0x01:
		case 0x02:
		case 0x05:
		case 0x07:
			return ( ( offset & 0x0f ) << 4 ) | 0x0f;
			break;

		case 0x03:
			return 0xb5;
			break;

		case 0x04:
			{
#ifdef USE_MTRANDOM
				data = randomMT() & 0xff;
#else
				data = mame_rand() & 0xff;
#endif
				if ( offset >= 0x1000 )
					data |= 0x80;
				if ( offset >= 0x2000 )
					data |= 0x04;

				return (data);
			}
			break;
	}

	/* make compiler happy */
	return 0;
}

static WRITE_HANDLER( splatter_key_w ) {
	/*logerror("CPU #%d PC %08x: keychip write %04X=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);*/
	/* ignored */
}


/*******************************************************************************
*                                                                              *
*   Banking emulation (CUS117)                                                 *
*                                                                              *
*******************************************************************************/

static READ_HANDLER( soundram_r )
{
	if(offset<0x100)
		return namcos1_wavedata_r(offset);
	if(offset<0x140)
		return namcos1_sound_r(offset-0x100);

	/* shared ram */
	return namco_wavedata[offset];
}

static WRITE_HANDLER( soundram_w )
{
	if(offset<0x100)
	{
		namcos1_wavedata_w(offset,data);
		return;
	}
	if(offset<0x140)
	{
		namcos1_sound_w(offset-0x100,data);
		return;
	}
	/* shared ram */
	namco_wavedata[offset] = data;

	/*if(offset>=0x1000)*/
	/*  log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %04x: write shared ram %04x=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);*/
}

/* ROM handlers */

static WRITE_HANDLER( rom_w ) {
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %04x: warning - write %02x to rom address %04x\n",cpu_getactivecpu(),activecpu_get_pc(),data,offset);
}

/* error handlers */
static READ_HANDLER( unknown_r ) {
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %04x: warning - read from unknown chip\n",cpu_getactivecpu(),activecpu_get_pc() );
	return 0;
}

static WRITE_HANDLER( unknown_w ) {
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %04x: warning - wrote to unknown chip\n",cpu_getactivecpu(),activecpu_get_pc() );
}

/* Main bankswitching routine */
void namcos1_bankswitch(int cpu, offs_t offset, data8_t data)
{
	static int chip = 0;
	mem_read_handler handler_r;
	mem_write_handler handler_w;
	offs_t offs;

	if ( offset & 1 ) {
		int bank = (cpu*8) + ( ( offset >> 9 ) & 0x07 );

		chip &= 0x0300;
		chip |= ( data & 0xff );

		/* for BANK handlers , memory direct and OP-code base */
		cpu_setbank(bank+1,namcos1_bank_element[chip].bank_pointer);

		/* Addition OFFSET for stub handlers */
		offs = namcos1_bank_element[chip].bank_offset;

		/* read hardware */
		handler_r = namcos1_bank_element[chip].bank_handler_r;
		if( handler_r )
			/* I/O handler */
			memory_set_bankhandler_r( bank+1,offs,handler_r);
		else    /* memory direct */
			memory_set_bankhandler_r( bank+1,0,org_bank_handler_r[bank] );

		/* write hardware */
		handler_w = namcos1_bank_element[chip].bank_handler_w;
		if( handler_w )
			/* I/O handler */
			memory_set_bankhandler_w( bank+1,offs,handler_w);
		else    /* memory direct */
			memory_set_bankhandler_w( bank+1,0,org_bank_handler_w[bank] );

		/* unmapped bank warning */
		if( handler_r == unknown_r)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC %04x:warning unknown chip selected bank %x=$%04x\n", cpu , activecpu_get_pc(), bank , chip );
		}

		/* renew pc base */
	/*change_pc16(activecpu_get_pc());*/
	} else {
		chip &= 0x00ff;
		chip |= ( data & 0xff ) << 8;
	}
}

WRITE_HANDLER( namcos1_bankswitch_w ) {
	namcos1_bankswitch(cpu_getactivecpu(), offset, data);
}

/* Sub cpu set start bank port */
WRITE_HANDLER( namcos1_subcpu_bank_w )
{
	/*logerror("cpu1 bank selected %02x=%02x\n",offset,data);*/
	namcos1_cpu1_banklatch = (namcos1_cpu1_banklatch&0x300)|data;
	/* Prepare code for Cpu 1 */
	namcos1_bankswitch( 1, 0x0e00, namcos1_cpu1_banklatch>>8  );
	namcos1_bankswitch( 1, 0x0e01, namcos1_cpu1_banklatch&0xff);
	/* cpu_set_reset_line(1,PULSE_LINE); */

}

/*******************************************************************************
*                                                                              *
*   63701 MCU emulation (CUS64)                                                *
*                                                                              *
*******************************************************************************/

static int mcu_patch_data;

WRITE_HANDLER( namcos1_cpu_control_w )
{
	/*logerror("reset control pc=%04x %02x\n",activecpu_get_pc(),data);*/
	if( (data&1)^namcos1_reset)
	{
		namcos1_reset = data&1;
		if (namcos1_reset)
		{
			cpu_set_reset_line(1,CLEAR_LINE);
			cpu_set_reset_line(2,CLEAR_LINE);
			cpu_set_reset_line(3,CLEAR_LINE);
			mcu_patch_data = 0;
		}
		else
		{
			cpu_set_reset_line(1,ASSERT_LINE);
			cpu_set_reset_line(2,ASSERT_LINE);
			cpu_set_reset_line(3,ASSERT_LINE);
		}
	}
}

/*******************************************************************************
*                                                                              *
*   Sound banking emulation (CUS121)                                           *
*                                                                              *
*******************************************************************************/

WRITE_HANDLER( namcos1_sound_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	int bank = ( data >> 4 ) & 0x07;

	cpu_setbank( 17, &RAM[ 0x0c000 + ( 0x4000 * bank ) ] );
}

/*******************************************************************************
*                                                                              *
*   CPU idling spinlock routine                                                *
*                                                                              *
*******************************************************************************/
static unsigned char *sound_spinlock_ram;
static int sound_spinlock_pc;

/* sound cpu */
static READ_HANDLER( namcos1_sound_spinlock_r )
{
	if(activecpu_get_pc()==sound_spinlock_pc && *sound_spinlock_ram == 0)
		cpu_spinuntil_int();
	return *sound_spinlock_ram;
}

/*******************************************************************************
*                                                                              *
*   MCU banking emulation and patch                                            *
*                                                                              *
*******************************************************************************/

/* mcu banked rom area select */
WRITE_HANDLER( namcos1_mcu_bankswitch_w )
{
	int addr;
	/* bit 2-7 : chip select line of ROM chip */
	switch(data&0xfc)
	{
	case 0xf8: addr = 0x10000; break; /* bit 2 : ROM 0 */
	case 0xf4: addr = 0x30000; break; /* bit 3 : ROM 1 */
	case 0xec: addr = 0x50000; break; /* bit 4 : ROM 2 */
	case 0xdc: addr = 0x70000; break; /* bit 5 : ROM 3 */
	case 0xbc: addr = 0x90000; break; /* bit 6 : ROM 4 */
	case 0x7c: addr = 0xb0000; break; /* bit 7 : ROM 5 */
	default:   addr = 0x100000; /* illegal */
	}
	/* bit 0-1 : address line A15-A16 */
	addr += (data&3)*0x8000;
	if( addr >= memory_region_length(REGION_CPU4))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "unmapped mcu bank selected pc=%04x bank=%02x\n",activecpu_get_pc(),data);
		addr = 0x4000;
	}
	cpu_setbank( 20, memory_region(REGION_CPU4)+addr );
}

/* This point is very obscure, but i havent found any better way yet. */
/* Works with all games so far.                                       */

/* patch points of memory address */
/* CPU0/1 bank[17f][1000] */
/* CPU2   [7000]      */
/* CPU3   [c000]      */

/* This memory point should be set $A6 by anywhere, but         */
/* I found set $A6 only initialize in MCU                       */
/* This patch kill write this data by MCU case $A6 to xx(clear) */

WRITE_HANDLER( namcos1_mcu_patch_w )
{
	/*logerror("mcu C000 write pc=%04x data=%02x\n",activecpu_get_pc(),data);*/
	if(mcu_patch_data == 0xa6) return;
	mcu_patch_data = data;
	cpu_bankbase[19][offset] = data;
}

/*******************************************************************************
*                                                                              *
*   Initialization                                                             *
*                                                                              *
*******************************************************************************/

static void namcos1_install_bank(int start,int end,mem_read_handler hr,mem_write_handler hw,
			  int offset,unsigned char *pointer)
{
	int i;
	for(i=start;i<=end;i++)
	{
		namcos1_bank_element[i].bank_handler_r = hr;
		namcos1_bank_element[i].bank_handler_w = hw;
		namcos1_bank_element[i].bank_offset    = offset;
		namcos1_bank_element[i].bank_pointer   = pointer;
		offset  += 0x2000;
		if(pointer) pointer += 0x2000;
	}
}

static void namcos1_install_rom_bank(int start,int end,int size,int offset)
{
	unsigned char *BROM = memory_region(REGION_USER1);
	int step = size/0x2000;
	while(start < end)
	{
		namcos1_install_bank(start,start+step-1,0,rom_w,0,&BROM[offset]);
		start += step;
	}
}

static void namcos1_build_banks(mem_read_handler key_r,mem_write_handler key_w)
{
	int i;

	/* S1 RAM pointer set */
	s1ram = memory_region(REGION_USER2);

	/* clear all banks to unknown area */
	for(i=0;i<NAMCOS1_MAX_BANK;i++)
		namcos1_install_bank(i,i,unknown_r,unknown_w,0,0);

	/* RAM 6 banks - palette */
	namcos1_install_bank(0x170,0x172,namcos1_paletteram_r,namcos1_paletteram_w,0,s1ram);
	/* RAM 6 banks - work ram */
	namcos1_install_bank(0x173,0x173,0,0,0,&s1ram[0x6000]);
	/* RAM 5 banks - videoram */
	namcos1_install_bank(0x178,0x17b,namcos1_videoram_r,namcos1_videoram_w,0,0);
	/* key chip bank (rev1_key_w / rev2_key_w ) */
	namcos1_install_bank(0x17c,0x17c,key_r,key_w,0,0);
	/* RAM 7 banks - display control, playfields, sprites */
	namcos1_install_bank(0x17e,0x17e,0,namcos1_videocontrol_w,0,&s1ram[0x8000]);
	/* RAM 1 shared ram, PSG device */
	namcos1_install_bank(0x17f,0x17f,soundram_r,soundram_w,0,namco_wavedata);
	/* RAM 3 banks */
	namcos1_install_bank(0x180,0x183,0,0,0,&s1ram[0xc000]);
	/* PRG0 */
	namcos1_install_rom_bank(0x200,0x23f,0x20000 , 0xe0000);
	/* PRG1 */
	namcos1_install_rom_bank(0x240,0x27f,0x20000 , 0xc0000);
	/* PRG2 */
	namcos1_install_rom_bank(0x280,0x2bf,0x20000 , 0xa0000);
	/* PRG3 */
	namcos1_install_rom_bank(0x2c0,0x2ff,0x20000 , 0x80000);
	/* PRG4 */
	namcos1_install_rom_bank(0x300,0x33f,0x20000 , 0x60000);
	/* PRG5 */
	namcos1_install_rom_bank(0x340,0x37f,0x20000 , 0x40000);
	/* PRG6 */
	namcos1_install_rom_bank(0x380,0x3bf,0x20000 , 0x20000);
	/* PRG7 */
	namcos1_install_rom_bank(0x3c0,0x3ff,0x20000 , 0x00000);
}

MACHINE_INIT( namcos1 ) {

	int bank;

	/* Point all of our bankhandlers to the error handlers */
	for( bank =0 ; bank < 2*8 ; bank++ )
	{
		/* set bank pointer & handler for cpu interface */
		memory_set_bankhandler_r( bank+1,0,unknown_r);
		memory_set_bankhandler_w( bank+1,0,unknown_w);
	}

	/* Prepare code for Cpu 0 */
	namcos1_bankswitch(0, 0x0e00, 0x03 ); /* bank7 = 0x3ff(PRG7) */
	namcos1_bankswitch(0, 0x0e01, 0xff );

	/* Prepare code for Cpu 1 */
	namcos1_bankswitch(1, 0x0e00, 0x03);
	namcos1_bankswitch(1, 0x0e01, 0xff);

	namcos1_cpu1_banklatch = 0x03ff;

	/* Point mcu & sound shared RAM to destination */
	{
		unsigned char *RAM = namco_wavedata + 0x1000; /* Ram 1, bank 1, offset 0x1000 */
		cpu_setbank( 18, RAM );
		cpu_setbank( 19, RAM );
	}

	/* In case we had some cpu's suspended, resume them now */
	cpu_set_reset_line(1,ASSERT_LINE);
	cpu_set_reset_line(2,ASSERT_LINE);
	cpu_set_reset_line(3,ASSERT_LINE);

	namcos1_reset = 0;
	/* mcu patch data clear */
	mcu_patch_data = 0;

	berabohm_input_counter = 4; /* for berabohm pressure sensitive buttons */
}


/*******************************************************************************
*                                                                              *
*   driver specific initialize routine                                         *
*                                                                              *
*******************************************************************************/
struct namcos1_slice_timer
{
	int sync_cpu;   /* synchronus cpu attribute */
	int sliceHz;    /* slice cycle              */
	int delayHz;    /* delay>=0 : delay cycle   */
					/* delay<0  : slide cycle   */
};

struct namcos1_specific
{
	/* keychip */
	int key_id_query , key_id;
	mem_read_handler key_r;
	mem_write_handler key_w;
	/* cpu slice timer */
	const struct namcos1_slice_timer *slice_timer;
};

static void namcos1_driver_init(const struct namcos1_specific *specific )
{
	/* keychip id */
	key_id_query = specific->key_id_query;
	key_id       = specific->key_id;

	/* build bank elements */
	namcos1_build_banks(specific->key_r,specific->key_w);

	/* sound cpu speedup optimize (auto detect) */
	{
		unsigned char *RAM = memory_region(REGION_CPU3); /* sound cpu */
		int addr,flag_ptr;

		for(addr=0xd000;addr<0xd0ff;addr++)
		{
			if(RAM[addr+0]==0xb6 &&   /* lda xxxx */
			   RAM[addr+3]==0x27 &&   /* BEQ addr */
			   RAM[addr+4]==0xfb )
			{
				flag_ptr = RAM[addr+1]*256 + RAM[addr+2];
				if(flag_ptr>0x5140 && flag_ptr<0x5400)
				{
					sound_spinlock_pc   = addr+3;
					sound_spinlock_ram  = install_mem_read_handler(2,flag_ptr,flag_ptr,namcos1_sound_spinlock_r);
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Set sound cpu spinlock : pc=%04x , addr = %04x\n",sound_spinlock_pc,flag_ptr);
					break;
				}
			}
		}
	}
#if NEW_TIMER
	/* all cpu's does not need synchronization to all timers */
	cpu_set_full_synchronize(SYNC_NO_CPU);
	{
		const struct namcos1_slice_timer *slice = specific->slice_timer;
		while(slice->sync_cpu != SYNC_NO_CPU)
		{
			/* start CPU slice timer */
			cpu_start_extend_time_slice(slice->sync_cpu,
				TIME_IN_HZ(slice->delayHz),TIME_IN_HZ(slice->sliceHz) );
			slice++;
		}
	}
#else
	/* compatible with old timer system */
	timer_pulse(TIME_IN_HZ(60*25),0,0);
#endif
}

#if NEW_TIMER
/* normaly CPU slice optimize */
/* slice order is 0:2:1:x:0:3:1:x */
static const struct namcos1_slice_timer normal_slice[]={
	{ SYNC_2CPU(0,1),60*20,-60*20*2 },  /* CPU 0,1 20/vblank , slide slice */
	{ SYNC_2CPU(2,3),60*5,-(60*5*2+60*20*4) },  /* CPU 2,3 10/vblank */
	{ SYNC_NO_CPU }
};
#else
static const struct namcos1_slice_timer normal_slice[]={{0}};
#endif

/*******************************************************************************
*   Shadowland / Youkai Douchuuki specific                                     *
*******************************************************************************/
DRIVER_INIT( shadowld )
{
	const struct namcos1_specific shadowld_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&shadowld_specific);
	namcos1_game_id = 0x0487;
}

/*******************************************************************************
*   Dragon Spirit specific                                                     *
*******************************************************************************/
DRIVER_INIT( dspirit )
{
	const struct namcos1_specific dspirit_specific=
	{
		0x00,0x36,                      /* key query , key id */
		dspirit_key_r,dspirit_key_w,    /* key handler */
		normal_slice,                   /* CPU slice normal */
	};
	namcos1_driver_init(&dspirit_specific);
	namcos1_game_id = 0x0136;
}

/*******************************************************************************
*   Quester specific                                                           *
*******************************************************************************/
DRIVER_INIT( quester )
{
	const struct namcos1_specific quester_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&quester_specific);
	namcos1_game_id = 0x0987;
}

/*******************************************************************************
*   Blazer specific                                                            *
*******************************************************************************/
DRIVER_INIT( blazer )
{
	const struct namcos1_specific blazer_specific=
	{
		0x00,0x13,                  /* key query , key id */
		blazer_key_r,blazer_key_w,  /* key handler */
		normal_slice,               /* CPU slice normal */
	};
	namcos1_driver_init(&blazer_specific);
	namcos1_game_id = 0x0787;
}

/*******************************************************************************
*   Pac-Mania / Pac-Mania (Japan) specific                                     *
*******************************************************************************/
DRIVER_INIT( pacmania )
{
	const struct namcos1_specific pacmania_specific=
	{
		0x4b,0x12,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&pacmania_specific);
	namcos1_game_id = 0x0151;
}

/*******************************************************************************
*   Galaga '88 / Galaga '88 (Japan) specific                                   *
*******************************************************************************/
DRIVER_INIT( galaga88 )
{
	const struct namcos1_specific galaga88_specific=
	{
		0x2d,0x31,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&galaga88_specific);
	namcos1_game_id = 0x0153;
}

/*******************************************************************************
*   World Stadium specific                                                     *
*******************************************************************************/
DRIVER_INIT( ws )
{
	const struct namcos1_specific ws_specific=
	{
		0xd3,0x07,              /* key query , key id */
		ws_key_r,ws_key_w,      /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&ws_specific);
	namcos1_game_id = 0x0154;
}

/*******************************************************************************
*   Beraboh Man specific                                                       *
*******************************************************************************/
static READ_HANDLER( berabohm_buttons_r )
{
	int res;


	if (offset == 0)
	{
		if (berabohm_input_counter == 0) res = readinputport(0);
		else
		{
			static int counter[4];

			res = readinputport(4 + (berabohm_input_counter-1));
			if (res & 0x80)
			{
				if (counter[berabohm_input_counter-1] >= 0)
/*                  res = 0x40 | counter[berabohm_input_counter-1]; I can't get max power with this...*/
					res = 0x40 | (counter[berabohm_input_counter-1]>>1);
				else
				{
					if (res & 0x40) res = 0x40;
					else res = 0x00;
				}
			}
			else if (res & 0x40)
			{
				if (counter[berabohm_input_counter-1] < 0x3f)
				{
					counter[berabohm_input_counter-1]++;
					res = 0x00;
				}
				else res = 0x7f;
			}
			else
				counter[berabohm_input_counter-1] = -1;
		}
		berabohm_input_counter = (berabohm_input_counter+1) % 5;
	}
	else
	{
		static int clk;

		res = 0;
		clk++;
		if (clk & 1) res |= 0x40;
		else if (berabohm_input_counter == 4) res |= 0x10;

		res |= (readinputport(1) & 0x8f);
	}

	return res;
}
DRIVER_INIT( berabohm )
{
	const struct namcos1_specific berabohm_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&berabohm_specific);
	install_mem_read_handler(3,0x1400,0x1401,berabohm_buttons_r);
	namcos1_game_id = 0x0588;
}

/*******************************************************************************
*   Alice in Wonderland / Marchen Maze specific                                *
*******************************************************************************/
DRIVER_INIT( alice )
{
	const struct namcos1_specific alice_specific=
	{
		0x5b,0x25,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&alice_specific);
	namcos1_game_id = 0x0152;
}

/*******************************************************************************
*   Bakutotsu Kijuutei specific                                                *
*******************************************************************************/
DRIVER_INIT( bakutotu )
{
	const struct namcos1_specific bakutotu_specific=
	{
		0x03,0x22,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&bakutotu_specific);
	namcos1_game_id = 0x0155;

	/* resolves CPU deadlocks caused by sloppy coding(see driver\namcos1.c)*/
	{
		data8_t target[8] = {0x34,0x37,0x35,0x37,0x96,0x00,0x2e,0xed};
		data8_t *rombase, *srcptr, *endptr, *scanptr;

		rombase = memory_region(REGION_USER1);
		srcptr = rombase + 0x1e000;
		endptr = srcptr + 0xa000;

		while ( (scanptr = memchr(srcptr, 0x34, endptr-srcptr)) )
		{
			if (!memcmp(scanptr, target, 8))
			{
				scanptr[7] = 0xfc;
				srcptr = scanptr + 8;

				log_cb(RETRO_LOG_DEBUG, LOGPRE "faulty loop patched at %l\n", scanptr-rombase+7);
			}
			else
				srcptr = scanptr + 1;
		}
	}
}

/*******************************************************************************
*   World Court specific                                                       *
*******************************************************************************/
DRIVER_INIT( wldcourt )
{
	const struct namcos1_specific worldcourt_specific=
	{
		0x00,0x35,              /* key query , key id */
		rev2_key_r,rev2_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&worldcourt_specific);
	namcos1_game_id = 0x0143;
}

/*******************************************************************************
*   Splatter House specific                                                    *
*******************************************************************************/
DRIVER_INIT( splatter )
{
	const struct namcos1_specific splatter_specific=
	{
		0x00,0x00,                      /* key query , key id */
		splatter_key_r,splatter_key_w,  /* key handler */
		normal_slice,                   /* CPU slice normal */
	};

	namcos1_driver_init(&splatter_specific);
	namcos1_game_id = 0x0181;

#ifdef USE_MTRANDOM
	seedMT(mame_rand());
#endif

}

/*******************************************************************************
*   Face Off specific                                                          *
*******************************************************************************/
DRIVER_INIT( faceoff )
{
	const struct namcos1_specific faceoff_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&faceoff_specific);
	namcos1_game_id = 0x1288;
}

/*******************************************************************************
*   Rompers specific                                                           *
*******************************************************************************/
DRIVER_INIT( rompers )
{
	const struct namcos1_specific rompers_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&rompers_specific);
	key[0x70] = 0xb6;
	namcos1_game_id = 0x0182;
}

/*******************************************************************************
*   Blast Off specific                                                         *
*******************************************************************************/
DRIVER_INIT( blastoff )
{
	const struct namcos1_specific blastoff_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&blastoff_specific);
	key[0] = 0xb7;
	namcos1_game_id = 0x0183;
}

/*******************************************************************************
*   World Stadium '89 specific                                                 *
*******************************************************************************/
DRIVER_INIT( ws89 )
{
	const struct namcos1_specific ws89_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&ws89_specific);

	key[0x20] = 0xb8;
	namcos1_game_id = 0x0184;
}

/*******************************************************************************
*   Dangerous Seed specific                                                    *
*******************************************************************************/
DRIVER_INIT( dangseed )
{
	const struct namcos1_specific dangseed_specific=
	{
		0x00,0x34,                      /* key query , key id */
		dangseed_key_r,dangseed_key_w,  /* key handler */
		normal_slice,                   /* CPU slice normal */
	};
	namcos1_driver_init(&dangseed_specific);
	namcos1_game_id = 0x0308;
}

/*******************************************************************************
*   World Stadium '90 specific                                                 *
*******************************************************************************/
DRIVER_INIT( ws90 )
{
	const struct namcos1_specific ws90_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&ws90_specific);

	key[0x47] = 0x36;
	key[0x40] = 0x36;
	namcos1_game_id = 0x0310;
}

/*******************************************************************************
*   Pistol Daimyo no Bouken specific                                           *
*******************************************************************************/
DRIVER_INIT( pistoldm )
{
	const struct namcos1_specific pistoldm_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&pistoldm_specific);
	/*key[0x17] = ;*/
	/*key[0x07] = ;*/
	key[0x43] = 0x35;
	namcos1_game_id = 0x0309;
}

/*******************************************************************************
*   Souko Ban DX specific                                                      *
*******************************************************************************/
DRIVER_INIT( soukobdx )
{
	const struct namcos1_specific soukobdx_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&soukobdx_specific);
	/*key[0x27] = ;*/
	/*key[0x07] = ;*/
	key[0x43] = 0x37;
	namcos1_game_id = 0x0311;
}

/*******************************************************************************
*   Puzzle Club specific                                                       *
*******************************************************************************/
DRIVER_INIT( puzlclub )
{
	const struct namcos1_specific puzlclub_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&puzlclub_specific);
	key[0x03] = 0x35;
	namcos1_game_id = 0xff90;
}

/*******************************************************************************
*   Tank Force specific                                                        *
*******************************************************************************/
DRIVER_INIT( tankfrce )
{
	const struct namcos1_specific tankfrce_specific=
	{
		0x00,0x00,              /* key query , key id */
		rev1_key_r,rev1_key_w,  /* key handler */
		normal_slice,           /* CPU slice normal */
	};
	namcos1_driver_init(&tankfrce_specific);
	/*key[0x57] = ;*/
	/*key[0x17] = ;*/
	key[0x2b] = 0xb9;
	key[0x50] = 0xb9;
	namcos1_game_id = 0x0185;
}
