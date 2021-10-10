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
#define AXTHREAD_IMPLEMENTATION
#include "mk-basic-async.h"

#include "mk-basic-assert.h"

mk_thread_t *mk_async_threadInit( mk_thread_t *thread, const char *name, mk_thread_func_t fn, void *arg ) {
	return (mk_thread_t*)axthread_init_named( (axthread_t*)thread, name, (axth_fn_thread_t)fn, arg );
}
void mk_async_threadFini( mk_thread_t *thread ) {
	(void)axthread_fini( (axthread_t*)thread );
}
void mk_async_threadRequestQuit( mk_thread_t *thread ) {
	axthread_signal_quit( (axthread_t*)thread );
}
int mk_async_threadIsQuitRequested( const mk_thread_t *thread ) {
	return axthread_is_quitting( (const axthread_t *)thread );
}
int mk_async_threadIsRunning( const mk_thread_t *thread ) {
	return axthread_is_running( (const axthread_t *)thread );
}
int mk_async_threadGetExitCode( const mk_thread_t *thread ) {
	MK_ASSERT_MSG( !mk_async_threadIsRunning(thread), "Cannot get exit code of thread that is still running" );
	return axthread_get_exit_code( (const axthread_t *)thread );
}

mk_uint32_t mk_async_atomicInc_pre( volatile mk_uint32_t *dst ) {
	return AX_ATOMIC_FETCH_ADD_FULL32( dst, 1 );
}
mk_uint32_t mk_async_atomicDec_post( volatile mk_uint32_t *dst ) {
	return AX_ATOMIC_FETCH_SUB_FULL32( dst, 1 ) - 1;
}

mk_semaphore_t *mk_async_semInit( mk_semaphore_t *sem, mk_uint32_t base ) {
	return (mk_semaphore_t*)axth_sem_init( (axth_sem_t*)sem, base );
}
mk_semaphore_t *mk_async_semFini( mk_semaphore_t *sem ) {
	return (mk_semaphore_t*)axth_sem_fini( (axth_sem_t*)sem );
}
int mk_async_semRaise( mk_semaphore_t *sem ) {
	return axth_sem_signal( (axth_sem_t*)sem );
}
int mk_async_semWait( mk_semaphore_t *sem ) {
	return axth_sem_wait( (axth_sem_t*)sem );
}

