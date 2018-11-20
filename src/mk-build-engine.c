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
#include "mk-build-engine.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-logging.h"
#include "mk-basic-options.h"
#include "mk-basic-stringList.h"
#include "mk-basic-types.h"
#include "mk-build-autolib.h"
#include "mk-build-dependency.h"
#include "mk-build-library.h"
#include "mk-build-makefileDependency.h"
#include "mk-build-project.h"
#include "mk-build-projectFS.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"
#include "mk-frontend.h"
#include "mk-system-output.h"
#include "mk-util-git.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

MkStrList mk__g_unitTestCompiles = (MkStrList)0;
MkStrList mk__g_unitTestRuns     = (MkStrList)0;
MkStrList mk__g_unitTestNames    = (MkStrList)0;

/* initialize unit test arrays */
void mk_bld_initUnitTestArrays( void ) {
	mk__g_unitTestCompiles = mk_sl_new();
	mk__g_unitTestRuns     = mk_sl_new();
	mk__g_unitTestNames    = mk_sl_new();
}

/* find which libraries are to be autolinked from a source file */
int mk_bld_findSourceLibs( MkStrList dst, int sys, const char *obj, const char *dep ) {
	MkAutolink al;
	size_t i, n;
	MkDep d;
	MkLib l;

#if MK_DEBUG_FIND_SOURCE_LIBS_ENABLED
	mk_dbg_outf( "mk_bld_findSourceLibs: \"%s\", \"%s\"\n", obj, dep );
#endif

	d = mk_dep_find( obj );
	if( !d ) {
		if( !mk_mfdep_load( dep ) ) {
			mk_log_errorMsg( mk_com_va( "failed to read dependency ^F\"%s\"^&", dep ) );
			return 0;
		}

		d = mk_dep_find( obj );
		if( !d ) {
			mk_log_errorMsg( mk_com_va( "mk_dep_find(^F\"%s\"^&) failed", obj ) );
			mk_dep_debugPrintAll();
			return 0;
		}
	}

	n = mk_dep_getSize( d );
	for( i = 0; i < n; i++ ) {
		if( !( al = mk_al_find( sys, mk_dep_at( d, i ) ) ) ) {
			continue;
		}

#if MK_DEBUG_FIND_SOURCE_LIBS_ENABLED
		mk_dbg_outf( "  found dependency on \"%s\"; investigating\n", al->lib );
#endif

		l = mk_lib_find( al->lib );
		if( !l ) {
#if MK_DEBUG_FIND_SOURCE_LIBS_ENABLED
			mk_dbg_outf( "   -ignoring because did not find associated lib\n" );
#endif
			continue;
		}

		/* a project does not have a dependency on itself (for linking
		 * purposes); ignore */
		if( l->proj && l->proj->libs == dst ) {
#if MK_DEBUG_FIND_SOURCE_LIBS_ENABLED
			mk_dbg_outf( "   -ignoring because is current project\n" );
#endif
			continue;
		}

		mk_sl_pushBack( dst, mk_al_getLib( al ) );
#if MK_DEBUG_FIND_SOURCE_LIBS_ENABLED
		mk_dbg_outf( "   +keeping\n", mk_al_getLib( al ) );
#endif
	}

	mk_sl_makeUnique( dst );
	return 1;
}

/* determine whether a source file should be built */
int mk_bld_shouldCompile( const char *obj ) {
	MkStat_t s, obj_s;
	size_t i, n;
	MkDep d;
	char dep[PATH_MAX];

	MK_ASSERT( obj != (const char *)0 );

	if( mk__g_flags & kMkFlag_Rebuild_Bit ) {
		return 1;
	}
	if( mk__g_flags & kMkFlag_NoCompile_Bit ) {
		return 0;
	}

	if( stat( obj, &obj_s ) == -1 ) {
		return 1;
	}

	mk_com_substExt( dep, sizeof( dep ), obj, ".d" );

	if( stat( dep, &s ) == -1 ) {
		return 1;
	}

	d = mk_dep_find( obj );
	if( !d ) {
		if( !mk_mfdep_load( dep ) ) {
			return 1;
		}

		d = mk_dep_find( obj );
		if( !d ) {
			return 1;
		}
	}

	n = mk_dep_getSize( d );
	for( i = 0; i < n; i++ ) {
		if( stat( mk_dep_at( d, i ), &s ) == -1 ) {
			return 1; /* need recompile for new dependency list; this file is
			             (potentially) missing */
		}

		if( obj_s.st_mtime <= s.st_mtime ) {
			return 1;
		}
	}

	return 0; /* no reason to rebuild */
}

/* determine whether a project should be linked */
int mk_bld_shouldLink( const char *bin, int numbuilds ) {
	MkStat_t bin_s;

	MK_ASSERT( bin != (const char *)0 );

	if( mk__g_flags & kMkFlag_Rebuild_Bit ) {
		return 1;
	}
	if( mk__g_flags & kMkFlag_NoLink_Bit ) {
		return 0;
	}

	if( stat( bin, &bin_s ) == -1 ) {
		return 1;
	}

	if( numbuilds != 0 ) {
		return 1;
	}

	return 0;
}

/* retrieve the compiler to use */
const char *mk_bld_getCompiler( int iscxx ) {
	static char cc[128], cxx[128];
	static int didinit = 0;

	/*
	 *	TODO: Check command-line option
	 */

	if( !didinit ) {
		const char *p;
		didinit = 1;

		p = getenv( "CC" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( cc, sizeof( cc ), p );
		} else {
#ifdef __clang__
			mk_com_strcpy( cc, sizeof( cc ), "clang" );
#else
			mk_com_strcpy( cc, sizeof( cc ), "gcc" );
#endif
		}

		p = getenv( "CXX" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( cxx, sizeof( cxx ), p );
		} else {
			if( strcmp( cc, "clang" ) == 0 ) {
				mk_com_strcpy( cxx, sizeof( cxx ), "clang++" );
			} else {
				char *q;
				mk_com_strcpy( cxx, sizeof( cxx ), cc );
				q = strstr( cxx, "gcc" );
				if( q != (char *)0 ) {
					q[1] = '+';
					q[2] = '+';
				}
			}
		}
	}

	return iscxx ? cxx : cc;
}

/* retrieve the warning flags for compilation */
void mk_bld_getCFlags_warnings( char *flags, size_t nflags ) {
	static int didinit         = 0;
	static char defflags[1024] = { '\0' };

	if( !didinit ) {
		const char *p;

		p = getenv( "CFLAGS_WARNINGS" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defflags, sizeof( defflags ), p );
			mk_com_strcat( defflags, sizeof( defflags ), " " );
		} else {
			mk_com_strcpy( defflags, sizeof( defflags ),
			    "-W -Wall -Wextra -Warray-bounds -pedantic " );
		}

		didinit = 1;
	}

	/*
	 *	TODO: Allow the front-end to override the warning level
	 */

	/* cl: /Wall */
	mk_com_strcat( flags, nflags, defflags );
}
/* figure out the standard :: returns 1 if c++ file */
int mk_bld_getCFlags_standard( char *flags, size_t nflags, const char *filename ) {
	static int didinit = 0;
	static char defcxxpthread[128];
	static char defcxxpedantic[128];
	static char defcxxstandard[128];
	static char defcpthread[128];
	static char defcpedantic[128];
	static char defcstandard[128];
	const char *p;
	int iscplusplus;

	if( !didinit ) {
		p = getenv( "CFLAGS_PTHREAD" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defcpthread, sizeof( defcpthread ), p );
			mk_com_strcat( defcpthread, sizeof( defcpthread ), " " );
		} else {
			mk_com_strcpy( defcpthread, sizeof( defcpthread ), "-pthread " );
		}

		p = getenv( "CXXFLAGS_PTHREAD" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defcxxpthread, sizeof( defcxxpthread ), p );
			mk_com_strcat( defcxxpthread, sizeof( defcxxpthread ), " " );
		} else {
			mk_com_strcpy( defcxxpthread, sizeof( defcxxpthread ), defcpthread );
		}

		p = getenv( "CFLAGS_PEDANTIC" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defcpedantic, sizeof( defcpedantic ), p );
			mk_com_strcat( defcpedantic, sizeof( defcpedantic ), " " );
		} else {
			mk_com_strcpy( defcpedantic, sizeof( defcpedantic ), "" );
		}

		p = getenv( "CXXFLAGS_PEDANTIC" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defcxxpedantic, sizeof( defcxxpedantic ), p );
			mk_com_strcat( defcxxpedantic, sizeof( defcxxpedantic ), " " );
		} else {
			mk_com_strcpy( defcxxpedantic, sizeof( defcxxpedantic ), "-Weffc++ " );
		}

		p = getenv( "CFLAGS_STANDARD" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defcstandard, sizeof( defcstandard ), p );
			mk_com_strcat( defcstandard, sizeof( defcstandard ), " " );
		} else {
			mk_com_strcpy( defcstandard, sizeof( defcstandard ), "-std=gnu17 " );
		}

		p = getenv( "CXXFLAGS_STANDARD" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defcxxstandard, sizeof( defcxxstandard ), p );
			mk_com_strcat( defcxxstandard, sizeof( defcxxstandard ), " " );
		} else {
			mk_com_strcpy( defcxxstandard, sizeof( defcxxstandard ), "-std=gnu++17 " );
		}

		didinit = 1;
	}

	{
		MkLanguage lang;

		lang = mk_fs_getLanguage( filename );
		iscplusplus =
		    ( lang >= kMkLanguage_Cxx_Begin && lang <= kMkLanguage_Cxx_End );
	}

	if( ~mk__g_flags & kMkFlag_OutSingleThread_Bit ) {
		if( iscplusplus ) {
			mk_com_strcat( flags, nflags, defcxxpthread );
		} else {
			mk_com_strcat( flags, nflags, defcpthread );
		}
	}

	if( mk__g_flags & kMkFlag_Pedantic_Bit ) {
		if( iscplusplus ) {
			mk_com_strcat( flags, nflags, defcxxpedantic );
		} else {
			mk_com_strcat( flags, nflags, defcpedantic );
		}
	}

	if( iscplusplus ) {
		mk_com_strcat( flags, nflags, defcxxstandard );
	} else {
		mk_com_strcat( flags, nflags, defcstandard );
	}

	return iscplusplus;
}
/* get configuration specific flags */
void mk_bld_getCFlags_config( char *flags, size_t nflags, int projarch ) {
	static int didinit = 0;
	static char defdbgflags[512];
	static char defrelflags[512];

	if( !didinit ) {
		const char *p;

		p = getenv( "CFLAGS_DEBUG" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defdbgflags, sizeof( defdbgflags ), p );
			mk_com_strcat( defdbgflags, sizeof( defdbgflags ), " " );
		} else {
			mk_com_strcpy( defdbgflags, sizeof( defdbgflags ), "-g -D_DEBUG " );
		}

		p = getenv( "CFLAGS_RELEASE" );
		if( p != (const char *)0 ) {
			mk_com_strcpy( defrelflags, sizeof( defrelflags ), p );
			mk_com_strcat( defrelflags, sizeof( defrelflags ), " " );
		} else {
			mk_com_strcpy( defrelflags, sizeof( defrelflags ),
			    "-DNDEBUG -s -O2 -fno-strict-aliasing " );
		}

		didinit = 1;
	}

	/* optimization/debugging */
	if( mk__g_flags & kMkFlag_Release_Bit ) {
		mk_com_strcat( flags, nflags, defrelflags );

		switch( projarch ) {
		case kMkCPU_X86:
			/* cl: /arch:SSE */
			mk_com_strcat( flags, nflags, "-fomit-frame-pointer " );
			break;
		case kMkCPU_X86_64:
			/* cl: /arch:SSE2 */
			mk_com_strcat( flags, nflags, "-fomit-frame-pointer " );
			break;
		default:
			/*
			 *	NOTE: If you have architecture specific optimizations to apply
			 *        to release code, add them here.
			 */
			break;
		}
	} else {
		/* cl: /Zi /D_DEBUG /DDEBUG /D__debug__ */
		mk_com_strcat( flags, nflags, defdbgflags );
	}
}
/* get platform specific flags */
void mk_bld_getCFlags_platform( char *flags, size_t nflags, int projarch, int projsys, int usenative ) {
	switch( projarch ) {
	case kMkCPU_X86:
		if( usenative ) {
			mk_com_strcat( flags, nflags, "-m32 " );
		} else {
			mk_com_strcat( flags, nflags, "-m32 -march=pentium -mtune=core2 " );
		}
		break;
	case kMkCPU_X86_64:
		if( usenative ) {
			mk_com_strcat( flags, nflags, "-m64 " );
		} else {
			mk_com_strcat( flags, nflags, "-m64 -march=core2 -mtune=core2 " );
		}
		break;
	default:
		break;
	}

	if( usenative ) {
#if 0
		mk_com_strcat(flags, nflags, "-march=native -mtune=native ");
#endif
	}

	/* add a macro for the target system (some systems don't provide their own,
	   or aren't consistent/useful */
	switch( projsys ) {
	case kMkOS_MSWin:
		/* cl: /DMK_MSWIN */
		mk_com_strcat( flags, nflags, "-DMK_MSWIN " );
		break;
	case kMkOS_UWP:
		/* cl: /DMK_UWP */
		mk_com_strcat( flags, nflags, "-DMK_UWP " );
		break;
	case kMkOS_Cygwin:
		/* cl: /DMK_CYGWIN */
		mk_com_strcat( flags, nflags, "-DMK_CYGWIN " );
		break;
	case kMkOS_Linux:
		/* cl: /DMK_LINUX */
		mk_com_strcat( flags, nflags, "-DMK_LINUX " );
		break;
	case kMkOS_MacOSX:
		/* cl: /DMK_MACOS */
		mk_com_strcat( flags, nflags, "-DMK_MACOSX " );
		break;
	case kMkOS_Unix:
		/* cl: /DMK_UNIX */
		mk_com_strcat( flags, nflags, "-DMK_UNIX " );
		break;
	default:
		/*
		 *	PLAN: Add Android/iOS support here too.
		 */
		MK_ASSERT_MSG( 0, "project system not handled" );
		break;
	}
}
/* get project type (executable, dll, ...) specific flags */
void mk_bld_getCFlags_projectType( char *flags, size_t nflags, int projtype ) {
	/* add a macro for the target build type */
	switch( projtype ) {
	case kMkProjTy_Application:
		/* cl: /DAPPLICATION */
		mk_com_strcat( flags, nflags, "-DAPPLICATION -DMK_APPLICATION " );
		break;
	case kMkProjTy_Program:
		/* cl: /DEXECUTABLE */
		mk_com_strcat( flags, nflags, "-DEXECUTABLE -DMK_EXECUTABLE " );
		break;
	case kMkProjTy_StaticLib:
		/* cl: /DLIBRARY */
		mk_com_strcat( flags, nflags, "-DLIBRARY -DMK_LIBRARY "
		                              "-DLIB -DMK_LIB " );
		break;
	case kMkProjTy_DynamicLib:
		/* cl: /DDYNAMICLIBRARY */
		mk_com_strcat( flags, nflags, "-DDYNAMICLIBRARY -DMK_DYNAMICLIBRARY "
		                              "-DDLL -DMK_DLL " );
		/* " -fPIC" */
		break;
	default:
		MK_ASSERT_MSG( 0, "project type not handled" );
		break;
	}
}
/* add all include paths */
void mk_bld_getCFlags_incDirs( char *flags, size_t nflags ) {
	size_t i, n;

	mk_com_strcat( flags, nflags, mk_com_va( "-I \"%s/..\" ", mk_opt_getBuildGenIncDir() ) );

	/* add the include search paths */
	n = mk_sl_getSize( mk__g_incdirs );
	for( i = 0; i < n; i++ ) {
		/* cl: "/I \"%s\" " */
		mk_com_strcat( flags, nflags, mk_com_va( "-I \"%s\" ", mk_sl_at( mk__g_incdirs, i ) ) );
	}
}
/* add all preprocessor definitions */
void mk_bld_getCFlags_defines( char *flags, size_t nflags, MkStrList defs ) {
	size_t i, n;

	/* add project definitions */
	n = mk_sl_getSize( defs );
	for( i = 0; i < n; i++ ) {
		/* cl: "\"/D%s\" " */
		mk_com_strcat( flags, nflags, mk_com_va( "\"-D%s\" ", mk_sl_at( defs, i ) ) );
	}
}
/* add the input/output flags */
void mk_bld_getCFlags_unitIO( char *flags, size_t nflags, const char *obj, const char *src ) {
	/*
	 *	TODO: Visual C++ and dependencies. How?
	 *	-     We can use /allincludes (or whatevertf it's called) to get them...
	 */

	/* add the remaining flags (e.g., dependencies, compile-only, etc) */
	mk_com_strcat( flags, nflags, "-MD -MP -c " );

	/* add the appropriate compilation flags */
	mk_com_strcat( flags, nflags, mk_com_va( "-o \"%s\" \"%s\"", obj, src ) );
}

/* retrieve the flags for compiling a particular source file */
const char *mk_bld_getCFlags( MkProject proj, const char *obj, const char *src ) {
	static char flags[16384];

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( obj != (const char *)0 );
	MK_ASSERT( src != (const char *)0 );

	flags[0] = '\0';

	mk_bld_getCFlags_warnings( flags, sizeof( flags ) );
	mk_bld_getCFlags_standard( flags, sizeof( flags ), src );
	mk_bld_getCFlags_config( flags, sizeof( flags ), proj->arch );
	mk_bld_getCFlags_platform( flags, sizeof( flags ), proj->arch, proj->sys, 0 );
	mk_bld_getCFlags_projectType( flags, sizeof( flags ), proj->type );
	mk_bld_getCFlags_incDirs( flags, sizeof( flags ) );
	mk_bld_getCFlags_defines( flags, sizeof( flags ), proj->defs );
	mk_bld_getCFlags_unitIO( flags, sizeof( flags ), obj, src );

	return flags;
}

/* retrieve the dependencies of a project and its subprojects */
void mk_bld_getDeps_r( MkProject proj, MkStrList deparray ) {
	static size_t nest      = 0;
	static char spaces[512] = {
		'\0',
	};
	size_t i, n, j, m;
	const char *libname;
	MkLib lib;
	char src[PATH_MAX], obj[PATH_MAX], dep[PATH_MAX];

	if( !spaces[0] ) {
		for( i = 0; i < sizeof( spaces ) - 1; i++ ) {
			spaces[i] = ' ';
		}

		spaces[i] = '\0';
	}

	nest++;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( deparray != (MkStrList)0 );

	/* ensure the project is up to date */
	mk_dbg_outf( " **** mk_bld_getDeps_r \"%s\" ****\n", proj->name );
	n = mk_prj_numSourceFiles( proj );
	for( i = 0; i < n; i++ ) {
		mk_com_relPathCWD( src, sizeof( src ), mk_prj_sourceFileAt( proj, i ) );
		mk_bld_getObjName( proj, obj, sizeof( obj ), src );
		mk_com_substExt( dep, sizeof( dep ), obj, ".d" );
		mk_dbg_outf( " **** src:\"%s\" obj:\"%s\" dep:\"%s\" ****\n",
		    src, obj, dep );
		if( !mk_bld_findSourceLibs( proj->libs, proj->sys, obj, dep ) ) {
			mk_dbg_outf( "mk_bld_findSourceLibs(\"%s\"->libs, %i, \"%s\", \"%s\") "
			             "call failed\n",
			    proj->name, proj->sys, obj, dep );
			continue;
		}
	}

	/* check the libraries */
	n = mk_sl_getSize( proj->libs );
	for( i = 0; i < n; i++ ) {
		libname = mk_sl_at( proj->libs, i );
		if( !libname ) {
			continue;
		}

		mk_dbg_outf( "%.*s[dep] %s\n", ( nest - 1 ) * 2, spaces, libname );

		lib = mk_lib_find( libname );
		if( !lib || !lib->proj || lib->proj == proj ) {
			continue;
		}

		m = mk_sl_getSize( deparray );
		for( j = 0; j < m; j++ ) {
			if( !strcmp( mk_sl_at( deparray, j ), libname ) ) {
				break;
			}
		}

		if( j < m ) {
			continue;
		}

		mk_sl_pushBack( deparray, mk_sl_at( proj->libs, i ) );
		mk_bld_getDeps_r( lib->proj, deparray );
	}

	nest--;
}

/* determine whether a library depends upon another library */
int mk_bld_doesLibDependOnLib( MkLib mainlib, MkLib deplib ) {
	static size_t nest      = 0;
	static char spaces[512] = {
		'\0',
	};
	size_t i, n;
	MkLib libs[1024];

	MK_ASSERT( mainlib != (MkLib)0 );
	MK_ASSERT( deplib != (MkLib)0 );

	if( !spaces[0] ) {
		for( i = 0; i < sizeof( spaces ) - 1; i++ )
			spaces[i] = ' ';

		spaces[i] = '\0';
	}

	nest++;

	mk_dbg_outf( "%.*sdoeslibdependonlib_r(\"%s\",\"%s\")\n",
	    ( nest - 1 ) * 2, spaces, mainlib->name, deplib->name );

	if( nest > 10 ) {
		nest--;
		return 0;
	}

#if 0
	printf("%s%p %s%p\n", _WIN32 ? "0x" : "",
		(void *)mainlib, _WIN32 ? "0x" : "", (void *)deplib);
	printf("%s %s\n", mainlib->name, deplib->name);
#endif

	if( !mainlib->proj || mainlib == deplib ) {
		nest--;
		return 0;
	}

	mk_sl_makeUnique( mainlib->proj->libs );
	n = mk_sl_getSize( mainlib->proj->libs );
	if( n > sizeof( libs ) / sizeof( libs[0] ) ) {
		mk_log_error( __FILE__, __LINE__, __func__, "n > 1024; too many libraries" );
		exit( EXIT_FAILURE );
	}

	/* first check; any immediate dependencies? */
	for( i = 0; i < n; i++ ) {
		mk_dbg_outf( "%.*s%.2u of %u \"%s\"\n", ( nest - 1 ) * 2, spaces,
		    i + 1, n, mk_sl_at( mainlib->proj->libs, i ) );
		libs[i] = mk_lib_find( mk_sl_at( mainlib->proj->libs, i ) );

		if( libs[i] == deplib ) {
			mk_dbg_outf( "%.*s\"%s\" depends on \"%s\"\n",
			    ( nest - 1 ) * 2, spaces, libs[i]->name, deplib->name );
			nest--;
			return 1;
		}
	}

	/* second check; recursive check */
	for( i = 0; i < n; i++ ) {
		if( !mk_bld_doesLibDependOnLib( libs[i], deplib ) ) {
			continue;
		}

		nest--;
		return 1;
	}

	/* nope; this lib does not depend on that lib */
	nest--;
	return 0;
}

/* determine which library should come first */
int mk_bld__cmpLibs( const void *a, const void *b ) {
#if 0
	int r;
	printf("About to compare \"%s\" to \"%s\"\n",
		(*(MkLib *)a)->name, (*(MkLib *)b)->name);
	fflush(stdout);
	r = mk_bld_doesLibDependOnLib(*(MkLib *)a, *(MkLib *)b);
	printf("mk_bld__cmpLibs \"%s\" \"%s\" -> %i\n",
		(*(MkLib *)a)->name, (*(MkLib *)b)->name, r);
	fflush(stdout);
	return 1 - r;
#else
	return 1 - mk_bld_doesLibDependOnLib( *(MkLib *)a, *(MkLib *)b );
#endif
}
void mk_bld_sortDeps( MkStrList deparray ) {
	size_t i, n;
	MkLib libs[1024];

	MK_ASSERT( deparray != (MkStrList)0 );

	/* remove duplicates from the array */
	mk_dbg_outf( "Dependency Array (Before Unique):\n" );
	mk_sl_makeUnique( deparray );

	mk_dbg_outf( "Dependency Array:\n" );
	mk_sl_debugPrint( deparray );

	/* too many libraries? */
	n = mk_sl_getSize( deparray );
	if( n > sizeof( libs ) / sizeof( libs[0] ) ) {
		mk_log_error( __FILE__, __LINE__, __func__, "Too many libraries" );
		exit( EXIT_FAILURE );
	}

	/* fill the array, then sort it */
	for( i = 0; i < n; i++ ) {
		libs[i] = mk_lib_find( mk_sl_at( deparray, i ) );
		if( !libs[i] ) {
			mk_log_fatalError( mk_com_va( "Couldn't find library ^E\"%s\"^&",
			    mk_sl_at( deparray, i ) ) );
		}
	}

	mk_dbg_outf( "About to sort...\n" );
	qsort( (void *)libs, n, sizeof( MkLib ), mk_bld__cmpLibs );
	mk_dbg_outf( "Sorted!\n" );

	/* set the array elements, sorted */
	for( i = 0; i < n; i++ ) {
		mk_sl_set( deparray, i, mk_lib_getName( libs[i] ) );
	}

	mk_dbg_outf( "Sorted Dependency Array:\n" );
	mk_sl_debugPrint( deparray );
}

/* get the linker flags for linking dependencies of a project */
const char *mk_bld_getProjDepLinkFlags( MkProject proj ) {
	static char buf[65536];
	MkStrList deps;
	size_t i, n;
	MkLib lib;

	deps = mk_sl_new();

	mk_bld_getDeps_r( proj, deps );
	mk_dbg_outf( "Project: \"%s\"\n", proj->name );
	mk_bld_sortDeps( deps );

	buf[0] = '\0';

	n = mk_sl_getSize( deps );
	for( i = 0; i < n; i++ ) {
		lib = mk_lib_find( mk_sl_at( deps, i ) );
		if( !lib ) {
			continue;
		}

		if( !lib->flags[proj->sys] ) {
			continue;
		}

		if( !lib->flags[proj->sys][0] ) {
			continue;
		}

		mk_com_strcat( buf, sizeof( buf ), mk_com_va( "%s ", lib->flags[proj->sys] ) );
	}

	mk_sl_delete( deps );
	return buf;
}

/* construct a list of dependent libraries */
/*
 *	???: Dependent on what? Libraries the project passed in relies on? Libraries
 *	     other projects would need if they were using this?
 *
 *	NOTE: It appears this function is used by mk_bld_getLFlags() to determine how to
 *	      link with a given project.
 */
void mk_bld_getLibs( MkProject proj, char *dst, size_t n ) {
	const char *name, *suffix = "";
	MkProject p;
	char bin[PATH_MAX];
	int type;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( dst != (char *)0 );
	MK_ASSERT( n > 1 );

	/*
	 *	HACK: "cannot find -lblah" is really annoying when you don't want the
	 *	      project
	 */
	if( !mk_prj_isTarget( proj ) ) {
		return;
	}

	type = mk_prj_getType( proj );
	name = mk_prj_getName( proj );

	suffix = mk__g_flags & kMkFlag_Release_Bit ? "" : mk_opt_getDebugSuffix();

	if( type == kMkProjTy_StaticLib ) {
		/*
		 *	???: Why is this here? What is it doing?
		 *	NOTE: This code did fix a bug... but which bug? And why?
		 *
		 *	???: Was this to make sure that a dependent project with no source
		 *	     files wouldn't be linked in?
		 */
		if( ( ~proj->config & kMkProjCfg_Linking_Bit ) && mk_prj_numSourceFiles( proj ) > 0 ) {
			mk_com_strcat( dst, n, mk_com_va( "-l%s%s ", name, suffix ) );
		}

		/* NOTE: these should be linked after the library itself is linked in */
		for( p = mk_prj_head( proj ); p; p = mk_prj_next( p ) )
			mk_bld_getLibs( p, dst, n );
	} else if( type == kMkProjTy_DynamicLib ) {
		if( ( ~proj->config & kMkProjCfg_Linking_Bit ) && mk_prj_numSourceFiles( proj ) > 0 ) {
			mk_bld_getBinName( proj, bin, sizeof( bin ) );
			mk_com_strcat( dst, n, mk_com_va( "\"%s\" ", bin ) );
			/*
			mk_com_strcat(dst, n, mk_com_va("-L \"%s\" ", mk_prj_getOutPath(proj)));
			mk_com_strcat(dst, n, mk_com_va("-l%s%s.dll ", name, suffix));
			*/
		}
	}
}

/* retrieve the flags for linking a project */
const char *mk_bld_getLFlags( MkProject proj, const char *bin, MkStrList objs ) {
	static char flags[32768];
	static char libs[16384], libs_stripped[16384];
	const char *pszStaticFlags;
	MkProject p;
	size_t i, n;

	(void)p;
	(void)libs_stripped;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( bin != (const char *)0 );
	MK_ASSERT( objs != (MkStrList)0 );

	flags[0] = '\0';
	libs[0]  = '\0';

	pszStaticFlags = "";
	if( proj->sys == kMkOS_MSWin || proj->sys == kMkOS_UWP ) {
		pszStaticFlags = "-static -static-libgcc ";
	}

	switch( mk_prj_getType( proj ) ) {
	case kMkProjTy_Application:
		if( mk__g_flags & kMkFlag_Release_Bit ) {
			mk_com_strcat( flags, sizeof( flags ), "-s " );
		}
		if( proj->sys == kMkOS_MSWin && ( mk__g_flags & kMkFlag_Release_Bit ) ) {
			mk_com_strcat( flags, sizeof( flags ),
			    mk_com_va( "%s-Wl,subsystem,windows -o \"%s\" ", pszStaticFlags, bin ) );
		} else {
			mk_com_strcat( flags, sizeof( flags ),
			    mk_com_va( "%s-o \"%s\" ", pszStaticFlags, bin ) );
		}
		break;
	case kMkProjTy_Program:
		if( mk__g_flags & kMkFlag_Release_Bit ) {
			mk_com_strcat( flags, sizeof( flags ), "-s " );
		}
		mk_com_strcat( flags, sizeof( flags ), mk_com_va( "%s-o \"%s\" ", pszStaticFlags, bin ) );
		break;
	case kMkProjTy_StaticLib:
		mk_com_strcat( flags, sizeof( flags ), mk_com_va( "cr \"%s\" ", bin ) );
		break;
	case kMkProjTy_DynamicLib:
		mk_com_strcat( flags, sizeof( flags ),
		    mk_com_va( "%s-shared -o \"%s\" ", pszStaticFlags, bin ) );
		break;
	default:
		MK_ASSERT_MSG( 0, "unhandled project type" );
		break;
	}

	if( mk_prj_getType( proj ) != kMkProjTy_StaticLib ) {
		n = mk_sl_getSize( mk__g_libdirs );
		for( i = 0; i < n; i++ ) {
			mk_com_strcat( flags, sizeof( flags ), mk_com_va( "-L \"%s\" ", mk_sl_at( mk__g_libdirs, i ) ) );
		}
	}

	n = mk_sl_getSize( objs );
	for( i = 0; i < n; i++ ) {
		mk_com_strcat( flags, sizeof( flags ), mk_com_va( "\"%s\" ", mk_sl_at( objs, i ) ) );
	}

	if( mk_prj_getType( proj ) != kMkProjTy_StaticLib ) {
		proj->config |= kMkProjCfg_Linking_Bit;

#if 0
		/*
		 *	NOTE: This should no longer be necessary. The "lib" system used now
		 *	      should take care of this. (Dependencies found by the header
		 *	      files of each project.)
		 */
		for( p=mk__g_lib_proj_head; p; p=p->lib_next )
			mk_bld_getLibs(p, libs, sizeof(libs));

		p = mk_prj_getParent(proj) ? mk_prj_head(mk_prj_getParent(proj))
			: mk_prj_rootHead();
		while( p != (MkProject)0 ) {
			MK_ASSERT( p != (MkProject)0 );
			mk_bld_getLibs(p, libs, sizeof(libs));
			p = mk_prj_next(p);
		}
#else
		mk_com_strcpy( libs, sizeof( libs ), mk_bld_getProjDepLinkFlags( proj ) );
#endif

		proj->config &= ~kMkProjCfg_Linking_Bit;

#if 0
		mk_com_stripArgs(libs_stripped, sizeof(libs_stripped), libs);
		mk_com_strcat(flags, sizeof(flags), libs_stripped);
#else
		mk_com_strcat( flags, sizeof( flags ), libs );
#endif

		switch( proj->sys ) {
		case kMkOS_MSWin:
			if( mk_prj_getType( proj ) == kMkProjTy_DynamicLib ) {
				mk_com_strcat( flags, sizeof( flags ),
				    /* NOTE: we don't use the import library; it's pointless */
				    mk_com_va( /*"\"-Wl,--out-implib=%s%s.a\" "*/
				        "-Wl,--export-all-symbols "
				        "-Wl,--enable-auto-import " /*, bin,
					   ~mk__g_flags & kMkFlag_Release_Bit ? mk_opt_getDebugSuffix() : ""*/
				        ) );
			}

			/*mk_com_strcat(flags, sizeof(flags), "-lkernel32 -luser32 -lgdi32 "
				"-lshell32 -lole32 -lopengl32 -lmsimg32");*/
			break;
		case kMkOS_Linux:
			/*mk_com_strcat(flags, sizeof(flags), "-lGL");*/
			break;
		case kMkOS_MacOSX:
			/*mk_com_strcat(flags, sizeof(flags), "-lobjc -framework Cocoa "
				"-framework OpenGL");*/
			break;
		default:
			/*
			 *	NOTE: Add OS specific link flags here.
			 */
			break;
		}

		mk_com_strcat( flags, sizeof( flags ), mk_prj_getLinkFlags( proj ) );
	}
#if 0
	else {
		static char extras[32768];

		proj->config |= kMkProjCfg_Linking_Bit;

		p = mk_prj_getParent(proj) ? mk_prj_head(mk_prj_getParent(proj))
			: mk_prj_rootHead();
		for( p=p; p!=(MkProject)0; p=mk_prj_next(p) ) {
			if( p==proj )
				continue;

			extras[0] = '\0';
			mk_prj_completeExtraLibs(p, extras, sizeof(extras));
			mk_prj_appendExtraLibs(proj, extras);
		}
		while( p != (MkProject)0 ) {
			MK_ASSERT( p != (MkProject)0 );
			mk_bld_getLibs(p, libs, sizeof(libs));
			p = mk_prj_next(p);
		}
	}
#endif

	return flags;
}

/* find the name of an object file for a given source file */
void mk_bld_getObjName( MkProject proj, char *obj, size_t n, const char *src ) {
	const char *p = "";

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( obj != (char *)0 );
	MK_ASSERT( n > 1 );
	MK_ASSERT( src != (const char *)0 );

	switch( mk_prj_getType( proj ) ) {
	case kMkProjTy_Application:
		p = "appexec";
		break;
	case kMkProjTy_Program:
		p = "exec";
		break;
	case kMkProjTy_StaticLib:
		p = "lib";
		break;
	case kMkProjTy_DynamicLib:
		p = "dylib";
		break;
	default:
		MK_ASSERT_MSG( 0, "project type is invalid" );
		break;
	}

	mk_com_substExt( obj, n, mk_com_va( "%s/%s/%s/%s", mk_opt_getObjdirBase(), mk_opt_getConfigName(), p, src ), ".o" );
}

/* find the binary name of a target */
void mk_bld_getBinName( MkProject proj, char *bin, size_t n ) {
	const char *dir, *prefix, *name, *symbol, *suffix = "";
	int type;
	int sys;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( bin != (char *)0 ? n > 1 : 1 );

	if( !proj->binname || bin != (char *)0 ) {
		type = mk_prj_getType( proj );
		sys  = proj->sys;

		/*dir = type==kMkProjTy_StaticLib ? "lib/" : "bin/";*/
		dir = mk_prj_getOutPath( proj );
		MK_ASSERT( dir != (const char *)0 );
		prefix = "";
		name   = mk_prj_getName( proj );
		symbol = mk__g_flags & kMkFlag_Release_Bit ? "" : mk_opt_getDebugSuffix();

		switch( type ) {
		case kMkProjTy_Application:
		case kMkProjTy_Program:
			if( sys == kMkOS_MSWin || sys == kMkOS_UWP || sys == kMkOS_Cygwin ) {
				suffix = ".exe";
			}
			break;
		case kMkProjTy_StaticLib:
			prefix = "lib";
			suffix = ".a";
			break;
		case kMkProjTy_DynamicLib:
			if( sys != kMkOS_MSWin && sys != kMkOS_UWP ) {
				prefix = "lib";
			}
			if( sys == kMkOS_MSWin || sys == kMkOS_UWP || sys == kMkOS_Cygwin ) {
				suffix = ".dll";
			} else if( sys == kMkOS_Linux || sys == kMkOS_Unix ) {
				suffix = ".so";
			} else if( sys == kMkOS_MacOSX ) {
				suffix = ".dylib";
			}
			break;
		default:
			MK_ASSERT_MSG( 0, "unhandled project type" );
			break;
		}

		proj->binname = mk_com_dup( proj->binname, mk_com_va( "%s%s%s%s%s", dir, prefix, name, symbol, suffix ) );
	}

	if( bin != (char *)0 ) {
		mk_com_strcpy( bin, n, proj->binname );
	}
}

/* compile and run a unit test */
void mk_bld_unitTest( MkProject proj, const char *src ) {
	static char flags[32768];
	static char libs[16384], libs_stripped[16384];
	const char *tool, *libname, *libf;
	const char *cc, *cxx;
	MkProject chld;
	size_t i, j, n;
	MkLib lib;
	char out[PATH_MAX], dep[PATH_MAX], projbin[PATH_MAX];

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( src != (const char *)0 );

	(void)chld;
	(void)libs_stripped;

	cc  = mk_bld_getCompiler( 0 );
	cxx = mk_bld_getCompiler( 1 );

	/* determine the name for the unit test's executable */
	mk_com_strcpy( out, sizeof( out ), mk_com_va( "%s/%s/%s/test/", mk_opt_getObjdirBase(), mk_opt_getConfigName(), mk_prj_getName( proj ) ) );
	mk_fs_makeDirs( out );
	mk_com_substExt( out, sizeof( out ), mk_com_va( "%s%s", out, strrchr( src, '/' ) + 1 ),
	    ".test" );

#if MK_WINDOWS_ENABLED
	mk_com_strcat( out, sizeof( out ), ".exe" );
	mk_com_substExt( dep, sizeof( dep ), out, ".d" );
#else
	mk_com_strcpy( dep, sizeof( dep ), out );
	mk_com_strcat( dep, sizeof( dep ), ".d" );
#endif

	flags[0] = '\0';

	mk_bld_getCFlags_warnings( flags, sizeof( flags ) );
	if( mk_bld_getCFlags_standard( flags, sizeof( flags ), src ) ) {
		tool = cxx;
	} else {
		tool = ( proj->config & kMkProjCfg_UsesCxx_Bit ) ? cxx : cc;
	}
	mk_bld_getCFlags_config( flags, sizeof( flags ), proj->arch );
	mk_bld_getCFlags_platform( flags, sizeof( flags ), proj->arch, proj->sys, 1 );
	mk_com_strcat( flags, sizeof( flags ), "-DTEST -DMK_TEST " );
	mk_com_strcat( flags, sizeof( flags ), "-DEXECUTABLE -DMK_EXECUTABLE " );
	mk_bld_getCFlags_incDirs( flags, sizeof( flags ) );
	mk_bld_getCFlags_defines( flags, sizeof( flags ), proj->defs );

	/* retrieve all of the library directories */
	n = mk_sl_getSize( mk__g_libdirs );
	for( i = 0; i < n; i++ ) {
		mk_com_strcat( flags, sizeof( flags ), mk_com_va( "-L \"%s\" ", mk_sl_at( mk__g_libdirs, i ) ) );
	}

	/* determine compilation flags: output and source */
	mk_com_strcat( flags, sizeof( flags ), mk_com_va( "-o \"%s\" \"%s\" ", out, src ) );

	/* link to the project directly if it's a library (static or dynamic) */
	switch( mk_prj_getType( proj ) ) {
	case kMkProjTy_StaticLib:
	case kMkProjTy_DynamicLib:
		mk_bld_getBinName( proj, projbin, sizeof( projbin ) );
		mk_com_strcat( flags, sizeof( flags ), mk_com_va( "\"%s\" ", projbin ) );
		break;
	}

	/* now include all of the necessary libs we depend on (we're assuming these
	   are the same libs the project itself depends on)

	   FIXME: this won't work if we try using a lib that the project doesn't
	          depend on here... perhaps the solution is to make unit tests their
			  own projects? */
	libs[0] = '\0';

#if 0
	/* first pass: grab all sibling projects */
	chld = mk_prj_getParent(proj) ? mk_prj_head(mk_prj_getParent(proj))
	       : mk_prj_rootHead();

	for( chld=chld; chld != (MkProject)0; chld=mk_prj_next(chld) ) {
		MK_ASSERT( chld != (MkProject)0 );

		if( chld==proj )
			continue;

		mk_bld_getLibs(chld, libs, sizeof(libs));
		chld = mk_prj_next(chld);
	}

	/* second pass: grab all of our direct dependencies */
	for( chld=mk_prj_head(proj); chld != (MkProject)0; chld=mk_prj_next(chld) )
	{
		MK_ASSERT( chld != (MkProject)0 );
		mk_bld_getLibs(chld, libs, sizeof(libs));
		chld = mk_prj_next(chld);
	}
#endif

	/*
	 *	TODO: Use the proper dependency checking method.
	 */

	/* grab all of the directly dependent libraries */
	n = mk_sl_getSize( proj->libs );
	for( i = 0; i < n; i++ ) {
		/* if we have a null pointer in the array, skip it */
		if( !( libname = mk_sl_at( proj->libs, i ) ) ) {
			continue;
		}

		/* check for a mk_com_dup library */
		for( j = 0; j < i; j++ ) {
			/* skip null pointers... */
			if( !mk_sl_at( proj->libs, j ) ) {
				continue;
			}

			/* is there a match? (did we already include it, that is) */
			if( !strcmp( mk_sl_at( proj->libs, j ), libname ) ) {
				break;
			}
		}

		/* found a duplicate, don't include again */
		if( j != i ) {
			continue;
		}

		/* try to find the library handle from the name in the array */
		if( !( lib = mk_lib_find( libname ) ) ) {
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_YELLOW, "WARN" );
			mk_sys_uncoloredPuts( kMkSIO_Err, ": ", 2 );
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_PURPLE, src );
			mk_sys_uncoloredPuts( kMkSIO_Err, ": couldn't find library \"", 0 );
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_BROWN, libname );
			mk_sys_uncoloredPuts( kMkSIO_Err, "\"\n", 2 );
			continue;
		}

		/* retrieve the flags... if we have a non-empty string, add them to the
		   linker flags */
		if( ( libf = mk_lib_getFlags( lib, proj->sys ) ) != (const char *)0 && *libf != '\0' ) {
			mk_com_strcat( libs, sizeof( libs ), mk_com_va( "%s ", libf ) );
		}
	}

	/* link in all libraries */
#if 0
	/*
	 *	NOTE: This should no longer be necessary. Using the mk_al_autolink system.
	 */
	for( chld=mk__g_lib_proj_head; chld; chld=chld->lib_next )
		mk_bld_getLibs(chld, libs, sizeof(libs));

#	if 0
	mk_com_stripArgs(libs_stripped, sizeof(libs_stripped), libs);
	mk_com_strcat(flags, sizeof(flags), libs_stripped);
#	else
	mk_com_strcat(flags, sizeof(flags), libs);
#	endif
#endif

	/* queue unit tests */
	mk_sl_pushBack( mk__g_unitTestCompiles, mk_com_va( "%s %s", tool, flags ) );
	mk_sl_pushBack( mk__g_unitTestRuns, out );

	mk_com_relPathCWD( out, sizeof( out ), src );
	mk_sl_pushBack( mk__g_unitTestNames, out );
}

/* perform each unit test */
void mk_bld_runTests( void ) {
	static size_t buffer[65536];
	MkStrList failedtests;
	size_t i, n;
	int e;

	MK_ASSERT( mk_sl_getSize( mk__g_unitTestCompiles ) == mk_sl_getSize( mk__g_unitTestRuns ) );
	MK_ASSERT( mk_sl_getSize( mk__g_unitTestRuns ) == mk_sl_getSize( mk__g_unitTestNames ) );

	n = mk_sl_getSize( mk__g_unitTestCompiles );
	if( !n ) {
		return;
	}
	MK_ASSERT( n <= sizeof( buffer ) / sizeof( buffer[0] ) );
	mk_sl_orderedSort( mk__g_unitTestNames, buffer, sizeof( buffer ) / sizeof( buffer[0] ) );
	mk_sl_indexedSort( mk__g_unitTestCompiles, buffer, n );
	mk_sl_indexedSort( mk__g_unitTestRuns, buffer, n );

	failedtests = mk_sl_new();

	for( i = 0; i < n; i++ ) {
		/* compile the unit test */
		if( mk_com_shellf( "%s", mk_sl_at( mk__g_unitTestCompiles, i ) ) != 0 ) {
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_LIGHT_RED, "KO" );
			mk_sys_uncoloredPuts( kMkSIO_Err, ": ", 2 );
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_PURPLE, mk_sl_at( mk__g_unitTestNames, i ) );
			mk_sys_uncoloredPuts( kMkSIO_Err, " (did not build)\n", 0 );
			mk_sl_pushBack( failedtests, mk_sl_at( mk__g_unitTestNames, i ) );
			continue;
		}

		/* run the unit test */
		e = mk_com_shellf( "%s", mk_sl_at( mk__g_unitTestRuns, i ) );
		if( e != 0 ) {
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_LIGHT_RED, "KO" );
			mk_sys_uncoloredPuts( kMkSIO_Err, ": ", 2 );
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_RED, mk_sl_at( mk__g_unitTestNames, i ) );
			mk_sys_printf( kMkSIO_Err, " (returned " MK_S_COLOR_WHITE "%i" MK_S_COLOR_RESTORE ")\n", e );
			mk_sl_pushBack( failedtests, mk_sl_at( mk__g_unitTestRuns, i ) );
		} else {
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_LIGHT_GREEN, "OK" );
			mk_sys_uncoloredPuts( kMkSIO_Err, ": ", 2 );
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_WHITE, mk_sl_at( mk__g_unitTestNames, i ) );
			mk_sys_uncoloredPuts( kMkSIO_Err, "\n", 1 );
		}
	}

	n = mk_sl_getSize( failedtests );
	if( n > 0 ) {
		mk_sys_printStr( kMkSIO_Err, MK_COLOR_RED, "\n  *** " );
		mk_sys_printStr( kMkSIO_Err, MK_COLOR_WHITE, mk_com_va( "%u", (unsigned int)n ) );
		mk_sys_printStr( kMkSIO_Err, MK_COLOR_LIGHT_RED, n == 1 ? " FAILURE" : " FAILURES" );
		mk_sys_printStr( kMkSIO_Err, MK_COLOR_RED, " ***\n  " );
		for( i = 0; i < n; i++ ) {
			mk_sys_printStr( kMkSIO_Err, MK_COLOR_YELLOW, mk_sl_at( failedtests, i ) );
			mk_sys_uncoloredPuts( kMkSIO_Err, "\n  ", 3 );
		}
		mk_sys_printStr( kMkSIO_Err, MK_COLOR_RED, "\n" );
	}

	mk_sl_delete( failedtests );
}

/* sort the projects in a list */
void mk_bld_sortProjects( struct MkProject_s *proj ) {
	struct MkProject_s **head, **tail;
	struct MkProject_s *p, *next;
	int numsorts;

	head = proj ? &proj->head : &mk__g_proj_head;
	tail = proj ? &proj->tail : &mk__g_proj_tail;

	do {
		numsorts = 0;

		for( p = *head; p; p = next ) {
			if( !( next = p->next ) ) {
				break;
			}

			if( p->type > next->type ) {
				numsorts++;

				if( p == *head ) {
					*head = next;
				}
				if( next == *tail ) {
					*tail = p;
				}

				if( p->prev ) {
					p->prev->next = next;
				}
				if( next->next ) {
					next->next->prev = p;
				}

				next->prev = p->prev;
				p->prev    = next;
				p->next    = next->next;
				next->next = p;

				next = p;
			}
		}
	} while( numsorts > 0 );
}

/* set a projects dependents to be relinked */
void mk_bld_relinkDeps( MkProject proj ) {
	MkProject prnt, next;

	MK_ASSERT( proj != (MkProject)0 );

	for( prnt = proj->prnt; prnt; prnt = prnt->prnt ) {
		prnt->config |= kMkProjCfg_NeedRelink_Bit;
	}

	if( proj->type == kMkProjTy_Program || proj->type == kMkProjTy_Application ) {
		return;
	}

	for( next = proj->next; next; next = next->next ) {
		if( proj->type > next->type ) {
			break;
		}

		next->config |= kMkProjCfg_NeedRelink_Bit;
		mk_bld_relinkDeps( next );
	}
}

/* build a project */
int mk_bld_makeProject( MkProject proj ) {
	const char *src, *lnk, *tool, *cxx, *cc;
	MkProject chld;
	MkStrList objs;
	size_t cwd_l;
	size_t i, n;
	char cwd[PATH_MAX], obj[PATH_MAX], bin[PATH_MAX];
	int numbuilds;

	/* build the child projects */
	mk_bld_sortProjects( proj );
	for( chld = mk_prj_head( proj ); chld; chld = mk_prj_next( chld ) ) {
		if( !mk_bld_makeProject( chld ) ) {
			return 0;
		}
	}

	/* if this project isn't targetted, just return now */
	if( !mk_prj_isTarget( proj ) )
		return 1;

	/* retrieve the current working directory */
	if( getcwd( cwd, sizeof( cwd ) ) == (char *)0 ) {
		mk_log_fatalError( "getcwd() failed" );
	}

	cwd_l = mk_com_strlen( cwd );

	/*
	 *	NOTE: This appears to be a hack.
	 */
	if( !mk_prj_numSourceFiles( proj ) /*&& mk_prj_getType( proj ) != kMkProjTy_DynamicLib*/ ) {
		/*
		mk_log_errorMsg( mk_com_va( "project ^E'%s'^& has no source files!",
		    mk_prj_getName( proj ) ) );
		*/
		return 1;
	}

	cc   = mk_bld_getCompiler( 0 );
	cxx  = mk_bld_getCompiler( 1 );
	tool = proj->config & kMkProjCfg_UsesCxx_Bit ? cxx : cc;

	/* make the object directories */
	mk_prjfs_makeObjDirs( proj );

	/* store each object file */
	objs = mk_sl_new();

	/* run through each source file */
	numbuilds = 0;
	n         = mk_prj_numSourceFiles( proj );
	for( i = 0; i < n; i++ ) {
		src = mk_prj_sourceFileAt( proj, i );
		mk_bld_getObjName( proj, obj, sizeof( obj ), &src[cwd_l + 1] );
		mk_sl_pushBack( objs, obj );

		if( mk_bld_shouldCompile( obj ) ) {
			if( mk_com_shellf( "%s %s", tool,
			        mk_bld_getCFlags( proj, obj, &src[cwd_l + 1] ) ) ) {
				mk_sl_delete( objs );
				return 0;
			}
			numbuilds++;
		}

		mk_com_substExt( bin, sizeof( bin ), obj, ".d" );
		if( ( ~mk__g_flags & kMkFlag_NoLink_Bit ) && !mk_bld_findSourceLibs( proj->libs, proj->sys, obj, bin ) ) {
			mk_log_errorMsg( "call to mk_bld_findSourceLibs() failed" );
			return 0;
		}
	}

	/* link the project's object files together */
	mk_bld_getBinName( proj, bin, sizeof( bin ) );
	/*printf("bin: %s\n", bin);*/
	if( ( proj->config & kMkProjCfg_NeedRelink_Bit ) || mk_bld_shouldLink( bin, numbuilds ) ) {
		mk_fs_makeDirs( mk_prj_getOutPath( proj ) );
		mk_sl_makeUnique( proj->libs );
		mk_prj_calcLibFlags( proj );

		/* find the libraries this project needs */
		lnk = mk_prj_getType( proj ) == kMkProjTy_StaticLib ? "ar" : tool;
		if( mk_com_shellf( "%s %s", lnk, mk_bld_getLFlags( proj, bin, objs ) ) ) {
			mk_sl_delete( objs );
			return 0;
		}

		/* dependent projects need to be rebuilt */
		mk_bld_relinkDeps( proj );
	} else if( ~mk__g_flags & kMkFlag_FullClean_Bit ) {
		mk_prj_calcDeps( proj );
	}

	/* unit testing */
	if( mk__g_flags & kMkFlag_Test_Bit ) {
		n = mk_prj_numTestSourceFiles( proj );
		for( i = 0; i < n; i++ ) {
			mk_bld_unitTest( proj, mk_prj_testSourceFileAt( proj, i ) );
		}
	}

	/* clean (removes temporaries) -- only if not rebuilding */
	if( mk__g_flags & kMkFlag_LightClean_Bit ) {
		n = mk_sl_getSize( objs );
		for( i = 0; i < n; i++ ) {
			mk_com_substExt( obj, sizeof( obj ), mk_sl_at( objs, i ), ".d" );
			mk_fs_remove( mk_sl_at( objs, i ) ); /* object (.o) */
			mk_fs_remove( obj ); /* dependency (.d) */
		}

		if( mk__g_flags & kMkFlag_NoLink_Bit ) {
			mk_fs_remove( bin );
		}
	}

	mk_sl_delete( objs );

	return 1;
}

/* build all the projects */
int mk_bld_makeAllProjects( void ) {
	MkProject proj;

	if( mk__g_flags & kMkFlag_FullClean_Bit ) {
		mk_fs_remove( mk_opt_getObjdirBase() );
	}

	mk_bld_sortProjects( (struct MkProject_s *)0 );

	mk_git_generateInfo();

	for( proj = mk_prj_rootHead(); proj; proj = mk_prj_next( proj ) ) {
		if( !mk_bld_makeProject( proj ) ) {
			return 0;
		}
	}

	mk_bld_runTests();

	return 1;
}
