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
#include "mk-basic-variable.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-memory.h"
#include "mk-basic-stringList.h"

/*

	COMMAND FORMAT
	==============

	Command strings are written as normal shell strings, but can have special
	variables within them. The variables are written similar to Makefiles.

	For example, "Hello $(Sekai)" would expand to "Hello World" if the variable
	"Sekai" had the value "World" stored within it.

	Variables can be strings or arrays.

*/

struct MkVariable_s {
	struct MkVariableSet_s *vs_prnt;

	struct MkVariable_s *v_prev, *v_next;

	char *name;
	unsigned int nameHash;

	MkStrList values;
};
struct MkVariableSet_s {
	struct MkVariableSet_s *vs_prnt;
	struct MkVariableSet_s *vs_head, *vs_tail;
	struct MkVariableSet_s *vs_prev, *vs_next;

	struct MkVariable_s *v_head, *v_tail;
};

static struct MkVariableSet_s *g_vs_head = NULL, *g_vs_tail = NULL;

typedef struct {
	char *buffer;
	size_t len, capacity;
} stringBuilder_t;

static stringBuilder_t *sb_init( stringBuilder_t *sb, size_t initCapacity ) {
	MK_ASSERT( sb != (stringBuilder_t *)0 );

	if( !initCapacity ) {
		initCapacity = 128;
	}

	sb->buffer = (char *)mk_com_memory( (char*)0, initCapacity );

	sb->len      = 0;
	sb->capacity = initCapacity;

	return sb;
}
static char *sb_done( stringBuilder_t *sb ) {
	MK_ASSERT( sb != (stringBuilder_t *)0 );
	MK_ASSERT( sb->buffer != (char *)0 );
	MK_ASSERT( sb->len < sb->capacity );

	sb->buffer[ sb->len ] = '\0';
	return sb->buffer;
}
static stringBuilder_t *sb_pushSubstr( stringBuilder_t *sb, const char *s, const char *e ) {
	size_t len;

	MK_ASSERT( sb != (stringBuilder_t *)0 );
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
static stringBuilder_t *sb_pushStr( stringBuilder_t *sb, const char *s ) {
	return sb_pushSubstr( sb, s, (const char *)0 );
}
static stringBuilder_t *sb_pushChar( stringBuilder_t *sb, char c ) {
	const char *s, *e;

	if( c == '\0' ) {
		return sb;
	}

	s = &c;
	e = s + 1;

	return sb_pushSubstr( sb, s, e );
}
static stringBuilder_t *sb_pushVar( stringBuilder_t *sb, MkVariable v ) {
	size_t i, n;
	char prefix, quote;

	if( !v ) {
		return sb;
	}

	if( !v->values ) {
		return sb;
	}

	n = mk_sl_getSize( v->values );
	prefix = '\0';
	for( i = 0; i < n; ++i ) {
		const char *str;

		str = mk_sl_at( v->values, i );
		if( !str || *str == '\0' ) {
			continue;
		}

		sb_pushChar( sb, prefix );
		prefix = ' ';

		// FIXME: Also implement quote escaping
		quote = '\0';
		if( strchr( str, ' ' ) != (const char *)0 ) {
			quote = '\"';
		}

		sb_pushChar( sb, quote );
		sb_pushStr( sb, str );
		sb_pushChar( sb, quote );
	}

	return sb;
}

static unsigned int murmur3_scramble( unsigned int k ) {
	k *= 0xCC9E2D51;
	k  = (k << 15) | (k >> 17);
	k *= 0x1b873593;

	return k;
}

static unsigned int murmur3( const char *s, const char *e ) {
	static const unsigned int seed = 0x734C825D;

	unsigned int h, k;
	size_t i, len;

	if( e == (const char *)0 ) {
		e = strchr( s, '\0' );
	}

	len = (size_t)(ptrdiff_t)( e - s );
	h = seed;

	for( i = len/4; i != 0; --i ) {
		k =
			( ( (unsigned int)(unsigned char)s[0] ) << 24 ) |
			( ( (unsigned int)(unsigned char)s[1] ) << 16 ) |
			( ( (unsigned int)(unsigned char)s[2] ) <<  8 ) |
			( ( (unsigned int)(unsigned char)s[3] ) <<  0 );

		s += 4;

		h ^= murmur3_scramble( k );
		h  = ( h << 13 ) | ( h >> 19 );
		h  = h*5 + 0xE6546B64;
	}

	k = 0;
	while( s != e ) {
		k <<= 8;
		k  |= *s;

		s += 1;
	}

	h ^= murmur3_scramble( k );

	h ^= len;
	h ^= h >> 16;
	h *= 0x85EBCA6B;
	h ^= h >> 13;
	h *= 0xC2B2AE35;
	h ^= h >> 16;

	return h;
}

MkVariableSet mk_vs_new() {
	MkVariableSet vs;
	
	vs = (MkVariableSet)mk_com_memory( (void*)0, sizeof(*vs) );

	if( ( vs->vs_prev = g_vs_tail ) != NULL ) {
		g_vs_tail->vs_next = vs;
	} else {
		g_vs_head = vs;
	}
	g_vs_tail = vs;

	return vs;
}
static void mk_vs_unlink( MkVariableSet vs ) {
	MkVariableSet *p_vs_head, *p_vs_tail;

	if( vs->vs_prnt != (MkVariableSet)0 ) {
		p_vs_head = &vs->vs_prnt->vs_head;
		p_vs_tail = &vs->vs_prnt->vs_tail;
	} else {
		p_vs_head = &g_vs_head;
		p_vs_tail = &g_vs_tail;
	}

	if( vs->vs_prev != (MkVariableSet)0 ) {
		vs->vs_prev->vs_next = vs->vs_next;
	} else {
		*p_vs_head = vs->vs_next;
	}
	if( vs->vs_next != (MkVariableSet)0 ) {
		vs->vs_next->vs_prev = vs->vs_prev;
	} else {
		*p_vs_tail = vs->vs_prev;
	}

	vs->vs_prev = (MkVariableSet)0;
	vs->vs_next = (MkVariableSet)0;
	vs->vs_prnt = (MkVariableSet)0;
}
void mk_vs_delete( MkVariableSet vs ) {
	if( !vs ) {
		return;
	}

	mk_vs_clear( vs );
	mk_vs_unlink( vs );

	mk_com_memory( (void*)vs, 0 );
}
void mk_vs_clear( MkVariableSet vs ) {
	MK_ASSERT( vs != (MkVariableSet)0 );

	while( vs->v_head ) {
		mk_v_delete( vs->v_head );
	}
}
int mk_vs_isDescendant( MkVariableSet vs, MkVariableSet ancestor ) {
	MK_ASSERT( vs != (MkVariableSet)0 );
	MK_ASSERT( ancestor != (MkVariableSet)0 );

	while( vs != (MkVariableSet)0 ) {
		if( vs == ancestor ) {
			return 1;
		}

		vs = vs->vs_prnt;
	}

	return 0;
}
void mk_vs_setParent( MkVariableSet vs, MkVariableSet prnt ) {
	MkVariableSet *p_vs_head, *p_vs_tail;

	MK_ASSERT( mk_vs_isDescendant( prnt, vs ) == 0 );

	mk_vs_unlink( vs );

	if( prnt != (MkVariableSet)0 ) {
		p_vs_head = &prnt->vs_head;
		p_vs_tail = &prnt->vs_tail;
	} else {
		p_vs_head = &g_vs_head;
		p_vs_tail = &g_vs_tail;
	}

	vs->vs_prnt = prnt;
	if( ( vs->vs_prev = *p_vs_tail ) != (MkVariableSet)0 ) {
		vs->vs_prev->vs_next = vs;
	} else {
		*p_vs_head = vs;
	}
	*p_vs_tail = vs;
}

MkVariable mk_v_new( MkVariableSet vs, const char *name ) {
	MkVariable v;
	unsigned int hash;

	MK_ASSERT( vs != (MkVariableSet)0 );
	MK_ASSERT( name != (const char *)0 );

	hash = murmur3( name, (const char *)0 );

	for( v = vs->v_head; v != (MkVariable)0; v = v->v_next ) {
		MK_ASSERT_MSG( v->nameHash != hash, "Variable with this name already exists in current set" );
	}

	v = (MkVariable)mk_com_memory( (void*)0, sizeof(*v) );

	v->vs_prnt = vs;
	if( ( v->v_prev = vs->v_tail ) != (MkVariable)0 ) {
		v->v_prev->v_next = v;
	} else {
		vs->v_head = v;
	}
	vs->v_tail = v;

	v->name     = mk_com_strdup( name );
	v->nameHash = hash;

	v->values = mk_sl_new();

	return v;
}
void mk_v_delete( MkVariable v ) {
	if( !v ) {
		return;
	}

	MK_ASSERT( v->vs_prnt != (MkVariableSet)0 );

	if( v->v_prev != (MkVariable)0 ) {
		v->v_prev->v_next = v->v_next;
	} else {
		v->vs_prnt->v_head = v->v_next;
	}

	if( v->v_next != (MkVariable)0 ) {
		v->v_next->v_prev = v->v_prev;
	} else {
		v->vs_prnt->v_tail = v->v_prev;
	}

	v->vs_prnt = (MkVariableSet)0;
	v->v_prev  = (MkVariable)0;
	v->v_next  = (MkVariable)0;

	v->name     = (char *)mk_com_memory( (void*)v->name, 0 );
	v->nameHash = 0;

	mk_sl_delete( v->values );
	v->values = (MkStrList)0;

	mk_com_memory( (void *)v, 0 );
}

MkVariable mk_vs_findVarBySubstr( MkVariableSet vs, const char *s, const char *e ) {
	MkVariable v;
	unsigned int hash;

	MK_ASSERT( vs != (MkVariableSet)0 );
	MK_ASSERT( s != (const char *)0 );

	hash = murmur3( s, e );

	do {
		for( v = vs->v_head; v != (MkVariable)0; v = v->v_next ) {
			if( v->nameHash == hash ) {
				return v;
			}
		}

		vs = vs->vs_prnt;
	} while( vs != (MkVariableSet)0 );

	return (MkVariable)0;
}
MkVariable mk_vs_findVar( MkVariableSet vs, const char *name ) {
	return mk_vs_findVarBySubstr( vs, name, (const char *)0 );
}
MkStrList mk_v_values( MkVariable v ) {
	if( !v ) {
		return (MkStrList)0;
	}

	return v->values;
}

void mk_v_setValueBySubstr( MkVariable v, const char *s, const char *e ) {
	MK_ASSERT( v != (MkVariable)0 );
	MK_ASSERT( s != (const char *)0 );

	if( !e ) {
		e = strchr( s, '\0' );
	}

	mk_sl_resize( v->values, 1 );
	mk_sl_set( v->values, 0, mk_com_va( "%.*s", (int)(ptrdiff_t)( e - s ), s ) );
}
void mk_v_setValueByStr( MkVariable v, const char *s ) {
	MK_ASSERT( v != (MkVariable)0 );
	MK_ASSERT( s != (const char *)0 );

	mk_sl_resize( v->values, 1 );
	mk_sl_set( v->values, 0, s );
}
void mk_v_setValueByVariable( MkVariable v, MkVariable other ) {
	stringBuilder_t sb;

	MK_ASSERT( v != (MkVariable)0 );

	sb_init( &sb, 0 );
	sb_pushVar( &sb, other );
	mk_v_setValueByStr( v, sb_done( &sb ) );
	mk_com_memory( (void*)sb.buffer, 0 );
}

static const char *strchrz( const char *s, const char *e, char ch ) {
	size_t n;
	const char *p;

	n = (size_t)(ptrdiff_t)( e - s );

	if( ( p = memchr( s, ch, n ) ) == (const char *)0 ) {
		p = e;
	}

	return p;
}
char *mk_vs_evalSubstr_r( MkVariableSet vs, const char *s, const char *e ) {
	stringBuilder_t sb;
	const char *p, *q;
	MkVariable v;

	MK_ASSERT( vs != (MkVariableSet)0 );
	MK_ASSERT( s != (const char *)0 );

	if( e == (const char *)0 ) {
		e = strchr( s, '\0' );
	}

	sb_init( &sb, 0 );

	p = s;
	for(;;) {
		q = strchrz( p, e, '$' );
		sb_pushSubstr( &sb, p, q );

		p = q;
		if( p == e ) {
			break;
		}

		MK_ASSERT( *p == '$' );
		++p;

		if( *p == '$' ) {
			sb_pushChar( &sb, '$' );
			++p;
			continue;
		}

		if( *p == '(' ) {
			const char *next;
			char *evalstr;
			int level = 1;
			int hasEval = 0;

			do {
				++p;
			} while( p != e && *(unsigned char *)p <= ' ' );

			for( q = p; q != e; ++q ) {
				if( *q == '(' ) {
					++level;
					continue;
				}
				
				if( *q == ')' ) {
					if( --level == 0 ) {
						break;
					}

					continue;
				}

				if( *q == '$' ) {
					if( q + 1 != e ) {
						if( q[1] == '$' ) {
							++q;
							continue;
						}

						if( q[1] == ')' ) {
							continue;
						}

						hasEval = 1;
					}
				}
			}

			next = q;
			if( next != e ) {
				++next;
			}

			while( q != p && *(const unsigned char *)( q - 1 ) <= ' ' ) {
				--q;
			}

			if( hasEval ) {
				evalstr = mk_vs_evalSubstr_r( vs, p, q );
				v       = mk_vs_findVar( vs, evalstr );
				evalstr = (char *)mk_com_memory( (void*)evalstr, 0 );
			} else {
				v = mk_vs_findVarBySubstr( vs, p, q );
			}

			p = next;
		} else {
			q = p + 1;
			v = mk_vs_findVarBySubstr( vs, p, q );

			p = q;
		}

		sb_pushVar( &sb, v );
	}

	return sb_done( &sb );
}
char *mk_vs_eval( MkVariableSet vs, const char *s ) {
	return mk_vs_evalSubstr_r( vs, s, (const char *)0 );
}
