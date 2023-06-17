#ifndef STREAMS_H
#define STREAMS_H

#ifdef __cplusplus
extern "C" {
#endif

void set_RC_filter(int channel,int R1,int R2,int R3,int C);

int streams_sh_start(void);
void streams_sh_stop(void);
void streams_sh_update(void);

int stream_init(const char *name,int default_mixing_level,
		int sample_rate,
		int param,void (*callback)(int param,INT16 *buffer,int length));
int stream_init_multi(int channels,const char **names,const int *default_mixing_levels,
		int sample_rate,
		int param,void (*callback)(int param,INT16 **buffer,int length));
void stream_update(int channel,int min_interval);	/* min_interval is in usec */

#ifdef __cplusplus
}
#endif

#endif
