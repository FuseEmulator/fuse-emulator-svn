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

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "spectrum-string.h"

error_t
string_new( struct string **string )
{
  *string = malloc( sizeof **string );
  if( (*string) == NULL ) return BASIC_ERROR_MEMORY;

  (*string)->buffer = NULL; (*string)->length = 0;

  return BASIC_ERROR_NONE;
}

error_t
string_create( struct string *string, char *buffer, size_t length )
{
  char *ptr, *ptr2;
  size_t double_quotes;

  /* Replace every occurence of `""' in `buffer' with just one `"' in
     the string itself. A single `"' should never appear in the string */
  ptr = buffer; double_quotes = 0;
  while( ptr < buffer + length ) {

    if( *ptr == '"' ) {

      if( *(ptr+1) != '"' ) {
	fprintf( stderr,
		 "string_create: string contains a single '\"' character\n" );
	return BASIC_ERROR_LOGIC;
      } else {
	ptr++;
      }

      double_quotes++;
    }

    ptr++;
  }

  /* Get rid of anything previously stored in this variable */
  string_free( string );

  /* The length of the string is the length of `buffer' minus one for
     every set of double quotes that we found */
  string->length = length - double_quotes;

  if( string->length ) {
    string->buffer = malloc( string->length * sizeof *string->buffer );
    if( string->buffer == NULL ) return BASIC_ERROR_MEMORY;

    ptr = buffer; ptr2 = string->buffer;
    while( ptr < buffer + length ) {
      
      /* Skip the first of any pair of double quotes */
      if( *ptr == '"' ) ptr++;
      
      *ptr2++ = *ptr++;
    }
      
  } else {
    string->buffer = NULL;
  }

  return BASIC_ERROR_NONE;
}

error_t
string_copy( struct string *dest, struct string *src )
{
  string_free( dest );
  dest->length = src->length;

  dest->buffer = malloc( dest->length * sizeof *dest->buffer );
  if( dest->buffer == NULL ) return BASIC_ERROR_MEMORY;
  memcpy( dest->buffer, src->buffer, dest->length );

  return BASIC_ERROR_NONE;
}

error_t
string_concat( struct string *dest,
	       struct string *string1, struct string *string2 )
{
  string_free( dest );
  dest->length = string1->length + string2->length;
  if( dest->length ) {
    dest->buffer = malloc( dest->length * sizeof *dest->buffer );
    if( dest->buffer == NULL ) return BASIC_ERROR_MEMORY;
    memcpy( dest->buffer,                   string1->buffer, string1->length );
    memcpy( dest->buffer + string1->length, string2->buffer, string2->length );
  } else {
    dest->buffer = NULL;
  }

  return BASIC_ERROR_NONE;
}

error_t
string_generate_printable( struct string *string, char **output )
{
  int i; char *ptr;

  *output = malloc( ( string->length + 1 ) * sizeof **output );
  if( *output == NULL ) return BASIC_ERROR_MEMORY;

  for( i=0, ptr = string->buffer; i < string->length; i++, ptr++ )
    (*output)[i] = ( (*ptr) >= 32 && (*ptr) < 127 ? (*ptr) : '?' );

  (*output)[ string->length ] = '\0';

  return BASIC_ERROR_NONE;
}

error_t
string_free( struct string *string )
{
  if( string->buffer != NULL ) {
    free( string->buffer );
    string->buffer = NULL; string->length = 0;
  }

  return BASIC_ERROR_NONE;
}

error_t
string_destroy( struct string **string )
{
  string_free( *string ); free( *string );
  string = NULL;
  return BASIC_ERROR_NONE;
}
