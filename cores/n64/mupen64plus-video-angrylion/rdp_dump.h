#ifndef RDP_DUMP_H
#define RDP_DUMP_H

#include <stdint.h>
#include <boolean.h>

bool rdp_dump_init(const char *path, uint32_t dram_size, uint32_t hidden_dram_size);
void rdp_dump_end(void);
void rdp_dump_flush_dram(const void *dram, uint32_t size);
void rdp_dump_flush_hidden_dram(const void *dram, uint32_t size);

void rdp_dump_signal_complete(void);
void rdp_dump_emit_command(uint32_t command, const uint32_t *cmd_data, uint32_t cmd_words);

void rdp_dump_set_vi_register(uint32_t vi_register, uint32_t value);
void rdp_dump_end_frame(void);

#endif
