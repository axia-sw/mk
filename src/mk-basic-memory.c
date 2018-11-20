/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "mk-basic-memory.h"

#include "mk-basic-assert.h"
#include "mk-basic-debug.h"
#include "mk-basic-logging.h"
#include "mk-basic-types.h"

#include <stdlib.h>
#include <string.h>

static void mk_mem__unlink( struct MkMem__Hdr_s *pHdr ) {
	if( pHdr->pPrev != NULL ) {
		pHdr->pPrev->pNext = pHdr->pNext;
	} else if( pHdr->pPrnt != NULL ) {
		pHdr->pPrnt->pHead = pHdr->pNext;
	}

	if( pHdr->pNext != NULL ) {
		pHdr->pNext->pPrev = pHdr->pPrev;
	} else if( pHdr->pPrnt != NULL ) {
		pHdr->pPrnt->pTail = pHdr->pPrev;
	}

	pHdr->pPrnt = NULL;
	pHdr->pPrev = NULL;
	pHdr->pNext = NULL;
}

/*
================
mk_mem__maybeAlloc

Potentially allocate memory. (Returns NULL on failure.)
================
*/
void *mk_mem__maybeAlloc( size_t cBytes, bitfield_t uFlags, const char *pszFile, unsigned int uLine, const char *pszFunction ) {
	struct MkMem__Hdr_s *pHdr;
	void *p;

	pHdr = (struct MkMem__Hdr_s *)malloc( sizeof( *pHdr ) + cBytes );
	if( !pHdr ) {
		return NULL;
	}

	p = (void *)( pHdr + 1 );

	pHdr->pPrnt   = NULL;
	pHdr->pPrev   = NULL;
	pHdr->pNext   = NULL;
	pHdr->pHead   = NULL;
	pHdr->pTail   = NULL;
	pHdr->pfnFini = NULL;
	pHdr->cRefs   = 1;
	pHdr->cBytes  = cBytes;
#if MK_MEM_LOCTRACE_ENABLED
	pHdr->pszFile     = pszFile;
	pHdr->uLine       = uLine;
	pHdr->pszFunction = pszFunction;
#else
	(void)pszFile;
	(void)uLine;
	(void)pszFunction;
#endif

	if( ( uFlags & kMkMemF_Uninitialized ) == 0 ) {
		memset( p, 0, cBytes );
	}

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_outf( "ALLOC: %s(%i) in %s: %p, %u;", pszFile, uLine, pszFunction, p,
	    (unsigned int)cBytes );
#endif

	return p;
}
/*
================
mk_mem__alloc

Allocate memory without returning NULL. If an allocation fails, provide an error
then exit the process.
================
*/
void *mk_mem__alloc( size_t cBytes, bitfield_t uFlags, const char *pszFile, unsigned int uLine, const char *pszFunction ) {
	void *p;

	p = mk_mem__maybeAlloc( cBytes, uFlags, pszFile, uLine, pszFunction );
	if( !p ) {
		mk_log_fatalError( "Out of memory" );
	}

	return p;
}
/*
================
mk_mem__dealloc

Decrement the reference count of the memory block. If the reference count
reaches zero then deallocate all sub-blocks of memory. Throw an error if any of
the sub-blocks have a reference count greater than one.
================
*/
void *mk_mem__dealloc( void *pBlock, const char *pszFile, unsigned int uLine, const char *pszFunction ) {
	struct MkMem__Hdr_s *pHdr;

	if( !pBlock ) {
		return NULL;
	}

	MK_ASSERT( (size_t)pBlock > 4096 );

	pHdr = (struct MkMem__Hdr_s *)pBlock - 1;

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_outf( "DEALLOC: %s(%i) in %s: %s%p, %u (refcnt=%u);", pszFile, uLine,
	    pszFunction, pHdr->pPrnt != NULL ? "[sub]" : "", pBlock,
	    (unsigned int)pHdr->cBytes, (unsigned int)pHdr->cRefs );
#endif

	MK_ASSERT( pHdr->cRefs > 0 );
	if( --pHdr->cRefs != 0 ) {
		return NULL;
	}

	while( pHdr->pHead != NULL ) {
		MK_ASSERT( pHdr->pHead->cRefs <= 1 );
		mk_mem__dealloc( (void *)( pHdr->pHead + 1 ), pszFile, uLine, pszFunction );
	}

	if( pHdr->pfnFini != NULL ) {
		pHdr->pfnFini( pBlock );
	}

	mk_mem__unlink( pHdr );

	free( (void *)pHdr );
	return NULL;
}
/*
================
mk_mem__addRef

Increase the reference count of a memory block. Returns the address passed in.
================
*/
void *mk_mem__addRef( void *pBlock, const char *pszFile, unsigned int uLine, const char *pszFunction ) {
	struct MkMem__Hdr_s *pHdr;

	MK_ASSERT( pBlock != NULL );

	pHdr = (struct MkMem__Hdr_s *)pBlock - 1;

	++pHdr->cRefs;

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_outf( "MEM-ADDREF: %s(%i) in %s: %p, %u (refcnt=%u);", pszFile, uLine,
	    pszFunction, pBlock, (unsigned int)cBytes, (unsigned int)pHdr->cRefs );
#else
	(void)pszFile;
	(void)uLine;
	(void)pszFunction;
#endif

	return pBlock;
}
/*
================
mk_mem__attach

Set a block to be the sub-block of some super-block. The sub-block will be
deallocated when the super-block is deallocated. The sub-block does NOT have its
reference count increased upon being attached to the super-block.

The super-block (pSuperBlock) cannot be NULL. To remove a block from an existing
super-block, call mk_mem__detach().

Returns the address of the sub-block (pBlock) passed in.
================
*/
void *mk_mem__attach( void *pBlock, void *pSuperBlock, const char *pszFile, unsigned int uLine, const char *pszFunction ) {
	struct MkMem__Hdr_s *pHdr;
	struct MkMem__Hdr_s *pSuperHdr;

	MK_ASSERT( pBlock != NULL );
	MK_ASSERT( pSuperBlock != NULL );

	pHdr      = (struct MkMem__Hdr_s *)pBlock - 1;
	pSuperHdr = (struct MkMem__Hdr_s *)pSuperBlock - 1;

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_outf( "MEM-ATTACH: %s(%i) in %s: %p(%u)->%p(%u);",
	    pszFile, uLine, pszFunction,
	    pBlock, (unsigned int)pBlock->cBytes,
	    pSuperBlock, (unsigned int)pSuperBlock->cBytes );
#else
	(void)pszFile;
	(void)uLine;
	(void)pszFunction;
#endif

	mk_mem__unlink( pHdr );

	pHdr->pPrnt = pSuperHdr;
	pHdr->pPrev = NULL;
	pHdr->pNext = pSuperHdr->pHead;
	if( pSuperHdr->pHead != NULL ) {
		pSuperHdr->pHead->pPrev = pHdr;
	} else {
		pSuperHdr->pTail = pHdr;
	}
	pSuperHdr->pHead = pHdr;

	return pBlock;
}
/*
================
mk_mem__detach

Remove a block from the super-block it is attached to, if any. This ensures the
block will not be deallocated when its super-block is allocated. (Useful when
new ownership is necessary.)

Only call this function if absolutely necessary. Changing or removing ownership
is discouraged.

Returns the address passed in (pBlock).
================
*/
void *mk_mem__detach( void *pBlock, const char *pszFile, unsigned int uLine, const char *pszFunction ) {
	struct MkMem__Hdr_s *pHdr;

	MK_ASSERT( pBlock != NULL );

	pHdr = (struct MkMem__Hdr_s *)pBlock - 1;

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_outf( "MEM-DETACH: %s(%i) in %s: %p(%u);",
	    pszFile, uLine, pszFunction,
	    pBlock, (unsigned int)pBlock->cBytes );
#else
	(void)pszFile;
	(void)uLine;
	(void)pszFunction;
#endif

	mk_mem__unlink( pHdr );
	return pBlock;
}
/*
================
mk_mem__setFini

Set the function to call when the program is finished with the block of memory.
(i.e., the clean-up / destructor function.)

Returns the address passed in (pBlock).
================
*/
void *mk_mem__setFini( void *pBlock, MkMem_Fini_fn_t pfnFini ) {
	struct MkMem__Hdr_s *pHdr;

	MK_ASSERT( pBlock != NULL );

	pHdr          = (struct MkMem__Hdr_s *)pBlock - 1;
	pHdr->pfnFini = pfnFini;

	return pBlock;
}
/*
================
mk_mem__size

Retrieve the size of a block of memory, in bytes.
================
*/
size_t mk_mem__size( const void *pBlock ) {
	const struct MkMem__Hdr_s *pHdr;

	if( !pBlock ) {
		return 0;
	}

	pHdr = (const struct MkMem__Hdr_s *)pBlock - 1;

	return pHdr->cBytes;
}
