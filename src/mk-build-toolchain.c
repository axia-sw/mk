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
#include "mk-build-toolchain.h"

#include "mk-basic-assert.h"
#include "mk-basic-common.h"
#include "mk-basic-memory.h"
#include "mk-basic-stringList.h"
#include "mk-basic-variable.h"

#include "mk-build-platform.h"

struct MkTool_s {
	struct MkTool_s *tc_prev, *tc_next;

	/* list of file extensions this tool recognizes */
	MkStrList fileExts;

	/* command to run per file */
	char *perFileCommand;
	/* command to run per project */
	char *perProjCommand;
};

struct MkToolchain_s {
	/* list of all the tools within this toolchain */
	struct {
		struct MkTool_s *head, *tail;
	} tools;

	/* whether this toolchain applies specifically to the targets being built */
	int isDevToolchain;

	/* shortcut to specific compilers */
	struct MkTool_s *tool_c;
	struct MkTool_s *tool_cpp;
};

MkToolchain mk_tc_new() {
	return (MkToolchain)0;
}

void mk_tc_delete( MkToolchain tc ) {
	if( !tc ) {
		return;
	}

	mk_com_memory( (void*)tc, 0 );
}
