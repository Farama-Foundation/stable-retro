/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/

/*
  Changelog, Hiromitsu Shioya 02/05/2002
  fix start address decode timing. (sample loop bug.)

	Changelog, Mish, August 1999:
		Removed interface support for different memory regions per channel.
		Removed interface support for differing channel volume.

		Added bankswitching.
		Added support for multiple chips.

		(Nb:  Should different memory regions per channel be needed
		the bankswitching function can set this up).

NS990821
support for the K007232_VOL() macro.
added external port callback, and functions to set the volume of the channels

*/


#include "driver.h"
#include <math.h>


#define  KDAC_A_PCM_MAX    (2)		/* Channels per chip */


typedef struct kdacApcm
{
  unsigned char vol[KDAC_A_PCM_MAX][2];	/* volume for the left and right channel */
  unsigned int  addr[KDAC_A_PCM_MAX];
  unsigned int  start[KDAC_A_PCM_MAX];
  unsigned int  step[KDAC_A_PCM_MAX];
  unsigned int  bank[KDAC_A_PCM_MAX];
  int play[KDAC_A_PCM_MAX];

  unsigned char wreg[0x10];	/* write data */
  unsigned char *pcmbuf[2];	/* Channel A & B pointers */

  unsigned int  clock;          /* chip clock */
  unsigned int  pcmlimit;
} KDAC_A_PCM;

static KDAC_A_PCM    kpcm[MAX_K007232];

static int pcm_chan[MAX_K007232];

static const struct K007232_interface *intf;

#define   BASE_SHIFT    (12)



#if 0
static int kdac_note[] = {
  261.63/8, 277.18/8,
  293.67/8, 311.13/8,
  329.63/8,
  349.23/8, 369.99/8,
  392.00/8, 415.31/8,
  440.00/8, 466.16/8,
  493.88/8,

  523.25/8,
};

static float kdaca_fn[][2] = {
  /* B */
  { 0x03f, 493.88/8 },		/* ?? */
  { 0x11f, 493.88/4 },		/* ?? */
  { 0x18f, 493.88/2 },		/* ?? */
  { 0x1c7, 493.88   },
  { 0x1e3, 493.88*2 },
  { 0x1f1, 493.88*4 },		/* ?? */
  { 0x1f8, 493.88*8 },		/* ?? */
  /* A+ */
  { 0x020, 466.16/8 },		/* ?? */
  { 0x110, 466.16/4 },		/* ?? */
  { 0x188, 466.16/2 },
  { 0x1c4, 466.16   },
  { 0x1e2, 466.16*2 },
  { 0x1f1, 466.16*4 },		/* ?? */
  { 0x1f8, 466.16*8 },		/* ?? */
  /* A */
  { 0x000, 440.00/8 },		/* ?? */
  { 0x100, 440.00/4 },		/* ?? */
  { 0x180, 440.00/2 },
  { 0x1c0, 440.00   },
  { 0x1e0, 440.00*2 },
  { 0x1f0, 440.00*4 },		/* ?? */
  { 0x1f8, 440.00*8 },		/* ?? */
  { 0x1fc, 440.00*16},		/* ?? */
  { 0x1fe, 440.00*32},		/* ?? */
  { 0x1ff, 440.00*64},		/* ?? */
  /* G+ */
  { 0x0f2, 415.31/4 },
  { 0x179, 415.31/2 },
  { 0x1bc, 415.31   },
  { 0x1de, 415.31*2 },
  { 0x1ef, 415.31*4 },		/* ?? */
  { 0x1f7, 415.31*8 },		/* ?? */
  /* G */
  { 0x0e2, 392.00/4 },
  { 0x171, 392.00/2 },
  { 0x1b8, 392.00   },
  { 0x1dc, 392.00*2 },
  { 0x1ee, 392.00*4 },		/* ?? */
  { 0x1f7, 392.00*8 },		/* ?? */
  /* F+ */
  { 0x0d0, 369.99/4 },		/* ?? */
  { 0x168, 369.99/2 },
  { 0x1b4, 369.99   },
  { 0x1da, 369.99*2 },
  { 0x1ed, 369.99*4 },		/* ?? */
  { 0x1f6, 369.99*8 },		/* ?? */
  /* F */
  { 0x0bf, 349.23/4 },		/* ?? */
  { 0x15f, 349.23/2 },
  { 0x1af, 349.23   },
  { 0x1d7, 349.23*2 },
  { 0x1eb, 349.23*4 },		/* ?? */
  { 0x1f5, 349.23*8 },		/* ?? */
  /* E */
  { 0x0ac, 329.63/4 },
  { 0x155, 329.63/2 },		/* ?? */
  { 0x1ab, 329.63   },
  { 0x1d5, 329.63*2 },
  { 0x1ea, 329.63*4 },		/* ?? */
  { 0x1f4, 329.63*8 },		/* ?? */
  /* D+ */
  { 0x098, 311.13/4 },		/* ?? */
  { 0x14c, 311.13/2 },
  { 0x1a6, 311.13   },
  { 0x1d3, 311.13*2 },
  { 0x1e9, 311.13*4 },		/* ?? */
  { 0x1f4, 311.13*8 },		/* ?? */
  /* D */
  { 0x080, 293.67/4 },		/* ?? */
  { 0x140, 293.67/2 },		/* ?? */
  { 0x1a0, 293.67   },
  { 0x1d0, 293.67*2 },
  { 0x1e8, 293.67*4 },		/* ?? */
  { 0x1f4, 293.67*8 },		/* ?? */
  { 0x1fa, 293.67*16},		/* ?? */
  { 0x1fd, 293.67*32},		/* ?? */
  /* C+ */
  { 0x06d, 277.18/4 },		/* ?? */
  { 0x135, 277.18/2 },		/* ?? */
  { 0x19b, 277.18   },
  { 0x1cd, 277.18*2 },
  { 0x1e6, 277.18*4 },		/* ?? */
  { 0x1f2, 277.18*8 },		/* ?? */
  /* C */
  { 0x054, 261.63/4 },
  { 0x12a, 261.63/2 },
  { 0x195, 261.63   },
  { 0x1ca, 261.63*2 },
  { 0x1e5, 261.63*4 },
  { 0x1f2, 261.63*8 },		/* ?? */

  { -1, -1 },
};
#endif

static float fncode[0x200];
/*************************************************************/
static void KDAC_A_make_fncode( void ){
  int i;
#if 0
  int i, j, k;
  float fn;
  for( i = 0; i < 0x200; i++ )  fncode[i] = 0;

  i = 0;
  while( (int)kdaca_fn[i][0] != -1 ){
    fncode[(int)kdaca_fn[i][0]] = kdaca_fn[i][1];
    i++;
  }

  i = j = 0;
  while( i < 0x200 ){
    if( fncode[i] != 0 ){
      if( i != j ){
	fn = (fncode[i] - fncode[j]) / (i - j);
	for( k = 1; k < (i-j); k++ )
	  fncode[k+j] = fncode[j] + fn*k;
	j = i;
      }
    }
    i++;
  }
  for( i = 0; i < 0x200; i++ )
    log_cb(RETRO_LOG_DEBUG, LOGPRE "fncode[%04x] = %.2f\n", i, fncode[i] );

#else
  for( i = 0; i < 0x200; i++ ){
    /*fncode[i] = (0x200 * 55) / (0x200 - i);*/
    fncode[i] = ((0x200 * 55.2) / (0x200 - i)) / (440.00 / 2);
    log_cb(RETRO_LOG_DEBUG, LOGPRE "2 : fncode[%04x] = %.2f\n", i, fncode[i] );
  }

#endif
}


/************************************************/
/*    Konami PCM update                         */
/************************************************/

static void KDAC_A_update(int chip, INT16 **buffer, int buffer_len)
{
  int i;

  memset(buffer[0],0,buffer_len * sizeof(INT16));
  memset(buffer[1],0,buffer_len * sizeof(INT16));

  for( i = 0; i < KDAC_A_PCM_MAX; i++ )
    {
      if (kpcm[chip].play[i])
	{
	  int volA,volB,j,out;
	  unsigned int addr, old_addr;
	  /*int cen;*/

	  /**** PCM setup ****/
	  addr = kpcm[chip].start[i] + ((kpcm[chip].addr[i]>>BASE_SHIFT)&0x000fffff);
	  volA = kpcm[chip].vol[i][0] * 2;
	  volB = kpcm[chip].vol[i][1] * 2;
#if 0
	   cen = (volA + volB) / 2;
	  volA = (volA + cen) < 0x1fe ? (volA + cen) : 0x1fe;
	  volB = (volB + cen) < 0x1fe ? (volB + cen) : 0x1fe;
#endif

	  for( j = 0; j < buffer_len; j++ )
	    {
	      old_addr = addr;
	      addr = kpcm[chip].start[i] + ((kpcm[chip].addr[i]>>BASE_SHIFT)&0x000fffff);
	      while (old_addr <= addr)
		{
		  if( (kpcm[chip].pcmbuf[i][old_addr] & 0x80) || old_addr >= kpcm[chip].pcmlimit )
		    {
		      /* end of sample */

		      if( kpcm[chip].wreg[0x0d]&(1<<i) )
			{
			  /* loop to the beginning */
			  kpcm[chip].start[i] =
			    ((((unsigned int)kpcm[chip].wreg[i*0x06 + 0x04]<<16)&0x00010000) |
			     (((unsigned int)kpcm[chip].wreg[i*0x06 + 0x03]<< 8)&0x0000ff00) |
			     (((unsigned int)kpcm[chip].wreg[i*0x06 + 0x02]    )&0x000000ff) |
			     kpcm[chip].bank[i]);
			  addr = kpcm[chip].start[i];
			  kpcm[chip].addr[i] = 0;
			  old_addr = addr; /* skip loop */
			}
		      else
			{
			  /* stop sample */
			  kpcm[chip].play[i] = 0;
			}
		      break;
		    }

		  old_addr++;
		}

	      if (kpcm[chip].play[i] == 0)
		break;

	      kpcm[chip].addr[i] += kpcm[chip].step[i];

	      out = (kpcm[chip].pcmbuf[i][addr] & 0x7f) - 0x40;

	      buffer[0][j] += out * volA;
	      buffer[1][j] += out * volB;
	    }
	}
    }
}


/************************************************/
/*    Konami PCM start                          */
/************************************************/
int K007232_sh_start(const struct MachineSound *msound)
{
  int i,j;

  intf = msound->sound_interface;

  /* Set up the chips */
  for (j=0; j<intf->num_chips; j++)
    {
      char buf[2][40];
      const char *name[2];
      int vol[2];

      kpcm[j].pcmbuf[0] = (unsigned char *)memory_region(intf->bank[j]);
      kpcm[j].pcmbuf[1] = (unsigned char *)memory_region(intf->bank[j]);
      kpcm[j].pcmlimit  = (unsigned int)memory_region_length(intf->bank[j]);

	kpcm[j].clock = intf->baseclock;

      for( i = 0; i < KDAC_A_PCM_MAX; i++ )
	{
	  kpcm[j].start[i] = 0;
	  kpcm[j].step[i] = 0;
	  kpcm[j].play[i] = 0;
	  kpcm[j].bank[i] = 0;
	}
      kpcm[j].vol[0][0] = 255;	/* channel A output to output A */
      kpcm[j].vol[0][1] = 0;
      kpcm[j].vol[1][0] = 0;
      kpcm[j].vol[1][1] = 255;	/* channel B output to output B */

      for( i = 0; i < 0x10; i++ )  kpcm[j].wreg[i] = 0;

      if( (intf->volume[j]&0xff00) == MIXER_PAN_CENTER ){
	for (i = 0;i < 2;i++){
	  name[i] = buf[i];
	  sprintf(buf[i],"007232 #%d Ch %c",j,'A'+i);
	}
      }
      else{
	for (i = 0;i < 2;i++){
	  name[i] = buf[i];
	  sprintf( buf[i], "007232 #%d Ch A&B", j );
	}
      }

      vol[0]=intf->volume[j] & 0xffff;
      vol[1]=intf->volume[j] >> 16;

      pcm_chan[j] = stream_init_multi(2,name,vol,Machine->sample_rate,
				      j,KDAC_A_update);
    }

  KDAC_A_make_fncode();

  return 0;
}

/************************************************/
/*    Konami PCM write register                 */
/************************************************/
static void K007232_WriteReg( int r, int v, int chip )
{
  int  data;

  if (Machine->sample_rate == 0) return;

  stream_update(pcm_chan[chip],0);

  kpcm[chip].wreg[r] = v;			/* stock write data */

  if (r == 0x0c){
    /* external port, usually volume control */
    if (intf->portwritehandler[chip]) (*intf->portwritehandler[chip])(v);
    return;
  }
  else if( r == 0x0d ){
    /* loopflag. */
    return;
  }
  else{
    int  reg_port;

    reg_port = 0;
    if (r >= 0x06){
      reg_port = 1;
      r -= 0x06;
    }

    switch (r){
    case 0x00:
    case 0x01:
				/**** address step ****/
      data = (((((unsigned int)kpcm[chip].wreg[reg_port*0x06 + 0x01])<<8)&0x0100) | (((unsigned int)kpcm[chip].wreg[reg_port*0x06 + 0x00])&0x00ff));
      if( !reg_port && r == 1 )
	      log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x\n" ,data );
      kpcm[chip].step[reg_port] =
	( (7850.0 / (float)Machine->sample_rate) ) *
	fncode[data] *
	( (float)kpcm[chip].clock / (float)4000000 ) *
	(1<<BASE_SHIFT);
      break;

    case 0x02:
    case 0x03:
    case 0x04:
      break;
    case 0x05:
				/**** start address ****/
      kpcm[chip].start[reg_port] =
	((((unsigned int)kpcm[chip].wreg[reg_port*0x06 + 0x04]<<16)&0x00010000) |
	 (((unsigned int)kpcm[chip].wreg[reg_port*0x06 + 0x03]<< 8)&0x0000ff00) |
	 (((unsigned int)kpcm[chip].wreg[reg_port*0x06 + 0x02]    )&0x000000ff) |
	 kpcm[chip].bank[reg_port]);
      if (kpcm[chip].start[reg_port] < kpcm[chip].pcmlimit ){
	kpcm[chip].play[reg_port] = 1;
	kpcm[chip].addr[reg_port] = 0;
      }
      break;
    }
  }
}

/************************************************/
/*    Konami PCM read register                  */
/************************************************/
static int K007232_ReadReg( int r, int chip )
{
  int  ch = 0;

  if( r == 0x0005 || r == 0x000b ){
    ch = r/0x0006;
    r  = ch * 0x0006;

    kpcm[chip].start[ch] =
      ((((unsigned int)kpcm[chip].wreg[r + 0x04]<<16)&0x00010000) |
       (((unsigned int)kpcm[chip].wreg[r + 0x03]<< 8)&0x0000ff00) |
       (((unsigned int)kpcm[chip].wreg[r + 0x02]    )&0x000000ff) |
       kpcm[chip].bank[ch]);

    if (kpcm[chip].start[ch] <  kpcm[chip].pcmlimit ){
      kpcm[chip].play[ch] = 1;
      kpcm[chip].addr[ch] = 0;
    }
  }
  return 0;
}

/*****************************************************************************/

WRITE_HANDLER( K007232_write_port_0_w )
{
  K007232_WriteReg(offset,data,0);
}

READ_HANDLER( K007232_read_port_0_r )
{
  return K007232_ReadReg(offset,0);
}

WRITE_HANDLER( K007232_write_port_1_w )
{
  K007232_WriteReg(offset,data,1);
}

READ_HANDLER( K007232_read_port_1_r )
{
  return K007232_ReadReg(offset,1);
}

WRITE_HANDLER( K007232_write_port_2_w )
{
  K007232_WriteReg(offset,data,2);
}

READ_HANDLER( K007232_read_port_2_r )
{
  return K007232_ReadReg(offset,2);
}

void K007232_set_volume(int chip,int channel,int volumeA,int volumeB)
{
  kpcm[chip].vol[channel][0] = volumeA;
  kpcm[chip].vol[channel][1] = volumeB;
}

void K007232_set_bank( int chip, int chABank, int chBBank )
{
  kpcm[chip].bank[0] = chABank<<17;
  kpcm[chip].bank[1] = chBBank<<17;
}

/*****************************************************************************/
