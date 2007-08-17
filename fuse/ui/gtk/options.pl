#!/usr/bin/perl -w

# options.pl: generate options dialog boxes
# $Id$

# Copyright (c) 2002-2004 Philip Kendall

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Author contact information:

# E-mail: philip-fuse@shadowmagic.org.uk

use strict;

use Fuse;
use Fuse::Dialog;

die "No data file specified" unless @ARGV;

my @dialogs = Fuse::Dialog::read( shift @ARGV );

print Fuse::GPL( 'options.c: options dialog boxes',
		 '2001-2004 Philip Kendall' ) . << "CODE";

/* This file is autogenerated from options.dat by options.pl.
   Do not edit unless you know what you\'re doing! */

#include <config.h>

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "../../compat.h"
#include "../../display.h"
#include "../../fuse.h"
#include "gtkinternals.h"
#include "options.h"
#include "../../periph.h"
#include "../../settings.h"
CODE

foreach( @dialogs ) {

    print << "CODE";

static void menu_options_$_->{name}_done( GtkWidget *widget,
					  gpointer user_data );

void
menu_options_$_->{name}( GtkWidget *widget GCC_UNUSED,
			 gpointer data GCC_UNUSED )
{
  menu_options_$_->{name}_t dialog;
  GtkWidget *frame, *hbox, *text;
  gchar buffer[80];

  frame = hbox = text = NULL;
  buffer[0] = '\\0';		/* Shut gcc up */
  
  /* Firstly, stop emulation */
  fuse_emulation_pause();

  /* Create the necessary widgets */
  dialog.dialog = gtkstock_dialog_new( "Fuse - $_->{title}", NULL );

  /* Create the various widgets */
CODE

    foreach my $widget ( @{ $_->{widgets} } ) {

	foreach my $type ( $widget->{type} ) {

	    my $text = $widget->{text}; $text =~ tr/()//d;

	    if( $type eq "Checkbox" ) {

		print << "CODE";
  dialog.$widget->{value} =
    gtk_check_button_new_with_label( "$text" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.$widget->{value} ),
				settings_current.$widget->{value} );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog.dialog )->vbox ),
		     dialog.$widget->{value} );

CODE
            } elsif( $type eq "Entry" ) {

                # FIXME: Make the entry widget resize sensibly

		print << "CODE";
  frame = gtk_frame_new( "$text" );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog.dialog )->vbox ),
			       frame );
				    
  hbox = gtk_hbox_new( FALSE, 0 );
  gtk_container_set_border_width( GTK_CONTAINER( hbox ), 4 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  dialog.$widget->{value} = gtk_entry_new();
  gtk_entry_set_max_length( GTK_ENTRY( dialog.$widget->{value} ),
	   		    $widget->{data1} );
  snprintf( buffer, 80, "%d", settings_current.$widget->{value} );
  gtk_entry_set_text( GTK_ENTRY( dialog.$widget->{value} ), buffer );
  gtk_box_pack_start_defaults( GTK_BOX( hbox ), dialog.$widget->{value} );

  text = gtk_label_new( "$widget->{data2}" );
  gtk_box_pack_start( GTK_BOX( hbox ), text, FALSE, FALSE, 5 );

CODE
	    } else {
		die "Unknown type `$type'";
	    }
	}
    }

    print << "CODE";
  /* Create the OK and Cancel buttons */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
			     GTK_SIGNAL_FUNC( menu_options_$_->{name}_done ),
			     (gpointer) &dialog, NULL );

  /* Display the window */
  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();
}

static void
menu_options_$_->{name}_done( GtkWidget *widget GCC_UNUSED,
			      gpointer user_data )
{
  menu_options_$_->{name}_t *ptr = user_data;

CODE

    foreach my $widget ( @{ $_->{widgets} } ) {

	if( $widget->{type} eq "Checkbox" ) {

	    print << "CODE";
  settings_current.$widget->{value} =
    gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( ptr->$widget->{value} ) );

CODE
        } elsif( $widget->{type} eq "Entry" ) {

	    print << "CODE";
  settings_current.$widget->{value} =
    atoi( gtk_entry_get_text( GTK_ENTRY( ptr->$widget->{value} ) ) );

CODE
    	} else {
	    die "Unknown type `$widget->{type}'";
	}
    }

    print << "CODE";
  gtk_widget_destroy( ptr->dialog );

CODE

    print "  $_->{posthook}();\n\n" if $_->{posthook};

    print << "CODE";
  gtkstatusbar_set_visibility( settings_current.statusbar );
  display_refresh_all();

  gtk_main_quit();
}
CODE

}

print << "CODE"

#endif			/* #ifdef UI_GTK */
CODE
