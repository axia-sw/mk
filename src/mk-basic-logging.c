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
#include "mk-basic-logging.h"

#include "mk-basic-debug.h"
#include "mk-system-output.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* display a simple mk_log_error message */
void mk_log_errorMsg( const char *message ) {
	unsigned char curColor;

	curColor = mk_sys_getCurrColor( kMkSIO_Err );
	mk_sys_printf( kMkSIO_Err, MK_S_COLOR_RED "ERROR" MK_S_COLOR_RESTORE ": %s", message );
	mk_sys_setCurrColor( kMkSIO_Err, curColor );
	if( errno ) {
		mk_sys_printf( kMkSIO_Err, ": %s [" MK_S_COLOR_MAGENTA "%d" MK_S_COLOR_RESTORE "]", strerror( errno ), errno );
	}
	mk_sys_uncoloredPuts( kMkSIO_Err, "\n", 1 );

	mk_dbg_backtrace();
}

/* exit with an mk_log_error message; if errno is set, display its mk_log_error */
MK_NORETURN void mk_log_fatalError( const char *message ) {
	mk_log_errorMsg( message );
	exit( EXIT_FAILURE );
}

/* provide a general purpose mk_log_error, without terminating */
void mk_log_error( const char *file, unsigned int line, const char *func,
    const char *message ) {
	if( file ) {
		mk_sys_printStr( kMkSIO_Err, MK_COLOR_WHITE, file );
		mk_sys_uncoloredPuts( kMkSIO_Err, ":", 1 );
		if( line ) {
			mk_sys_printUint( kMkSIO_Err, MK_COLOR_BROWN, line );
			mk_sys_uncoloredPuts( kMkSIO_Err, ":", 1 );
		}
		mk_sys_uncoloredPuts( kMkSIO_Err, " ", 1 );
	}

	mk_sys_puts( kMkSIO_Err, MK_S_COLOR_RED "ERROR" MK_S_COLOR_RESTORE ": " );

	if( func ) {
		unsigned char curColor;

		curColor = mk_sys_getCurrColor( kMkSIO_Err );
		mk_sys_puts( kMkSIO_Err, "in " MK_S_COLOR_GREEN );
		mk_sys_uncoloredPuts( kMkSIO_Err, func, 0 );
		mk_sys_setCurrColor( kMkSIO_Err, curColor );
		mk_sys_uncoloredPuts( kMkSIO_Err, ": ", 2 );
	}

	if( message ) {
		mk_sys_uncoloredPuts( kMkSIO_Err, message, 0 );
	}

	if( ( errno && message ) || !message ) {
		if( message ) {
			mk_sys_uncoloredPuts( kMkSIO_Err, ": ", 2 );
		}
		mk_sys_uncoloredPuts( kMkSIO_Err, strerror( errno ), 0 );
		mk_sys_printf( kMkSIO_Err, "[" MK_S_COLOR_MAGENTA "%d" MK_S_COLOR_RESTORE "]",
		    errno );
	}

	mk_sys_uncoloredPuts( kMkSIO_Err, "\n", 1 );
	mk_dbg_backtrace();
}

/* provide an mk_log_error for a failed MK_ASSERT */
void mk_log_errorAssert( const char *file, unsigned int line, const char *func,
    const char *message ) {
	mk_log_error( file, line, func, message );
#if MK_WINDOWS_ENABLED
	fflush( stdout );
	fflush( stderr );

	if( IsDebuggerPresent() ) {
#	if MK_VCVER
		__debugbreak();
#	elif defined( __GNUC__ ) || defined( __clang__ )
		__builtin_trap();
#	else
		DebugBreak();
#	endif
	}
#endif
	/*exit(EXIT_FAILURE);*/
	abort();
}
