/* gtkinternals.h: stuff internal to the GTK+ UI
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

#ifndef FUSE_GTKINTERNALS_H
#define FUSE_GTKINTERNALS_H

#include <gtk/gtk.h>
#include <libspectrum.h>

/*
 * Display routines (gtkdisplay.c)
 */

/* The colour palette in use */
extern libspectrum_dword gtkdisplay_colours[ 16 ];

int gtkdisplay_init( void );
int gtkdisplay_end( void );

/*
 * Keyboard routines (gtkkeyboard.c)
 */

int gtkkeyboard_keypress( GtkWidget *widget, GdkEvent *event,
			  gpointer data);
int gtkkeyboard_keyrelease( GtkWidget *widget, GdkEvent *event,
			    gpointer data);
int gtkkeyboard_release_all( GtkWidget *widget, GdkEvent *event,
			     gpointer data );

/*
 * Mouse routines (gtkmouse.c)
 */

gboolean gtkmouse_position( GtkWidget*, GdkEventMotion*, gpointer );
gboolean gtkmouse_button( GtkWidget*, GdkEventButton*, gpointer);

/*
 * General user interface routines (gtkui.c)
 */

extern GtkWidget *gtkui_window;
extern GtkWidget *gtkui_drawing_area;

void gtkui_destroy_widget_and_quit( GtkWidget *widget, gpointer data );

int gtkui_confirm( const char *string );

int gtkui_picture( const char *filename, int border );

extern void gtkui_popup_menu(void);

GtkAccelGroup* gtkstock_add_accel_group( GtkWidget *widget );

/* Set modifier=0 to use the first default accel key.
 * Set modifier_alt=0 to use the second default accel key.
 * For either, GDK_VoidSymbol means "no accel key".
 */
typedef struct gtkstock_button {
  gchar *label;
  GtkSignalFunc action;		/* "clicked" func; data is actiondata. */
  gpointer actiondata;
  GtkSignalFunc destroy;	/* "clicked" func; data is parent widget */
  guint shortcut;
  GdkModifierType modifier;     /* primary shortcut */
  guint shortcut_alt;
  GdkModifierType modifier_alt; /* secondary shortcut */
} gtkstock_button;

/* GTK1: create a simple button with the given label.
 *   "gtk-" prefixes are stripped and are used to select default accel keys.
 * GTK2: chooses between stock and normal based on "gtk-" prefix.
 *
 * Some stock labels are defined below for use with GTK1: their names are the
 * same as in GTK2, but the strings are different.
 *
 * If the target widget is a GtkDialog, then created buttons are put in its
 * action area.
 *
 * If the label begins with "!", then gtk_signal_connect_object, rather than
 * gtk_signal_connect, is used to connect the action function.
 */
GtkWidget* gtkstock_create_button( GtkWidget *widget, GtkAccelGroup *accel,
				   const gtkstock_button *btn );
GtkAccelGroup*
gtkstock_create_buttons( GtkWidget *widget, GtkAccelGroup *accel,
			 const gtkstock_button *buttons, size_t count );
GtkAccelGroup* gtkstock_create_ok_cancel( GtkWidget *widget,
					  GtkAccelGroup *accel,
	/* for OK button -> */	          GtkSignalFunc action,
				          gpointer actiondata,
	/* for both buttons -> */         GtkSignalFunc destroy );
GtkAccelGroup* gtkstock_create_close( GtkWidget *widget, GtkAccelGroup *accel,
				      GtkSignalFunc destroy,
				      gboolean esconly );
	/* destroy==NULL => use DEFAULT_DESTROY */

#define DEFAULT_DESTROY ( GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ) )

#ifndef UI_GTK2
#define GTK_STOCK_OK		"gtk-OK"
       /* gets accel Return */
#define GTK_STOCK_CANCEL	"gtk-Cancel"
       /* gets accel Escape */
#define GTK_STOCK_CLOSE		"gtk-Close"
       /* gets accel Return, Escape */
#define GTK_STOCK_SAVE		"gtk-Save"
       /* gets accel Alt-S, Return */
#define GTK_STOCK_YES		"gtk-Yes"
       /* gets accel Alt-Y, Return */
#define GTK_STOCK_NO		"gtk-No"
       /* gets accel Alt-N, Escape */
#endif			/* #ifndef UI_GTK2 */

GtkWidget *gtkstock_dialog_new( const gchar *title, GtkSignalFunc destroy );

#ifdef UI_GTK2
typedef PangoFontDescription *gtkui_font;
#else				/* #ifdef UI_GTK2 */
typedef GtkStyle *gtkui_font;
#endif				/* #ifdef UI_GTK2 */

int gtkui_get_monospaced_font( gtkui_font *font );
void gtkui_free_font( gtkui_font font );
void gtkui_set_font( GtkWidget *widget, gtkui_font font );

/*
 * The menu data (menu_data.c)
 */

extern GtkItemFactoryEntry gtkui_menu_data[];
extern guint gtkui_menu_data_size;

/*
 * The icon pixmaps (pixmaps.c)
 */
extern char *gtkpixmap_tape_inactive[];
extern char *gtkpixmap_tape_active[];
extern char *gtkpixmap_disk_inactive[];
extern char *gtkpixmap_disk_active[];
extern char *gtkpixmap_pause_inactive[];
extern char *gtkpixmap_pause_active[];
extern char *gtkpixmap_tape_marker[];

/*
 * Statusbar routines (statusbar.c)
 */

int gtkstatusbar_create( GtkBox *parent );
int gtkstatusbar_set_visibility( int visible );

#endif				/* #ifndef FUSE_GTKINTERNALS_H */
