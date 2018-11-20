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
#include "mk-build-dependency.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-stringList.h"
#include "mk-defs-config.h"

#include <string.h>

struct MkDep_s {
	char *name;
	MkStrList deps;

	struct MkDep_s *prev, *next;
};
MkDep mk__g_dep_head = (MkDep)0;
MkDep mk__g_dep_tail = (MkDep)0;

/* create a new dependency list */
MkDep mk_dep_new( const char *name ) {
	MkDep dep;

	MK_ASSERT( name != (const char *)0 );

	dep = (MkDep)mk_com_memory( (void *)0, sizeof( *dep ) );

	dep->name = mk_com_strdup( name );
	dep->deps = mk_sl_new();

	dep->next = (MkDep)0;
	if( ( dep->prev = mk__g_dep_tail ) != (MkDep)0 ) {
		mk__g_dep_tail->next = dep;
	} else {
		mk__g_dep_head = dep;
	}
	mk__g_dep_tail = dep;

	return dep;
}

/* delete a dependency list */
void mk_dep_delete( MkDep dep ) {
	if( !dep ) {
		return;
	}

	dep->name = (char *)mk_com_memory( (void *)dep->name, 0 );
	mk_sl_delete( dep->deps );
	dep->deps = (MkStrList)0;

	if( dep->prev ) {
		dep->prev->next = dep->next;
	}
	if( dep->next ) {
		dep->next->prev = dep->prev;
	}

	if( mk__g_dep_head == dep ) {
		mk__g_dep_head = dep->next;
	}
	if( mk__g_dep_tail == dep ) {
		mk__g_dep_tail = dep->prev;
	}

	mk_com_memory( (void *)dep, 0 );
}

/* delete all dependency lists */
void mk_dep_deleteAll( void ) {
	while( mk__g_dep_head ) {
		mk_dep_delete( mk__g_dep_head );
	}
}

/* retrieve the name of the file a dependency list is tracking */
const char *mk_dep_getFile( MkDep dep ) {
	MK_ASSERT( dep != (MkDep)0 );

	return dep->name;
}

/* add a dependency to a list */
void mk_dep_push( MkDep dep, const char *name ) {
	MK_ASSERT( dep != (MkDep)0 );
	MK_ASSERT( name != (const char *)0 );

	mk_sl_pushBack( dep->deps, name );
#if MK_DEBUG_DEPENDENCY_TRACKER_ENABLED
	mk_dbg_outf( "~ mk_dep_push \"%s\": \"%s\";\n", dep->name, name );
#endif
}

/* retrieve the number of dependencies in a list */
size_t mk_dep_getSize( MkDep dep ) {
	MK_ASSERT( dep != (MkDep)0 );

	return mk_sl_getSize( dep->deps );
}

/* retrieve a dependency from a list */
const char *mk_dep_at( MkDep dep, size_t i ) {
	MK_ASSERT( dep != (MkDep)0 );
	MK_ASSERT( i < mk_dep_getSize( dep ) );

	return mk_sl_at( dep->deps, i );
}

/* find a dependency */
MkDep mk_dep_find( const char *name ) {
	MkDep dep;

	for( dep = mk__g_dep_head; dep; dep = dep->next ) {
		if( !strcmp( dep->name, name ) ) {
			return dep;
		}
	}

	return (MkDep)0;
}

/* print all known dependencies */
void mk_dep_debugPrintAll( void ) {
#if MK_DEBUG_DEPENDENCY_TRACKER_ENABLED
	MkDep dep;

	for( dep = mk__g_dep_head; dep; dep = dep->next ) {
		mk_dbg_outf( " ** dep: \"%s\"\n", dep->name );
	}
#endif
}
