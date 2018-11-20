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
#include "mk-basic-sourceBuffer.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-logging.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int mk_buf__copyRange( char **dst, const char *src, size_t len ) {
	size_t n;
	char *p;

	if( !src ) {
		if( *dst ) {
			mk_com_memory( (void *)*dst, 0 );
			*dst = NULL;
		}

		return 1;
	}

	len = len ? len : strlen( src );
	n   = len + 1;

	p = (char *)mk_com_memory( (void *)*dst, n );
	if( !p ) {
		return 0;
	}

	*dst = p;

	memcpy( p, src, len );
	p[len] = '\0';

	return 1;
}
static int mk_buf__copy( char **dst, const char *src ) {
	return mk_buf__copyRange( dst, src, 0 );
}
#if 0
static int mk_buf__append(char **dst, const char *src, size_t *offset) {
	char *p;
	size_t len;

	if( (!src) || (!*dst && *offset>0) ) {
		return 0;
	}

	if( !*dst ) {
		return mk_buf__copy(dst, src);
	}

	len = strlen(src) + 1;

	p = (char *)(mk_com_memory((void *)*dst, *offset + len));
	if( !p ) {
		return 0;
	}

	*dst = p;

	memcpy(&((*dst)[*offset]), src, len);
	*offset += len - 1;

	return 1;
}
#endif

int mk_buf_setFilename( MkBuffer text, const char *filename ) {
	return mk_buf__copy( &text->file, filename );
}
int mk_buf_setFunction( MkBuffer text, const char *func ) {
	return mk_buf__copy( &text->func, func );
}

static int mk_buf__addLinePtr( MkBuffer text, char *ptr ) {
	char **arr;
	size_t amt;

	if( !ptr ) {
		if( text->linePtrs ) {
			mk_com_memory( (void *)( text->linePtrs ), 0 );
			text->linePtrs = NULL;
			text->numLines = 0;
		}

		return 1;
	}

	amt = sizeof( char * ) * ( text->numLines + 1 );
	arr = (char **)mk_com_memory( (void *)text->linePtrs, amt );
	if( !arr ) {
		return 0;
	}

	text->linePtrs                   = arr;
	text->linePtrs[text->numLines++] = ptr;

	return 1;
}

static MkBuffer mk_buf__alloc() {
	MkBuffer text;

	text = (MkBuffer)mk_com_memory( 0, sizeof( *text ) );
	if( !text ) {
		return NULL;
	}

	text->file = NULL;
	text->func = NULL;

	text->text   = NULL;
	text->endPtr = NULL;
	text->ptr    = NULL;

	text->numLines = 0;
	text->linePtrs = NULL;

	return text;
}
static void mk_buf__fini( MkBuffer text ) {
	mk_buf__addLinePtr( text, NULL );

	mk_buf__copy( &text->file, NULL );
	mk_buf__copy( &text->func, NULL );
	text->ptr = NULL;
}

#if MK_PROCESS_NEWLINE_CONCAT_ENABLED
static const char *mk_buf__skipNewline( const char *p ) {
	const char *e;

	e = NULL;
	if( *p == '\r' ) {
		e = ++p;
	}

	if( *p == '\n' ) {
		e = ++p;
	}

	if( *p == '\0' ) {
		e = p;
	}

	return e;
}
#endif
static int mk_buf__initFromMemory( MkBuffer text, const char *filename, const char *source, size_t len ) {
#if MK_PROCESS_NEWLINE_CONCAT_ENABLED
	const char *s, *e, *next;
	const char *add;
#else
	int add;
#endif

	MK_ASSERT( !!text );
	MK_ASSERT( !!filename );
	MK_ASSERT( !!source );

	if( !mk_buf_setFilename( text, filename ) ) {
		return 0;
	}

#if MK_PROCESS_NEWLINE_CONCAT_ENABLED
	if( !len ) {
		len = strlen( source );
	}

	text->text = mk_com_memory( 0, len + 1 );
	if( !text->text ) {
		return 0;
	}

	*text->text = '\0';
#else
	if( !mk_buf__copyRange( &text->text, source, len ) ) {
		return 0;
	}
#endif

	mk_buf__addLinePtr( text, NULL );
	mk_buf__addLinePtr( text, text->text );

#if MK_PROCESS_NEWLINE_CONCAT_ENABLED
	text->ptr = text->text;
	s         = source;
	for( e = s; *e; e = next ) {
		next = e + 1;
		add  = NULL; /*this will point to AFTER the newline or AT the EOF*/

		if( *e == '\\' ) {
			add = mk_buf__skipNewline( e + 1 );
			if( !add ) {
				continue;
			}
		} else {
			add = mk_buf__skipNewline( e );
			if( !add ) {
				continue;
			}

			e = add; /*want newlines in source when not escaped!*/
		}

		memcpy( text->ptr, s, e - s );
		text->ptr += e - s;

		s    = add;
		next = add;

		if( !mk_buf__addLinePtr( text, text->ptr ) ) {
			mk_buf__fini( text );
			return 0;
		}
	}

	*text->ptr = '\0';
#else
	for( text->ptr = text->text; *text->ptr != '\0'; text->ptr++ ) {
		add = 0;

		if( *text->ptr == '\r' ) {
			if( text->ptr[1] == '\n' ) {
				text->ptr++;
			}
			add = 1;
		}
		if( *text->ptr == '\n' ) {
			add = 1;
		}

		if( add && !mk_buf__addLinePtr( text, &text->ptr[1] ) ) {
			mk_buf__fini( text );
			return 0;
		}
	}
#endif

	text->endPtr = text->ptr;
	text->ptr    = text->text;

#if 0
	printf("******************************************************\n");
	printf("#### ORIGINAL ####\n");
	printf("%s\n", source);
	printf("******************************************************\n");
	printf("#### READ-IN ####\n");
	printf("%s\n", text->text);
	printf("******************************************************\n");
#endif

	return 1;
}
static int mk_buf__initFromFile( MkBuffer text, const char *filename ) {
#if 0
		FILE *f;
		char *p, buf[8192];
		size_t offset;
		int r;

#	if MK_SECLIB
		if( fopen_s(&f, filename, "rb")!=0 ) {
			f = NULL;
		}
#	else
		f = fopen(filename, "rb");
#	endif
		if( !f ) {
			return 0;
		}

		p = NULL;
		offset = 0;
		r = 1;

		while( !feof(f) ) {
			int n;

			n = fread(buf, 1, sizeof(buf) - 1, f);
			if( n <= 0 ) {
				r = !ferror(f);
				break;
			}

			buf[ n ] = '\0';

			if( !mk_buf__append(&p, buf, &offset) ) {
				r = 0;
				break;
			}
		}

		fclose(f);

		if( !r || !mk_buf__initFromMemory(text, filename, p, 0) ) {
			mk_buf__copy(&p, NULL);
			return 0;
		}

		return 1;
#else
	FILE *f;
	char *p;
	size_t n;

#	if MK_SECLIB
	if( fopen_s( &f, filename, "rb" ) != 0 ) {
		f = NULL;
	}
#	else
	f = fopen( filename, "rb" );
#	endif
	if( !f ) {
		return 0;
	}

	fseek( f, 0, SEEK_END );
	n = (size_t)ftell( f );

	fseek( f, 0, SEEK_SET );

	p = (char *)mk_com_memory( NULL, n + 1 );
	if( !fread( (void *)p, n, 1, f ) ) {
		mk_com_memory( (void *)p, 0 );
		fclose( f );
		return 0;
	}

	fclose( f );
	f = NULL;

	p[n] = '\0';
	if( !mk_buf__initFromMemory( text, filename, p, 0 ) ) {
		mk_com_memory( (void *)p, 0 );
		return 0;
	}

	mk_com_memory( (void *)p, 0 );
	return 1;
#endif
}

MkBuffer mk_buf_loadMemoryRange( const char *filename, const char *source, size_t len ) {
	MkBuffer text;

	text = mk_buf__alloc();
	if( !text ) {
		return NULL;
	}

	if( !mk_buf__initFromMemory( text, filename, source, len ) ) {
		mk_com_memory( (void *)text, 0 );
		return NULL;
	}

	return text;
}
MkBuffer mk_buf_loadMemory( const char *filename, const char *source ) {
	return mk_buf_loadMemoryRange( filename, source, 0 );
}
MkBuffer mk_buf_loadFile( const char *filename ) {
	MkBuffer text;

	text = mk_buf__alloc();
	if( !text ) {
		return NULL;
	}

	if( !mk_buf__initFromFile( text, filename ) ) {
		mk_com_memory( (void *)text, 0 );
		return NULL;
	}

	return text;
}
MkBuffer mk_buf_delete( MkBuffer text ) {
	if( !text ) {
		return NULL;
	}

	mk_buf__fini( text );
	mk_com_memory( (void *)text, 0 );

	return NULL;
}

const char *mk_buf_getFilename( const MkBuffer text ) {
	return text->file;
}
const char *mk_buf_getFunction( const MkBuffer text ) {
	return text->func;
}
size_t mk_buf_calculateLine( const MkBuffer text ) {
	size_t i;

	for( i = 0; i < text->numLines; i++ ) {
		if( text->ptr < text->linePtrs[i] ) {
			break;
		}
	}

	return i + 1;
}

size_t mk_buf_getLength( const MkBuffer text ) {
	return text->endPtr - text->text;
}

void mk_buf_seek( MkBuffer text, size_t pos ) {
	text->ptr = &text->text[pos];

	if( text->ptr > text->endPtr ) {
		text->ptr = text->endPtr;
	}
}
size_t mk_buf_tell( const MkBuffer text ) {
	return text->ptr - text->text;
}
char *mk_buf_getPtr( MkBuffer text ) {
	return text->ptr;
}

char mk_buf_read( MkBuffer text ) {
	char c;

	c = *text->ptr++;
	if( text->ptr > text->endPtr ) {
		text->ptr = text->endPtr;
	}

	return c;
}
char mk_buf_peek( MkBuffer text ) {
	return *text->ptr;
}
char mk_buf_lookAhead( MkBuffer text, size_t offset ) {
	if( text->ptr + offset >= text->endPtr ) {
		return '\0';
	}

	return text->ptr[offset];
}
int mk_buf_advanceIfCharEq( MkBuffer text, char ch ) {
	if( *text->ptr != ch ) {
		return 0;
	}

	if( ++text->ptr > text->endPtr ) {
		text->ptr = text->endPtr;
	}

	return 1;
}

void mk_buf_skip( MkBuffer text, size_t offset ) {
	mk_buf_seek( text, text->ptr - text->text + offset );
}
void mk_buf_skipWhite( MkBuffer text ) {
	while( (unsigned char)( *text->ptr ) <= ' ' && text->ptr < text->endPtr ) {
		text->ptr++;
	}
}
void mk_buf_skipLine( MkBuffer text ) {
	int r;

	r = 0;
	while( text->ptr < text->endPtr ) {
		if( *text->ptr == '\r' ) {
			r = 1;
			text->ptr++;
		}
		if( *text->ptr == '\n' ) {
			r = 1;
			text->ptr++;
		}

		if( r ) {
			return;
		}

		text->ptr++;
	}
}

int mk_buf_skipLineIfStartsWith( MkBuffer text, const char *pszCommentDelim ) {
	size_t n;

	MK_ASSERT( pszCommentDelim != (const char *)0 );

	n = strlen( pszCommentDelim );
	if( text->ptr + n > text->endPtr ) {
		return 0;
	}

	if( strncmp( text->ptr, pszCommentDelim, (int)n ) != 0 ) {
		return 0;
	}

	text->ptr += n;
	mk_buf_skipLine( text );

	return 1;
}

int mk_buf_readLine( MkBuffer text, char *dst, size_t dstn ) {
	const char *s, *e;

	s = text->ptr;
	if( s == text->endPtr ) {
		return -1;
	}

	mk_buf_skipLine( text );
	e = text->ptr;

	if( e > s && *( e - 1 ) == '\n' ) {
		--e;
	}

	if( e > s && *( e - 1 ) == '\r' ) {
		--e;
	}

	mk_com_strncpy( dst, dstn, s, e - s );

	return (int)( e - s );
}

void mk_buf_errorfv( MkBuffer text, const char *format, va_list args ) {
	char buf[4096];

#if __STDC_WANT_SECURE_LIB__
	vsprintf_s( buf, sizeof( buf ), format, args );
#else
	vsnprintf( buf, sizeof( buf ), format, args );
	buf[sizeof( buf ) - 1] = '\0';
#endif

	mk_log_error( text->file, mk_buf_calculateLine( text ), text->func, buf );
}
void mk_buf_errorf( MkBuffer text, const char *format, ... ) {
	va_list args;

	va_start( args, format );
	mk_buf_errorfv( text, format, args );
	va_end( args );
}
