/* val.c: evaluate a VAL or VAL$ call
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

#include <config.h>

#include "numexp.h"
#include "parse.h"
#include "program.h"
#include "string.h"

error_t
val_eval( struct program *program, struct string *string, float *value )
{
  error_t error;
  struct numexp *expression;

  expression = parse_numexp( string->buffer, string->length );
  if( !expression ) {
    /* Parsing failed => C Nonsense in BASIC */
    program->error = BASIC_PROGRAM_ERROR_C;
    return BASIC_ERROR_NONE;
  }

  error = numexp_eval( program, expression, value );
  if( error ) {
    numexp_free( expression );
    return error;
  }

  numexp_free( expression );

  return BASIC_ERROR_NONE;
}

error_t
vals_eval( struct program *program, struct string *string1,
	   struct string **string )
{
  error_t error;
  struct strexp *expression;

  expression = parse_strexp( string1->buffer, string1->length );
  if( !expression ) {
    /* Parsing failed => C Nonsense in BASIC */
    program->error = BASIC_PROGRAM_ERROR_C;
    return BASIC_ERROR_NONE;
  }

  error = strexp_eval( program, expression, string );
  if( error ) {
    strexp_free( expression );
    return error;
  }

  strexp_free( expression );

  return BASIC_ERROR_NONE;
}
