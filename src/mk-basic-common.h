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
 *	UTILITY CODE
 *	========================================================================
 *	A set of utility functions for making the rest of this easier.
 */

#include "mk-defs-platform.h"

#include <stddef.h>

#ifndef MK_VA_MINSIZE
#	define MK_VA_MINSIZE 1024
#endif

#define mk_com_memory( p_, n_ ) mk_com__memory( (void *)( p_ ), ( size_t )( n_ ), __FILE__, __LINE__, MK_CURFUNC )
#define mk_com_strdup( cstr_ ) mk_com__strdup( ( cstr_ ), __FILE__, __LINE__, MK_CURFUNC )

void *      mk_com__memory( void *p, size_t n, const char *pszFile, unsigned int uLine, const char *pszFunction );
const char *mk_com_va( const char *format, ... );
size_t      mk_com_strlen( const char *src );
size_t      mk_com_strcat( char *buf, size_t bufn, const char *src );
size_t      mk_com_strncat( char *buf, size_t bufn, const char *src, size_t srcn );
void        mk_com_strcpy( char *dst, size_t dstn, const char *src );
void        mk_com_strncpy( char *buf, size_t bufn, const char *src, size_t srcn );
char *      mk_com__strdup( const char *cstr, const char *file, unsigned int line, const char *func );

char *      mk_com_dup( char *dst, const char *src );
char *      mk_com_dupn( char *dst, const char *src, size_t numchars );
char *      mk_com_append( char *dst, const char *src );
const char *mk_com_extractDir( char *buf, size_t n, const char *filename );
void        mk_com_substExt( char *dst, size_t dstn, const char *src, const char *ext );
int         mk_com_shellf( const char *format, ... );
int         mk_com_matchPath( const char *rpath, const char *apath );
int         mk_com_getIntDate( void );

const char *mk_com_findArgEnd( const char *arg );
int         mk_com_matchArg( const char *a, const char *b );
const char *mk_com_skipArg( const char *arg );
void        mk_com_stripArgs( char *dst, size_t n, const char *src );
int         mk_com_cmpPathChar( char a, char b );

int mk_com_relPath( char *dst, size_t dstn, const char *curpath, const char *abspath );
int mk_com_relPathCWD( char *dst, size_t dstn, const char *abspath );

const char *mk_com_getenv( char *dst, size_t dstn, const char *var );
int         mk_com_strstarts( const char *src, const char *with );
int         mk_com_strends( const char *src, const char *with );

void mk_com_fixpath( char *path );
