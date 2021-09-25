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

typedef enum {
	kMkOS_MSWin,
	kMkOS_UWP,
	kMkOS_Cygwin,
	kMkOS_Linux,
	kMkOS_MacOSX,
	kMkOS_Unix,

	kMkNumOS
} MkOS_t;

typedef enum {
	kMkCPU_X86,
	kMkCPU_X86_64,
	kMkCPU_ARM,
	kMkCPU_AArch64,
	kMkCPU_PowerPC,
	kMkCPU_MIPS,
	kMkCPU_RiscV,
	kMkCPU_WebAssembly,

	kMkNumCPU
} MkCPU_t;

static const int mk__g_hostOS =
#if MK_HOST_OS_MSWIN
    kMkOS_MSWin;
#elif MK_HOST_OS_CYGWIN
    kMkOS_Cygwin;
#elif MK_HOST_OS_LINUX
    kMkOS_Linux;
#elif MK_HOST_OS_MACOSX
    kMkOS_MacOSX;
#elif MK_HOST_OS_UNIX
    kMkOS_Unix;
#else
#	error "OS not recognized."
#endif

static const int mk__g_hostCPU =
#if MK_HOST_CPU_X86
    kMkCPU_X86;
#elif MK_HOST_CPU_X86_64
    kMkCPU_X86_64;
#elif MK_HOST_CPU_ARM
    kMkCPU_ARM;
#elif MK_HOST_CPU_AARCH64
    kMkCPU_AArch64;
#elif MK_HOST_CPU_PPC
    kMkCPU_PowerPC;
#elif MK_HOST_CPU_MIPS
        kMkCPU_MIPS;
#else
/* TODO: kMkCPU_AArch64 */
#	error "Architecture not recognized."
#endif
