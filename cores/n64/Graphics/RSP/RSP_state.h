#ifndef _RSP_STATE_H
#define _RSP_STATE_H

#include <stdint.h>

#include <boolean.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DL_STACK_SIZE 32

typedef struct
{
   /* Display list stack */
   uint32_t countdown[MAX_DL_STACK_SIZE];
   uint32_t PC[MAX_DL_STACK_SIZE];     /* Display List Program Counter stack */
   int64_t  PCi;                       /* Current Program Counter index on the stack */

   uint32_t busy;
   uint32_t halt;                      /* Marks the end of Display List execution */
   uint32_t DList;
   uint32_t close;
   uint32_t uc_start;
   uint32_t uc_dstart;
   uint32_t cmd;
   uint32_t nextCmd;
	int32_t  count;       /* Number of instructions before returning */
	bool bLLE;            /* LLE mode enabled? */

   /* Next command */
   uint32_t w0;
   uint32_t w1;

	char romname[21];
} RSPInfo;

extern RSPInfo __RSP;

void RSP_CheckDLCounter(void);

#ifdef __cplusplus
}
#endif

#endif
