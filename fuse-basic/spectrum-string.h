/* string.c: ZX Spectrum BASIC strings
   Copyright (C) 2002 Philip Kendall

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

#ifndef BASIC_STRING_H
#define BASIC_STRING_H

#include <stdlib.h>

#ifndef BASIC_BASIC_H
#include "basic.h"
#endif				/* #ifndef BASIC_BASIC_H */

struct string {

  size_t length;
  char *buffer;

};

error_t string_new( struct string **string );
error_t string_create( struct string *string, char *buffer, size_t length );
error_t string_copy( struct string *dest, struct string *src );
error_t string_concat( struct string *dest,
		       struct string *string1, struct string *string2 );
error_t string_generate_printable( struct string *string, char **output );
error_t string_free( struct string *string );
error_t string_destroy( struct string **string );

#endif				/* #ifndef BASIC_STRING_H */
