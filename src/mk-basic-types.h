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

#include "mk-defs-config.h"
#include "mk-defs-platform.h"

#if MK_WINDOWS_ENABLED
#	if MK_VC_VER
#		define _CRT_SECURE_NO_WARNINGS 1
#	endif
#	define WIN32_LEAN_AND_MEAN 1
#	include <windows.h>
#endif
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PATH_MAX
#	if defined( _MAX_PATH )
#		define PATH_MAX _MAX_PATH
#	elif defined( _PATH_MAX )
#		define PATH_MAX _PATH_MAX
#	elif defined( MAX_PATH )
#		define PATH_MAX MAX_PATH
#	elif defined( MAXPATHLEN )
#		define PATH_MAX MAXPATHLEN
#	else
#		define PATH_MAX 512
#	endif
#endif

#ifndef S_IFREG
#	define S_IFREG 0100000
#endif
#ifndef S_IFDIR
#	define S_IFDIR 0040000
#endif

typedef struct stat MkStat_t;

#ifndef bitfield_t_defined
#	define bitfield_t_defined 1
typedef size_t bitfield_t;
#endif
