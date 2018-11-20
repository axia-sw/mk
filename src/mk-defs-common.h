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
================
MK_PPTOKSTR(X_)

Turns the given token (X_) into a string.

e.g.,

	```
	#define SOMEVALUE_INT 7
	#define SOMEVALUE_STR MK_VERSION_STR(SOMEVALUE_INT)
	
	...
	
	printf( "%s\n", SOMEVALUE_STR ); // "7"
	```

This is used by MK_VERSION_STR.
================
*/
#define MK_PPTOKSTR__IMPL( X_ ) #X_
#define MK_PPTOKSTR( X_ ) MK_PPTOKSTR__IMPL( X_ )

/*
================
MK_PPTOKCAT(X,Y)

Concatenates two tokens together using the preprocessor pasting operator.

e.g.,

	```
	#define PREFIX myPrefix_
	
	// defines function "myPrefix_someFunc"
	void MK_PPTOKCAT(PREFIX,someFunc)(void) {}
	```

This can be used to create somewhat unique identifiers by combining with
__COUNTER__ or __LINE__. (Useful for "anonymous" variables in macros.)
================
*/
#define MK_PPTOKCAT__IMPL( X, Y ) X##Y
#define MK_PPTOKCAT( X, Y ) MK_PPTOKCAT__IMPL( X, Y )
