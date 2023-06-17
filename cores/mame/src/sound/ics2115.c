/*
   ICS 2115 sound synthesizer.

   By O. Galibert, with a lot of help from the nebula
   ics emulation by Elsemi.
*/

#include <math.h>
#include "driver.h"
#include "cpuintrf.h"
#include "cpu/z80/z80.h"
#include "sound/ics2115.h"

/* a:401ae90.000 l:1c23c.0 e:1e1d8.0  09  tone*/
/* a:4023c40.000 l:25d60.0 e:266cb.0  08  tone*/
/* a:4028fc0.000 l:28fc0.0 e:2b6ef.0  01  violon, noisy*/
/* a:4034020.000 l:34560.0 e:36001.0  19  percussion*/
/* a:4034218.1e8 l:34560.0 e:36001.0  19  percussion*/
/* a:4037f10.000 l:37f3e.0 e:39cb7.0  08  tone*/
/* a:4044f10.000 l:463ea.0 e:46476.0  09  tone*/
/* a:40490d0.000 l:760e9.0 e:910d8.0  19  percussion moche*/
/* a:4051bd0.000 l:51bd0.0 e:528df.0  01  percussion*/
/* a:40621f0.000 l:621f0.0 e:62aef.0  01  percussion faible*/
/* a:4063430.000 l:63c78.0 e:63d25.0  08  tone*/
/* a:40668a0.000 l:668a0.0 e:670ec.0  01  percussion*/
/* a:4067940.000 l:67940.0 e:68140.0  01  percussion*/
/* a:40aff36.000 l:aff36.0 e:b194d.0  20  Selection menu*/
/* a:40b5f26.000 l:b5f26.0 e:b63a5.0  20  Move up/down*/
/* a:4102772.000 l:02772.0 e:03a31.0  20  Voice test (fucked?)*/

/* conf:*/
/*   10b6: 00*/
/*   11ee: 20*/
/*   1867: a0*/
/*   188b: 00*/
/*   20ba: 01 08 09*/
/*   2299: 01 09 19*/

#define FLAG_OSCCONF_DISABLE	(1 << 1)
#define FLAG_OSCCONF_NOENVELOPE	(FLAG_OSCCONF_DISABLE)
#define FLAG_OSCCONF_EIGHTBIT	(1 << 2)
#define FLAG_OSCCONF_LOOP	(1 << 3)
#define FLAG_OSCCONF_LOOP_BIDIR	(1 << 4)
#define FLAG_OSCCONF_IRQ	(1 << 5)
#define FLAG_OSCCONF_INVERT	(1 << 6)
/* Possibly enables IRQ for completion */

#define FLAG_VOLCTL_DONE	(1 << 0)
/* Unsure, but envelope can be disabled with this, so I assume it's a disable bit... */
#define FLAG_VOLCTL_DISABLE (1 << 1)
#define FLAG_VOLCTL_NOENVELOPE	(FLAG_VOLCTL_DONE | FLAG_VOLCTL_DISABLE)
#define FLAG_VOLCTL_LOOP	(1 << 3)
#define FLAG_VOLCTL_LOOP_BIDIR	(1 << 4)
#define FLAG_VOLCTL_IRQ		(1 << 5)
#define FLAG_VOLCTL_INVERT	(1 << 6)

#define	FLAG_STATE_ON		(1 << 0)
#define	FLAG_STATE_WAVEIRQ	(1 << 1)
#define	FLAG_STATE_VOLIRQ	(1 << 2)
#define	FLAG_STATE_IRQ		(FLAG_STATE_VOLIRQ | FLAG_STATE_WAVEIRQ)


enum { V_ON = 1, V_DONE = 2 };

static UINT32 ramp[32];

static struct {
	struct ics2115_interface *intf;
	UINT8 *rom;
	INT16 *ulaw;
	UINT16 *voltbl;

	struct {
		INT16 left;
		UINT16 add;
		UINT16 fc, addrh, addrl, strth, endh, volacc, incr, tout;
		UINT8 strtl, endl, saddr, pan, conf, ctl;
		UINT8 vstart, vend, vctl;
		UINT8 state;
	} voice[32];

	struct {
		UINT8 scale, preset;
		mame_timer *timer;
		double period;
	} timer[2];

	UINT8 reg, osc;
	UINT8 irq_en, irq_pend;
	int irq_on;
	int stream;
} ics2115;

static int caller_get_pc(void)
{
	int pc = activecpu_get_pc();
#if 0
	if(pc == 0x14b || pc == 0x26e || pc == 0x284 || pc == 0x28d ||
	   pc == 0x290 || pc == 0x299 || pc == 0x2a2 || pc == 0x2b3) {
		int sp = z80_get_reg(Z80_SP);
		pc = cpu_readmem16(sp)|(cpu_readmem16((UINT16)(sp+1)) << 8);
	}
#endif

	return pc;
}

static void recalc_irq(void)
{
	int i;
    int irq = 0;
	if(ics2115.irq_en & ics2115.irq_pend)
		irq = 1;
	for(i=0; !irq && i<32; i++)
		if(ics2115.voice[i].state & V_DONE)
			irq = 1;
	if(irq != ics2115.irq_on) {
		ics2115.irq_on = irq;
		if(ics2115.intf->irq_cb)
			ics2115.intf->irq_cb(irq ? ASSERT_LINE : CLEAR_LINE);
	}
}


static void update(int param, INT16 **buffer, int length)
{
	int osc, i;
	int rec_irq = 0;

	memset(buffer[0], 0, length*2);
	memset(buffer[1], 0, length*2);

	for(osc = 0; osc < 32; osc++)
		if(ics2115.voice[osc].state & V_ON) {
			UINT32 adr = (ics2115.voice[osc].addrh << 16) | ics2115.voice[osc].addrl;
			UINT32 end = (ics2115.voice[osc].endh << 16) | (ics2115.voice[osc].endl << 8);
			UINT32 loop = (ics2115.voice[osc].strth << 16) | (ics2115.voice[osc].strtl << 8);
			UINT32 badr = (ics2115.voice[osc].saddr << 20) & 0xffffff;
			UINT32 delta = (ics2115.voice[osc].fc << 2)*(33075.0/44100.0);
			UINT8 conf = ics2115.voice[osc].conf;
			INT32 vol = ics2115.voice[osc].volacc;
			UINT32 volacc = (ics2115.voice[osc].volacc) >> 4 & 0xffff;
			ics2115.voice[osc].add = (double)((ics2115.voice[osc].incr & 0x3F) << 4) / (double)(1 << (3 * ((ics2115.voice[osc].incr >> 6) & 3)));
			
			if ((ics2115.voice[osc].tout > 0) && (osc < 8))ics2115.voice[osc].tout--;
			if (volacc == 0xe68) volacc = 0xe20;
			if (volacc == 0xee0) volacc = 0xe20;
			if (volacc == 0xfcc) volacc = 0xf40;
			if (volacc == 0xe18) volacc = 0xeb0;
			vol = ics2115.voltbl[volacc];
			if (!(ics2115.voice[osc].ctl & 0x8)) ramp[osc]  = 0x0;
			if (ics2115.voice[osc].ctl & 0x8) ramp[osc]  += 0x150;
			
			if ((ramp[osc] + 0x50) > vol) vol = 0x0;
			else vol -= ramp[osc];
			
			/*printf("ramp:%x vol:%x\n",volacc,vol); */

			log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: KEYRUN %02d adr=%08x end=%08x delta=%08x\n",
					 osc, adr, end, delta);

			for(i=0; i<length; i++) {
				INT32 v = ics2115.rom[badr|(adr >> 12)];
				if(conf & 1)
					v = ics2115.ulaw[v];
				else
					v = ((INT8)v) << 6;

if(1)
{
				v = (v * vol) >> (18);
				buffer[0][i] += v;
				buffer[1][i] += v;


				adr += delta;
				if(adr >= end) 
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: KEYDONE %2d\n", osc);
					adr -= (end-loop);
					if((!(conf & 0x8)) || ics2115.voice[osc].tout == 0) {
						ics2115.voice[osc].state &= ~V_ON;
						ics2115.voice[osc].state |= V_DONE;
						
					}
					
					rec_irq = 1;
					break;
				}
			}
			}
			ics2115.voice[osc].addrh = adr >> 16;
			ics2115.voice[osc].addrl = adr;
		}
	if(rec_irq)
		recalc_irq();
}

static void keyon(int osc)
{
	logerror("ICS2115: KEYON %2d conf:%02x vctl:%02x a:%07x.%03x l:%05x.%x e:%05x.%x v:%03x f:%d\n",
			 osc,
			 ics2115.voice[ics2115.osc].conf,
			 ics2115.voice[ics2115.osc].vctl,
			 (ics2115.voice[osc].saddr << 20)|(ics2115.voice[osc].addrh << 4)|(ics2115.voice[osc].addrl >> 12),
			 (ics2115.voice[osc].addrl >> 3) & 0x1ff,
			 (ics2115.voice[osc].strth << 4)|(ics2115.voice[osc].strtl >> 4),
			 ics2115.voice[osc].strtl & 0xf,
			 (ics2115.voice[osc].endh << 4)|(ics2115.voice[osc].endl >> 4),
			 ics2115.voice[osc].endl & 0xf,
			 ics2115.voice[osc].volacc>>4,
			 (ics2115.voice[ics2115.osc].fc*33075+512)/1024);
	ics2115.voice[osc].state |= V_ON;
}


static void timer_cb(int timer)
{
	ics2115.irq_pend |= 1<<timer;
	recalc_irq();
}

/* Arcadez keep original timers otherwise all games drop to 5fps */
static void recalc_timer(int timer)
{
	double period = ics2115.timer[timer].scale*ics2115.timer[timer].preset / 33868800.0;
	if(period)
		period = 1/62.8206;
	if(period)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: timer %d freq=%gHz\n", timer, 1/period);
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: timer %d off\n", timer);

	if(ics2115.timer[timer].period != period) {
		ics2115.timer[timer].period = period;
		if(period)
			timer_adjust(ics2115.timer[timer].timer, TIME_IN_SEC(period), timer, TIME_IN_SEC(period));
		else
			timer_adjust(ics2115.timer[timer].timer, TIME_NEVER, timer, 0);
	}
}


static void ics2115_reg_w(UINT8 reg, UINT8 data, int msb)
{
	ics2115.voice[ics2115.osc].tout = 40;
	switch(reg) {
	case 0x00: /* [osc] Oscillator Configuration*/
		if(msb) {
			ics2115.voice[ics2115.osc].conf = data;
			logerror("ICS2115: %2d: conf = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].conf, caller_get_pc());
		}
		break;

	case 0x01: /* [osc] Wavesample frequency*/
		/* freq = fc*33075/1024 in 32 voices mode, fc*44100/1024 in 24 voices mode*/
		if(msb)
			ics2115.voice[ics2115.osc].fc = (ics2115.voice[ics2115.osc].fc & 0xff)|(data << 8);
		else
			ics2115.voice[ics2115.osc].fc = (ics2115.voice[ics2115.osc].fc & 0xff00)|data;
		logerror("ICS2115: %2d: fc = %04x (%dHz) (%04x)\n", ics2115.osc,
				 ics2115.voice[ics2115.osc].fc, ics2115.voice[ics2115.osc].fc*33075/1024, caller_get_pc());
		break;

	case 0x02: /* [osc] Wavesample loop start address 19-4*/
		if(msb)
			ics2115.voice[ics2115.osc].strth = (ics2115.voice[ics2115.osc].strth & 0xff)|(data << 8);
		else
			ics2115.voice[ics2115.osc].strth = (ics2115.voice[ics2115.osc].strth & 0xff00)|data;
		logerror("ICS2115: %2d: strth = %04x (%04x)\n", ics2115.osc,
				 ics2115.voice[ics2115.osc].strth, caller_get_pc());
		break;

	case 0x03: /* [osc] Wavesample loop start address 3-0.3-0*/
		if(msb) {
			ics2115.voice[ics2115.osc].strtl = data;
			logerror("ICS2115: %2d: strtl = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].strtl, caller_get_pc());
		}
		break;

	case 0x04: /* [osc] Wavesample loop end address 19-4*/
		if(msb)
			ics2115.voice[ics2115.osc].endh = (ics2115.voice[ics2115.osc].endh & 0xff)|(data << 8);
		else
			ics2115.voice[ics2115.osc].endh = (ics2115.voice[ics2115.osc].endh & 0xff00)|data;
		logerror("ICS2115: %2d: endh = %04x (%04x)\n", ics2115.osc,
				 ics2115.voice[ics2115.osc].endh, caller_get_pc());
		break;

	case 0x05: /* [osc] Wavesample loop end address 3-0.3-0*/
		if(msb) {
			ics2115.voice[ics2115.osc].endl = data;
			logerror("ICS2115: %2d: endl = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].endl, caller_get_pc());
		}
		break;

	case 0x06: /* [osc] Volume Increment */
		if(msb) 
				ics2115.voice[ics2115.osc].incr = (ics2115.voice[ics2115.osc].incr & ~0xFF00) | (data << 8);
		else /* This is unused? */
				ics2115.voice[ics2115.osc].incr = (ics2115.voice[ics2115.osc].incr & ~0x00FF) | (data << 0);
        
		break;	
		

	case 0x07: /* [osc] Volume Start*/
		if(msb) {
			ics2115.voice[ics2115.osc].vstart = data;
			logerror("ICS2115: %2d: vstart = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].vstart, caller_get_pc());
		}
		break;

	case 0x08: /* [osc] Volume End*/
		if(msb) {
			ics2115.voice[ics2115.osc].vend = data;
			logerror("ICS2115: %2d: vend = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].vend, caller_get_pc());
		}
		break;

	case 0x09: /* [osc] Volume accumulator*/
		if(msb)
			ics2115.voice[ics2115.osc].volacc = (ics2115.voice[ics2115.osc].volacc & 0xff)|(data << 8);
		else
			ics2115.voice[ics2115.osc].volacc = (ics2115.voice[ics2115.osc].volacc & 0xff00)|data;
		logerror("ICS2115: %2d: volacc = %04x (%04x)\n", ics2115.osc,
				 ics2115.voice[ics2115.osc].volacc, caller_get_pc());
		break;

	case 0x0a: /* [osc] Wavesample address 19-4*/
		if(msb)
			ics2115.voice[ics2115.osc].addrh = (ics2115.voice[ics2115.osc].addrh & 0xff)|(data << 8);
		else
			ics2115.voice[ics2115.osc].addrh = (ics2115.voice[ics2115.osc].addrh & 0xff00)|data;
		logerror("ICS2115: %2d: addrh = %04x (%04x)\n", ics2115.osc,
				 ics2115.voice[ics2115.osc].addrh, caller_get_pc());
		break;

	case 0x0b: /* [osc] Wavesample address 3-0.8-0*/
		if(msb)
			ics2115.voice[ics2115.osc].addrl = (ics2115.voice[ics2115.osc].addrl & 0xff)|(data << 8);
		else
			ics2115.voice[ics2115.osc].addrl = (ics2115.voice[ics2115.osc].addrl & 0xff00)|data;
		logerror("ICS2115: %2d: addrl = %04x (%04x)\n", ics2115.osc,
				 ics2115.voice[ics2115.osc].addrl, caller_get_pc());
		break;


	case 0x0c: /* [osc] Pan*/
		if(msb) {
			ics2115.voice[ics2115.osc].pan = data;
			logerror("ICS2115: %2d: pan = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].pan, caller_get_pc());
		}
		break;

	case 0x0d: /* [osc] Volume Enveloppe Control*/
		if(msb) {
			ics2115.voice[ics2115.osc].vctl = data;
			logerror("ICS2115: %2d: vctl = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].vctl, caller_get_pc());
		}
		break;

	case 0x10: /* [osc] Oscillator Control*/
		if(msb) {
			ics2115.voice[ics2115.osc].ctl = data;
			logerror("ICS2115: %2d: ctl = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].ctl, caller_get_pc());
			if(data == 0)
				keyon(ics2115.osc);
		}
		break;

	case 0x11: /* [osc] Wavesample static address 27-20*/
		if(msb) {
			ics2115.voice[ics2115.osc].saddr = data;
			logerror("ICS2115: %2d: saddr = %02x (%04x)\n", ics2115.osc,
					 ics2115.voice[ics2115.osc].saddr, caller_get_pc());
		}
		break;

	case 0x40: /* Timer 1 Preset*/
		if(!msb) {
			ics2115.timer[0].preset = data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: t1preset = %d (%04x)\n", ics2115.timer[0].preset, caller_get_pc());
			recalc_timer(0);
		}
		break;

	case 0x41: /* Timer 2 Preset*/
		if(!msb) {
			ics2115.timer[1].preset = data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: t2preset = %d (%04x)\n", ics2115.timer[1].preset, caller_get_pc());
			recalc_timer(1);
		}
		break;

	case 0x42: /* Timer 1 Prescaler*/
		if(!msb) {
			ics2115.timer[0].scale = data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: t1scale = %d (%04x)\n", ics2115.timer[0].scale, caller_get_pc());
			recalc_timer(0);
		}
		break;

	case 0x43: /* Timer 2 Prescaler*/
		if(!msb) {
			ics2115.timer[1].scale = data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: t2scale = %d (%04x)\n", ics2115.timer[1].scale, caller_get_pc());
			recalc_timer(1);
		}
		break;

	case 0x4a: /* IRQ Enable*/
		if(!msb) {
			ics2115.irq_en = data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: irq_en = %02x (%04x)\n", ics2115.irq_en, caller_get_pc());
			recalc_irq();
		}
		break;

	case 0x4f: /* Oscillator Address being Programmed*/
		if(!msb) {
			ics2115.osc = data & 31;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: oscnumber = %d (%04x)\n", ics2115.osc, caller_get_pc());
		}
		break;

	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: write %02x, %02x:%d (%04x)\n", reg, data, msb, caller_get_pc());
	}
}

static UINT16 ics2115_reg_r(UINT8 reg)
{
	switch(reg) {
	case 0x06: /* [osc] Volume Increment */
			return ics2115.voice[ics2115.osc].incr;

			break;


	case 0x0d: /* [osc] Volume Enveloppe Control*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: %2d: read vctl (%04x)\n", ics2115.osc, caller_get_pc());
		/*		res = ics2115.voice[ics2115.osc].vctl << 8;*/
		/* may expect |8 on voice irq with &40 == 0*/
		/* may expect |8 on reg 0 on voice irq with &80 == 0*/
		return 0x100;

	case 0x0f:{/* [osc] Interrupt source/oscillator*/
		int osc;
		UINT8 res = 0xff;
		for(osc = 0; osc < 32; osc++)
			if(ics2115.voice[osc].state & V_DONE) {
				ics2115.voice[osc].state &= ~V_DONE;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: KEYOFF %2d\n", osc);
				recalc_irq();
				res = 0x40 | osc; /* 0x40 ? 0x80 ?*/
				break;
			}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: read irqv %02x (%04x)\n", res, caller_get_pc());
		return res << 8;
	}
		
	case 0x40: /* Timer 0 clear irq*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: clear timer 0 (%04x)\n", caller_get_pc());
		ics2115.irq_pend &= ~(1<<0);
		recalc_irq();
		return ics2115.timer[0].preset;

	case 0x41: /* Timer 1 clear irq*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: clear timer 1 (%04x)\n", caller_get_pc());
		ics2115.irq_pend &= ~(1<<1);
		recalc_irq();
		return ics2115.timer[1].preset;

	case 0x43: /* Timer status*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: read timer status %02x (%04x)\n", ics2115.irq_pend & 3, caller_get_pc());
		return ics2115.irq_pend & 3;

	case 0x4a: /* IRQ Pending*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: read irq_pend %02x (%04x)\n", ics2115.irq_pend, caller_get_pc());
		return ics2115.irq_pend;

	case 0x4b: /* Address of Interrupting Oscillator*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: %2d: read intoscaddr (%04x)\n", ics2115.osc, caller_get_pc());
		return 0x80;

	case 0x4c: /* Chip revision*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: read revision (%04x)\n", caller_get_pc());
		return 0x01;

	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: read %02x unmapped (%04x)\n", reg, caller_get_pc());
		return 0;
	}
}


int ics2115_sh_start( const struct MachineSound *msound )
{
	const char *bufp[2] = { "ICS2115 L", "ICS2115 R" };
	int i;

	memset(&ics2115, 0, sizeof(ics2115));
	ics2115.intf = msound->sound_interface;
	ics2115.rom = memory_region(ics2115.intf->region);
	ics2115.timer[0].timer = timer_alloc(timer_cb);
	ics2115.timer[1].timer = timer_alloc(timer_cb);
	ics2115.ulaw = auto_malloc(256*sizeof(INT16));
	ics2115.stream = stream_init_multi(2, bufp, ics2115.intf->mixing_level, Machine->sample_rate, 0, update);
	ics2115.voltbl = auto_malloc(8192);
   
    for (i = 0; i < 0x1000; i++) {
		ics2115.voltbl[i] = floor(pow(2.0,(((double)i/256 - 16) + 14.7)*1.06));

    }

	if(!ics2115.timer[0].timer || !ics2115.timer[1].timer || !ics2115.ulaw)
		return 1;

	for(i=0; i<256; i++) {
		UINT8 c = ~i;
		int v;
		v = ((c & 15) << 1) + 33;
		v <<= ((c & 0x70) >> 4);
		if(c & 0x80)
			v = 33-v;
		else
			v = v-33;
		ics2115.ulaw[i] = v;
	}

	return 0;
}

void ics2115_sh_stop( void )
{
}

READ_HANDLER( ics2115_r )
{
	switch(offset) {
	case 0: {
		UINT8 res = 0;
		if(ics2115.irq_on) {
			int i;
			res |= 0x80;
			if(ics2115.irq_en & ics2115.irq_pend & 3)
				res |= 1; /* Timer irq*/
			for(i=0; i<32; i++)
				if(ics2115.voice[i].state & V_DONE) {
					res |= 2;
					break;
				}
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: read status %02x (%04x)\n", res, caller_get_pc());
		
		return res;
	}
	case 1:
	    return ics2115.reg;
	case 2:
		return ics2115_reg_r(ics2115.reg);
	case 3:
	default:
		return ics2115_reg_r(ics2115.reg) >> 8;
	}
}

WRITE_HANDLER( ics2115_w )
{
	switch(offset) {
	case 1:
		ics2115.reg = data;
		break;
	case 2:
		ics2115_reg_w(ics2115.reg, data, 0);
		break;
	case 3:
		ics2115_reg_w(ics2115.reg, data, 1);
		break;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "ICS2115: wi %d, %02x (%04x)\n", offset, data, caller_get_pc());
}

void ics2115_reset(void)
{
	ics2115.irq_en = 0;
	ics2115.irq_pend = 0;
	memset(ics2115.voice, 0, sizeof(ics2115.voice));
	timer_adjust(ics2115.timer[0].timer, TIME_NEVER, 0, 0);
	timer_adjust(ics2115.timer[1].timer, TIME_NEVER, 1, 0);
	ics2115.timer[0].period = 0;
	ics2115.timer[1].period = 0;
	recalc_irq();
}
