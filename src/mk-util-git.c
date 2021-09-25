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
#include "mk-util-git.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-logging.h"
#include "mk-basic-memory.h"
#include "mk-basic-options.h"
#include "mk-basic-sourceBuffer.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

struct MkGitInfo_s {
	char *pszBranchFile;
	char *pszCommit;
	char *pszTimestamp;
};

/* find the nearest ".git" directory at or below the current path */
char *mk_git_findRoot( void ) {
	/* szDir always ends with '/'; szGitDir is szDir + ".git" */
	char szDir[PATH_MAX], szGitDir[PATH_MAX + 8];
	/* pszDirPos always points to the last '/' in szDir */
	char *pszDirPos;

	/* start searching from the current directory */
	if( !mk_fs_getCWD( szDir, (int)sizeof( szDir ) - 1 ) ) {
		return (char *)0;
	}

	/* find the NUL terminator */
	pszDirPos = strchr( szDir, '\0' );

	/* quick sanity check */
	if( pszDirPos == &szDir[0] || !pszDirPos ) {
		mk_dbg_outf( "mk_git_findRoot: mk_fs_getCWD returned an empty directory path\n" );
		return (char *)0;
	}

	/* `szDir` must end with '/' and `pszDirPos` must point to the last '/' */
	if( *( pszDirPos - 1 ) != '/' ) {
		pszDirPos[0] = '/';
		pszDirPos[1] = '\0';
	} else {
		--pszDirPos;
	}

	mk_dbg_outf( "Searching for .git directory, starting from: <%s>\n", szDir );

	/* find the .git directory (if successful, this part returns) */
	do {
		/* construct the .git path name */
		mk_com_strcpy( szGitDir, sizeof( szGitDir ), szDir );
		mk_com_strcat( szGitDir, sizeof( szGitDir ), ".git" );
		mk_dbg_outf( "\tTrying: <%s>\n", szGitDir );

		/* if this is a valid .git directory, then done */
		if( mk_fs_isDir( szGitDir ) ) {
			mk_dbg_outf( "\t\tFound!\n" );
			return mk_com_dup( (char *)0, szGitDir );
		}

		/* find the beginning of the next directory up */
		while( --pszDirPos > &szDir[0] ) {
			if( *pszDirPos == '/' ) {
				pszDirPos[1] = '\0';
				break;
			}
		}
	} while( pszDirPos != &szDir[0] );

	/* no .git directory found */
	mk_dbg_outf( "\tNo .git found\n" );
	return (char *)0;
}
/* find the file belonging to the current branch of the given .git directory */
char *mk_git_findBranchPath( const char *pszGitDir ) {
	static const char szRefPrefix[] = "ref: ";
	static const size_t cRefPrefix  = sizeof( szRefPrefix ) - 1;
	MkBuffer buf;
	char szHeadFile[PATH_MAX];
	char szRefLine[512];

	MK_ASSERT( pszGitDir != (const char *)0 && *pszGitDir != '\0' );

	mk_com_strcpy( szHeadFile, sizeof( szHeadFile ), pszGitDir );
	mk_com_strcat( szHeadFile, sizeof( szHeadFile ), "/HEAD" );

	mk_dbg_outf( "mk_git_findBranchPath: %s: <%s>\n", pszGitDir, szHeadFile );
	if( !( buf = mk_buf_loadFile( szHeadFile ) ) ) {
		return (char *)0;
	}

	mk_buf_readLine( buf, szRefLine, sizeof( szRefLine ) );
	mk_dbg_outf( "mk_git_findBranchPath: %s: <%s>\n", szHeadFile, szRefLine );
	mk_buf_delete( buf );

	if( strncmp( szRefLine, szRefPrefix, (int)(unsigned)cRefPrefix ) != 0 ) {
		mk_dbg_outf( "%s: first line does not begin with '%s'\n", szHeadFile, szRefPrefix );
		return (char *)0;
	}

	return mk_com_dup( (char *)0, mk_com_va( "%s/%s", pszGitDir, &szRefLine[cRefPrefix] ) );
}
/* get the name of the latest commit from the given branch file */
char *mk_git_getCommit( const char *pszBranchFile ) {
	MkBuffer buf;
	char szCommitLine[512];

	MK_ASSERT( pszBranchFile != (const char *)0 && *pszBranchFile != '\0' );

	if( !( buf = mk_buf_loadFile( pszBranchFile ) ) ) {
		return (char *)0;
	}

	mk_buf_readLine( buf, szCommitLine, sizeof( szCommitLine ) );
	mk_dbg_outf( "mk_git_getCommit(\"%s\"): <%s>\n", pszBranchFile, szCommitLine );
	mk_buf_delete( buf );

	return mk_com_dup( (char *)0, szCommitLine );
}
/* get the timestamp of the latest commit from the given branch file */
char *mk_git_getCommitTimestamp( const char *pszBranchFile, const MkStat_t *pStat ) {
	MkStat_t s;
	struct tm *t;
	char szBuf[128];

	MK_ASSERT( pszBranchFile != (const char *)0 && *pszBranchFile != '\0' );

	if( pStat != (const MkStat_t *)0 ) {
		s = *pStat;
	} else if( stat( pszBranchFile, &s ) != 0 ) {
		mk_log_error( pszBranchFile, 0, (const char *)0, "Stat failed" );
		return (char *)0;
	}

	if( !( t = localtime( &s.st_mtime ) ) ) {
		mk_log_error( pszBranchFile, 0, (const char *)0, "Could not determine time from stat" );
		return (char *)0;
	}

	strftime( szBuf, sizeof( szBuf ), "%Y-%m-%d %H:%M:%S", t );
	mk_dbg_outf( "mk_git_getCommitTimestamp(\"%s\"): %s\n", pszBranchFile, szBuf );
	return mk_com_dup( (char *)0, szBuf );
}

/* load gitinfo from the cache or generate it (NOTE: expects to be in project's source directory) */
MkGitInfo mk_git_loadInfo( void ) {
	static const char *const pszHeader            = "mk-gitinfo-cache::v1";
	static const char *const pszHeaderUnavailable = "mk-gitinfo-cache::unavailable";
	static const char szPrefixGitBranchFile[]     = "git-branch-file:";
	static const size_t cPrefixGitBranchFile      = sizeof( szPrefixGitBranchFile ) - 1;
	static const char *const pszTailer            = "mk-gitinfo-cache::end";

	MkGitInfo pGitInfo;
	MkBuffer buf;
	FILE *pCacheFile;
	char *pszGitPath;
	char szGitCacheFile[PATH_MAX];
	char szLine[512];

	if( !( pGitInfo = (MkGitInfo)mk_mem_alloc( sizeof( *pGitInfo ) ) ) ) {
		return (MkGitInfo)0;
	}

	/* construct the name of the cache file */
	mk_com_strcpy( szGitCacheFile, sizeof( szGitCacheFile ), mk_com_va( "%s/cache.gitinfo", mk_opt_getBuildGenPath() ) );
	mk_dbg_outf( "mk_git_loadInfo: cache-file: %s\n", szGitCacheFile );

	/* load the cache file */
	if( ( buf = mk_buf_loadFile( szGitCacheFile ) ) != (MkBuffer)0 ) {
		/* using `do/while(0)` for ability to `break` on error */
		do {
			char *pszGitBranchFile;

			/* check the header for the proper version (if not then we need to regenerate) */
			mk_buf_readLine( buf, szLine, sizeof( szLine ) );
			mk_dbg_outf( "\t%s\n", szLine );
			if( strcmp( szLine, pszHeader ) != 0 ) {
				if( strcmp( szLine, pszHeaderUnavailable ) == 0 ) {
					mk_mem_dealloc( (void *)pGitInfo );
					return (MkGitInfo)0;
				}
				mk_dbg_outf( "%s: expected \"%s\" on line 1; regenerating\n", szGitCacheFile, pszHeader );
				break;
			}

			/* grab the branch file name */
			mk_buf_readLine( buf, szLine, sizeof( szLine ) );
			mk_dbg_outf( "\t%s\n", szLine );
			if( strncmp( szLine, szPrefixGitBranchFile, cPrefixGitBranchFile ) != 0 ) {
				mk_dbg_outf( "%s: expected \"%s\" on line 2; regenerating\n", szGitCacheFile, szPrefixGitBranchFile );
				break;
			}

			/* pszGitBranchFile guaranteed to be not null from `mk_com_dup` */
			pszGitBranchFile = mk_com_dup( (char *)0, &szLine[cPrefixGitBranchFile] );
			mk_dbg_outf( "\t\t<%s>\n", pszGitBranchFile );

			/* find the commit name */
			if( !( pGitInfo->pszCommit = mk_git_getCommit( pszGitBranchFile ) ) ) {
				pszGitBranchFile = (char *)mk_mem_dealloc( (void *)pszGitBranchFile );
				break;
			}

			/* grab the timestamp for the commit */
			if( !( pGitInfo->pszTimestamp = mk_git_getCommitTimestamp( pszGitBranchFile, (const MkStat_t *)0 ) ) ) {
				pGitInfo->pszCommit = (char *)mk_mem_dealloc( (void *)pGitInfo->pszCommit );
				pszGitBranchFile    = (char *)mk_mem_dealloc( (void *)pszGitBranchFile );
				break;
			}

			/* store the branch file to indicate success */
			pGitInfo->pszBranchFile = pszGitBranchFile;
		} while( 0 );
		mk_buf_delete( buf );

		/* return if we successfully filled the structure (if pszBranchFile is set, assume filled) */
		if( pGitInfo->pszBranchFile != (char *)0 ) {
			return pGitInfo;
		}
	} else {
		mk_dbg_outf( "\tCould not load cache file\n" );
	}

	if( !( pCacheFile = fopen( szGitCacheFile, "wb" ) ) ) {
		mk_log_error( szGitCacheFile, 0, (const char *)0, "Failed to open gitinfo cache file for writing" );
	}

	/* using `do/while(0)` for easy `break` (to avoid code duplication) */
	do {
		/* find the git path */
		if( !( pszGitPath = mk_git_findRoot() ) ) {
			break;
		}

		/* find the git branch file */
		if( !( pGitInfo->pszBranchFile = mk_git_findBranchPath( pszGitPath ) ) ) {
			pszGitPath = (char *)mk_mem_dealloc( (void *)pszGitPath );
			break;
		}

		/* get the commit for the branch */
		if( !( pGitInfo->pszCommit = mk_git_getCommit( pGitInfo->pszBranchFile ) ) ) {
			pGitInfo->pszBranchFile = (char *)mk_mem_dealloc( (void *)pGitInfo->pszBranchFile );
			pszGitPath              = (char *)mk_mem_dealloc( (void *)pszGitPath );
			break;
		}

		/* get the timestamp for the branch */
		if( !( pGitInfo->pszTimestamp = mk_git_getCommitTimestamp( pGitInfo->pszBranchFile, (const MkStat_t *)0 ) ) ) {
			pGitInfo->pszCommit     = (char *)mk_mem_dealloc( (void *)pGitInfo->pszCommit );
			pGitInfo->pszBranchFile = (char *)mk_mem_dealloc( (void *)pGitInfo->pszBranchFile );
			pszGitPath              = (char *)mk_mem_dealloc( (void *)pszGitPath );
			break;
		}

		/* write the results to the debug log */
		mk_dbg_outf( "Caching new gitinfo ***\n" );
		mk_dbg_outf( "\tbranch: %s\n", pGitInfo->pszBranchFile );
		mk_dbg_outf( "\tcommit: %s\n", pGitInfo->pszCommit );
		mk_dbg_outf( "\ttstamp: %s\n", pGitInfo->pszTimestamp );
		mk_dbg_outf( "\n" );

		/* cache the branch file */
		if( pCacheFile != (FILE *)0 ) {
			fprintf( pCacheFile, "%s\r\n", pszHeader );
			fprintf( pCacheFile, "%s%s\r\n", szPrefixGitBranchFile, pGitInfo->pszBranchFile );
			fprintf( pCacheFile, "%s\r\n", pszTailer );

			pCacheFile = ( fclose( pCacheFile ), (FILE *)0 );
		}

		/* done */
		return pGitInfo;
	} while( 0 );

	/* if this point is reached, we failed; free the gitinfo */
	mk_mem_dealloc( (void *)pGitInfo );

	/* write the "unavailable" cache file */
	if( pCacheFile != (FILE *)0 ) {
		fprintf( pCacheFile, "%s\r\n", pszHeaderUnavailable );
		pCacheFile = ( fclose( pCacheFile ), (FILE *)0 );
	}

	/* done */
	return (MkGitInfo)0;
}
/* write out the header containing git info of the given commit and timestamp */
int mk_git_writeHeader( const char *pszHFilename, MkGitInfo pGitInfo ) {
	MkStat_t branchStat, headerStat;
	const char *pszBranchFile;
	const char *pszCommit;
	const char *pszTimestamp;

	const char *pszBranchName;
	FILE *fp;

	MK_ASSERT( pszHFilename != (const char *)0 && *pszHFilename != '\0' );

	if( pGitInfo != (MkGitInfo)0 ) {
		pszBranchFile = pGitInfo->pszBranchFile;
		MK_ASSERT( pszBranchFile != (const char *)0 && *pszBranchFile != '\0' );

		pszCommit = pGitInfo->pszCommit;
		MK_ASSERT( pszCommit != (const char *)0 && *pszCommit != '\0' );

		pszTimestamp = pGitInfo->pszTimestamp;
		MK_ASSERT( pszTimestamp != (const char *)0 && *pszTimestamp != '\0' );

		if( !( pszBranchName = strrchr( pszBranchFile, '/' ) ) ) {
			mk_dbg_outf( "mk_git_writeHeader: Invalid branch filename \"%s\"\n", pszBranchFile );
			return 0;
		}

		/* eat the leading '/' (instead of "/master" we want "master") */
		++pszBranchName;

		/* early exit if we don't need a new file */
		if( stat( pszBranchFile, &branchStat ) == 0 && stat( pszHFilename, &headerStat ) == 0 && branchStat.st_mtime < headerStat.st_mtime ) {
			/* file already made; don't modify */
			return 1;
		}

		if( !( fp = fopen( pszHFilename, "wb" ) ) ) {
			mk_log_error( pszHFilename, 0, (const char *)0, "Failed to open file for writing" );
			return 0;
		}

		fprintf( fp, "/* This file is generated automatically; edit at your own risk! */\r\n" );
		fprintf( fp, "#ifndef BUILDGEN_GITINFO_H__%s\r\n", pszCommit );
		fprintf( fp, "#define BUILDGEN_GITINFO_H__%s\r\n", pszCommit );
		fprintf( fp, "\r\n" );
		fprintf( fp, "#if defined( _MSC_VER ) || defined( __GNUC__ ) || defined( __clang__ )\r\n" );
		fprintf( fp, "# pragma once\r\n" );
		fprintf( fp, "#endif\r\n" );
		fprintf( fp, "\r\n" );
		fprintf( fp, "#define BUILDGEN_GITINFO_AVAILABLE 1\r\n" );
		fprintf( fp, "\r\n" );
		fprintf( fp, "#define BUILDGEN_GITINFO_BRANCH \"%s\"\r\n", pszBranchName );
		fprintf( fp, "#define BUILDGEN_GITINFO_COMMIT \"%s\"\r\n", pszCommit );
		fprintf( fp, "#define BUILDGEN_GITINFO_TSTAMP \"%s\"\r\n", pszTimestamp );
		fprintf( fp, "\r\n" );
		fprintf( fp, "#endif /* BUILDGEN_GITINFO_H */\r\n" );

		fclose( fp );
		return 1;
	}

	/* if the file exists then just return now, assuming it's already set to "unavailable" */
	if( mk_fs_isFile( pszHFilename ) ) {
		/* FIXME: Would be better to compare against the gitinfo cache file */
		return 1;
	}

	if( !( fp = fopen( pszHFilename, "wb" ) ) ) {
		mk_log_error( pszHFilename, 0, (const char *)0, "Failed to open file for writing" );
		return 0;
	}

	fprintf( fp, "/* This file is generated automatically; edit at your own risk! */\r\n" );
	fprintf( fp, "#ifndef BUILDGEN_GITINFO_H__\r\n" );
	fprintf( fp, "#define BUILDGEN_GITINFO_H__\r\n" );
	fprintf( fp, "\r\n" );
	fprintf( fp, "#if defined( _MSC_VER ) || defined( __GNUC__ ) || defined( __clang__ )\r\n" );
	fprintf( fp, "# pragma once\r\n" );
	fprintf( fp, "#endif\r\n" );
	fprintf( fp, "\r\n" );
	fprintf( fp, "#define BUILDGEN_GITINFO_AVAILABLE 0\r\n" );
	fprintf( fp, "\r\n" );
	fprintf( fp, "#endif\r\n" );

	fclose( fp );
	return 1;
}

/* generate the gitinfo header if necessary */
int mk_git_generateInfo( void ) {
	MkGitInfo pGitInfo;
	char szHeaderDir[PATH_MAX];
	char szHeaderFile[PATH_MAX];
	int r;

	mk_com_strcpy( szHeaderDir, sizeof( szHeaderDir ), mk_opt_getBuildGenIncDir() );
	mk_com_strcpy( szHeaderFile, sizeof( szHeaderFile ), mk_com_va( "%s/gitinfo.h", szHeaderDir ) );

	mk_fs_makeDirs( szHeaderDir );

	pGitInfo = mk_git_loadInfo();
	r        = mk_git_writeHeader( szHeaderFile, pGitInfo );
	pGitInfo = (MkGitInfo)mk_mem_dealloc( (void *)pGitInfo );

	return r;
}
