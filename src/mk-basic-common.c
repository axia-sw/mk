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
#include "mk-basic-common.h"

#include "mk-basic-assert.h"
#include "mk-basic-debug.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-logging.h"
#include "mk-basic-memory.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"
#include "mk-frontend.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
================
mk_com__memory

manage memory; never worry about errors
================
*/
void *mk_com__memory( void *p, size_t n, const char *pszFile, unsigned int uLine, const char *pszFunction ) {
	errno = 0;

	if( n > 0 ) {
		void *q;

		q = mk_mem__alloc( n, 0, pszFile, uLine, pszFunction );

		if( p != NULL ) {
			size_t c;
#if MK_DEBUG_ENABLED
			const struct MkMem__Hdr_s *pHdr;

			pHdr = (const struct MkMem__Hdr_s *)p - 1;
			MK_ASSERT_MSG( pHdr->cRefs == 1, "Cannot reallocate object: there are multiple references" );
			MK_ASSERT_MSG( pHdr->pHead == NULL, "Cannot reallocate object: it has associated objects" );
			MK_ASSERT_MSG( pHdr->pPrnt == NULL, "Cannot reallocate object: it is owned by another object" );
#endif

			c = mk_mem__size( p );
			memcpy( q, p, c < n ? c : n );
			mk_mem__dealloc( p, pszFile, uLine, pszFunction );
		}

		return q;
	}

	mk_mem__dealloc( p, pszFile, uLine, pszFunction );
	return NULL;
}

/*
================
mk_com_va

provide printf-style formatting on the fly
================
*/
const char *mk_com_va( const char *format, ... ) {
	static char buf[65536];
	static size_t index = 0;
	va_list args;
	size_t n;
	char *p;

	p = &buf[index];

	va_start( args, format );
#if MK_SECLIB
	n = vsprintf_s( &buf[index], sizeof( buf ) - index, format, args );
#else
	n                      = vsnprintf( &buf[index], sizeof( buf ) - index, format, args );
	buf[sizeof( buf ) - 1] = '\0';
#endif
	va_end( args );

	index += n;
	if( index + MK_VA_MINSIZE > sizeof( buf ) ) {
		index = 0;
	}

	return p;
}

/* secure strlen */
size_t mk_com_strlen( const char *src ) {
	if( src != (const char *)0 ) {
		return strlen( src );
	}

	return 0;
}

/* secure strcat */
size_t mk_com_strcat( char *buf, size_t bufn, const char *src ) {
	size_t index, len;

	index = mk_com_strlen( buf );
	len   = mk_com_strlen( src );

	if( index + len >= bufn ) {
		mk_dbg_enter( "mk_com_strcat!buffer-overflow" );
		mk_dbg_outf( "buflen: %u; srclen: %u\n", (unsigned int)index, (unsigned int)len );
		mk_dbg_outf( "buf@%p: %.*s\n", buf, (int)(ptrdiff_t)index, buf );
		mk_dbg_outf( "src@%p: %.*s\n", src, (int)(ptrdiff_t)len, src );
		mk_dbg_leave();
		mk_log_fatalError( "strcat: detected overflow" );
	}

	memcpy( (void *)&buf[index], (const void *)src, len + 1 );
	return index + len;
}

/* secure strncat */
size_t mk_com_strncat( char *buf, size_t bufn, const char *src, size_t srcn ) {
	size_t index, len;

	index = mk_com_strlen( buf );
	len   = srcn ? srcn : mk_com_strlen( src );

	if( index + len >= bufn ) {
		mk_dbg_enter( "mk_com_strncat!buffer-overflow" );
		mk_dbg_outf( "buflen: %u; srclen: %u\n", (unsigned int)index, (unsigned int)len );
		mk_dbg_outf( "buf@%p: %.*s\n", buf, (int)(ptrdiff_t)index, buf );
		mk_dbg_outf( "src@%p: %.*s\n", src, (int)(ptrdiff_t)len, src );
		mk_dbg_leave();
		mk_log_fatalError( "strncat: detected overflow" );
	}

	memcpy( (void *)&buf[index], (const void *)src, len );
	buf[index + len] = '\0';

	return index + len;
}

/* secure strcpy */
void mk_com_strcpy( char *dst, size_t dstn, const char *src ) {
	size_t i;

	for( i = 0; src[i]; i++ ) {
		dst[i] = src[i];
		if( i == dstn - 1 ) {
			mk_log_fatalError( "strcpy: detected overflow" );
		}
	}

	dst[i] = 0;
}

/* secure strncpy */
void mk_com_strncpy( char *buf, size_t bufn, const char *src, size_t srcn ) {
	if( srcn >= bufn ) {
		mk_log_fatalError( "strncpy: detected overflow" );
	}

	strncpy( buf, src, srcn );
	buf[srcn] = 0;
}

/* secure strdup; uses mk_com_memory() */
char *mk_com__strdup( const char *cstr, const char *file, unsigned int line, const char *func ) {
	size_t n;
	char *p;

	MK_ASSERT( cstr != (const char *)0 );

	n = strlen( cstr ) + 1;

	p = (char *)mk_mem__alloc( n, kMkMemF_Uninitialized, file, line, func );
	memcpy( (void *)p, (const void *)cstr, n );

	return p;
}

/* strdup() in-place alternative */
char *mk_com_dup( char *dst, const char *src ) {
	size_t n;

	if( src ) {
		n = mk_com_strlen( src ) + 1;

		dst = (char *)mk_com_memory( (void *)dst, n );
		memcpy( (void *)dst, (const void *)src, n );
	} else {
		dst = (char *)mk_com_memory( (void *)dst, 0 );
	}

	return dst;
}
char *mk_com_dupn( char *dst, const char *src, size_t numchars ) {
	if( !numchars ) {
		numchars = mk_com_strlen( src );
	}

	if( src ) {
		dst = (char *)mk_com_memory( (void *)dst, numchars + 1 );
		memcpy( (void *)dst, (const void *)src, numchars );
		dst[numchars] = '\0';
	} else {
		dst = (char *)mk_com_memory( (void *)dst, 0 );
	}

	return dst;
}

/* dynamic version of strcat() */
char *mk_com_append( char *dst, const char *src ) {
	size_t l, n;

	if( !src ) {
		return dst;
	}

	l = dst ? mk_com_strlen( dst ) : 0;
	n = mk_com_strlen( src ) + 1;

	dst = (char *)mk_com_memory( (void *)dst, l + n );
	memcpy( (void *)&dst[l], (const void *)src, n );

	return dst;
}

/* extract the directory part of a filename */
const char *mk_com_extractDir( char *buf, size_t n, const char *filename ) {
	const char *p;

	if( ( p = strrchr( filename, '/' ) ) != (const char *)0 ) {
		mk_com_strncpy( buf, n, filename, ( p - filename ) + 1 );
		return &p[1];
	}

	*buf = 0;
	return filename;
}

/* copy the source string into the destination string, overwriting the
   extension (if present, otherwise appending) with 'ext' */
void mk_com_substExt( char *dst, size_t dstn, const char *src, const char *ext ) {
	const char *p;

	MK_ASSERT( dst != (char *)0 );
	MK_ASSERT( dstn > 1 );
	MK_ASSERT( src != (const char *)0 );

	p = strrchr( src, '.' );
	if( !p ) {
		p = strchr( src, 0 );
	}

	MK_ASSERT( p != (const char *)0 );

	mk_com_strncpy( dst, dstn, src, p - src );
	mk_com_strcpy( &dst[p - src], dstn - ( size_t )( p - src ), ext );
}

/* run a command in the mk_com_shellf */
int mk_com_shellf( const char *format, ... ) {
	static char cmd[16384];
	va_list args;

	va_start( args, format );
#if MK_SECLIB
	vsprintf_s( cmd, sizeof( cmd ), format, args );
#else
	vsnprintf( cmd, sizeof( cmd ), format, args );
	cmd[sizeof( cmd ) - 1] = 0;
#endif
	va_end( args );

#if MK_WINDOWS_ENABLED
	{
		char *p, *e;

		e = strchr( cmd, ' ' );
		if( !e ) {
			e = strchr( cmd, '\0' );
		}

		for( p = &cmd[0]; p && p < e; p = strchr( p, '/' ) ) {
			if( *p == '/' ) {
				*p = '\\';
			}
		}
	}
#endif

	if( mk__g_flags & kMkFlag_Verbose_Bit ) {
		mk_sys_printStr( kMkSIO_Err, MK_COLOR_LIGHT_CYAN, "> " );
		mk_sys_printStr( kMkSIO_Err, MK_COLOR_CYAN, cmd );
		mk_sys_uncoloredPuts( kMkSIO_Err, "\n", 1 );
	}

	fflush( mk__g_siof[kMkSIO_Err] );

	return system( cmd );
}

/* determine whether a given relative path is part of an absolute path */
int mk_com_matchPath( const char *rpath, const char *apath ) {
	size_t rl, al;

	MK_ASSERT( rpath != (const char *)0 );
	MK_ASSERT( apath != (const char *)0 );

	rl = mk_com_strlen( rpath );
	al = mk_com_strlen( apath );

	if( rl > al ) {
		return 0;
	}

#if MK_WINDOWS_ENABLED
	if( _stricmp( &apath[al - rl], rpath ) != 0 ) {
		return 0;
	}
#else
	if( strcmp( &apath[al - rl], rpath ) != 0 ) {
		return 0;
	}
#endif

	if( al - rl > 0 ) {
		if( apath[al - rl - 1] != '/' ) {
			return 0;
		}
	}

	return 1;
}

/* retrieve the current date as an integral date -- YYYYMMDD, e.g., 20120223 */
int mk_com_getIntDate( void ) {
	struct tm *p;
	time_t t;

	t = time( 0 );
	if( !( p = localtime( &t ) ) ) {
		return 0;
	}

	return ( p->tm_year + 1900 ) * 10000 + ( p->tm_mon + 1 ) * 100 + p->tm_mday;
}

/* find the end of an argument within a string */
const char *mk_com_findArgEnd( const char *arg ) {
	const char *p;

	if( *arg != '\"' ) {
		p = strchr( arg, ' ' );
		if( !p ) {
			p = strchr( arg, '\0' );
		}

		return p;
	}

	for( p = arg; *p != '\0'; p++ ) {
		if( *p == '\\' ) {
			p++;
			continue;
		}

		if( *p == '\"' ) {
			return p + 1;
		}
	}

	return p;
}

/* determine whether one argument in a string is equal to another */
int mk_com_matchArg( const char *a, const char *b ) {
	const char *x, *y;

	x = mk_com_findArgEnd( a );
	y = mk_com_findArgEnd( b );

	if( x - a != y - b ) {
		return 0;
	}

	/*
	 * TODO: compare content of each argument, not the raw input
	 */

	return (int)( strncmp( a, b, x - a ) == 0 );
}

/* skip to next argument within a string */
const char *mk_com_skipArg( const char *arg ) {
	const char *p;

	p = mk_com_findArgEnd( arg );
	while( *p <= ' ' && *p != '\0' ) {
		p++;
	}

	if( *p == '\0' ) {
		return NULL;
	}

	return p;
}

/* puts the lotion on its skin; else it gets the hose again */
void mk_com_stripArgs( char *dst, size_t n, const char *src ) {
	const char *p, *q, *next;

	mk_com_strcpy( dst, n, "" );

	for( p = src; p != NULL; p = next ) {
		next = mk_com_skipArg( p );

		for( q = src; q < p; q = mk_com_skipArg( q ) ) {
			if( mk_com_matchArg( q, p ) ) {
				break;
			}
		}

		if( q < p ) {
			continue;
		}

		q = next ? next : strchr( p, '\0' );
		mk_com_strncat( dst, n, p, q - p );
	}
}

/* determine whether two characters of a path match */
int mk_com_cmpPathChar( char a, char b ) {
#if MK_WINDOWS_ENABLED
	if( a >= 'A' && a <= 'Z' ) {
		a = a - 'A' + 'a';
	}
	if( b >= 'A' && b <= 'Z' ) {
		b = b - 'A' + 'a';
	}
	if( a == '\\' ) {
		a = '/';
	}
	if( b == '\\' ) {
		b = '/';
	}
#endif

	return a == b ? 1 : 0;
}

/* retrieve a relative path */
int mk_com_relPath( char *dst, size_t dstn, const char *curpath, const char *abspath ) {
	size_t i;

	MK_ASSERT( dst != (char *)0 );
	MK_ASSERT( dstn > 1 );
	MK_ASSERT( curpath != (const char *)0 );
	MK_ASSERT( abspath != (const char *)0 );

	i = 0;
	while( mk_com_cmpPathChar( curpath[i], abspath[i] ) && curpath[i] != '\0' ) {
		i++;
	}

	if( curpath[i] == '\0' && abspath[i] == '\0' ) {
		mk_com_strcpy( dst, dstn, "" );
	} else {
		mk_com_strcpy( dst, dstn, &abspath[i] );
	}

	return 1;
}

/* retrieve a relative path based on the current working directory */
int mk_com_relPathCWD( char *dst, size_t dstn, const char *abspath ) {
	char cwd[PATH_MAX];

	if( !mk_fs_getCWD( cwd, sizeof( cwd ) ) ) {
		return 0;
	}

	return mk_com_relPath( dst, dstn, cwd, abspath );
}

/* retrieve an environment variable */
const char *mk_com_getenv( char *dst, size_t dstn, const char *var ) {
	const char *p;

	MK_ASSERT( dst != (char *)0 );
	MK_ASSERT( dstn > 0 );
	MK_ASSERT( var != (const char *)0 );

	if( !( p = getenv( var ) ) ) {
		*dst = '\0';
		return (const char *)0;
	}

	mk_com_strcpy( dst, dstn, p );
	return dst;
}

/* determine whether a given string starts with another string */
int mk_com_strstarts( const char *src, const char *with ) {
	MK_ASSERT( src != (const char *)0 );
	MK_ASSERT( with != (const char *)0 );

	return strncmp( src, with, mk_com_strlen( with ) ) == 0;
}
/* determine whether a given string ends with another string */
int mk_com_strends( const char *src, const char *with ) {
	size_t cSrc, cWith;

	MK_ASSERT( src != (const char *)0 );
	MK_ASSERT( with != (const char *)0 );

	cSrc  = mk_com_strlen( src );
	cWith = mk_com_strlen( with );

	if( cWith > cSrc ) {
		return 0;
	}

	return strcmp( src + ( cSrc - cWith ), with ) == 0;
}

/* perform in-place fixing of the path; sets directory separator to '/' on Windows */
void mk_com_fixpath( char *path ) {
#if MK_WINDOWS_ENABLED
	char *p;

	MK_ASSERT( path != (char *)0 );

	p = path;
	while( ( p = strchr( p, '\\' ) ) != (char *)0 ) {
		*p++ = '/';
	}
#endif

	(void)path;
}
