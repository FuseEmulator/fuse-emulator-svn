/* numexp.c: a BASIC numerical expression
   Copyright (c) 2002-2004 Philip Kendall

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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basic.h"
#include "numexp.h"
#include "program.h"
#include "strexp.h"
#include "spectrum-string.h"
#include "token.h"
#include "val.h"

static error_t function_eval( struct program *program,
			      struct numexp_fn *function, float *value );

error_t
numexp_new_variable( struct numexp **numexp, const char *name )
{
  *numexp = malloc( sizeof **numexp );
  if( !(*numexp) ) return BASIC_ERROR_MEMORY;

  (*numexp)->type = NUMEXP_VARIABLE_ID;
  (*numexp)->types.name = strdup( name );
  if( (*numexp)->types.name == NULL ) return BASIC_ERROR_MEMORY;

  return BASIC_ERROR_NONE;
}

error_t
numexp_new_array( struct numexp **numexp, const char *name,
		  struct explist *subscripts )
{
  *numexp = malloc( sizeof **numexp );
  if( !(*numexp ) ) return BASIC_ERROR_MEMORY;

  (*numexp)->type = NUMEXP_ARRAY_ID;
  (*numexp)->types.array.name = strdup( name );
  if( (*numexp)->types.array.name == NULL ) {
    free( *numexp );
    return BASIC_ERROR_MEMORY;
  }
  (*numexp)->types.array.subscripts = subscripts;

  return BASIC_ERROR_NONE;
}

error_t
numexp_new_noarg( struct numexp **numexp, int type )
{
  *numexp = malloc( sizeof **numexp );
  if( !(*numexp) ) return BASIC_ERROR_MEMORY;

  (*numexp)->type = NUMEXP_NO_ARG_ID;
  (*numexp)->types.no_arg = type;

  return BASIC_ERROR_NONE;
}

error_t
numexp_new_onearg( struct numexp **numexp, int type, struct numexp *exp )
{
  *numexp = malloc( sizeof **numexp );
  if( !(*numexp) ) return BASIC_ERROR_MEMORY;

  (*numexp)->type = NUMEXP_ONE_ARG_ID;
  (*numexp)->types.one_arg.type = type;
  (*numexp)->types.one_arg.exp = exp;

  return BASIC_ERROR_NONE;
}

error_t
numexp_new_twoarg( struct numexp **numexp, int type,
		   struct numexp *exp1, struct numexp *exp2 )
{
  *numexp = malloc( sizeof **numexp );
  if( !(*numexp) ) return BASIC_ERROR_MEMORY;

  (*numexp)->type = NUMEXP_TWO_ARG_ID;
  (*numexp)->types.two_arg.type = type;
  (*numexp)->types.two_arg.exp1 = exp1;
  (*numexp)->types.two_arg.exp2 = exp2;

  return BASIC_ERROR_NONE;
}

error_t
numexp_new_strexp( struct numexp **numexp, int type, struct strexp *exp )
{
  *numexp = malloc( sizeof **numexp );
  if( !(*numexp) ) return BASIC_ERROR_MEMORY;

  (*numexp)->type = NUMEXP_STREXP_ID;
  (*numexp)->types.strexp.type = type;
  (*numexp)->types.strexp.exp = exp;

  return BASIC_ERROR_NONE;
}

error_t
numexp_new_fn( struct numexp **numexp, const char *name,
	       struct explist *explist )
{
  *numexp = malloc( sizeof **numexp );
  if( !(*numexp) ) return BASIC_ERROR_MEMORY;

  (*numexp)->type = NUMEXP_FN_ID;

  (*numexp)->types.function.name = strdup( name );
  if( (*numexp)->types.function.name == NULL ) return BASIC_ERROR_MEMORY;

  (*numexp)->types.function.arguments = explist;

  return BASIC_ERROR_NONE;
}

error_t
numexp_eval( struct program *program, struct numexp *numexp, float *value )
{
  error_t error; float value1, value2; struct string *string;

  switch( numexp->type ) {

  case NUMEXP_NUMBER_ID:
    *value = numexp->types.number;
    break;

  case NUMEXP_VARIABLE_ID:
    error = program_lookup_numeric( program, numexp->types.name, value );
    if( error || program->error ) return error;
    break;

  case NUMEXP_ARRAY_ID:
    error =
      program_get_numeric_array_element( program, numexp->types.array.name,
					 numexp->types.array.subscripts,
					 value );
    if( error || program->error ) return error;
    break;

  case NUMEXP_NO_ARG_ID:
    switch( numexp->types.no_arg ) {
    case PI: (*value) = M_PI; return BASIC_ERROR_NONE;

      /* FIXME: problems if running multiple programs at the same time */
    case RND: (*value) = (float)rand() / RAND_MAX; return BASIC_ERROR_NONE;

    default:
      fprintf( stderr, "Unknown no arg numexp type %d evaluated\n",
	       numexp->types.one_arg.type );
      return BASIC_ERROR_UNKNOWN;
    }
    break;
    
  case NUMEXP_ONE_ARG_ID:
    error = numexp_eval( program, numexp->types.one_arg.exp, &value1 );
    if( error || program->error ) return error;

    switch( numexp->types.one_arg.type ) {
    case '-':
      (*value) = -value1;
      break;
    case SPECTRUM_ABS: (*value) = value1 >= 0 ? value1 : -value1; break;
    case ACS:
      if( value1 < -1 || value1 > 1 ) {
	program->error = BASIC_PROGRAM_ERROR_A;
	return BASIC_ERROR_NONE;
      }
      (*value) = acos( value1 );
      break;
    case ASN:
      if( value1 < -1 || value1 > 1 ) {
	program->error = BASIC_PROGRAM_ERROR_A;
	return BASIC_ERROR_NONE;
      }
      (*value) = asin( value1 );
      break;
    case ATN: (*value) = atan( value1 ); break;
    case COS: (*value) = cos( value1 ); break;
    case EXP: (*value) = exp( value1 ); break;
    case INT: (*value) = floor( value1 ); break;
    case LN:
      if( value1 <= 0 ) {
	program->error = BASIC_PROGRAM_ERROR_A; /* Invalid argument */
	return BASIC_ERROR_NONE;
      }
      (*value) = log( value1 );
      break;
    case NOT: (*value) = value1 ? 0 : 1; break;
    case SGN: (*value) = value1 < 0 ? -1 : ( value1 > 0 ? 1 : 0 ); break;
    case SIN: (*value) = sin( value1 ); break;
    case SQR:
      if( value1 < 0 ) {
	program->error = BASIC_PROGRAM_ERROR_A; /* Invalid argument */
	return BASIC_ERROR_NONE;
      }
      (*value) = sqrt( value1 );
      break;
    case TAN: (*value) = tan( value1 ); break;

    default:
      fprintf( stderr, "Unknown one arg numexp type %d evaluated\n",
	       numexp->types.one_arg.type );
      return BASIC_ERROR_UNKNOWN;
    }
    break;

  case NUMEXP_TWO_ARG_ID:
    error = numexp_eval( program, numexp->types.two_arg.exp1, &value1 );
    if( error || program->error ) return error;

    error = numexp_eval( program, numexp->types.two_arg.exp2, &value2 );
    if( error || program->error ) return error;

    switch( numexp->types.two_arg.type ) {

      /* FIXME: error checking */
    case '+': *value = value1 + value2; break;
    case '-': *value = value1 - value2; break;
    case '*': *value = value1 * value2; break;
    case '/': *value = value1 / value2; break;
    case '^': *value = pow( value1, value2 ); break;

    case '=': *value = value1 == value2 ? 1 : 0; break;
    case '<': *value = value1 < value2 ? 1 : 0; break;
    case '>': *value = value1 > value2 ? 1 : 0; break;
    case NE: *value = value1 != value2 ? 1 : 0; break;
    case LE: *value = value1 <= value2 ? 1 : 0; break;
    case GE: *value = value1 >= value2 ? 1 : 0; break;

    case AND: *value = value2 ? value1 : 0; break;
    case OR: *value = value2 ? 1 : value1; break;
      
    default:
      fprintf( stderr, "Unknown two arg numexp type %d evaluated\n",
	       numexp->types.two_arg.type );
      return BASIC_ERROR_UNKNOWN;
    }
    break;

  case NUMEXP_STREXP_ID:
    error = strexp_eval( program, numexp->types.strexp.exp, &string );
    if( error || program->error ) return error;

    switch( numexp->types.strexp.type ) {
    case CODE:
      (*value) = string->length ? string->buffer[0] : 0; break;
    case LEN:
      (*value) = string->length; break;
    case VAL:
      error = val_eval( program, string, value );
      if( error || program->error ) return error;
      break;

    default:
      fprintf( stderr, "Unknown strexp numexp type %d evaluated\n",
	       numexp->types.two_arg.type );
      string_free( string );
      return BASIC_ERROR_UNKNOWN;
    }
    string_free( string );
    break;

  case NUMEXP_FN_ID:
    error = function_eval( program, &(numexp->types.function), value );
    if( error || program->error ) return error;
    break;

  default:
    fprintf( stderr, "Unknown numexp type %d evaluated\n", numexp->type );
    return BASIC_ERROR_UNKNOWN;
  }

  return BASIC_ERROR_NONE;
}

/* function_eval: evaluate a FN call

   Note that this won't necessarily give the same errors as the Spectrum
   would. Known differences:

   * The Spectrum evaluates all arguments before testing number and types,
     whereas this tests number and type of arguments first. Hence a call
     which has both wrong arguments and undefined variables used in those
     arguments gives `2 Variable not found' on the Spectrum, but
     `Q Parameter error' here.

   * If a function takes arguments, but is called without any then the
     Spectrum gives `C Nonsense in BASIC'; again we return `Q Parameter error'.
*/
static error_t
function_eval( struct program *program, struct numexp_fn *function_call,
	       float *result )
{
  struct program_function *function_def;
  GSList *argument_list, *value_list;
  struct expression *argument, *value;
  error_t error;

  /* First, see if the function exists */
  function_def = g_hash_table_lookup( program->functions,
				      function_call->name );

  /* If it doesn't, return with error */
  if( !function_def ) {
    program->error = BASIC_PROGRAM_ERROR_P; /* FN without DEF */
    return BASIC_ERROR_NONE;
  }

  /* Now check the number and type of arguments */
  argument_list = function_def->arguments->items;
  value_list    = function_call->arguments->items;

  while( argument_list && value_list ) {

    argument = argument_list->data;
    value =    value_list->data;

    if( argument->type != value->type ) {
      program->error = BASIC_PROGRAM_ERROR_Q; /* Parameter error */
      return BASIC_ERROR_NONE;
    }

    argument_list = argument_list->next;
    value_list    = value_list->next;
  }

  /* Now both the arguments and the values should have reached the end
     at the same time */
  if( argument_list || value_list ) {
    program->error = BASIC_PROGRAM_ERROR_Q; /* Parameter error */
    return BASIC_ERROR_NONE;
  }

  /* Now create a new scratchpad for the lexicals */
  error = program_push_scratchpad( program, function_def->arguments,
				   function_call->arguments );
  if( error || program->error ) return error;

  error = numexp_eval( program, function_def->definition, result );
  if( error || program->error ) return error;

  error = program_pop_scratchpad( program );
  if( error || program->error ) return error;

  return BASIC_ERROR_NONE;
}

error_t
numexp_free( struct numexp *numexp )
{
  switch( numexp->type ) {
    
  case NUMEXP_NUMBER_ID: break;
  case NUMEXP_VARIABLE_ID: free( numexp->types.name ); break;

  case NUMEXP_ARRAY_ID:
    free( numexp->types.array.name );
    explist_free( numexp->types.array.subscripts );
    break;

  case NUMEXP_NO_ARG_ID: break;
  case NUMEXP_ONE_ARG_ID: numexp_free( numexp->types.one_arg.exp ); break;

  case NUMEXP_TWO_ARG_ID:
    numexp_free( numexp->types.two_arg.exp1 );
    numexp_free( numexp->types.two_arg.exp2 );
    break;

  case NUMEXP_STREXP_ID: strexp_free( numexp->types.strexp.exp ); break;

  case NUMEXP_FN_ID:
    free( numexp->types.function.name );
    explist_free( numexp->types.function.arguments );
    break;

  default:
    fprintf( stderr, "Attempt to free unknown numexp type %d\n",
	     numexp->type );
    break;
  }

  free( numexp );

  return BASIC_ERROR_NONE;
}
