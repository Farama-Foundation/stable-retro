/***************************************************************************

Shanghai

driver by Nicola Salmoria

The HD63484 emulation is incomplete, it implements the bare minimum required
to run these games.

The end of round animation in Shanghai is wrong; change the opcode at 0xfb1f2
to a NOP to jump to it immediately at the beginning of a round.

I'm not sure about the refresh rate, 60Hz makes time match the dip switch
settings, but music runs too fast.

***************************************************************************/

#include "driver.h"

/* the on-chip FIFO is 16 bytes long, but we use a larger one to simplify */
/* decoding of long commands. Commands can be up to 64KB long... but Shanghai */
/* doesn't reach that length. */
#define FIFO_LENGTH 50
#define HD63484_RAM_SIZE 0x200000
static int fifo_counter;
static UINT16 fifo[FIFO_LENGTH];
static UINT16 readfifo;
static UINT8 *HD63484_ram;
static UINT16 HD63484_reg[256/2];
static int org,rwp;
static UINT16 cl0,cl1,ccmp;
static INT16 cpx,cpy;


static int instruction_length[64] =
{
	 0, 3, 2, 1,	/* 0x */
	 0, 0,-1, 2,	/* 1x */
	 0, 3, 3, 3,	/* 2x */
	 0, 0, 0, 0,	/* 3x */
	 0, 1, 2, 2,	/* 4x */
	 0, 0, 4, 4,	/* 5x */
	 5, 5, 5, 5,	/* 6x */
	 5, 5, 5, 5,	/* 7x */
	 3, 3, 3, 3, 	/* 8x */
	 3, 3,-2,-2,	/* 9x */
	-2,-2, 2, 4,	/* Ax */
	 5, 5, 7, 7,	/* Bx */
	 3, 3, 1, 1,	/* Cx */
	 2, 2, 2, 2,	/* Dx */
	 5, 5, 5, 5,	/* Ex */
	 5, 5, 5, 5 	/* Fx */
};

static const char *instruction_name[64] =
{
	"undef","ORG  ","WPR  ","RPR  ",	/* 0x */
	"undef","undef","WPTN ","RPTN ",	/* 1x */
	"undef","DRD  ","DWT  ","DMOD ",	/* 2x */
	"undef","undef","undef","undef",	/* 3x */
	"undef","RD   ","WT   ","MOD  ",	/* 4x */
	"undef","undef","CLR  ","SCLR ",	/* 5x */
	"CPY  ","CPY  ","CPY  ","CPY  ",	/* 6x */
	"SCPY ","SCPY ","SCPY ","SCPY ",	/* 7x */
	"AMOVE","RMOVE","ALINE","RLINE", 	/* 8x */
	"ARCT ","RRCT ","APLL ","RPLL ",	/* 9x */
	"APLG ","RPLG ","CRCL ","ELPS ",	/* Ax */
	"AARC ","RARC ","AEARC","REARC",	/* Bx */
	"AFRCT","RFRCT","PAINT","DOT  ",	/* Cx */
	"PTN  ","PTN  ","PTN  ","PTN  ",	/* Dx */
	"AGCPY","AGCPY","AGCPY","AGCPY",	/* Ex */
	"RGCPY","RGCPY","RGCPY","RGCPY" 	/* Fx */
};

int HD63484_start(void)
{
	fifo_counter = 0;
	HD63484_ram = auto_malloc(HD63484_RAM_SIZE);
	if (!HD63484_ram) return 1;
	memset(HD63484_ram,0,HD63484_RAM_SIZE);
	return 0;
}

void HD63484_stop(void)
{
}

static void doclr(int opcode,UINT16 fill,int *dst,INT16 _ax,INT16 _ay)
{
	INT16 ax,ay;

	ax = _ax;
	ay = _ay;

	for (;;)
	{
		for (;;)
		{
			switch (opcode & 0x0003)
			{
				case 0:
					HD63484_ram[*dst]  = fill; break;
				case 1:
					HD63484_ram[*dst] |= fill; break;
				case 2:
					HD63484_ram[*dst] &= fill; break;
				case 3:
					HD63484_ram[*dst] ^= fill; break;
			}
			if (ax == 0) break;
			else if (ax > 0)
			{
				*dst = (*dst + 1) & (HD63484_RAM_SIZE-1);
				ax--;
			}
			else
			{
				*dst = (*dst - 1) & (HD63484_RAM_SIZE-1);
				ax++;
			}
		}

		ax = _ax;
		if (_ay < 0)
		{
			*dst = (*dst + 384 - ax) & (HD63484_RAM_SIZE-1);
			if (ay == 0) break;
			ay++;
		}
		else
		{
			*dst = (*dst - 384 - ax) & (HD63484_RAM_SIZE-1);
			if (ay == 0) break;
			ay--;
		}
	}
}

static void docpy(int opcode,int src,int *dst,INT16 _ax,INT16 _ay)
{
	int dstep1,dstep2;
	int ax = _ax;
	int ay = _ay;

	switch (opcode & 0x0700)
	{
		default:
		case 0x0000: dstep1 =  1; dstep2 = -384; break;
		case 0x0100: dstep1 =  1; dstep2 =  384; break;
		case 0x0200: dstep1 = -1; dstep2 = -384; break;
		case 0x0300: dstep1 = -1; dstep2 =  384; break;
		case 0x0400: dstep1 = -384; dstep2 =  1; break;
		case 0x0500: dstep1 =  384; dstep2 =  1; break;
		case 0x0600: dstep1 = -384; dstep2 = -1; break;
		case 0x0700: dstep1 =  384; dstep2 = -1; break;
	}
	dstep2 -= ax * dstep1;

	for (;;)
	{
		for (;;)
		{
			switch (opcode & 0x0007)
			{
				case 0:
					HD63484_ram[*dst]  = HD63484_ram[src]; break;
				case 1:
					HD63484_ram[*dst] |= HD63484_ram[src]; break;
				case 2:
					HD63484_ram[*dst] &= HD63484_ram[src]; break;
				case 3:
					HD63484_ram[*dst] ^= HD63484_ram[src]; break;
				case 4:
					if (HD63484_ram[*dst] == (ccmp & 0xff))
						HD63484_ram[*dst] = HD63484_ram[src];
					break;
				case 5:
					if (HD63484_ram[*dst] != (ccmp & 0xff))
						HD63484_ram[*dst] = HD63484_ram[src];
					break;
				case 6:
					if (HD63484_ram[*dst] < HD63484_ram[src])
						HD63484_ram[*dst] = HD63484_ram[src];
					break;
				case 7:
					if (HD63484_ram[*dst] > HD63484_ram[src])
						HD63484_ram[*dst] = HD63484_ram[src];
					break;
			}

			if (opcode & 0x0800)
			{
				if (ay == 0) break;
				else if (ay > 0)
				{
					src = (src - 384) & (HD63484_RAM_SIZE-1);
					*dst = (*dst + dstep1) & (HD63484_RAM_SIZE-1);
					ay--;
				}
				else
				{
					src = (src + 384) & (HD63484_RAM_SIZE-1);
					*dst = (*dst + dstep1) & (HD63484_RAM_SIZE-1);
					ay++;
				}
			}
			else
			{
				if (ax == 0) break;
				else if (ax > 0)
				{
					src = (src + 1) & (HD63484_RAM_SIZE-1);
					*dst = (*dst + dstep1) & (HD63484_RAM_SIZE-1);
					ax--;
				}
				else
				{
					src = (src - 1) & (HD63484_RAM_SIZE-1);
					*dst = (*dst + dstep1) & (HD63484_RAM_SIZE-1);
					ax++;
				}
			}
		}

		if (opcode & 0x0800)
		{
			ay = _ay;
			if (_ax < 0)
			{
				src = (src - 1 - ay) & (HD63484_RAM_SIZE-1);
				*dst = (*dst + dstep2) & (HD63484_RAM_SIZE-1);
				if (ax == 0) break;
				ax++;
			}
			else
			{
				src = (src + 1 - ay) & (HD63484_RAM_SIZE-1);
				*dst = (*dst + dstep2) & (HD63484_RAM_SIZE-1);
				if (ax == 0) break;
				ax--;
			}
		}
		else
		{
			ax = _ax;
			if (_ay < 0)
			{
				src = (src + 384 - ax) & (HD63484_RAM_SIZE-1);
				*dst = (*dst + dstep2) & (HD63484_RAM_SIZE-1);
				if (ay == 0) break;
				ay++;
			}
			else
			{
				src = (src - 384 - ax) & (HD63484_RAM_SIZE-1);
				*dst = (*dst + dstep2) & (HD63484_RAM_SIZE-1);
				if (ay == 0) break;
				ay--;
			}
		}
	}
}



#define PLOT(addr,OPM)								\
switch (OPM)										\
{													\
	case 0:											\
		HD63484_ram[addr]  = cl0; break;			\
	case 1:											\
		HD63484_ram[addr] |= cl0; break;			\
	case 2:											\
		HD63484_ram[addr] &= cl0; break;			\
	case 3:											\
		HD63484_ram[addr] ^= cl0; break;			\
	case 4:											\
		if (HD63484_ram[addr] == (ccmp & 0xff))		\
			HD63484_ram[addr] = cl0;				\
		break;										\
	case 5:											\
		if (HD63484_ram[addr] != (ccmp & 0xff))		\
			HD63484_ram[addr] = cl0;				\
		break;										\
	case 6:											\
		if (HD63484_ram[addr] < (cl0 & 0xff))		\
			HD63484_ram[addr] = cl0;				\
		break;										\
	case 7:											\
		if (HD63484_ram[addr] > (cl0 & 0xff))		\
			HD63484_ram[addr] = cl0;				\
		break;										\
}													\



void HD63484_command_w(UINT16 cmd)
{
	int len;

	fifo[fifo_counter++] = cmd;

	len = instruction_length[fifo[0]>>10];
	if (len == -1)
	{
		if (fifo_counter < 2) return;
		else len = fifo[1]+2;
	}
	else if (len == -2)
	{
		if (fifo_counter < 2) return;
		else len = 2*fifo[1]+2;
	}

	if (fifo_counter >= len)
	{
		int i;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %05x: HD63484 command %s (%04x) ",activecpu_get_pc(),instruction_name[fifo[0]>>10],fifo[0]);
		for (i = 1;i < fifo_counter;i++)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x ",fifo[i]);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");

		if (fifo[0] == 0x0400)	/* ORG */
			org = ((fifo[1] & 0x00ff) << 12) | ((fifo[2] & 0xfff0) >> 4);
		else if ((fifo[0] & 0xffe0) == 0x0800)	/* WPR */
		{
			if (fifo[0] == 0x0800)
				cl0 = fifo[1];
			else if (fifo[0] == 0x0801)
				cl1 = fifo[1];
			else if (fifo[0] == 0x0802)
				ccmp = fifo[1];
			else if (fifo[0] == 0x080c)
				rwp = (rwp & 0x00fff) | ((fifo[1] & 0x00ff) << 12);
			else if (fifo[0] == 0x080d)
				rwp = (rwp & 0xff000) | ((fifo[1] & 0xfff0) >> 4);
			else
log_cb(RETRO_LOG_DEBUG, LOGPRE "unsupported register\n");
		}
		else if ((fifo[0] & 0xfff0) == 0x1800)	/* WPTN */
		{
			/* pattern RAM not supported */
		}
		else if (fifo[0] == 0x4400)	/* RD */
		{
			readfifo = HD63484_ram[2*rwp] | (HD63484_ram[2*rwp+1] << 8);
			rwp = (rwp + 1) & (HD63484_RAM_SIZE/2-1);
		}
		else if (fifo[0] == 0x4800)	/* WT */
		{
			HD63484_ram[2*rwp]   = fifo[1] & 0x00ff ;
			HD63484_ram[2*rwp+1] = (fifo[1] & 0xff00) >> 8;
			rwp = (rwp + 1) & (HD63484_RAM_SIZE/2-1);
		}
		else if (fifo[0] == 0x5800)	/* CLR */
		{
rwp *= 2;
			doclr(fifo[0],fifo[1],&rwp,2*fifo[2]+1,fifo[3]);
rwp /= 2;
		}
		else if ((fifo[0] & 0xfffc) == 0x5c00)	/* SCLR */
		{
rwp *= 2;
			doclr(fifo[0],fifo[1],&rwp,2*fifo[2]+1,fifo[3]);
rwp /= 2;
		}
		else if ((fifo[0] & 0xf0ff) == 0x6000)	/* CPY */
		{
			int src;

			src = ((fifo[1] & 0x00ff) << 12) | ((fifo[2] & 0xfff0) >> 4);
rwp *= 2;
			docpy(fifo[0],2*src,&rwp,2*fifo[3]+1,fifo[4]);
rwp /= 2;
		}
		else if ((fifo[0] & 0xf0fc) == 0x7000)	/* SCPY */
		{
			int src;

			src = ((fifo[1] & 0x00ff) << 12) | ((fifo[2] & 0xfff0) >> 4);
rwp *= 2;
			docpy(fifo[0],2*src,&rwp,2*fifo[3]+1,fifo[4]);
rwp /= 2;
		}
		else if (fifo[0] == 0x8000)	/* AMOVE */
		{
			cpx = fifo[1];
			cpy = fifo[2];
		}
/*		else if ((fifo[0] & 0xff00) == 0x8800)	 // ALINE /*/
		else if ((fifo[0] & 0xfff8) == 0x8800)	/* ALINE */
		{
			INT16 ex,ey,sx,sy;
			INT16 ax,ay;
			int dst;

			sx = cpx;
			sy = cpy;
			ex = fifo[1];
			ey = fifo[2];

			ax = ex - sx;
			ay = ey - sy;

			if (abs(ax) >= abs(ay))
			{
				while (ax)
				{
					dst = (2*org + cpx - cpy * 384) & (HD63484_RAM_SIZE-1);
					PLOT(dst,fifo[0] & 0x0007)

					if (ax > 0)
					{
						cpx++;
						ax--;
					}
					else
					{
						cpx--;
						ax++;
					}
					cpy = sy + ay * (cpx - sx) / (ex - sx);
				}
			}
			else
			{
				while (ay)
				{
					dst = (2*org + cpx - cpy * 384) & (HD63484_RAM_SIZE-1);
					PLOT(dst,fifo[0] & 0x0007)

					if (ay > 0)
					{
						cpy++;
						ay--;
					}
					else
					{
						cpy--;
						ay++;
					}
					cpx = sx + ax * (cpy - sy) / (ey - sy);
				}
			}
		}
/*		else if ((fifo[0] & 0xff00) == 0x9000)	 // ARCT /*/
		else if ((fifo[0] & 0xfff8) == 0x9000)	/* ARCT */
		{
			INT16 pcx,pcy;
			INT16 ax,ay;
			int dst;

			pcx = fifo[1];
			pcy = fifo[2];
			dst = (2*org + cpx - cpy * 384) & (HD63484_RAM_SIZE-1);

			ax = pcx - cpx;
			for (;;)
			{
				PLOT(dst,fifo[0] & 0x0007)

				if (ax == 0) break;
				else if (ax > 0)
				{
					dst = (dst + 1) & (HD63484_RAM_SIZE-1);
					ax--;
				}
				else
				{
					dst = (dst - 1) & (HD63484_RAM_SIZE-1);
					ax++;
				}
			}

			ay = pcy - cpy;
			for (;;)
			{
				PLOT(dst,fifo[0] & 0x0007)

				if (ay == 0) break;
				else if (ay > 0)
				{
					dst = (dst - 384) & (HD63484_RAM_SIZE-1);
					ay--;
				}
				else
				{
					dst = (dst + 384) & (HD63484_RAM_SIZE-1);
					ay++;
				}
			}

			ax = cpx - pcx;
			for (;;)
			{
				PLOT(dst,fifo[0] & 0x0007)

				if (ax == 0) break;
				else if (ax > 0)
				{
					dst = (dst + 1) & (HD63484_RAM_SIZE-1);
					ax--;
				}
				else
				{
					dst = (dst - 1) & (HD63484_RAM_SIZE-1);
					ax++;
				}
			}

			ay = cpy - pcy;
			for (;;)
			{
				PLOT(dst,fifo[0] & 0x0007)

				if (ay == 0) break;
				else if (ay > 0)
				{
					dst = (dst - 384) & (HD63484_RAM_SIZE-1);
					ay--;
				}
				else
				{
					dst = (dst + 384) & (HD63484_RAM_SIZE-1);
					ay++;
				}
			}
		}
/*		else if ((fifo[0] & 0xff00) == 0xc000)	 // AFRCT /*/
		else if ((fifo[0] & 0xfff8) == 0xc000)	/* AFRCT */
		{
			INT16 pcx,pcy;
			INT16 ax,ay;
			int dst;

			pcx = fifo[1];
			pcy = fifo[2];
			ax = pcx - cpx;
			ay = pcy - cpy;
			dst = (2*org + cpx - cpy * 384) & (HD63484_RAM_SIZE-1);

			for (;;)
			{
				for (;;)
				{
					PLOT(dst,fifo[0] & 0x0007)

					if (ax == 0) break;
					else if (ax > 0)
					{
						dst = (dst + 1) & (HD63484_RAM_SIZE-1);
						ax--;
					}
					else
					{
						dst = (dst - 1) & (HD63484_RAM_SIZE-1);
						ax++;
					}
				}

				ax = pcx - cpx;
				if (pcy < cpy)
				{
					dst = (dst + 384 - ax) & (HD63484_RAM_SIZE-1);
					if (ay == 0) break;
					ay++;
				}
				else
				{
					dst = (dst - 384 - ax) & (HD63484_RAM_SIZE-1);
					if (ay == 0) break;
					ay--;
				}
			}
		}
/*		else if ((fifo[0] & 0xff00) == 0xcc00)	 // DOT /*/
		else if ((fifo[0] & 0xfff8) == 0xcc00)	/* DOT */
		{
			int dst;

			dst = (2*org + cpx - cpy * 384) & (HD63484_RAM_SIZE-1);

			PLOT(dst,fifo[0] & 0x0007)
		}
/*		else if ((fifo[0] & 0xf000) == 0xe000)	 // AGCPY /*/
		else if ((fifo[0] & 0xf0f8) == 0xe000)	/* AGCPY */
		{
			INT16 pcx,pcy;
			int src,dst;

			pcx = fifo[1];
			pcy = fifo[2];
			src = (2*org + pcx - pcy * 384) & (HD63484_RAM_SIZE-1);
			dst = (2*org + cpx - cpy * 384) & (HD63484_RAM_SIZE-1);

			docpy(fifo[0],src,&dst,fifo[3],fifo[4]);

			cpx = (dst - 2*org) % 384;
			cpy = (dst - 2*org) / 384;
		}
		else
{
log_cb(RETRO_LOG_DEBUG, LOGPRE "unsupported command\n");
usrintf_showmessage("unsupported command %s (%04x)",instruction_name[fifo[0]>>10],fifo[0]);
}

		fifo_counter = 0;
	}
}

static int regno;

static READ_HANDLER( HD63484_status_r )
{
	if (offset == 1) return 0xff;	/* high 8 bits - not used */

	if (activecpu_get_pc() != 0xfced6 && activecpu_get_pc() != 0xfe1d6) log_cb(RETRO_LOG_DEBUG, LOGPRE "%05x: HD63484 status read\n",activecpu_get_pc());
	return 0x22|4;	/* write FIFO ready + command end    + read FIFO ready */
}

static WRITE_HANDLER( HD63484_address_w )
{
	static unsigned char reg[2];

	reg[offset] = data;
	regno = reg[0];	/* only low 8 bits are used */
/*if (offset == 0)*/
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %05x: HD63484 select register %02x\n",activecpu_get_pc(),regno);*/
}

static WRITE_HANDLER( HD63484_data_w )
{
	static unsigned char dat[2];

	dat[offset] = data;
	if (offset == 1)
	{
		int val = dat[0] + 256 * dat[1];

		if (regno == 0)	/* FIFO */
			HD63484_command_w(val);
		else
		{
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %05x: HD63484 register %02x write %04x\n",activecpu_get_pc(),regno,val);
			HD63484_reg[regno/2] = val;
			if (regno & 0x80) regno += 2;	/* autoincrement */
		}
	}
}

static READ_HANDLER( HD63484_data_r )
{
	int res;

	if (regno == 0x80)
	{
		res = cpu_getscanline();
	}
	else if (regno == 0)
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "%05x: HD63484 read FIFO\n",activecpu_get_pc());
		res = readfifo;
	}
	else
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "%05x: HD63484 read register %02x\n",activecpu_get_pc(),regno);
		res = 0;
	}

	if (offset == 0)
		return res & 0xff;
	else
		return (res >> 8) & 0xff;
}




PALETTE_INIT( shanghai )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (i >> 2) & 0x01;
		bit1 = (i >> 3) & 0x01;
		bit2 = (i >> 4) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (i >> 5) & 0x01;
		bit1 = (i >> 6) & 0x01;
		bit2 = (i >> 7) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (i >> 0) & 0x01;
		bit2 = (i >> 1) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
	}
}

VIDEO_START( shanghai )
{
	return HD63484_start();
}

VIDEO_STOP( shanghai )
{
	HD63484_stop();
}

VIDEO_UPDATE( shanghai )
{
	int x,y,b;


	b = 2 * (((HD63484_reg[0xcc/2] & 0x001f) << 16) + HD63484_reg[0xce/2]);
	for (y = 0;y < 280;y++)
	{
		for (x = 0;x < 384;x++)
		{
			b &= (HD63484_RAM_SIZE-1);
			plot_pixel(bitmap,x,y,Machine->pens[HD63484_ram[b]]);
			b++;
		}
	}

	if ((HD63484_reg[0x06/2] & 0x0300) == 0x0300)
	{
		int sy = (HD63484_reg[0x94/2] & 0x0fff) - (HD63484_reg[0x88/2] >> 8);
		int h = HD63484_reg[0x96/2] & 0x0fff;
		int sx = ((HD63484_reg[0x92/2] >> 8) - (HD63484_reg[0x84/2] >> 8)) * 4;
		int w = (HD63484_reg[0x92/2] & 0xff) * 4;
		if (sx < 0) sx = 0;	/* not sure about this (shangha2 title screen) */

		b = 2 * (((HD63484_reg[0xdc/2] & 0x001f) << 16) + HD63484_reg[0xde/2]);
		for (y = sy;y <= sy+h && y < 280;y++)
		{
			for (x = 0;x < 384;x++)
			{
				b &= (HD63484_RAM_SIZE-1);
				if (x <= w && x + sx >= 0 && x+sx < 384)
					plot_pixel(bitmap,x+sx,y,Machine->pens[HD63484_ram[b]]);
				b++;
			}
		}
	}
}


static INTERRUPT_GEN( shanghai_interrupt )
{
	cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0x80);
}

static WRITE_HANDLER( shanghai_coin_w )
{
	coin_counter_w(0,data & 1);
	coin_counter_w(1,data & 2);
}

static MEMORY_READ_START( readmem )
	{ 0x00000, 0x03fff, MRA_RAM },
	{ 0x80000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x00000, 0x03fff, MWA_RAM },
	{ 0x80000, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x01, HD63484_status_r },
	{ 0x02, 0x03, HD63484_data_r },
	{ 0x20, 0x20, YM2203_status_port_0_r },
	{ 0x22, 0x22, YM2203_read_port_0_r },
	{ 0x40, 0x40, input_port_0_r },
	{ 0x44, 0x44, input_port_1_r },
	{ 0x48, 0x48, input_port_2_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x01, HD63484_address_w },
	{ 0x02, 0x03, HD63484_data_w },
	{ 0x20, 0x20, YM2203_control_port_0_w },
	{ 0x22, 0x22, YM2203_write_port_0_w },
	{ 0x4c, 0x4c, shanghai_coin_w },
PORT_END


static MEMORY_READ_START( shangha2_readmem )
	{ 0x00000, 0x03fff, MRA_RAM },
	{ 0x80000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( shangha2_writemem )
	{ 0x00000, 0x03fff, MWA_RAM },
	{ 0x04000, 0x041ff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0x80000, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( shangha2_readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x10, 0x10, input_port_1_r },
	{ 0x20, 0x20, input_port_2_r },
	{ 0x30, 0x31, HD63484_status_r },
	{ 0x32, 0x33, HD63484_data_r },
	{ 0x40, 0x40, YM2203_status_port_0_r },
	{ 0x42, 0x42, YM2203_read_port_0_r },
PORT_END

static PORT_WRITE_START( shangha2_writeport )
	{ 0x30, 0x31, HD63484_address_w },
	{ 0x32, 0x33, HD63484_data_w },
	{ 0x40, 0x40, YM2203_control_port_0_w },
	{ 0x42, 0x42, YM2203_write_port_0_w },
	{ 0x50, 0x50, shanghai_coin_w },
PORT_END



INPUT_PORTS_START( shanghai )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Confirmation" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "Help" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x08, "2 Players Move Time" )
	PORT_DIPSETTING(    0x0c, "8" )
	PORT_DIPSETTING(    0x08, "10" )
	PORT_DIPSETTING(    0x04, "12" )
	PORT_DIPSETTING(    0x00, "14" )
	PORT_DIPNAME( 0x30, 0x20, "Bonus Time for Making Pair" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0xc0, 0x40, "Start Time" )
	PORT_DIPSETTING(    0xc0, "30" )
	PORT_DIPSETTING(    0x80, "60" )
	PORT_DIPSETTING(    0x40, "90" )
	PORT_DIPSETTING(    0x00, "120" )
INPUT_PORTS_END

INPUT_PORTS_START( shangha2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x00, "2 Players Move Time" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x30, 0x20, "Bonus Time for Making Pair" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0xc0, 0x40, "Start Time" )
	PORT_DIPSETTING(    0xc0, "30" )
	PORT_DIPSETTING(    0x80, "60" )
	PORT_DIPSETTING(    0x40, "90" )
	PORT_DIPSETTING(    0x00, "120" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Mystery Tiles" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
INPUT_PORTS_END



static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	16000000/4,	/* ? */
	{ YM2203_VOL(80,15) },
	{ input_port_3_r },
	{ input_port_4_r },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( shanghai )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,16000000/2)	/* ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(shanghai_interrupt,1)

	MDRV_FRAMES_PER_SECOND(30)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 280)
	MDRV_VISIBLE_AREA(0, 384-1, 0, 280-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(shanghai)
	MDRV_VIDEO_START(shanghai)
	MDRV_VIDEO_UPDATE(shanghai)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( shangha2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,16000000/2)	/* ? */
	MDRV_CPU_MEMORY(shangha2_readmem,shangha2_writemem)
	MDRV_CPU_PORTS(shangha2_readport,shangha2_writeport)
	MDRV_CPU_VBLANK_INT(shanghai_interrupt,1)

	MDRV_FRAMES_PER_SECOND(30)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 280)
	MDRV_VISIBLE_AREA(0, 384-1, 0, 280-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(shanghai)
	MDRV_VIDEO_UPDATE(shanghai)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( shanghai )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "shg-22a.rom", 0xa0001, 0x10000, CRC(e0a085be) SHA1(e281043f97c4cd34a33eb1ec7154abbe67a9aa03) )
	ROM_LOAD16_BYTE( "shg-21a.rom", 0xa0000, 0x10000, CRC(4ab06d32) SHA1(02667d1270b101386b947d5b9bfe64052e498041) )
	ROM_LOAD16_BYTE( "shg-28a.rom", 0xc0001, 0x10000, CRC(983ec112) SHA1(110e120e35815d055d6108a7603e83d2d990c666) )
	ROM_LOAD16_BYTE( "shg-27a.rom", 0xc0000, 0x10000, CRC(41af0945) SHA1(dfc4638a17f716ccc8e59f275571d6dc1093a745) )
	ROM_LOAD16_BYTE( "shg-37b.rom", 0xe0001, 0x10000, CRC(3f192da0) SHA1(e70d5da5d702e9bf9ac6b77df62bcf51894aadcf) )
	ROM_LOAD16_BYTE( "shg-36b.rom", 0xe0000, 0x10000, CRC(a1d6af96) SHA1(01c4c22bf03b3d260fffcbc6dfc5f2dd2bcba14a) )
ROM_END

ROM_START( shangha2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sht-27j", 0x80001, 0x20000, CRC(969cbf00) SHA1(350025f4e39c7d89cb72e46b52fb467e3e9056f4) )
	ROM_LOAD16_BYTE( "sht-26j", 0x80000, 0x20000, CRC(4bf01ab4) SHA1(6928374db080212a371991ee98cd563e158907f0) )
	ROM_LOAD16_BYTE( "sht-31j", 0xc0001, 0x20000, CRC(312e3b9d) SHA1(f15f76a087d4972aa72145eced8d1fb15329b359) )
	ROM_LOAD16_BYTE( "sht-30j", 0xc0000, 0x20000, CRC(2861a894) SHA1(6da99d15f41e900735f8943f2710487817f98579) )
ROM_END



GAMEX(1988, shanghai, 0, shanghai, shanghai, 0, ROT0, "Sunsoft", "Shanghai (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME( 1989, shangha2, 0, shangha2, shangha2, 0, ROT0, "Sunsoft", "Shanghai II (Japan)" )
