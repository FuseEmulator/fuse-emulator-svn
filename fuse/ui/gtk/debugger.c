/* debugger.c: the GTK+ debugger
   Copyright (c) 2002 Philip Kendall

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

#include "debugger/debugger.h"
#include "fuse.h"
#include "gtkui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

static void gtkui_debugger_done_step( GtkWidget *widget,
				      gpointer user_data GCC_UNUSED );
static void gtkui_debugger_done_continue( GtkWidget *widget,
					  gpointer user_data GCC_UNUSED );

int
ui_debugger_activate( void )
{
  GtkWidget *dialog, *label, *step_button, *continue_button;
  char buffer[80];

  fuse_emulation_pause();

  dialog = gtk_dialog_new();

  step_button = gtk_button_new_with_label( "Single Step" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     step_button );

  continue_button = gtk_button_new_with_label( "Continue" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     continue_button );

  snprintf( buffer, 80, "PC = 0x%04x", PC );
  label = gtk_label_new( buffer );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog )->vbox ), label );

  gtk_signal_connect_object( GTK_OBJECT( step_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_debugger_done_step ),
			     GTK_OBJECT( dialog ) );
  gtk_signal_connect_object( GTK_OBJECT( continue_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_debugger_done_continue ),
			     GTK_OBJECT( dialog ) );

  gtk_signal_connect( GTK_OBJECT( dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
		      (gpointer) NULL );

  /* Esc `cancels' the selector by just continuing with emulation */
  gtk_widget_add_accelerator( continue_button, "clicked",
                              gtk_accel_group_get_default(),
                              GDK_Escape, 0, 0 );

  gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_widget_show_all( dialog );

  gtk_main();

  fuse_emulation_unpause();

  return 0;
}

static void
gtkui_debugger_done_step( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  debugger_mode = DEBUGGER_MODE_STEP;

  gtk_widget_destroy( widget );
  gtk_main_quit();
}

static void
gtkui_debugger_done_continue( GtkWidget *widget,
			      gpointer user_data GCC_UNUSED )
{
  debugger_mode = 
    debugger_breakpoint == DEBUGGER_BREAKPOINT_UNSET ?
    DEBUGGER_MODE_INACTIVE			     :
    DEBUGGER_MODE_ACTIVE;

  gtk_widget_destroy( widget );
  gtk_main_quit();
}
