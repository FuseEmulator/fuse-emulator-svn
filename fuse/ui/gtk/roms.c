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
#include "menu.h"
#include "settings.h"
#include "ui/ui.h"

static void add_rom( GtkBox *parent, size_t start, gint row );
static void select_new_rom( GtkWidget *widget, gpointer data );
static void roms_done( GtkButton *button, gpointer data );

/* The labels used to display the current ROMs */
GtkWidget *rom[ SETTINGS_ROM_COUNT ];

struct callback_info {

  size_t start, n;

};

int
menu_select_roms( libspectrum_machine machine, size_t start, size_t n )
{
  GtkWidget *dialog;
  GtkBox *vbox;

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

  gtk_signal_connect( GTK_OBJECT( dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  info.start = start;
  info.n = n;

  /* Create the OK and Cancel buttons */
  gtkstock_create_ok_cancel( dialog, NULL, GTK_SIGNAL_FUNC( roms_done ), &info,
			     NULL );

  /* And the current values of each of the ROMs */
  vbox = GTK_BOX( GTK_DIALOG( dialog )->vbox );

  gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );
  for( i = 0; i < n; i++ ) add_rom( vbox, start, i );

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
add_rom( GtkBox *parent, size_t start, gint row )
{
  GtkWidget *frame, *hbox, *change_button;
  char buffer[ 80 ], **setting;

  snprintf( buffer, 80, "ROM %d", row );
  frame = gtk_frame_new( buffer );
  gtk_box_pack_start( parent, frame, FALSE, FALSE, 2 );

  hbox = gtk_hbox_new( FALSE, 4 );
  gtk_container_set_border_width( GTK_CONTAINER( hbox ), 4 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  setting = settings_get_rom_setting( &settings_current, start + row );
  rom[ row ] = gtk_entry_new();
  gtk_entry_set_text( GTK_ENTRY( rom[ row ] ), *setting );
  gtk_box_pack_start( GTK_BOX( hbox ), rom[ row ], FALSE, FALSE, 2 );

  change_button = gtk_button_new_with_label( "Select..." );
  gtk_signal_connect( GTK_OBJECT( change_button ), "clicked",
		      GTK_SIGNAL_FUNC( select_new_rom ),
		      rom[ row ] );
  gtk_box_pack_start( GTK_BOX( hbox ), change_button, FALSE, FALSE, 2 );
}

static void
select_new_rom( GtkWidget *widget GCC_UNUSED, gpointer data )
{
  char *filename;

  GtkWidget *entry = data;

  filename = menu_get_filename( "Fuse - Select ROM" );
  if( !filename ) return;

  gtk_entry_set_text( GTK_ENTRY( entry ), filename );
}

static void
roms_done( GtkButton *button GCC_UNUSED, gpointer data )
{
  size_t i;
  int error;
  
  char **setting; const char *string;

  struct callback_info *info = data;

  for( i = 0; i < info->n; i++ ) {

    setting = settings_get_rom_setting( &settings_current, info->start + i );
    string = gtk_entry_get_text( GTK_ENTRY( rom[i] ) );

    error = settings_set_string( setting, string ); if( error ) return;

  }
}

#endif			/* #ifdef UI_GTK */
