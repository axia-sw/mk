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
 *	=======================================================================
 *	GIT VERSION CONTROL INFO
 *	=======================================================================
 *	Retrieve information about the git repo from the perspective of the
 *	current directory. Header information can then be written to a
 *	gitinfo.h file that the project can include.
 *
 *	e.g., a project might include the gitinfo.h in one of their version
 *	`     source files (such as for an "about box" or debug logging) as
 *	`     follows:
 *
 *		#include <build-generated/gitinfo.h>
 *
 *	The file will define the following macros, if git is used:
 *
 *		BUILDGEN_GITINFO_BRANCH (e.g., "master")
 *		BUILDGEN_GITINFO_COMMIT (e.g., "9d4bf863ed1ea878fde62756eea2c6e85fc7a6de")
 *		BUILDGEN_GITINFO_TSTAMP (e.g., "2016-01-10 16:47:11")
 *
 *	If git is used, BUILDGEN_GITINFO_AVAILABLE will be defined to 1,
 *	otherwise it will be defined to 0.
 *
 *	It is planned to expand this in the future to also grab the tag of the
 *	branch, if any, which may be used for versioning. (As Swift's packages
 *	use.) Then, should zip/tar file generation be supported, said files can
 *	also use the version they're defined against.
 */

#include "mk-basic-types.h"

typedef struct MkGitInfo_s *MkGitInfo;

char *    mk_git_findRoot( void );
char *    mk_git_findBranchPath( const char *pszGitDir );
char *    mk_git_getCommit( const char *pszBranchFile );
char *    mk_git_getCommitTimestamp( const char *pszBranchFile, const MkStat_t *pStat );
MkGitInfo mk_git_loadInfo( void );
int       mk_git_writeHeader( const char *pszHFilename, MkGitInfo pGitInfo );

int mk_git_generateInfo( void );
