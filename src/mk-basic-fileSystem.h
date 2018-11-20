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
 *	FILE SYSTEM MANAGEMENT CODE
 *	========================================================================
 *	This code deals with various file system related subjects. This includes
 *	making directories and finding where the executable is, etc.
 */

#include "mk-basic-stringList.h"
#include "mk-basic-types.h"

#if MK_HAS_DIRENT
#	include <dirent.h>
#else
#	include "dirent.h" /* user-provided replacement */
#endif

extern MkStrList mk__g_fs_dirstack;

void mk_fs_init( void );
void mk_fs_fini( void );

char *mk_fs_getCWD( char *cwd, size_t n );
int mk_fs_enter( const char *path );
void mk_fs_leave( void );
int mk_fs_isFile( const char *path );
int mk_fs_isDir( const char *path );
void mk_fs_makeDirs( const char *dirs );
char *mk_fs_realPath( const char *filename, char *resolvedname, size_t maxn );

DIR *mk_fs_openDir( const char *path );
DIR *mk_fs_closeDir( DIR *p );
struct dirent *mk_fs_readDir( DIR *d );
void mk_fs_remove( const char *path );

typedef enum MkLanguage_e {
	kMkLanguage_Unknown,

	kMkLanguage_C89,
	kMkLanguage_C99,
	kMkLanguage_C11,

	kMkLanguage_Cxx98,
	kMkLanguage_Cxx03,
	kMkLanguage_Cxx11,
	kMkLanguage_Cxx14,

	kMkLanguage_ObjC,
	kMkLanguage_ObjCxx,

	kMkLanguage_Assembly,

	kMkLanguage_CFamily_Begin = kMkLanguage_C89,
	kMkLanguage_CFamily_End   = kMkLanguage_ObjCxx,

	kMkLanguage_C_Begin = kMkLanguage_C89,
	kMkLanguage_C_End   = kMkLanguage_C11,

	kMkLanguage_Cxx_Begin = kMkLanguage_Cxx98,
	kMkLanguage_Cxx_End   = kMkLanguage_Cxx14,

	kMkLanguage_ObjC_Begin = kMkLanguage_ObjC,
	kMkLanguage_ObjC_End   = kMkLanguage_ObjC,

	kMkLanguage_ObjCxx_Begin = kMkLanguage_ObjCxx,
	kMkLanguage_ObjCxx_End   = kMkLanguage_ObjCxx
} MkLanguage;
MkLanguage mk_fs_getLanguage( const char *filename );
