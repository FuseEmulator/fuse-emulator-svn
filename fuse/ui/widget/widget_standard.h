/* widget_standard.h: Simple keyboard-driven dialog boxes for all user interfaces.
   Copyright (c) 2001-2004 Matan Ziv-Av, Philip Kendall

   $Id: widget.h 3319 2007-11-21 20:53:21Z zubzero $

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

#ifndef FUSE_WIDGET_STANDARD_H
#define FUSE_WIDGET_STANDARD_H

void widget_up_arrow( int x, int y, int colour );
void widget_down_arrow( int x, int y, int colour );
void widget_print_checkbox( int x, int y, int value );

int widget_dialog( int x, int y, int width, int height );
int widget_dialog_with_border( int x, int y, int width, int height );

/* The various widgets which are available */
typedef enum widget_type {

  WIDGET_TYPE_FILESELECTOR,	/* File selector (load) */
  WIDGET_TYPE_FILESELECTOR_SAVE,/* File selector (save) */
  WIDGET_TYPE_GENERAL,		/* General options */
  WIDGET_TYPE_PICTURE,		/* Keyboard picture */
  WIDGET_TYPE_MENU,		/* General menu */
  WIDGET_TYPE_SELECT,		/* Select machine */
  WIDGET_TYPE_SOUND,		/* Sound options */
  WIDGET_TYPE_ERROR,		/* Error report */
  WIDGET_TYPE_RZX,		/* RZX options */
  WIDGET_TYPE_BROWSE,		/* Browse tape */
  WIDGET_TYPE_TEXT,		/* Text entry widget */
  WIDGET_TYPE_DEBUGGER,		/* Debugger widget */
  WIDGET_TYPE_POKEFINDER,	/* Poke finder widget */
  WIDGET_TYPE_MEMORYBROWSER,	/* Memory browser widget */
  WIDGET_TYPE_ROM,		/* ROM selector widget */
  WIDGET_TYPE_PERIPHERALS,	/* Peripherals options */
  WIDGET_TYPE_QUERY,		/* Query (yes/no) */
  WIDGET_TYPE_QUERY_SAVE,	/* Query (save/don't save/cancel) */

} widget_type;

#endif				/* #ifndef FUSE_WIDGET_STANDARD_H */
