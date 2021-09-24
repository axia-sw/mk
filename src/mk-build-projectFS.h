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
 *	DIRECTORY MANAGEMENT
 *	========================================================================
 */

#include "mk-basic-stringList.h"
#include "mk-build-project.h"

int mk_prjfs_isSpecialDir( MkProject proj, const char *name );
int mk_prjfs_isIncDir( MkProject proj, const char *name );
int mk_prjfs_isLibDir( MkProject proj, const char *name );
int mk_prjfs_isTestDir( MkProject proj, const char *name );
int mk_prjfs_isDirOwner( const char *path );

void mk_prjfs_enumSourceFiles( MkProject proj, const char *srcdir );
void mk_prjfs_enumTestSourceFiles( MkProject proj, const char *srcdir );

int       mk_prjfs_calcName( MkProject proj, const char *path, const char *file );
MkProject mk_prjfs_add( MkProject prnt, const char *path, const char *file, int type );

void mk_prjfs_findPackages( const char *pkgdir );
void mk_prjfs_findDynamicLibs( const char *dllsdir );
void mk_prjfs_findTools( const char *tooldir );
void mk_prjfs_findProjects( MkProject prnt, const char *srcdir );
void mk_prjfs_findRootDirs( MkStrList srcdirs, MkStrList incdirs, MkStrList libdirs, MkStrList pkgdirs, MkStrList tooldirs, MkStrList dllsdirs );

int mk_prjfs_makeObjDirs( MkProject proj );
