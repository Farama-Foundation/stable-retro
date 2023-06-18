#ifndef DATAFILE_H
#define DATAFILE_H

struct tDatafileIndex
{
	long offset;
	const struct GameDriver *driver;
};

extern int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize);

#endif
