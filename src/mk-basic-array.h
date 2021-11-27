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

#include "mk-defs-config.h"
#include "mk-defs-platform.h"

#include <stddef.h>

/*
===============================================================================

	DYNAMIC ARRAYS

	Wrap your arrays in a structure:

		struct {
			size_t len;
			T *ptr;
		} myarray;

	Note, the order of the fields doesn't matter. The most important point is
	that they be named `len` and `ptr`.

	The type (T) is inferred from the `ptr` member in this structure.

	Also, `len` must be a size_t.

	Call array functions to manipulate:

		struct {
			size_t len;
			int   *ptr;
		} intarray;

		mk_arr_init( intarray );

		mk_arr_append( intarray, 1 );
		mk_arr_append( intarray, 1 + 1 );
		mk_arr_append( intarray, 2 + 2 - 1 );

		for( size_t i = 0; i < mk_arr_len(intarray); ++i ) {
			printf( "%i\n", mk_arr_at(intarray, i) );
		}

		mk_arr_fini( intarray );

===============================================================================
*/

#define mk_arr_init(Arr_)           do{        \
	*((void**)&(Arr_).ptr) = (void*)0;         \
	(Arr_).len = 0;                            \
}while(0)

#define mk_arr_fini(Arr_)           do{        \
	mk_array_info_t mkarrinf = {               \
		.len=&(Arr_).len,                      \
		.ptr=(void**)&(Arr_).ptr,              \
		.stride=sizeof(*((Arr_).ptr))          \
	};                                         \
	mk_arr__int_fini(&mkarrinf);               \
}while(0)

#define mk_arr_resize(Arr_,Length_) do{        \
	mk_array_info_t mkarrinf = {               \
		.len=&(Arr_).len,                      \
		.ptr=(void**)&(Arr_).ptr,              \
		.stride=sizeof(*((Arr_).ptr))          \
	};                                         \
	mk_arr__int_resize(&mkarrinf, (Length_));  \
}while(0)

#define mk_arr_len(Arr_)            ((Arr_).len)
#define mk_arr_at(Arr_,Index_)      ((Arr_).ptr[(Index_)])

#define mk_arr_append(Arr_,Val_)    do{        \
	mk_array_info_t mkarrinf = {               \
		.len=&(Arr_).len,                      \
		.ptr=(void**)&(Arr_).ptr,              \
		.stride=sizeof(*((Arr_).ptr))          \
	};                                         \
	if(mk_arr__int_append(&mkarrinf)) {        \
		((Arr_).ptr)[(Arr_).len - 1] = (Val_); \
	}                                          \
}while(0)

#define mk_arr_clear(Arr_)          ((void)(((Arr_).len) = 0))

#define mk_arr_for(Arr_,Idx_)       for((Idx_)=0; (Idx_)<mk_arr_len(Arr_); (Idx_)+=1)

typedef struct mk_array_info_s {
	size_t *len;
	void **ptr;

	size_t stride;
} mk_array_info_t;

void mk_arr__int_fini( mk_array_info_t *info );
int  mk_arr__int_resize( mk_array_info_t *info, size_t new_len );
int  mk_arr__int_append( mk_array_info_t *info );
