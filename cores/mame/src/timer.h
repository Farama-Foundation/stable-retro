/***************************************************************************

  timer.c

  Functions needed to generate timing and synchronization between several
  CPUs.

***************************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

extern double cycles_to_sec[];
extern double sec_to_cycles[];

#define TIME_IN_HZ(hz)        (1.0 / (double)(hz))
#define TIME_IN_CYCLES(c,cpu) ((double)(c) * cycles_to_sec[cpu])
#define TIME_IN_SEC(s)        ((double)(s))
#define TIME_IN_MSEC(ms)      ((double)(ms) * (1.0 / 1000.0))
#define TIME_IN_USEC(us)      ((double)(us) * (1.0 / 1000000.0))
#define TIME_IN_NSEC(us)      ((double)(us) * (1.0 / 1000000000.0))

#define TIME_NOW              (0.0)
#define TIME_NEVER            (1.0e30)

#define TIME_TO_CYCLES(cpu,t) ((int)((t) * sec_to_cycles[cpu]))

typedef struct _mame_timer mame_timer;


void timer_init(void);
void timer_free(void);
double timer_time_until_next_timer(void);
void timer_adjust_global_time(double delta);
mame_timer *timer_alloc(void (*callback)(int));
void timer_adjust(mame_timer *which, double duration, int param, double period);
void timer_pulse(double period, int param, void (*callback)(int));
void timer_set(double duration, int param, void (*callback)(int));
void timer_reset(mame_timer *which, double duration);
void timer_remove(mame_timer *which);
int timer_enable(mame_timer *which, int enable);
double timer_timeelapsed(mame_timer *which);
double timer_timeleft(mame_timer *which);
double timer_get_time(void);
double timer_starttime(mame_timer *which);
double timer_firetime(mame_timer *which);

#ifdef __cplusplus
}
#endif

#endif
