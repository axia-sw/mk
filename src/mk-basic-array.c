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

#include "mk-basic-array.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"

#include <string.h>

void mk_arr__int_fini( mk_array_info_t *info ) {
	MK_ASSERT( info != (mk_array_info_t *)0 );

	*(info->ptr) = mk_com_memory( *(info->ptr), 0 );
	*(info->len) = 0;
}
int mk_arr__int_resize( mk_array_info_t *info, size_t new_len ) {
	union {
		void *p;
		unsigned char *bp;
	} q;
	size_t old_len;

	MK_ASSERT( info != (mk_array_info_t *)0 );
	MK_ASSERT( info->stride != 0 );

	old_len = *(info->len);
	if( new_len <= old_len ) {
		*(info->len) = new_len;
		return 1;
	}

	q.p = mk_com_memory( *(info->ptr), new_len*info->stride );
	if( !q.p ) {
		return 0;
	}

	*(info->ptr) = q.p;
	*(info->len) = new_len;

	memset( q.bp + old_len*info->stride, 0, ( new_len - old_len )*info->stride );

	return 1;
}
int mk_arr__int_append( mk_array_info_t *info ) {
	MK_ASSERT( info != (mk_array_info_t *)0 );
	MK_ASSERT( info->stride != 0 );

	return mk_arr__int_resize( info, *(info->len) + 1 );
}
