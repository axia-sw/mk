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
 *	COMPILATION MANAGEMENT
 *	========================================================================
 */

#include <stddef.h>

#include "mk-basic-stringList.h"
#include "mk-build-library.h"
#include "mk-build-project.h"
#include "mk-defs-config.h"

#if !MK_DEBUG_ENABLED
#	undef MK_DEBUG_FIND_SOURCE_LIBS_ENABLED
#	define MK_DEBUG_FIND_SOURCE_LIBS_ENABLED 0
#else
#	ifndef MK_DEBUG_FIND_SOURCE_LIBS_ENABLED
#		define MK_DEBUG_FIND_SOURCE_LIBS_ENABLED 0
#	endif
#endif

void mk_bld_initUnitTestArrays( void );
void mk_bld_unitTest( MkProject proj, const char *src );
void mk_bld_runTests( void );

int mk_bld_findSourceLibs( MkStrList dst, int sys, const char *obj, const char *dep );
int mk_bld_shouldCompile( const char *obj );
int mk_bld_shouldLink( const char *bin, int numbuilds );

const char *mk_bld_getCompiler( int iscxx );
void        mk_bld_getCFlags_warnings( char *flags, size_t nflags );
int         mk_bld_getCFlags_standard( char *flags, size_t nflags, const char *filename );
void        mk_bld_getCFlags_config( char *flags, size_t nflags, int projarch );
void        mk_bld_getCFlags_platform( char *flags, size_t nflags, int projarch, int projsys, int usenative );
void        mk_bld_getCFlags_projectType( char *flags, size_t nflags, int projtype );
void        mk_bld_getCFlags_incDirs( char *flags, size_t nflags );
void        mk_bld_getCFlags_defines( char *flags, size_t nflags, MkStrList defs );
void        mk_bld_getCFlags_unitIO( char *flags, size_t nflags, const char *obj, const char *src );
const char *mk_bld_getCFlags( MkProject proj, const char *obj, const char *src );

void        mk_bld_getDeps_r( MkProject proj, MkStrList deparray );
int         mk_bld_doesLibDependOnLib( MkLib mainlib, MkLib deplib );
void        mk_bld_sortDeps( MkStrList deparray );
const char *mk_bld_getProjDepLinkFlags( MkProject proj );

void        mk_bld_getLibs( MkProject proj, char *dst, size_t n );
const char *mk_bld_getLFlags( MkProject proj, const char *bin, MkStrList objs );

void mk_bld_getObjName( MkProject proj, char *obj, size_t n, const char *src );
void mk_bld_getBinName( MkProject proj, char *bin, size_t n );

void mk_bld_sortProjects( struct MkProject_s *proj );
void mk_bld_relinkDeps( MkProject proj );
int  mk_bld_makeProject( MkProject proj );
int  mk_bld_makeAllProjects( void );
