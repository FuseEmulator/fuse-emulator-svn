/* pokefinder.c: GTK+ interface to the poke finder
   Copyright (c) 2003-2004 Philip Kendall

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

#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "debugger/debugger.h"
#include "gtkinternals.h"
#include "pokefinder/pokefinder.h"
#include "ui/ui.h"

static int create_dialog( void );
static void gtkui_pokefinder_incremented( GtkWidget *widget,
					  gpointer user_data GCC_UNUSED );
static void gtkui_pokefinder_decremented( GtkWidget *widget,
					  gpointer user_data GCC_UNUSED );
static void gtkui_pokefinder_search( GtkWidget *widget, gpointer user_data );
static void gtkui_pokefinder_reset( GtkWidget *widget, gpointer user_data );
static void gtkui_pokefinder_close( GtkWidget *widget, gpointer user_data );
static gboolean delete_dialog( GtkWidget *widget, GdkEvent *event,
			       gpointer user_data );
static void update_pokefinder( void );
static void possible_click( GtkCList *clist, gint row, gint column,
			    GdkEventButton *event, gpointer user_data );

static int dialog_created = 0;

static GtkWidget
  *dialog,			/* The dialog box itself */
  *count_label,			/* The number of possible locations */
  *location_list;		/* The list of possible locations */

/* The possible locations */

#define MAX_POSSIBLE 20

int possible_page[ MAX_POSSIBLE ];
libspectrum_word possible_offset[ MAX_POSSIBLE ];

void
menu_machine_pokefinder( GtkWidget *widget GCC_UNUSED,
			 gpointer data GCC_UNUSED )
{
  int error;

  if( !dialog_created ) {
    error = create_dialog(); if( error ) return;
  }

  gtk_widget_show_all( dialog );
  update_pokefinder();
}

static int
create_dialog( void )
{
  GtkWidget *hbox, *vbox, *label, *entry, *button;
  GtkAccelGroup *accel_group;
  size_t i;

  gchar *location_titles[] = { "Page", "Offset" };

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Poke Finder" );

  /* Keyboard shortcuts */
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accel_group );

  hbox = gtk_hbox_new( FALSE, 0 );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox );

  label = gtk_label_new( "Search for:" );
  gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 5 );

  entry = gtk_entry_new();
  gtk_signal_connect( GTK_OBJECT( entry ), "activate",
		      GTK_SIGNAL_FUNC( gtkui_pokefinder_search ), NULL );
  gtk_box_pack_start( GTK_BOX( hbox ), entry, TRUE, TRUE, 5 );

  vbox = gtk_vbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 5 );

  count_label = gtk_label_new( "" );
  gtk_box_pack_start( GTK_BOX( vbox ), count_label, TRUE, TRUE, 5 );

  location_list = gtk_clist_new_with_titles( 2, location_titles );
  gtk_clist_column_titles_passive( GTK_CLIST( location_list ) );
  for( i = 0; i < 2; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( location_list ), i, TRUE );
  gtk_box_pack_start( GTK_BOX( vbox ), location_list, TRUE, TRUE, 5 );

  gtk_signal_connect( GTK_OBJECT( location_list ), "select-row",
		      GTK_SIGNAL_FUNC( possible_click ), NULL );

  button = gtk_button_new_with_label( "Incremented" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_pokefinder_incremented ), NULL );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );

  button = gtk_button_new_with_label( "Decremented" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_pokefinder_decremented ), NULL );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );

  button = gtk_button_new_with_label( "Search" );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_pokefinder_search ),
		      GTK_OBJECT( entry ) );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_widget_add_accelerator( button, "clicked", accel_group, GDK_Return,
			      0, 0 );

  button = gtk_button_new_with_label( "Reset" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_pokefinder_reset ), NULL );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );

  button = gtk_button_new_with_label( "Close" );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_pokefinder_close ),
			     GTK_OBJECT( dialog ) );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_widget_add_accelerator( button, "clicked", accel_group, GDK_Escape,
			      0, 0 );

  /* Users shouldn't be able to resize this window */
  gtk_window_set_policy( GTK_WINDOW( dialog ), FALSE, FALSE, TRUE );

  gtk_signal_connect( GTK_OBJECT( dialog ), "delete-event",
		      GTK_SIGNAL_FUNC( delete_dialog ), NULL );

  dialog_created = 1;

  return 0;
}

static void
gtkui_pokefinder_incremented( GtkWidget *widget,
			      gpointer user_data GCC_UNUSED )
{
  pokefinder_incremented();
  update_pokefinder();
}

static void
gtkui_pokefinder_decremented( GtkWidget *widget,
			      gpointer user_data GCC_UNUSED )
{
  pokefinder_decremented();
  update_pokefinder();
}

static void
gtkui_pokefinder_search( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  pokefinder_search( atoi( gtk_entry_get_text( GTK_ENTRY( widget ) ) ) );
  update_pokefinder();
}

static void
gtkui_pokefinder_reset( GtkWidget *widget GCC_UNUSED,
			gpointer user_data GCC_UNUSED)
{
  pokefinder_clear();
  update_pokefinder();
}

static gboolean
delete_dialog( GtkWidget *widget, GdkEvent *event GCC_UNUSED,
	       gpointer user_data )
{
  gtkui_pokefinder_close( widget, user_data );
  return TRUE;
}

static void
gtkui_pokefinder_close( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  gtk_widget_hide_all( widget );
}

static void
update_pokefinder( void )
{
  size_t page, offset;
  gchar buffer[256], *possible_text[2] = { &buffer[0], &buffer[128] };

  gtk_clist_freeze( GTK_CLIST( location_list ) );
  gtk_clist_clear( GTK_CLIST( location_list ) );

  if( pokefinder_count && pokefinder_count <= MAX_POSSIBLE ) {

    size_t which;

    which = 0;

    for( page = 0; page < 8; page++ )
      for( offset = 0; offset < 0x4000; offset++ )
	if( pokefinder_possible[page][offset] != -1 ) {

	  possible_page[ which ] = page;
	  possible_offset[ which ] = offset;
	  which++;
	
	  snprintf( possible_text[0], 128, "%lu", (unsigned long)page );
	  snprintf( possible_text[1], 128, "0x%04X", (unsigned)offset );

	  gtk_clist_append( GTK_CLIST( location_list ), possible_text );
	}

    gtk_widget_show( location_list );

  } else {
    gtk_widget_hide( location_list );
  }

  gtk_clist_thaw( GTK_CLIST( location_list ) );

  snprintf( buffer, 256, "Possible locations: %lu",
	    (unsigned long)pokefinder_count );
  gtk_label_set_text( GTK_LABEL( count_label ), buffer );
}  

static void
possible_click( GtkCList *clist, gint row, gint column,
		GdkEventButton *event, gpointer user_data )
{
  int error;

  /* Ignore events which aren't double-clicks or select-via-keyboard */
  if( event && event->type != GDK_2BUTTON_PRESS ) return;

  error = debugger_breakpoint_add_address(
    DEBUGGER_BREAKPOINT_TYPE_WRITE, possible_page[ row ],
    possible_offset[ row ], 0, DEBUGGER_BREAKPOINT_LIFE_PERMANENT, NULL
  );
  if( error ) return;

  ui_debugger_update();
}
