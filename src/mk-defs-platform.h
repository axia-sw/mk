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

#define MK_HOST_OS_MSWIN 0
#define MK_HOST_OS_CYGWIN 0
#define MK_HOST_OS_MACOSX 0
#define MK_HOST_OS_LINUX 0
#define MK_HOST_OS_UNIX 0

#if defined( _WIN32 )
#	if defined( __CYGWIN__ )
#		undef MK_HOST_OS_CYGWIN
#		define MK_HOST_OS_CYGWIN 1
#	else
#		undef MK_HOST_OS_MSWIN
#		define MK_HOST_OS_MSWIN 1
#	endif
#elif defined( __linux__ ) || defined( __linux )
#	undef MK_HOST_OS_LINUX
#	define MK_HOST_OS_LINUX 1
#elif defined( __APPLE__ ) && defined( __MACH__ )
#	undef MK_HOST_OS_MACOSX
#	define MK_HOST_OS_MACOSX 1
#elif defined( __unix__ ) || defined( __unix )
#	undef MK_HOST_OS_UNIX
#	define MK_HOST_OS_UNIX 1
#else
#	error "Unrecognized host OS."
#endif

#define MK_HOST_CPU_X86 0
#define MK_HOST_CPU_X86_64 0
#define MK_HOST_CPU_ARM 0
#define MK_HOST_CPU_AARCH64 0
#define MK_HOST_CPU_PPC 0
#define MK_HOST_CPU_MIPS 0
#define MK_HOST_CPU_WASM 0
#define MK_HOST_CPU_RISCV 0

#if defined( __amd64__ ) || defined( __x86_64__ ) || defined( _M_X64 )
#	undef MK_HOST_CPU_X86_64
#	define MK_HOST_CPU_X86_64 1
#elif defined( __i386__ ) || defined( _M_IX86 )
#	undef MK_HOST_CPU_X86
#	define MK_HOST_CPU_X86 1
#elif defined( __aarch64__ )
#	undef MK_HOST_CPU_AARCH64
#	define MK_HOST_CPU_AARCH64 1
#elif defined( __arm__ ) || defined( _ARM )
#	undef MK_HOST_CPU_ARM
#	define MK_HOST_CPU_ARM 1
#elif defined( __powerpc__ ) || defined( __POWERPC__ ) || defined( __ppc__ )
#	undef MK_HOST_CPU_PPC
#	define MK_HOST_CPU_PPC 1
#elif defined( _M_PPC ) || defined( _ARCH_PPC )
#	undef MK_HOST_CPU_PPC
#	define MK_HOST_CPU_PPC 1
#elif defined( __mips__ ) || defined( __MIPS__ )
#	undef MK_HOST_CPU_MIPS
#	define MK_HOST_CPU_MIPS 1
#elif defined( __webassembly__ )
#	undef MK_HOST_CPU_WASM
#	define MK_HOST_CPU_WASM 1
#elif defined( __riscv__ )
#	undef MK_HOST_CPU_RISCV
#	define MK_HOST_CPU_RISCV 1
#else
#	error "Unrecognized host CPU."
#endif

#ifdef _MSC_VER
#	define MK_VC_VER _MSC_VER
#else
#	define MK_VC_VER 0
#endif

#ifndef MK_HAS_DIRENT
#	if !MK_VC_VER
#		define MK_HAS_DIRENT 1
#	else
#		define MK_HAS_DIRENT 0
#	endif
#endif
#ifndef MK_HAS_UNISTD
#	if !MK_VC_VER
#		define MK_HAS_UNISTD 1
#	else
#		define MK_HAS_UNISTD 0
#	endif
#endif

#ifndef MK_HAS_EXECINFO
#	if !MK_HOST_OS_MSWIN
#		define MK_HAS_EXECINFO 1
#	else
#		define MK_HAS_EXECINFO 0
#	endif
#endif

#ifndef MK_HAS_PWD
#	if defined( __APPLE__ ) || defined( __linux__ ) || defined( __unix__ )
#		define MK_HAS_PWD 1
#	else
#		define MK_HAS_PWD 0
#	endif
#endif

#ifndef MK_SECLIB
#	if defined( __STDC_WANT_SECURE_LIB__ ) && defined( _MSC_VER )
#		define MK_SECLIB 1
#	else
#		define MK_SECLIB 0
#	endif
#endif

#ifndef MK_NORETURN
#	if MK_VC_VER
#		define MK_NORETURN _declspec( noreturn )
#	else
#		define MK_NORETURN __attribute__( ( noreturn ) )
#	endif
#endif

#ifndef MK_UNUSED
#	if MK_VC_VER
#		define MK_UNUSED /* doesn't exist for MSVC? */
#	else
#		define MK_UNUSED __attribute__( ( unused ) )
#	endif
#endif

#ifndef MK_CURFUNC
#	if MK_VC_VER
#		define MK_CURFUNC __FUNCTION__
#	else
#		define MK_CURFUNC __func__
#	endif
#endif
