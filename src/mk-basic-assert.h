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

#include "mk-basic-logging.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"

/* macros for MK_ASSERT errors */
#if MK_DEBUG_ENABLED
#	undef MK_ASSERT
#	undef MK_ASSERT_MSG
#	define MK_ASSERT_MSG( x, m )                                        \
		do {                                                             \
			if( !( x ) ) {                                               \
				mk_log_errorAssert( __FILE__, __LINE__, MK_CURFUNC, m ); \
			}                                                            \
		} while( 0 )
#	define MK_ASSERT( x ) \
		MK_ASSERT_MSG( ( x ), "MK_ASSERT(" #x ") failed!" )
#else
#	undef MK_ASSERT
#	undef MK_ASSERT_MSG
#	define MK_ASSERT( x ) /*nothing*/
#	define MK_ASSERT_MSG( x, m ) /*nothing*/
#endif
