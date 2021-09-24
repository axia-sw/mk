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

#include "mk-basic-stringList.h"
#include "mk-basic-types.h"
#include "mk-build-library.h"
#include "mk-build-platform.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"

#include <stddef.h>

/*
 *	========================================================================
 *	PROJECT MANAGEMENT
 *	========================================================================
 */

#if !MK_DEBUG_ENABLED
#	undef MK_DEBUG_LIBDEPS_ENABLED
#	define MK_DEBUG_LIBDEPS_ENABLED 0
#else
#	ifndef MK_DEBUG_LIBDEPS_ENABLED
#		define MK_DEBUG_LIBDEPS_ENABLED 1
#	endif
#endif

typedef struct MkProject_s *MkProject;

typedef enum {
	/* static library (lib<blah>.a) */
	kMkProjTy_StaticLib,
	/* dynamic library (<blah>.dll) */
	kMkProjTy_DynamicLib,
	/* (console) executable (<blah>.exe) */
	kMkProjTy_Program,
	/* (gui) executable (<blah>.exe) -- gets console in debug mode */
	kMkProjTy_Application
} MkProjectType_t;

static const struct {
	const char *const name;
	const int type;
} mk__g_ifiles[] = {
	{ ".library", kMkProjTy_StaticLib },
	{ ".mk-library", kMkProjTy_StaticLib },
	{ ".dynamiclibrary", kMkProjTy_DynamicLib },
	{ ".mk-dynamiclibrary", kMkProjTy_DynamicLib },
	{ ".executable", kMkProjTy_Program },
	{ ".mk-executable", kMkProjTy_Program },
	{ ".application", kMkProjTy_Application },
	{ ".mk-application", kMkProjTy_Application },

	/* ".txt" indicator files for easier creation on Windows - notkyon:160204 */
	{ "mk-staticlib.txt", kMkProjTy_StaticLib },
	{ "mk-dynamiclib.txt", kMkProjTy_DynamicLib },
	{ "mk-executable.txt", kMkProjTy_Program },
	{ "mk-application.txt", kMkProjTy_Application }
};

enum {
	kMkProjCfg_UsesCxx_Bit    = 0x01,
	kMkProjCfg_NeedRelink_Bit = 0x02,
	kMkProjCfg_Linking_Bit    = 0x04,
	kMkProjCfg_Package_Bit    = 0x08
};
enum {
	kMkProjStat_LibFlags_Bit = 0x01,
	kMkProjStat_CalcDeps_Bit = 0x02
};

struct MkProject_s {
	char *name;
	char *path;
	char *outpath;
	char *binname;
	int type;
	int sys;
	int arch;

	MkStrList defs;

	MkStrList sources;
	MkStrList specialdirs;
	MkStrList libs;

	MkStrList testsources;

	MkStrList srcdirs; /* needed for determining object paths */

	char *linkerflags;
	char *extralibs;

	char *deplibsfilename;

	bitfield_t config;
	bitfield_t status;

	MkLib lib;

	struct MkProject_s *prnt, **p_head, **p_tail;
	struct MkProject_s *head, *tail;
	struct MkProject_s *prev, *next;

	struct MkProject_s *lib_prev, *lib_next;
};

extern struct MkProject_s *mk__g_proj_head;
extern struct MkProject_s *mk__g_proj_tail;
extern struct MkProject_s *mk__g_lib_proj_head;
extern struct MkProject_s *mk__g_lib_proj_tail;

MkProject mk_prj_new( MkProject prnt );
void      mk_prj_delete( MkProject proj );
void      mk_prj_deleteAll( void );

MkProject mk_prj_rootHead( void );
MkProject mk_prj_rootTail( void );
MkProject mk_prj_getParent( MkProject proj );
MkProject mk_prj_head( MkProject proj );
MkProject mk_prj_tail( MkProject proj );
MkProject mk_prj_prev( MkProject proj );
MkProject mk_prj_next( MkProject proj );

MkProject mk_prj_find_r( MkProject from, const char *name );
MkProject mk_prj_find( const char *name );

void        mk_prj_setName( MkProject proj, const char *name );
void        mk_prj_setOutPath( MkProject proj, const char *path );
void        mk_prj_setPath( MkProject proj, const char *path );
void        mk_prj_setType( MkProject proj, int type );
const char *mk_prj_getName( MkProject proj );
const char *mk_prj_getPath( MkProject proj );
const char *mk_prj_getOutPath( MkProject proj );
int         mk_prj_getType( MkProject proj );

void        mk_prj_addLib( MkProject proj, const char *libname );
size_t      mk_prj_numLibs( MkProject proj );
const char *mk_prj_libAt( MkProject proj, size_t i );
void        mk_prj_addLinkFlags( MkProject proj, const char *flags );
const char *mk_prj_getLinkFlags( MkProject proj );

void        mk_prj_appendExtraLibs( MkProject proj, const char *extras );
const char *mk_prj_getExtraLibs( MkProject proj );
void        mk_prj_completeExtraLibs( MkProject proj, char *extras, size_t n );

void        mk_prj_addSourceFile( MkProject proj, const char *src );
size_t      mk_prj_numSourceFiles( MkProject proj );
const char *mk_prj_sourceFileAt( MkProject proj, size_t i );
void        mk_prj_addTestSourceFile( MkProject proj, const char *src );
size_t      mk_prj_numTestSourceFiles( MkProject proj );
const char *mk_prj_testSourceFileAt( MkProject proj, size_t i );

void        mk_prj_addSpecialDir( MkProject proj, const char *dir );
size_t      mk_prj_numSpecialDirs( MkProject proj );
const char *mk_prj_specialDirAt( MkProject proj, size_t i );

int  mk_prj_isTarget( MkProject proj );
void mk_prj_printAll( MkProject proj, const char *margin );
void mk_prj_calcDeps( MkProject proj );
void mk_prj_calcLibFlags( MkProject proj );
