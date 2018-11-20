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
 *	AUTOLINK SYSTEM
 *	========================================================================
 *	This system maps which header files are used to access which libraries. This
 *	allows the dependency system to be exploited to reveal what flags need to be
 *	passed to the linker for the system to "just work."
 */

#include "mk-build-platform.h"
#include "mk-defs-config.h"

#if !MK_DEBUG_ENABLED
#	undef MK_DEBUG_AUTOLIBCONF_ENABLED
#	define MK_DEBUG_AUTOLIBCONF_ENABLED 0
#else
#	ifndef MK_DEBUG_AUTOLIBCONF_ENABLED
#		define MK_DEBUG_AUTOLIBCONF_ENABLED 1
#	endif
#endif

typedef struct MkAutolink_s *MkAutolink;

struct MkAutolink_s {
	char *header[kMkNumOS];
	char *lib;

	struct MkAutolink_s *prev, *next;
};

MkAutolink mk_al_new( void );
void mk_al_delete( MkAutolink al );
void mk_al_deleteAll( void );
void mk_al_setHeader( MkAutolink al, int sys, const char *header );
void mk_al_setLib( MkAutolink al, const char *libname );
const char *mk_al_getHeader( MkAutolink al, int sys );
const char *mk_al_getLib( MkAutolink al );
MkAutolink mk_al_find( int sys, const char *header );
MkAutolink mk_al_lookup( int sys, const char *header );
const char *mk_al_autolink( int sys, const char *header );
void mk_al_managePackage_r( const char *libname, int sys, const char *incdir );

/*
===============================================================================

	AUTOLINK CONFIG FILE FORMAT
	===========================

	This allows you to configure a set of libraries/autolink headers. It's
	pretty simple:

		libname "linkflags" plat:"linkflags" plat:"linkflags" {
			"header/file/name.h"
			plat:"header/file/name.h"
			plat:"header/file/name.h"
		}

	e.g., for OpenGL:

		opengl "-lGL" mswin:"-lopengl32" apple:"-framework OpenGL" {
			"GL/gl.h"
			apple:"OpenGL/OpenGL.h"
		}

	New entries override old entries.

===============================================================================
*/

int mk_al_loadConfig( const char *filename );
