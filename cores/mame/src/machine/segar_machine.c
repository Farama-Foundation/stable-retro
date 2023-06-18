/***************************************************************************

	Sega G-80 raster hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "segar.h"

void (*sega_decrypt)(int,unsigned int *);

UINT8 *segar_mem;

WRITE_HANDLER( segar_characterram_w );
WRITE_HANDLER( segar_characterram2_w );
WRITE_HANDLER( segar_colortable_w );
WRITE_HANDLER( segar_bcolortable_w );


WRITE_HANDLER( segar_w )
{
	int pc,op,page,off;
	unsigned int bad;

	off=offset;

	pc=activecpu_get_previouspc();
	if (pc != -1)
	{
		op=segar_mem[pc] & 0xFF;

		if (op==0x32)
		{
			bad  = offset & 0x00FF;
			page = offset & 0xFF00;
			(*sega_decrypt)(pc,&bad);
			off=page | bad;
		}
	}


	/* MWA_ROM */
	if      ((off>=0x0000) && (off<=0xC7FF))
	{
		;
	}
	/* MWA_RAM */
	else if ((off>=0xC800) && (off<=0xCFFF))
	{
		segar_mem[off]=data;
	}
	else if ((off>=0xE000) && (off<=0xE3FF))
	{
		videoram_w(off - 0xE000,data);
	}
	/* MWA_RAM */
	else if ((off>=0xE400) && (off<=0xE7FF))
	{
		segar_mem[off]=data;
	}
	else if ((off>=0xE800) && (off<=0xEFFF))
	{
		segar_characterram_w(off - 0xE800,data);
	}
	else if ((off>=0xF000) && (off<=0xF03F))
	{
		segar_colortable_w(off - 0xF000,data);
	}
	else if ((off>=0xF040) && (off<=0xF07F))
	{
		segar_bcolortable_w(off - 0xF040,data);
	}
	/* MWA_RAM */
	else if ((off>=0xF080) && (off<=0xF7FF))
	{
		segar_mem[off]=data;
	}
	else if ((off>=0xF800) && (off<=0xFFFF))
	{
		segar_characterram2_w(off - 0xF800,data);
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "unmapped write at %04X:%02X\n",off,data);
	}
}


/****************************************************************************/
/* MB 971025 - Emulate Sega G80 security chip 315-0062                      */
/****************************************************************************/
static void sega_decrypt62(int pc,unsigned int *lo)
{
	unsigned int i = 0;
	unsigned int b = *lo;

	switch (pc & 0x03)
	{
		case 0x00:
			/* D */
			i=b & 0x23;
			i+=((b    & 0xC0) >> 4);
			i+=((b    & 0x10) << 2);
			i+=((b    & 0x08) << 1);
			i+=(((~b) & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x01:
			/* C */
			i=b & 0x03;
			i+=((b    & 0x80) >> 4);
			i+=(((~b) & 0x40) >> 1);
			i+=((b    & 0x20) >> 1);
			i+=((b    & 0x10) >> 2);
			i+=((b    & 0x08) << 3);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x02:
			/* B */
			i=b & 0x03;
			i+=((b    & 0x80) >> 1);
			i+=((b    & 0x60) >> 3);
			i+=((~b) & 0x10);
			i+=((b    & 0x08) << 2);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x03:
			/* A */
			i=b;
			break;
	}

	*lo=i;
}

/****************************************************************************/
/* MB 971025 - Emulate Sega G80 security chip 315-0063                      */
/****************************************************************************/
static void sega_decrypt63(int pc,unsigned int *lo)
{
	unsigned int i = 0;
	unsigned int b = *lo;

	switch (pc & 0x09)
	{
		case 0x00:
			/* D */
			i=b & 0x23;
			i+=((b    & 0xC0) >> 4);
			i+=((b    & 0x10) << 2);
			i+=((b    & 0x08) << 1);
			i+=(((~b) & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x01:
			/* C */
			i=b & 0x03;
			i+=((b    & 0x80) >> 4);
			i+=(((~b) & 0x40) >> 1);
			i+=((b    & 0x20) >> 1);
			i+=((b    & 0x10) >> 2);
			i+=((b    & 0x08) << 3);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x08:
			/* B */
			i=b & 0x03;
			i+=((b    & 0x80) >> 1);
			i+=((b    & 0x60) >> 3);
			i+=((~b) & 0x10);
			i+=((b    & 0x08) << 2);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x09:
			/* A */
			i=b;
			break;
	}

	*lo=i;
}

/****************************************************************************/
/* MB 971025 - Emulate Sega G80 security chip 315-0064                      */
/****************************************************************************/
static void sega_decrypt64(int pc,unsigned int *lo)
{
	unsigned int i = 0;
	unsigned int b = *lo;

	switch (pc & 0x03)
	{
		case 0x00:
			/* A */
			i=b;
			break;
		case 0x01:
			/* B */
			i=b & 0x03;
			i+=((b    & 0x80) >> 1);
			i+=((b    & 0x60) >> 3);
			i+=((~b) & 0x10);
			i+=((b    & 0x08) << 2);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x02:
			/* C */
			i=b & 0x03;
			i+=((b    & 0x80) >> 4);
			i+=(((~b) & 0x40) >> 1);
			i+=((b    & 0x20) >> 1);
			i+=((b    & 0x10) >> 2);
			i+=((b    & 0x08) << 3);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x03:
			/* D */
			i=b & 0x23;
			i+=((b    & 0xC0) >> 4);
			i+=((b    & 0x10) << 2);
			i+=((b    & 0x08) << 1);
			i+=(((~b) & 0x04) << 5);
			i &= 0xFF;
			break;
	}

	*lo=i;
}


/****************************************************************************/
/* MB 971025 - Emulate Sega G80 security chip 315-0070                      */
/****************************************************************************/
static void sega_decrypt70(int pc,unsigned int *lo)
{
	unsigned int i = 0;
	unsigned int b = *lo;

	switch (pc & 0x09)
	{
		case 0x00:
			/* B */
			i=b & 0x03;
			i+=((b    & 0x80) >> 1);
			i+=((b    & 0x60) >> 3);
			i+=((~b) & 0x10);
			i+=((b    & 0x08) << 2);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x01:
			/* A */
			i=b;
			break;
		case 0x08:
			/* D */
			i=b & 0x23;
			i+=((b    & 0xC0) >> 4);
			i+=((b    & 0x10) << 2);
			i+=((b    & 0x08) << 1);
			i+=(((~b) & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x09:
			/* C */
			i=b & 0x03;
			i+=((b    & 0x80) >> 4);
			i+=(((~b) & 0x40) >> 1);
			i+=((b    & 0x20) >> 1);
			i+=((b    & 0x10) >> 2);
			i+=((b    & 0x08) << 3);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
	}

	*lo=i;
}

/****************************************************************************/
/* MB 971025 - Emulate Sega G80 security chip 315-0076                      */
/****************************************************************************/
static void sega_decrypt76(int pc,unsigned int *lo)
{
	unsigned int i = 0;
	unsigned int b = *lo;

	switch (pc & 0x09)
	{
		case 0x00:
			/* A */
			i=b;
			break;
		case 0x01:
			/* B */
			i=b & 0x03;
			i+=((b    & 0x80) >> 1);
			i+=((b    & 0x60) >> 3);
			i+=((~b) & 0x10);
			i+=((b    & 0x08) << 2);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x08:
			/* C */
			i=b & 0x03;
			i+=((b    & 0x80) >> 4);
			i+=(((~b) & 0x40) >> 1);
			i+=((b    & 0x20) >> 1);
			i+=((b    & 0x10) >> 2);
			i+=((b    & 0x08) << 3);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x09:
			/* D */
			i=b & 0x23;
			i+=((b    & 0xC0) >> 4);
			i+=((b    & 0x10) << 2);
			i+=((b    & 0x08) << 1);
			i+=(((~b) & 0x04) << 5);
			i &= 0xFF;
			break;
	}

	*lo=i;
}

/****************************************************************************/
/* MB 971025 - Emulate Sega G80 security chip 315-0082                      */
/****************************************************************************/
static void sega_decrypt82(int pc,unsigned int *lo)
{
	unsigned int i = 0;
	unsigned int b = *lo;

	switch (pc & 0x11)
	{
		case 0x00:
			/* A */
			i=b;
			break;
		case 0x01:
			/* B */
			i=b & 0x03;
			i+=((b    & 0x80) >> 1);
			i+=((b    & 0x60) >> 3);
			i+=((~b) & 0x10);
			i+=((b    & 0x08) << 2);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x10:
			/* C */
			i=b & 0x03;
			i+=((b    & 0x80) >> 4);
			i+=(((~b) & 0x40) >> 1);
			i+=((b    & 0x20) >> 1);
			i+=((b    & 0x10) >> 2);
			i+=((b    & 0x08) << 3);
			i+=((b    & 0x04) << 5);
			i &= 0xFF;
			break;
		case 0x11:
			/* D */
			i=b & 0x23;
			i+=((b    & 0xC0) >> 4);
			i+=((b    & 0x10) << 2);
			i+=((b    & 0x08) << 1);
			i+=(((~b) & 0x04) << 5);
			i &= 0xFF;
			break;
	}

	*lo=i;
}

/****************************************************************************/
/* MB 971031 - Emulate no Sega G80 security chip                            */
/****************************************************************************/
static void sega_decrypt0(int pc,unsigned int *lo)
{
        return;
}

/****************************************************************************/
/* MB 971025 - Set the security chip to be used                             */
/****************************************************************************/
void sega_security(int chip)
{
	switch (chip)
	{
		case 62:
			sega_decrypt=sega_decrypt62;
			break;
		case 63:
			sega_decrypt=sega_decrypt63;
			break;
		case 64:
			sega_decrypt=sega_decrypt64;
			break;
		case 70:
			sega_decrypt=sega_decrypt70;
			break;
		case 76:
			sega_decrypt=sega_decrypt76;
			break;
		case 82:
			sega_decrypt=sega_decrypt82;
			break;
		default:
			sega_decrypt=sega_decrypt0;
			break;
	}
}
