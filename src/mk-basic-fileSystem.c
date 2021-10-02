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
#include "mk-basic-fileSystem.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-logging.h"
#include "mk-basic-stringList.h"
#include "mk-basic-types.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if MK_HAS_UNISTD
#	include <unistd.h>
#else
#	include <direct.h>
#	include <io.h>
#endif

#if MK_HAS_DIRENT
#	include <dirent.h>
#else
#	include "dirent.h" /* user-provided replacement */
#endif

MkStrList mk__g_fs_dirstack = (MkStrList)0;

/* initialize file system */
void mk_fs_init( void ) {
	atexit( mk_fs_fini );
}

/* deinitialize file system */
void mk_fs_fini( void ) {
	mk_sl_delete( mk__g_fs_dirstack );
	mk__g_fs_dirstack = (MkStrList)0;
}

/* retrieve the current directory */
char *mk_fs_getCWD( char *cwd, size_t n ) {
	char *p;

	MK_ASSERT( cwd != (char *)0 );
	MK_ASSERT( n > 1 );

#if MK_VC_VER
	if( !( p = _getcwd( cwd, n ) ) ) {
		mk_log_fatalError( "getcwd() failed" );
	}
#else
	if( !( p = getcwd( cwd, n ) ) ) {
		mk_log_fatalError( "getcwd() failed" );
	}
#endif

#if MK_WINDOWS_ENABLED
	while( ( p = strchr( p, '\\' ) ) != (char *)0 ) {
		*p = '/';
	}
#endif

	p = strrchr( cwd, '/' );
	if( ( p && *( p + 1 ) != '\0' ) || !p ) {
		mk_com_strcat( cwd, n, "/" );
	}

	return cwd;
}

/* enter a directory (uses directory stack) */
int mk_fs_enter( const char *path ) {
	char cwd[PATH_MAX];

	MK_ASSERT( path != (const char *)0 );

	mk_dbg_enter( "mk_fs_enter(\"%s\")", path );

	mk_fs_getCWD( cwd, sizeof( cwd ) );

	if( !mk__g_fs_dirstack ) {
		mk__g_fs_dirstack = mk_sl_new();
	}

	mk_sl_pushBack( mk__g_fs_dirstack, cwd );

	if( chdir( path ) == -1 ) {
		mk_log_errorMsg( mk_com_va( "chdir(^F\"%s\"^&) failed", path ) );
		if( chdir( cwd ) == -1 ) {
			mk_log_fatalError( "failed to restore current directory" );
		}

		return 0;
	}

	return 1;
}

/* exit the current directory (uses directory stack) */
void mk_fs_leave( void ) {
	size_t i;

	MK_ASSERT( mk__g_fs_dirstack != (MkStrList)0 );
	MK_ASSERT( mk_sl_getSize( mk__g_fs_dirstack ) > 0 );

	mk_dbg_leave();

	i = mk_sl_getSize( mk__g_fs_dirstack ) - 1;

	if( chdir( mk_sl_at( mk__g_fs_dirstack, i ) ) == -1 ) {
		mk_log_fatalError( mk_com_va( "chdir(\"%s\") failed",
		    mk_sl_at( mk__g_fs_dirstack, i ) ) );
	}

	mk_sl_set( mk__g_fs_dirstack, i, (const char *)0 );
	mk_sl_resize( mk__g_fs_dirstack, i );
}

/* determine whether the path specified is a file. */
int mk_fs_isFile( const char *path ) {
	MkStat_t s;

	if( stat( path, &s ) == -1 ) {
		errno = 0;
		return 0;
	}

	if( ~s.st_mode & S_IFREG ) {
		/*errno = s.st_mode & S_IFDIR ? EISDIR : EBADF;*/
		return 0;
	}

	return 1;
}

/* determine whether the path specified is a directory. */
int mk_fs_isDir( const char *path ) {
	static char temp[PATH_MAX];
	MkStat_t s;
	const char *p;

	p = strrchr( path, '/' );
	if( p && *( p + 1 ) == '\0' ) {
		mk_com_strncpy( temp, sizeof( temp ), path, p - path );
		temp[p - path] = '\0';
		p              = &temp[0];
	} else {
		p = path;
	}

	if( stat( p, &s ) == -1 ) {
		errno = 0;
		return 0;
	}

	if( ~s.st_mode & S_IFDIR ) {
		/*errno = ENOTDIR;*/
		return 0;
	}

	return 1;
}

/* create a series of directories (e.g., a/b/c/d/...) */
void mk_fs_makeDirs( const char *dirs ) {
	/*
	 *	! This is old code !
	 *	Just ignore bad practices, mmkay?
	 */
	const char *p;
	char buf[PATH_MAX], *path;
	int ishidden;

	/* ignore the root directory */
	if( dirs[0] == '/' ) {
		buf[0] = dirs[0];
		path   = &buf[1];
		p      = &dirs[1];
	} else if( dirs[1] == ':' && dirs[2] == '/' ) {
		buf[0] = dirs[0];
		buf[1] = dirs[1];
		buf[2] = dirs[2];
		path   = &buf[3];
		p      = &dirs[3];
	} else {
		path = &buf[0];
		p    = &dirs[0];
	}

	/* not hidden unless path component begins with '.' */
	ishidden = (int)( dirs[0] == '.' );

	/* make each directory, one by one */
	while( 1 ) {
		if( *p == '/' || *p == 0 ) {
			*path = 0;

			errno = 0;
#if MK_WINDOWS_ENABLED
			mkdir( buf );
#else
			mkdir( buf, 0740 );
#endif
			if( errno && errno != EEXIST ) {
				mk_log_fatalError( mk_com_va( "couldn't create directory \"%s\"", buf ) );
			}

#if MK_WINDOWS_ENABLED
			if( ishidden ) {
				SetFileAttributesA( buf, FILE_ATTRIBUTE_HIDDEN );
			}
#endif

			if( p[0] == '/' && p[1] == '.' ) {
				ishidden = 1;
			} else {
				ishidden = 0;
			}

			if( !( *path++ = *p++ ) ) {
				return;
			} else if( *p == '\0' ) { /* handle a '/' ending */
				return;
			}
		} else {
			*path++ = *p++;
		}

		if( path == &buf[sizeof( buf ) - 1] )
			mk_log_fatalError( "path is too long" );
	}
}

/* find the real path to a file */
#if MK_WINDOWS_ENABLED
char *mk_fs_realPath( const char *filename, char *resolvedname, size_t maxn ) {
	static char buf[PATH_MAX];
	size_t i;
	DWORD r;

	if( !( r = GetFullPathNameA( filename, sizeof( buf ), buf, (char **)0 ) ) ) {
		errno = ENOSYS;
		return (char *)0;
	} else if( r >= (DWORD)maxn ) {
		errno = ERANGE;
		return (char *)0;
	}

	for( i = 0; i < sizeof( buf ); i++ ) {
		if( !buf[i] ) {
			break;
		}

		if( buf[i] == '\\' ) {
			buf[i] = '/';
		}
	}

	if( buf[1] == ':' ) {
		if( buf[0] >= 'A' && buf[0] <= 'Z' ) {
			buf[0] = buf[0] - 'A' + 'a';
		}
	}

	strncpy( resolvedname, buf, maxn - 1 );
	resolvedname[maxn - 1] = 0;

	return resolvedname;
}
#else /*#elif __linux__||__linux||linux*/
char *mk_fs_realPath( const char *filename, char *resolvedname, size_t maxn ) {
	static char buf[PATH_MAX + 1];

	if( maxn > PATH_MAX ) {
		if( !realpath( filename, resolvedname ) ) {
			return (char *)0;
		}

		resolvedname[PATH_MAX] = 0;
		return resolvedname;
	}

	if( !realpath( filename, buf ) ) {
		return (char *)0;
	}

	buf[PATH_MAX] = 0;
	strncpy( resolvedname, buf, maxn );

	if( mk_fs_isDir( resolvedname ) ) {
		mk_com_strcat( resolvedname, maxn, "/" );
	}

	resolvedname[maxn - 1] = 0;
	return resolvedname;
}
#endif

DIR *mk_fs_openDir( const char *path ) {
	static char buf[PATH_MAX];
	const char *p, *usepath;
	DIR *d;

	mk_dbg_outf( "mk_fs_openDir(\"%s\")\n", path );

	p = strrchr( path, '/' );
	if( p && *( p + 1 ) == '\0' ) {
		mk_com_strncpy( buf, sizeof( buf ), path, ( size_t )( p - path ) );
		usepath = buf;
	} else {
		usepath = path;
	}

	if( !( d = opendir( usepath ) ) ) {
		const char *es;
		int e;

		e  = errno;
		es = strerror( e );

		mk_dbg_outf( "\tKO: %i (%s)\n", e, es );
		return (DIR *)0;
	}

	mk_dbg_outf( "\tOK: #%x\n", (unsigned)(size_t)d );
	return d;
}
DIR *mk_fs_closeDir( DIR *p ) {
	int e;

	e = errno;
	mk_dbg_outf( "mk_fs_closeDir(#%x)\n", (unsigned)(size_t)p );
	if( p != (DIR *)0 ) {
		closedir( p );
	}
	errno = e;

	return (DIR *)0;
}
struct dirent *mk_fs_readDir( DIR *d ) {
	struct dirent *dp;

	/* Okay... how is errno getting set? From what? */
	if( errno != 0 ) {
		mk_log_error( NULL, 0, "mk_fs_readDir", NULL );
	}

	for(;;) {
		dp = readdir( d );
		if( dp != (struct dirent *)0 ) {
			if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) ) {
				continue;
			}
		}

		break;
	}

	if( dp != (struct dirent *)0 || errno == ENOTDIR ) {
		errno = 0;
	}

	return dp;
}

void mk_fs_remove( const char *path ) {
	if( mk_fs_isDir( path ) ) {
		DIR *d;
		struct dirent *dp;
		char *filepath;
		char *filepath_namepart;

		filepath = (char *)mk_com_memory( NULL, PATH_MAX );
		mk_com_strcpy( filepath, PATH_MAX, path );

		filepath_namepart = strchr( filepath, '\0' );
		if( filepath_namepart > filepath ) {
			char dirsep;

			dirsep = *( filepath_namepart - 1 );
			if( dirsep != '/' && dirsep != '\\' ) {
				*filepath_namepart++ = '/';
			}
		}

		d = mk_fs_openDir( path );
		while( ( dp = mk_fs_readDir( d ) ) != NULL ) {
			mk_com_strcpy( filepath_namepart,
			    PATH_MAX - (size_t)filepath_namepart, dp->d_name );
			mk_fs_remove( filepath );
		}
		mk_fs_closeDir( d );

		filepath = (char *)mk_com_memory( (void *)filepath, 0 );

		mk_dbg_outf( "Deleting directory \"%s\"...\n", path );
		rmdir( path );
	} else {
		errno = 0;

		mk_dbg_outf( "Deleting file \"%s\"...\n", path );
		remove( path );
	}
}

MkLanguage mk_fs_getLanguage( const char *filename ) {
#define EQ( X_, Y_ ) strcmp( ( X_ ), ( Y_ ) ) == 0
	static const MkLanguage defaultCStandard      = kMkLanguage_C_Default;
	static const MkLanguage defaultCxxStandard    = kMkLanguage_Cxx_Default;
	static const MkLanguage defaultObjCStandard   = kMkLanguage_ObjC;
	static const MkLanguage defaultObjCxxStandard = kMkLanguage_ObjCxx;
	const char *p;

	p = strrchr( filename, '.' );
	if( !p ) {
		p = filename;
	}

	if( EQ( p, ".c" ) || EQ( p, ".i" ) ) {
		return defaultCStandard;
	}
	if( EQ( p, ".cc" ) || EQ( p, ".cxx" ) || EQ( p, ".cpp" ) || EQ( p, ".c++" ) || EQ( p, ".ii" ) ) {
		return defaultCxxStandard;
	}
	if( EQ( p, ".m" ) || EQ( p, ".mi" ) ) {
		return defaultObjCStandard;
	}
	if( EQ( p, ".mm" ) || EQ( p, ".mii" ) ) {
		return defaultObjCxxStandard;
	}
	if( EQ( p, ".s" ) ) {
		return kMkLanguage_Assembly;
	}

	if( EQ( p, ".c89" ) || EQ( p, ".c90" ) ) {
		return kMkLanguage_C89;
	}
	if( EQ( p, ".c99" ) ) {
		return kMkLanguage_C99;
	}
	if( EQ( p, ".c11" ) || EQ( p, ".c1x" ) ) {
		return kMkLanguage_C11;
	}

	if( EQ( p, ".cc98" ) || EQ( p, ".cxx98" ) || EQ( p, ".cpp98" ) || EQ( p, ".c++98" ) ) {
		return kMkLanguage_Cxx98;
	}
	if( EQ( p, ".cc03" ) || EQ( p, ".cxx03" ) || EQ( p, ".cpp03" ) || EQ( p, ".c++03" ) ) {
		return kMkLanguage_Cxx03;
	}
	if( EQ( p, ".cc11" ) || EQ( p, ".cxx11" ) || EQ( p, ".cpp11" ) || EQ( p, ".c++11" ) ) {
		return kMkLanguage_Cxx11;
	}
	if( EQ( p, ".cc14" ) || EQ( p, ".cxx14" ) || EQ( p, ".cpp14" ) || EQ( p, ".c++14" ) ) {
		return kMkLanguage_Cxx14;
	}
	if( EQ( p, ".cc17" ) || EQ( p, ".cxx17" ) || EQ( p, ".cpp17" ) || EQ( p, ".c++17" ) ) {
		return kMkLanguage_Cxx17;
	}
	if( EQ( p, ".cc20" ) || EQ( p, ".cxx20" ) || EQ( p, ".cpp20" ) || EQ( p, ".c++20" ) ) {
		return kMkLanguage_Cxx20;
	}

	return kMkLanguage_Unknown;
}
