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
 *	COLORED PRINTING CODE
 *	========================================================================
 *	Display colored output.
 */

#include "mk-basic-types.h"

#include <stddef.h>
#include <stdio.h>

typedef enum {
	kMkSIO_Out,
	kMkSIO_Err,

	kMkNumSIO
} MkSIO_t;

#if MK_WINDOWS_ENABLED
extern HANDLE mk__g_sioh[kMkNumSIO];
#endif
extern FILE *mk__g_siof[kMkNumSIO];

#define MK_S_COLOR_BLACK "^0"
#define MK_S_COLOR_BLUE "^1"
#define MK_S_COLOR_GREEN "^2"
#define MK_S_COLOR_CYAN "^3"
#define MK_S_COLOR_RED "^4"
#define MK_S_COLOR_MAGENTA "^5"
#define MK_S_COLOR_BROWN "^6"
#define MK_S_COLOR_LIGHT_GREY "^7"
#define MK_S_COLOR_DARK_GREY "^8"
#define MK_S_COLOR_LIGHT_BLUE "^9"
#define MK_S_COLOR_LIGHT_GREEN "^A"
#define MK_S_COLOR_LIGHT_CYAN "^B"
#define MK_S_COLOR_LIGHT_RED "^C"
#define MK_S_COLOR_LIGHT_MAGENTA "^D"
#define MK_S_COLOR_LIGHT_BROWN "^E"
#define MK_S_COLOR_WHITE "^F"

#define MK_S_COLOR_PURPLE "^5"
#define MK_S_COLOR_YELLOW "^E"
#define MK_S_COLOR_VIOLET "^D"

#define MK_S_COLOR_RESTORE "^&"

#define MK_COLOR_BLACK 0x0
#define MK_COLOR_BLUE 0x1
#define MK_COLOR_GREEN 0x2
#define MK_COLOR_CYAN 0x3
#define MK_COLOR_RED 0x4
#define MK_COLOR_MAGENTA 0x5
#define MK_COLOR_BROWN 0x6
#define MK_COLOR_LIGHT_GREY 0x7
#define MK_COLOR_DARK_GREY 0x8
#define MK_COLOR_LIGHT_BLUE 0x9
#define MK_COLOR_LIGHT_GREEN 0xA
#define MK_COLOR_LIGHT_CYAN 0xB
#define MK_COLOR_LIGHT_RED 0xC
#define MK_COLOR_LIGHT_MAGENTA 0xD
#define MK_COLOR_LIGHT_BROWN 0xE
#define MK_COLOR_WHITE 0xF

#define MK_COLOR_PURPLE 0x5
#define MK_COLOR_YELLOW 0xE
#define MK_COLOR_VIOLET 0xD

int mk_sys_isColoredOutputEnabled( void );
void mk_sys_initColoredOutput( void );

unsigned char mk_sys_getCurrColor( MkSIO_t sio );
void mk_sys_setCurrColor( MkSIO_t sio, unsigned char color );
void mk_sys_uncoloredPuts( MkSIO_t sio, const char *text, size_t len );
int mk_sys_charToColorCode( char c );
void mk_sys_puts( MkSIO_t sio, const char *text );
void mk_sys_printf( MkSIO_t sio, const char *format, ... );

void mk_sys_printStr( MkSIO_t sio, unsigned char color, const char *str );
void mk_sys_printUint( MkSIO_t sio, unsigned char color, unsigned int val );
void mk_sys_printInt( MkSIO_t sio, unsigned char color, int val );
