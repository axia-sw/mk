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
 *	LIBRARIAN
 *	========================================================================
 *	Manages individual libraries. Each library is abstracted with a platform
 *	independent name (e.g., "opengl") which is then mapped to platform
 *	dependent flags. (e.g., on Windows the above would be "-lopengl32," but on
 *	GNU/Linux it would be "-lGL.")
 */

#include "mk-basic-types.h"
#include "mk-build-platform.h"

struct MkProject_s;
typedef struct MkLib_s *MkLib;

enum {
	kMkLib_Processed_Bit = 0x01 /* indicates a library has been "processed" */
};

struct MkLib_s {
	size_t name_n;
	char *name;
	char *flags[kMkNumOS];

	struct MkProject_s *proj;

	bitfield_t config;

	struct MkLib_s *prev, *next;
};

MkLib mk_lib_new( void );
void  mk_lib_delete( MkLib lib );
void  mk_lib_deleteAll( void );

void        mk_lib_setName( MkLib lib, const char *name );
void        mk_lib_setFlags( MkLib lib, int sys, const char *flags );
const char *mk_lib_getName( MkLib lib );
const char *mk_lib_getFlags( MkLib lib, int sys );

MkLib mk_lib_prev( MkLib lib );
MkLib mk_lib_next( MkLib lib );
MkLib mk_lib_head( void );
MkLib mk_lib_tail( void );
MkLib mk_lib_find( const char *name );
MkLib mk_lib_lookup( const char *name );

void mk_lib_clearAllProcessed( void );
void mk_lib_clearProcessed( MkLib lib );
void mk_lib_setProcessed( MkLib lib );
int  mk_lib_isProcessed( MkLib lib );
