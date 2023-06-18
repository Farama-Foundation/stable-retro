/**
 * @brief memdbg.c
 */

/*

To use, #include "memdbg.h"
Always call MALLOC and FREE instead of malloc,free.
If the symbol USE_MEMDBG is defined, MALLOC and FREE are rerouted to
special tracking routines, that also check for corruption, bogus frees,
etc.

You can call memdbg_Debug() at any time to get a detailed dump of all
outstanding allocations, listing the pointer, size, module number, and
line number.

Phil

*/

#include <string.h>
#include <assert.h>
#include "memdbg.h"

/* MEM_UNINITIALIZED_VALUE gets copied to memory regions when they are alloc'd or free'd */
#define MEM_UNINITIALIZED_VALUE	0x0A

/* mGuardData is a signature that is written at the end of every allocated block, to help
 * detect corruption due to overwriting an allocated memory pointer.
 */
static const char mGuardData[] = {'d','o','n','o','t','o','v','e','r','w','r','i','t','e'};

struct MemPtr
{
	const char *pModule;
	int line;
	size_t numBytes;
	struct MemPtr *pNextAllocation;
	/* This header is exactly 16 bytes; it should not interfere with any alignment
	 * requirements for the data chunk itself, which immediately follows this header.
	 *
	 * uint8_t data[numBytes];
	 * uint8_t guardData[]; /* copy of mGuardData string*/
	 */
};
typedef struct MemPtr MemPtr;

void *MEMPTR_TO_PDATA( MemPtr * );
/* The following macro provides syntactic sugar to get from a raw MemPtr to the payload. */
#define MEMPTR_TO_PDATA( MEMPTR ) ((void *)((MEMPTR)+1))


void *PDATA_TO_MEMPTR( void * );
#define PDATA_TO_MEMPTR( MEMPTR ) (((MemPtr *)(MEMPTR))-1)

/* head of linked list containing all currently allocated chunks of memory */
static MemPtr *mpFirstAllocation;

/* BeginCriticalSection and EndCriticalSection must be implemented if this code is used
 * from multiple threads.
 */
static void BeginCriticalSection( void ){ /* STUB */ }
static void EndCriticalSection( void ){ /* STUB */ }

void
memdbg_Debug( void )
{
#if defined(USE_MEMDBG)
	MemPtr *pMemPtr;

	BeginCriticalSection();

	pMemPtr = mpFirstAllocation;
	log_cb(RETRO_LOG_ERROR, LOGPRE  "\nmemdbg allocations:\n" );
	while( pMemPtr )
	{
		printf( "%p\tsize: %d\t file %s line %d\n",
			MEMPTR_TO_PDATA(pMemPtr),
			pMemPtr->numBytes,
			pMemPtr->pModule,
			pMemPtr->line );

		pMemPtr = pMemPtr->pNextAllocation;
	}

	EndCriticalSection();
#endif
} /* memdbg_Debug */

void *
memdbg_Alloc( size_t numBytes, const char *pModule, int line )
{
	void *pData;

	BeginCriticalSection();

	pData = NULL; /* default */
	if( !numBytes )
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE  "WARNING: attempt to memdbg_Alloc zero bytes!\n" );
	}
	else
	{
		MemPtr *pMemPtr = malloc(sizeof(MemPtr)+numBytes+sizeof(mGuardData));
		if( !pMemPtr )
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE  "memdbg_Alloc failure: insufficient memory!\n" );
		}
		else
		{
			pMemPtr->pNextAllocation = mpFirstAllocation;
			mpFirstAllocation = pMemPtr;
			pMemPtr->numBytes = numBytes;
			pMemPtr->pModule = pModule;
			pMemPtr->line = line;
			pData = MEMPTR_TO_PDATA(pMemPtr);
			memset( pData, MEM_UNINITIALIZED_VALUE, numBytes );
			memcpy( numBytes+(char *)pData,mGuardData,sizeof(mGuardData) );
		}
	}

	EndCriticalSection();

	return pData;
} /* memdbg_Alloc */

void
memdbg_Free( void *pData )
{
	BeginCriticalSection();

	if( !pData )
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE  "WARNING: attempt to memdbg_Free a NULL pointer!\n" );
	}
	else
	{
		const char *pErrorString = "attempt to memdbg_Free a bogus pointer";

		MemPtr *pMemPtr = PDATA_TO_MEMPTR(pData);
		if( memcmp(pMemPtr->numBytes+(char *)pData, mGuardData, sizeof(mGuardData) )!=0 )
		{
			pErrorString = "corruption detected";
		}
		else
		{
			MemPtr *pPrevAlloc = NULL;
			MemPtr *pCurAlloc = mpFirstAllocation;
			while( pCurAlloc )
			{
				if( pCurAlloc == pMemPtr )
				{
					if( pPrevAlloc )
					{
						pPrevAlloc->pNextAllocation = pMemPtr->pNextAllocation;
					}
					else
					{
						mpFirstAllocation = pMemPtr->pNextAllocation;
					}
					memset( pData, MEM_UNINITIALIZED_VALUE, pMemPtr->numBytes );
					free( pMemPtr );
					pErrorString = NULL;
					break;
				}
				pPrevAlloc = pCurAlloc;
				pCurAlloc = pCurAlloc->pNextAllocation;
			}
		}
		if( pErrorString )
		{
			printf( "ERROR!!! %s!: %p %s.%d\n",
				pErrorString,
				pData,
				pMemPtr->pModule,
				pMemPtr->line );

			assert(0);
		}
	}

	EndCriticalSection();
} /* memdbg_Free */
