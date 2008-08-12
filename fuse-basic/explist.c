/* explist.c: a list of expressions
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

#include <stdio.h>
#include <stdlib.h>

#include "explist.h"

static void free_item( gpointer data, gpointer user_data );

error_t
expression_new_numexp( struct expression **expression,
		       struct numexp *numexp )
{
  *expression = malloc( sizeof **expression );
  if( !(*expression) ) return BASIC_ERROR_MEMORY;

  (*expression)->type = EXPRESSION_NUMEXP;
  (*expression)->types.numexp = numexp;

  return BASIC_ERROR_NONE;
}

error_t
expression_new_strexp( struct expression **expression,
		       struct strexp *strexp )
{
  *expression = malloc( sizeof **expression );
  if( !(*expression) ) return BASIC_ERROR_MEMORY;

  (*expression)->type = EXPRESSION_STREXP;
  (*expression)->types.strexp = strexp;

  return BASIC_ERROR_NONE;
}

error_t explist_new( struct explist **list )
{
  *list = malloc( sizeof **list );
  if( *list == NULL ) return BASIC_ERROR_MEMORY;

  (*list)->items = NULL;

  return BASIC_ERROR_NONE;
}

error_t
explist_append_numexp( struct explist *list, struct numexp *numexp )
{
  struct expression *exp;

  exp = malloc( sizeof *exp );
  if( exp == NULL ) return BASIC_ERROR_MEMORY;

  exp->type = EXPRESSION_NUMEXP;
  exp->types.numexp = numexp;

  list->items = g_slist_append( list->items, (gpointer)exp );
  return BASIC_ERROR_NONE;
}

error_t
explist_append_strexp( struct explist *list, struct strexp *strexp )
{
  struct expression *exp;

  exp = malloc( sizeof *exp );
  if( exp == NULL ) return BASIC_ERROR_MEMORY;

  exp->type = EXPRESSION_STREXP;
  exp->types.strexp = strexp;

  list->items = g_slist_append( list->items, (gpointer)exp );
  return BASIC_ERROR_NONE;
}

error_t
explist_append_forvar( struct explist *list, const char *name )
{
  struct expression *exp; error_t error;

  exp = malloc( sizeof *exp );
  if( exp == NULL ) return BASIC_ERROR_MEMORY;

  exp->type = EXPRESSION_NUMEXP;
  error = numexp_new_variable( &(exp->types.numexp), name );
  if( error ) { free( exp ); return error; }

  list->items = g_slist_append( list->items, (gpointer)exp );
  return BASIC_ERROR_NONE;
}

error_t
explist_append_strvar( struct explist *list, const char *name )
{
  struct expression *exp; error_t error;

  exp = malloc( sizeof *exp );
  if( exp == NULL ) return BASIC_ERROR_MEMORY;

  exp->type = EXPRESSION_STREXP;
  error = strexp_new_variable( &(exp->types.strexp), name );
  if( error ) { free( exp ); return error; }

  list->items = g_slist_append( list->items, (gpointer)exp );
  return BASIC_ERROR_NONE;
}

/* Convert a list of numeric expressions into an array of integer values */
error_t
explist_to_integer_array( struct program *program, struct explist *list,
			  size_t **values, size_t *length )
{
  GSList *items;
  size_t *ptr;
  float value;
  error_t error;

  items = list->items;

  *length = g_slist_length( items );

  *values = malloc( *length * sizeof **values );
  if( !*values ) return BASIC_ERROR_MEMORY;

  ptr = *values;

  while( items ) {

    struct expression *exp;

    exp = items->data;

    if( exp->type != EXPRESSION_NUMEXP ) {
      free( *values );
      fprintf( stderr, "Internal error: non-numeric expression found at %s:%d",
	       __FILE__, __LINE__ );
      return BASIC_ERROR_LOGIC;
    }

    error = numexp_eval( program, exp->types.numexp, &value );
    if( error ) { free( *values ); return error; }

    *ptr = value + 0.5;

    items = items->next;
    ptr++;

  }

  return BASIC_ERROR_NONE;
}

error_t
explist_free( struct explist *explist )
{
  g_slist_foreach( explist->items, free_item, NULL );
  g_slist_free( explist->items );
  free( explist );

  return BASIC_ERROR_NONE;
}

static void
free_item( gpointer data, gpointer user_data )
{
  struct expression *expression = data;

  switch( expression->type ) {
    
  case EXPRESSION_NUMEXP: numexp_free( expression->types.numexp ); break;
  case EXPRESSION_STREXP: strexp_free( expression->types.strexp ); break;

  default:
    fprintf( stderr, "Attempt to free unknown expression type %d\n",
	     expression->type );
    break;
  }

  free( expression );
}
