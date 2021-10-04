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
#include "mk-basic-stringBuilder.h"
#include "mk-basic-stringList.h"

#include <string.h>

/*

	COMMAND FORMAT
	==============

	Command strings are written as normal shell strings, but can have special
	variables within them. The variables are written similar to Makefiles.

	For example, "Hello $(Sekai)" would expand to "Hello World" if the variable
	"Sekai" had the value "World" stored within it.

	Variables can be strings or arrays.

*/

struct MkCommand_s {
	struct MkCommand_s *c_prev, *c_next;

	char *name;
	unsigned int nameHash;

	MkCmd_Eval_fn_t eval_fn;
};

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

static struct MkCommand_s *g_c_head = NULL, *g_c_tail = NULL;
static struct MkVariableSet_s *g_vs_head = NULL, *g_vs_tail = NULL;

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

static void link_command( MkCommand cmd ) {
	cmd->c_next = (MkCommand)0;

	if( ( cmd->c_prev = g_c_tail ) != (MkCommand)0 ) {
		g_c_tail->c_next = cmd;
	} else {
		g_c_head = cmd;
	}
	g_c_tail = cmd;
}
static void unlink_command( MkCommand cmd ) {
	if( cmd->c_prev != (MkCommand)0 ) {
		cmd->c_prev->c_next = cmd->c_next;
	} else {
		g_c_head = cmd->c_next;
	}

	if( cmd->c_next != (MkCommand)0 ) {
		cmd->c_next->c_prev = cmd->c_prev;
	} else {
		g_c_tail = cmd->c_prev;
	}

	cmd->c_prev = (MkCommand)0;
	cmd->c_next = (MkCommand)0;
}

MkCommand mk_cmd_new( const char *name, MkCmd_Eval_fn_t eval ) {
	unsigned int nameHash;
	MkCommand cmd;

	MK_ASSERT_MSG( name != (const char *)0, "Commands must have a valid name at creation" );
	MK_ASSERT_MSG( eval != (MkCmd_Eval_fn_t)0, "Commands must have a valid evaluation function at creation" );

	nameHash = murmur3( name, (const char *)0 );

	if( ( cmd = mk_cmd_findByStr( name ) ) != (MkCommand)0 ) {
		MK_ASSERT_MSG( cmd != (MkCommand)0, "Command has already been registered" );
		return (MkCommand)0;
	}

	cmd = (MkCommand)mk_com_memory( (void*)0, sizeof(*cmd) );

	cmd->name     = mk_com_strdup( name );
	cmd->nameHash = nameHash;

	cmd->eval_fn = eval;

	link_command( cmd );

	return cmd;
}
void mk_cmd_delete( MkCommand cmd ) {
	if( !cmd ) {
		return;
	}

	unlink_command( cmd );

	cmd->name = (char *)mk_com_memory( (void*)cmd->name, 0 );

	mk_com_memory( (void*)cmd, 0 );
}

MkCommand mk_cmd_findBySubstr( const char *s, const char *e ) {
	MkCommand cmd;
	unsigned int findHash;
	
	MK_ASSERT( s != (const char *)0 );

	if( !e ) {
		e = strchr( s, '\0' );
	}

	findHash = murmur3( s, e );

	for( cmd = g_c_head; cmd != (MkCommand)0; cmd = cmd->c_next ) {
		if( cmd->nameHash == findHash ) {
			return cmd;
		}
	}

	return (MkCommand)0;
}
MkCommand mk_cmd_findByStr( const char *s ) {
	return mk_cmd_findBySubstr( s, (const char *)0 );
}

int mk_cmd_eval( MkCommand cmd, MkVariableSet context, MkStrList args, MkStrList results ) {
	MK_ASSERT( context != (MkVariableSet)0 );
	MK_ASSERT( args != (MkStrList)0 );
	MK_ASSERT( results != (MkStrList)0 );

	if( !cmd ) {
		return -1;
	}

	MK_ASSERT( cmd->eval_fn != (MkCmd_Eval_fn_t)0 );

	return cmd->eval_fn( cmd, context, args, results );
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
	MkStringBuilder sb;

	MK_ASSERT( v != (MkVariable)0 );

	mk_sb_init( &sb, 0 );
	mk_sb_pushVar( &sb, other );
	mk_v_setValueByStr( v, mk_sb_done( &sb ) );
	mk_com_memory( (void*)sb.buffer, 0 );
}

static const char *strchrz( const char *s, const char *e, char ch ) {
	return mk_com_memchrz( s, (size_t)(ptrdiff_t)( e - s ), ch );
}

char *mk_vs_evalSubstr_r( MkVariableSet vs, const char *s, const char *e ) {
	MkStringBuilder sb, cmdsb;
	const char *p, *q;
	MkVariable v;
	MkCommand cmd;
	MkStrList args, results;

	MK_ASSERT( vs != (MkVariableSet)0 );
	MK_ASSERT( s != (const char *)0 );

	if( e == (const char *)0 ) {
		e = strchr( s, '\0' );
	}

	mk_sb_init( &sb, 0 );
	mk_sb_init( &cmdsb, 0 );

	args    = mk_sl_new();
	results = mk_sl_new();

	p = s;
	for(;;) {
		q = strchrz( p, e, '$' );
		mk_sb_pushSubstr( &sb, p, q );

		p = q;
		if( p == e ) {
			break;
		}

		MK_ASSERT( *p == '$' );
		++p;

		if( *p == '$' ) {
			mk_sb_pushChar( &sb, '$' );
			++p;
			continue;
		}

		if( *p == '(' ) {
			const char *expr_s, *expr_e;
			const char *name_s, *name_e;
			const char *args_s, *args_e;
			const char *next;
			char *evalstr;
			int level = 1;
			int hasEval = 0, searchCmd = 0;

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

			expr_s = p;
			expr_e = q;

			if( hasEval ) {
				evalstr = mk_vs_evalSubstr_r( vs, expr_s, expr_e );

				expr_s = evalstr;
				expr_e = strchr( evalstr, '\0' );
			}

			name_s = expr_s;
			if( *name_s == '/' ) {
				name_s    += 1;
				searchCmd  = 1;
			}

			args_s = expr_e;
			args_e = args_s;

			name_e = name_s;
			while( name_e != expr_e ) {
				if( *name_e == ' ' || *name_e == ',' ) {
					args_s    = name_e + 1;
					searchCmd = 1;
					break;
				}

				if( *name_e == '\"' ) {
					args_s    = name_e;
					searchCmd = 1;
					break;
				}

				++name_e;
			}

			mk_sl_clear( args );

			p = args_s;
			for(;;) {
				/* skip spaces */
				while( p != args_e && *(const unsigned char *)p <= ' ' ) {
					++p;
				}

				q = p;
				if( q == args_e ) {
					break;
				}

				mk_sb_clear( &cmdsb );

				/* quoted add */
				if( *q == '\"' ) {
					++q;
					while( q != args_e && *q != '\"' ) {
						if( q + 1 != args_e && *q == '\\' ) {
							++q;
						}
						++q;
					}
					if( q != args_e && *q == '\"' ) {
						++q;
					}
					mk_sb_quoteAndPushSubstr( &cmdsb, p, q );
				/* plain add */
				} else {
					const char *qq;

					while( q != args_e && *q != ',' ) {
						++q;
					}

					qq = q;
					
					if( q != args_e ) {
						while( q != p && *(const unsigned char *)( q - 1 ) <= ' ' ) {
							--q;
						}
					}

					mk_sb_pushSubstr( &cmdsb, p, q );

					q = qq;
					if( q != args_e && *q == ',' ) {
						++q;
					}
				}

				p = q;

				mk_sl_pushBack( args, mk_sb_done( &cmdsb ) );
			}

			cmd = mk_cmd_findBySubstr( name_s, name_e );
			if( !searchCmd && !cmd ) {
				v = mk_vs_findVarBySubstr( vs, name_s, name_e );
			} else {
				v = (MkVariable)0;
			}

			if( hasEval ) {
				evalstr = (char *)mk_com_memory( (void*)evalstr, 0 );
			}

			p = next;
		} else {
			q = p + 1;

			cmd = (MkCommand)0;
			v   = mk_vs_findVarBySubstr( vs, p, q );

			p = q;
		}

		if( cmd != (MkCommand)0 ) {
			int cmdExitStatus;

			mk_sl_clear( results );
			if( ( cmdExitStatus = mk_cmd_eval( cmd, vs, args, results ) ) == 0 ) {
				size_t i, n;
				n = mk_sl_getSize( results );
				for( i = 0; i < n; ++i ) {
					mk_sb_quoteAndPushStr( &sb, mk_sl_at( results, i ) );
				}
			}
		} else if( v != (MkVariable)0 ) {
			mk_sb_pushVar( &sb, v );
		}
	}

	(void)mk_com_memory( cmdsb.buffer, 0 );

	return mk_sb_done( &sb );
}
char *mk_vs_eval( MkVariableSet vs, const char *s ) {
	return mk_vs_evalSubstr_r( vs, s, (const char *)0 );
}
