/* picture.c: GTK+ routines to draw the keyboard picture
   Copyright (c) 2002-2005 Philip Kendall

   $Id$

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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libspectrum.h>

#include "display.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "ui/ui.h"
#include "utils.h"

/* An RGB image of the keyboard picture */
static guchar picture[ DISPLAY_SCREEN_HEIGHT * DISPLAY_ASPECT_WIDTH * 4 ];
static const gint picture_pitch = DISPLAY_ASPECT_WIDTH * 4;

static int dialog_created = 0;

static int read_screen( const char *filename, utils_file *screen );
static void draw_screen( libspectrum_byte *screen, int border );
static gint
picture_expose( GtkWidget *widget, GdkEvent *event, gpointer data );

static GtkWidget *dialog;

int
gtkui_picture( const char *filename, int border )
{
  utils_file screen;

  GtkWidget *drawing_area;

  if( !dialog_created ) {

    if( read_screen( filename, &screen ) ) {
      return 1;
    }

    draw_screen( screen.buffer, border );

    if( utils_close_file( &screen ) ) {
      return 1;
    }

    dialog = gtkstock_dialog_new( "Fuse - Keyboard",
				  GTK_SIGNAL_FUNC( gtk_widget_hide ) );
  
    drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_size( GTK_DRAWING_AREA( drawing_area ),
			   DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT );
    gtk_signal_connect( GTK_OBJECT( drawing_area ),
			"expose_event", GTK_SIGNAL_FUNC( picture_expose ),
			NULL );
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ),
		       drawing_area );

    gtkstock_create_close( dialog, NULL, GTK_SIGNAL_FUNC( gtk_widget_hide ),
			   FALSE );

    /* Stop users resizing this window */
    gtk_window_set_policy( GTK_WINDOW( dialog ), FALSE, FALSE, TRUE );

    dialog_created = 1;
  }

  gtk_widget_show_all( dialog );

  return 0;
}

static int
read_screen( const char *filename, utils_file *screen )
{
  int fd, error;

  fd = utils_find_auxiliary_file( filename, UTILS_AUXILIARY_LIB );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't find keyboard picture ('%s')",
	      filename );
    return 1;
  }
  
  error = utils_read_fd( fd, filename, screen );
  if( error ) return error;

  if( screen->length != 6912 ) {
    utils_close_file( screen );
    ui_error( UI_ERROR_ERROR, "keyboard picture ('%s') is not 6912 bytes long",
	      filename );
    return 1;
  }

  return 0;
}

static void
draw_screen( libspectrum_byte *screen, int border )
{
  int i, x, y, ink, paper;
  libspectrum_byte attr, data; 

  for( y=0; y < DISPLAY_BORDER_HEIGHT; y++ ) {
    for( x=0; x < DISPLAY_ASPECT_WIDTH; x++ ) {
      *(libspectrum_dword*)( picture + y * picture_pitch + 4 * x ) =
	gtkdisplay_colours[border];
      *(libspectrum_dword*)(
          picture +
	  ( y + DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) * picture_pitch +
	  4 * x
	) = gtkdisplay_colours[ border ];
    }
  }

  for( y=0; y<DISPLAY_HEIGHT; y++ ) {

    for( x=0; x < DISPLAY_BORDER_ASPECT_WIDTH; x++ ) {
      *(libspectrum_dword*)
	(picture + ( y + DISPLAY_BORDER_HEIGHT) * picture_pitch + 4 * x) =
	gtkdisplay_colours[ border ];
      *(libspectrum_dword*)(
          picture +
	  ( y + DISPLAY_BORDER_HEIGHT ) * picture_pitch +
	  4 * ( x+DISPLAY_ASPECT_WIDTH-DISPLAY_BORDER_ASPECT_WIDTH )
	) = gtkdisplay_colours[ border ];
    }

    for( x=0; x < DISPLAY_WIDTH_COLS; x++ ) {

      attr = screen[ display_attr_start[y] + x ];

      ink = ( attr & 0x07 ) + ( ( attr & 0x40 ) >> 3 );
      paper = ( attr & ( 0x0f << 3 ) ) >> 3;

      data = screen[ display_line_start[y]+x ];

      for( i=0; i<8; i++ ) {
	*(libspectrum_dword*)(
	    picture +
	    ( y + DISPLAY_BORDER_HEIGHT ) * picture_pitch +
	    4 * ( 8 * x + DISPLAY_BORDER_ASPECT_WIDTH + i )
	  ) = ( data & 0x80 ) ? gtkdisplay_colours[ ink ]
	                      : gtkdisplay_colours[ paper ];
	data <<= 1;
      }
    }

  }
}

static gint
picture_expose( GtkWidget *widget, GdkEvent *event, gpointer data GCC_UNUSED )
{
  int x = event->expose.area.x, y = event->expose.area.y;

  gdk_draw_rgb_32_image( widget->window,
			 widget->style->fg_gc[ GTK_STATE_NORMAL ],
			 x, y,
			 event->expose.area.width, event->expose.area.height,
			 GDK_RGB_DITHER_NONE,
			 picture + y * picture_pitch + 4 * x, picture_pitch );

  return TRUE;
}

#endif			/* #ifdef UI_GTK */
