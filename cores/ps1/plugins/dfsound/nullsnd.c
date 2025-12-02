#include "out.h"

// SETUP SOUND
static int none_init(void)
{
  return 0;
}

// REMOVE SOUND
static void none_finish(void)
{
}

// GET BYTES BUFFERED
static int none_busy(void)
{
  return 1;
}

// FEED SOUND DATA
static void none_feed(void *buf, int bytes)
{
}

void out_register_none(struct out_driver *drv)
{
	drv->name = "none";
	drv->init = none_init;
	drv->finish = none_finish;
	drv->busy = none_busy;
	drv->feed = none_feed;
}
