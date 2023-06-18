/*****************************************************************************

  MAME/MESS NES APU CORE

  Based on the Nofrendo/Nosefart NES N2A03 sound emulation core written by
  Matthew Conte (matt@conte.com) and redesigned for use in MAME/MESS by
  Who Wants to Know? (wwtk@mail.com)

  This core is written with the advise and consent of Matthew Conte and is
  released under the GNU Public License.  This core is freely avaiable for
  use in any freeware project, subject to the following terms:

  Any modifications to this code must be duly noted in the source and
  approved by Matthew Conte and myself prior to public submission.

 *****************************************************************************

   NES_APU.C

   Actual NES APU interface.

    LAST MODIFIED 02/29/2004

     - Based on Matthew Conte's Nofrendo/Nosefart core and redesigned to
     use MAME system calls and to enable multiple APUs.  Sound at this
     point should be just about 100% accurate, though I cannot tell for
     certain as yet.

     A queue interface is also available for additional speed.  However,
     the implementation is not yet 100% (DPCM sounds are inaccurate),
     so it is disabled by default.

	 *****************************************************************************
     BUGFIXES:
     - Various bugs concerning the DPCM channel fixed. (Oliver Achten)
     - Fixed $4015 read behaviour. (Oliver Achten)
 *****************************************************************************/
 


#include "driver.h"
#include "nes_apu.h"
#include "cpu/m6502/m6502.h"

/* UNCOMMENT THIS LINE TO ACTIVATE THE EVENT QUEUE. */
/* IT SPEEDS UP THE EMULATION BUT COSTS SOME EXTRA MEMORY. */
/* IT IS CURRENTLY DISABLED BECAUSE THE DPOCM DOESN'T WORK RIGHT WITH IT. */
/*#define USE_QUEUE*/

#include "nes_defs.h"

/* GLOBAL CONSTANTS */
#define  SYNCS_MAX1     0x20
#define  SYNCS_MAX2     0x80

/* GLOBAL VARIABLES */
static apu_t   APU[MAX_NESPSG];       /* Actual APUs */
static apu_t * cur;                   /* Pointer to an APU */
static float   apu_incsize;           /* Adjustment increment */
static uint16  samps_per_sync;        /* Number of samples per vsync */
static uint16  buffer_size;           /* Actual buffer size in bytes */
static uint16  real_rate;             /* Actual playback rate */
static uint16  chip_max;              /* Desired number of chips in use */
static uint8   noise_lut[NOISE_LONG]; /* Noise sample lookup table */
static uint16  vbl_times[0x20];       /* VBL durations in samples */
static uint32  sync_times1[SYNCS_MAX1]; /* Samples per sync table */
static uint32  sync_times2[SYNCS_MAX2]; /* Samples per sync table */
static int     channel;

#define SETAPU(x) cur = &APU[x]

#ifdef USE_QUEUE
/* QUEUE-RELATED FUNCTIONS */

/* IS THE QUEUE EMPTY? */
static char apu_queuenotempty(unsigned int chip)
{
  int head=APU[chip].head;

  if (APU[chip].head<APU[chip].tail) head+=QUEUE_SIZE;

  return (head-APU[chip].tail>1);
}

/* IS THE QUEUE FULL? */
static char apu_queuenotfull(unsigned int chip)
{
  int tail=APU[chip].tail;

  if (APU[chip].tail<APU[chip].head) tail+=QUEUE_SIZE;

  return (tail-APU[chip].head>1);
}

/* ADD AN ENTRY TO THE QUEUE */
static INLINE void apu_enqueue(unsigned int chip,unsigned char reg,unsigned char val)
{
  queue_t *entry;

  SETAPU(chip);

  /* Only enqueue if it isn't full */
  if (apu_queuenotfull(chip))
  {
    cur->head++;
    cur->head&=QUEUE_MAX;
    entry=&cur->queue[cur->head];
    entry->pos=sound_scalebufferpos(samps_per_sync);
    entry->reg=reg;
    entry->val=val;
  }
}

/* EXTRACT AN ENTRY FROM THE QUEUE */
static INLINE queue_t * apu_dequeue(int chip)
{
  queue_t *p=&cur->queue[(cur->tail)];

  SETAPU(chip);

  /* Only increment if it isn't empty */
  if (apu_queuenotempty(chip))
  {
    cur->tail++;
    cur->tail&=QUEUE_MAX;
  }
  return p;
}

#endif

/* INTERNAL FUNCTIONS */

/* INITIALIZE WAVE TIMES RELATIVE TO SAMPLE RATE */
static void create_vbltimes(uint16 * table,const uint8 *vbl,unsigned int rate)
{
  int i;

  for (i=0;i<0x20;i++)
    table[i]=vbl[i]*rate;
}

/* INITIALIZE SAMPLE TIMES IN TERMS OF VSYNCS */
static void create_syncs(unsigned long sps)
{
  int i;
  unsigned long val=sps;

  for (i=0;i<SYNCS_MAX1;i++)
  {
    sync_times1[i]=val;
    val+=sps;
  }

  val=0;
  for (i=0;i<SYNCS_MAX2;i++)
  {
    sync_times2[i]=val;
    sync_times2[i]>>=2;
    val+=sps;
  }
}

/* INITIALIZE NOISE LOOKUP TABLE */
static void create_noise(uint8 *buf, const int bits, int size)
{
   static int m = 0x0011;
   int xor_val, i;

   for (i = 0; i < size; i++)
   {
      xor_val = m & 1;
      m >>= 1;
      xor_val ^= (m & 1);
      m |= xor_val << (bits - 1);

      buf[i] = m;
   }
}

/* TODO: sound channels should *ALL* have DC volume decay */

/* OUTPUT SQUARE WAVE SAMPLE (VALUES FROM -16 to +15) */
static int8 apu_square(square_t *chan)
{
   int env_delay;
   int sweep_delay;
   int8 output;

   /* reg0: 0-3=volume, 4=envelope, 5=hold, 6-7=duty cycle
   ** reg1: 0-2=sweep shifts, 3=sweep inc/dec, 4-6=sweep length, 7=sweep on
   ** reg2: 8 bits of freq
   ** reg3: 0-2=high freq, 7-4=vbl length counter
   */

   if (FALSE == chan->enabled)
      return 0;

   /* enveloping */
   env_delay = sync_times1[chan->regs[0] & 0x0F];

   /* decay is at a rate of (env_regs + 1) / 240 secs */
   chan->env_phase -= 4;
   while (chan->env_phase < 0)
   {
      chan->env_phase += env_delay;
      if (chan->regs[0] & 0x20)
         chan->env_vol = (chan->env_vol + 1) & 15;
      else if (chan->env_vol < 15)
         chan->env_vol++;
   }

   /* vbl length counter */
   if (chan->vbl_length > 0 && 0 == (chan->regs [0] & 0x20))
      chan->vbl_length--;

   if (0 == chan->vbl_length)
      return 0;

   /* freqsweeps */
   if ((chan->regs[1] & 0x80) && (chan->regs[1] & 7))
   {
      sweep_delay = sync_times1[(chan->regs[1] >> 4) & 7];
      chan->sweep_phase -= 2;
      while (chan->sweep_phase < 0)
      {
         chan->sweep_phase += sweep_delay;
         if (chan->regs[1] & 8)
            chan->freq -= chan->freq >> (chan->regs[1] & 7);
         else
            chan->freq += chan->freq >> (chan->regs[1] & 7);
      }
   }

   if ((0 == (chan->regs[1] & 8) && (chan->freq >> 16) > freq_limit[chan->regs[1] & 7])
       || (chan->freq >> 16) < 4)
      return 0;

   chan->phaseacc -= (float) apu_incsize; /* # of cycles per sample */

   while (chan->phaseacc < 0)
   {
      chan->phaseacc += (chan->freq >> 16);
      chan->adder = (chan->adder + 1) & 0x0F;
   }

   if (chan->regs[0] & 0x10) /* fixed volume */
      output = chan->regs[0] & 0x0F;
   else
      output = 0x0F - chan->env_vol;

   if (chan->adder < (duty_lut[chan->regs[0] >> 6]))
      output = -output;

   return (int8) output;
}

/* OUTPUT TRIANGLE WAVE SAMPLE (VALUES FROM -16 to +15) */
static int8 apu_triangle(triangle_t *chan)
{
   int freq;
   int8 output;
   /* reg0: 7=holdnote, 6-0=linear length counter
   ** reg2: low 8 bits of frequency
   ** reg3: 7-3=length counter, 2-0=high 3 bits of frequency
   */

   if (FALSE == chan->enabled)
      return 0;

   if (FALSE == chan->counter_started && 0 == (chan->regs[0] & 0x80))
   {
      if (chan->write_latency)
         chan->write_latency--;
      if (0 == chan->write_latency)
         chan->counter_started = TRUE;
   }

   if (chan->counter_started)
   {
      if (chan->linear_length > 0)
         chan->linear_length--;
      if (chan->vbl_length && 0 == (chan->regs[0] & 0x80))
            chan->vbl_length--;

      if (0 == chan->vbl_length)
         return 0;
   }

   if (0 == chan->linear_length)
      return 0;

   freq = (((chan->regs[3] & 7) << 8) + chan->regs[2]) + 1;

   if (freq < 4) /* inaudible */
      return 0;

   chan->phaseacc -= (float) apu_incsize; /* # of cycles per sample */
   while (chan->phaseacc < 0)
   {
      chan->phaseacc += freq;
      chan->adder = (chan->adder + 1) & 0x1F;

      output = (chan->adder & 7) << 1;
      if (chan->adder & 8)
         output = 0x10 - output;
      if (chan->adder & 0x10)
         output = -output;

      chan->output_vol = output;
   }

   return (int8) chan->output_vol;
}

/* OUTPUT NOISE WAVE SAMPLE (VALUES FROM -16 to +15) */
static int8 apu_noise(noise_t *chan)
{
   int freq, env_delay;
   uint8 outvol;
   uint8 output;

   /* reg0: 0-3=volume, 4=envelope, 5=hold
   ** reg2: 7=small(93 byte) sample,3-0=freq lookup
   ** reg3: 7-4=vbl length counter
   */

   if (FALSE == chan->enabled)
      return 0;

   /* enveloping */
   env_delay = sync_times1[chan->regs[0] & 0x0F];

   /* decay is at a rate of (env_regs + 1) / 240 secs */
   chan->env_phase -= 4;
   while (chan->env_phase < 0)
   {
      chan->env_phase += env_delay;
      if (chan->regs[0] & 0x20)
         chan->env_vol = (chan->env_vol + 1) & 15;
      else if (chan->env_vol < 15)
         chan->env_vol++;
   }

   /* length counter */
   if (0 == (chan->regs[0] & 0x20))
   {
      if (chan->vbl_length > 0)
         chan->vbl_length--;
   }

   if (0 == chan->vbl_length)
      return 0;

   freq = noise_freq[chan->regs[2] & 0x0F];
   chan->phaseacc -= (float) apu_incsize; /* # of cycles per sample */
   while (chan->phaseacc < 0)
   {
      chan->phaseacc += freq;

      chan->cur_pos++;
      if (NOISE_SHORT == chan->cur_pos && (chan->regs[2] & 0x80))
         chan->cur_pos = 0;
      else if (NOISE_LONG == chan->cur_pos)
         chan->cur_pos = 0;
   }

   if (chan->regs[0] & 0x10) /* fixed volume */
      outvol = chan->regs[0] & 0x0F;
   else
      outvol = 0x0F - chan->env_vol;

   output = noise_lut[chan->cur_pos];
   if (output > outvol)
      output = outvol;

   if (noise_lut[chan->cur_pos] & 0x80) /* make it negative */
      output = -output;

   return (int8) output;
}

/* RESET DPCM PARAMETERS */
static INLINE void apu_dpcmreset(dpcm_t *chan)
{
   chan->address = 0xC000 + (uint16) (chan->regs[2] << 6);
   chan->length = (uint16) (chan->regs[3] << 4) + 1;
   chan->bits_left = chan->length << 3;
   chan->irq_occurred = FALSE;
   chan->enabled = TRUE; /* Fixed * Proper DPCM channel ENABLE/DISABLE flag behaviour*/
   chan->vol = 0; /* Fixed * DPCM DAC resets itself when restarted */
}

/* OUTPUT DPCM WAVE SAMPLE (VALUES FROM -64 to +63) */
/* TODO: centerline naughtiness */
static int8 apu_dpcm(dpcm_t *chan)
{
   int freq, bit_pos;

   /* reg0: 7=irq gen, 6=looping, 3-0=pointer to clock table
   ** reg1: output dc level, 7 bits unsigned
   ** reg2: 8 bits of 64-byte aligned address offset : $C000 + (value * 64)
   ** reg3: length, (value * 16) + 1
   */

   if (chan->enabled)
   {
      freq = dpcm_clocks[chan->regs[0] & 0x0F];
      chan->phaseacc -= (float) apu_incsize; /* # of cycles per sample */

      while (chan->phaseacc < 0)
      {
         chan->phaseacc += freq;

         if (0 == chan->length)
         {
            chan->enabled = FALSE; /* Fixed * Proper DPCM channel ENABLE/DISABLE flag behaviour*/
            chan->vol=0; /* Fixed * DPCM DAC resets itself when restarted */ 
			if (chan->regs[0] & 0x40)
				apu_dpcmreset(chan);
            else
            {
               if (chan->regs[0] & 0x80) /* IRQ Generator */
               {
                  chan->irq_occurred = TRUE;
                  n2a03_irq();
               }
               break;
            }
         }

         chan->bits_left--;
         bit_pos = 7 - (chan->bits_left & 7);
         if (7 == bit_pos)
         {
            chan->cur_byte = chan->cpu_mem[chan->address];
            chan->address++;
            chan->length--;
         }

         if (chan->cur_byte & (1 << bit_pos))
/*            chan->regs[1]++;*/
            chan->vol+=2;
         else
/*            chan->regs[1]--;*/
            chan->vol-=2;
      }
   }

   if (chan->vol > 63)
      chan->vol = 63;
   else if (chan->vol < -64)
      chan->vol = -64;

   return (int8) (chan->vol);
}

/* WRITE REGISTER VALUE */
static INLINE void apu_regwrite(int chip,int address, uint8 value)
{
   int chan = (address & 4) ? 1 : 0;

   SETAPU(chip);
   switch (address)
   {
   /* squares */
   case APU_WRA0:
   case APU_WRB0:
      cur->squ[chan].regs[0] = value;
      break;

   case APU_WRA1:
   case APU_WRB1:
      cur->squ[chan].regs[1] = value;
      break;

   case APU_WRA2:
   case APU_WRB2:
      cur->squ[chan].regs[2] = value;
      if (cur->squ[chan].enabled)
         cur->squ[chan].freq = ((((cur->squ[chan].regs[3] & 7) << 8) + value) + 1) << 16;
      break;

   case APU_WRA3:
   case APU_WRB3:
      cur->squ[chan].regs[3] = value;

      if (cur->squ[chan].enabled)
      {
         cur->squ[chan].vbl_length = vbl_times[value >> 3];
         cur->squ[chan].env_vol = 0;
         cur->squ[chan].freq = ((((value & 7) << 8) + cur->squ[chan].regs[2]) + 1) << 16;
      }

      break;

   /* triangle */
   case APU_WRC0:
      cur->tri.regs[0] = value;

      if (cur->tri.enabled)
      {                                          /* ??? */
         if (FALSE == cur->tri.counter_started)
            cur->tri.linear_length = sync_times2[value & 0x7F];
      }

      break;

   case 0x4009:
      /* unused */
      cur->tri.regs[1] = value;
      break;

   case APU_WRC2:
      cur->tri.regs[2] = value;
      break;

   case APU_WRC3:
      cur->tri.regs[3] = value;

      /* this is somewhat of a hack.  there is some latency on the Real
      ** Thing between when trireg0 is written to and when the linear
      ** length counter actually begins its countdown.  we want to prevent
      ** the case where the program writes to the freq regs first, then
      ** to reg 0, and the counter accidentally starts running because of
      ** the sound queue's timestamp processing.
      **
      ** set to a few NES sample -- should be sufficient
      **
      **     3 * (1789772.727 / 44100) = ~122 cycles, just around one scanline
      **
      ** should be plenty of time for the 6502 code to do a couple of table
      ** dereferences and load up the other triregs
      */

      cur->tri.write_latency = 3;

      if (cur->tri.enabled)
      {
         cur->tri.counter_started = FALSE;
         cur->tri.vbl_length = vbl_times[value >> 3];
         cur->tri.linear_length = sync_times2[cur->tri.regs[0] & 0x7F];
      }

      break;

   /* noise */
   case APU_WRD0:
      cur->noi.regs[0] = value;
      break;

   case 0x400D:
      /* unused */
      cur->noi.regs[1] = value;
      break;

   case APU_WRD2:
      cur->noi.regs[2] = value;
      break;

   case APU_WRD3:
      cur->noi.regs[3] = value;

      if (cur->noi.enabled)
      {
         cur->noi.vbl_length = vbl_times[value >> 3];
         cur->noi.env_vol = 0; /* reset envelope */
      }
      break;

   /* DMC */
   case APU_WRE0:
      cur->dpcm.regs[0] = value;
      if (0 == (value & 0x80))
         cur->dpcm.irq_occurred = FALSE;
      break;

   case APU_WRE1: /* 7-bit DAC */
      /*cur->dpcm.regs[1] = value - 0x40;*/
      cur->dpcm.regs[1] = value & 0x7F;
      cur->dpcm.vol = (cur->dpcm.regs[1]-64);
      break;

   case APU_WRE2:
      cur->dpcm.regs[2] = value;
      /*apu_dpcmreset(cur->dpcm);*/
      break;

   case APU_WRE3:
      cur->dpcm.regs[3] = value;
      /*apu_dpcmreset(cur->dpcm);*/
      break;

   case APU_SMASK:
      if (value & 0x01)
         cur->squ[0].enabled = TRUE;
      else
      {
         cur->squ[0].enabled = FALSE;
         cur->squ[0].vbl_length = 0;
      }

      if (value & 0x02)
         cur->squ[1].enabled = TRUE;
      else
      {
         cur->squ[1].enabled = FALSE;
         cur->squ[1].vbl_length = 0;
      }

      if (value & 0x04)
         cur->tri.enabled = TRUE;
      else
      {
         cur->tri.enabled = FALSE;
         cur->tri.vbl_length = 0;
         cur->tri.linear_length = 0;
         cur->tri.counter_started = FALSE;
         cur->tri.write_latency = 0;
      }

      if (value & 0x08)
         cur->noi.enabled = TRUE;
      else
      {
         cur->noi.enabled = FALSE;
         cur->noi.vbl_length = 0;
      }

      if (value & 0x10)
      {
         /* only reset dpcm values if DMA is finished */
         if (FALSE == cur->dpcm.enabled)
         {
            cur->dpcm.enabled = TRUE;
            apu_dpcmreset(&cur->dpcm);
         }
      }
      else
         cur->dpcm.enabled = FALSE;

      cur->dpcm.irq_occurred = FALSE;

      break;
   default:
      log_cb(RETRO_LOG_DEBUG, LOGPRE "invalid apu write: $%02X at $%04X\n", value, address);
      break;
   }
}

/* UPDATE SOUND BUFFER USING CURRENT DATA */
static INLINE void apu_update(int chip)
{
   static INT16 *buffer16 = NULL;
   int accum;
   int endp = sound_scalebufferpos(samps_per_sync);
   int elapsed;

#ifdef USE_QUEUE
   queue_t *q=NULL;

   elapsed=0;
#endif

   SETAPU(chip);
   buffer16  = cur->buffer;

#ifndef USE_QUEUE
   /* Recall last position updated and restore pointers */
   elapsed = cur->buf_pos;
   buffer16 += elapsed;
#endif

   while (elapsed<endp)
   {
#ifdef USE_QUEUE
      while (apu_queuenotempty(chip) && (cur->queue[cur->head].pos==elapsed))
      {
         q = apu_dequeue(chip);
         apu_regwrite(chip,q->reg,q->val);
      }
#endif
      elapsed++;

      accum = apu_square(&cur->squ[0]);
      accum += apu_square(&cur->squ[1]);
      accum += apu_triangle(&cur->tri);
      accum += apu_noise(&cur->noi);
      accum += apu_dpcm(&cur->dpcm);

      /* 8-bit clamps */
      if (accum > 127)
         accum = 127;
      else if (accum < -128)
         accum = -128;

      *(buffer16++)=accum<<8;
   }
#ifndef USE_QUEUE
   cur->buf_pos = endp;
#endif
}

/* READ VALUES FROM REGISTERS */
static INLINE uint8 apu_read(int chip,int address)
{
  if (address == 0x0f) /*FIXED* Address $4015 has different behaviour*/
  {
  	int readval = 0;
  		if ( cur->dpcm.enabled == TRUE )
  			{
  				readval |= 0x10;
  			}
  		if ( cur->dpcm.irq_occurred == TRUE )
  			{
  				readval |= 0x80;
  			}
  		return readval;
  }
  else
  return APU[chip].regs[address];
}

/* WRITE VALUE TO TEMP REGISTRY AND QUEUE EVENT */
static INLINE void apu_write(int chip,int address, uint8 value)
{
   APU[chip].regs[address]=value;
#ifdef USE_QUEUE
   apu_enqueue(chip,address,value);
#else
   apu_update(chip);
   apu_regwrite(chip,address,value);
#endif
}

/* EXTERNAL INTERFACE FUNCTIONS */

/* REGISTER READ/WRITE FUNCTIONS */
READ_HANDLER( NESPSG_0_r ) {return apu_read(0,offset);}
READ_HANDLER( NESPSG_1_r ) {return apu_read(1,offset);}
WRITE_HANDLER( NESPSG_0_w ) {apu_write(0,offset,data);}
WRITE_HANDLER( NESPSG_1_w ) {apu_write(1,offset,data);}

/* INITIALIZE APU SYSTEM */
int NESPSG_sh_start(const struct MachineSound *msound)
{
  struct NESinterface *intf = msound->sound_interface;
  int i;

  /* Initialize global variables */
  samps_per_sync = Machine->sample_rate / Machine->drv->frames_per_second;
  buffer_size = samps_per_sync;
  real_rate = samps_per_sync * Machine->drv->frames_per_second;
  chip_max = intf->num;
  apu_incsize = (float) (N2A03_DEFAULTCLOCK / (float) real_rate);

  /* Use initializer calls */
  create_noise(noise_lut, 13, NOISE_LONG);
  create_vbltimes(vbl_times,vbl_length,samps_per_sync);
  create_syncs(samps_per_sync);

  /* Adjust buffer size if 16 bits */
  buffer_size+=samps_per_sync;

  /* Initialize individual chips */
  for (i = 0;i < chip_max;i++)
  {
     SETAPU(i);

     memset(cur,0,sizeof(apu_t));

     /* Check for buffer allocation failure and bail out if necessary */
     if ((cur->buffer = malloc(buffer_size))==NULL)
     {
       while (--i >= 0) free(APU[i].buffer);
       return 1;
     }

#ifdef USE_QUEUE
     cur->head=0;cur->tail=QUEUE_MAX;
#endif
     (cur->dpcm).cpu_mem=memory_region(intf->region[i]);
  }

  channel = mixer_allocate_channels(chip_max,intf->volume);
  for (i = 0;i < chip_max;i++)
  {
    char name[40];

    sprintf(name,"%s #%d",sound_name(msound),i);
    mixer_set_name(channel,name);
  }

  return 0;
}

/* HALT APU SYSTEM */
void NESPSG_sh_stop(void)
{
  int i;

  for (i=0;i<chip_max;i++)
    free(APU[i].buffer);
}

/* UPDATE APU SYSTEM */
void NESPSG_sh_update(void)
{
  int i;

  if (real_rate==0) return;

  for (i = 0;i < chip_max;i++)
  {
    apu_update(i);
#ifndef USE_QUEUE
    APU[i].buf_pos=0;
#endif
    mixer_play_streamed_sample_16(channel+i,APU[i].buffer,buffer_size,real_rate);
  }
}
