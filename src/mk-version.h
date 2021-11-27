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

#include "mk-defs-common.h"

#define MK_VERSION_MAJOR 0
#define MK_VERSION_MINOR 7
#define MK_VERSION_PATCH 6

#define MK_VERSION ( MK_VERSION_MAJOR * 10000 + MK_VERSION_MINOR * 100 + MK_VERSION_PATCH )

#define MK_VERSION_STR MK_PPTOKSTR( MK_VERSION_MAJOR ) \
"." MK_PPTOKSTR( MK_VERSION_MINOR ) "." MK_PPTOKSTR( MK_VERSION_PATCH )
