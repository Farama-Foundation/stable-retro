#ifndef __PL_GUN_TS_H__
#define __PL_GUN_TS_H__

#ifdef HAVE_TSLIB

struct tsdev;

struct tsdev *pl_gun_ts_init(void);
void pl_gun_ts_update(struct tsdev *ts, int *x, int *y, int *in);
void pl_set_gun_rect(int x, int y, int w, int h);

int pl_gun_ts_update_raw(struct tsdev *ts, int *x, int *y, int *p);
int pl_gun_ts_get_fd(struct tsdev *ts);

#else

#define pl_gun_ts_init() NULL
#define pl_gun_ts_update(...) do {} while (0)
#define pl_set_gun_rect(...) do {} while (0)

#endif

#endif /* __PL_GUN_TS_H__ */
