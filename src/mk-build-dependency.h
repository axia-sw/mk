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
 *	DEPENDENCY TRACKER
 *	========================================================================
 *	Track dependencies and manage the general structure.
 */

#include "mk-defs-config.h"
#include <stddef.h>

#if !MK_DEBUG_ENABLED
#	undef MK_DEBUG_DEPENDENCY_TRACKER_ENABLED
#	define MK_DEBUG_DEPENDENCY_TRACKER_ENABLED 0
#else
#	ifndef MK_DEBUG_DEPENDENCY_TRACKER_ENABLED
#		define MK_DEBUG_DEPENDENCY_TRACKER_ENABLED 0
#	endif
#endif

typedef struct MkDep_s *MkDep;

MkDep mk_dep_new( const char *name );
void mk_dep_delete( MkDep dep );
void mk_dep_deleteAll( void );
const char *mk_dep_getFile( MkDep dep );
void mk_dep_push( MkDep dep, const char *name );
size_t mk_dep_getSize( MkDep dep );
const char *mk_dep_at( MkDep dep, size_t i );
MkDep mk_dep_find( const char *name );
void mk_dep_debugPrintAll( void );
