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
#include "mk-build-projectFS.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-logging.h"
#include "mk-basic-options.h"
#include "mk-basic-stringList.h"
#include "mk-basic-types.h"
#include "mk-build-autolib.h"
#include "mk-build-engine.h"
#include "mk-build-library.h"
#include "mk-build-project.h"
#include "mk-defs-config.h"
#include "mk-frontend.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* determine if the directory name specified is special */
int mk_prjfs_isSpecialDir( MkProject proj, const char *name ) {
	static const char *const specialdirs[] = {
		MK_PLATFORM_OS_NAME_MSWIN,
		MK_PLATFORM_OS_NAME_UWP,
		MK_PLATFORM_OS_NAME_CYGWIN,
		MK_PLATFORM_OS_NAME_LINUX,
		MK_PLATFORM_OS_NAME_MACOSX,
		MK_PLATFORM_OS_NAME_UNIX,

		MK_PLATFORM_CPU_NAME_X86,
		MK_PLATFORM_CPU_NAME_X64,
		MK_PLATFORM_CPU_NAME_ARM,
		MK_PLATFORM_CPU_NAME_AARCH64,
		MK_PLATFORM_CPU_NAME_MIPS,
		MK_PLATFORM_CPU_NAME_PPC,
		MK_PLATFORM_CPU_NAME_WASM,

		"appexec", "exec", "lib", "dylib"
	};
	size_t i;

	/* check the target system */
	switch( proj->sys ) {
#define MK__TRY_PLAT( ProjSys_, Plat_ ) \
	case ProjSys_:                      \
		if( !strcmp( name, Plat_ ) ) {  \
			return 1;                   \
		}                               \
		break

		MK__TRY_PLAT( kMkOS_MSWin, MK_PLATFORM_OS_NAME_MSWIN );
		MK__TRY_PLAT( kMkOS_UWP, MK_PLATFORM_OS_NAME_UWP );
		MK__TRY_PLAT( kMkOS_Cygwin, MK_PLATFORM_OS_NAME_CYGWIN );
		MK__TRY_PLAT( kMkOS_Linux, MK_PLATFORM_OS_NAME_LINUX );
		MK__TRY_PLAT( kMkOS_MacOSX, MK_PLATFORM_OS_NAME_MACOSX );
		MK__TRY_PLAT( kMkOS_Unix, MK_PLATFORM_OS_NAME_UNIX );

#undef MK__TRY_PLAT

	default:
		/*
		 *	NOTE: If you get this message, it's likely because you failed to add
		 *	      the appropriate case statement to this switch. Make sure you
		 *	      port this function too, when adding a new target system.
		 */
		mk_log_errorMsg( mk_com_va( "unknown project system ^E'%d'^&", proj->sys ) );
		break;
	}

	/* check the target architecture */
	switch( proj->arch ) {
#define MK__TRY_PLAT( ProjArch_, Plat_ ) \
	case ProjArch_:                      \
		if( !strcmp( name, Plat_ ) ) {   \
			return 1;                    \
		}                                \
		break

		MK__TRY_PLAT( kMkCPU_X86, MK_PLATFORM_CPU_NAME_X86 );
		MK__TRY_PLAT( kMkCPU_X86_64, MK_PLATFORM_CPU_NAME_X64 );
		MK__TRY_PLAT( kMkCPU_ARM, MK_PLATFORM_CPU_NAME_ARM );
		MK__TRY_PLAT( kMkCPU_AArch64, MK_PLATFORM_CPU_NAME_AARCH64 );
		MK__TRY_PLAT( kMkCPU_PowerPC, MK_PLATFORM_CPU_NAME_PPC );
		MK__TRY_PLAT( kMkCPU_MIPS, MK_PLATFORM_CPU_NAME_MIPS );
		MK__TRY_PLAT( kMkCPU_WebAssembly, MK_PLATFORM_CPU_NAME_WASM );

#undef MK__TRY_PLAT

	default:
		/*
		 *	NOTE: If you get this message, it's likely because you failed to add
		 *	      the appropriate case statement to this switch. Make sure you
		 *	      port this function too, when adding a new target architecture.
		 */
		mk_log_errorMsg( mk_com_va( "unknown project architecture ^E'%d'^&", proj->arch ) );
		break;
	}

	/* check the project type */
	switch( mk_prj_getType( proj ) ) {
	case kMkProjTy_Application:
		if( !strcmp( name, "appexec" ) ) {
			return 1;
		}
		break;
	case kMkProjTy_Program:
		if( !strcmp( name, "exec" ) ) {
			return 1;
		}
		break;
	case kMkProjTy_StaticLib:
		if( !strcmp( name, "lib" ) ) {
			return 1;
		}
		break;
	case kMkProjTy_DynamicLib:
		if( !strcmp( name, "dylib" ) ) {
			return 1;
		}
		break;
	default:
		/*
		 *	NOTE: If you get this message, it's likely because you failed to add
		 *	      the appropriate case statement to this switch. Make sure you
		 *	      port this function too, when adding a new binary target.
		 */
		mk_log_errorMsg( mk_com_va( "unknown project type ^E'%d'^&", proj->type ) );
		break;
	}

	/*
	 * XXX: STUPID HACK! Special directories cannot be included if their
	 *      condition does not match. We're testing for all of the special
	 *      directories again here to ensure this doesn't happen now that any
	 *      non-project-owning directory gets scavanged for source files.
	 */
	for( i = 0; i < sizeof( specialdirs ) / sizeof( specialdirs[0] ); i++ ) {
		if( !strcmp( name, specialdirs[i] ) ) {
			return -1;
		}
	}

	/*
	 *	PLAN: Add a ".freestanding" (or perhaps ".native?") indicator file to
	 *        support OS/kernel development. Would be cool.
	 */

	return 0;
}

/* determine whether the directory is an include directory */
int mk_prjfs_isIncDir( MkProject proj, const char *name ) {
	static const char *const names[] = {
		"inc",
		"incs",
		"include",
		"includes",
		"headers"
	};
	size_t i;

	(void)proj;

	for( i = 0; i < sizeof( names ) / sizeof( names[0] ); i++ ) {
		if( !strcmp( name, names[i] ) ) {
			return 1;
		}
	}

	return 0;
}

/* determine whether the directory is a library directory */
int mk_prjfs_isLibDir( MkProject proj, const char *name ) {
	static const char *const names[] = {
		"lib",
		"libs",
		"library",
		"libraries"
	};
	size_t i;

	(void)proj;

	for( i = 0; i < sizeof( names ) / sizeof( names[0] ); i++ ) {
		if( !strcmp( name, names[i] ) ) {
			return 1;
		}
	}

	return 0;
}

/* determine if the directory is a unit testing directory */
int mk_prjfs_isTestDir( MkProject proj, const char *name ) {
	if( proj ) {
	}

	if( !strcmp( name, "uts" ) || !strcmp( name, "tests" ) || !strcmp( name, "units" ) ) {
		return 1;
	}

	return 0;
}

/* determine whether a directory owns a project indicator file or not */
int mk_prjfs_isDirOwner( const char *path ) {
	MkStat_t s;
	size_t i;

	for( i = 0; i < sizeof( mk__g_ifiles ) / sizeof( mk__g_ifiles[0] ); i++ ) {
		if( stat( mk_com_va( "%s%s", path, mk__g_ifiles[i] ), &s ) != -1 ) {
			return 1;
		}
	}

	return 0;
}

/* enumerate all source files in a directory */
static void mk_prj__enumSourceFilesImpl( MkProject proj, const char *srcdir ) {
	struct dirent *dp;
	char path[PATH_MAX], *p;
	DIR *d;
	int r;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( srcdir != (const char *)0 );

	if( !( d = mk_fs_openDir( srcdir ) ) ) {
		mk_log_error( srcdir, 0, 0, "mk_fs_openDir() call in mk_prjfs_enumSourceFiles() failed" );
		return;
	}

	while( ( ( errno = 0 ), 1 ) && ( dp = mk_fs_readDir( d ) ) != (struct dirent *)0 ) {
		mk_com_strcpy( path, sizeof( path ), srcdir );
		mk_com_strcat( path, sizeof( path ), dp->d_name );

		p = strrchr( dp->d_name, '.' );
		if( p != (char *)0 ) {
			if( *( p + 1 ) == 0 ) {
				continue;
			}

			if( !strcmp( p, ".c" ) || !strcmp( p, ".cc" ) || !strcmp( p, ".cpp" ) || !strcmp( p, ".cxx" ) || !strcmp( p, ".c++" ) || !strcmp( p, ".m" ) || !strcmp( p, ".mm" ) ) {
				if( !mk_fs_isFile( path ) ) {
					continue;
				}

				mk_prj_addSourceFile( proj, path );
				continue;
			}
		}

		if( !mk_fs_isDir( path ) ) {
			continue;
		}

		/* the following tests all require an ending '/' */
		mk_com_strcat( path, sizeof( path ), "/" );

		r = mk_prjfs_isSpecialDir( proj, dp->d_name ); /*can now return -1*/
		if( r != 0 ) {
			if( r == 1 ) {
				mk_prj_addSpecialDir( proj, dp->d_name );
				mk_prjfs_enumSourceFiles( proj, path );
			}
			continue;
		}

		if( mk_prjfs_isIncDir( proj, dp->d_name ) ) {
			mk_front_pushIncDir( path );
			mk_al_managePackage_r( proj->name, proj->sys, path );
			continue;
		}

		if( mk_prjfs_isLibDir( proj, dp->d_name ) ) {
			if( proj->config & kMkProjCfg_Package_Bit ) {
				mk_prj_setOutPath( proj, path );
			}
			mk_front_pushLibDir( path );
			continue;
		}

		if( mk_prjfs_isTestDir( proj, dp->d_name ) ) {
			mk_prjfs_enumTestSourceFiles( proj, path );
			continue;
		}

		if( !mk_prjfs_isDirOwner( path ) ) {
			mk_prjfs_enumSourceFiles( proj, path );
			continue;
		}
	}

	mk_fs_closeDir( d );

	if( errno ) {
		mk_log_error( srcdir, 0, 0, "mk_prjfs_enumSourceFiles() failed" );
	}
}
void mk_prjfs_enumSourceFiles( MkProject proj, const char *srcdir ) {
	mk_dbg_enter( "mk_prjfs_enumSourceFiles(project:\"%s\", srcdir:\"%s\")", proj->name, srcdir );
	mk_prj__enumSourceFilesImpl( proj, srcdir );
	mk_dbg_leave();
}

/* enumerate all unit tests in a directory */
static void mk_prj__enumTestSourceFilesImpl( MkProject proj, const char *srcdir ) {
	struct dirent *dp;
	char path[PATH_MAX], *p;
	DIR *d;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( srcdir != (const char *)0 );

	if( !( d = mk_fs_openDir( srcdir ) ) ) {
		mk_log_error( srcdir, 0, 0, "mk_fs_openDir() call in mk_prjfs_enumTestSourceFiles() failed" );
		return;
	}

	while( ( dp = mk_fs_readDir( d ) ) != (struct dirent *)0 ) {
		mk_com_strcpy( path, sizeof( path ), srcdir );
		mk_com_strcat( path, sizeof( path ), dp->d_name );

		p = strrchr( dp->d_name, '.' );
		if( p ) {
			if( *( p + 1 ) == 0 ) {
				continue;
			}

			if( !strcmp( p, ".c" ) || !strcmp( p, ".cc" ) || !strcmp( p, ".cpp" ) || !strcmp( p, ".cxx" ) || !strcmp( p, ".c++" ) || !strcmp( p, ".m" ) || !strcmp( p, ".mm" ) ) {
				if( !mk_fs_isFile( path ) ) {
					continue;
				}

				mk_prj_addTestSourceFile( proj, path );
				continue;
			}
		}

		if( !mk_fs_isDir( path ) ) {
			errno = 0;
			continue;
		}

		if( mk_prjfs_isSpecialDir( proj, dp->d_name ) ) {
			mk_prj_addSpecialDir( proj, dp->d_name );
			mk_com_strcat( path, sizeof( path ), "/" ); /* ending '/' is necessary */
			mk_prjfs_enumTestSourceFiles( proj, path );
		}
	}

	mk_fs_closeDir( d );

	if( errno ) {
		mk_log_error( srcdir, 0, 0, "mk_prjfs_enumTestSourceFiles() failed" );
	}
}
void mk_prjfs_enumTestSourceFiles( MkProject proj, const char *srcdir ) {
	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( proj->name != (const char *)0 );

	mk_dbg_enter( "mk_prjfs_enumTestSourceFiles(project:\"%s\", srcdir:\"%s\")", proj->name, srcdir );
	mk_prj__enumTestSourceFilesImpl( proj, srcdir );
	mk_dbg_leave();
}

/* calculate the name of a project based on a directory or file */
int mk_prjfs_calcName( MkProject proj, const char *path, const char *file ) {
	size_t i;
	FILE *f;
	char buf[256], cwd[256], *p;

	MK_ASSERT( proj != (MkProject)0 );
	MK_ASSERT( path != (const char *)0 );

	/* read a single line from the file */
	if( file != (const char *)0 ) {
		if( !( f = fopen( file, "r" ) ) ) {
			mk_log_error( file, 0, 0, "fopen() call in mk_prjfs_calcName() failed" );
			return 0;
		}

		buf[0] = 0;
		fgets( buf, sizeof( buf ), f );

		fclose( f );

		/* strip the line of whitespace on both ends */
		i = 0;
		while( buf[i] <= ' ' && buf[i] != 0 ) {
			i++;
		}

		for( p = &buf[i]; *p > ' '; p++ )
			;
		if( *p != 0 ) {
			*p = 0;
		}
	} else {
#if 0
		/*
		 * XXX: Why did I use 'cwd' instead of path? ...
		 */
		mk_fs_getCWD(cwd, sizeof(cwd));
#else
		mk_com_strcpy( cwd, sizeof( cwd ), path );
#endif

		if( !( p = strrchr( cwd, '/' ) ) ) {
			p = cwd;
		} else if( *( p + 1 ) == 0 ) {
			if( strcmp( cwd, "/" ) != 0 ) {
				*p = 0;
				p  = strrchr( cwd, '/' );
				if( !p ) {
					p = cwd;
				}
			}
		}

		MK_ASSERT( p != (char *)0 );

		if( p != cwd && *p == '/' ) {
			p++;
		}

		mk_com_strcpy( buf, sizeof( buf ), p );
		i = 0;
	}

	/* if no name was specified in the file, use the directory name */
	if( !buf[i] ) {
		mk_com_strcpy( buf, sizeof( buf ), path );

		/* mk_fs_realPath() always adds a '/' at the end of a directory */
		p = strrchr( buf, '/' );

		/* ensure this is the case (debug-mode only) */
		MK_ASSERT( p != (char *)0 );
		MK_ASSERT( *( p + 1 ) == 0 );
		*p = 0; /* remove this ending '/' to not conflict with the following */

		/* if there's an ending '/' (always should be), mark the character
		   past that as the start of the directory name */
		i = ( ( p = strrchr( buf, '/' ) ) != (char *)0 ) ? ( p - buf ) + 1 : 0;
	}

	/* set the project's name */
	mk_prj_setName( proj, &buf[i] );

	/* done */
	return 1;
}

/* add a single project using 'file' for information */
MkProject mk_prjfs_add( MkProject prnt, const char *path, const char *file, int type ) {
	MkProject proj;

	MK_ASSERT( path != (const char *)0 );

	/* create the project */
	proj = mk_prj_new( prnt );

	/* calculate the appropriate name for the project */
	if( !mk_prjfs_calcName( proj, path, file ) ) {
		mk_prj_delete( proj );
		return (MkProject)0;
	}

	/* prepare the passed settings of the project */
	mk_prj_setPath( proj, path );
	mk_prj_setType( proj, type );
	mk_bld_getBinName( proj, (char *)0, 0 );

	if( proj->type == kMkProjTy_StaticLib || proj->type == kMkProjTy_DynamicLib ) {
		proj->lib       = mk_lib_new();
		proj->lib->proj = proj;
		mk_lib_setName( proj->lib, proj->name );
		mk_lib_setFlags( proj->lib, proj->sys, mk_com_va( "\"%s\"", proj->binname ) );
	}

	/* add the sources for the project */
	mk_prjfs_enumSourceFiles( proj, path );

	/* return the project */
	return proj;
}

/* find all projects within a specific packages directory */
void mk_prjfs_findPackages( const char *pkgdir ) {
	struct dirent *dp;
	MkProject proj;
	char path[PATH_MAX];
	DIR *d;

	/* opne the directory to find packages */
	if( !( d = mk_fs_openDir( pkgdir ) ) ) {
		mk_log_fatalError( pkgdir );
	}

	/* run through each entry in the directory */
	while( ( dp = mk_fs_readDir( d ) ) != (struct dirent *)0 ) {
		if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) ) {
			continue;
		}

		/* validate the directory */
		if( !mk_fs_realPath( mk_com_va( "%s/%s/", pkgdir, dp->d_name ), path,
		        sizeof( path ) ) ) {
			errno = 0;
			continue;
		}

		/* this is a project directory; treat as a static library */
		proj = mk_prjfs_add( (MkProject)0, path, (const char *)0,
		    kMkProjTy_StaticLib );
		if( !proj ) {
			continue;
		}
	}

	/* close this directory */
	mk_fs_closeDir( d );

	/* if an error occurred, display it */
	if( errno ) {
		mk_log_fatalError( mk_com_va( "readdir(\"%s\") failed", pkgdir ) );
	}
}

/* find all dlls */
void mk_prjfs_findDynamicLibs( const char *dllsdir ) {
	struct dirent *dp;
	MkProject proj;
	char path[PATH_MAX];
	DIR *d;

	/* opne the directory to find packages */
	if( !( d = mk_fs_openDir( dllsdir ) ) ) {
		mk_log_fatalError( dllsdir );
	}

	/* run through each entry in the directory */
	while( ( dp = mk_fs_readDir( d ) ) != (struct dirent *)0 ) {
		if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) ) {
			continue;
		}

		/* validate the directory */
		if( !mk_fs_realPath( mk_com_va( "%s/%s/", dllsdir, dp->d_name ), path,
		        sizeof( path ) ) ) {
			errno = 0;
			continue;
		}

		/* this is a project directory; treat as a static library */
		proj = mk_prjfs_add( (MkProject)0, path, (const char *)0,
		    kMkProjTy_DynamicLib );
		if( !proj ) {
			continue;
		}
	}

	/* close this directory */
	mk_fs_closeDir( d );

	/* if an error occurred, display it */
	if( errno ) {
		mk_log_fatalError( mk_com_va( "readdir(\"%s\") failed", dllsdir ) );
	}
}

/* find all tools */
void mk_prjfs_findTools( const char *tooldir ) {
	struct dirent *dp;
	MkProject proj;
	char path[PATH_MAX];
	DIR *d;

	/* opne the directory to find packages */
	if( !( d = mk_fs_openDir( tooldir ) ) ) {
		mk_log_fatalError( tooldir );
	}

	/* run through each entry in the directory */
	while( ( dp = mk_fs_readDir( d ) ) != (struct dirent *)0 ) {
		if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) ) {
			continue;
		}

		/* validate the directory */
		if( !mk_fs_realPath( mk_com_va( "%s/%s/", tooldir, dp->d_name ), path,
		        sizeof( path ) ) ) {
			errno = 0;
			continue;
		}

		/* this is a project directory; treat as a static library */
		proj = mk_prjfs_add( (MkProject)0, path, (const char *)0,
		    kMkProjTy_Program );
		if( !proj ) {
			continue;
		}
	}

	/* close this directory */
	mk_fs_closeDir( d );

	/* if an error occurred, display it */
	if( errno ) {
		mk_log_fatalError( mk_com_va( "readdir(\"%s\") failed", tooldir ) );
	}
}

/* find all projects within a specific source directory, adding them as children
   to the 'prnt' project. */
void mk_prjfs_findProjects( MkProject prnt, const char *srcdir ) {
	static const char *libs[] = {
		".libs", ".user.libs",
		".mk-libs", ".user.mk-libs",
		"mk-libs.txt", "mk-libs.user.txt"
	};
	static char buf[32768];
	struct dirent *dp;
	MkStat_t s;
	MkProject proj;
	size_t i, j;
	FILE *f;
	char path[PATH_MAX], file[PATH_MAX], *p;
	DIR *d;

	proj = (MkProject)0;

	/* open the source directory to add projects */
	if( !( d = mk_fs_openDir( srcdir ) ) ) {
		mk_log_fatalError( srcdir );
	}

	/* run through each entry in the directory */
	while( ( dp = mk_fs_readDir( d ) ) != (struct dirent *)0 ) {
		if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) ) {
			continue;
		}

		/* make sure this is a directory; get the real (absolute) path */
		if( !mk_fs_realPath( mk_com_va( "%s/%s/", srcdir, dp->d_name ), path,
		        sizeof( path ) ) ) {
			errno = 0;
			continue;
		}

		/* run through each indicator file test (.executable, ...) */
		for( i = 0; i < sizeof( mk__g_ifiles ) / sizeof( mk__g_ifiles[0] ); i++ ) {
			mk_com_strcpy( file, sizeof( file ), mk_com_va( "%s%s", path, mk__g_ifiles[i].name ) );

			/* make sure this file exists */
			if( !mk_fs_isFile( file ) ) {
				errno = 0;
				continue;
			}

			/* add the project file */
			proj = mk_prjfs_add( prnt, path, file, mk__g_ifiles[i].type );
			if( !proj ) {
				continue;
			}

			/* if this entry contains a '.libs' file, then add to the project */
			for( j = 0; j < sizeof( libs ) / sizeof( libs[0] ); j++ ) {
				mk_com_strcpy( file, sizeof( file ), mk_com_va( "%s/%s", srcdir, libs[j] ) );

				if( stat( file, &s ) != 0 ) {
					continue;
				}

				f = fopen( file, "r" );
				if( f ) {
					if( fgets( buf, sizeof( buf ), f ) != (char *)0 ) {
						p = strchr( buf, '\r' );
						if( p ) {
							*p = '\0';
						}

						p = strchr( buf, '\n' );
						if( p ) {
							*p = '\0';
						}

						mk_prj_appendExtraLibs( proj, buf );
					} else {
						mk_log_error( file, 1, 0, "failed to read line" );
					}

					fclose( f );
					f = (FILE *)0;
				} else {
					mk_log_error( file, 0, 0, "failed to open for reading" );
				}
			}

			/* find sub-projects */
			mk_prjfs_findProjects( proj, path );
		}
	}

	/* close this directory */
	mk_fs_closeDir( d );

	/* if an error occurred, display it */
	if( errno ) {
		mk_log_fatalError( mk_com_va( "readdir(\"%s\") failed", srcdir ) );
	}
}

/* find the root directories, adding projects for any of the 'srcdirs' */
void mk_prjfs_findRootDirs( MkStrList srcdirs, MkStrList incdirs, MkStrList libdirs, MkStrList pkgdirs, MkStrList tooldirs, MkStrList dllsdirs ) {
	struct {
		const char *name;
		MkStrList dst;
	} tests[] = {
		{ "src", (MkStrList)0 }, { "source", (MkStrList)0 },
		{ "code", (MkStrList)0 },
		{ "projects", (MkStrList)0 },
		{ "inc", (MkStrList)1 }, { "include", (MkStrList)1 },
		{ "headers", (MkStrList)1 },
		{ "includes", (MkStrList)1 },
		{ "lib", (MkStrList)2 }, { "library", (MkStrList)2 },
		{ "libraries", (MkStrList)2 },
		{ "libs", (MkStrList)2 },
		{ "pkg", (MkStrList)3 }, { "packages", (MkStrList)3 },
		{ "repository", (MkStrList)3 },
		{ "repo", (MkStrList)3 },
		{ "tools", (MkStrList)4 }, { "games", (MkStrList)4 },
		{ "apps", (MkStrList)4 }, { "dlls", (MkStrList)5 },
		{ "sdk/include", (MkStrList)1 },
		{ "sdk/lib", (MkStrList)2 },
		{ "sdk/packages", (MkStrList)3 },
		{ "sdk/tools", (MkStrList)4 },
		{ "sdk/dlls", (MkStrList)5 }
	};
	static const char *workspaces[] = {
		".workspace", ".user.workspace",
		".mk-workspace", ".user.mk-workspace",
		"mk-workspace.txt", "mk-workspace.user.txt"
	};
	static char buf[32768];
	MkStat_t s;
	MkProject proj;
	size_t i, n;
	FILE *f;
	char *p;
	char path[256];

	MK_ASSERT( srcdirs != (MkStrList)0 );
	MK_ASSERT( incdirs != (MkStrList)0 );
	MK_ASSERT( libdirs != (MkStrList)0 );
	MK_ASSERT( pkgdirs != (MkStrList)0 );
	MK_ASSERT( tooldirs != (MkStrList)0 );
	MK_ASSERT( dllsdirs != (MkStrList)0 );

	/* "initializer element is not computable at load time" warning -- fix */
	for( i = 0; i < sizeof( tests ) / sizeof( tests[0] ); i++ ) {
		if( tests[i].dst == (MkStrList)0 ) {
			tests[i].dst = srcdirs;
		} else if( tests[i].dst == (MkStrList)1 ) {
			tests[i].dst = incdirs;
		} else if( tests[i].dst == (MkStrList)2 ) {
			tests[i].dst = libdirs;
		} else if( tests[i].dst == (MkStrList)4 ) {
			tests[i].dst = tooldirs;
		} else if( tests[i].dst == (MkStrList)5 ) {
			tests[i].dst = dllsdirs;
		} else {
			MK_ASSERT( tests[i].dst == (MkStrList)3 );
			tests[i].dst = pkgdirs;
		}
	}

	/* run through each of the tests in the 'tests' array to see which
	   directories will be used in the build... */
	for( i = 0; i < sizeof( tests ) / sizeof( tests[0] ); i++ ) {
		MK_ASSERT( tests[i].name != (const char *)0 );
		MK_ASSERT( tests[i].dst != (MkStrList)0 );

		if( mk_sl_getSize( tests[i].dst ) > 0 ) {
			continue;
		}

		if( stat( tests[i].name, &s ) == -1 ) {
			continue;
		}
		if( ~s.st_mode & S_IFDIR ) {
			continue;
		}

		mk_sl_pushBack( tests[i].dst, tests[i].name );
	}

	/* use the .workspace file to add extra directories */
	for( i = 0; i < sizeof( workspaces ) / sizeof( workspaces[0] ); i++ ) {
		f = fopen( workspaces[i], "r" );
		if( f ) {
			/* every line of the file specifies another directory */
			while( fgets( buf, sizeof( buf ), f ) != (char *)0 ) {
				p = strchr( buf, '\r' );
				if( p ) {
					*p = '\0';
				}

				p = strchr( buf, '\n' );
				if( p ) {
					*p = '\0';
				}

				if( !strncmp( buf, "src:", 4 ) ) {
					mk_front_pushSrcDir( &buf[4] );
					continue;
				}
				if( !strncmp( buf, "inc:", 4 ) ) {
					mk_front_pushIncDir( &buf[4] );
					continue;
				}
				if( !strncmp( buf, "lib:", 4 ) ) {
					mk_front_pushLibDir( &buf[4] );
					continue;
				}
				if( !strncmp( buf, "pkg:", 4 ) ) {
					mk_front_pushPkgDir( &buf[4] );
					continue;
				}
				if( !strncmp( buf, "tool:", 5 ) ) {
					mk_front_pushToolDir( &buf[5] );
					continue;
				}
				if( !strncmp( buf, "dlls:", 5 ) ) {
					mk_front_pushDynamicLibsDir( &buf[5] );
					continue;
				}
			}

			fclose( f );
		}
	}

	errno = 0;

	/* find packages first */
	n = mk_sl_getSize( pkgdirs );
	for( i = 0; i < n; i++ ) {
		mk_prjfs_findPackages( mk_sl_at( pkgdirs, i ) );
	}

	/* find dlls */
	n = mk_sl_getSize( dllsdirs );
	for( i = 0; i < n; i++ ) {
		mk_prjfs_findDynamicLibs( mk_sl_at( dllsdirs, i ) );
	}

	/* find tools next */
	n = mk_sl_getSize( tooldirs );
	for( i = 0; i < n; i++ ) {
		mk_prjfs_findTools( mk_sl_at( tooldirs, i ) );
	}

	/* we'll try to find projects in the source directories */
	n = mk_sl_getSize( srcdirs );
	for( i = 0; i < n; i++ ) {
		mk_prjfs_findProjects( (MkProject)0, mk_sl_at( srcdirs, i ) );
	}

	/* if we found no projects in srcdirs then treat the current directory as a
	   project */
	if( !mk_prj_rootHead() && n > 0 ) {
		if( !mk_fs_realPath( "./", path, sizeof( path ) ) ) {
			return;
		}

		proj = mk_prj_new( (MkProject)0 );

		if( !mk_prjfs_calcName( proj, path, (const char *)0 ) ) {
			mk_prj_delete( proj );
			return;
		}

		mk_com_strcat( path, sizeof( path ), mk_com_va( "%s/", mk_sl_at( srcdirs, 0 ) ) );

		mk_prj_setPath( proj, path );
		mk_prj_setType( proj, kMkProjTy_Program );

		mk_prjfs_enumSourceFiles( proj, path );
	}
}

/* create the object directories for a particular project */
int mk_prjfs_makeObjDirs( MkProject proj ) {
	const char *typename = "";
	size_t i, n;
	char objdir[PATH_MAX], cwd[PATH_MAX];

	MK_ASSERT( proj != (MkProject)0 );

	if( getcwd( cwd, sizeof( cwd ) ) == (char *)0 ) {
		mk_log_errorMsg( "getcwd() failed" );
		return 0;
	}

	n = mk_com_strlen( cwd );

	switch( mk_prj_getType( proj ) ) {
	case kMkProjTy_Application:
		typename = "appexec";
		break;
	case kMkProjTy_Program:
		typename = "exec";
		break;
	case kMkProjTy_StaticLib:
		typename = "lib";
		break;
	case kMkProjTy_DynamicLib:
		typename = "dylib";
		break;
	default:
		MK_ASSERT_MSG( 0, "project type is invalid" );
		break;
	}

#if MK_SECLIB
#	define snprintf sprintf_s
#endif
	snprintf( objdir, sizeof( objdir ) - 1, "%s/%s/%s/%s",
	    mk_opt_getObjdirBase(),
	    mk_opt_getConfigName(), typename,
	    &mk_prj_getPath( proj )[n] );
	objdir[sizeof( objdir ) - 1] = 0;
#if MK_SECLIB
#	undef snprintf
#endif

	if( ~mk__g_flags & kMkFlag_NoCompile_Bit ) {
		mk_fs_makeDirs( objdir );
	}

#if 0
	n = mk_prj_numSpecialDirs(proj);
	for( i=0; i<n; i++ )
		mk_fs_makeDirs(mk_com_va("%s%s/", objdir, mk_prj_specialDirAt(proj, i)));
#else
	n = mk_sl_getSize( proj->sources );
	for( i = 0; i < n; i++ ) {
		char rel[PATH_MAX];
		char *p;
#	ifdef _WIN32
		char *q;
#	endif

		mk_com_relPath( rel, sizeof( rel ), proj->path, mk_sl_at( proj->sources, i ) );
		p = strrchr( rel, '/' );
#	ifdef _WIN32
		q = strrchr( rel, '\\' );
		if( q != NULL && q > p ) {
			p = q;
		}
#	endif
		if( p != NULL ) {
			*p = '\0';
		}
		mk_sl_pushBack( proj->srcdirs, rel );
	}
#endif

	mk_sl_makeUnique( proj->srcdirs );
	if( ~mk__g_flags & kMkFlag_NoCompile_Bit ) {
		n = mk_sl_getSize( proj->srcdirs );
		for( i = 0; i < n; i++ ) {
			mk_fs_makeDirs( mk_com_va( "%s%s/", objdir, mk_sl_at( proj->srcdirs, i ) ) );
		}
	}

	return 1;
}
