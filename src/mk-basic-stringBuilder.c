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
#include "mk-basic-stringBuilder.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-memory.h"
#include "mk-basic-stringList.h"
#include "mk-basic-variable.h"

#include <string.h>

MkStringBuilder *mk_sb_init( MkStringBuilder *sb, size_t initCapacity ) {
	MK_ASSERT( sb != (MkStringBuilder *)0 );

	if( !initCapacity ) {
		initCapacity = 128;
	}

	sb->buffer = (char *)mk_com_memory( (char*)0, initCapacity );

	sb->len      = 0;
	sb->capacity = initCapacity;

	return sb;
}
char *mk_sb_done( MkStringBuilder *sb ) {
	MK_ASSERT( sb != (MkStringBuilder *)0 );
	MK_ASSERT( sb->buffer != (char *)0 );
	MK_ASSERT( sb->len < sb->capacity );

	sb->buffer[ sb->len ] = '\0';
	return sb->buffer;
}

MkStringBuilder *mk_sb_clear( MkStringBuilder *sb ) {
	MK_ASSERT( sb != (MkStringBuilder *)0 );

	sb->len = 0;
	return sb;
}

MkStringBuilder *mk_sb_pushSubstr( MkStringBuilder *sb, const char *s, const char *e ) {
	size_t len;

	MK_ASSERT( sb != (MkStringBuilder *)0 );
	MK_ASSERT( s != (const char *)0 );

	if( e == (const char *)0 ) {
		e = strchr( s, '\0' );
	}

	len = (size_t)(ptrdiff_t)( e - s );

	if( sb->len + len + 1 > sb->capacity ) {
		static const size_t alignment = 1024;
		size_t capacity;

		capacity  = sb->len + len + 1;
		capacity += ( alignment - ( capacity % alignment ) )%alignment;

		sb->buffer   = (char *)mk_com_memory( (void*)sb->buffer, capacity );
		sb->capacity = capacity;
	}

	memcpy( (void *)&sb->buffer[sb->len], (const void *)s, len );
	sb->len += len;

	return sb;
}
MkStringBuilder *mk_sb_pushStr( MkStringBuilder *sb, const char *s ) {
	return mk_sb_pushSubstr( sb, s, (const char *)0 );
}
MkStringBuilder *mk_sb_pushChar( MkStringBuilder *sb, char c ) {
	const char *s, *e;

	if( c == '\0' ) {
		return sb;
	}

	s = &c;
	e = s + 1;

	return mk_sb_pushSubstr( sb, s, e );
}
MkStringBuilder *mk_sb_pushVar( MkStringBuilder *sb, MkVariable v ) {
	MkStrList values;
	size_t i, n;
	char prefix;

	if( !v ) {
		return sb;
	}

	if( !( values = mk_v_values( v ) ) ) {
		return sb;
	}

	n      = mk_sl_getSize( values );
	prefix = '\0';
	for( i = 0; i < n; ++i ) {
		const char *str;

		str = mk_sl_at( values, i );
		if( !str || *str == '\0' ) {
			continue;
		}

		mk_sb_pushChar( sb, prefix );
		prefix = ' ';

		mk_sb_quoteAndPushStr( sb, str );
	}

	return sb;
}

MkStringBuilder *mk_sb_quoteAndPushSubstr( MkStringBuilder *sb, const char *s, const char *e ) {
	size_t n;
	char quote;
	int hasInternalQuotes;

	MK_ASSERT( sb != (MkStringBuilder *)0 );
	MK_ASSERT( s != (const char *)0 );

	if( !e ) {
		n = strlen( s );
		e = s + n;
	} else {
		n = (size_t)(ptrdiff_t)( e - s );
	}

	quote = '\0';
	if( memchr( s, ' ', n ) != NULL ) {
		quote = '\"';
	}

	mk_sb_pushChar( sb, quote );

	hasInternalQuotes = +( memchr( s, '\"', n ) != NULL );
	if( hasInternalQuotes ) {
		const char *p;

		for( p = s; p != e; ++p ) {
			if( *p == '\\' || *p == '\"' ) {
				mk_sb_pushChar( sb, '\\' );
			}

			mk_sb_pushChar( sb, *p );
		}
	} else {
		mk_sb_pushSubstr( sb, s, e );
	}

	mk_sb_pushChar( sb, quote );
	return sb;
}
MkStringBuilder *mk_sb_quoteAndPushStr( MkStringBuilder *sb, const char *s ) {
	return mk_sb_quoteAndPushSubstr( sb, s, (const char *)0 );
}
MkStringBuilder *mk_sb_unquoteAndPushSubstr( MkStringBuilder *sb, const char *s, const char *e ) {
	const char *p, *q;
	size_t n;

	MK_ASSERT( sb != (MkStringBuilder *)0 );
	MK_ASSERT( s != (const char *)0 );

	if( !e ) {
		n = strlen( s );
		e = s + n;
	} else {
		n = (size_t)(ptrdiff_t)( e - s );
	}

	if( *s != '\"' || n < 2 ) {
		return mk_sb_pushSubstr( sb, s, e );
	}

	++s;
	--e;

	/* check for malformed input and adjust trim if caught */
	if( *e != '\"' ) {
		++e;
	}

	for( p = s; p != e; p = q ) {
		q = mk_com_memchrz( p, (size_t)(ptrdiff_t)( e - p ), '\\' );
		mk_sb_pushSubstr( sb, p, q );

		if( q != e ) {
			++q;
			if( q != e ) {
				mk_sb_pushChar( sb, *q );
				++q;
			}
		}
	}

	return sb;
}
MkStringBuilder *mk_sb_unquoteAndPushStr( MkStringBuilder *sb, const char *s ) {
	return mk_sb_unquoteAndPushSubstr( sb, s, (const char *)0 );
}
