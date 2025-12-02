
extern u32 zero_mem[0x1000/4];

void new_dyna_pcsx_mem_init(void);
void new_dyna_pcsx_mem_reset(void);
void new_dyna_pcsx_mem_load_state(void);
void new_dyna_pcsx_mem_isolate(int enable);
void new_dyna_pcsx_mem_shutdown(void);

int pcsxmem_is_handler_dynamic(unsigned int addr);
