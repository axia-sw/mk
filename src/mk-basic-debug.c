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
#include "mk-basic-debug.h"

#include "mk-basic-assert.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-options.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"
#include "mk-version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if MK_HAS_EXECINFO
#	include <execinfo.h>
#endif

#if MK_DEBUG_ENABLED
FILE *mk__g_pDebugLog = (FILE *)0;
#endif
unsigned mk__g_cDebugIndents = 0;

#if MK_DEBUG_ENABLED
static void mk_dbg__closeLog_f( void ) {
	if( !mk__g_pDebugLog ) {
		return;
	}

	fprintf( mk__g_pDebugLog, "\n\n[[== DEBUG LOG CLOSED ==]]\n\n" );

	fclose( mk__g_pDebugLog );
	mk__g_pDebugLog = (FILE *)0;
}
#endif

/* write to the debug log, no formatting */
void mk_dbg_out( const char *str ) {
#if MK_DEBUG_ENABLED
	static const char szTabs[]  = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	static const unsigned cTabs = sizeof( szTabs ) - 1;
	static int didWriteNewline = 1;

	const char *pstr;
	size_t len;

	unsigned cIndents;

	if( !mk__g_pDebugLog ) {
		time_t rawtime;
		struct tm *timeinfo;
		char szTimeBuf[128];

		mk__g_pDebugLog = fopen( mk_opt_getDebugLogPath(), "a+" );
		if( !mk__g_pDebugLog ) {
			return;
		}

		szTimeBuf[0] = '\0';

		time( &rawtime );
		if( ( timeinfo = localtime( &rawtime ) ) != (struct tm *)0 ) {
			strftime( szTimeBuf, sizeof( szTimeBuf ), " (%Y-%m-%d %H:%M:%S)", timeinfo );
		}

		fprintf( mk__g_pDebugLog, "\n\n[[== DEBUG LOG OPENED%s ==]]\n\n", szTimeBuf );
		fprintf( mk__g_pDebugLog, "#### Mk version: " MK_VERSION_STR " ****\n\n" );

		atexit( mk_dbg__closeLog_f );
	}

	pstr = str;
	do {
		/* write leading indentation (if any) */
		cIndents = mk__g_cDebugIndents;
		while( cIndents > 0 && didWriteNewline ) {
			unsigned cIndentsToWrite;

			cIndentsToWrite = cIndents < cTabs ? cIndents : cTabs;
			fwrite( &szTabs[0], (size_t)cIndentsToWrite, 1, mk__g_pDebugLog );

			cIndents -= cIndentsToWrite;
		}

		/* find the end of the current line to write */
		const char *nstr = strchr( pstr, '\n' );
		if( nstr != (const char *)0 ) {
			didWriteNewline = 1;
			++nstr;
		} else {
			didWriteNewline = 0;
			nstr = strchr( pstr, '\0' );
		}

		/* write the actual line */
		len = (size_t)(ptrdiff_t)( nstr - pstr );
		fwrite( pstr, len, 1, mk__g_pDebugLog );

		/* set next search point */
		pstr = nstr;
	} while( *pstr != '\0' );

	/* ensure all debug data is written in case we crash right after this */
	fflush( mk__g_pDebugLog );
#else
	(void)str;
#endif
}

/* write to the debug log (va_args) */
void mk_dbg_outfv( const char *format, va_list args ) {
#if MK_DEBUG_ENABLED
	static char buf[65536];

#	if MK_SECLIB
	vsprintf_s( buf, sizeof( buf ), format, args );
#	else
	vsnprintf( buf, sizeof( buf ), format, args );
	buf[sizeof( buf ) - 1] = '\0';
#	endif

	mk_dbg_out( buf );
#else
	(void)format;
	(void)args;
#endif
}
/* write to the debug log */
void mk_dbg_outf( const char *format, ... ) {
	va_list args;

	va_start( args, format );
	mk_dbg_outfv( format, args );
	va_end( args );
}

/* enter a debug layer */
void mk_dbg_enter( const char *format, ... ) {
	va_list args;

	mk_dbg_out( "\n" );
	va_start( args, format );
	mk_dbg_outfv( format, args );
	mk_dbg_out( "\n" );
	mk_dbg_out( "{\n" );
	va_end( args );

	++mk__g_cDebugIndents;
}
/* leave a debug layer */
void mk_dbg_leave( void ) {
	MK_ASSERT( mk__g_cDebugIndents > 0 );

	if( mk__g_cDebugIndents > 0 ) {
		--mk__g_cDebugIndents;
	}

	mk_dbg_out( "}\n\n" );
}

/* backtrace support... */
void mk_dbg_backtrace( void ) {
#if MK_HAS_EXECINFO
	int i, n;
	void *buffer[PATH_MAX];
	char **strings;

	n = backtrace( buffer, sizeof( buffer ) / sizeof( buffer[0] ) );
	if( n < 1 ) {
		return;
	}

	strings = backtrace_symbols( buffer, n );
	if( !strings ) {
		return;
	}

	mk_dbg_enter( "BACKTRACE(%i)", n );
	for( i = 0; i < n; i++ ) {
		mk_dbg_outf( "%s\n", strings[i] );
	}
	mk_dbg_leave();

	free( (void *)strings );
#endif
}
