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
#include "mk-basic-stringList.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-memory.h"
#include "mk-basic-stringList.h"
#include "mk-defs-config.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MkStrList_s {
	size_t capacity;
	size_t size;
	char **data;

	struct MkStrList_s *prev, *next;
};
struct MkStrList_s *mk__g_arr_head = (struct MkStrList_s *)0;
struct MkStrList_s *mk__g_arr_tail = (struct MkStrList_s *)0;

/* create a new (empty) array */
MkStrList mk_sl_new( void ) {
	MkStrList arr;

	arr = (MkStrList)mk_com_memory( (void *)0, sizeof( *arr ) );

	arr->capacity = 0;
	arr->size     = 0;
	arr->data     = (char **)0;

	arr->next = (struct MkStrList_s *)0;
	if( ( arr->prev = mk__g_arr_tail ) != (struct MkStrList_s *)0 ) {
		mk__g_arr_tail->next = arr;
	} else {
		mk__g_arr_head = arr;
	}
	mk__g_arr_tail = arr;

	return arr;
}

/* retrieve the current amount of mk_com_memory allocated for the array */
size_t mk_sl_getCapacity( MkStrList arr ) {
	MK_ASSERT( arr != (MkStrList)0 );

	return arr->capacity;
}

/* retrieve how many elements are in the array */
size_t mk_sl_getSize( MkStrList arr ) {
	if( !arr ) {
		return 0;
	}

	return arr->size;
}

/* retrieve the data of the array */
char **mk_sl_getData( MkStrList arr ) {
	MK_ASSERT( arr != (MkStrList)0 );

	return arr->data;
}

/* retrieve a single element of the array */
char *mk_sl_at( MkStrList arr, size_t i ) {
	MK_ASSERT( arr != (MkStrList)0 );
	MK_ASSERT( i < arr->size );

	return arr->data[i];
}

static size_t calculate_string_size( const char *cstr ) {
	if( !cstr ) {
		return 0;
	}

	return mk_com_strlen( cstr ) + 1;
}

/* set a single element of the array */
void mk_sl_set( MkStrList arr, size_t i, const char *cstr ) {
	MK_ASSERT( arr != (MkStrList)0 );
	MK_ASSERT( i < arr->size );

	size_t len = calculate_string_size( cstr );

	arr->data[i] = (char *)mk_com_memory( (void *)arr->data[i], calculate_string_size( cstr ) );
	MK_ASSERT( len == 0 || arr->data[i] != (char *)0 );

	if( cstr != (const char *)0 ) {
		memcpy( arr->data[i], cstr, len );
	}
}

/* deallocate the internal mk_com_memory used by the array */
void mk_sl_clear( MkStrList arr ) {
	size_t i;

	MK_ASSERT( arr != (MkStrList)0 );

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_enter( "mk_sl_clear(%p)", arr );
#endif

	for( i = arr->size; i-- > 0; ) {
		arr->data[i] = (char *)mk_com_memory( (void *)arr->data[i], 0 );
	}

	arr->data     = (char **)mk_com_memory( (void *)arr->data, 0 );
	arr->capacity = 0;
	arr->size     = 0;

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_leave();
#endif
}

/* delete an array */
void mk_sl_delete( MkStrList arr ) {
	if( !arr ) {
		return;
	}

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_enter( "mk_sl_delete(%p)", arr );
#endif

	mk_sl_clear( arr );

	if( arr->prev ) {
		arr->prev->next = arr->next;
	} else {
		mk__g_arr_head = arr->next;
	}

	if( arr->next ) {
		arr->next->prev = arr->prev;
	} else {
		mk__g_arr_tail = arr->prev;
	}

	arr->prev = (MkStrList)0;
	arr->next = (MkStrList)0;

	mk_com_memory( (void *)arr, 0 );

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_leave();
#endif
}

/* delete all arrays */
void mk_sl_deleteAll( void ) {
#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_enter("mk_sl_deleteAll");
#endif

	while( mk__g_arr_head ) {
		mk_sl_delete( mk__g_arr_head );
	}

#if MK_LOG_MEMORY_ALLOCS_ENABLED
	mk_dbg_leave();
#endif
}

/* set the new size of an array */
void mk_sl_resize( MkStrList arr, size_t n ) {
	size_t i;

	MK_ASSERT( arr != (MkStrList)0 );

	if( n > arr->capacity ) {
		i = arr->capacity;
		arr->capacity += 4096 / sizeof( void * );
		/*arr->capacity += 32;*/

		arr->data = (char **)mk_com_memory( (void *)arr->data,
		    arr->capacity * sizeof( char * ) );

		memset( (void *)&arr->data[i], 0, ( arr->capacity - i ) * sizeof( char * ) );
	}

	arr->size = n;
}

/* add an element to the array, resizing if necessary */
void mk_sl_pushBack( MkStrList arr, const char *cstr ) {
	size_t i;

	MK_ASSERT( arr != (MkStrList)0 );

	i = mk_sl_getSize( arr );
	mk_sl_resize( arr, i + 1 );
	mk_sl_set( arr, i, cstr );
}
/* remove the last element in the array */
void mk_sl_popBack( MkStrList arr ) {
	MK_ASSERT( arr != (MkStrList)0 );

	if( !arr->size ) {
		return;
	}

	mk_sl_set( arr, arr->size - 1, (const char *)0 );
	--arr->size;
}

/* calculate the number of digits in a given number */
static int count_digits( size_t n ) {
	int numdigits = 0;
	do {
		numdigits += 1;
		n /= 10;
	} while( n != 0 );
	return numdigits;
}

/* display the contents of an array */
void mk_sl_print( MkStrList arr ) {
	size_t i, n;
	int numdigits;

	MK_ASSERT( arr != (MkStrList)0 );

	n = mk_sl_getSize( arr );
	numdigits = count_digits( n );
	for( i = 0; i < n; i++ ) {
#if MK_WINDOWS_ENABLED
		printf( "%*u. \"%s\"\n", numdigits, (unsigned int)i, mk_sl_at( arr, i ) );
#else
		/* ISO C90 does not support the 'z' modifier; ignore... */
		printf( "%*zu. \"%s\"\n", numdigits, i, mk_sl_at( arr, i ) );
#endif
	}

	printf( "\n" );
}
void mk_sl_debugPrint( MkStrList arr ) {
	size_t i, n;
	int numdigits;

	MK_ASSERT( arr != (MkStrList)0 );

	n = mk_sl_getSize( arr );
	numdigits = count_digits( n );
	for( i = 0; i < n; i++ ) {
#if MK_WINDOWS_ENABLED
		mk_dbg_outf( "%*u. \"%s\"\n", numdigits, (unsigned int)i, mk_sl_at( arr, i ) );
#else
		/* ISO C90 does not support the 'z' modifier; ignore... */
		mk_dbg_outf( "%*zu. \"%s\"\n", numdigits, i, mk_sl_at( arr, i ) );
#endif
	}

	mk_dbg_outf( "\n" );
}

static int sl__cmp_f( const void *a, const void *b ) {
	return strcmp( *(char *const *)a, *(char *const *)b );
}

/* alphabetically sort an array */
void mk_sl_sort( MkStrList arr ) {
	if( arr->size < 2 ) {
		return;
	}

	qsort( (void *)arr->data, arr->size, sizeof( char * ), sl__cmp_f );
}

void mk_sl_orderedSort( MkStrList arr, size_t *const buffer, size_t maxBuffer ) {
	size_t i, j;
	size_t n;

	MK_ASSERT( buffer != NULL );
	MK_ASSERT( maxBuffer >= arr->size );

	n = arr->size < maxBuffer ? arr->size : maxBuffer;

	for( i = 0; i < n; i++ ) {
		buffer[i] = (size_t)arr->data[i];
	}

	mk_sl_sort( arr );

	for( i = 0; i < n; i++ ) {
		for( j = 0; j < n; j++ ) {
			if( buffer[i] == (size_t)arr->data[j] ) {
				buffer[i] = j;
				break;
			}
		}

		MK_ASSERT( j < n ); /* did exit from loop? */
	}
}
void mk_sl_indexedSort( MkStrList arr, const size_t *const buffer, size_t bufferLen ) {
#if 1
	MkStrList backup;
	size_t i, n;

	/*
	 *	XXX: This is ugly.
	 *	FIXME: Ugly.
	 *	XXX: You can't fix ugly.
	 *	TODO: Buy a makeup kit for the beast.
	 *
	 *	In all seriousness, an in-place implementation should be written. This works for now,
	 *	though.
	 */

	MK_ASSERT( buffer != NULL );
	MK_ASSERT( bufferLen > 0 );

	backup = mk_sl_new();
	n      = bufferLen < arr->size ? bufferLen : arr->size;
	for( i = 0; i < n; i++ ) {
		mk_sl_pushBack( backup, mk_sl_at( arr, i ) );
	}

	for( i = 0; i < n; i++ ) {
		mk_sl_set( arr, buffer[i], mk_sl_at( backup, i ) );
	}

	mk_sl_delete( backup );
#else
	char *tmp;
	size_t i;

	/*
	 *	FIXME: This partially works, but produces bad results.
	 */

	for( i = 0; i < bufferLen; i++ ) {
		if( i > buffer[i] )
			continue;

		tmp = arr->data[buffer[i]];
		arr->data[buffer[i]] = arr->data[i];
		arr->data[i] = tmp;
	}
#endif
}
void mk_sl_printOrderedBuffer( const size_t *const buffer, size_t bufferLen ) {
	size_t i;

	printf( "array buffer %p (%u element%s):\n", (void *)buffer,
	    (unsigned int)bufferLen, bufferLen == 1 ? "" : "s" );
	for( i = 0; i < bufferLen; i++ ) {
		printf( "  %u\n", (unsigned int)buffer[i] );
	}
	printf( "\n" );
}

/* remove duplicate entries from an array */
void mk_sl_makeUnique( MkStrList arr ) {
	const char *a, *b;
	size_t i, j, k, n;

	n = mk_sl_getSize( arr );
	for( i = 0; i < n; i++ ) {
		if( !( a = mk_sl_at( arr, i ) ) ) {
			continue;
		}

		for( j = i + 1; j < n; j++ ) {
			if( !( b = mk_sl_at( arr, j ) ) ) {
				continue;
			}

			if( strcmp( a, b ) == 0 ) {
				mk_sl_set( arr, j, (const char *)0 );
			}
		}
	}

	arr->size = 0;
	for( i = 0; i < n; ++i ) {
		if( arr->data[i] != (char *)0 ) {
			arr->size = i + 1;
			continue;
		}

		j = i + 1;
		while( j < n && arr->data[j] == (char *)0 ) {
			++j;
		}

		if( j == n ) {
			break;
		}

		for( k = 0; j + k < n; ++k ) {
			arr->data[i + k] = arr->data[j + k];
		}

		--i;
	}
}
