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
 *	OPTIONS SYSTEM
 *	========================================================================
 *	Retrieve several options. In the future these are to be made more
 *	configurable.
 */

#include "mk-defs-config.h"

typedef enum {
	kMkColorMode_None,
	kMkColorMode_ANSI,
#if MK_WINDOWS_COLORS_ENABLED
	kMkColorMode_Windows,
#endif

	kMkNumColorModes
} MkColorMode_t;

const char *  mk_opt_getObjdirBase( void );
const char *  mk_opt_getDebugLogPath( void );
const char *  mk_opt_getDebugSuffix( void );
MkColorMode_t mk_opt_getColorMode( void );
const char *  mk_opt_getConfigName( void );

const char *mk_opt_getBuildGenPath( void );
const char *mk_opt_getBuildGenIncDir( void );

const char *mk_opt_getGlobalDir( void );
const char *mk_opt_getGlobalSharedDir( void );
const char *mk_opt_getGlobalVersionDir( void );
