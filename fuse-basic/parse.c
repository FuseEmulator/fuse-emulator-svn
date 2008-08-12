/* parse.c: routines to parse a buffer to the specified type
   Copyright (c) 2004 Philip Kendall

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

/* The buffer we're parsing from */
static const char *parse_buffer;

/* The number of bytes left in parse_buffer */
static size_t parse_buffer_length;

/* What we want to parse to */
enum parse_target_type parse_target;

/* The various things we can parse to */
struct program *parse_program_target;
struct numexp *parse_numexp_target;
struct strexp *parse_strexp_target;

/* Need to declare these somewhere */
extern int yydebug;

int yyparse( void );

struct program *
parse_program( const char *buffer, size_t length )
{
  parse_target = PARSE_PROGRAM;
  parse_buffer = buffer;
  parse_buffer_length = length;

  return yyparse() ? NULL : parse_program_target;
}

struct numexp *
parse_numexp( const char *buffer, size_t length )
{
  parse_target = PARSE_NUMEXP;
  parse_buffer = buffer;
  parse_buffer_length = length;

  return yyparse() ? NULL : parse_numexp_target;
}
  
struct strexp *
parse_strexp( const char *buffer, size_t length )
{
  parse_target = PARSE_STREXP;
  parse_buffer = buffer;
  parse_buffer_length = length;

  return yyparse() ? NULL : parse_strexp_target;
}
  
/* Called to get up to 'max_size' bytes of the buffer */
int
parse_fill_buffer( char *buf, int *result, int max_size )
{
  size_t length;

  if( !parse_buffer_length ) return 0;

  length = parse_buffer_length < max_size ? parse_buffer_length : max_size;

  memcpy( buf, parse_buffer, length );
  *result = length; parse_buffer += length; parse_buffer_length -= length;
  
  return 1;
}

/* Really interesting error reporting function */
void
yyerror( char *s )
{
  fprintf( stderr, "%s\n", s );
}
