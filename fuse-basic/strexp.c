/* strexp.c: A string expression
   Copyright (c) 2002-2003 Philip Kendall

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "numexp.h"
#include "program.h"
#include "spectrum-string.h"
#include "strexp.h"
#include "val.h"

error_t
strexp_new_string( struct strexp **strexp, struct string *string )
{
  *strexp = malloc( sizeof **strexp );
  if( (*strexp) == NULL ) return BASIC_ERROR_MEMORY;

  (*strexp)->type = STREXP_STRING_ID;
  (*strexp)->types.string = string;

  return BASIC_ERROR_NONE;
}

error_t
strexp_new_twostrexp( struct strexp **strexp, int type,
		      struct strexp *exp1, struct strexp *exp2 )
{
  *strexp = malloc( sizeof **strexp );
  if( (*strexp) == NULL ) return BASIC_ERROR_MEMORY;

  (*strexp)->type = STREXP_TWOSTREXP_ID;
  (*strexp)->types.twostrexp.type = '+';
  (*strexp)->types.twostrexp.exp1 = exp1;
  (*strexp)->types.twostrexp.exp2 = exp2;

  return BASIC_ERROR_NONE;
}

error_t
strexp_new_variable( struct strexp **strexp, const char *name )
{
  *strexp = malloc( sizeof **strexp );
  if( !(*strexp) ) return BASIC_ERROR_MEMORY;

  (*strexp)->type = STREXP_VARIABLE_ID;
  (*strexp)->types.name = strdup( name );
  if( (*strexp)->types.name == NULL ) return BASIC_ERROR_MEMORY;

  return BASIC_ERROR_NONE;
}

error_t
strexp_new_numexp( struct strexp **strexp, enum basic_token type,
		   struct numexp *exp )
{
  *strexp = malloc( sizeof **strexp );
  if( !(*strexp) ) return BASIC_ERROR_MEMORY;

  (*strexp)->type = STREXP_NUMEXP_ID;
  (*strexp)->types.numexp.type = type;
  (*strexp)->types.numexp.exp = exp;

  return BASIC_ERROR_NONE;
}

error_t
strexp_new_strexp( struct strexp **strexp, enum basic_token type,
		   struct strexp *exp )
{
  *strexp = malloc( sizeof **strexp );
  if( !(*strexp) ) return BASIC_ERROR_MEMORY;

  (*strexp)->type = STREXP_STREXP_ID;
  (*strexp)->types.strexp.type = type;
  (*strexp)->types.strexp.exp = exp;

  return BASIC_ERROR_NONE;
}

error_t
strexp_new_and( struct strexp **strexp,
		struct strexp *exp1, struct numexp *exp2 )
{
  *strexp = malloc( sizeof **strexp );
  if( (*strexp) == NULL ) return BASIC_ERROR_MEMORY;

  (*strexp)->type = STREXP_AND_ID;
  (*strexp)->types.and.exp1 = exp1;
  (*strexp)->types.and.exp2 = exp2;

  return BASIC_ERROR_NONE;
}

error_t
strexp_new_slicer( struct strexp **strexp, struct strexp *exp,
		   struct slicer *slicer )
{
  *strexp = malloc( sizeof **strexp );
  if( (*strexp) == NULL ) return BASIC_ERROR_MEMORY;

  (*strexp)->type = STREXP_SLICER_ID;
  (*strexp)->types.slicer.exp = exp;
  (*strexp)->types.slicer.slicer = slicer;

  return BASIC_ERROR_NONE;
}

error_t
strexp_eval( struct program *program, struct strexp *strexp,
	     struct string **string )
{
  error_t error; struct string *string1, *string2; float value1, value2;
  size_t start, end, length;

  int integer; char character, buffer[256];

  error = string_new( string ); if( error != BASIC_ERROR_NONE ) return error;

  switch( strexp->type ) {

  case STREXP_STRING_ID:
    error = string_copy( (*string), strexp->types.string );
    if( error != BASIC_ERROR_NONE ) { string_destroy( string ); return error; }
    break;

  case STREXP_VARIABLE_ID:
    error = program_lookup_string( program, strexp->types.name, *string );
    if( error || program->error ) { string_destroy( string ); return error; }
    break;

  case STREXP_NUMEXP_ID:
    error = numexp_eval( program, strexp->types.numexp.exp, &value1 );
    if( error || program->error ) { string_destroy( string ); return error; }

    switch( strexp->types.numexp.type ) {
    case CHRS:			/* CHR$ */
      integer = value1;
      if( integer < 0 || integer >= 256 ) {
	program->error = BASIC_PROGRAM_ERROR_B;
	string_destroy( string );
	return BASIC_ERROR_NONE;
      }
      
      character = integer;
      error = string_create( (*string), &character, 1 );
      if( error ) { string_destroy( string ); return error; }
      break;

    case STRS:			/* STR$ */
      snprintf( buffer, 256, "%f", value1 );
      error = string_create( (*string), buffer, strlen( buffer ) );
      if( error ) { string_destroy( string ); return error; }
      break;

    default:
      fprintf( stderr, "Unknown numexp strexp type %d evaluated\n",
	       strexp->types.numexp.type );
      return BASIC_ERROR_UNKNOWN;
    }
    break;

  case STREXP_STREXP_ID:
    error = strexp_eval( program, strexp->types.strexp.exp, &string1 );
    if( error || program->error ) { string_destroy( string ); return error; }

    switch( strexp->types.strexp.type ) {
    case VALS:
      error = vals_eval( program, string1, string );
      if( error || program->error ) return error;
      break;

    default:
      fprintf( stderr, "Unknown strexp strexp type %d evaluated\n",
	       strexp->types.strexp.type );
      string_destroy( string ); string_free( string1 );
      return BASIC_ERROR_UNKNOWN;
    }
    break;

  case STREXP_TWOSTREXP_ID:
    error = strexp_eval( program, strexp->types.twostrexp.exp1, &string1 );
    if( error || program->error ) { string_destroy( string ); return error; }

    error = strexp_eval( program, strexp->types.twostrexp.exp2, &string2 );
    if( error || program->error ) {
      string_destroy( string ); string_free( string1 );
      return error;
    }

    switch( strexp->types.twostrexp.type ) {
    case '+':
      error = string_concat( (*string), string1, string2 );
      if( error != BASIC_ERROR_NONE ) {
	string_destroy( string );
	string_free( string1 ); string_free( string2 );
	return error;
      }
      break;

    default:
      fprintf( stderr, "Unknown two arg strexp type %d evaluated\n",
	       strexp->types.twostrexp.type );
      string_destroy( string ); string_free( string1 ); string_free( string2 );
      return BASIC_ERROR_UNKNOWN;
    }
    string_free( string1 ); string_free( string2 );
    break;

  case STREXP_AND_ID:
    error = numexp_eval( program, strexp->types.and.exp2, &value1 );
    if( error || program->error ) { string_destroy( string ); return error; }
    
    error = strexp_eval( program, strexp->types.and.exp1, &string1 );
    if( error || program->error ) { string_destroy( string ); return error; }

    if( value1 ) {
      error = string_copy( *string, string1 );
      if( error ) {
	string_destroy( string ); string_free( string1 );
	return error;
      }
    }
    
    string_free( string1 );
    break;

  case STREXP_SLICER_ID:
    error = strexp_eval( program, strexp->types.slicer.exp, &string1 );
    if( error || program->error ) { string_destroy( string ); return error; }

    if( strexp->types.slicer.slicer->start ) {
      error = numexp_eval( program, strexp->types.slicer.slicer->start,
			   &value1 );
      if( error || program->error ) {
	string_destroy( string ); string_free( string1 );
	return error;
      }
      start = value1;
    } else {
      start = 1;
    }

    if( strexp->types.slicer.slicer->end ) {
      error = numexp_eval( program, strexp->types.slicer.slicer->end,
			   &value2 );
      if( error || program->error ) {
	string_destroy( string ); string_free( string1 );
	return error;
      }
      end = value2;
    } else {
      end = string1->length;
    }

    if( start < 1 || end > string1->length ) {
      string_destroy( string ); string_free( string1 );
      program->error = BASIC_PROGRAM_ERROR_3;
      return BASIC_ERROR_NONE;
    }

    length = end >= start ? end - start + 1 : 0;
    error = string_create( (*string), &((string1->buffer)[start-1]), length );
    if( error ) {
      string_destroy( string ); string_free( string1 );
      return error;
    }

    string_free( string1 );
    return BASIC_ERROR_NONE;

  default:
    fprintf( stderr, "Unknown strexp type %d evaluated\n", strexp->type );
    string_destroy( string );
    return BASIC_ERROR_UNKNOWN;

  }

  return BASIC_ERROR_NONE;
}

error_t
slicer_new( struct slicer **slicer, struct numexp *start, struct numexp *end )
{
  *slicer = malloc( sizeof **slicer );
  if( (*slicer) == NULL ) return BASIC_ERROR_MEMORY;

  (*slicer)->start = start;
  (*slicer)->end   = end;

  return BASIC_ERROR_NONE;
}

error_t
strexp_free( struct strexp *strexp )
{
  switch( strexp->type ) {

  case STREXP_STRING_ID: string_free( strexp->types.string ); break;
  case STREXP_VARIABLE_ID: free( strexp->types.name ); break;
  case STREXP_NUMEXP_ID: numexp_free( strexp->types.numexp.exp ); break;
  case STREXP_STREXP_ID: strexp_free( strexp->types.strexp.exp ); break;
    
  case STREXP_TWOSTREXP_ID:
    strexp_free( strexp->types.twostrexp.exp1 );
    strexp_free( strexp->types.twostrexp.exp2 );
    break;

  case STREXP_AND_ID:
    strexp_free( strexp->types.and.exp1 );
    numexp_free( strexp->types.and.exp2 );
    break;

  case STREXP_SLICER_ID:
    strexp_free( strexp->types.slicer.exp );
    if( strexp->types.slicer.slicer->start )
      numexp_free( strexp->types.slicer.slicer->start );
    if( strexp->types.slicer.slicer->end )
      numexp_free( strexp->types.slicer.slicer->end );
    free( strexp->types.slicer.slicer );
    break;

  default:
    fprintf( stderr, "Attempt to free unknown strexp type %d\n",
	     strexp->type );
    break;
  }

  free( strexp );

  return BASIC_ERROR_NONE;
}
