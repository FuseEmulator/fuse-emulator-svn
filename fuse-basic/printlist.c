/* printlist.c: a list of items to a PRINT command
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

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "basic.h"
#include "numexp.h"
#include "strexp.h"
#include "printlist.h"
#include "token.h"

static void free_item( gpointer data, gpointer user_data );

error_t
printitem_new_numexp( struct printitem **item, struct numexp *numexp )
{
  *item = malloc( sizeof **item );
  if( *item == NULL ) return BASIC_ERROR_MEMORY;

  (*item)->type = PRINTITEM_NUMEXP;
  (*item)->types.numexp = numexp;

  return BASIC_ERROR_NONE;
}

error_t
printitem_new_strexp( struct printitem **item, struct strexp *strexp )
{
  *item = malloc( sizeof **item );
  if( *item == NULL ) return BASIC_ERROR_MEMORY;

  (*item)->type = PRINTITEM_STREXP;
  (*item)->types.strexp = strexp;

  return BASIC_ERROR_NONE;
}

error_t
printitem_new_separator( struct printitem **item, enum basic_token separator )
{
  *item = malloc( sizeof **item );
  if( *item == NULL ) return BASIC_ERROR_MEMORY;

  (*item)->type = PRINTITEM_SEPARATOR;
  (*item)->types.separator = separator;

  return BASIC_ERROR_NONE;
}

error_t
printlist_new( struct printlist **list )
{
  *list = malloc( sizeof **list );
  if( *list == NULL ) return BASIC_ERROR_MEMORY;

  (*list)->items = NULL;

  return BASIC_ERROR_NONE;
}

error_t
printlist_append( struct printlist *list, struct printitem *item )
{
  list->items = g_slist_append( list->items, (gpointer)item );
  return BASIC_ERROR_NONE;
}

error_t
printlist_free( struct printlist *printlist )
{
  g_slist_foreach( printlist->items, free_item, NULL );
  g_slist_free( printlist->items );
  free( printlist );

  return BASIC_ERROR_NONE;
}

static void
free_item( gpointer data, gpointer user_data )
{
  struct printitem *printitem = data;

  switch( printitem->type ) {

  case PRINTITEM_NUMEXP: numexp_free( printitem->types.numexp ); break;
  case PRINTITEM_STREXP: strexp_free( printitem->types.strexp ); break;
  case PRINTITEM_SEPARATOR: break;

  default:
    fprintf( stderr, "Attempt to free unknown printitem type %d\n",
	     printitem->type );
    break;
  }

  free( printitem );
}

