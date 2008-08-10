/* statement.c: a BASIC statement
   Copyright (c) 2002-2004 Philip Kendall

   $Id: statement.c,v 1.37 2004-05-02 12:12:41 pak21 Exp $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include "config.h"

#define _GNU_SOURCE	/* FIXME: needed for getline(3) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "basic.h"
#include "numexp.h"
#include "printlist.h"
#include "program.h"
#include "statement.h"
#include "strexp.h"
#include "string.h"
#include "token.h"
#include "val.h"

static error_t string_assign_procrustean( struct program *program,
					  const char *name,
					  struct slicer *slicer,
					  struct string *src );
static error_t print_statement( struct program *program,
				struct printlist *list );
static error_t print_numexp( struct program *program, struct numexp *numexp );
static error_t print_strexp( struct program *program, struct strexp *numexp );
static error_t print_separator( enum basic_token separator );

static error_t dim_numeric_statement(
  struct program *program, struct statement_dim_numeric *dim_numeric
);

static error_t
input_statement( struct program *program, struct printlist *printlist );
static error_t get_input_string( struct string **string );

error_t
statement_new_noarg( struct statement **statement, int type )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_NO_ARGS_ID;
  (*statement)->types.no_args = type;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_numexp( struct statement **statement, int type,
		      struct numexp *exp )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_NUMEXP_ARG_ID;
  (*statement)->types.numexp_arg.type = type;
  (*statement)->types.numexp_arg.exp = exp;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_optnumexp( struct statement **statement, int type,
			 struct numexp *exp )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_OPTNUMEXP_ARG_ID;
  (*statement)->types.numexp_arg.type = type;

  (*statement)->types.numexp_arg.exp = exp;
  (*statement)->types.numexp_arg.expression_present = exp ? 1 : 0;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_twonumexp( struct statement **statement, int type,
			 struct numexp *exp1, struct numexp *exp2 )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_TWONUMEXP_ARG_ID;
  (*statement)->types.twonumexp_arg.type = type;
  (*statement)->types.twonumexp_arg.exp1 = exp1;
  (*statement)->types.twonumexp_arg.exp2 = exp2;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_strexp( struct statement **statement, int type,
		      struct strexp *exp )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_STREXP_ARG_ID;
  (*statement)->types.strexp_arg.type = type;
  (*statement)->types.strexp_arg.exp = exp;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_let_numeric( struct statement **statement, const char *name,
			   struct numexp *exp )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_LET_NUMERIC_ID;

  (*statement)->types.let_numeric.name = strdup( name );
  if( (*statement)->types.let_numeric.name == NULL ) {
    free( *statement );
    return BASIC_ERROR_MEMORY;
  }

  (*statement)->types.let_numeric.exp = exp;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_let_numeric_array( struct statement **statement,
				 const char *name, struct explist *subscripts,
				 struct numexp *exp )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_LET_NUMERIC_ARRAY_ID;

  (*statement)->types.let_numeric_array.name = strdup( name );
  if( (*statement)->types.let_numeric.name == NULL ) {
    free( *statement );
    return BASIC_ERROR_MEMORY;
  }
  (*statement)->types.let_numeric_array.subscripts = subscripts;
  (*statement)->types.let_numeric_array.exp = exp;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_let_string( struct statement **statement, const char *name,
			  struct strexp *exp, struct slicer *slicer )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_LET_STRING_ID;

  (*statement)->types.let_numeric.name = strdup( name );
  if( (*statement)->types.let_numeric.name == NULL ) {
    free( *statement );
    return BASIC_ERROR_MEMORY;
  }

  (*statement)->types.let_string.exp = exp;
  (*statement)->types.let_string.slicer = slicer;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_print( struct statement **statement,
		     struct printlist *printlist )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_PRINT_ID;

  (*statement)->types.printlist = printlist;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_if( struct statement **statement, struct numexp *condition,
		  struct statement *true_clause )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_IF_ID;

  (*statement)->types.if_statement.condition = condition;
  (*statement)->types.if_statement.true_clause = true_clause;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_for( struct statement **statement, const char *control,
		   struct numexp *start, struct numexp *end,
		   struct numexp *step )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_FOR_ID;

  (*statement)->types.for_loop.control = strdup( control );
  if( (*statement)->types.for_loop.control == NULL ) {
    free( *statement );
    return BASIC_ERROR_MEMORY;
  }

  (*statement)->types.for_loop.start = start;
  (*statement)->types.for_loop.end   = end;
  (*statement)->types.for_loop.step  = step;

  return BASIC_ERROR_NONE;
}

error_t statement_new_next( struct statement **statement,
			    const char *control )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_NEXT_ID;

  (*statement)->types.next = strdup( control );
  if( (*statement)->types.next == NULL ) {
    free( *statement );
    return BASIC_ERROR_MEMORY;
  }

  return BASIC_ERROR_NONE;
}

error_t
statement_new_deffn( struct statement **statement, const char *name,
		     struct explist *arguments, struct numexp *definition )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_DEFFN_ID;

  (*statement)->types.deffn.name = strdup( name );
  if( (*statement)->types.deffn.name == NULL ) {
    free( statement );
    return BASIC_ERROR_MEMORY;
  }

  (*statement)->types.deffn.arguments = arguments;
  (*statement)->types.deffn.definition = definition;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_dim_numeric( struct statement **statement, const char *name,
			   struct explist *dimensions )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_DIM_NUMERIC_ID;

  (*statement)->types.dim_numeric.name = strdup( name );
  if( (*statement)->types.dim_numeric.name == NULL ) {
    free( statement );
    return BASIC_ERROR_MEMORY;
  }

  (*statement)->types.dim_numeric.dimensions = dimensions;

  return BASIC_ERROR_NONE;
}

error_t
statement_new_input( struct statement **statement,
		     struct printlist *printlist )
{
  *statement = malloc( sizeof **statement );
  if( !(*statement) ) return BASIC_ERROR_MEMORY;

  (*statement)->type = STATEMENT_INPUT_ID;
  (*statement)->types.printlist = printlist;

  return BASIC_ERROR_NONE;
}

error_t
statement_execute( struct program *program, struct statement *statement )
{
  error_t error; float value1, value2, value3;
  struct string *string; char *ptr;

  switch( statement->type ) {

  case STATEMENT_NO_ARGS_ID:
    switch( statement->types.no_args ) {
    case RETURN:
      error = program_gosub_return( program );
      if( error || program->error ) return error;
      break;
    case STOP:
      program->error = BASIC_PROGRAM_ERROR_9;
      break;
    default:
      fprintf( stderr, "Unknown no args statement %d executed\n",
	       statement->types.no_args );
      return BASIC_ERROR_UNKNOWN;
    }
    break;

  case STATEMENT_NUMEXP_ARG_ID:
    error = numexp_eval( program, statement->types.numexp_arg.exp, &value1 );
    if( error || program->error ) return error;

    switch( statement->types.numexp_arg.type ) {
    case GOSUB:
      error = program_gosub( program, value1 );
      if( error || program->error ) return error;
      break;
    case GOTO:
      error = program_set_line( program, value1 );
      if( error || program->error ) return error;
      break;
    case PAUSE:
      printf( "Pausing for %g seconds\n", value1/50 );
      break;
    default:
      fprintf( stderr, "Unknown numexp arg statement %d executed\n",
	       statement->types.numexp_arg.type );
      return BASIC_ERROR_UNKNOWN;
    }
    break;

  case STATEMENT_OPTNUMEXP_ARG_ID:
    if( statement->types.numexp_arg.expression_present ) {
      error = numexp_eval( program, statement->types.numexp_arg.exp, &value1 );
      if( error || program->error ) return error;
    }

    switch( statement->types.numexp_arg.type ) {
    case RANDOMIZE:
      if( statement->types.numexp_arg.expression_present ) {
	srand( value1 );
      } else {
	srand( time( NULL ) );
      }
      break;
    case CLEAR:
      /* Ignore any argument as RAMTOP has little meaning for us */
      error = program_clear_variables( program );
      if( error || program->error ) return error;
      break;
    case RESTORE:
      if( statement->types.numexp_arg.expression_present ) {
	printf( "Restoring to line %g\n", value1 );
      } else {
	printf( "Restoring to start of program\n" );
      }
      break;
    default:
      fprintf( stderr, "Unknown numexp arg statement %d executed\n",
	       statement->types.numexp_arg.type );
      return BASIC_ERROR_UNKNOWN;
    }
    break;

  case STATEMENT_TWONUMEXP_ARG_ID:
    error = numexp_eval( program, statement->types.twonumexp_arg.exp1,
			 &value1 );
    if( error || program-> error ) return error;

    error = numexp_eval( program, statement->types.twonumexp_arg.exp2,
			 &value2 );
    if( error || program->error ) return error;

    switch( statement->types.twonumexp_arg.type ) {
    case BEEP:
      printf( "Beeping at pitch %g for %g seconds\n", value2, value1 );
      break;
    default:
      fprintf( stderr, "Unknown two numexp arg statement %d executed\n",
	       statement->types.twonumexp_arg.type );
      return BASIC_ERROR_UNKNOWN;
    }
    break;

  case STATEMENT_STREXP_ARG_ID:
    error = strexp_eval( program, statement->types.strexp_arg.exp, &string );
    if( error || program->error ) return error;

    switch( statement->types.strexp_arg.type ) {
    case MERGE:
      error = string_generate_printable( string, &ptr );
      if( error ) { string_free( string ); return error; }

      printf( "Merging from \"%s\"\n", ptr );
      free( ptr );
      break;

    default:
      fprintf( stderr, "Unknown strexp arg statement %d executed\n",
	       statement->types.twonumexp_arg.type );
      string_free( string );
      return BASIC_ERROR_UNKNOWN;
    }
    string_free( string );
    break;

  case STATEMENT_LET_NUMERIC_ID:
    error = numexp_eval( program, statement->types.let_numeric.exp, &value1 );
    if( error || program->error ) return error;

    error = program_insert_numeric( program, statement->types.let_numeric.name,
				    value1 );
    if( error || program->error ) return error;
    break;

  case STATEMENT_LET_NUMERIC_ARRAY_ID:
    error = numexp_eval( program, statement->types.let_numeric_array.exp,
			 &value1 );
    if( error || program->error ) return error;

    error = program_set_numeric_array_element(
      program, statement->types.let_numeric_array.name,
      statement->types.let_numeric_array.subscripts, value1
    );
    if( error || program->error ) return error;
    break;

  case STATEMENT_LET_STRING_ID:
    error = strexp_eval( program, statement->types.let_string.exp, &string );
    if( error || program->error ) return error;

    if( statement->types.let_string.slicer ) {
      error =
	string_assign_procrustean( program, statement->types.let_string.name,
				   statement->types.let_string.slicer,
				   string );
    } else {
      error =
	program_insert_string( program, statement->types.let_string.name,
			       string );
    }
    if( error || program->error ) { string_free( string ); return error; }

    string_free( string );
    break;

  case STATEMENT_PRINT_ID:
    error = print_statement( program, statement->types.printlist );
    if( error || program->error ) return error;
    break;

  case STATEMENT_IF_ID:
    error = numexp_eval( program, statement->types.if_statement.condition,
			 &value1 );
    if( error || program->error ) return error;

    /* If condition was false, move along to the next line */
    if( !value1 ) {
      program->current_statement = NULL;
      return BASIC_ERROR_NONE;
    }

    error = statement_execute( program,
			       statement->types.if_statement.true_clause );
    if( error || program->error ) return error;
    break;

  case STATEMENT_FOR_ID:
    error = numexp_eval( program, statement->types.for_loop.start, &value1 );
    if( error || program->error ) return error;

    error = numexp_eval( program, statement->types.for_loop.end, &value2 );
    if( error || program->error ) return error;

    if( statement->types.for_loop.step ) {
      error = numexp_eval( program, statement->types.for_loop.step, &value3 );
      if( error || program->error ) return error;
    } else {
      value3 = 1;
    }

    error = program_initialise_for_loop( program,
					 statement->types.for_loop.control,
					 value1, value2, value3 );
    if( error || program->error ) return error;
    break;

  case STATEMENT_NEXT_ID:
    error = program_for_loop_next( program, statement->types.next );
    if( error || program->error ) return error;
    break;

  case STATEMENT_DEFFN_ID:
    /* Does nothing */
    break;

  case STATEMENT_DIM_NUMERIC_ID:
    error = dim_numeric_statement( program, &(statement->types.dim_numeric) );
    if( error || program->error ) return error;
    break;

  case STATEMENT_INPUT_ID:
    error = input_statement( program, statement->types.printlist );
    if( error || program->error ) return error;
    break;

  default:
    fprintf( stderr, "Unknown statement type %d executed\n", statement->type );
    return BASIC_ERROR_UNKNOWN;

  }

  return BASIC_ERROR_NONE;
}

static error_t
string_assign_procrustean( struct program *program, const char *name,
			   struct slicer *slicer, struct string *src )
{
  struct string *dest;
  float value;
  ssize_t start, end;
  size_t i;
  char *src_ptr, *src_end, *dest_ptr;
  error_t error;

  error = string_new( &dest );
  if( error || program->error ) return error;

  error = program_lookup_string( program, name, dest );
  if( error || program->error ) { string_destroy( &dest ); return error; }

  if( slicer->start ) {
    error = numexp_eval( program, slicer->start, &value );
    if( error || program->error ) { string_destroy( &dest ); return error; }
    start = value + 0.5;
  } else {
    start = 1;
  }

  if( slicer->end ) {
    error = numexp_eval( program, slicer->end, &value );
    if( error || program->error ) { string_destroy( &dest ); return error; }
    end = value + 0.5;
  } else {
    end = dest->length;
  }

  if( start < 0 || end < 0 ) {
    program->error = BASIC_PROGRAM_ERROR_B;	/* Integer out of range */
    string_destroy( &dest );
    return BASIC_ERROR_NONE;
  }

  /* Do nothing if start is after end */
  if( start > end ) { string_destroy( &dest ); return BASIC_ERROR_NONE; }

  if( start == 0 || end > dest->length ) {
    program->error = BASIC_PROGRAM_ERROR_3;	/* Subscript error */
    string_destroy( &dest );
    return BASIC_ERROR_NONE;
  }

  src_ptr = src->buffer;
  src_end = src->buffer + src->length;
  dest_ptr = dest->buffer + start - 1;

  for( i = start; i <= end; i++, src_ptr++, dest_ptr++ )
    *dest_ptr = src_ptr < src_end ? *src_ptr : ' ';

  error = program_insert_string( program, name, dest );
  if( error || program->error ) return error;
  
  string_destroy( &dest );

  return BASIC_ERROR_NONE;
}

static error_t
print_statement( struct program *program, struct printlist *list )
{
  GSList *items;
  enum basic_token last_separator;
  struct printitem *item;
  error_t error;

  items = list->items;
  last_separator = 0;

  while( items ) {

    item = items->data;

    switch( item->type ) {

    case PRINTITEM_NUMEXP:
      error = print_numexp( program, item->types.numexp );
      if( error || program->error ) return error;

      last_separator = 0;
      break;

    case PRINTITEM_STREXP:
      error = print_strexp( program, item->types.strexp );
      if( error || program->error ) return error;

      last_separator = 0;
      break;

    case PRINTITEM_SEPARATOR:
      error = print_separator( item->types.separator );
      if( error ) return error;

      last_separator = item->types.separator;
      break;

    default:
      fprintf( stderr, "Unknown print item type %d found\n", item->type );
      return BASIC_ERROR_UNKNOWN;

    }

    items = items->next;

  }

  if( last_separator == 0 ) printf( "\n" );

  return BASIC_ERROR_NONE;
}

static error_t
print_numexp( struct program *program, struct numexp *numexp )
{
  float value;
  error_t error;

  error = numexp_eval( program, numexp, &value );
  if( error || program->error ) return error;

  printf( "%g", value );

  return BASIC_ERROR_NONE;
}

static error_t
print_strexp( struct program *program, struct strexp *strexp )
{
  struct string *string;
  char *ptr;
  error_t error;

  error = strexp_eval( program, strexp, &string );
  if( error || program->error ) return error;

  error = string_generate_printable( string, &ptr );
  if( error ) { string_free( string ); return error; }

  printf( "%s", ptr );

  free( ptr ); string_destroy( &string );

  return BASIC_ERROR_NONE;
}

static error_t
print_separator( enum basic_token separator )
{
  switch( separator ) {
  case ';':  break;
  case ',':  printf( "\t" ); break;
  case '\'': printf( "\n" ); break;
  default:
    fprintf( stderr, "Unknown print separator %d found\n", separator );
    return BASIC_ERROR_UNKNOWN;
  }

  return BASIC_ERROR_NONE;
}


static error_t
dim_numeric_statement( struct program *program,
		       struct statement_dim_numeric *dim_numeric )
{
  size_t *dimensions, count, i;
  error_t error;

  error = explist_to_integer_array( program, dim_numeric->dimensions,
				    &dimensions, &count );
  if( error ) return error;

  for( i = 0; i < count; i++ ) {
    if( dimensions[i] <= 0 ) {
      program->error = BASIC_PROGRAM_ERROR_3;	/* Subscript wrong */
      free( dimensions );
      return BASIC_ERROR_NONE;
    }
  }

  error = program_create_numeric_array( program, dim_numeric->name,
					dimensions, count );
  if( error || program->error ) { free( dimensions ); return error; }

  return BASIC_ERROR_NONE;
}

static error_t
input_statement( struct program *program, struct printlist *printlist )
{
  GSList *items;
  struct numexp *numexp;
  struct strexp *strexp;
  struct string *string;
  float value;
  error_t error;

  items = printlist->items;

  while( items ) {

    struct printitem *item = items->data;

    switch( item->type ) {

    case PRINTITEM_NUMEXP:
      numexp = item->types.numexp;

      if( numexp->type == NUMEXP_VARIABLE_ID ||
	  numexp->type == NUMEXP_ARRAY_ID       ) {

	error = get_input_string( &string );
	if( error ) return error;

	error = val_eval( program, string, &value );
	if( error ) { string_destroy( &string ); return error; }

	string_destroy( &string );

	if( numexp->type == NUMEXP_VARIABLE_ID ) {
	  error = program_insert_numeric( program, numexp->types.name, value );
	} else {
	  error = program_set_numeric_array_element(
            program, numexp->types.array.name, numexp->types.array.subscripts,
	    value
          );
	}
	if( error ) return error;

      } else {
	error = print_numexp( program, numexp );
	if( error || program->error ) return error;
      }
      break;

    case PRINTITEM_STREXP:
      strexp = item->types.strexp;

      if( strexp->type == STREXP_VARIABLE_ID ) {

	error = get_input_string( &string );
	if( error ) return error;

	error = program_insert_string( program, strexp->types.name, string );
	if( error ) { string_destroy( &string ); return error; }

	string_destroy( &string );

      } else {
	error = print_strexp( program, strexp );
	if( error || program->error ) return error;
      }
      break;

    case PRINTITEM_SEPARATOR:
      error = print_separator( item->types.separator );
      if( error ) return error;
      break;

    default:
      fprintf( stderr, "Unknown print item type %d found\n", item->type );
      return BASIC_ERROR_UNKNOWN;

    }

    items = items->next;
  }

  return BASIC_ERROR_NONE;
}

static error_t
get_input_string( struct string **string )
{
  char *buffer;
  size_t length;
  error_t error;

  buffer = NULL;
  getline( &buffer, &length, stdin );

  /* Remove trailing newline if present */
  length = strlen( buffer );
  if( buffer[ length - 1 ] == '\n' ) length--;

  error = string_new( string );
  if( error ) { free( buffer ); return error; }

  error = string_create( *string, buffer, length );
  if( error ) { string_destroy( string ); free( buffer ); return error; }

  free( buffer );

  return BASIC_ERROR_NONE;
}

void
statement_free( gpointer data, gpointer user_data )
{
  struct statement *statement = data;

  switch( statement->type ) {

  case STATEMENT_NO_ARGS_ID: break;

  case STATEMENT_NUMEXP_ARG_ID:
    numexp_free( statement->types.numexp_arg.exp );
    break;

  case STATEMENT_OPTNUMEXP_ARG_ID:
    if( statement->types.numexp_arg.expression_present )
      numexp_free( statement->types.numexp_arg.exp );
    break;

  case STATEMENT_LET_NUMERIC_ID:
    free( statement->types.let_numeric.name );
    numexp_free( statement->types.let_numeric.exp );
    break;

  case STATEMENT_LET_NUMERIC_ARRAY_ID:
    free( statement->types.let_numeric_array.name );
    explist_free( statement->types.let_numeric_array.subscripts );
    numexp_free( statement->types.let_numeric_array.exp );
    break;

  case STATEMENT_LET_STRING_ID:
    free( statement->types.let_string.name );
    strexp_free( statement->types.let_string.exp );
    break;

  case STATEMENT_PRINT_ID:
    printlist_free( statement->types.printlist );
    break;

  case STATEMENT_IF_ID:
    numexp_free( statement->types.if_statement.condition );
    statement_free( statement->types.if_statement.true_clause, NULL );
    break;

  case STATEMENT_FOR_ID:
    free( statement->types.for_loop.control );
    numexp_free( statement->types.for_loop.start );
    numexp_free( statement->types.for_loop.end );
    if( statement->types.for_loop.step )
      numexp_free( statement->types.for_loop.step );
    break;

  case STATEMENT_NEXT_ID: free( statement->types.next ); break;

  case STATEMENT_DEFFN_ID:
    free( statement->types.deffn.name );
    explist_free( statement->types.deffn.arguments );
    numexp_free( statement->types.deffn.definition );
    break;

  case STATEMENT_DIM_NUMERIC_ID:
    free( statement->types.dim_numeric.name );
    explist_free( statement->types.dim_numeric.dimensions );
    break;

  case STATEMENT_INPUT_ID:
    printlist_free( statement->types.printlist );
    break;

  default:
    fprintf( stderr, "Attempt to free unknown statement type %d\n",
	     statement->type );
    break;
  }

  free( statement );
}
