/* widget.h: Simple dialog boxes for all user interfaces.
   Copyright (c) 2001-2004 Matan Ziv-Av, Philip Kendall

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

#ifndef FUSE_WIDGET_H
#define FUSE_WIDGET_H

#include "input.h"
#include "ui/scaler/scaler.h"
#include "ui/ui.h"

/* How many levels deep have we recursed through widgets; -1 => none */
extern int widget_level;

/* Code called at start and end of emulation */
int widget_init( void );
int widget_end( void );

/* Look for widget keyboard shortcuts */
void widget_shortcuts( input_key key );

/* Activate a widget */
int widget_do( int which, void *data );

/* Finish with widgets for now */
void widget_finish( void );

/* A function to handle keypresses */
typedef void (*widget_keyhandler_fn)( input_key key );

/* The current widget keyhandler */
extern widget_keyhandler_fn widget_keyhandler;

/* Widget-specific bits */

/* Menu widget */

/* A generic callback function */
typedef void (*widget_menu_callback_fn)( int action );

/* A general menu */
typedef struct widget_menu_entry {
  const char *text;		/* Menu entry text */
  input_key key;		/* Which key to activate this widget */

  struct widget_menu_entry *submenu;
  widget_menu_callback_fn callback;

  int action;
  int inactive;

} widget_menu_entry;

/* The main menu as activated with F1 */
extern widget_menu_entry widget_menu[];

/* The name returned from the file selector */
extern char* widget_filesel_name;

/* Select a machine */
int widget_select_machine( void *data );
     
/* The error widget data type */

typedef struct widget_error_t {
  ui_error_level severity;
  const char *message;
} widget_error_t;

#endif				/* #ifndef FUSE_WIDGET_H */
