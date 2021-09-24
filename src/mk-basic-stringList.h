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
 *	DYNAMIC STRING ARRAY SYSTEM
 *	========================================================================
 *	Manages a dynamic array of strings. Functions similarly to the C++ STL's
 *	std::vector<std::string> class.
 */

#include <stddef.h>

typedef struct MkStrList_s *MkStrList;

MkStrList mk_sl_new( void );
void      mk_sl_delete( MkStrList arr );
void      mk_sl_deleteAll( void );

size_t mk_sl_getCapacity( MkStrList arr );
size_t mk_sl_getSize( MkStrList arr );
char **mk_sl_getData( MkStrList arr );
char * mk_sl_at( MkStrList arr, size_t i );
void   mk_sl_set( MkStrList arr, size_t i, const char *cstr );
void   mk_sl_clear( MkStrList arr );
void   mk_sl_resize( MkStrList arr, size_t n );
void   mk_sl_pushBack( MkStrList arr, const char *cstr );
void   mk_sl_popBack( MkStrList arr );
void   mk_sl_print( MkStrList arr );
void   mk_sl_debugPrint( MkStrList arr );
void   mk_sl_sort( MkStrList arr );

void mk_sl_orderedSort( MkStrList arr, size_t *const buffer, size_t maxBuffer );
void mk_sl_indexedSort( MkStrList arr, const size_t *const buffer, size_t bufferLen );
void mk_sl_printOrderedBuffer( const size_t *const buffer, size_t bufferLen );
void mk_sl_makeUnique( MkStrList arr );
