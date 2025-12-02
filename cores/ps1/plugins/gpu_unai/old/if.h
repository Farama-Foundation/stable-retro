#ifndef GPU_UNAI_NO_OLD

struct rearmed_cbs;

void oldunai_renderer_init(void);
int  oldunai_do_cmd_list(uint32_t *list, int list_len,
       int *cycles_sum_out, int *cycles_last, int *last_cmd);
void oldunai_renderer_sync_ecmds(uint32_t *ecmds);
void oldunai_renderer_set_config(const struct rearmed_cbs *cbs);

#else

#define oldunai_renderer_init()
#define oldunai_do_cmd_list(...) 0
#define oldunai_renderer_sync_ecmds(x)
#define oldunai_renderer_set_config(x)

#endif
