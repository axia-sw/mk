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
#include "mk-build-makefileDependency.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-sourceBuffer.h"
#include "mk-build-dependency.h"

#include <stddef.h>
#include <stdlib.h>

enum {
	kMkMfDepTok_EOF   = 0,
	kMkMfDepTok_Colon = ':',

	kMkMfDepTok__EnumBase = 256,
	kMkMfDepTok_Ident
};

/* internal function ported from previous version of 'mk_mfdep_load' */
static char mk_mfdep__read( MkBuffer buf, char *cur, char *look ) {
	/*
	 *	! This is based on old code.
	 */

	MK_ASSERT( buf != (MkBuffer)0 );
	MK_ASSERT( cur != (char *)0 );
	MK_ASSERT( look != (char *)0 );

	*cur  = mk_buf_read( buf );
	*look = mk_buf_peek( buf );

	return *cur;
}

/* read a token from the buffer */
static int mk_mfdep__lex( MkBuffer buf, char *dst, size_t dstn, char *cur, char *look ) {
	size_t i;

	MK_ASSERT( buf != (MkBuffer)0 );
	MK_ASSERT( dst != (char *)0 );
	MK_ASSERT( dstn > 1 );

	i      = 0;
	dst[0] = 0;

	if( !*cur ) {
		do {
			if( !mk_mfdep__read( buf, cur, look ) ) {
				return kMkMfDepTok_EOF;
			}
		} while( *cur <= ' ' );
	}

	if( *cur == ':' ) {
		dst[0] = ':';
		dst[1] = 0;

		*cur = 0; /* force 'cur' to be retrieved again, else infinite loop */

		return kMkMfDepTok_Colon;
	}

	while( 1 ) {
		if( *cur == '\\' ) {
			/*
			 *	! Original code expected that '\r' was possible, hence this
			 *	  setup was needed to make this portion easier. Needs to be
			 *	  fixed later.
			 */
			if( *look == '\n' ) {
				do {
					if( !mk_mfdep__read( buf, cur, look ) ) {
						break;
					}
				} while( *cur <= ' ' );

				continue;
			}

			if( *look == ' ' ) {
				dst[i++] = mk_mfdep__read( buf, cur, look );
				if( i == dstn ) {
					mk_buf_errorf( buf, "overflow detected" );
					exit( EXIT_FAILURE );
				}
				continue;
			}

			*cur = '/'; /* correct path reads */
		} else if( *cur == ':' ) {
			if( *look <= ' ' ) {
				dst[i] = 0;
				return kMkMfDepTok_Ident; /* leave 'cur' for the next call */
			}
		} else if( *cur <= ' ' ) {
			dst[i] = 0;
			*cur   = 0; /* force read on next call */

			return kMkMfDepTok_Ident;
		}

		dst[i++] = *cur;
		if( i == dstn ) {
			mk_buf_errorf( buf, "overflow detected" );
			exit( EXIT_FAILURE );
		}

		mk_mfdep__read( buf, cur, look );
	}
}

/* read dependencies from a file */
int mk_mfdep_load( const char *filename ) {
	MkBuffer buf;
	MkDep dep;
	char lexan[PATH_MAX], ident[PATH_MAX], cur, look;
	int tok;

	ident[0] = '\0';
	lexan[0] = '\0';
	cur      = 0;
	look     = 0;

	buf = mk_buf_loadFile( filename );
	if( !buf ) {
		return 0;
	}

	mk_mfdep__read( buf, &cur, &look );

	dep = (MkDep)0;

	do {
		tok = mk_mfdep__lex( buf, lexan, sizeof( lexan ), &cur, &look );
#if 0
		mk_dbg_outf( "%s(%i): %i: \"%s\"\n", mk_buf_getFilename(buf),
			(int)mk_buf_calculateLine(buf), tok, lexan );
#endif

		if( tok == kMkMfDepTok_Colon ) {
			if( ident[0] ) {
				dep      = mk_dep_new( ident );
				ident[0] = 0;
			}

			continue;
		}

		if( dep && ident[0] ) {
			mk_dep_push( dep, ident );
			ident[0] = 0;
		}

		if( tok == kMkMfDepTok_Ident ) {
			mk_com_strcpy( ident, sizeof( ident ), lexan );
		}
	} while( tok != kMkMfDepTok_EOF );

	mk_buf_delete( buf );
	return 1;
}
