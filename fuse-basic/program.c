/* program.c: routines for dealing with an entire program
   Copyright (c) 2002-2004 Philip Kendall

   $Id: program.c,v 1.31 2004-05-03 14:24:38 pak21 Exp $

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "basic.h"
#include "line.h"
#include "program.h"
#include "statement.h"

static gint sort_by_line_number( gconstpointer a, gconstpointer b );
static error_t free_lines( GSList *lines );
static error_t find_function_in_statement( struct program *program,
					   struct statement *statement );
static error_t add_function( struct program *program,
			     struct statement_deffn *function );
static error_t free_functions( GHashTable *functions );
static void free_function( gpointer key, gpointer value, gpointer user_data );
static gint find_line( gconstpointer from_list, gconstpointer from_caller );

static error_t find_closing_next( struct program *program,
				  const char *control );
static error_t check_for_next( struct statement *statement,
			       const char *control );

static error_t
get_numeric_array_element_ptr( struct program *program, const char *name,
			       struct explist *subscripts, float **value_ptr );

static error_t initialise_symbol_table( struct program_symbol_table *table );
static error_t destroy_symbol_table( struct program_symbol_table *table );
static error_t clear_symbol_table( struct program_symbol_table *table );
static gboolean free_numeric_var( gpointer key, gpointer value,
				  gpointer user_data );
static gboolean free_string_var( gpointer key, gpointer value,
				 gpointer user_data );
static gboolean free_numeric_array( gpointer key, gpointer value,
				    gpointer user_data );

/* A position in the program */
struct program_position {

  GSList* line;
  GSList* statement;

};

/* A numeric variable, along with the appropriate magic for dealing with
   FOR loops */
struct numeric_variable {

  float value;		/* Hopefully obvious */

  int magic;		/* Does this variable have any magic? */

  /* FOR variable magic */
  float end;
  float step;

  struct program_position loop; /* which instruction to loop to */

};

/* Types of variable magic */
const int PROGRAM_VARIABLE_MAGIC_FOR = 0x01; /* A FOR loop control variable */

/* A numeric array */
struct numeric_array {

  size_t count;			/* Number of dimensions */
  size_t *dimensions;		/* Number of elements in each dimension */
  float *values;		/* The actual values */

};

error_t
program_initialise( struct program *program )
{
  error_t error;

  program->current_line = NULL;
  program->lines = NULL;
  program->error = BASIC_PROGRAM_ERROR_NONE;

  error = initialise_symbol_table( &(program->variables) );
  if( error != BASIC_ERROR_NONE ) return error;

  program->scratchpads = NULL;

  program->gosub_stack = g_array_new( FALSE, FALSE,
				      sizeof( struct program_position ) );

  program->functions = g_hash_table_new( g_str_hash, g_str_equal );

  return BASIC_ERROR_NONE;
}

error_t
program_add_line( struct program *program, struct line *line )
{
  if( !line ) return BASIC_ERROR_NONE;

  /* FIXME: check line number doesn't already exist */
  program->lines = g_slist_insert_sorted( program->lines, line,
					  sort_by_line_number );

  return BASIC_ERROR_NONE;
}

/* Comparison function to keep program sorted in line number order */
static gint
sort_by_line_number( gconstpointer a, gconstpointer b )
{
  const struct line *ptr1 = a;
  const struct line *ptr2 = b;

  return ptr1->line_number - ptr2->line_number;
}

static error_t
free_lines( GSList *lines )
{
  g_slist_foreach( lines, line_free, NULL );
  g_slist_free( lines );

  return BASIC_ERROR_NONE;
}

error_t
program_find_functions( struct program *program )
{
  GSList *line_list, *statement_list;
  error_t error;

  line_list = program->lines;

  while( line_list ) {

    struct line *line;

    line = line_list->data;
    statement_list = line->statements;

    while( statement_list ) {
      error = find_function_in_statement( program, statement_list->data );
      if( error ) return error;

      statement_list = statement_list->next;
    }

    line_list = line_list->next;
  }

  return BASIC_ERROR_NONE;
}

static error_t
find_function_in_statement( struct program *program,
			    struct statement *statement )
{
  error_t error;

  switch( statement->type ) {
  case STATEMENT_DEFFN_ID:
    error = add_function( program, &(statement->types.deffn) );
    if( error ) return error;
    break;

    /* This bit is evil: even something like the very counter intuitive
       IF 0 THEN DEF FN a(b) = SQR b
       will define the function `a' due to the way the ROM stores
       statements as token sequences rather than as a parse tree */
  case STATEMENT_IF_ID:
    error = find_function_in_statement(
      program, statement->types.if_statement.true_clause
    );
    if( error ) return error;
    break;

  default:
    break;
  }

  return BASIC_ERROR_NONE;
}

static error_t
add_function( struct program *program, struct statement_deffn *function )
{
  gpointer hash_entry;
  struct program_function *value;

  /* First, see if this function is already defined */
  hash_entry = g_hash_table_lookup( program->functions, function->name );

  if( hash_entry ) {
    fprintf( stderr, "Warning: redefinition of function `%s'; later definition(s) will be ignored\n",
	     function->name );
    return BASIC_ERROR_NONE;
  }

  /* Create a new object to go into the hash table */
  value = malloc( sizeof *value );
  if( value == NULL ) return BASIC_ERROR_MEMORY;

  value->arguments = function->arguments;
  value->definition = function->definition;

  /* And place it into the hash table */
  g_hash_table_insert( program->functions, function->name, value );
  
  return BASIC_ERROR_NONE;
}

static error_t
free_functions( GHashTable *functions )
{
  g_hash_table_foreach( functions, free_function, NULL );
  g_hash_table_destroy( functions );

  return BASIC_ERROR_NONE;
}

static void
free_function( gpointer key, gpointer value, gpointer user_data )
{
  struct program_function *function = value;

  free( function );
}

error_t
program_execute_statement( struct program *program )
{
  struct line *line;
  struct statement *statement;
  error_t error;

  /* If we haven't got a current line, use the first line of the program */
  if( program->current_line == NULL ) {

    /* If we don't actually have a program, return successfully */
    if( program->lines == NULL ) {
      program->error = BASIC_PROGRAM_ERROR_0;
      return BASIC_ERROR_NONE;
    }

    /* Otherwise, use the first statement in the first line of the program */
    program->current_line = program->lines;
    line = program->current_line->data;
    program->current_statement = line->statements;
  }

  /* If the current statement is NULL (end of line), then move along to the
     next line with a statement on */
  while( program->current_statement == NULL ) {

    program->current_line = program->current_line->next;
    /* Check that wasn't the end of program; if so, return successfully */
    if( program->current_line == NULL ) {
      program->error = BASIC_PROGRAM_ERROR_0;
      return BASIC_ERROR_NONE;
    }

    line = program->current_line->data;
    program->current_statement = line->statements;
  }

  /* Get this statement */
  statement = program->current_statement->data;

  /* And move along to the next one */
  program->current_statement = program->current_statement->next;

  /* Now actually execute the statement; done in this order so GO TO et al
     work properly */
  error = statement_execute( program, statement );
  if( error ) return error;

  /* Return if BASIC execution gave an error */
  /* FIXME: do we want to move the current statement along in this case? */
  if( program->error ) return BASIC_ERROR_NONE;

  return BASIC_ERROR_NONE;
}

/* Insert a numeric variable into the symbol table */
error_t
program_insert_numeric( struct program *program, const char *name,
			float numeric_value )
{
  char *key; struct numeric_variable *value;

  /* See if this variable already exists; if it does, update its value */
  value = g_hash_table_lookup( program->variables.numeric, name );

  if( value ) {
    value->value = numeric_value;
    return BASIC_ERROR_NONE;
  }

  /* If not, allocate memory for the new key and value */
  key = strdup( name ); if( key == NULL ) return BASIC_ERROR_MEMORY;
  value = malloc( sizeof *value );
  if( value == NULL ) { free( key ); return BASIC_ERROR_MEMORY; }

  value->magic = 0;			/* Nothing special about this one */
  value->value = numeric_value;

  /* Finally put the new value in */
  g_hash_table_insert( program->variables.numeric, key, value );

  return BASIC_ERROR_NONE;
}

/* Fetch a numeric variable, either from the current lexical scope
   or from the global symbol table */
error_t
program_lookup_numeric( struct program *program, const char *name,
			float *value )
{
  struct program_symbol_table *scratchpad;
  struct numeric_variable *variable;

  variable = NULL;

  /* First look in the current lexical variables (if any exist) */
  if( program->scratchpads ) {
    scratchpad = program->scratchpads->data;
    variable = g_hash_table_lookup( scratchpad->numeric, name );
  }

  /* If that didn't work, try the global variables */
  if( !variable ) {
    variable = g_hash_table_lookup( program->variables.numeric, name );
    /* If we still couldn't find it, return with error */
    if( !variable ) {
      program->error = BASIC_PROGRAM_ERROR_2;
      return BASIC_ERROR_NONE;
    }
  }

  *value = variable->value;
  return BASIC_ERROR_NONE;
}

/* Insert a string variable into the symbol table */
error_t
program_insert_string( struct program *program, const char *name,
		       struct string *string )
{
  error_t error;
  char *key; struct string *value;

  key = NULL;

  value = g_hash_table_lookup( program->variables.string, name );

  /* If the variable doesn't exist already, allocate memory */
  if( !value ) {
    key = strdup( name ); if( key == NULL ) return BASIC_ERROR_MEMORY;

    error = string_new( &value ); if( error ) { free( key ); return error; }
    g_hash_table_insert( program->variables.string, key, value );
  }

  /* Copy the new value in */
  error = string_copy( value, string );
  if( error ) {
    /* Free up memory if we allocated it */
    if( key ) { free( key ); string_destroy( &value ); }
    return error;
  }

  return BASIC_ERROR_NONE;
}

/* Fetch a string variable either from the current lexical scope or
   the global symbol table */
error_t
program_lookup_string( struct program *program, const char *name,
		       struct string *string )
{
  struct program_symbol_table *scratchpad;
  struct string *variable;
  error_t error;

  variable = NULL;

  /* First look in the current lexical variables (if any exist) */
  if( program->scratchpads ) {
    scratchpad = program->scratchpads->data;
    variable = g_hash_table_lookup( scratchpad->string, name );
  }

  /* If that didn't work, try the global variables */
  if( !variable ) {
    variable = g_hash_table_lookup( program->variables.string, name );
    /* If we still couldn't find it, return with error */
    if( !variable ) {
      program->error = BASIC_PROGRAM_ERROR_2;
      return BASIC_ERROR_NONE;
    }
  }

  error = string_copy( string, variable );
  if( error ) return error;

  return BASIC_ERROR_NONE;
}

/* Create a new numeric array (or possibly replace an existing one) */
error_t
program_create_numeric_array( struct program *program, const char *name,
			      size_t *dimensions, size_t count )
{
  char *key; int free_key;
  struct numeric_array *value;
  float *elements; size_t element_count, i;

  /* See if this array already exists */
  value = g_hash_table_lookup( program->variables.numeric_array, name );

  /* If it does, remove the old value */
  if( value ) {
    free( value->dimensions ); free( value->values );
    free_key = 1;			/* Old key will be reused */
  } else {
    free_key = 0;
  }

  key = strdup( name ); if( !key ) return BASIC_ERROR_MEMORY;

  value = malloc( sizeof *value );
  if( !value ) { free( key ); return BASIC_ERROR_MEMORY; }
  
  /* FIXME: Possibility of integer overflow */
  element_count = 1;
  for( i=0; i<count; i++ ) element_count *= dimensions[i];

  elements = malloc( element_count * sizeof *elements );
  if( !elements ) { free( value ); free( key ); return BASIC_ERROR_MEMORY; }

  for( i = 0; i < element_count; i++ ) elements[i] = 0;

  value->count = count; value->dimensions = dimensions;
  value->values = elements;

  /* Finally put the new value in */
  g_hash_table_insert( program->variables.numeric_array, key, value );

  if( free_key ) free( key );

  return BASIC_ERROR_NONE;
}

/* Add a value into an existing numeric array */
error_t
program_set_numeric_array_element( struct program *program, const char *name,
				   struct explist *subscripts, float value )
{
  float *value_ptr;
  error_t error;

  error = get_numeric_array_element_ptr( program, name, subscripts,
					 &value_ptr );
  if( error || program->error ) return error;

  *value_ptr = value;

  return BASIC_ERROR_NONE;
}

error_t
program_get_numeric_array_element( struct program *program, const char *name,
				   struct explist *subscripts, float *value )
{
  float *value_ptr;
  error_t error;

  error = get_numeric_array_element_ptr( program, name, subscripts,
					 &value_ptr );
  if( error || program->error ) return error;

  *value = *value_ptr;

  return BASIC_ERROR_NONE;
}

static error_t
get_numeric_array_element_ptr( struct program *program, const char *name,
			       struct explist *subscripts, float **value_ptr )
{
  struct numeric_array *array;
  size_t *dimensions, count, multiplier, element, i;
  error_t error;

  /* Check this array exists */
  array = g_hash_table_lookup( program->variables.numeric_array, name );

  /* `Variable not found' if it doesn't */
  if( !array ) {
    program->error = BASIC_PROGRAM_ERROR_2;	/* Variable not found */
    return BASIC_ERROR_NONE;
  }

  error = explist_to_integer_array( program, subscripts, &dimensions, &count );
  if( error ) return error;

  /* Check the number of dimensions is right */
  if( count != array->count ) {
    program->error = BASIC_PROGRAM_ERROR_3;    /* Subscript wrong */
    return BASIC_ERROR_NONE;
  }

  /* Check each of the dimensions is in bounds, and work out which
     element we want from the array */
  multiplier = 1; element = 0;
  for( i = 0; i < count; i++ ) {

    size_t offset;

    /* Convert to zero based arrays */
    offset = dimensions[i] - 1;

    if( offset >= array->dimensions[i] ) {
      program->error = BASIC_PROGRAM_ERROR_3;	/* Subscript wrong */
      free( dimensions );
      return BASIC_ERROR_NONE;
    }

    element = multiplier * element + offset;
    multiplier *= array->dimensions[i];
  }

  (*value_ptr) = &( array->values[element] );

  free( dimensions );

  return BASIC_ERROR_NONE;
}

/* Create a new lexical scratchpad
   
   `names' should consist of a list of variable names (either string
   or numeric) and `values' should be a list of expressions to put in
   those names. Ensure that the types match up and that there are (at
   least) as many values as there are names.
*/
error_t
program_push_scratchpad( struct program *program, struct explist *names,
			 struct explist *values )
{
  struct program_symbol_table *pad;
  GSList *name_list, *value_list;
  struct expression *name, *value;
  struct numeric_variable *numeric;
  char *key;
  error_t error;

  /* Start setting up the new context */
  pad = malloc( sizeof *pad );
  if( pad == NULL ) return BASIC_ERROR_MEMORY;

  error = initialise_symbol_table( pad );
  if( error != BASIC_ERROR_NONE ) { free( pad ); return error; }

  /* Now populate the scratchpad with the name-variable pairs */
  name_list  = names->items;
  value_list = values->items;

  while( name_list ) {

    name  = name_list->data;
    value = value_list->data;

    switch( name->type ) {

    case EXPRESSION_NUMEXP:
      numeric = malloc( sizeof *numeric );
      if( numeric == NULL ) {
	destroy_symbol_table( pad ); free( pad );
	return BASIC_ERROR_MEMORY;
      }

      key = strdup( name->types.numexp->types.name );
      if( key == NULL ) {
	free( numeric );
	destroy_symbol_table( pad ); free( pad );
	return BASIC_ERROR_MEMORY;
      }
		
      error = numexp_eval( program, value->types.numexp, &(numeric->value) );
      if( error || program->error ) {
	free( numeric );
	destroy_symbol_table( pad ); free( pad );
	return error;
      }

      g_hash_table_insert( pad->numeric, key, numeric );
      break;

      /* FIXME: string variables */

    default:
      fprintf( stderr, "Unknown expression type %d encountered\n",
	       name->type );
      destroy_symbol_table( pad ); free( pad );
      return BASIC_ERROR_UNKNOWN;
    }

    name_list  = name_list->next;
    value_list = value_list->next;
  }

  /* Now change context by putting the new scratchpad on the context
     stack */
  program->scratchpads = g_slist_prepend( program->scratchpads, pad );

  return BASIC_ERROR_NONE;
}

error_t
program_pop_scratchpad( struct program *program )
{
  error_t error;

  /* Free the memory used by the scratchpad */
  error = destroy_symbol_table( program->scratchpads->data );
  if( error ) return error;

  /* And then that of the scratchpad itself */
  free( program->scratchpads->data );

  /* Finally, remove the pad from the scratchpads list */
  program->scratchpads = g_slist_remove( program->scratchpads,
					 program->scratchpads->data );

  return BASIC_ERROR_NONE;
}

/* Clear all variables from the symbol table */
error_t
program_clear_variables( struct program *program )
{
  error_t error;

  error = clear_symbol_table( &(program->variables) );
  if( error ) return error;

  return BASIC_ERROR_NONE;
}

/* Initialise a symbol table */
static error_t
initialise_symbol_table( struct program_symbol_table *table )
{
  table->numeric       = g_hash_table_new( g_str_hash, g_str_equal );
  table->string        = g_hash_table_new( g_str_hash, g_str_equal );
  table->numeric_array = g_hash_table_new( g_str_hash, g_str_equal );

  return BASIC_ERROR_NONE;
}

static error_t
destroy_symbol_table( struct program_symbol_table *table )
{
  error_t error;

  error = clear_symbol_table( table );
  if( error ) return error;

  g_hash_table_destroy( table->numeric );
  g_hash_table_destroy( table->string );
  g_hash_table_destroy( table->numeric_array );

  return BASIC_ERROR_NONE;
}

/* Clear the memory used by one symbol table */
static error_t
clear_symbol_table( struct program_symbol_table *table )
{
  g_hash_table_foreach_remove( table->numeric, free_numeric_var, NULL );
  g_hash_table_foreach_remove( table->string,  free_string_var, NULL );
  g_hash_table_foreach_remove( table->numeric_array, free_numeric_array,
			       NULL );
  return BASIC_ERROR_NONE;
}

/* Free the memory used by one variable */
static gboolean
free_numeric_var( gpointer key, gpointer value, gpointer user_data )
{
  free( key ); free( value );
  return TRUE;
}

static gboolean
free_string_var( gpointer key, gpointer value, gpointer user_data )
{
  struct string *string = value;

  free( key ); 
  string_destroy( &string );
  return TRUE;
}

static gboolean
free_numeric_array( gpointer key, gpointer value, gpointer user_data )
{
  struct numeric_array *array;

  free( key );

  array = value;
  free( array->dimensions ); free( array->values );
  free( array );

  return TRUE;
}

/* Jump to a specified line */
error_t
program_set_line( struct program *program, float line )
{
  int line_number;
  struct line *current_line;

  line_number = line;

  if( line < 0 || line >= 10000 ) {
    program->error = BASIC_PROGRAM_ERROR_B;
    return BASIC_ERROR_NONE;
  }

  /* Get the first line at or greater than the given line */
  program->current_line =
    g_slist_find_custom( program->lines, GINT_TO_POINTER( line_number ),
			 find_line );

  /* If that was off the end of the program, return successfully */
  if( program->current_line == NULL ) {
    program->error = BASIC_PROGRAM_ERROR_0;
    return BASIC_ERROR_NONE;
  }

  /* And we're at the first statement on this line */
  current_line = program->current_line->data;
  program->current_statement = current_line->statements;

  return BASIC_ERROR_NONE;
}

static gint
find_line( gconstpointer from_list, gconstpointer from_caller )
{
  const struct line *line = from_list;
  int line_number = GPOINTER_TO_INT( from_caller );

  if( line->line_number >= line_number ) return 0;

  return 1;
}

/* Go to a particular line, first remembering where we should return to */
error_t
program_gosub( struct program *program, float line )
{
  struct program_position return_to;
  error_t error;

  return_to.line = program->current_line;
  return_to.statement = program->current_statement;

  g_array_append_val( program->gosub_stack, return_to );

  error = program_set_line( program, line );
  if( error || program->error ) return error;

  return BASIC_ERROR_NONE;
}

/* Return from a GO SUB call */
error_t
program_gosub_return( struct program *program )
{
  struct program_position return_to;

  if( !program->gosub_stack->len ) {
    program->error = BASIC_PROGRAM_ERROR_7;
    return BASIC_ERROR_NONE;
  }

  return_to = g_array_index( program->gosub_stack, struct program_position,
			     program->gosub_stack->len - 1 );
  g_array_remove_index_fast( program->gosub_stack,
			     program->gosub_stack->len - 1 );

  program->current_line = return_to.line;
  program->current_statement = return_to.statement;

  return BASIC_ERROR_NONE;
}

/* Set up a FOR variable */
error_t
program_initialise_for_loop( struct program *program, const char *control,
			     float start, float end, float step )
{
  struct numeric_variable *value; char *key;

  /* See if this variable exists already */
  value = g_hash_table_lookup( program->variables.numeric, control );

  /* If it doesn't, allocate memory for the new key and value */
  if( !value ) {
    key = strdup( control ); if( key == NULL ) return BASIC_ERROR_MEMORY;
    value = malloc( sizeof *value );
    if( value == NULL ) { free( key ); return BASIC_ERROR_MEMORY; }

    /* And put the new variable in */
    g_hash_table_insert( program->variables.numeric, key, value );
  }

  value->value = start;
  value->magic = PROGRAM_VARIABLE_MAGIC_FOR;

  value->end = end; value->step = step;
  value->loop.line = program->current_line;
  value->loop.statement = program->current_statement;

  /* Zero iteration loops skip immediately to the 'closing' NEXT statement */
  if( ( value->step >= 0 && value->value > value->end ) ||
      ( value->step <  0 && value->value < value->end )    )
    return find_closing_next( program, control );

  return BASIC_ERROR_NONE;
}

static error_t
find_closing_next( struct program *program, const char *control )
{
  GSList *lines, *statements;

  struct line *line;
  struct statement *statement;

  lines = program->current_line;
  statements = program->current_statement;

  while( 1 ) {

    while( statements ) {

      statement = statements->data;

      if( check_for_next( statement, control ) ) {
	program->current_line = lines;
	program->current_statement = statements->next;
	return BASIC_ERROR_NONE;
      }
      
      statements = statements->next;

    }

    lines = lines->next; if( !lines ) break;

    line = lines->data;
    statements = line->statements;
  }

  program->error = BASIC_PROGRAM_ERROR_I;	/* FOR without NEXT */
  return BASIC_ERROR_NONE;
}

static error_t
check_for_next( struct statement *statement, const char *control )
{
  switch( statement->type ) {

  case STATEMENT_NEXT_ID:
    return !strcmp( control, statement->types.next );

  case STATEMENT_IF_ID:
    return check_for_next( statement->types.if_statement.true_clause,
			   control );

  default:
    return 0;
  }
}

error_t
program_for_loop_next( struct program *program, const char *control )
{
  struct numeric_variable *value;

  /* First see if this variable exists at all */
  value = g_hash_table_lookup( program->variables.numeric, control );
  /* If not, return with error */
  if( !value ) {
    program->error = BASIC_PROGRAM_ERROR_2;
    return BASIC_ERROR_NONE;
  }

  /* Now see if this is a FOR variable; if not, return with error */
  if( !( value->magic & PROGRAM_VARIABLE_MAGIC_FOR ) ) {
    program->error = BASIC_PROGRAM_ERROR_1;
    return BASIC_ERROR_NONE;
  }

  /* Increment the counter */
  value->value += value->step;

  /* Now see if we want to loop */
  if( ( value->step >  0 && value->value <= value->end ) ||
      ( value->step <= 0 && value->value >= value->end )    ) {
    program->current_line = value->loop.line;
    program->current_statement = value->loop.statement;
  }

  return BASIC_ERROR_NONE;
}

const char*
program_strerror( enum program_errors program_error )
{
  switch( program_error ) {

  /* FIXME: what to do here? */
  case BASIC_PROGRAM_ERROR_NONE: return "(Program not finished)";

  case BASIC_PROGRAM_ERROR_0: return "0 OK";
  case BASIC_PROGRAM_ERROR_1: return "1 NEXT without FOR";
  case BASIC_PROGRAM_ERROR_2: return "2 Variable not found";
  case BASIC_PROGRAM_ERROR_3: return "3 Subscript wrong";
  case BASIC_PROGRAM_ERROR_7: return "7 RETURN without GOSUB";
  case BASIC_PROGRAM_ERROR_9: return "9 Stop statement";
  case BASIC_PROGRAM_ERROR_A: return "A Invalid argument";
  case BASIC_PROGRAM_ERROR_B: return "B Integer out of range";
  case BASIC_PROGRAM_ERROR_C: return "C Nonsense in BASIC";
  case BASIC_PROGRAM_ERROR_I: return "I FOR without NEXT";
  case BASIC_PROGRAM_ERROR_P: return "P FN without DEF";
  case BASIC_PROGRAM_ERROR_Q: return "Q Parameter error";

  }

  /* FIXME: what to do here? */
  return "(Unknown error)";
}

error_t
program_free( struct program *program )
{
  free_lines( program->lines );

  destroy_symbol_table( &( program->variables ) );
  while( program->scratchpads ) program_pop_scratchpad( program );

  g_array_free( program->gosub_stack, TRUE );
  free_functions( program->functions );

  return BASIC_ERROR_NONE;
}
