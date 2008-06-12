/* variable.c: Debugger variables
   Copyright (c) 2008 Philip Kendall

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

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

#include "debugger_internals.h"
#include "ui/ui.h"

static GHashTable *debugger_variables;

int
debugger_variable_init( void )
{
  debugger_variables = g_hash_table_new( g_str_hash, g_str_equal );
  if( !debugger_variables ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  return 0;
}

void
debugger_variable_set( const char *name, libspectrum_dword value )
{
  /* Check if we need to allocate memory for this key */
  if( !g_hash_table_lookup( debugger_variables, name ) ) {
    name = strdup( name );
    if( !name ) {
      ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
      return;
    }
  }

  /* Cast is safe as we have either taken a copy of the key, or know that
     it already exists so we're not going to use it */
  g_hash_table_insert( debugger_variables, (char*)name, GINT_TO_POINTER(value) );
}

libspectrum_dword
debugger_variable_get( const char *name )
{
  gpointer v = g_hash_table_lookup( debugger_variables, name );

  return v ? GPOINTER_TO_INT(v) : 0;
}