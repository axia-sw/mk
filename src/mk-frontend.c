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
#include "mk-frontend.h"

#include "mk-basic-array.h"
#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-debug.h"
#include "mk-basic-fileSystem.h"
#include "mk-basic-logging.h"
#include "mk-basic-stringList.h"
#include "mk-basic-types.h"
#include "mk-build-autolib.h"
#include "mk-build-dependency.h"
#include "mk-build-engine.h"
#include "mk-build-library.h"
#include "mk-build-project.h"
#include "mk-build-projectFS.h"
#include "mk-defs-config.h"
#include "mk-defs-platform.h"
#include "mk-system-output.h"
#include "mk-version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct { const char *const header[3], *const lib; } autolinks[] = {
	/* OpenGL */
	{ { "GL/gl.h", "GL/gl.h", "OpenGL/OpenGL.h" },
		"opengl" },
	{ { (const char *)0, (const char *)0, "GL/gl.h" },
		"opengl" },
	{ { (const char *)0, (const char *)0, "OpenGL/gl.h" },
		"opengl" },

	/* OpenGL - GLU */
	{ { "GL/glu.h", "GL/glu.h", "OpenGL/glu.h" },
		"glu" },

	/* OpenGL - GLFW/GLEW */
	{ { "GL/glfw.h", "GL/glfw.h", "GL/glfw.h" },
		"glfw" },
	{ { "GLFW/glfw3.h", "GLFW/glfw3.h", "GLFW/glfw3.h" },
		"glfw3" },
	{ { "GL/glew.h", "GL/glew.h", "GL/glew.h" },
		"glew" },

	/* Vulkan */
	{ { "vulkan/vulkan.h", "vulkan/vulkan.h", "vulkan/vulkan.h" },
		"vulkan" },

	/* SDL */
	{ { "SDL/sdl.h", "SDL/sdl.h", "SDL/sdl.h" },
		"sdl" },
	{ { "SDL/sdl_mixer.h", "SDL/sdl_mixer.h", "SDL/sdl_mixer.h" },
		"sdl_mixer" },
	{ { "SDL/sdl_main.h", "SDL/sdl_main.h", "SDL/sdl_main.h" },
		"sdl_main" },

	/* SDL2 - EXPERIMENTAL */
	{ { "SDL2/SDL.h", "SDL2/SDL.h", "SDL2/SDL.h" },
		"sdl2" },

	/* SFML */
	{ { "SFML/Config.hpp", "SFML/Config.hpp", "SFML/Config.hpp" },
		"sfml" },

	/* Ogg/Vorbis/Opus */
	{ { "ogg/ogg.h", "ogg/ogg.h", "ogg/ogg.h" },
		"ogg" },
	{ { "vorbis/codec.h", "vorbis/codec.h", "vorbis/codec.h" },
		"vorbis" },
	{ { "vorbis/vorbisenc.h", "vorbis/vorbisenc.h", "vorbis/vorbisenc.h" },
		"vorbisenc" },
	{ { "vorbis/vorbisfile.h", "vorbis/vorbisfile.h", "vorbis/vorbisfile.h" },
		"vorbisfile" },
	{ { "opus/opus.h", "opus/opus.h", "opus/opus.h" },
		"opus" },
	{ { "opus/opusenc.h", "opus/opusenc.h", "opus/opusenc.h" },
		"opusenc" },
	{ { "opus/opusfile.h", "opus/opusfile.h", "opus/opusfile.h" },
		"opusfile" },

	/* Windows */
	{ { "winuser.h", (const char *)0, (const char *)0 },
		"user32" },
	{ { "winbase.h", (const char *)0, (const char *)0 },
		"kernel32" },
	{ { "shellapi.h", (const char *)0, (const char *)0 },
		"shell32" },
	{ { "ole.h", (const char *)0, (const char *)0 },
		"ole32" },
	{ { "commctrl.h", (const char *)0, (const char *)0 },
		"comctl32" },
	{ { "commdlg.h", (const char *)0, (const char *)0 },
		"comdlg32" },
	{ { "wininet.h", (const char *)0, (const char *)0 },
		"wininet" },
	{ { "mmsystem.h", (const char *)0, (const char *)0 },
		"winmm" },
	{ { "uxtheme.h", (const char *)0, (const char *)0 },
		"uxtheme" },
	{ { "wingdi.h", (const char *)0, (const char *)0 },
		"gdi32" },
	{ { "winsock2.h", (const char *)0, (const char *)0 },
		"winsock2" },

	/* POSIX */
	{ { (const char *)0, "time.h", "time.h" },
		"realtime" },
	{ { (const char *)0, "math.h", "math.h" },
		"math" },

	/* PNG */
	{ { "png.h", "png.h", "png.h" },
		"png" },

	/* BZip2 */
	{ { "bzlib.h", "bzlib.h", "bzlib.h" },
		"bzip2" },

	/* ZLib */
	{ { "zlib.h", "zlib.h", "zlib.h" },
		"z" },

	/* PThread */
	{ { "pthread.h", "pthread.h", "pthread.h" },
		"pthread" },

	/* Curses */
	{ { "curses.h", "curses.h", "curses.h" },
		"curses" },
};
static const struct { const char *const lib, *const flags[3]; } libs[] = {
	/* OpenGL (and friends) */
	{ "opengl",
		{ "-lopengl32", "-lGL", "-framework OpenGL" } },
	{ "glu",
		{ "-lglu32", "-lGLU", "-lGLU" } },
	{ "glfw",
		{ "-lglfw", "-lglfw", "-lglfw" } },
	{ "glfw3",
		{ "-lglfw", "-lglfw", "-lglfw" } },
	{ "glew",
		{ "-lglew32", "-lGLEW", "-lGLEW" } },

	/* Vulkan */
	{ "vulkan",
		{ "-lvulkan", "-lvulkan", "-lvulkan" } },

	/* SDL */
	{ "sdl",
		{ "-lSDL", "-lSDL", "-lSDL" } },
	{ "sdl_mixer",
		{ "-lSDL_mixer", "-lSDL_mixer", "-lSDL_mixer" } },
	{ "sdl_main",
		{ "-lSDLmain", "-lSDLmain", "-lSDLmain" } },

	/* SDL2 - EXPERIMENTAL */
	{ "sdl2",
		{ "-lSDL2 -luser32 -lkernel32 -lshell32 -lole32 -lwininet -lwinmm"
			" -limm32 -lgdi32 -loleaut32 -lversion -luuid",
			"-lSDL2", "-lSDL2" } },

	/* SFML */
	{ "sfml",
		{ "-lsfml-window-s -lsfml-graphics-s -lsfml-audio-s"
			" -lsfml-network-s -lsfml-main -lsfml-system-s",
			"-lsfml-window-s -lsfml-graphics-s -lsfml-audio-s"
			" -lsfml-network-s -lsfml-main -lsfml-system-s",
			"-lsfml-window-s -lsfml-graphics-s -lsfml-audio-s"
			" -lsfml-network-s -lsfml-main -lsfml-system-s" } },

	/* Ogg/Vorbis/Opus */
	{ "ogg",
		{ "-logg", "-logg", "-logg" } },
	{ "vorbis",
		{ "-lvorbis", "-lvorbis", "-lvorbis" } },
	{ "vorbisenc",
		{ "-lvorbisenc", "-lvorbisenc", "-lvorbisenc" } },
	{ "vorbisfile",
		{ "-lvorbisfile", "-lvorbisfile", "-lvorbisfile" } },
	{ "opus",
		{ "-lopus", "-lopus", "-lopus" } },
	{ "opusenc",
		{ "-lopusenc", "-lopusenc", "-lopusenc" } },
	{ "opusfile",
		{ "-lopusfile", "-lopusfile", "-lopusfile" } },

	/* Windows */
	{ "user32",
		{ "-luser32", (const char *)0, (const char *)0 } },
	{ "kernel32",
		{ "-lkernel32", (const char *)0, (const char *)0 } },
	{ "shell32",
		{ "-lshell32", (const char *)0, (const char *)0 } },
	{ "ole32",
		{ "-lole32", (const char *)0, (const char *)0 } },
	{ "comctl32",
		{ "-lcomctl32", (const char *)0, (const char *)0 } },
	{ "comdlg32",
		{ "-lcomdlg32", (const char *)0, (const char *)0 } },
	{ "wininet",
		{ "-lwininet", (const char *)0, (const char *)0 } },
	{ "winmm",
		{ "-lwinmm", (const char *)0, (const char *)0 } },
	{ "uxtheme",
		{ "-luxtheme", (const char *)0, (const char *)0 } },
	{ "gdi32",
		{ "-lgdi32", (const char *)0, (const char *)0 } },
	{ "winsock2",
		{ "-lws2_32", (const char *)0, (const char *)0 } },

	/* POSIX */
	{ "realtime",
		{ (const char *)0, "-lrt", (const char *)0 } },
	{ "math",
		{ (const char *)0, "-lm", "-lm" } },

	/* PNG */
	{ "png",
		{ "-lpng", "-lpng", "-lpng" } },

	/* BZip2 */
	{ "bzip2",
		{ "-lbzip2", "-lbzip2", "-lbzip2" } },

	/* ZLib */
	{ "z",
		{ "-lz", "-lz", "-lz" } },

	/* PThread */
	{ "pthread",
		{ "-lpthread", "-lpthread", "-lpthread" } },

	/* Curses */
	{ "curses",
		{ "-lncurses", "-lncurses", "-lncurses" } }
};

MkStrList mk__g_targets  = (MkStrList)0;
MkStrList mk__g_srcdirs  = (MkStrList)0;
MkStrList mk__g_incdirs  = (MkStrList)0;
MkStrList mk__g_libdirs  = (MkStrList)0;
MkStrList mk__g_pkgdirs  = (MkStrList)0;
MkStrList mk__g_tooldirs = (MkStrList)0;
MkStrList mk__g_dllsdirs = (MkStrList)0;

bitfield_t mk__g_flags          = 0;
MkColorMode_t mk__g_flags_color = MK__DEFAULT_COLOR_MODE_IMPL;

MkActions mk__g_actions = { .len = 0, .ptr = (MkAction *)0 };

void mk_front_pushSrcDir( const char *srcdir ) {
	if( !mk_fs_isDir( srcdir ) ) {
		mk_log_errorMsg( mk_com_va( "^F%s^&", srcdir ) );
		return;
	}

	mk_sl_pushBack( mk__g_srcdirs, srcdir );
}
void mk_front_pushIncDir( const char *incdir ) {
	char inc[PATH_MAX];

	if( !mk_fs_isDir( incdir ) ) {
		mk_log_errorMsg( mk_com_va( "^F%s^&", incdir ) );
		return;
	}

	mk_com_relPathCWD( inc, sizeof( inc ), incdir );
	mk_sl_pushBack( mk__g_incdirs, inc );
}
void mk_front_pushLibDir( const char *libdir ) {
#if 0
	/*
	 *	XXX: If project has not yet been built, the lib dir might not exist.
	 */
	if( !mk_fs_isDir(libdir) ) {
		mk_log_errorMsg(mk_com_va("^F%s^&", libdir));
		return;
	}
#endif

	mk_sl_pushBack( mk__g_libdirs, libdir );
}
void mk_front_pushPkgDir( const char *pkgdir ) {
	if( !mk_fs_isDir( pkgdir ) ) {
		mk_log_errorMsg( mk_com_va( "^F%s^&", pkgdir ) );
		return;
	}

	mk_sl_pushBack( mk__g_pkgdirs, pkgdir );
}
void mk_front_pushToolDir( const char *tooldir ) {
	if( !mk_fs_isDir( tooldir ) ) {
		mk_log_errorMsg( mk_com_va( "^F%s^&", tooldir ) );
		return;
	}

	mk_sl_pushBack( mk__g_tooldirs, tooldir );
}
void mk_front_pushDynamicLibsDir( const char *dllsdir ) {
	if( !mk_fs_isDir( dllsdir ) ) {
		mk_log_errorMsg( mk_com_va( "^F%s^&", dllsdir ) );
		return;
	}

	mk_sl_pushBack( mk__g_dllsdirs, dllsdir );
}

void mk_fs_unwindDirs( void ) {
	while( mk_sl_getSize( mk__g_fs_dirstack ) > 0 ) {
		mk_fs_leave();
	}
}

static int strEqAnyUntilNull( const char *a, ... ) {
	const char *b;
	const char *adash;
	va_list args;
	int r;

	adash = a;
	while( *adash == '-' ) {
		++adash;
	}

	if( *adash == '\0' ) {
		return 0;
	}

	va_start( args, a );
	r = 0;
	while( ( b = va_arg( args, const char * ) ) != (const char *)0 ) {
		const char *bdash;

		bdash = b;
		while( *bdash == '-' ) {
			++bdash;
		}

		if( strcmp( a, b ) == 0 ) {
			r = 1;
			break;
		}

		if( bdash != b && strcmp( adash, bdash ) == 0 ) {
			r = 1;
			break;
		}
	}
	va_end( args );

	return r;
}

static int isAction( MkAction *act, const char *s ) {
	MkActionType_t *p_ty;
	MkActionType_t dummyty;

	MK_ASSERT( s != (const char *)0 );

	if( act != (MkAction *)0 ) {
		p_ty = &act->type;
	} else {
		p_ty = &dummyty;
	}

	if( strEqAnyUntilNull( s, /*"help",*/ "-h", "--help", NULL ) ) {
		*p_ty = kMkAction_ShowHelp;
		return 1;
	}

	if( strEqAnyUntilNull( s, /*"version",*/ "-v", "--version", NULL ) ) {
		*p_ty = kMkAction_ShowVersion;
		return 1;
	}

	if( strEqAnyUntilNull( s, "build", NULL ) ) {
		*p_ty = kMkAction_BuildTarget;
		return 1;
	}

	if( strEqAnyUntilNull( s, /*"clean",*/ "-C", "--clean", NULL ) ) {
		*p_ty = kMkAction_CleanTarget;
		return 1;
	}

	if( strEqAnyUntilNull( s, /*"test",*/ "-T", "--test", NULL ) ) {
		*p_ty = kMkAction_TestTarget;
		return 1;
	}

	return 0;
}
static int actionExpectsTarget( MkActionType_t ty ) {
	switch( ty ) {
	case kMkAction_BuildTarget:
	case kMkAction_CleanTarget:
	case kMkAction_TestTarget:
		return 1;

	default:
		break;
	}

	return 0;
}

static int parseAction( MkAction *act, int *p_i, int argc, const char **argv ) {
	int i, i_begin;
	int acceptsTargets;
	int expectingTarget;
	int endOfAction;

	MK_ASSERT( act != (MkAction *)0 );
	MK_ASSERT( argv != (const char **)0 );
	MK_ASSERT( p_i != (int *)0 );

	i = *p_i;
	MK_ASSERT( i <= argc );
	MK_ASSERT( i >= 0 );

	if( i == argc ) {
		return 0;
	}

	if( !isAction( act, argv[i] ) ) {
		return 0;
	}

	expectingTarget = actionExpectsTarget( act->type );
	acceptsTargets  = expectingTarget;

	i_begin     = i++;
	endOfAction = 0;
	while( i < argc ) {
		if( expectingTarget ) {
			++i;
			expectingTarget = 0;
			continue;
		}

		if( acceptsTargets && strEqAnyUntilNull( argv[i], "--target", "--config", NULL ) ) {
			++i;
			expectingTarget = 1;
			continue;
		}

		if( strcmp( argv[i], "--end-action" ) == 0 ) {
			endOfAction = 1;
			break;
		}

		if( isAction( (MkAction *)0, argv[i] ) ) {
			break;
		}

		++i;
	}

	act->argc =    i - i_begin;
	act->argv = argv + i_begin;

	if( endOfAction ) {
		++i;
	}

	*p_i = i;

	return 1;
}

typedef enum {
	kCheckForAction_No,
	kCheckForAction_Yes
} checkForAction_t;
typedef enum {
	kAcceptTargets_No,
	kAcceptTargets_Yes
} acceptTargets_t;
static void processSharedArguments( int argc, const char **argv, checkForAction_t check, acceptTargets_t acceptTargets ) {
	static const char *optlinks[256];
	static int didinitoptlinks = 0;

	const char *p, *arg;
	char temp[PATH_MAX];
	int i, op;
	int acceptingTargets;

	if( !didinitoptlinks ) {
		didinitoptlinks = 1;

		memset( (void *)optlinks, 0, sizeof( optlinks ) );

		optlinks['V'] = "verbose";
		optlinks['r'] = "release";
		optlinks['H'] = "print-hierarchy";
		optlinks['p'] = "pedantic";
		optlinks['D'] = "dir";
	}

	acceptingTargets = +( acceptTargets == kAcceptTargets_Yes );

	for( i = 1; i < argc; i++ ) {
#define REMOVE_ARG() do { arg = argv[i]; argv[i] = (const char *)0; } while(0)
		const char *opt;

		opt = argv[i];
		temp[0] = '\0';

		if( check == kCheckForAction_Yes ) {
			MkAction act;

			if( parseAction( &act, &i, argc, argv ) ) {
				if( act.type == kMkAction_ShowHelp ) {
					mk__g_flags |= kMkFlag_ShowHelp_Bit;
				} else if( act.type == kMkAction_ShowVersion ) {
					mk__g_flags |= kMkFlag_ShowVersion_Bit;
				}
				mk_arr_append( mk__g_actions, act );

				--i;
				continue;
			}
		}

		if( *opt == '-' ) {
			op = 0;
			p  = (const char *)0;

			if( *( opt + 1 ) == '-' ) {
				opt = &opt[2];

				if( ( op = !strncmp( opt, "no-", 3 ) ? 1 : 0 ) == 1 ) {
					opt = &opt[3];
				}

				if( ( p = strchr( opt, '=' ) ) != (const char *)0 ) {
					mk_com_strncpy( temp, sizeof( temp ), opt, p - opt );
					p++;
					opt = temp;
				}
			} else {
				if( *( opt + 1 ) == 'I' ) {
					REMOVE_ARG();
					if( *( opt + 2 ) == 0 ) {
						p = argv[++i];
					} else {
						p = &opt[2];
					}

					opt = "incdir";
				} else if( *( opt + 1 ) == 'L' ) {
					REMOVE_ARG();
					if( *( opt + 2 ) == 0 ) {
						p = argv[++i];
					} else {
						p = &opt[2];
					}

					opt = "libdir";
				} else if( *( opt + 1 ) == 'S' ) {
					REMOVE_ARG();
					if( *( opt + 2 ) == 0 ) {
						p = argv[++i];
					} else {
						p = &opt[2];
					}

					opt = "srcdir";
				} else if( *( opt + 1 ) == 'P' ) {
					REMOVE_ARG();
					if( *( opt + 2 ) == 0 ) {
						p = argv[++i];
					} else {
						p = &opt[2];
					}
				} else {
					/* not allowing repeats (e.g., -hv) yet */
					if( *( opt + 2 ) != 0 ) {
						mk_log_errorMsg( mk_com_va( "^E'%s'^& is a malformed argument",
						    argv[i] ) );
						continue;
					}

					if( !( opt = optlinks[(unsigned char)*( opt + 1 )] ) ) {
						mk_log_errorMsg( mk_com_va( "unknown option ^E'%s'^&; ignoring",
						    argv[i] ) );
						continue;
					}
				}
			}

			MK_ASSERT( opt != (const char *)0 );

#define PROCESS_BIT(b) \
	REMOVE_ARG();\
	\
	if( op ) {\
		mk__g_flags &= ~(b);\
	} else {\
		mk__g_flags |= (b);\
	}\
	\
	continue

			if( !strcmp( opt, "verbose" ) ) {
				PROCESS_BIT(kMkFlag_Verbose_Bit);
			}

			if( !strcmp( opt, "release" ) ) {
				PROCESS_BIT(kMkFlag_Release_Bit);
			}

			if( !strcmp( opt, "print-hierarchy" ) ) {
				PROCESS_BIT(kMkFlag_PrintHierarchy_Bit);
			}

			if( !strcmp( opt, "pedantic" ) ) {
				PROCESS_BIT(kMkFlag_Pedantic_Bit);
			}

			if( !strcmp( opt, "color" ) ) {
				REMOVE_ARG();
				if( op ) {
					mk__g_flags_color = kMkColorMode_None;
				} else {
					mk__g_flags_color = MK__DEFAULT_COLOR_MODE_IMPL;
				}
				continue;
			}

			if( !strcmp( opt, "ansi-colors" ) ) {
				REMOVE_ARG();
				if( op ) {
					mk__g_flags_color = kMkColorMode_None;
				} else {
					mk__g_flags_color = kMkColorMode_ANSI;
				}
				continue;
			}

			if( !strcmp( opt, "win32-colors" ) ) {
				REMOVE_ARG();
#if MK_WINDOWS_COLORS_ENABLED
				if( op ) {
					mk__g_flags_color = kMkColorMode_None;
				} else {
					mk__g_flags_color = kMkColorMode_Windows;
				}
#else
				mk_log_errorMsg( "option \"--[no-]win32-colors\" is disabled in this build" );
#endif
				continue;
			}

#define PROCESS_DIR_ARG() \
	REMOVE_ARG();\
	\
	if( i + 1 == argc && !p ) {\
		mk_log_errorMsg( mk_com_va( "expected argument to ^E'%s'^&", arg ) );\
		continue;\
	}\
	\
	((void)0)

			if( acceptingTargets ) {
				if( !strcmp( opt, "srcdir" ) ) {
					PROCESS_DIR_ARG();
					mk_front_pushSrcDir( p ? p : argv[++i] );
					continue;
				}

				if( !strcmp( opt, "incdir" ) ) {
					PROCESS_DIR_ARG();
					mk_front_pushIncDir( p ? p : argv[++i] );
					continue;
				}

				if( !strcmp( opt, "libdir" ) ) {
					PROCESS_DIR_ARG();
					mk_front_pushLibDir( p ? p : argv[++i] );
					continue;
				}

				if( !strcmp( opt, "pkgdir" ) ) {
					PROCESS_DIR_ARG();
					mk_front_pushPkgDir( p ? p : argv[++i] );
					continue;
				}

				if( !strcmp( opt, "toolsdir" ) ) {
					PROCESS_DIR_ARG();
					mk_front_pushToolDir( p ? p : argv[++i] );
					continue;
				}

				if( !strcmp( opt, "dllsdir" ) ) {
					PROCESS_DIR_ARG();
					mk_front_pushDynamicLibsDir( p ? p : argv[++i] );
					continue;
				}

				if( !strcmp( opt, "target" ) ) {
					char *q;

					REMOVE_ARG();

					if( temp[0] == '\0' ) {
						if( i + 1 == argc ) {
							mk_log_errorMsg( mk_com_va( "expecting argument to ^E'%s'^&", arg ) );
							continue;
						}

						++i;
						mk_com_strcpy( temp, sizeof(temp), argv[ i ] );
					}

					if( ( q = strchr( temp, ':' ) ) != (const char *)0 ) {
						*q = '\0';
						mk_sl_pushBack( mk__g_targets, temp );
						*q = ':';

						mk_com_strcpy( temp, sizeof(temp), q + 1 );
						/* FIXME: How to specify a configuration for a given target */
						mk_log_errorMsg( mk_com_va( "explicit configuration option not yet supported: %s", temp ) );
					} else {
						mk_sl_pushBack( mk__g_targets, opt );
					}

					continue;
				}

				if( !strcmp( opt, "config" ) ) {
					REMOVE_ARG();

					if( temp[0] == '\0' ) {
						if( i + 1 == argc ) {
							mk_log_errorMsg( mk_com_va( "expecting argument to ^E'%s'^&", arg ) );
							continue;
						}

						++i;
						mk_com_strcpy( temp, sizeof(temp), argv[ i ] );
					}

					/* FIXME: How to specify a configuration for a given target */
					mk_log_errorMsg( mk_com_va( "explicit configuration option not yet supported: %s", temp ) );
					continue;
				}
			} /* if acceptingTargets */

			if( !strcmp( opt, "dir" ) ) {
				PROCESS_DIR_ARG();
				mk_fs_enter( p ? p : argv[++i] );
				continue;
			}
#undef PROCESS_DIR_ARG
		} else /* opt[0] == '-' */ if( acceptingTargets ) {
			REMOVE_ARG();

			if( ( p = strchr( opt, ':' ) ) != (const char *)0 ) {
				mk_com_strncpy( temp, sizeof(temp), opt, (size_t)(ptrdiff_t)( p - opt ) );
				mk_sl_pushBack( mk__g_targets, temp );

				mk_com_strcpy( temp, sizeof(temp), p + 1 );
				/* FIXME: How to specify a configuration for a given target */
				mk_log_errorMsg( mk_com_va( "explicit configuration option not yet supported: %s", temp ) );
			} else {
				mk_sl_pushBack( mk__g_targets, opt );
			}
		}
	}
}

static void showHelp(void) {
	static int didshow = 0;

	if( didshow ) {
		return;
	}

	didshow = 1;

	printf( "Usage: mk [options...] [targets...]\n" );
	printf( "Options:\n" );
	printf( "  -h,--help                Show this help message.\n" );
	printf( "  -v,--version             Show the version.\n" );
	printf( "  -V,--verbose             Show the commands invoked.\n" );
	printf( "  -b,--brush               Remove intermediate files after "
			"building.\n" );
	printf( "  -C,--clean               Remove \"%s\"\n", mk_opt_getObjdirBase() );
	printf( "  -r,--release             Build in release mode.\n" );
	printf( "  -R,--rebuild             "
			"Force a rebuild, without cleaning.\n" );
	printf( "  -T,--test                Run unit tests.\n" );
	printf( "  -c,--compile-only        Just compile; do not link.\n" );
	printf( "  -p,--pedantic            Enable pedantic warnings.\n" );
	printf( "  --[no-]pthread           Enable -pthread compiler flag [default].\n" );
	printf( "  -H,--print-hierarchy     Display the project hierarchy.\n" );
	printf( "  -S,--srcdir=<dir>        Add a source directory.\n" );
	printf( "  -I,--incdir=<dir>        Add an include directory.\n" );
	printf( "  -L,--libdir=<dir>        Add a library directory.\n" );
	printf( "  -P,--pkgdir=<dir>        Add a package directory.\n" );
	printf( "  --[no-]color             Enable or disable colored output.\n" );
	printf( "    --ansi-colors          Enable ANSI-based coloring.\n" );
#if MK_WINDOWS_COLORS_ENABLED
	printf( "    --win32-colors         Enable Windows-based coloring.\n" );
#endif
	printf( "  --[no-]builtin-autolinks Enable built-in autolinks (default).\n" );
	printf( "  --[no-]user-autolinks    Enable loading of mk-autolinks.txt (default).\n" );
	printf( "\n" );
	printf( "See the documentation (or source code) for more details.\n" );
}

static void showVersion(void) {
	static int didshow = 0;

	if( didshow ) {
		return;
	}

	didshow = 1;

	printf(
		"mk " MK_VERSION_STR " - compiled %s\nCopyright (c) 2012-2021 NotKyon\n\n"
		"This software contains ABSOLUTELY NO WARRANTY.\n\n",
		__DATE__ );
}

static void processActions(void) {
	size_t i;

	mk_arr_for(mk__g_actions, i) {
		MkAction *act;

		act = &mk_arr_at( mk__g_actions, i );

		switch( act->type ) {
		case kMkAction_ShowHelp:
			/* FIXME: Check for specific items to give extra help on */
			showHelp();
			break;

		case kMkAction_ShowVersion:
			showVersion();
			break;

		case kMkAction_BuildTarget:
			processSharedArguments( act->argc, act->argv, kCheckForAction_No, kAcceptTargets_Yes );
			break;

		case kMkAction_CleanTarget:
			processSharedArguments( act->argc, act->argv, kCheckForAction_No, kAcceptTargets_Yes );
			break;

		case kMkAction_TestTarget:
			processSharedArguments( act->argc, act->argv, kCheckForAction_No, kAcceptTargets_Yes );
			break;
		}
	}
}

void mk_main_init( int argc, char **argv ) {
	MkAutolink al;
	size_t j;
	MkLib lib;
	int builtinautolinks = 1, userautolinks = 1;

	/* core initialization */
	mk_arr_init( mk__g_actions );
	atexit( mk_main_fini );
	mk_sys_initColoredOutput();
	mk_fs_init();
	atexit( mk_fs_unwindDirs );
	atexit( mk_al_deleteAll );
	atexit( mk_dep_deleteAll );
	atexit( mk_prj_deleteAll );
	mk_bld_initUnitTestArrays();

	/* string-lists need to be initialized */
	mk__g_targets  = mk_sl_new();
	mk__g_srcdirs  = mk_sl_new();
	mk__g_incdirs  = mk_sl_new();
	mk__g_libdirs  = mk_sl_new();
	mk__g_pkgdirs  = mk_sl_new();
	mk__g_tooldirs = mk_sl_new();
	mk__g_dllsdirs = mk_sl_new();

	/* process command line arguments */
	processSharedArguments( argc, (const char **)argv, kCheckForAction_Yes, kAcceptTargets_No );

	/* support "clean rebuilds" */
	if( ( mk__g_flags & kMkFlag_Rebuild_Bit ) && ( mk__g_flags & ( kMkFlag_LightClean_Bit | kMkFlag_FullClean_Bit ) ) ) {
		mk__g_flags &= ~( kMkFlag_NoCompile_Bit | kMkFlag_NoLink_Bit );
	}

	processActions();

	/* exit if no targets were specified and a message was requested */
	if( mk__g_flags & ( kMkFlag_ShowVersion_Bit | kMkFlag_ShowHelp_Bit ) && !mk_sl_getSize( mk__g_targets ) ) {
		exit( EXIT_SUCCESS );
	}

	/* initialize the global directories */
	mk_fs_makeDirs( mk_opt_getGlobalSharedDir() );
	mk_fs_makeDirs( mk_opt_getGlobalVersionDir() );

	/* add builtin autolinks / libraries */
	if( builtinautolinks ) {
		/* add the autolinks */
		for( j = 0; j < sizeof( autolinks ) / sizeof( autolinks[0] ); j++ ) {
			al = mk_al_new();

			mk_al_setLib( al, autolinks[j].lib );

			mk_al_setHeader( al, kMkOS_MSWin, autolinks[j].header[0] );
			mk_al_setHeader( al, kMkOS_UWP, autolinks[j].header[0] );
			mk_al_setHeader( al, kMkOS_Cygwin, autolinks[j].header[1] );
			mk_al_setHeader( al, kMkOS_Linux, autolinks[j].header[1] );
			mk_al_setHeader( al, kMkOS_MacOSX, autolinks[j].header[2] );
			mk_al_setHeader( al, kMkOS_Unix, autolinks[j].header[1] );
		}

		/* add the libraries */
		for( j = 0; j < sizeof( libs ) / sizeof( libs[0] ); j++ ) {
			lib = mk_lib_new();

			mk_lib_setName( lib, libs[j].lib );

			mk_lib_setFlags( lib, kMkOS_MSWin, libs[j].flags[0] );
			mk_lib_setFlags( lib, kMkOS_UWP, libs[j].flags[0] );
			mk_lib_setFlags( lib, kMkOS_Cygwin, libs[j].flags[1] );
			mk_lib_setFlags( lib, kMkOS_Linux, libs[j].flags[1] );
			mk_lib_setFlags( lib, kMkOS_MacOSX, libs[j].flags[2] );
			mk_lib_setFlags( lib, kMkOS_Unix, libs[j].flags[1] );
		}
	}

	/* load user autolinks / libraries */
	if( userautolinks ) {
		char szFile[PATH_MAX];

		/* load from .mk/share/mk-autolinks.txt first */
		mk_com_strcpy( szFile, sizeof( szFile ), mk_opt_getGlobalSharedDir() );
		MK_ASSERT( mk_com_strends( szFile, "/" ) && "mk_opt_getGlobalSharedDir() must return path with a trailing '/'" );
		mk_com_strcat( szFile, sizeof( szFile ), "mk-autolinks.txt" );
		(void)mk_al_loadConfig( szFile );

		/* load from the current directory */
		(void)mk_al_loadConfig( "mk-autolinks.txt" );
	}

	/* grab the available directories */
	mk_prjfs_findRootDirs( mk__g_srcdirs, mk__g_incdirs, mk__g_libdirs, mk__g_pkgdirs, mk__g_tooldirs,
	    mk__g_dllsdirs );

	/* if there aren't any source directories, complain */
	if( !mk_sl_getSize( mk__g_srcdirs ) && !mk_sl_getSize( mk__g_pkgdirs ) ) {
		mk_log_fatalError( "no source ('src' or 'source') or package ('pkg') directories" );
	}
}

void mk_main_fini( void ) {
	mk_sl_deleteAll();
	mk_arr_fini( mk__g_actions );
}
