/* val.h: evaluate a VAL or VAL$ call
   Copyright (c) 2003-2004 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef BASIC_VAL_H
#define BASIC_VAL_H

#ifndef BASIC_PROGRAM_H
#include "program.h"
#endif				/* #ifndef BASIC_PROGRAM_H */

#ifndef BASIC_STRING_H
#include "spectrum-string.h"
#endif				/* #ifndef BASIC_STRING_H */

error_t
val_eval( struct program *program, struct string *string, float *value );
error_t vals_eval( struct program *program, struct string *string1,
		   struct string **string );

#endif				/* #ifndef BASIC_VAL_H */
