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

MkStrList mk__g_targets  = (MkStrList)0;
MkStrList mk__g_srcdirs  = (MkStrList)0;
MkStrList mk__g_incdirs  = (MkStrList)0;
MkStrList mk__g_libdirs  = (MkStrList)0;
MkStrList mk__g_pkgdirs  = (MkStrList)0;
MkStrList mk__g_tooldirs = (MkStrList)0;
MkStrList mk__g_dllsdirs = (MkStrList)0;

bitfield_t mk__g_flags          = 0;
MkColorMode_t mk__g_flags_color = MK__DEFAULT_COLOR_MODE_IMPL;

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

void mk_main_init( int argc, char **argv ) {
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
		{ { "GL/glew.h", "GL/glew.h", "GL/glew.h" },
		    "glew" },

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

		/* Ogg/Vorbis */
		{ { "ogg/ogg.h", "ogg/ogg.h", "ogg/ogg.h" },
		    "ogg" },
		{ { "vorbis/codec.h", "vorbis/codec.h", "vorbis/codec.h" },
		    "vorbis" },
		{ { "vorbis/vorbisenc.h", "vorbis/vorbisenc.h", "vorbis/vorbisenc.h" },
		    "vorbisenc" },
		{ { "vorbis/vorbisfile.h", "vorbis/vorbisfile.h", "vorbis/vorbisfile.h" },
		    "vorbisfile" },

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
		{ "glew",
		    { "-lglew32", "-lGLEW", "-lGLEW" } },

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

		/* Ogg/Vorbis */
		{ "ogg",
		    { "-logg", "-logg", "-logg" } },
		{ "vorbis",
		    { "-lvorbis", "-lvorbis", "-lvorbis" } },
		{ "vorbisenc",
		    { "-lvorbisenc", "-lvorbisenc", "-lvorbisenc" } },
		{ "vorbisfile",
		    { "-lvorbisfile", "-lvorbisfile", "-lvorbisfile" } },

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
		    { (const char *)0, "-lrt", "-lrt" } },
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
	const char *optlinks[256], *p;
	bitfield_t bit;
	MkAutolink al;
	size_t j;
	MkLib lib;
	char temp[PATH_MAX];
	int builtinautolinks = 1, userautolinks = 1;
	int i, op;

	/* core initialization */
	mk_sys_initColoredOutput();
	atexit( mk_sl_deleteAll );
	mk_fs_init();
	atexit( mk_fs_unwindDirs );
	atexit( mk_al_deleteAll );
	atexit( mk_dep_deleteAll );
	atexit( mk_prj_deleteAll );
	mk_bld_initUnitTestArrays();

	/* set single-character options here */
	memset( (void *)optlinks, 0, sizeof( optlinks ) );
	optlinks['h'] = "help";
	optlinks['v'] = "version";
	optlinks['V'] = "verbose";
	optlinks['r'] = "release";
	optlinks['R'] = "rebuild";
	optlinks['c'] = "compile-only";
	optlinks['b'] = "brush";
	optlinks['C'] = "clean";
	optlinks['H'] = "print-hierarchy";
	optlinks['p'] = "pedantic";
	optlinks['T'] = "test";
	optlinks['D'] = "dir";

	/* arrays need to be initialized */
	mk__g_targets  = mk_sl_new();
	mk__g_srcdirs  = mk_sl_new();
	mk__g_incdirs  = mk_sl_new();
	mk__g_libdirs  = mk_sl_new();
	mk__g_pkgdirs  = mk_sl_new();
	mk__g_tooldirs = mk_sl_new();
	mk__g_dllsdirs = mk_sl_new();

	/* process command line arguments */
	for( i = 1; i < argc; i++ ) {
		const char *opt;

		opt = argv[i];
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
					if( *( opt + 2 ) == 0 ) {
						p = argv[++i];
					} else {
						p = &opt[2];
					}

					opt = "incdir";
				} else if( *( opt + 1 ) == 'L' ) {
					if( *( opt + 2 ) == 0 ) {
						p = argv[++i];
					} else {
						p = &opt[2];
					}

					opt = "libdir";
				} else if( *( opt + 1 ) == 'S' ) {
					if( *( opt + 2 ) == 0 ) {
						p = argv[++i];
					} else {
						p = &opt[2];
					}

					opt = "srcdir";
				} else if( *( opt + 1 ) == 'P' ) {
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

			bit = 0;
			if( !strcmp( opt, "help" ) ) {
				bit = kMkFlag_ShowHelp_Bit;
			} else if( !strcmp( opt, "version" ) ) {
				bit = kMkFlag_ShowVersion_Bit;
			} else if( !strcmp( opt, "verbose" ) ) {
				bit = kMkFlag_Verbose_Bit;
			} else if( !strcmp( opt, "release" ) ) {
				bit = kMkFlag_Release_Bit;
			} else if( !strcmp( opt, "rebuild" ) ) {
				bit = kMkFlag_Rebuild_Bit;
			} else if( !strcmp( opt, "compile-only" ) ) {
				bit = kMkFlag_NoLink_Bit;
			} else if( !strcmp( opt, "brush" ) ) {
				bit = kMkFlag_NoCompile_Bit | kMkFlag_NoLink_Bit | kMkFlag_LightClean_Bit;
			} else if( !strcmp( opt, "clean" ) ) {
				bit = kMkFlag_NoCompile_Bit | kMkFlag_NoLink_Bit | kMkFlag_FullClean_Bit;
			} else if( !strcmp( opt, "print-hierarchy" ) ) {
				bit = kMkFlag_PrintHierarchy_Bit;
			} else if( !strcmp( opt, "pedantic" ) ) {
				bit = kMkFlag_Pedantic_Bit;
			} else if( !strcmp( opt, "test" ) ) {
				bit = kMkFlag_Test_Bit;
			} else if( !strcmp( opt, "pthread" ) ) {
				bit = kMkFlag_OutSingleThread_Bit;
				op  = !op;
			} else if( !strcmp( opt, "color" ) ) {
				if( op ) {
					mk__g_flags_color = kMkColorMode_None;
				} else {
					mk__g_flags_color = MK__DEFAULT_COLOR_MODE_IMPL;
				}
				continue;
			} else if( !strcmp( opt, "ansi-colors" ) ) {
				if( op ) {
					mk__g_flags_color = kMkColorMode_None;
				} else {
					mk__g_flags_color = kMkColorMode_ANSI;
				}
				continue;
			} else if( !strcmp( opt, "win32-colors" ) ) {
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
			} else if( !strcmp( opt, "builtin-autolinks" ) ) {
				builtinautolinks = (int)!op;
				continue;
			} else if( !strcmp( opt, "user-autolinks" ) ) {
				userautolinks = (int)!op;
				continue;
			} else if( !strcmp( opt, "srcdir" ) ) {
				if( i + 1 == argc && !p ) {
					mk_log_errorMsg( mk_com_va( "expected argument to ^E'%s'^&", argv[i] ) );
					continue;
				}

				mk_front_pushSrcDir( p ? p : argv[++i] );
				continue;
			} else if( !strcmp( opt, "incdir" ) ) {
				if( i + 1 == argc && !p ) {
					mk_log_errorMsg( mk_com_va( "expected argument to ^E'%s'^&", argv[i] ) );
					continue;
				}

				mk_front_pushIncDir( p ? p : argv[++i] );
				continue;
			} else if( !strcmp( opt, "libdir" ) ) {
				if( i + 1 == argc && !p ) {
					mk_log_errorMsg( mk_com_va( "expected argument to ^E'%s'^&", argv[i] ) );
					continue;
				}

				mk_front_pushLibDir( p ? p : argv[++i] );
				continue;
			} else if( !strcmp( opt, "pkgdir" ) ) {
				if( i + 1 == argc && !p ) {
					mk_log_errorMsg( mk_com_va( "expected argument to ^E'%s'^&", argv[i] ) );
					continue;
				}

				mk_front_pushPkgDir( p ? p : argv[++i] );
				continue;
			} else if( !strcmp( opt, "toolsdir" ) ) {
				if( i + 1 == argc && !p ) {
					mk_log_errorMsg( mk_com_va( "expected argument to ^E'%s'^&", argv[i] ) );
					continue;
				}

				mk_front_pushToolDir( p ? p : argv[++i] );
				continue;
			} else if( !strcmp( opt, "dllsdir" ) ) {
				if( i + 1 == argc && !p ) {
					mk_log_errorMsg( mk_com_va( "expected argument to ^E'%s'^&", argv[i] ) );
					continue;
				}

				mk_front_pushDynamicLibsDir( p ? p : argv[++i] );
				continue;
			} else if( !strcmp( opt, "dir" ) ) {
				if( i + 1 == argc && !p ) {
					mk_log_errorMsg( mk_com_va( "expected argument to ^E'%s'^&", argv[i] ) );
					continue;
				}

				mk_fs_enter( p ? p : argv[++i] );
				continue;
			} else {
				mk_log_errorMsg( mk_com_va( "unknown option ^E'%s'^&; ignoring", argv[i] ) );
				continue;
			}

			MK_ASSERT( bit != 0 );
			MK_ASSERT( op == 0 || op == 1 );

			if( op ) {
				mk__g_flags &= ~bit;
			} else {
				mk__g_flags |= bit;
			}
		} else {
			mk_sl_pushBack( mk__g_targets, opt );
		}
	}

	/* support "clean rebuilds" */
	if( ( mk__g_flags & kMkFlag_Rebuild_Bit ) && ( mk__g_flags & ( kMkFlag_LightClean_Bit | kMkFlag_FullClean_Bit ) ) ) {
		mk__g_flags &= ~( kMkFlag_NoCompile_Bit | kMkFlag_NoLink_Bit );
	}

	/* show the version */
	if( mk__g_flags & kMkFlag_ShowVersion_Bit ) {
		printf(
		    "mk " MK_VERSION_STR " - compiled %s\nCopyright (c) 2012-2018 NotKyon\n\n"
		    "This software contains ABSOLUTELY NO WARRANTY.\n\n",
		    __DATE__ );
	}

	/* show the help */
	if( mk__g_flags & kMkFlag_ShowHelp_Bit ) {
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
