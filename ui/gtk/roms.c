/* roms.c: ROM selector dialog box
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
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "settings.h"
#include "ui/ui.h"

static int select_roms( libspectrum_machine machine, size_t start, size_t n );
static void add_rom( GtkWidget *table, size_t start, gint row );
static void select_new_rom( GtkWidget *widget, gpointer data );
static void roms_done( GtkButton *button, gpointer data );

/* The labels used to display the current ROMs */
GtkWidget *rom[ SETTINGS_ROM_COUNT ];

struct callback_info {

  GtkWidget *dialog;
  size_t start, n;

};

void
gtkui_roms( gpointer callback_data, guint callback_action, GtkWidget *widget )
{
  switch( callback_action ) {

  case  1: select_roms( LIBSPECTRUM_MACHINE_16,      0, 1 ); return;
  case  2: select_roms( LIBSPECTRUM_MACHINE_48,      1, 1 ); return;
  case  3: select_roms( LIBSPECTRUM_MACHINE_128,     2, 2 ); return;
  case  4: select_roms( LIBSPECTRUM_MACHINE_PLUS2,   4, 2 ); return;
  case  5: select_roms( LIBSPECTRUM_MACHINE_PLUS2A,  6, 4 ); return;
  case  6: select_roms( LIBSPECTRUM_MACHINE_PLUS3,  10, 4 ); return;
  case  7: select_roms( LIBSPECTRUM_MACHINE_TC2048, 14, 1 ); return;
  case  8: select_roms( LIBSPECTRUM_MACHINE_TC2068, 15, 2 ); return;
  case  9: select_roms( LIBSPECTRUM_MACHINE_PENT,   17, 3 ); return;
  case 10: select_roms( LIBSPECTRUM_MACHINE_SCORP,  20, 4 ); return;

  }

  ui_error( UI_ERROR_ERROR, "gtkui_roms: unknown action %u", callback_action );
  fuse_abort();
}

static int
select_roms( libspectrum_machine machine, size_t start, size_t n )
{
  GtkWidget *dialog;
  GtkAccelGroup *accel_group;
  GtkWidget *table;
  GtkWidget *ok_button, *cancel_button;

  struct callback_info info;

  char buffer[ 256 ];
  size_t i;

  /* Firstly, stop emulation */
  fuse_emulation_pause();

  /* Give me a new dialog box */
  dialog = gtk_dialog_new();
  snprintf( buffer, 256, "Fuse - Select ROMs - %s",
	    libspectrum_machine_name( machine ) );
  gtk_window_set_title( GTK_WINDOW( dialog ), buffer );

  /* A table to put all the labels in */
  table = gtk_table_new( n, 3, FALSE );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ),
		     table );

  /* And the current values of each of the ROMs */
  for( i = 0; i < n; i++ ) add_rom( table, start, i );

  /* Create the OK and Cancel buttons */
  ok_button = gtk_button_new_with_label( "OK" );
  cancel_button = gtk_button_new_with_label( "Cancel" );

  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     ok_button );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     cancel_button );
  
  info.dialog = dialog;
  info.start = start;
  info.n = n;
  gtk_signal_connect( GTK_OBJECT( ok_button ), "clicked",
		      GTK_SIGNAL_FUNC( roms_done ), &info );

  gtk_signal_connect_object( GTK_OBJECT( cancel_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accel_group );

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator( cancel_button, "clicked",
			      accel_group, GDK_Escape, 0, 0);

  /* Users shouldn't be able to resize this window */
  gtk_window_set_policy( GTK_WINDOW( dialog ), FALSE, FALSE, TRUE );

  /* Set the window to be modal and display it */
  gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_widget_show_all( dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return 0;
}

static void
add_rom( GtkWidget *table, size_t start, gint row )
{
  GtkWidget *label, *change_button;
  char **setting;

  label = gtk_label_new( settings_rom_name[ start + row ] );
  gtk_table_attach( GTK_TABLE( table ), label, 0, 1, row, row + 1,
		    0, 0, 2, 2 );

  setting = settings_get_rom_setting( &settings_current, start + row );
  rom[ row ] = gtk_label_new( *setting );
  gtk_table_attach( GTK_TABLE( table ), rom[ row ], 1, 2, row, row + 1,
		    0, 0, 2, 2 );

  change_button = gtk_button_new_with_label( "Change" );
  gtk_signal_connect( GTK_OBJECT( change_button ), "clicked",
		      GTK_SIGNAL_FUNC( select_new_rom ),
		      rom[ row ] );
  gtk_table_attach( GTK_TABLE( table ), change_button, 2, 3, row, row + 1,
		    0, 0, 2, 2 );
}

static void
select_new_rom( GtkWidget *widget GCC_UNUSED, gpointer data )
{
  char *filename;

  GtkWidget *label = data;

  filename = gtkui_fileselector_get_filename( "Fuse - Select ROM" );
  if( !filename ) return;

  gtk_label_set( GTK_LABEL( label ), filename );
}

static void
roms_done( GtkButton *button GCC_UNUSED, gpointer data )
{
  size_t i;
  int error;
  
  char *string, **setting;

  struct callback_info *info = data;

  for( i = 0; i < info->n; i++ ) {

    setting = settings_get_rom_setting( &settings_current, info->start + i );
    gtk_label_get( GTK_LABEL( rom[i] ), &string );

    error = settings_set_string( setting, string ); if( error ) return;

  }

  gtkui_destroy_widget_and_quit( info->dialog, NULL );
}

#endif			/* #ifdef UI_GTK */
