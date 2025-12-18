#include "rdp_dump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *rdp_file;
static uint8_t *rdp_dram_cache;
static uint8_t *rdp_hidden_dram_cache;

enum rdp_dump_cmd
{
	RDP_DUMP_CMD_INVALID = 0,
	RDP_DUMP_CMD_UPDATE_DRAM = 1,
	RDP_DUMP_CMD_RDP_COMMAND = 2,
	RDP_DUMP_CMD_SET_VI_REGISTER = 3,
	RDP_DUMP_CMD_END_FRAME = 4,
	RDP_DUMP_CMD_SIGNAL_COMPLETE = 5,
	RDP_DUMP_CMD_EOF = 6,
	RDP_DUMP_CMD_UPDATE_DRAM_FLUSH = 7,
	RDP_DUMP_CMD_UPDATE_HIDDEN_DRAM = 8,
	RDP_DUMP_CMD_UPDATE_HIDDEN_DRAM_FLUSH = 9,
	RDP_DUMP_CMD_INT_MAX = 0x7fffffff
};

bool rdp_dump_init(const char *path, uint32_t dram_size, uint32_t hidden_dram_size)
{
	if (rdp_file)
		return false;

	free(rdp_dram_cache);
	rdp_dram_cache = calloc(1, dram_size);
	if (!rdp_dram_cache)
		return false;
	rdp_hidden_dram_cache = calloc(1, hidden_dram_size);
	if (!rdp_hidden_dram_cache)
	{
		free(rdp_dram_cache);
		rdp_dram_cache = NULL;
		return false;
	}

	rdp_file = fopen(path, "wb");
	if (!rdp_file)
	{
		free(rdp_dram_cache);
		free(rdp_hidden_dram_cache);
		rdp_dram_cache = NULL;
		rdp_hidden_dram_cache = NULL;
		return false;
	}

	fwrite("RDPDUMP2", 8, 1, rdp_file);
	fwrite(&dram_size, sizeof(dram_size), 1, rdp_file);
	fwrite(&hidden_dram_size, sizeof(hidden_dram_size), 1, rdp_file);
	return true;
}

void rdp_dump_end_frame(void)
{
	if (!rdp_file)
		return;

	uint32_t cmd = RDP_DUMP_CMD_END_FRAME;
	fwrite(&cmd, sizeof(cmd), 1, rdp_file);
}

void rdp_dump_end(void)
{
	if (!rdp_file)
		return;

	uint32_t cmd = RDP_DUMP_CMD_EOF;
	fwrite(&cmd, sizeof(cmd), 1, rdp_file);

	fclose(rdp_file);
	rdp_file = NULL;

	free(rdp_dram_cache);
	rdp_dram_cache = NULL;
	free(rdp_hidden_dram_cache);
	rdp_hidden_dram_cache = NULL;
}

static void rdp_dump_flush(const void *dram_, uint32_t size,
		enum rdp_dump_cmd block_cmd, enum rdp_dump_cmd flush_cmd, uint8_t *cache)
{
	if (!rdp_file)
		return;

	const uint8_t *dram = dram_;
	const uint32_t block_size = 4 * 1024;
	uint32_t i = 0;

	for (i = 0; i < size; i += block_size)
	{
		if (memcmp(dram + i, cache + i, block_size))
		{
			uint32_t cmd = block_cmd;
			fwrite(&cmd, sizeof(cmd), 1, rdp_file);
			fwrite(&i, sizeof(i), 1, rdp_file);
			fwrite(&block_size, sizeof(block_size), 1, rdp_file);
			fwrite(dram + i, 1, block_size, rdp_file);
			memcpy(cache + i, dram + i, block_size);
		}
	}

	uint32_t cmd = flush_cmd;
	fwrite(&cmd, sizeof(cmd), 1, rdp_file);

}

void rdp_dump_flush_dram(const void *dram_, uint32_t size)
{
	rdp_dump_flush(dram_, size, RDP_DUMP_CMD_UPDATE_DRAM, RDP_DUMP_CMD_UPDATE_DRAM_FLUSH, rdp_dram_cache);
}

void rdp_dump_flush_hidden_dram(const void *dram_, uint32_t size)
{
	rdp_dump_flush(dram_, size, RDP_DUMP_CMD_UPDATE_HIDDEN_DRAM, RDP_DUMP_CMD_UPDATE_HIDDEN_DRAM_FLUSH, rdp_hidden_dram_cache);
}

void rdp_dump_signal_complete(void)
{
	if (!rdp_file)
		return;

	uint32_t cmd = RDP_DUMP_CMD_SIGNAL_COMPLETE;
	fwrite(&cmd, sizeof(cmd), 1, rdp_file);
}

void rdp_dump_emit_command(uint32_t command, const uint32_t *cmd_data, uint32_t cmd_words)
{
	if (!rdp_file)
		return;

	uint32_t cmd = RDP_DUMP_CMD_RDP_COMMAND;
	fwrite(&cmd, sizeof(cmd), 1, rdp_file);
	fwrite(&command, sizeof(command), 1, rdp_file);
	fwrite(&cmd_words, sizeof(cmd_words), 1, rdp_file);
	fwrite(cmd_data, sizeof(*cmd_data), cmd_words, rdp_file);
}

void rdp_dump_set_vi_register(uint32_t vi_register, uint32_t value)
{
	if (!rdp_file)
		return;

	uint32_t cmd = RDP_DUMP_CMD_SET_VI_REGISTER;
	fwrite(&cmd, sizeof(cmd), 1, rdp_file);
	fwrite(&vi_register, sizeof(vi_register), 1, rdp_file);
	fwrite(&value, sizeof(value), 1, rdp_file);
}
