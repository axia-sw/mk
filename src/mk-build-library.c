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
#include "mk-build-library.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-build-project.h"

#include <stddef.h>
#include <string.h>

struct MkLib_s *mk__g_lib_head = (struct MkLib_s *)0;
struct MkLib_s *mk__g_lib_tail = (struct MkLib_s *)0;

/* create a new library */
MkLib mk_lib_new( void ) {
	size_t i;
	MkLib lib;

	lib = (MkLib)mk_com_memory( (void *)0, sizeof( *lib ) );

	lib->name = (char *)0;
	for( i = 0; i < sizeof( lib->flags ) / sizeof( lib->flags[0] ); i++ ) {
		lib->flags[i] = (char *)0;
	}

	lib->proj = (MkProject)0;

	lib->config = 0;

	lib->next = (struct MkLib_s *)0;
	if( ( lib->prev = mk__g_lib_tail ) != (struct MkLib_s *)0 ) {
		mk__g_lib_tail->next = lib;
	} else {
		mk__g_lib_head = lib;
	}
	mk__g_lib_tail = lib;

	return lib;
}

/* delete an existing library */
void mk_lib_delete( MkLib lib ) {
	size_t i;

	if( !lib ) {
		return;
	}

	lib->name_n = 0;
	lib->name   = (char *)mk_com_memory( (void *)lib->name, 0 );
	for( i = 0; i < sizeof( lib->flags ) / sizeof( lib->flags[0] ); i++ ) {
		lib->flags[i] = (char *)mk_com_memory( (void *)lib->flags[i], 0 );
	}

	if( lib->prev ) {
		lib->prev->next = lib->next;
	}
	if( lib->next ) {
		lib->next->prev = lib->prev;
	}

	if( mk__g_lib_head == lib ) {
		mk__g_lib_head = lib->next;
	}
	if( mk__g_lib_tail == lib ) {
		mk__g_lib_tail = lib->prev;
	}

	mk_com_memory( (void *)lib, 0 );
}

/* delete all existing libraries */
void mk_lib_deleteAll( void ) {
	while( mk__g_lib_head )
		mk_lib_delete( mk__g_lib_head );
}

/* set the name of a library */
void mk_lib_setName( MkLib lib, const char *name ) {
	MK_ASSERT( lib != (MkLib)0 );

	lib->name_n = name != (const char *)0 ? strlen( name ) : 0;
	lib->name   = mk_com_dup( lib->name, name );
}

/* set the flags of a library */
void mk_lib_setFlags( MkLib lib, int sys, const char *flags ) {
	MK_ASSERT( lib != (MkLib)0 );
	MK_ASSERT( sys >= 0 && sys < kMkNumOS );

	lib->flags[sys] = mk_com_dup( lib->flags[sys], flags );
}

/* retrieve the name of a library */
const char *mk_lib_getName( MkLib lib ) {
	MK_ASSERT( lib != (MkLib)0 );

	return lib->name;
}

/* retrieve the flags of a library */
const char *mk_lib_getFlags( MkLib lib, int sys ) {
	MK_ASSERT( lib != (MkLib)0 );
	MK_ASSERT( sys >= 0 && sys < kMkNumOS );

	return lib->flags[sys];
}

/* retrieve the library before another */
MkLib mk_lib_prev( MkLib lib ) {
	MK_ASSERT( lib != (MkLib)0 );

	return lib->prev;
}

/* retrieve the library after another */
MkLib mk_lib_next( MkLib lib ) {
	MK_ASSERT( lib != (MkLib)0 );

	return lib->next;
}

/* retrieve the first library */
MkLib mk_lib_head( void ) {
	return mk__g_lib_head;
}

/* retrieve the last library */
MkLib mk_lib_tail( void ) {
	return mk__g_lib_tail;
}

/* find a library by its name */
MkLib mk_lib_find( const char *name ) {
	size_t name_n;
	MkLib lib;

	MK_ASSERT( name != (const char *)0 );

	name_n = strlen( name );

	for( lib = mk__g_lib_head; lib; lib = lib->next ) {
		if( !lib->name || name_n != lib->name_n ) {
			continue;
		}

		if( strcmp( lib->name, name ) == 0 ) {
			return lib;
		}
	}

	return (MkLib)0;
}

/* find or create a library by name */
MkLib mk_lib_lookup( const char *name ) {
	MkLib lib;

	MK_ASSERT( name != (const char *)0 );

	if( ( lib = mk_lib_find( name ) ) != (MkLib)0 ) {
		return lib;
	}

	lib = mk_lib_new();
	mk_lib_setName( lib, name );

	return lib;
}

/* mark all libraries as "not processed" */
void mk_lib_clearAllProcessed( void ) {
	MkLib lib;

	for( lib = mk__g_lib_head; lib; lib = lib->next ) {
		lib->config &= ~kMkLib_Processed_Bit;
	}
}

/* mark a library as "not processed" */
void mk_lib_clearProcessed( MkLib lib ) {
	MK_ASSERT( lib != (MkLib)0 );

	lib->config &= ~kMkLib_Processed_Bit;
}

/* mark a library as "processed" */
void mk_lib_setProcessed( MkLib lib ) {
	MK_ASSERT( lib != (MkLib)0 );

	lib->config |= kMkLib_Processed_Bit;
}

/* determine whether a library was marked as "processed" */
int mk_lib_isProcessed( MkLib lib ) {
	MK_ASSERT( lib != (MkLib)0 );

	return lib->config & kMkLib_Processed_Bit ? 1 : 0;
}
