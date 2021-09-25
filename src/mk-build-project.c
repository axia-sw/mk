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
#include "mk-build-project.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-logging.h"
#include "mk-basic-options.h"
#include "mk-basic-stringList.h"
#include "mk-basic-types.h"
#include "mk-build-engine.h"
#include "mk-build-library.h"
#include "mk-frontend.h"

#include <stdio.h>
#include <string.h>

struct MkProject_s *mk__g_proj_head     = (struct MkProject_s *)0;
struct MkProject_s *mk__g_proj_tail     = (struct MkProject_s *)0;
struct MkProject_s *mk__g_lib_proj_head = (struct MkProject_s *)0;
struct MkProject_s *mk__g_lib_proj_tail = (struct MkProject_s *)0;

/* create a new (empty) project */
MkProject mk_prj_new( MkProject prnt ) {
	MkProject proj;

	proj = (MkProject)mk_com_memory( (void *)0, sizeof( *proj ) );

	proj->name    = (char *)0;
	proj->path    = (char *)0;
	proj->outpath = (char *)0;
	proj->binname = (char *)0;

	proj->type = kMkProjTy_Program;
	proj->sys  = mk__g_hostOS;
	proj->arch = mk__g_hostCPU;

	proj->defs = mk_sl_new();

	proj->sources     = mk_sl_new();
	proj->specialdirs = mk_sl_new();
	proj->libs        = mk_sl_new();

	proj->testsources = mk_sl_new();

	proj->srcdirs = mk_sl_new();

	proj->linkerflags = (char *)0;
	proj->extralibs   = (char *)0;

	proj->deplibsfilename = (char *)0;

	proj->config = 0;
	proj->status = 0;

	proj->lib = (MkLib)0;

	proj->prnt   = prnt;
	proj->p_head = prnt ? &prnt->head : &mk__g_proj_head;
	proj->p_tail = prnt ? &prnt->tail : &mk__g_proj_tail;
	proj->head   = (struct MkProject_s *)0;
	proj->tail   = (struct MkProject_s *)0;
	proj->next   = (struct MkProject_s *)0;
	if( ( proj->prev = *proj->p_tail ) != (struct MkProject_s *)0 ) {
		( *proj->p_tail )->next = proj;
	} else {
		*proj->p_head = proj;
	}
	*proj->p_tail = proj;

	proj->lib_prev = (struct MkProject_s *)0;
	proj->lib_next = (struct MkProject_s *)0;

	return proj;
}

/* delete an existing project */
void mk_prj_delete( MkProject proj ) {
	if( !proj ) {
		return;
	}

	while( proj->head ) {
		mk_prj_delete( proj->head );
	}

	proj->name    = (char *)mk_com_memory( (void *)proj->name, 0 );
	proj->path    = (char *)mk_com_memory( (void *)proj->path, 0 );
	proj->outpath = (char *)mk_com_memory( (void *)proj->outpath, 0 );
	proj->binname = (char *)mk_com_memory( (void *)proj->binname, 0 );

	mk_sl_delete( proj->defs );

	mk_sl_delete( proj->sources );
	mk_sl_delete( proj->specialdirs );
	mk_sl_delete( proj->libs );

	mk_sl_delete( proj->testsources );

	mk_sl_delete( proj->srcdirs );

	proj->linkerflags = (char *)mk_com_memory( (void *)proj->linkerflags, 0 );
	proj->extralibs   = (char *)mk_com_memory( (void *)proj->extralibs, 0 );

	proj->deplibsfilename = (char *)mk_com_memory( (void *)proj->deplibsfilename, 0 );

	if( proj->lib ) {
		mk_lib_delete( proj->lib );
	}

	if( proj->prev ) {
		proj->prev->next = proj->next;
	}
	if( proj->next ) {
		proj->next->prev = proj->prev;
	}

	MK_ASSERT( proj->p_head != (struct MkProject_s **)0 );
	MK_ASSERT( proj->p_tail != (struct MkProject_s **)0 );

	if( *proj->p_head == proj ) {
		*proj->p_head = proj->next;
	}
	if( *proj->p_tail == proj ) {
		*proj->p_tail = proj->prev;
	}

	if( proj->type == kMkProjTy_StaticLib ) {
		if( proj->lib_prev ) {
			proj->lib_prev->lib_next = proj->lib_next;
		} else {
			mk__g_lib_proj_head = proj->lib_next;
		}

		if( proj->lib_next ) {
			proj->lib_next->lib_prev = proj->lib_prev;
		} else {
			mk__g_lib_proj_tail = proj->lib_prev;
		}
	}

	mk_com_memory( (void *)proj, 0 );
}

/* delete all projects */
void mk_prj_deleteAll( void ) {
	while( mk__g_proj_head )
		mk_prj_delete( mk__g_proj_head );
}

/* retrieve the first root project */
MkProject mk_prj_rootHead( void ) {
	return mk__g_proj_head;
}

/* retrieve the last root project */
MkProject mk_prj_rootTail( void ) {
	return mk__g_proj_tail;
}

/* retrieve the parent of a project */
MkProject mk_prj_getParent( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->prnt;
}

/* retrieve the first child project of the given project */
MkProject mk_prj_head( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->head;
}

/* retrieve the last child project of the given project */
MkProject mk_prj_tail( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->tail;
}

/* retrieve the sibling project before the given project */
MkProject mk_prj_prev( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->prev;
}

/* retrieve the sibling project after the given project */
MkProject mk_prj_next( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->next;
}

/* set the name of a project */
void mk_prj_setName( MkProject proj, const char *name ) {
	const char *p;
	char tmp[PATH_MAX];

	MK_ASSERT( proj != (MkProject)0 );

	tmp[0] = 0;
	p      = (const char *)0;

	if( name ) {
		if( !( p = strrchr( name, '/' ) ) )
			p = strrchr( name, '\\' );

		if( p ) {
			/* can't end in "/" */
			if( *( p + 1 ) == 0 ) {
				mk_log_fatalError( mk_com_va( "invalid project name \"%s\"", name ) );
			}

			mk_com_strncpy( tmp, sizeof( tmp ), name, ( p - name ) + 1 );
			p++;
		} else {
			p = name;
		}
	}

	MK_ASSERT( p != (const char *)0 );

	proj->outpath = mk_com_dup( proj->outpath, tmp[0] ? tmp : (const char *)0 );
	proj->name    = mk_com_dup( proj->name, p );

	proj->deplibsfilename = mk_com_dup( proj->deplibsfilename,
	    mk_com_va( "%s/%s/%s.deplibs",
	        mk_opt_getObjdirBase(),
	        mk_opt_getConfigName(),
	        proj->name ) );
}

/* set the output path of a project */
void mk_prj_setOutPath( MkProject proj, const char *path ) {
	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( path != (const char *)0 );

	proj->outpath = mk_com_dup( proj->outpath, path );
}

/* set the path of a project */
void mk_prj_setPath( MkProject proj, const char *path ) {
	MK_ASSERT( proj != (MkProject)0 );

	proj->path = mk_com_dup( proj->path, path );
}

/* set the type of a project (e.g., kMkProjTy_StaticLib) */
void mk_prj_setType( MkProject proj, int type ) {
	MK_ASSERT( proj != (MkProject)0 );

	if( !proj->outpath ) {
		static const char *const pszlibdir = "lib/" MK_PLATFORM_DIR;
		static const char *const pszbindir = "bin/" MK_PLATFORM_DIR;

		proj->outpath = mk_com_dup( proj->outpath, mk_com_va( "%s/", type == kMkProjTy_StaticLib ? pszlibdir : pszbindir ) );
		if( type == kMkProjTy_StaticLib && !mk_sl_getSize( mk__g_libdirs ) ) {
			mk_front_pushLibDir( pszlibdir );
		}
	}

	if( proj->type == kMkProjTy_StaticLib ) {
		if( proj->lib_prev ) {
			proj->lib_prev->lib_next = proj->lib_next;
		} else {
			mk__g_lib_proj_head = proj->lib_next;
		}

		if( proj->lib_next ) {
			proj->lib_next->lib_prev = proj->lib_prev;
		} else {
			mk__g_lib_proj_tail = proj->lib_prev;
		}
	}

	proj->type = type;

	if( proj->type == kMkProjTy_StaticLib ) {
		proj->lib_next = (struct MkProject_s *)0;
		if( ( proj->lib_prev = mk__g_lib_proj_tail ) != (struct MkProject_s *)0 ) {
			mk__g_lib_proj_tail->lib_next = proj;
		} else {
			mk__g_lib_proj_head = proj;
		}
		mk__g_lib_proj_tail = proj;
	}
}

/* retrieve the (non-modifyable) name of a project */
const char *mk_prj_getName( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->name;
}

/* retrieve the (non-modifyable) path of a project */
const char *mk_prj_getPath( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->path;
}

/* retrieve the output path of a project */
const char *mk_prj_getOutPath( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->outpath;
}

/* retrieve the type of a project (e.g., kMkProjTy_Program) */
int mk_prj_getType( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->type;
}

/* add a library to the project */
void mk_prj_addLib( MkProject proj, const char *libname ) {
	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( libname != (const char *)0 );

	mk_sl_pushBack( proj->libs, libname );
}

/* retrieve the number of libraries in a project */
size_t mk_prj_numLibs( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return mk_sl_getSize( proj->libs );
}

/* retrieve a library of a project */
const char *mk_prj_libAt( MkProject proj, size_t i ) {
	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( i < mk_sl_getSize( proj->libs ) );

	return mk_sl_at( proj->libs, i );
}

/* append a linker flag to the project */
void mk_prj_addLinkFlags( MkProject proj, const char *flags ) {
	size_t l;

	MK_ASSERT( proj != (MkProject)0 );
	/* MK_ASSERT( flags != (const char *)0 ); :: this happens due to built-in libs */
	if( !flags ) {
		return;
	}

	l = proj->linkerflags ? proj->linkerflags[0] != 0 : 0;

	if( l ) {
		proj->linkerflags = mk_com_append( proj->linkerflags, " " );
	}

	proj->linkerflags = mk_com_append( proj->linkerflags, flags );
}

/* retrieve the current linker flags of a project */
const char *mk_prj_getLinkFlags( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->linkerflags ? proj->linkerflags : "";
}

/* append "extra libs" to the project (found in ".libs" files) */
void mk_prj_appendExtraLibs( MkProject proj, const char *extras ) {
	size_t l;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( extras != (const char *)0 );

	l = proj->extralibs ? proj->extralibs[0] != 0 : 0;

	if( l ) {
		proj->extralibs = mk_com_append( proj->extralibs, " " );
	}

	proj->extralibs = mk_com_append( proj->extralibs, extras );
}

/* retrieve the "extra libs" of a project */
const char *mk_prj_getExtraLibs( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	return proj->extralibs ? proj->extralibs : "";
}

/* retrieve all "extra libs" of a project (includes project children) */
void mk_prj_completeExtraLibs( MkProject proj, char *extras, size_t n ) {
	MkProject x;

	for( x = mk_prj_head( proj ); x; x = mk_prj_next( x ) ) {
		mk_prj_completeExtraLibs( x, extras, n );
	}

	mk_com_strcat( extras, n, mk_prj_getExtraLibs( proj ) );
	mk_com_strcat( extras, n, " " );
}

/* add a source file to a project */
void mk_prj_addSourceFile( MkProject proj, const char *src ) {
	const char *p;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( src != (const char *)0 );

	MK_ASSERT( proj->sources != (MkStrList)0 );

	if( ( p = strrchr( src, '.' ) ) != (const char *)0 ) {
		if( !strcmp( p, ".cc" ) || !strcmp( p, ".cxx" ) || !strcmp( p, ".cpp" ) || !strcmp( p, ".c++" ) ) {
			proj->config |= kMkProjCfg_UsesCxx_Bit;
		} else if( !strcmp( p, ".mm" ) ) {
			proj->config |= kMkProjCfg_UsesCxx_Bit;
		}
	}

	mk_sl_pushBack( proj->sources, src );

	p = strrchr( src, '/' );
	if( !p ) {
		p = strrchr( src, '\\' );
	}

	if( p ) {
		char temp[PATH_MAX], rel[PATH_MAX];

		mk_com_strncpy( temp, sizeof( temp ), src, p - src );
		temp[p - src] = '\0';

		mk_com_relPath( rel, sizeof( rel ), proj->path, temp );

		mk_sl_pushBack( proj->srcdirs, rel );
	}
}

/* retrieve the number of source files within a project */
size_t mk_prj_numSourceFiles( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	MK_ASSERT( proj->sources != (MkStrList)0 );

	return mk_sl_getSize( proj->sources );
}

/* retrieve a source file of a project */
const char *mk_prj_sourceFileAt( MkProject proj, size_t i ) {
	MK_ASSERT( proj != (MkProject)0 );

	MK_ASSERT( proj->sources != (MkStrList)0 );

	return mk_sl_at( proj->sources, i );
}

/* add a test source file to a project */
void mk_prj_addTestSourceFile( MkProject proj, const char *src ) {
	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( src != (const char *)0 );

	MK_ASSERT( proj->testsources != (MkStrList)0 );

	/*
	if( (p = strrchr(src, '.')) != (const char *)0 ) {
		if( !strcmp(p, ".cc") || !strcmp(p, ".cxx") || !strcmp(p, ".cpp" )
		 || !strcmp(p, ".c++"))
			proj->config |= kMkProjCfg_UsesCxx_Bit;
		else if(!strcmp(p, ".mm"))
			proj->config |= kMkProjCfg_UsesCxx_Bit;
	}
	*/

	mk_sl_pushBack( proj->testsources, src );
}

/* retrieve the number of test source files within a project */
size_t mk_prj_numTestSourceFiles( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	MK_ASSERT( proj->sources != (MkStrList)0 );

	return mk_sl_getSize( proj->testsources );
}

/* retrieve a test source file of a project */
const char *mk_prj_testSourceFileAt( MkProject proj, size_t i ) {
	MK_ASSERT( proj != (MkProject)0 );

	MK_ASSERT( proj->sources != (MkStrList)0 );

	return mk_sl_at( proj->testsources, i );
}

/* add a "special directory" to a project */
void mk_prj_addSpecialDir( MkProject proj, const char *dir ) {
	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( dir != (const char *)0 );

	MK_ASSERT( proj->specialdirs != (MkStrList)0 );

	mk_sl_pushBack( proj->specialdirs, dir );
}

/* retrieve the number of "special directories" within a project */
size_t mk_prj_numSpecialDirs( MkProject proj ) {
	MK_ASSERT( proj != (MkProject)0 );

	MK_ASSERT( proj->specialdirs != (MkStrList)0 );

	return mk_sl_getSize( proj->specialdirs );
}

/* retrieve a "special directory" of a project */
const char *mk_prj_specialDirAt( MkProject proj, size_t i ) {
	MK_ASSERT( proj != (MkProject)0 );

	MK_ASSERT( proj->specialdirs != (MkStrList)0 );

	return mk_sl_at( proj->specialdirs, i );
}

/* determine if a given project is targetted */
int mk_prj_isTarget( MkProject proj ) {
	size_t i, n;

	if( !( n = mk_sl_getSize( mk__g_targets ) ) ) {
		return 1;
	}

	for( i = 0; i < n; i++ ) {
		if( !strcmp( mk_prj_getName( proj ), mk_sl_at( mk__g_targets, i ) ) ) {
			return 1;
		}
	}

	return 0;
}

/* display a tree of projects to stdout */
void mk_prj_printAll( MkProject proj, const char *margin ) {
	size_t i, n;
	char marginbuf[256];
	char bin[256];

	MK_ASSERT( margin != (const char *)0 );

	if( !proj ) {
		for( proj = mk_prj_rootHead(); proj; proj = mk_prj_next( proj ) ) {
			mk_prj_printAll( proj, margin );
		}

		return;
	}

	if( mk_prj_isTarget( proj ) ) {
		mk_bld_getBinName( proj, bin, sizeof( bin ) );

		printf( "%s%s; \"%s\"\n", margin, mk_prj_getName( proj ), bin );

		n = mk_prj_numSourceFiles( proj );
		for( i = 0; i < n; i++ ) {
			printf( "%s * %s\n", margin, mk_prj_sourceFileAt( proj, i ) );
		}

		n = mk_prj_numTestSourceFiles( proj );
		for( i = 0; i < n; i++ ) {
			printf( "%s # %s\n", margin, mk_prj_testSourceFileAt( proj, i ) );
		}
	}

	mk_com_strcpy( marginbuf, sizeof( marginbuf ), margin );
	mk_com_strcat( marginbuf, sizeof( marginbuf ), "  " );
	for( proj = mk_prj_head( proj ); proj; proj = mk_prj_next( proj ) ) {
		mk_prj_printAll( proj, marginbuf );
	}
}

/* expand library dependencies */
static void mk_prj__expandLibDeps_r( MkProject proj ) {
	const char *libname;
	MkProject libproj;
	size_t i, j, n, m;
	MkLib lib;

	do {
		n = mk_sl_getSize( proj->libs );
		for( i = 0; i < n; ++i ) {
			if( !( libname = mk_sl_at( proj->libs, i ) ) ) {
				continue;
			}

			if( !( lib = mk_lib_find( libname ) ) ) {
				mk_log_errorMsg( mk_com_va( "couldn't find library ^E\"%s\"^&",
				    libname ) );
				continue;
			}

			if( !lib->proj ) {
				continue;
			}

			libproj   = lib->proj;
			lib->proj = (MkProject)0; /*prevent infinite recursion*/
			mk_prj__expandLibDeps_r( libproj );
			lib->proj = libproj; /*restore*/

			m = mk_sl_getSize( libproj->libs );
			for( j = 0; j < m; ++j ) {
				mk_sl_pushBack( proj->libs, mk_sl_at( libproj->libs, j ) );
			}
		}

		/* optimization -- if there were no changes to the working set then
		 * don't spend time looking for unique entries */
		if( mk_sl_getSize( proj->libs ) == n ) {
			break;
		}

		mk_sl_makeUnique( proj->libs );
	} while( mk_sl_getSize( proj->libs ) != n ); /* mk_sl_makeUnique can alter count */
}
static void mk_prj__saveLibDeps( MkProject proj ) {
	const char *p;
	size_t i, n, len;
	FILE *fp;

	MK_ASSERT( proj->deplibsfilename != (char *)0 );

	fp = fopen( proj->deplibsfilename, "wb" );
	if( !fp ) {
		mk_log_errorMsg( mk_com_va( "failed to write ^E\"%s\"^&",
		    proj->deplibsfilename ) );
	}

	fprintf( fp, "DEPS1.00" );

	n = mk_sl_getSize( proj->libs );
	fwrite( &n, sizeof( n ), 1, fp );
	for( i = 0; i < n; ++i ) {
		p = mk_sl_at( proj->libs, i );
		if( !p ) {
			p = "";
		}

		len = strlen( p );

		fwrite( &len, sizeof( len ), 1, fp );
		fwrite( (const void *)p, len, 1, fp );
	}

	fclose( fp );

#if MK_DEBUG_LIBDEPS_ENABLED
	mk_dbg_outf( "libdeps: saved \"%s\"\n", proj->deplibsfilename );
#endif
}
static void mk_prj__loadLibDeps( MkProject proj ) {
	size_t i, n, temp;
	FILE *fp;
	char buf[512];

	MK_ASSERT( proj->deplibsfilename != (char *)0 );

#if MK_DEBUG_LIBDEPS_ENABLED
	mk_dbg_outf( "libdeps: opening \"%s\"...\n", proj->deplibsfilename );
#endif

	fp = fopen( proj->deplibsfilename, "rb" );
	if( !fp ) {
#if MK_DEBUG_LIBDEPS_ENABLED
		mk_dbg_outf( "libdeps: failed to open\n" );
#endif
		return;
	}

	if( !fread( &buf[0], 8, 1, fp ) ) {
		fclose( fp );
#if MK_DEBUG_LIBDEPS_ENABLED
		mk_dbg_outf( "libdeps: failed; no header\n" );
#endif
		return;
	}
	buf[8] = '\0';
	if( strcmp( buf, "DEPS1.00" ) != 0 ) {
		fclose( fp );
#if MK_DEBUG_LIBDEPS_ENABLED
		mk_dbg_outf( "libdeps: failed; invalid header "
		             "[%.2X,%.2X,%.2X,%.2X,%.2X,%.2X,%.2X,%.2X]{\"%.8s\"}\n",
		    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
		    buf );
#endif
		return;
	}

	if( !fread( &n, sizeof( n ), 1, fp ) ) {
		fclose( fp );
#if MK_DEBUG_LIBDEPS_ENABLED
		mk_dbg_outf( "libdeps: failed; no item count\n" );
#endif
		return;
	}

	for( i = 0; i < n; ++i ) {
		if( !fread( &temp, sizeof( temp ), 1, fp ) ) {
			fclose( fp );
#if MK_DEBUG_LIBDEPS_ENABLED
			mk_dbg_outf( "libdeps: failed; item %u -- missing length\n",
			    (unsigned int)i );
#endif
			return;
		}

		if( temp >= sizeof( buf ) ) {
			fclose( fp );
#if MK_DEBUG_LIBDEPS_ENABLED
			mk_dbg_outf( "libdeps: failed; item %u is too long (%u >= %u)\n",
			    (unsigned int)i, (unsigned int)temp,
			    (unsigned int)sizeof( buf ) );
#endif
			while( i > 0 ) {
				mk_sl_popBack( proj->libs );
				--i;
			}
			return;
		}

		memset( &buf[0], 0, sizeof( buf ) );
		if( !fread( &buf[0], temp, 1, fp ) ) {
			fclose( fp );
#if MK_DEBUG_LIBDEPS_ENABLED
			mk_dbg_outf( "libdeps: failed; item %u -- missing data\n",
			    (unsigned int)i );
#endif
			while( i > 0 ) {
				mk_sl_popBack( proj->libs );
				--i;
			}
			return;
		}

		mk_sl_pushBack( proj->libs, buf );
	}

	fclose( fp );

#if MK_DEBUG_LIBDEPS_ENABLED
	mk_dbg_outf( "libdeps: succeeded\n" );
#endif

	mk_sl_makeUnique( proj->libs );

#if MK_DEBUG_LIBDEPS_ENABLED
	mk_dbg_enter( "libdeps-project(\"%s\")", proj->name );
	n = mk_sl_getSize( proj->libs );
	for( i = 0; i < n; ++i ) {
		mk_dbg_outf( "\"%s\"\n", mk_sl_at( proj->libs, i ) );
	}
	mk_dbg_leave();
#endif
}

void mk_prj_calcDeps( MkProject proj ) {
	if( proj->status & kMkProjStat_CalcDeps_Bit ) {
		return;
	}
	proj->status |= kMkProjStat_CalcDeps_Bit;

	/* FIXME: requires rebuild if a library is removed */
	mk_prj__loadLibDeps( proj );
	/* end of fixme */

	mk_prj__expandLibDeps_r( proj );
	mk_prj__saveLibDeps( proj );
}

/* given an array of library names, return a string of flags */
void mk_prj_calcLibFlags( MkProject proj ) {
	static char flags[32768];
	const char *libname;
	size_t i, n;
	MkLib lib;

	if( proj->status & kMkProjStat_LibFlags_Bit ) {
		return;
	}

	proj->status |= kMkProjStat_LibFlags_Bit;

	/*
	 * TODO: This would be a good place to parse the extra libs of the project
	 *       to remove any "!blah" linker flags. Also, it would be good to check
	 *       if the extra libs is referring to a Mk-known library (e.g.,
	 *       "opengl"), or a command-line setting as well (e.g., "-lGL") as the
	 *       latter is not very platform friendly. Elsewhere, conditionals could
	 *       be checked too. (e.g., "mswin:-lopengl32 linux:-lGL" etc...)
	 *       For now, the current system (just extra linker flags) will work.
	 */

	mk_prj_calcDeps( proj );

	/* find libraries */
	n = mk_sl_getSize( proj->libs );
	for( i = 0; i < n; i++ ) {
		if( !( libname = mk_sl_at( proj->libs, i ) ) ) {
			continue;
		}

		if( !( lib = mk_lib_find( libname ) ) ) {
			/*
			mk_log_errorMsg(mk_com_va("couldn't find library ^E\"%s\"^&", libname));
			*/
			continue;
		}

#if MK_DEBUG_LIBDEPS_ENABLED
		mk_dbg_outf( "libdeps: \"%s\" <- \"%s\"\n", proj->name, libname );
#endif

		/*
		 * NOTE: Projects are managed as dependencies; not as linker flags
		 */
		if( lib->proj ) {
			continue;
		}

		mk_prj_addLinkFlags( proj, mk_lib_getFlags( lib, proj->sys ) );
	}

	/* add these flags */
	mk_prj_completeExtraLibs( proj, flags, sizeof( flags ) );
	mk_prj_addLinkFlags( proj, flags );
}

/* find a project by name */
MkProject mk_prj_find_r( MkProject from, const char *name ) {
	MkProject p, r;

	MK_ASSERT( from != (MkProject)0 );
	MK_ASSERT( name != (const char *)0 );

	if( !strcmp( mk_prj_getName( from ), name ) ) {
		return from;
	}

	for( p = mk_prj_head( from ); p; p = mk_prj_next( p ) ) {
		r = mk_prj_find_r( p, name );
		if( r != (MkProject)0 ) {
			return r;
		}
	}

	return (MkProject)0;
}
MkProject mk_prj_find( const char *name ) {
	MkProject p, r;

	MK_ASSERT( name != (const char *)0 );

	for( p = mk_prj_rootHead(); p; p = mk_prj_next( p ) ) {
		r = mk_prj_find_r( p, name );
		if( r != (MkProject)0 ) {
			return r;
		}
	}

	return (MkProject)0;
}
