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

typedef enum {
	/*

		$ mk help
		$ mk -h         $ mk --help

			Show the default help prompt.

		$ mk help [command]

			Show help related to a given command.

	*/
	kMkAction_ShowHelp,
	/*

		$ mk version
		$ mk -v         $ mk --version

			Show the current Mk version.

	*/
	kMkAction_ShowVersion,
	/*

		$ mk
		$ mk build      $ mk build [all:]debug

			Build all targets in debug mode.

		$ mk build [all:]release

			Build all targets in release mode.

		$ mk build target-name:config-name
		$ mk build --target=[target-name] --config=[config-name]

			Build a given target for a given configuration.

	*/
	kMkAction_BuildTarget,
	/*

		$ mk clean
		$ mk -C         $ mk --clean

			Clean all targets.

		$ mk clean target-name[:config-name]
		$ mk clean --target=[target-name] --config=[config-name]

			Cleans a specific target (including all configs if no config-name specified).
	
	*/
	kMkAction_CleanTarget,
	/*

		$ mk test
		$ mk -T         $ mk --test

			Runs all unit tests on all targets.

		$ mk test target-name
		$ mk test --target=target-name

			Runs unit tests on the specific target.

	*/
	kMkAction_TestTarget,
} MkActionType_t;

typedef struct MkAction_s {
	MkActionType_t type;

	int argc;
	const char **argv;
} MkAction;

typedef struct MkActions_s {
	size_t len;
	MkAction *ptr;
} MkActions;

void mk_front_pushSrcDir( const char *srcdir );
void mk_front_pushIncDir( const char *incdir );
void mk_front_pushLibDir( const char *libdir );
void mk_front_pushPkgDir( const char *pkgdir );
void mk_front_pushToolDir( const char *tooldir );
void mk_front_pushDynamicLibsDir( const char *dllsdir );

void mk_fs_unwindDirs( void );

void mk_main_init( int argc, char **argv );
void mk_main_fini( void );

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

extern MkActions mk__g_actions;
