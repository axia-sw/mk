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
#include "mk-build-autolib.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-logging.h"
#include "mk-basic-sourceBuffer.h"
#include "mk-build-library.h"
#include "mk-build-project.h"
#include "mk-defs-config.h"

#include <stddef.h>
#include <string.h>

struct MkAutolink_s *mk__g_al_head = (struct MkAutolink_s *)0;
struct MkAutolink_s *mk__g_al_tail = (struct MkAutolink_s *)0;

/* allocate a new auto-link entry */
MkAutolink mk_al_new( void ) {
	MkAutolink al;
	size_t i;

	al = (MkAutolink)mk_com_memory( (void *)0, sizeof( *al ) );

	for( i = 0; i < kMkNumOS; i++ ) {
		al->header[i] = (char *)0;
	}
	al->lib = (char *)0;

	al->next = (struct MkAutolink_s *)0;
	if( ( al->prev = mk__g_al_tail ) != (struct MkAutolink_s *)0 ) {
		mk__g_al_tail->next = al;
	} else {
		mk__g_al_head = al;
	}
	mk__g_al_tail = al;

	return al;
}

/* deallocate an existing auto-link entry */
void mk_al_delete( MkAutolink al ) {
	size_t i;

	if( !al ) {
		return;
	}

	for( i = 0; i < kMkNumOS; i++ ) {
		al->header[i] = (char *)mk_com_memory( (void *)al->header[i], 0 );
	}
	al->lib = (char *)mk_com_memory( (void *)al->lib, 0 );

	if( al->prev ) {
		al->prev->next = al->next;
	}
	if( al->next ) {
		al->next->prev = al->prev;
	}

	if( mk__g_al_head == al ) {
		mk__g_al_head = al->next;
	}
	if( mk__g_al_tail == al ) {
		mk__g_al_tail = al->prev;
	}

	mk_com_memory( (void *)al, 0 );
}

/* deallocate all existing auto-link entries */
void mk_al_deleteAll( void ) {
	while( mk__g_al_head ) {
		mk_al_delete( mk__g_al_head );
	}
}

/* set a header for an auto-link entry (determines whether to auto-link) */
void mk_al_setHeader( MkAutolink al, int sys, const char *header ) {
	static char rel[PATH_MAX];
	const char *p;

	MK_ASSERT( al != (MkAutolink)0 );
	MK_ASSERT( sys >= 0 && sys < kMkNumOS );

	if( header != (const char *)0 ) {
		mk_com_relPathCWD( rel, sizeof( rel ), header );
		p = rel;
	} else {
		p = (const char *)0;
	}

	al->header[sys] = mk_com_dup( al->header[sys], p );
}

/* set the library an auto-link entry refers to */
void mk_al_setLib( MkAutolink al, const char *libname ) {
	MK_ASSERT( al != (MkAutolink)0 );

	al->lib = mk_com_dup( al->lib, libname );
}

/* retrieve a header of an auto-link entry */
const char *mk_al_getHeader( MkAutolink al, int sys ) {
	MK_ASSERT( al != (MkAutolink)0 );
	MK_ASSERT( sys >= 0 && sys < kMkNumOS );

	return al->header[sys];
}

/* retrieve the library an auto-link entry refers to */
const char *mk_al_getLib( MkAutolink al ) {
	MK_ASSERT( al != (MkAutolink)0 );

	return al->lib;
}

/* find an auto-link entry by header and system */
MkAutolink mk_al_find( int sys, const char *header ) {
	MkAutolink al;

	MK_ASSERT( sys >= 0 && sys < kMkNumOS );
	MK_ASSERT( header != (const char *)0 );

	for( al = mk__g_al_head; al; al = al->next ) {
		if( !al->header[sys] ) {
			continue;
		}

		if( mk_com_matchPath( al->header[sys], header ) ) {
			return al;
		}
	}

	return (MkAutolink)0;
}

/* find or create an auto-link entry by header and system */
MkAutolink mk_al_lookup( int sys, const char *header ) {
	MkAutolink al;

	MK_ASSERT( sys >= 0 && sys < kMkNumOS );
	MK_ASSERT( header != (const char *)0 );

	if( !( al = mk_al_find( sys, header ) ) ) {
		al = mk_al_new();
		mk_al_setHeader( al, sys, header );
	}

	return al;
}

/* retrieve the flags needed for linking to a library based on its header */
const char *mk_al_autolink( int sys, const char *header ) {
	MkAutolink al;
	MkLib lib;

	MK_ASSERT( sys >= 0 && sys < kMkNumOS );
	MK_ASSERT( header != (const char *)0 );

	if( !( al = mk_al_find( sys, header ) ) ) {
		return (const char *)0;
	}

	if( !( lib = mk_lib_find( mk_al_getLib( al ) ) ) ) {
		return (const char *)0;
	}

	return mk_lib_getFlags( lib, sys );
}

/* recursively add autolinks for a lib from an include directory */
void mk_al_managePackage_r( const char *libname, int sys, const char *incdir ) {
	struct dirent *dp;
	MkAutolink al;
	DIR *d;

	mk_dbg_outf( "*** PKGAUTOLINK dir:\"%s\" lib:\"%s\" ***\n",
	    incdir, libname );

	mk_fs_enter( incdir );
	d = mk_fs_openDir( "./" );
	if( !d ) {
		mk_fs_leave();
		mk_dbg_outf( "    mk_al_managePackage_r failed to enter directory\n" );
		return;
	}
	while( ( dp = readdir( d ) ) != (struct dirent *)0 ) {
		if( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) ) {
			continue;
		}

		if( mk_fs_isDir( dp->d_name ) ) {
			mk_al_managePackage_r( libname, sys, dp->d_name );
			continue;
		}

		{
			char realpath[PATH_MAX];

			if( !mk_fs_realPath( dp->d_name, realpath, sizeof( realpath ) ) ) {
				continue;
			}

			al = mk_al_new();
			mk_al_setLib( al, libname );
			mk_al_setHeader( al, sys, realpath );

			mk_dbg_outf( "    autolinking path: \"%s\"\n", realpath );
		}
	}
	mk_fs_closeDir( d );
	mk_fs_leave();
}

/* -------------------------------------------------------------------------- */

/* skip past whitespace, including comments */
static void mk_al__lexWhite( MkBuffer buf ) {
	int r;
	do {
		r = 0;

		mk_buf_skipWhite( buf );

		r |= mk_buf_skipLineIfStartsWith( buf, "#" );
		r |= mk_buf_skipLineIfStartsWith( buf, "//" );
	} while( r );
}
/* check whether a given character is part of a "name" token */
static int mk_al__isLexName( char c ) {
	if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ) {
		return 1;
	}

	if( c >= '0' && c <= '9' ) {
		return 1;
	}

	if( c == '-' || c == '_' || c == '.' ) {
		return 1;
	}

	return 0;
}
/* check whether a given character is considered a valid delimiter (can break a name token) */
static int mk_al__isLexDelim( char c ) {
	if( (unsigned char)( c <= ' ' ) ) {
		return 1;
	}

	return c == '{' || c == '}' || c == ':' || c == ',';
}
/* lex in a name token; returns 1 on success or if not a name token; 0 on error */
static int mk_al__lexName( char *pszDst, size_t cDstMax, MkBuffer buf ) {
	char *p, *e;

	MK_ASSERT( pszDst != (char *)0 );
	MK_ASSERT( cDstMax > 1 );

	p = pszDst;
	e = pszDst + cDstMax - 1;

	for( ;; ) {
		char c;

		c = mk_buf_peek( buf );

		if( mk_al__isLexName( c ) ) {
			if( p == e ) {
				mk_buf_errorf( buf, "Length of libary name is too long; must be under %u bytes", (unsigned)cDstMax );
				pszDst[0] = '\0';
				return 0;
			}

			*p++ = c;
			(void)mk_buf_read( buf );
			continue;
		} else if( p == pszDst ) {
			break;
		}

		if( mk_al__isLexDelim( c ) ) {
			*p = '\0';
			return 1;
		}

		mk_buf_errorf( buf, "Unexpected character in library name: 0x%.2X (%c)", (unsigned)c, c );
		break;
	}

	pszDst[0] = '\0';
	return 0;
}
/* lex in a quote (string); returns 0 on error or if not a string (CONSISTENCY) */
static int mk_al__lexQuote( char *pszDst, size_t cDstMax, MkBuffer buf ) {
	char *p, *e;

	MK_ASSERT( pszDst != (char *)0 );
	MK_ASSERT( cDstMax > 3 );

	p = pszDst;
	e = pszDst + cDstMax - 1;

	if( !mk_buf_advanceIfCharEq( buf, '\"' ) ) {
		return 0;
	}

	for( ;; ) {
		char c;

		if( buf->ptr == buf->endPtr ) {
			mk_buf_errorf( buf, "Unexpected end of file while reading string" );
			pszDst[0] = '\0';
			return 0;
		}

		if( ( c = mk_buf_read( buf ) ) == '\"' ) {
			break;
		}

		if( c == '\\' ) {
			c = mk_buf_read( buf );

			switch( c ) {
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case '\'':
				c = '\'';
				break;
			case '\"':
				c = '\"';
				break;
			case '\?':
				c = '\?';
				break;
			default:
				mk_buf_errorf( buf, "Unknown escape sequence char 0x%.2X ('%c'); ignoring", (unsigned)c, c );
				break;
			}
		}

		if( p == e ) {
			mk_buf_errorf( buf, "String is too long; keep under %u bytes", (unsigned)cDstMax );
			*p = '\0';
			return 0;
		}

		*p++ = c;
	}

	*p = '\0';
	return 1;
}

typedef enum {
	kMkALTok_Error = -1,
	kMkALTok_EOF   = 0,

	kMkALTok_LBrace = '{',
	kMkALTok_RBrace = '}',
	kMkALTok_Colon  = ':',
	kMkALTok_Comma  = ',',

	kMkALTok_Name = 128,
	kMkALTok_Quote,

	kMkALTok_Null
} MkALToken_t;

/* lex a token from the buffer */
MkALToken_t mk_al__lex( char *pszDst, size_t cDstMax, MkBuffer buf ) {
	char *bufPos;

	MK_ASSERT( pszDst != (char *)0 );
	MK_ASSERT( cDstMax > 7 ); /* "(error)" == 7 bytes */
	MK_ASSERT( buf != (MkBuffer)0 );

	mk_al__lexWhite( buf );

	if( ( bufPos = buf->ptr ) == buf->endPtr ) {
		*pszDst = '\0';
		return kMkALTok_EOF;
	}

	if( mk_al__lexName( pszDst, cDstMax, buf ) && pszDst[0] != '\0' ) {
		if( *pszDst == 'n' && strcmp( pszDst, "null" ) == 0 ) {
			return kMkALTok_Null;
		}

		return kMkALTok_Name;
	}

	if( mk_buf_peek( buf ) == '\"' ) {
		if( !mk_al__lexQuote( pszDst, cDstMax, buf ) ) {
			buf->ptr = bufPos;
			mk_buf_errorf( buf, "Incomplete quote starts here" );
			buf->ptr = buf->endPtr;
			return kMkALTok_Error;
		}

		return kMkALTok_Quote;
	}

	if( mk_buf_advanceIfCharEq( buf, '{' ) ) {
		mk_com_strcpy( pszDst, cDstMax, "{" );
		return kMkALTok_LBrace;
	}

	if( mk_buf_advanceIfCharEq( buf, '}' ) ) {
		mk_com_strcpy( pszDst, cDstMax, "}" );
		return kMkALTok_RBrace;
	}

	if( mk_buf_advanceIfCharEq( buf, ':' ) ) {
		mk_com_strcpy( pszDst, cDstMax, ":" );
		return kMkALTok_Colon;
	}

	if( mk_buf_advanceIfCharEq( buf, ',' ) ) {
		mk_com_strcpy( pszDst, cDstMax, "," );
		return kMkALTok_Comma;
	}

	mk_com_strcpy( pszDst, cDstMax, "(error)" );

	mk_buf_errorf( buf, "Unrecognized character 0x%.2X ('%c')", (unsigned)mk_buf_peek( buf ), mk_buf_peek( buf ) );
	buf->ptr = buf->endPtr;

	return kMkALTok_Error;
}

/* retrieve the name of a token */
const char *mk_al__getTokenName( MkALToken_t tok ) {
	switch( tok ) {
	case kMkALTok_Error:
		return "(error)";
	case kMkALTok_EOF:
		return "EOF";

	case kMkALTok_LBrace:
		return "'{'";
	case kMkALTok_RBrace:
		return "'}'";
	case kMkALTok_Colon:
		return "':'";
	case kMkALTok_Comma:
		return "','";

	case kMkALTok_Name:
		return "name";
	case kMkALTok_Quote:
		return "quote";

	case kMkALTok_Null:
		return "'null'";
	}

	return "(invalid)";
}

/* expect any of the given token types (returns index of the token in the array, or -1 on error/unmatched) */
int mk_al__expectAny( char *pszDst, size_t cDstMax, MkBuffer buf, const MkALToken_t *pToks, size_t cToks ) {
	char szErrorBuf[256];
	MkALToken_t tok;
	size_t i;

	if( ( tok = mk_al__lex( pszDst, cDstMax, buf ) ) == kMkALTok_Error ) {
		return -1;
	}

	for( i = 0; i < cToks; ++i ) {
		if( pToks[i] == tok ) {
			return (int)(unsigned)i;
		}
	}

	mk_com_strcpy( szErrorBuf, sizeof( szErrorBuf ), "Expected " );
	for( i = 0; i < cToks; ++i ) {
		mk_com_strcat( szErrorBuf, sizeof( szErrorBuf ), mk_al__getTokenName( pToks[i] ) );
		if( i + 1 < cToks ) {
			mk_com_strcat( szErrorBuf, sizeof( szErrorBuf ), ( i + 2 == cToks ) ? ", or " : ", " );
		}
	}
	mk_com_strcat( szErrorBuf, sizeof( szErrorBuf ), ", but got " );
	mk_com_strcat( szErrorBuf, sizeof( szErrorBuf ), mk_al__getTokenName( tok ) );

	mk_buf_errorf( buf, "%s", szErrorBuf );
	return -1;
}
/* expect any of the given three token types (returns -1 on failure, or 0, 1, or 2 on success) */
int mk_al__expect3( char *pszDst, size_t cDstMax, MkBuffer buf, MkALToken_t tok1, MkALToken_t tok2, MkALToken_t tok3 ) {
	MkALToken_t toks[3];

	toks[0] = tok1;
	toks[1] = tok2;
	toks[2] = tok3;

	return mk_al__expectAny( pszDst, cDstMax, buf, toks, sizeof( toks ) / sizeof( toks[0] ) );
}
/* expect any of the given two token types (returns -1 on failure, or 0 or 1 on success) */
int mk_al__expect2( char *pszDst, size_t cDstMax, MkBuffer buf, MkALToken_t tok1, MkALToken_t tok2 ) {
	MkALToken_t toks[2];

	toks[0] = tok1;
	toks[1] = tok2;

	return mk_al__expectAny( pszDst, cDstMax, buf, toks, sizeof( toks ) / sizeof( toks[0] ) );
}
/* expect the given token type (returns 0 on failure, 1 on success) */
int mk_al__expect( char *pszDst, size_t cDstMax, MkBuffer buf, MkALToken_t tok ) {
	return mk_al__expectAny( pszDst, cDstMax, buf, &tok, 1 ) + 1;
}

/* retrieve the platform (kMkOS_*) from the string, or fail with -1 */
int mk_al__stringToOS( const char *pszPlat ) {
	if( strcmp( pszPlat, MK_PLATFORM_OS_NAME_MSWIN ) == 0 ) {
		return kMkOS_MSWin;
	}

	if( strcmp( pszPlat, MK_PLATFORM_OS_NAME_UWP ) == 0 ) {
		return kMkOS_UWP;
	}

	if( strcmp( pszPlat, MK_PLATFORM_OS_NAME_CYGWIN ) == 0 ) {
		return kMkOS_Cygwin;
	}

	if( strcmp( pszPlat, MK_PLATFORM_OS_NAME_LINUX ) == 0 ) {
		return kMkOS_Linux;
	}

	if( strcmp( pszPlat, MK_PLATFORM_OS_NAME_MACOSX ) == 0 ) {
		return kMkOS_MacOSX;
	}

	if( strcmp( pszPlat, MK_PLATFORM_OS_NAME_UNIX ) == 0 ) {
		return kMkOS_Unix;
	}

	return -1;
}

/* parse a series of comma-delimited platform names until a ':' is reached; expects pszDst to already have a name */
int mk_al__readOSNames( int *pDstPlats, size_t cDstPlatsMax, size_t *pcDstPlats, char *pszDst, size_t cDstMax, MkBuffer buf ) {
	size_t cPlats = 0;
	int r;

	MK_ASSERT( pDstPlats != (int *)0 );
	MK_ASSERT( cDstPlatsMax >= kMkNumOS );
	MK_ASSERT( pcDstPlats != (size_t *)0 );
	MK_ASSERT( pszDst != (char *)0 );
	MK_ASSERT( cDstMax > 0 );
	MK_ASSERT( buf != (MkBuffer)0 );

	*pcDstPlats = 0;

	for( ;; ) {
#if MK_DEBUG_AUTOLIBCONF_ENABLED
		mk_dbg_outf( "Got platform: \"%s\"\n", pszDst );
#endif
		if( ( pDstPlats[cPlats] = mk_al__stringToOS( pszDst ) ) == -1 ) {
			mk_buf_errorf( buf, "Unknown platform \"%s\"", pszDst );
			return 0;
		}

		MK_ASSERT( pDstPlats[cPlats] >= 0 && pDstPlats[cPlats] < kMkNumOS );

		++cPlats;

		if( ( r = mk_al__expect2( pszDst, cDstMax, buf, kMkALTok_Comma, kMkALTok_Colon ) ) == -1 ) {
			return 0;
		}

		if( r == 0 /* comma */ ) {
			if( cPlats == cDstPlatsMax ) {
				mk_buf_errorf( buf, "Too many platforms specified; keep under %u", (unsigned)cDstPlatsMax );
				return 0;
			}

			continue;
		}

		MK_ASSERT( r == 1 /* colon */ );
		break;
	}

	*pcDstPlats = cPlats;
	return 1;
}

/* parse the library's flags and the left-brace; returns 0 on fail, or 1 on success */
int mk_al__readLibFlagsAndLBrace( char *pszDst, size_t cDstMax, MkBuffer buf, MkLib lib ) {
	size_t cPlats, i;
	int plat[kMkNumOS];
	int r;

	MK_ASSERT( pszDst != (char *)0 );
	MK_ASSERT( cDstMax > 0 );
	MK_ASSERT( buf != (MkBuffer)0 );
	MK_ASSERT( lib != (MkLib)0 );

#if MK_DEBUG_AUTOLIBCONF_ENABLED
	mk_dbg_outf( "mk_al__readLibFlagsAndLBrace...\n" );
#endif

	/* keep going until we get an error or a left-brace */
	for( ;; ) {
		if( ( r = mk_al__expect3( pszDst, cDstMax, buf, kMkALTok_Name, kMkALTok_Quote, kMkALTok_LBrace ) ) == -1 ) {
			break;
		}

		cPlats = 0;
		switch( r ) {
		case 0: /* name */
			/* read the sequence of platform names (e.g., mswin,uwp,cygwin: etc...) */
			if( !mk_al__readOSNames( plat, sizeof( plat ) / sizeof( plat[0] ), &cPlats, pszDst, cDstMax, buf ) ) {
				return 0;
			}

			/* immediately following the platform names must be a quote or "null" */
			if( ( r = mk_al__expect2( pszDst, cDstMax, buf, kMkALTok_Quote, kMkALTok_Null ) ) == -1 ) {
				return 0;
			}

			/* if we got null, then we can't share the code */
			if( r == 1 /* null */ ) {
#if MK_DEBUG_AUTOLIBCONF_ENABLED
				mk_dbg_outf( "Setting platform linker flags to null.\n" );
#endif
				for( i = 0; i < cPlats; ++i ) {
					mk_lib_setFlags( lib, plat[i], (const char *)0 );
				}

				break;
			}

			/* fallthrough */
		case 1: /* quote */
			/* if we got a string and no platforms were specified then assume all */
			if( !cPlats ) {
#if MK_DEBUG_AUTOLIBCONF_ENABLED
				mk_dbg_outf( "No platforms given; assuming all for this flag.\n" );
#endif

				while( cPlats < (size_t)kMkNumOS ) {
					plat[cPlats] = (int)(unsigned)cPlats;
					++cPlats;
				}
			}

#if MK_DEBUG_AUTOLIBCONF_ENABLED
			mk_dbg_outf( "Setting platforms to linker flag: \"%s\"\n", pszDst );
#endif

			/* set each platform's library flags */
			for( i = 0; i < cPlats; ++i ) {
				mk_lib_setFlags( lib, plat[i], pszDst );
			}

			break;

		case 2: /* left-brace */
#if MK_DEBUG_AUTOLIBCONF_ENABLED
			mk_dbg_outf( "Got left-brace.\n\n" );
#endif
			return 1;
		}
	}

	/* we got an error */
	return 0;
}
/* parse the library's headers and the right-brace; returns 0 on fail, or 1 on success */
int mk_al__readLibHeadersAndRBrace( char *pszDst, size_t cDstMax, MkBuffer buf, MkLib lib ) {
	size_t cPlats, i;
	int plat[kMkNumOS];
	int r;

	MK_ASSERT( pszDst != (char *)0 );
	MK_ASSERT( cDstMax > 0 );
	MK_ASSERT( buf != (MkBuffer)0 );
	MK_ASSERT( lib != (MkLib)0 );

#if MK_DEBUG_AUTOLIBCONF_ENABLED
	mk_dbg_outf( "mk_al__readLibHeadersAndRBrace...\n" );
#endif

	/* keep going until we get an error or a right-brace */
	for( ;; ) {
		if( ( r = mk_al__expect3( pszDst, cDstMax, buf, kMkALTok_Name, kMkALTok_Quote, kMkALTok_RBrace ) ) == -1 ) {
			break;
		}

		cPlats = 0;
		switch( r ) {
		case 0: /* name */
			/* read the sequence of platform names (e.g., mswin,uwp,cygwin: etc...) */
			if( !mk_al__readOSNames( plat, sizeof( plat ) / sizeof( plat[0] ), &cPlats, pszDst, cDstMax, buf ) ) {
				return 0;
			}

			/* immediately following the platform names must be a quote */
			if( !mk_al__expect( pszDst, cDstMax, buf, kMkALTok_Quote ) ) {
				return 0;
			}

			/* fallthrough */
		case 1: /* quote */
			/* cannot have an empty string for autolinks */
			if( *pszDst == '\0' ) {
				mk_buf_errorf( buf, "Empty strings cannot be used for autolinks" );
				return 0;
			}

			/* if we got a string and no platforms were specified then assume all */
			if( !cPlats ) {
#if MK_DEBUG_AUTOLIBCONF_ENABLED
				mk_dbg_outf( "No platforms given; assuming all for this header.\n" );
#endif
				while( cPlats < (size_t)kMkNumOS ) {
					plat[cPlats] = (int)(unsigned)cPlats;
					++cPlats;
				}
			}

#if MK_DEBUG_AUTOLIBCONF_ENABLED
			mk_dbg_outf( "Adding autolink entries to %s for header: \"%s\"\n",
			    cPlats == 1 ? "this platform" : "these platforms",
			    pszDst );
#endif

			/* create the autolinks (pszDst has the header name) */
			for( i = 0; i < cPlats; ++i ) {
				MkAutolink al;

				al = mk_al_lookup( plat[i], pszDst );
				MK_ASSERT( al != (MkAutolink)0 && "mk_al_lookup() should never return NULL" );

				/* FIXME: This could be a bit higher performance */
				mk_al_setLib( al, mk_lib_getName( lib ) );
			}

			break;

		case 2: /* right-brace */
#if MK_DEBUG_AUTOLIBCONF_ENABLED
			mk_dbg_outf( "Got right-brace.\n" );
#endif
			return 1;
		}
	}

	/* we got an error */
	return 0;
}

/* parse a top-level library, returning 0 on failure or 1 on success */
int mk_al__readLib( char *pszDst, size_t cDstMax, MkBuffer buf ) {
	MkLib lib;
	int r;

	r = mk_al__expect2( pszDst, cDstMax, buf, kMkALTok_Name, kMkALTok_EOF );
	if( r != 0 /* not kMkALTok_Name */ ) {
		return r == 1; /* EOF is success */
	}

#if MK_DEBUG_AUTOLIBCONF_ENABLED
	mk_dbg_enter( "mk_al__readLib(\"%s\")", pszDst );
#endif

	/* find or create the library; if found we overwrite some stuff */
	lib = mk_lib_lookup( pszDst );
	MK_ASSERT( lib != (MkLib)0 );

	/* read the lib flags and the left-brace */
	if( !mk_al__readLibFlagsAndLBrace( pszDst, cDstMax, buf, lib ) ) {
#if MK_DEBUG_AUTOLIBCONF_ENABLED
		mk_dbg_leave();
#endif
		return 0;
	}

	/* read headers */
	if( !mk_al__readLibHeadersAndRBrace( pszDst, cDstMax, buf, lib ) ) {
#if MK_DEBUG_AUTOLIBCONF_ENABLED
		mk_dbg_leave();
#endif
		return 0;
	}

#if MK_DEBUG_AUTOLIBCONF_ENABLED
	mk_dbg_leave();
#endif

	/* done */
	return 1;
}

/* parse a file for autolinks and libraries */
int mk_al_loadConfig( const char *filename ) {
	MkBuffer buf;
	char szLexan[512];
	int r;

	MK_ASSERT( filename != (const char *)0 );

	if( !( buf = mk_buf_loadFile( filename ) ) ) {
		mk_dbg_outf( "%s: Failed to load autolink config\n", filename );
		return 0;
	}

	mk_dbg_enter( "mk_al_loadConfig(\"%s\")", filename );

	r = 1;
	do {
		if( !mk_al__readLib( szLexan, sizeof( szLexan ), buf ) ) {
			r = 0;
			break;
		}
	} while( buf->ptr != buf->endPtr );

	mk_dbg_leave();

	mk_buf_delete( buf );
	return r;
}
