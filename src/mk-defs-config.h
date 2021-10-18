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

#include "mk-defs-platform.h"

/*
================
MK_DEBUG_ENABLED

Whether internal debugging of Mk is enabled. If enabled, various debug layers
will be enabled (unless configured separately) and the debug log will be turned
on. (See mk-basic-debug.c, mk-basic-debug.h.)
================
*/
#ifndef MK_DEBUG_ENABLED
#	if defined( _DEBUG ) || defined( DEBUG ) || defined( __debug__ ) || defined( __DEBUG__ )
#		define MK_DEBUG_ENABLED 1
#	else
#		define MK_DEBUG_ENABLED 0
#	endif
#endif

/*
================
MK_WINDOWS_ENABLED

Controls whether we'll use Windows-specific features, or fall back to *nix
methods. In general, this shouldn't be changed from the default.
================
*/
#ifndef MK_WINDOWS_ENABLED
#	if MK_HOST_OS_MSWIN
#		define MK_WINDOWS_ENABLED 1
#	else
#		define MK_WINDOWS_ENABLED 0
#	endif
#endif

/*
================
MK_WINDOWS_COLORS_ENABLED

If enabled (default on Windows) it will include support for coloring the Windows
console. Otherwise, only ANSI colors will be supported.
================
*/
#ifndef MK_WINDOWS_COLORS_ENABLED
#	if MK_WINDOWS_ENABLED
#		define MK_WINDOWS_COLORS_ENABLED 1
#	else
#		define MK_WINDOWS_COLORS_ENABLED 0
#	endif
#endif
#if !MK_WINDOWS_ENABLED
#	undef MK_WINDOWS_COLORS_ENABLED
#	define MK_WINDOWS_COLORS_ENABLED 0
#endif

/*
================
MK_PWD_HOMEPATH_DETECTION_ENABLED

If enabled (default) then systems with pwd.h will have their homepath detected
with pwd(). See `mk_opt__getGlobalContainerDir()` in mk-basic-options.c for
implementation details.
================
*/
#ifndef MK_PWD_HOMEPATH_DETECTION_ENABLED
#	define MK_PWD_HOMEPATH_DETECTION_ENABLED 1
#endif
#if !MK_HAS_PWD
#	undef MK_PWD_HOMEPATH_DETECTION_ENABLED
#	define MK_PWD_HOMEPATH_DETECTION_ENABLED 0
#endif

/*
================
MK_PROCESS_NEWLINE_CONCAT_ENABLED

Define to 1 to enable newline concatenation through the backslash character.
Example:

	INPUT
	-----
	Test\
	Text

	OUTPUT
	------
	TestText

See mk-io-sourceBuffer.c, mk-io-sourceBuffer.h.
================
*/
#ifndef MK_PROCESS_NEWLINE_CONCAT_ENABLED
#	define MK_PROCESS_NEWLINE_CONCAT_ENABLED 1
#endif

/*
===============================================================================

	NAMING

	These define the default names for various things, such as directories
	and files.

===============================================================================
*/

/*
================
MK_DEFAULT_OBJDIR_BASE

Name of the local directory that Mk will put cache files and intermediate build
files into.

Default: ".mk-obj"
================
*/
#ifndef MK_DEFAULT_OBJDIR_BASE
#	define MK_DEFAULT_OBJDIR_BASE ".mk-obj"
#endif

/*
================
MK_DEFAULT_DEBUG_SUFFIX

The suffix placed on binaries built in debug mode, to distinguish them from
release mode binaries. e.g., libmy-project.a vs libmy-project-dbg.a.

Default: "-dbg"
================
*/
#ifndef MK_DEFAULT_DEBUG_SUFFIX
#	define MK_DEFAULT_DEBUG_SUFFIX "-dbg"
#endif

/*
================
MK_DEFAULT_DEBUGLOG_FILENAME

Name of the file that Mk will log debug output to.

NOTE: This file will be placed within the intermediate build directory.

Default: "mk-debug.log"
(Which is really: ".mk-obj/mk-debug.log")
================
*/
#ifndef MK_DEFAULT_DEBUGLOG_FILENAME
#	define MK_DEFAULT_DEBUGLOG_FILENAME "mk-debug.log"
#endif

/*
================
MK_DEFAULT_COLOR_MODE

The default coloring mode to use, preferably.

In the future, we would like to add an "Auto" mode, which would detect whether
it's targeting a Windows console, a terminal, or a file.

Default, on Windows: Windows
Default, otherwise: ANSI
================
*/
#ifndef MK_DEFAULT_COLOR_MODE
#	if MK_WINDOWS_COLORS_ENABLED
#		define MK_DEFAULT_COLOR_MODE Windows
#	else
#		define MK_DEFAULT_COLOR_MODE ANSI
#	endif
#endif
#define MK__DEFAULT_COLOR_MODE_IMPL MK_PPTOKCAT( kMkColorMode_, MK_DEFAULT_COLOR_MODE )

/*
================
MK_DEFAULT_RELEASE_CONFIG_NAME

The name of the release configuration, which affects internal directory names.

Default: "release"
================
*/
#ifndef MK_DEFAULT_RELEASE_CONFIG_NAME
#	define MK_DEFAULT_RELEASE_CONFIG_NAME "release"
#endif

/*
================
MK_DEFAULT_DEBUG_CONFIG_NAME

The name of the debug configuration, which affects internal directory names.

Default: "debug"
================
*/
#ifndef MK_DEFAULT_DEBUG_CONFIG_NAME
#	define MK_DEFAULT_DEBUG_CONFIG_NAME "debug"
#endif

/*
================
MK_PLATFORM_OS_NAME_*

The names of the various operating system platforms. This is how the platforms
will be referred to across all of Mk's systems. This affects build directories,
autolink configuration files, etc.

Modification is generally not encouraged.
================
*/

/* Windows */
#ifndef MK_PLATFORM_OS_NAME_MSWIN
#	define MK_PLATFORM_OS_NAME_MSWIN "mswin"
#endif
/* Universal Windows Program */
#ifndef MK_PLATFORM_OS_NAME_UWP
#	define MK_PLATFORM_OS_NAME_UWP "uwp"
#endif
/* Windows (via Cygwin) */
#ifndef MK_PLATFORM_OS_NAME_CYGWIN
#	define MK_PLATFORM_OS_NAME_CYGWIN "cygwin"
#endif
/* Mac OS X / macOS */
#ifndef MK_PLATFORM_OS_NAME_MACOSX
#	define MK_PLATFORM_OS_NAME_MACOSX "macos"
#endif
/* (GNU/)Linux */
#ifndef MK_PLATFORM_OS_NAME_LINUX
#	define MK_PLATFORM_OS_NAME_LINUX "linux"
#endif
/* iOS */
#ifndef MK_PLATFORM_OS_NAME_IOS
#	define MK_PLATFORM_OS_NAME_IOS "ios"
#endif
/* iPadOS */
#ifndef MK_PLATFORM_OS_NAME_IPADOS
#	define MK_PLATFORM_OS_NAME_IPADOS "ipados"
#endif
/* tvOS */
#ifndef MK_PLATFORM_OS_NAME_TVOS
#	define MK_PLATFORM_OS_NAME_TVOS "tvos"
#endif
/* watchOS */
#ifndef MK_PLATFORM_OS_NAME_WATCHOS
#	define MK_PLATFORM_OS_NAME_WATCHOS "watchos"
#endif
/* Android */
#ifndef MK_PLATFORM_OS_NAME_ANDROID
#	define MK_PLATFORM_OS_NAME_ANDROID "android"
#endif
/* Fuschia */
#ifndef MK_PLATFORM_OS_NAME_FUSCHIA
#	define MK_PLATFORM_OS_NAME_FUSCHIA "fuschia"
#endif
/* Some UNIX variant (e.g., FreeBSD, Solaris) */
#ifndef MK_PLATFORM_OS_NAME_UNIX
#	define MK_PLATFORM_OS_NAME_UNIX "unix"
#endif

/*
================
MK_PLATFORM_CPU_NAME_*

The names of the various architectures / CPUs. This is how the platforms will be
referred to across all of Mk's systems. This primarily affects build directory
naming and "special directories." (Special directories are directories which are
only included in the build if the target platform matches, or some other special
condition is met.)

Modification is generally not encouraged.
================
*/

/* x86 (32-bit) */
#ifndef MK_PLATFORM_CPU_NAME_X86
#	define MK_PLATFORM_CPU_NAME_X86 "x86"
#endif
/* x86-64 (64-bit) */
#ifndef MK_PLATFORM_CPU_NAME_X64
#	define MK_PLATFORM_CPU_NAME_X64 "x64"
#endif
/* ARM (32-bit) */
#ifndef MK_PLATFORM_CPU_NAME_ARM
#	define MK_PLATFORM_CPU_NAME_ARM "arm"
#endif
/* ARM / AArch64 (64-bit) */
#ifndef MK_PLATFORM_CPU_NAME_AARCH64
#	define MK_PLATFORM_CPU_NAME_AARCH64 "aarch64"
#endif
/* PowerPC (generic) */
#ifndef MK_PLATFORM_CPU_NAME_PPC
#	define MK_PLATFORM_CPU_NAME_PPC "ppc"
#endif
/* MIPS (generic) */
#ifndef MK_PLATFORM_CPU_NAME_MIPS
#	define MK_PLATFORM_CPU_NAME_MIPS "mips"
#endif
/* WebAssembly */
#ifndef MK_PLATFORM_CPU_NAME_WASM
#	define MK_PLATFORM_CPU_NAME_WASM "wasm"
#endif
/* Risc-V */
#ifndef MK_PLATFORM_CPU_NAME_RISCV
#	define MK_PLATFORM_CPU_NAME_RISCV "riscv"
#endif

/*
================
MK_PLATFORM_OS_NAME

Name of the OS Mk was built for.

NOTE: This should not be relied upon, as its existence is a flaw.
================
*/
#ifndef MK_PLATFORM_OS_NAME
#	if MK_HOST_OS_MSWIN
#		define MK_PLATFORM_OS_NAME MK_PLATFORM_OS_NAME_MSWIN
#	elif MK_HOST_OS_CYGWIN
#		define MK_PLATFORM_OS_NAME MK_PLATFORM_OS_NAME_CYGWIN
#	elif MK_HOST_OS_LINUX
#		define MK_PLATFORM_OS_NAME MK_PLATFORM_OS_NAME_LINUX
#	elif MK_HOST_OS_MACOSX
#		define MK_PLATFORM_OS_NAME MK_PLATFORM_OS_NAME_MACOSX
#	elif MK_HOST_OS_UNIX
#		define MK_PLATFORM_OS_NAME MK_PLATFORM_OS_NAME_UNIX
#	else
#		error "MK_PLATFORM_OS_NAME: Unhandled OS."
#	endif
#endif

/*
================
MK_PLATFORM_CPU_NAME

Name of the architecture Mk was built for.

NOTE: This should not be relied upon, as its existence is a flaw.
================
*/
#ifndef MK_PLATFORM_CPU_NAME
#	if MK_HOST_CPU_X86
#		define MK_PLATFORM_CPU_NAME MK_PLATFORM_CPU_NAME_X86
#	elif MK_HOST_CPU_X86_64
#		define MK_PLATFORM_CPU_NAME MK_PLATFORM_CPU_NAME_X64
#	elif MK_HOST_CPU_ARM
#		define MK_PLATFORM_CPU_NAME MK_PLATFORM_CPU_NAME_ARM
#	elif MK_HOST_CPU_AARCH64
#		define MK_PLATFORM_CPU_NAME MK_PLATFORM_CPU_NAME_AARCH64
#	elif MK_HOST_CPU_PPC
#		define MK_PLATFORM_CPU_NAME MK_PLATFORM_CPU_NAME_PPC
#	elif MK_HOST_CPU_MIPS
#		define MK_PLATFORM_CPU_NAME MK_PLATFORM_CPU_NAME_MIPS
#	elif MK_HOST_CPU_WASM
#		define MK_PLATFORM_CPU_NAME MK_PLATFORM_CPU_NAME_WASM
#	elif MK_HOST_CPU_RISCV
#		define MK_PLATFORM_CPU_NAME MK_PLATFORM_CPU_NAME_RISCV
#	else
#		error "MK_PLATFORM_CPU_NAME: Unhandled CPU."
#	endif
#endif

/*
================
MK_PLATFORM_DIR

Name of the default directory for platform-specific files.

NOTE: This should not be relied upon, as its existence is a flaw.
================
*/
#ifndef MK_PLATFORM_DIR
#	define MK_PLATFORM_DIR MK_PLATFORM_OS_NAME "-" MK_PLATFORM_CPU_NAME
#endif
