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
#include "mk-build-node.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-memory.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-stringList.h"

#include <stddef.h>


/*

	MEMORY ALLOCATION SHENANIGANS

	The following section of code is just a simple implementation of a paged
	linear allocator. It's used with the intention to improve performance in
	setting up the node system, as a lot of small allocations are made and
	kept around for the duration of the build.

*/

enum { LINEAR_ALLOC_BUF_MAX = 16384UL };
enum { LINEAR_ALLOC_ALIGN = 16UL };

typedef struct allocPage_s {
	char buf[ LINEAR_ALLOC_BUF_MAX ];
	struct allocPage_s *next;

	unsigned int index;
} allocPage_t;

#if defined(__GCC__) || defined(__clang__)
__attribute__((pure))
#endif
static unsigned int align_up( unsigned int x, unsigned int a ) {
	return x + ( a - x%a )%a;
}

MK_UNUSED
static void ap_zero( allocPage_t *ap ) {
	ap->next  = (allocPage_t*)0;
	ap->index = 0;
}
static allocPage_t *ap_new( allocPage_t *ap ) {
	allocPage_t *page;

	page = (allocPage_t*)mk_mem_alloc( sizeof(*page) );
	MK_ASSERT( page != (allocPage_t*)0 );

	memset( (void*)&ap->buf[0], 0, LINEAR_ALLOC_BUF_MAX );

	page->next  = ap;
	page->index = 0;

	return page;
}
MK_UNUSED
static allocPage_t *ap_delete_r( allocPage_t *ap ) {
	if( ap != (allocPage_t *)0 ) {
		ap_delete_r( ap->next );
		ap->next = (allocPage_t *)0;
	}

	mk_mem_dealloc( (void*)ap );
	return (allocPage_t *)0;
}
MK_UNUSED
static void *ap_alloc( allocPage_t **apbase, size_t n ) {
	allocPage_t *ap;
	void *p;

	MK_ASSERT( apbase != (allocPage_t **)0 );

	if( !n ) {
		return (void *)0;
	}

	MK_ASSERT_MSG( n <= LINEAR_ALLOC_BUF_MAX, "Allocation size not supported" );

	ap = *apbase;

	if( ( (size_t)ap->index ) + n > LINEAR_ALLOC_BUF_MAX ) {
		ap = ap_new( ap );
		*apbase = ap;
	}

	p = (void *)&ap->buf[ ap->index ];
	ap->index = align_up( ap->index + n, LINEAR_ALLOC_ALIGN );

	return p;
}


/*

	NODE ARRAYS

	These are just a convenience structure for the internal implementation of
	the build nodes. Memory management is not necessarily uniform between them
	all.

*/

typedef struct nodeArray_s {
	unsigned int num;
	MkBuildNode *nodes;
} nodeArray_t;

enum { NODE_ARRAY_CAPACITY_ALIGN = 32UL };

static unsigned int na_inferCapacity( const nodeArray_t *na ) {
	if( na->num == 0 ) {
		return 0;
	}

	return align_up( na->num, NODE_ARRAY_CAPACITY_ALIGN );
}

static void na_init( nodeArray_t *na ) {
	na->num   = 0;
	na->nodes = (MkBuildNode *)0;
}

MK_UNUSED
static void na_fini( nodeArray_t *na ) {
	na->nodes = (MkBuildNode *)mk_com_memory( (void *)na->nodes, 0 );
	na->num   = 0;
}

static void na_resize( nodeArray_t *na, mk_uint32_t n ) {
	mk_uint32_t oldCapacity, newCapacity;
	mk_uint32_t i;

	if( !n ) {
		return;
	}

	oldCapacity = na_inferCapacity( na );
	newCapacity = align_up( n, NODE_ARRAY_CAPACITY_ALIGN );

	i = na->num;
	na->num = n;

	if( oldCapacity != newCapacity ) {
		na->nodes = (MkBuildNode *)mk_com_memory( (void *)na->nodes, sizeof(MkBuildNode)*newCapacity );
	}

	for(; i < na->num; ++i ) {
		na->nodes[ i ] = (MkBuildNode)0;
	}
}

MK_UNUSED
static MkBuildNode na_pushBack( nodeArray_t *na, MkBuildNode node ) {
	mk_uint32_t i;

	i = na->num;
	
	na_resize( na, na->num + 1 );
	na->nodes[ i ] = node;

	return node;
}


/*

	BUILD NODES

*/

struct MkBuildNode_s {
	MkBuildContext ctx;

	struct {
		mk_uint32_t inputsRemaining;
	} async;

	nodeArray_t inputs;
	nodeArray_t outputs;

	char *      filename;
	mk_uint32_t flags;

	mk_build_func_t     pfn_build;
	mk_checkDeps_func_t pfn_checkDeps;
	void *              userData;
};

static mk_uint32_t bldno_countInputs_r( MkBuildNode bldno, mk_uint32_t n ) {
	mk_uint32_t i;

	for( i = 0; i < bldno->inputs.num; ++i ) {
		n += bldno_countInputs_r( bldno->inputs.nodes[ i ], n );
	}

	n += bldno->inputs.num;

	return n;
}
static mk_uint32_t bldno_countInputs( MkBuildNode bldno ) {
	return bldno_countInputs_r( bldno, 0 );
}

MK_UNUSED
static void bldno_resetSync( MkBuildNode bldno ) {
	bldno->async.inputsRemaining = bldno->inputs.num;
}
MK_UNUSED
static int bldno_isReady( MkBuildNode bldno ) {
	return +( bldno->async.inputsRemaining == 0 );
}
static void bldno_syncCompletion( MkBuildNode bldno ) {
	MkBuildNode node;
	mk_uint32_t i;

	bldno->flags |= kMkBldNo_Built_Bit;

	for( i = 0; i < bldno->outputs.num; ++i ) {
		node = bldno->outputs.nodes[ i ];

		if( mk_async_atomicDec_post( &node->async.inputsRemaining ) == 0 ) {
			// FIXME: !!! APPEND TO READY QUEUE !!!
		}
	}
}
static void bldno_syncFailure_r( MkBuildNode bldno ) {
	MkBuildNode node;
	mk_uint32_t i;

	if( ( bldno->flags & kMkBldNo_Unbuildable_Bit ) != 0 ) {
		bldno->flags |= kMkBldNo_Failed_Bit;
	}

	for( i = 0; i < bldno->outputs.num; ++i ) {
		node = bldno->outputs.nodes[ i ];

		node->flags |= kMkBldNo_Unbuildable_Bit;
		bldno_syncFailure_r( node );
	}
}


/*

	BUILD CONTEXT

*/

struct MkBuildContext_s {
	nodeArray_t targets;

	struct {
		nodeArray_t array;

		mk_uint32_t count;
		mk_uint32_t index;

		mk_semaphore_t waiter;
		volatile int   cancel;
	} readyQueue;

	/* FIXME: Add `trace` fields here */

	struct {
		allocPage_t *head;

		allocPage_t base;
	} memory;
};

static void bldctx_init_queue( MkBuildContext ctx, MkBuildNode node ) {
	mk_uint32_t n;

	n = ctx->readyQueue.count;
	MK_ASSERT( n < ctx->readyQueue.array.num );

	ctx->readyQueue.array.nodes[ n ]  = node;
	ctx->readyQueue.count            += 1;

	node->ctx = ctx;
}
static void bldctx_init_queueIfReady_r( MkBuildContext ctx, MkBuildNode node ) {
	mk_uint32_t i;

	if( node->inputs.num == 0 ) {
		bldctx_init_queue( ctx, node );
		return;
	}

	for( i = 0; i < node->inputs.num; ++i ) {
		bldctx_init_queueIfReady_r( ctx, node->inputs.nodes[ i ] );
	}
}
MK_UNUSED
static void bldctx_init_allocReadyQueue( MkBuildContext ctx ) {
	mk_uint32_t i, n;

	n = 0;
	for( i = 0; i < ctx->targets.num; ++i ) {
		n += bldno_countInputs( ctx->targets.nodes[ i ] );
	}

	na_init( &ctx->readyQueue.array );
	na_resize( &ctx->readyQueue.array, n );

	ctx->readyQueue.count = 0;
	ctx->readyQueue.index = 0;
}
MK_UNUSED
static void bldctx_init_fillReadyQueue( MkBuildContext ctx ) {
	mk_uint32_t i;

	for( i = 0; i < ctx->targets.num; ++i ) {
		bldctx_init_queueIfReady_r( ctx, ctx->targets.nodes[ i ] );
	}
}
static int bldctx_didAllTargetsFail( MkBuildContext ctx ) {
	mk_uint32_t i;

	for( i = 0; i < ctx->targets.num; ++i ) {
		mk_uint32_t flags;

		flags = ctx->targets.nodes[ i ]->flags;
		if( ( flags & kMkBldNo_Canceled_Bits ) == 0 ) {
			return 0;
		}
	}

	return 1;
}
enum { NUM_WORKER_THREAD_SEMAPHORE_NOTIFICATIONS = 128UL };
static void bldctx_cancel( MkBuildContext ctx ) {
	mk_uint32_t n;

	ctx->readyQueue.cancel = 1;

	// let every worker know about the cancellation
	for( n = NUM_WORKER_THREAD_SEMAPHORE_NOTIFICATIONS; n > 0; --n ) {
		(void)mk_async_semRaise( &ctx->readyQueue.waiter );
	}
}

MK_UNUSED
static int build_thread_f( mk_thread_t *thread, void *userdata ) {
	MkBuildContext ctx;
	MkBuildNode node;
	mk_uint32_t index;
	int r;
	int canceled;

	((void)thread);
	ctx = (MkBuildContext)userdata;

	canceled = 0;
	for(;;) {
		if( !mk_async_semWait( &ctx->readyQueue.waiter ) ) {
			break;
		}

		if( ctx->readyQueue.cancel != 0 ) {
			canceled = 1;
			break;
		}

		index = mk_async_atomicInc_pre( &ctx->readyQueue.index );
		MK_ASSERT_MSG( index < ctx->readyQueue.count, "Index is out of sync with job queue" );

		if( !( node = (MkBuildNode)mk_async_atomicSetPtr_pre( &ctx->readyQueue.array.nodes[ index ], NULL ) ) ) {
			continue;
		}

		r = 1;
		if( node->pfn_build != NULL ) {
			r =
				node->pfn_build(
					node, node->userData,
					/* inputs=*/(MkStrList)0,
					/*outputs=*/(MkStrList)0
				);
		}

		if( r != 0 && ( node->flags & kMkBldNo_Phony_Bit ) == 0 ) {
			if( !mk_fs_isFile( node->filename ) ) {
				mk_log_error( node->filename, 0, NULL, "File not generated after otherwise successful build step invocation." );
				r = 0;
			}
		}

		if( r != 0 ) {
			bldno_syncCompletion( node );
		} else {
			bldno_syncFailure_r( node );
			if( bldctx_didAllTargetsFail( ctx ) ) {
				bldctx_cancel( ctx );
				canceled = 1;
				break;
			}
		}
	}

	return canceled == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
