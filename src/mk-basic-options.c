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
#include "mk-basic-options.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-defs-config.h"
#include "mk-frontend.h"
#include "mk-version.h"

#if MK_HAS_PWD
#	include <pwd.h>
#endif
#include <unistd.h>

/* retrieve the object directory base (where intermediate files go) */
const char *mk_opt_getObjdirBase( void ) {
	return MK_DEFAULT_OBJDIR_BASE "/" MK_PLATFORM_DIR;
}
/* retrieve the path to the debug log */
const char *mk_opt_getDebugLogPath( void ) {
	return MK_DEFAULT_OBJDIR_BASE "/" MK_DEFAULT_DEBUGLOG_FILENAME;
}
/* retrieve the debug suffix used when naming debug target output */
const char *mk_opt_getDebugSuffix( void ) {
	return MK_DEFAULT_DEBUG_SUFFIX;
}
/* retrieve the color mode used when writing colored output */
MkColorMode_t mk_opt_getColorMode( void ) {
	return mk__g_flags_color;
}
/* retrieve the name of this configuration (debug, release) */
const char *mk_opt_getConfigName( void ) {
	return ( mk__g_flags & kMkFlag_Release_Bit ) != 0
	    ? MK_DEFAULT_RELEASE_CONFIG_NAME
	    : MK_DEFAULT_DEBUG_CONFIG_NAME;
}

/* retrieve the path to the "gen" directory */
const char *mk_opt_getBuildGenPath( void ) {
	return MK_DEFAULT_OBJDIR_BASE "/gen";
}
/* retrieve the path to the "build-generated" include directory */
const char *mk_opt_getBuildGenIncDir( void ) {
	return MK_DEFAULT_OBJDIR_BASE "/gen/include/build-generated";
}

/* find the path (on this platform) that the global directory (.mk/) should be stored within */
static const char *mk_opt__getGlobalContainerDir( void ) {
	static char szDir[PATH_MAX] = { '\0' };

	if( szDir[0] == '\0' ) {
#if MK_HAS_PWD
		const struct passwd *pwd;
#endif

#define MK__TRYENV( X_ )                                 \
	do {                                                 \
		if( !szDir[0] ) {                                \
			mk_com_getenv( szDir, sizeof( szDir ), X_ ); \
		}                                                \
	} while( 0 )

		MK__TRYENV( "MKPATH" );

#if MK_PWD_HOMEPATH_DETECTION_ENABLED
		if( !szDir[0] && ( pwd = getpwuid( getuid() ) ) != (const struct passwd *)0 ) {
			mk_com_strcpy( szDir, sizeof( szDir ), pwd->pw_dir );
			mk_com_strcat( szDir, sizeof( szDir ), "/.local/share/" );
		}
#endif
#if MK_WINDOWS_ENABLED
		MK__TRYENV( "LOCALAPPDATA" );
		MK__TRYENV( "USERPROFILE" );
		MK__TRYENV( "APPDATA" );
#endif
#ifdef __APPLE__
		/*
			FIXME: Should this really be "~/.local/share/", or should it be
			`      somewhere like "~/Library/Application Support/"?
		*/
		if( !szDir[0] ) {
			MK__TRYENV( "HOME" );
			if( szDir[0] != '\0' ) {
				mk_com_strcat( szDir, sizeof( szDir ), "/.local/share/" );
			}
		}
#else
		MK__TRYENV( "HOME" );
#endif

#undef MK__TRYENV

		mk_com_fixpath( szDir );

		if( !szDir[0] ) {
			mk_com_strcpy( szDir, sizeof( szDir ), "./" );
		} else if( !mk_com_strends( szDir, "/" ) ) {
			mk_com_strcat( szDir, sizeof( szDir ), "/" );
		}
	}

	return szDir;
}

/* determine the path that the global directory (.mk/) should exist */
const char *mk_opt_getGlobalDir( void ) {
	static char szDir[PATH_MAX] = { '\0' };

	if( szDir[0] == '\0' ) {
		mk_com_strcpy( szDir, sizeof( szDir ), mk_opt__getGlobalContainerDir() );
		MK_ASSERT( mk_com_strends( szDir, "/" ) && "mk_opt__getGlobalContainerDir() didn't end path with '/'" );

		if( mk_com_strends( szDir, ".local/share/" ) ) {
			mk_com_strcat( szDir, sizeof( szDir ), "mk/" );
		} else {
			mk_com_strcat( szDir, sizeof( szDir ), ".mk/" );
		}
	}

	return szDir;
}
/* retrieve the .mk/anyver/ directory, where version-agnostic mk data can be stored */
const char *mk_opt_getGlobalSharedDir( void ) {
	static char szDir[PATH_MAX] = { '\0' };

	if( szDir[0] == '\0' ) {
		mk_com_strcpy( szDir, sizeof( szDir ), mk_opt_getGlobalDir() );
		MK_ASSERT( mk_com_strends( szDir, "/" ) && "mk_opt_getGlobalDir() didn't end path with '/'" );
		mk_com_strcat( szDir, sizeof( szDir ), "anyver/" );
	}

	return szDir;
}
/* retrieve the .mk/<version>/ directory, where version-specific mk data can be stored */
const char *mk_opt_getGlobalVersionDir( void ) {
	static char szDir[PATH_MAX] = { '\0' };

	if( szDir[0] == '\0' ) {
		mk_com_strcpy( szDir, sizeof( szDir ), mk_opt_getGlobalDir() );
		MK_ASSERT( mk_com_strends( szDir, "/" ) && "mk_opt_getGlobalDir() didn't end path with '/'" );
		mk_com_strcat( szDir, sizeof( szDir ), MK_VERSION_STR "/" );
	}

	return szDir;
}
