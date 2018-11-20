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
 *	MAIN PROGRAM (FRONTEND)
 *	========================================================================
 */

#include "mk-basic-options.h"
#include "mk-basic-stringList.h"
#include "mk-basic-types.h"
#include "mk-system-output.h"

void mk_front_pushSrcDir( const char *srcdir );
void mk_front_pushIncDir( const char *incdir );
void mk_front_pushLibDir( const char *libdir );
void mk_front_pushPkgDir( const char *pkgdir );
void mk_front_pushToolDir( const char *tooldir );
void mk_front_pushDynamicLibsDir( const char *dllsdir );

void mk_fs_unwindDirs( void );

void mk_main_init( int argc, char **argv );

enum {
	kMkFlag_ShowHelp_Bit        = 0x01,
	kMkFlag_ShowVersion_Bit     = 0x02,
	kMkFlag_Verbose_Bit         = 0x04,
	kMkFlag_Release_Bit         = 0x08,
	kMkFlag_Rebuild_Bit         = 0x10,
	kMkFlag_NoCompile_Bit       = 0x20,
	kMkFlag_NoLink_Bit          = 0x40,
	kMkFlag_LightClean_Bit      = 0x80,
	kMkFlag_PrintHierarchy_Bit  = 0x100,
	kMkFlag_Pedantic_Bit        = 0x200,
	kMkFlag_Test_Bit            = 0x400,
	kMkFlag_FullClean_Bit       = 0x800,
	kMkFlag_OutSingleThread_Bit = 0x1000
};
extern bitfield_t mk__g_flags;
extern MkColorMode_t mk__g_flags_color;

extern MkStrList mk__g_targets;
extern MkStrList mk__g_srcdirs;
extern MkStrList mk__g_incdirs;
extern MkStrList mk__g_libdirs;
extern MkStrList mk__g_pkgdirs;
extern MkStrList mk__g_tooldirs;
extern MkStrList mk__g_dllsdirs;
