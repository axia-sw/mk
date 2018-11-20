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
#pragma once

/*
 *	========================================================================
 *	MEMORY CODE
 *	========================================================================
 *	Utility functions for managing memory. Allocated objects can have
 *	hierarchies of memory, such that when a "super" node is deallocated, its
 *	sub nodes are also deallocated. Destructor functions can be assigned as
 *	well to do other clean-up tasks. Reference counting is supported when
 *	ownership isn't as simple. (Objects allocated default to a reference
 *	count of one.)
 */

#include "mk-basic-types.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"

#ifndef MK_MEM_LOCTRACE_ENABLED
#	if MK_DEBUG_ENABLED
#		define MK_MEM_LOCTRACE_ENABLED 1
#	else
#		define MK_MEM_LOCTRACE_ENABLED 0
#	endif
#endif

#ifndef MK_LOG_MEMORY_ALLOCS_ENABLED
#	define MK_LOG_MEMORY_ALLOCS_ENABLED 0
#endif

typedef void ( *MkMem_Fini_fn_t )( void * );

struct MkMem__Hdr_s {
	struct MkMem__Hdr_s *pPrnt;
	struct MkMem__Hdr_s *pPrev, *pNext;
	struct MkMem__Hdr_s *pHead, *pTail;
	MkMem_Fini_fn_t pfnFini;
	size_t cRefs;
	size_t cBytes;
#if MK_MEM_LOCTRACE_ENABLED
	const char *pszFile;
	unsigned int uLine;
	const char *pszFunction;
#endif
};

enum {
	/* do not zero out the memory (caller *WILL* initialize it immediately) */
	kMkMemF_Uninitialized = 1 << 0
};

#define mk_mem_maybeAllocEx( cBytes_, uFlags_ ) ( mk_mem__maybeAlloc( ( cBytes_ ), ( uFlags_ ), __FILE__, __LINE__, MK_CURFUNC ) )
#define mk_mem_maybeAlloc( cBytes_ ) ( mk_mem__maybeAlloc( ( cBytes_ ), 0, __FILE__, __LINE__, MK_CURFUNC ) )
#define mk_mem_allocEx( cBytes_, uFlags_ ) ( mk_mem__alloc( ( cBytes_ ), ( uFlags_ ), __FILE__, __LINE__, MK_CURFUNC ) )
#define mk_mem_alloc( cBytes_ ) ( mk_mem__alloc( ( cBytes_ ), 0, __FILE__, __LINE__, MK_CURFUNC ) )
#define mk_mem_dealloc( pBlock_ ) ( mk_mem__dealloc( (void *)( pBlock_ ), __FILE__, __LINE__, MK_CURFUNC ) )
#define mk_mem_addRef( pBlock_ ) ( mk_mem__addRef( (void *)( pBlock_ ), __FILE__, __LINE__, MK_CURFUNC ) )
#define mk_mem_attach( pBlock_, pSuperBlock_ ) ( mk_mem__attach( (void *)( pBlock_ ), (void *)( pSuperBlock_ ), __FILE__, __LINE__, MK_CURFUNC ) )
#define mk_mem_detach( pBlock_ ) ( mk_mem__detach( (void *)( pBlock_ ), __FILE__, __LINE__, MK_CURFUNC ) )
#define mk_mem_setFini( pBlock_, pfnFini_ ) ( mk_mem__setFini( (void *)( pBlock_ ), ( MkMem_Fini_fn_t )( pfnFini_ ) ) )
#define mk_mem_size( pBlock_ ) ( mk_mem__size( (const void *)( pBlock_ ) ) )

void *mk_mem__maybeAlloc( size_t cBytes, bitfield_t uFlags, const char *pszFile, unsigned int uLine, const char *pszFunction );
void *mk_mem__alloc( size_t cBytes, bitfield_t uFlags, const char *pszFile, unsigned int uLine, const char *pszFunction );
void *mk_mem__dealloc( void *pBlock, const char *pszFile, unsigned int uLine, const char *pszFunction );
void *mk_mem__addRef( void *pBlock, const char *pszFile, unsigned int uLine, const char *pszFunction );
void *mk_mem__attach( void *pBlock, void *pSuperBlock, const char *pszFile, unsigned int uLine, const char *pszFunction );
void *mk_mem__detach( void *pBlock, const char *pszFile, unsigned int uLine, const char *pszFunction );
void *mk_mem__setFini( void *pBlock, MkMem_Fini_fn_t pfnFini );
size_t mk_mem__size( const void *pBlock );
