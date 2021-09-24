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
 *	BUFFER MANAGEMENT CODE
 *	========================================================================
 *	Manage input source file buffers (similar in nature to flex's buffer).
 *	Keeps track of the file's name and the current line.
 */

#include <stdarg.h>
#include <stddef.h>

typedef struct MkBuffer_s *MkBuffer;

struct MkBuffer_s {
	char *file;
	char *func;

	char *text;
	char *endPtr;
	char *ptr;

	size_t numLines;
	char **linePtrs;
};

MkBuffer mk_buf_loadMemoryRange( const char *filename, const char *source, size_t len );
MkBuffer mk_buf_loadMemory( const char *filename, const char *source );
MkBuffer mk_buf_loadFile( const char *filename );
MkBuffer mk_buf_delete( MkBuffer text );

int mk_buf_setFilename( MkBuffer text, const char *filename );
int mk_buf_setFunction( MkBuffer text, const char *func );

const char *mk_buf_getFilename( const MkBuffer text );
const char *mk_buf_getFunction( const MkBuffer text );
size_t      mk_buf_calculateLine( const MkBuffer text );

size_t mk_buf_getLength( const MkBuffer text );

void   mk_buf_seek( MkBuffer text, size_t pos );
size_t mk_buf_tell( const MkBuffer text );
char * mk_buf_getPtr( MkBuffer text );

char mk_buf_read( MkBuffer text );
char mk_buf_peek( MkBuffer text );
char mk_buf_lookAhead( MkBuffer text, size_t offset );
int  mk_buf_advanceIfCharEq( MkBuffer text, char ch );

void mk_buf_skip( MkBuffer text, size_t offset );
void mk_buf_skipWhite( MkBuffer text );
void mk_buf_skipLine( MkBuffer text );
int  mk_buf_skipLineIfStartsWith( MkBuffer text, const char *pszCommentDelim );

int mk_buf_readLine( MkBuffer text, char *dst, size_t dstn );

void mk_buf_errorfv( MkBuffer text, const char *format, va_list args );
void mk_buf_errorf( MkBuffer text, const char *format, ... );
