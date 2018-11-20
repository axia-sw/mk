/*
 *	Mk
 *	Written by NotKyon (Aaron J. Miller) <nocannedmeat@gmail.com>, 2012-2016.
 *	Contributions from James P. O'Hagan <johagan12@gmail.com>, 2012-2016.
 *
 *	This tool is used to simplify the build process of simple interdependent
 *	programs/libraries that need to be (or would benefit from being) distributed
 *	without requiring make/scons/<other builder program>.
 *
 *	The entire program was originally written as one source file to make it
 *	easier to build. However, after around 9,500~ lines we decided to break it
 *	up into multiple source files. It's still fairly easy to build though:
 *	$ gcc -g -D_DEBUG -W -Wall -pedantic -std=c99 -o mk mk*.c
 *
 *	To build this program in release mode, run the following:
 *	$ gcc -O3 -fomit-frame-pointer -W -Wall -pedantic -std=c99 -o mk mk*.c
 *
 *	Also, I apologize for the quality (or rather, the lack thereof) of the code.
 *	I intend to improve it over time, make some function names more consistent,
 *	and provide better internal source code documentation.
 *
 *	I did not attempt to compile with VC++. I do not believe "%zu" works in VC,
 *	so you may have to modify as necessary. Under GCC, I get absolutely no
 *	warnings for ISO C99; no extensions used.
 *
 *		-NotKyon
 *
 *	LICENSE
 *	=======
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

#include "mk-basic-stringList.h"
#include "mk-build-engine.h"
#include "mk-build-project.h"
#include "mk-frontend.h"

#include <stdio.h>
#include <stdlib.h>

int main( int argc, char **argv ) {
	mk_main_init( argc, argv );

	if( mk__g_flags & kMkFlag_PrintHierarchy_Bit ) {
		printf( "Source Directories:\n" );
		mk_sl_print( mk__g_srcdirs );

		printf( "Include Directories:\n" );
		mk_sl_print( mk__g_incdirs );

		printf( "Library Directories:\n" );
		mk_sl_print( mk__g_libdirs );

		printf( "Targets:\n" );
		mk_sl_print( mk__g_targets );

		printf( "Projects:\n" );
		mk_prj_printAll( (MkProject)0, "  " );
		printf( "\n" );
		fflush( stdout );
	}

	if( !mk_bld_makeAllProjects() ) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
