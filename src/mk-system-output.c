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
#include "mk-system-output.h"

#include "mk-basic-assert.h"
#include "mk-basic-debug.h"
#include "mk-basic-options.h"
#include "mk-basic-types.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"
#include "mk-frontend.h"

#include <stdio.h>
#include <string.h>

#if MK_WINDOWS_ENABLED
HANDLE mk__g_sioh[kMkNumSIO];
#endif
FILE *mk__g_siof[kMkNumSIO];

int mk_sys_isColoredOutputEnabled( void ) {
	return mk__g_flags_color != kMkColorMode_None;
}

void mk_sys_initColoredOutput( void ) {
#if MK_WINDOWS_ENABLED
	mk__g_sioh[kMkSIO_Out] = GetStdHandle( STD_OUTPUT_HANDLE );
	mk__g_sioh[kMkSIO_Err] = GetStdHandle( STD_ERROR_HANDLE );
#endif

	/*
	 * TODO: Make sure that colored output is possible on this terminal
	 */

	mk__g_siof[kMkSIO_Out] = stdout;
	mk__g_siof[kMkSIO_Err] = stderr;
}

unsigned char mk_sys_getCurrColor( MkSIO_t sio ) {
#if MK_WINDOWS_ENABLED
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if( mk__g_sioh[sio] != INVALID_HANDLE_VALUE ) {
		if( mk__g_sioh[sio] && GetConsoleScreenBufferInfo( mk__g_sioh[sio], &csbi ) ) {
			return csbi.wAttributes & 0xFF;
		}
	}
#else
	/*
	 * TODO: Support this on GNU/Linux...
	 */
	(void)sio;
#endif

	return 0x07;
}
void mk_sys_setCurrColor( MkSIO_t sio, unsigned char color ) {
	/*
	 * MAP ORDER:
	 * Black, Blue, Green, Cyan, Red, Magenta, Yellow, Grey
	 *
	 * TERMINAL COLOR ORDER:
	 * Black=0, Red=1, Green=2, Yellow=3, Blue=4, Magenta=5, Cyan=6, Grey=7
	 */
	static const char *const mapF[16] = {
		"\x1b[30;22m", "\x1b[34;22m", "\x1b[32;22m", "\x1b[36;22m",
		"\x1b[31;22m", "\x1b[35;22m", "\x1b[33;22m", "\x1b[37;22m",
		"\x1b[30;1m", "\x1b[34;1m", "\x1b[32;1m", "\x1b[36;1m",
		"\x1b[31;1m", "\x1b[35;1m", "\x1b[33;1m", "\x1b[37;1m"
	};
#if 0
	static const char *const mapB[16] = {
		"\x1b[40m", "\x1b[41m", "\x1b[42m", "\x1b[43m",
		"\x1b[44m", "\x1b[45m", "\x1b[46m", "\x1b[47m",
		"\x1b[40;1m", "\x1b[41;1m", "\x1b[42;1m", "\x1b[43;1m",
		"\x1b[44;1m", "\x1b[45;1m", "\x1b[46;1m", "\x1b[47;1m"
	};
#endif

	switch( mk_opt_getColorMode() ) {
	case kMkColorMode_None:
		break;

	case kMkColorMode_ANSI:
		fwrite( mapF[color & 0x0F], strlen( mapF[color & 0x0F] ), 1, mk__g_siof[sio] );
		break;

#if MK_WINDOWS_COLORS_ENABLED
	case kMkColorMode_Windows:
		if( mk__g_sioh[sio] != INVALID_HANDLE_VALUE ) {
			SetConsoleTextAttribute( mk__g_sioh[sio], (WORD)color );
		}
		break;
#endif

	case kMkNumColorModes:
		MK_ASSERT_MSG( 0, "Invalid color mode!" );
		break;
	}
}
void mk_sys_uncoloredPuts( MkSIO_t sio, const char *text, size_t len ) {
	if( !len ) {
		len = strlen( text );
	}

	mk_dbg_outf( "%.*s", len, text );

#if MK_WINDOWS_ENABLED && 0
	if( mk__g_sioh[sio] != INVALID_HANDLE_VALUE ) {
		WriteFile( mk__g_sioh[sio], text, len, NULL, NULL );
		return;
	}
#endif

	fwrite( text, len, 1, mk__g_siof[sio] );
}
int mk_sys_charToColorCode( char c ) {
	if( c >= '0' && c <= '9' ) {
		return 0 + (int)( c - '0' );
	}

	if( c >= 'A' && c <= 'F' ) {
		return 10 + (int)( c - 'A' );
	}

	if( c >= 'a' && c <= 'f' ) {
		return 10 + (int)( c - 'a' );
	}

	return -1;
}
void mk_sys_puts( MkSIO_t sio, const char *text ) {
	unsigned char prevColor;
	const char *s, *e;
	int color;

	/* set due to potential bad input pulling garbage color */
	prevColor = mk_sys_getCurrColor( sio );

	s = text;
	while( 1 ) {
		/* write normal characters in one chunk */
		if( *s != '^' ) {
			e = strchr( s, '^' );
			if( !e ) {
				e = strchr( s, '\0' );
			}

			mk_sys_uncoloredPuts( sio, s, e - s );
			if( *e == '\0' ) {
				break;
			}

			s = e;
			continue;
		}

		/* must be a special character, treat it as such */
		color = mk_sys_charToColorCode( *++s );
		if( color != -1 ) {
			prevColor = mk_sys_getCurrColor( sio );
			mk_sys_setCurrColor( sio, (unsigned char)color );
		} else if( *s == '&' ) {
			color     = (int)prevColor;
			prevColor = mk_sys_getCurrColor( sio );
			mk_sys_setCurrColor( sio, (unsigned char)color );
		} else if( *s == '^' ) {
			mk_sys_uncoloredPuts( sio, "^", 1 );
		}

		s++;
	}
}
void mk_sys_printf( MkSIO_t sio, const char *format, ... ) {
	static char buf[32768];
	va_list args;

	va_start( args, format );
#if MK_SECLIB
	vsprintf_s( buf, sizeof( buf ), format, args );
#else
	vsnprintf( buf, sizeof( buf ), format, args );
	buf[sizeof( buf ) - 1] = '\0';
#endif
	va_end( args );

	mk_sys_puts( sio, buf );
}

void mk_sys_printStr( MkSIO_t sio, unsigned char color, const char *str ) {
	unsigned char curColor;

	MK_ASSERT( str != (const char *)0 );

	curColor = mk_sys_getCurrColor( sio );
	mk_sys_setCurrColor( sio, color );
	mk_sys_uncoloredPuts( sio, str, 0 );
	mk_sys_setCurrColor( sio, curColor );
}
void mk_sys_printUint( MkSIO_t sio, unsigned char color, unsigned int val ) {
	char buf[64];

#if MK_SECLIB
	sprintf_s( buf, sizeof( buf ), "%u", val );
#else
	snprintf( buf, sizeof( buf ), "%u", val );
	buf[sizeof( buf ) - 1] = '\0';
#endif

	mk_sys_printStr( sio, color, buf );
}
void mk_sys_printInt( MkSIO_t sio, unsigned char color, int val ) {
	char buf[64];

#if MK_SECLIB
	sprintf_s( buf, sizeof( buf ), "%i", val );
#else
	snprintf( buf, sizeof( buf ), "%i", val );
	buf[sizeof( buf ) - 1] = '\0';
#endif

	mk_sys_printStr( sio, color, buf );
}
