/**
 * @file memdbg.h
 */
#ifndef MEMDBG_H
#define MEMDBG_H

#include <stdio.h>
#include <stdlib.h>

/* protypes for MALLOC, FREE macros.
 *
 * code using memdbg.c for tracing should always use MALLOC, FREE instead of direct calls
 * to malloc, free.
 */
void *MALLOC( size_t lSize );
void FREE( void *pMem );

/**
 * dumps a list of all current allocations, with module, line number, and allocation size
 */
extern void memdbg_Debug( void );

#if defined(USE_MEMDBG)
/* memory tracking is enabled; reroute the MALLOC, FREE macros to the appropriate functions
 * in memdbg.c
 */
	extern void *memdbg_Alloc( size_t lSize, const char *pFile, int line );
	extern void memdbg_Free( void *pMem );
	#define MALLOC( _SIZE ) memdbg_Alloc( (_SIZE) ,__FILE__, __LINE__)
	#define FREE( _PTR ) memdbg_Free( (_PTR) )
#else
/* memory tracking is not enabled; just route MALLOC, FREE macros directly to the core
 * malloc, free routines.
 */
	#define MALLOC( _SIZE ) malloc( (_SIZE) )
	#define FREE( _PTR ) free( (_PTR) )
#endif /* defined(MEMDBG) */

#endif /* MEMDBG_H */
