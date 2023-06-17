/*****************************************************************************

  MAME/MESS NES APU CORE

  Based on the Nofrendo/Nosefart NES N2A03 sound emulation core written by
  Matthew Conte (matt@conte.com) and redesigned for use in MAME/MESS by
  Who Wants to Know? (wwtk@mail.com)

  This core is written with the advise and consent of Matthew Conte and is
  released under the GNU Public License.  This core is freely avaiable for
  use in any freeware project, subject to the following terms:

  Any modifications to this code must be duly noted in the source and
  approved by Matthew Conte and myself prior to public submission.

 *****************************************************************************

   NES_APU.H

   NES APU external interface.

 *****************************************************************************/

#ifndef NES_APU_H
#define NES_APU_H

#include "driver.h"
#define MAX_NESPSG 2

/* AN EXPLANATION
 *
 * The NES APU is actually integrated into the Nintendo processor.
 * You must supply the same number of APUs as you do processors.
 * Also make sure to correspond the memory regions to those used in the
 * processor, as each is shared.
 */
struct NESinterface
{
   int num;                 /* total number of chips in the machine */
   int region[MAX_NESPSG];  /* DMC regions */
   int volume[MAX_NESPSG];
};

READ_HANDLER( NESPSG_0_r );
READ_HANDLER( NESPSG_1_r );
WRITE_HANDLER( NESPSG_0_w );
WRITE_HANDLER( NESPSG_1_w );

extern int NESPSG_sh_start(const struct MachineSound *);
extern void NESPSG_sh_stop(void);
extern void NESPSG_sh_update(void);

#endif
