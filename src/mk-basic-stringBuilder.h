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
 *	STRING BUILDER
 *	========================================================================
 *	Used to more easily construct large strings from code.
 *
 *	Example
 *	-------
 *	MkStringBuilder sb;
 *	char *builtString = NULL;
 *	MkVariable varInputFile = ...; // Set this to the relevant variable
 *
 *	// Initialize the buffer
 *	mk_sb_init( &sb, 0 );
 *
 *	// Format it in some way
 *	mk_sb_pushVar( mk_sb_pushStr( &sb, "InputFile = " ), varInputFile );
 *
 *	// Finalize its construction
 *	builtString = mk_sb_done( &sb );
 *
 *	// Do something with the built string
 *	printf( "%s\n", builtString );
 *
 *	// Free the string now that we're done with it
 *	builtString = (char*)mk_com_memory( builtString, 0 ); // free the string
 */

#include <stddef.h>

#include "mk-basic-variable.h"

typedef struct MkStringBuilder_s {
	char *buffer;
	size_t len, capacity;
} MkStringBuilder;

MkStringBuilder *mk_sb_init( MkStringBuilder *sb, size_t initCapacity );
char *           mk_sb_done( MkStringBuilder *sb );

MkStringBuilder *mk_sb_clear( MkStringBuilder *sb );

MkStringBuilder *mk_sb_pushSubstr( MkStringBuilder *sb, const char *s, const char *e );
MkStringBuilder *mk_sb_pushStr( MkStringBuilder *sb, const char *s );
MkStringBuilder *mk_sb_pushChar( MkStringBuilder *sb, char c );
MkStringBuilder *mk_sb_pushVar( MkStringBuilder *sb, MkVariable v );

/*
 *	Quote and Unquote
 *	-----------------
 *	The "quoteAndPush" functions look at the input string and determine whether
 *	any spaces or quotes are present. If there are spaces or quotes present in
 *	the string then quotes and backslashes are escaped with a prefix backslash,
 *	and the string is enclosed in quotes.
 *
 *	For example:
 *		Input : hello "world" from C:\ drive
 *		Output: "hello \"world\" from C:\\ drive"
 *
 *	The "unquoteAndPush" functions do the reverse. They first check to see if
 *	the string is enclosed in quotes, and if so removes the enclosing quotes and
 *	unescapes the string. (Only \\ and \" are unescaped.)
 *
 *	For example:
 *		Input : "hello \"world\" from C:\\ drive"
 *		Output: hello "world" from C:\ drive
 */

MkStringBuilder *mk_sb_quoteAndPushSubstr( MkStringBuilder *sb, const char *s, const char *e );
MkStringBuilder *mk_sb_quoteAndPushStr( MkStringBuilder *sb, const char *s );
MkStringBuilder *mk_sb_unquoteAndPushSubstr( MkStringBuilder *sb, const char *s, const char *e );
MkStringBuilder *mk_sb_unquoteAndPushStr( MkStringBuilder *sb, const char *s );
