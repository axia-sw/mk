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
 *	ASYNC / MULTI-THREADING CODE
 *	========================================================================
 *	Implements asynchronous primitives and functions.
 */

#include "mk-defs-platform.h"

#include "ext/ax_thread.h"

typedef axth_u32_t mk_uint32_t;

typedef axthread_t    mk_thread_t;
typedef axth_qmutex_t mk_mutex_t;
typedef axth_sem_t    mk_semaphore_t;

#define MK_THREAD_INITIALIZER    AXTHREAD_INITIALIZER
#define MK_MUTEX_INITIALIZER     AXTHREAD_QMUTEX_INITIALIZER
#define MK_SEMAPHORE_INITIALIZER AXTHREAD_SEM_INITIALIZER

typedef int( *mk_thread_func_t )( mk_thread_t *, void * );

mk_thread_t *mk_async_threadInit( mk_thread_t *thread, const char *name, mk_thread_func_t fn, void *arg );
void         mk_async_threadFini( mk_thread_t * );
void         mk_async_threadRequestQuit( mk_thread_t * );
int          mk_async_threadIsQuitRequested( const mk_thread_t * );
int          mk_async_threadIsRunning( const mk_thread_t * );
int          mk_async_threadGetExitCode( const mk_thread_t * );

/*

	ATOMIC OPERATIONS
	-----------------

	The mk_async_atomic*_(pre|post) functions perform a given operation
	as one synchronized (atomic) operation. The "pre" functions return the
	value of the destination memory as it was just prior to the operation. The
	"post" functions return the value of the destination memory as it was just
	after the operation.

	These are desirable as multithreaded programs risk concurrent accesses to
	the same memory resulting in race conditions and incorrect evaluation of
	algorithms due to this.

*/

mk_uint32_t mk_async_atomicInc_pre( volatile mk_uint32_t *dst );
mk_uint32_t mk_async_atomicDec_post( volatile mk_uint32_t *dst );
void *      mk_async_atomicSetPtr_pre( volatile void *dst, void *src );
void *      mk_async_atomicCmpSetPtr_post( volatile void *dst, void *src, void *cmp );

/*

	MUTEXES
	-------

	A mutex is a synchronization primitive used to control which threads have
	access to some resource at a given point in time. Mutexes specify a "mutual
	exclusion" access to a resource, so only one mutex holder can have access to
	the resource it's guarding at a given time. i.e., "locking" the resource.

	To gain access to a resource, use mk_async_mtxLock(). This forces the
	calling thread to wait until other mutexes have unlocked the resource. The
	calling thread then locks the resource until it is released.

	When done accessing a resource, use mk_async_mtxUnlock() to return control
	to other threads that may be waiting to access said resource.

	ALWAYS call mk_async_mtxUnlock() following a call to mk_async_mtxLock().
	Failing to unlock a resource after it's been locked will cause other threads
	to wait forever, hanging the program. Be especially mindful of early exits
	in functions where a lock has been acquired.

	Reduce the amount of time you access a resource within a lock as much as
	possible. If work can be done without necessarily having access to that
	resource right away, then try to do that work before locking the resource.

*/

mk_mutex_t *mk_async_mtxInit( mk_mutex_t *mtx );
mk_mutex_t *mk_async_mtxFini( mk_mutex_t *mtx );
void        mk_async_mtxLock( mk_mutex_t *mtx );
void        mk_async_mtxUnlock( mk_mutex_t *mtx );

/*

	SEMAPHORES
	----------

	A semaphore is a synchronization primitive used to control which threads
	have access to some resource at a given point in time. Semaphores retain an
	internal "count" that can either be incremented or decremented.

	To increment the count, mk_async_semRaise() can be used. This signals that
	a resource has been made available.

	To access such a resource, the count must be decremented. This can be done
	with mk_async_semWait(). If the count is 0, then the function will not
	return until the count reaches 0 or the semaphore was destroyed (via
	mk_async_semFini()).

	When initializing a semaphore, via mk_async_semInit(), a base count can be
	given to indicate some starting number of resources available.

*/

mk_semaphore_t *mk_async_semInit( mk_semaphore_t *sem, mk_uint32_t base );
mk_semaphore_t *mk_async_semFini( mk_semaphore_t *sem );
int             mk_async_semRaise( mk_semaphore_t *sem );
int             mk_async_semWait( mk_semaphore_t *sem );
