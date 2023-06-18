/***********************************************************

  random.h

  Mame's own random number generator.
  For now, this is the Mersenne Twister.

***********************************************************/

#ifndef MAME_RAND_H
#define MAME_RAND_H

#ifdef __cplusplus
extern "C" {
#endif

/* initializes random number generator with a seed */
void mame_srand(unsigned long s);

/* generates a random number on [0,0xffffffff]-interval */
unsigned long mame_rand(void);

#ifdef __cplusplus
}
#endif

#endif
