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

#include "mk-basic-stringList.h"

typedef struct MkVariable_s *MkVariable;
typedef struct MkVariableSet_s *MkVariableSet;

MkVariableSet mk_vs_new();
void          mk_vs_delete( MkVariableSet vs );
void          mk_vs_clear( MkVariableSet vs );

int  mk_vs_isDescendant( MkVariableSet vs, MkVariableSet ancestor );
void mk_vs_setParent( MkVariableSet vs, MkVariableSet prnt );

MkVariable mk_vs_findVarBySubstr( MkVariableSet vs, const char *s, const char *e );
MkVariable mk_vs_findVar( MkVariableSet vs, const char *name );

MkVariable mk_v_new( MkVariableSet vs, const char *name );
void       mk_v_delete( MkVariable v );
MkStrList  mk_v_values( MkVariable v );

void mk_v_setValueBySubstr( MkVariable v, const char *s, const char *e );
void mk_v_setValueByStr( MkVariable v, const char *s );
void mk_v_setValueByVariable( MkVariable v, MkVariable other );

char *mk_vs_evalSubstr_r( MkVariableSet vs, const char *s, const char *e );
char *mk_vs_eval( MkVariableSet vs, const char *s );
