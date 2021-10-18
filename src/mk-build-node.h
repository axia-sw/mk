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
#pragma once

/*
 *	========================================================================
 *	BUILD CONTEXT AND DAG/NODES
 *	========================================================================
 *
 *	Nodes
 *	-----
 *	A build node is a data structure/collection for processing different
 *	build tasks. The structure is separated into a set of jobs that one or
 *	more processing threads can execute simultaneously.
 *
 *	Build nodes track inputs/reads and outputs/writes. Nodes must wait for
 *	their inputs to finish processing before their work can be executed.
 *	Once a build node finishes executing, it updates its outputs, which are
 *	just the set of build nodes with *that* node as one of their inputs.
 *
 *	Build nodes are arranged in a directed-acyclic-graph (DAG). It's easy to
 *	follow the flow of the graph, and situations such as infinite recursion
 *	are caught and avoided (hence acyclic—as there are no cycles).
 *
 *	For a structural example, suppose there's a project with the following
 *	source structure:
 *
 *		- main.c
 *		- aux1.c
 *		- aux2.c
 *
 *	Supposing that each .c file yields a .o (object) file as well as a
 *	.d (dependency) file, and that the ultimate binary requires the .o files
 *	of each source file, then the following build nodes can be assumed:
 *
 *		- main.c -> main.o, main.d
 *		- aux1.c -> aux1.o, aux1.d
 *		- aux2.c -> aux2.o, aux2.d
 *
 *		- main.o, aux1.o, aux2.o -> app.exe
 *
 *	Thus, to build app.exe the files main.o, aux1.o, and aux2.o are needed.
 *	To build main.o, aux1.o, and aux2.o, the files main.c, aux1.c, and
 *	aux2.c must be processed. Build nodes allow this information to be
 *	expressed elegantly.
 *
 *	Moreover, the .d files have no ultimate target in this example. Mk could
 *	easily remove them and other build artifacts if the user requested it.
 *	In practice, Mk would want to use these .d files for processing more
 *	dependency information (in an effort to avoid full rebuilds on each
 *	invocation).
 *
 *
 *	Context
 *	-------
 *	A build context contains a collection of build nodes that serve as the
 *	ultimate targets that the user requested be generated.
 *
 *	Additionally, the build context contains supplemental data structures
 *	for managing and processing these build nodes.
 */

#include "mk-basic-async.h"

struct MkStrList_s;

typedef struct MkBuildContext_s *MkBuildContext;
typedef struct MkBuildNode_s *MkBuildNode;

typedef int(*mk_build_func_t)(MkBuildNode, void*, struct MkStrList_s *inputs, struct MkStrList_s *outputs);
typedef int(*mk_checkDeps_func_t)(MkBuildNode, void*, struct MkStrList_s *inputs);

enum {
	kMkBldNo_Phony_Bit       = 1UL << 0,
	kMkBldNo_Target_Bit      = 1UL << 1,
	kMkBldNo_Dependency_Bit  = 1UL << 2,
	kMkBldNo_Built_Bit       = 1UL << 3,
	kMkBldNo_Failed_Bit      = 1UL << 4,
	kMkBldNo_Unbuildable_Bit = 1UL << 5,

	kMkBldNo_Processed_Bits  = kMkBldNo_Built_Bit  | kMkBldNo_Failed_Bit,
	kMkBldNo_Canceled_Bits   = kMkBldNo_Failed_Bit | kMkBldNo_Unbuildable_Bit
};
