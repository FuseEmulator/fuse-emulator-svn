/* gtkdisplay.c: GTK+ routines for dealing with the Speccy screen
   Copyright (c) 2000-2003 Philip Kendall

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

#include <stddef.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "display.h"
#include "fuse.h"
#include "gtkui.h"
#include "screenshot.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "ui/scaler/scaler.h"
#include "scld.h"

GdkGC *gtkdisplay_gc = NULL;
static GdkImage *image;

unsigned long gtkdisplay_colours[16];

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The scaled image */
static WORD scaled_image[2*DISPLAY_SCREEN_HEIGHT][2*DISPLAY_SCREEN_WIDTH];
static const ptrdiff_t scaled_pitch =
                                     2 * DISPLAY_SCREEN_WIDTH * sizeof( WORD );

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int gtkdisplay_current_size=1;

static int gtkdisplay_allocate_colours( unsigned long *pixel_values );
static void gtkdisplay_area(int x, int y, int width, int height);
static int gtkdisplay_configure_notify( int width );
static int select_sensible_scaler( void );

/* Callbacks */

static gint gtkdisplay_expose(GtkWidget *widget, GdkEvent *event,
			      gpointer data);
static gint gtkdisplay_configure(GtkWidget *widget, GdkEvent *event,
				 gpointer data);

int
gtkdisplay_init( void )
{
  int x,y,get_width,get_height, depth;
  GdkGCValues gc_values;

  image = gdk_image_new(GDK_IMAGE_FASTEST, gdk_visual_get_system(),
			2 * DISPLAY_SCREEN_WIDTH, 2 * DISPLAY_SCREEN_HEIGHT );

  gtk_signal_connect( GTK_OBJECT(gtkui_drawing_area), "expose_event", 
		      GTK_SIGNAL_FUNC(gtkdisplay_expose), NULL);
  gtk_signal_connect( GTK_OBJECT(gtkui_drawing_area), "configure_event", 
		      GTK_SIGNAL_FUNC(gtkdisplay_configure), NULL);

  gdk_window_get_geometry( gtkui_drawing_area->window, &x, &y,
			   &get_width, &get_height, &depth );
  gtkdisplay_gc =
    gtk_gc_get( depth, gtk_widget_get_colormap( gtkui_drawing_area ),
		&gc_values, (GdkGCValuesMask) 0 );

  if( gtkdisplay_allocate_colours( gtkdisplay_colours ) ) return 1;

  display_ui_initialised = 1;

  return 0;
}

void
uidisplay_init_scalers( void )
{
  scaler_register_clear();

  switch( image_scale ) {

  case 1:	/* `Normal' machines */
    scaler_register( GFX_NORMAL );
    scaler_register( GFX_DOUBLESIZE );
    break;

  case 2:	/* Timex machines */
    scaler_register( GFX_HALF );
    scaler_register( GFX_NORMAL );
    break;

  default:
    ui_error( UI_ERROR_ERROR, "Unknown image scale %d", image_scale );
    break;

  }
}

int
uidisplay_init( int width, int height )
{
  int error;

  image_scale = width / DISPLAY_ASPECT_WIDTH;

  uidisplay_init_scalers();
  error = select_sensible_scaler(); if( error ) return error;

  display_refresh_all();

  return 0;
}

static int gtkdisplay_allocate_colours( unsigned long *pixel_values )
{
  GdkColor gdk_colours[16];
  GdkColormap *current_map;
  gboolean success[16];

  const char *colour_names[] = {
    "black",
    "blue3",
    "red3",
    "magenta3",
    "green3",
    "cyan3",
    "yellow3",
    "gray80",
    "black",
    "blue",
    "red",
    "magenta",
    "green",
    "cyan",
    "yellow",
    "white",
  };

  int i,ok;

  current_map = gtk_widget_get_colormap( gtkui_drawing_area );

  for(i=0;i<16;i++) {
    if( ! gdk_color_parse(colour_names[i],&gdk_colours[i]) ) {
      fprintf(stderr,"%s: couldn't parse colour `%s'\n",
	      fuse_progname, colour_names[i]);
      return 1;
    }
  }

  gdk_colormap_alloc_colors( current_map, gdk_colours, 16, FALSE, FALSE,
			     success );
  for(i=0,ok=1;i<16;i++) if(!success[i]) ok=0;

  if(!ok) {
    fprintf(stderr,"%s: couldn't allocate all colours in default colourmap\n"
	    "%s: switching to private colourmap\n",
	    fuse_progname,fuse_progname);
    /* FIXME: should free colours in default map here */
    current_map = gdk_colormap_new( gdk_visual_get_system(), FALSE );
    gdk_colormap_alloc_colors( current_map, gdk_colours, 16, FALSE, FALSE,
			       success );
    for(i=0,ok=1;i<16;i++) if(!success[i]) ok=0;
    if( !ok ) {
      fprintf(stderr,"%s: still couldn't allocate all colours\n",
	      fuse_progname);
      return 1;
    }
    gtk_widget_set_colormap( gtkui_window, current_map );
  }

  for(i=0;i<16;i++) pixel_values[i] = gdk_colours[i].pixel;

  return 0;

}
  
static int gtkdisplay_configure_notify( int width )
{
  int size, error;

  size = width / DISPLAY_ASPECT_WIDTH;

  /* If we're the same size as before, nothing special needed */
  if( size == gtkdisplay_current_size ) return 0;

  /* Else set ourselves to the new height */
  gtkdisplay_current_size=size;
  gtk_drawing_area_size( GTK_DRAWING_AREA(gtkui_drawing_area),
			 size * DISPLAY_ASPECT_WIDTH,
			 size * DISPLAY_SCREEN_HEIGHT );

  error = select_sensible_scaler(); if( error ) return error;

  /* Redraw the entire screen... */
  display_refresh_all();

  return 0;
}

static int
select_sensible_scaler( void )
{
  switch( gtkdisplay_current_size ) {

  case 1:
    if( image_scale == 2 ) {
      scaler_select_scaler( GFX_HALF );
    } else {
      scaler_select_scaler( GFX_NORMAL );
    }
    break;
  case 2:
    if( image_scale == 2 ) {
      scaler_select_scaler( GFX_NORMAL );
    } else {
      scaler_select_scaler( GFX_DOUBLESIZE );
    }
    break;
  default:
    ui_error( UI_ERROR_ERROR, "Unknown GTK+ display size %d",
	      gtkdisplay_current_size );
    return 1;
  }

  return 0;
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void
uidisplay_area( int x, int y, int w, int h )
{
  float scale = (float)gtkdisplay_current_size / image_scale;
  int scaled_x = scale * x, scaled_y = scale * y, xx, yy;

  /* Create scaled image */
  scaler_proc( (BYTE*)&display_image[y][x], display_pitch, NULL, 
	       (BYTE*)&scaled_image[scaled_y][scaled_x], scaled_pitch, w, h );

  w *= scale; h *= scale;

  /* Call putpixel multiple times */
  for( yy = scaled_y; yy < scaled_y + h; yy++ )
    for( xx = scaled_x; xx < scaled_x + w; xx++ ) {
      int colour = scaled_image[yy][xx];
      gdk_image_put_pixel( image, xx, yy, gtkdisplay_colours[ colour ] );
    }

  /* Blit to the real screen */
  gtkdisplay_area( scaled_x, scaled_y, w, h );
}

static void gtkdisplay_area(int x, int y, int width, int height)
{
  gdk_draw_image( gtkui_drawing_area->window, gtkdisplay_gc, image, x, y, x, y,
		  width, height );
}

void
uidisplay_hotswap_gfx_mode( void )
{
  fuse_emulation_pause();

  /* Redraw the entire screen... */
  display_refresh_all();

  fuse_emulation_unpause();
}

int
uidisplay_end( void )
{
  return 0;
}

int
gtkdisplay_end( void )
{
  /* Free the image */
  gdk_image_destroy( image );

  /* Free the allocated GC */
  gtk_gc_release( gtkdisplay_gc );

  return 0;
}

/* Callbacks */

/* Called by gtkui_drawing_area on "expose_event" */
static gint
gtkdisplay_expose( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
		   gpointer data GCC_UNUSED )
{
  gtkdisplay_area(event->expose.area.x, event->expose.area.y,
		  event->expose.area.width, event->expose.area.height);
  return TRUE;
}

/* Called by gtkui_drawing_area on "configure_event" */
static gint
gtkdisplay_configure( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
		      gpointer data GCC_UNUSED )
{
  gtkdisplay_configure_notify( event->configure.width );
  return FALSE;
}

#endif			/* #ifdef UI_GTK */
