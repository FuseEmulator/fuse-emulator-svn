/* sdldisplay.c: Routines for dealing with the SDL display
   Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Fredrick Meunier

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

#ifdef UI_SDL			/* Use this iff we're using SDL */

#include <stdio.h>
#include <string.h>
#include <SDL.h>

#include "fuse.h"
#include "display.h"
#include "ui/uidisplay.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */
#include "scld.h"
#include "screenshot.h"
#include "settings.h"
#include "ui/scaler/scaler.h"
#include "machine.h"

static SDL_Surface *gc=NULL;    /* Hardware screen */
static SDL_Surface *tmp_screen=NULL; /* Temporary screen for scalers */

static ScalerProc *scaler_proc;
static int tmp_screen_width;

static DWORD scaler_mode_flags;

enum {
  UPDATE_EXPAND_1_PIXEL = 1 << 0,
  UPDATE_EXPAND_2_Y_PIXELS = 1 << 1
};

static Uint32 colour_values[16];

static SDL_Color colour_palette[] = {
  { 000, 000, 000, 000 }, 
  { 000, 000, 192, 000 }, 
  { 192, 000, 000, 000 }, 
  { 192, 000, 192, 000 }, 
  { 000, 192, 000, 000 }, 
  { 000, 192, 192, 000 }, 
  { 192, 192, 000, 000 }, 
  { 192, 192, 192, 000 }, 
  { 000, 000, 000, 000 }, 
  { 000, 000, 255, 000 }, 
  { 255, 000, 000, 000 }, 
  { 255, 000, 255, 000 }, 
  { 000, 255, 000, 000 }, 
  { 000, 255, 255, 000 }, 
  { 255, 255, 000, 000 }, 
  { 255, 255, 255, 000 }
};

/* This is a rule of thumb for the maximum number of rects that can be updated
   each frame. If more are generated we just update the whole screen */
#define MAX_UPDATE_RECT 300
static SDL_Rect updated_rects[MAX_UPDATE_RECT];
static int num_rects = 0;
static BYTE sdldisplay_force_full_refresh = 1;

/* The current size of the display (in units of DISPLAY_SCREEN_*) */
static float sdldisplay_current_size = 1;

static BYTE sdldisplay_is_full_screen = 0;

static int image_width;
static int image_height;

static int timex;

static int sdldisplay_allocate_colours( int numColours, Uint32 *colour_values );

static int sdldisplay_load_gfx_mode( void );

void
uidisplay_init_scalers( void )
{
  scaler_register_clear();

  scaler_register( GFX_NORMAL );
  scaler_register( GFX_DOUBLESIZE );
  scaler_register( GFX_TRIPLESIZE );
  scaler_register( GFX_2XSAI );
  scaler_register( GFX_SUPER2XSAI );
  scaler_register( GFX_SUPEREAGLE );
  scaler_register( GFX_ADVMAME2X );
  if( machine_current->timex ) {
    scaler_register( GFX_HALF );
    scaler_register( GFX_TIMEXTV );
  } else {
    scaler_register( GFX_TV2X );
  }
}

int
uidisplay_init( int width, int height )
{
  int ret;
  image_width = width;
  image_height = height;

  timex = machine_current->timex;

  uidisplay_init_scalers();

  if ( scaler_select_scaler( current_scaler ) ) current_scaler = GFX_NORMAL;

  ret = sdldisplay_load_gfx_mode();

  SDL_WM_SetCaption( "Fuse", "Fuse" );

  /* We can now output error messages to our output device */
  display_ui_initialised = 1;

  return 0;
}

static int
sdldisplay_allocate_colours( int numColours, Uint32 *colour_values )
{
  int i;

  for( i = 0; i < numColours; i++ ) {
    colour_values[i] = SDL_MapRGB( tmp_screen->format, colour_palette[i].r,
                              colour_palette[i].g, colour_palette[i].b );
  }

  return 0;
}

static int
sdldisplay_load_gfx_mode( void )
{
  Uint16 *tmp_screen_pixels;

  sdldisplay_force_full_refresh = 1;
  scaler_mode_flags = 0;

  tmp_screen = NULL;
  tmp_screen_width = (image_width + 3);

  switch( current_scaler ) {
  case GFX_2XSAI:
    sdldisplay_current_size = 2;
    scaler_proc = _2xSaI;
    scaler_mode_flags = UPDATE_EXPAND_1_PIXEL;
    break;
  case GFX_SUPER2XSAI:
    sdldisplay_current_size = 2;
    scaler_proc = Super2xSaI;
    scaler_mode_flags = UPDATE_EXPAND_1_PIXEL;
    break;
  case GFX_SUPEREAGLE:
    sdldisplay_current_size = 2;
    scaler_proc = SuperEagle;
    scaler_mode_flags = UPDATE_EXPAND_1_PIXEL;
    break;
  case GFX_ADVMAME2X:
    sdldisplay_current_size = 2;
    scaler_proc = AdvMame2x;
    scaler_mode_flags = UPDATE_EXPAND_1_PIXEL;
    break;
  case GFX_TV2X:
    sdldisplay_current_size = 2;
    scaler_proc = TV2x;
    scaler_mode_flags = UPDATE_EXPAND_1_PIXEL;
    break;
  case GFX_TIMEXTV:
    sdldisplay_current_size = 1;
    scaler_proc = TimexTV;
    scaler_mode_flags = UPDATE_EXPAND_2_Y_PIXELS;
    break;
  case GFX_DOUBLESIZE:
    sdldisplay_current_size = 2;
    scaler_proc = Normal2x;
    break;
  case GFX_TRIPLESIZE:
    sdldisplay_current_size = 3;
    scaler_proc = Normal3x;
    break;
  case GFX_NORMAL:
    sdldisplay_current_size = 1;
    scaler_proc = Normal1x;
    break;
  case GFX_HALF:
    sdldisplay_current_size = 0.5;
    scaler_proc = Half;
    break;
  default:
    fprintf( stderr, "%s: unknown gfx mode defaulting to normal\n", fuse_progname);
    sdldisplay_current_size = 1;
    scaler_proc = Normal1x;
  }

  /* Create the surface that contains the scaled graphics in 16 bit mode */
  gc = SDL_SetVideoMode( image_width * sdldisplay_current_size,
                         image_height * sdldisplay_current_size,
                         16,
                         settings_current.full_screen ? (SDL_FULLSCREEN|SDL_SWSURFACE) : SDL_SWSURFACE
                       );
  if( !gc ) {
    fprintf( stderr, "%s: couldn't create gc\n", fuse_progname );
    return 1;
  }

  /* Distinguish 555 and 565 mode */
  if (gc->format->Rmask == 0x7C00)
    Init_2xSaI(555);
  else
    Init_2xSaI(565);

  /* Create the surface used for the graphics in 16 bit before scaling */

  /* Need some extra bytes around when using 2xSaI */
  tmp_screen_pixels = (Uint16*)calloc(tmp_screen_width*(image_height+3), sizeof(Uint16));
  tmp_screen = SDL_CreateRGBSurfaceFrom(tmp_screen_pixels,
                                        tmp_screen_width,
                                        image_height + 3,
                                        16, tmp_screen_width*2,
                                        gc->format->Rmask,
                                        gc->format->Gmask,
                                        gc->format->Bmask,
                                        gc->format->Amask);

  if( !tmp_screen ) {
    fprintf( stderr, "%s: couldn't create tmp_screen\n", fuse_progname );
    return 1;
  }

  sdldisplay_allocate_colours( 16, colour_values );

  /* Redraw the entire screen... */
  display_refresh_all();

  return 0;
}

void
uidisplay_hotswap_gfx_mode( void )
{
  /* Keep around the old image & tmp_screen so we can restore the screen data
     after the mode switch. */
  SDL_Surface *oldtmp_screen = tmp_screen;

  fuse_emulation_pause();

  /* Setup the new GFX mode */
  sdldisplay_load_gfx_mode();

  /* reset palette */
  SDL_SetColors( gc, colour_palette, 0, 16 );

  /* Restore old screen content */
  SDL_BlitSurface(oldtmp_screen, NULL, tmp_screen, NULL);
  SDL_UpdateRect(tmp_screen, 0,  0, 0, 0);

  /* Free the old surfaces */
  free(oldtmp_screen->pixels);
  SDL_FreeSurface(oldtmp_screen);

  /* Mac OS X resets the state of the cursor after a switch to full screen mode */
  if ( settings_current.full_screen )
    SDL_ShowCursor( SDL_DISABLE );
  else
    SDL_ShowCursor( SDL_ENABLE );

  /* Redraw the entire screen... */
  display_refresh_all();

  fuse_emulation_unpause();
}

void
uidisplay_frame_end( void )
{
  SDL_Rect *r;
  Uint32 tmp_screen_pitch, dstPitch;
  SDL_Rect *last_rect;

  /* We check for a switch to fullscreen here to give systems with a
     windowed-only UI a chance to free menu etc. resources before
     the switch to fullscreen (e.g. Mac OS X) */
  if( sdldisplay_is_full_screen != settings_current.full_screen ) {
    uidisplay_hotswap_gfx_mode();
    sdldisplay_is_full_screen = !sdldisplay_is_full_screen;
  }

  /* Force a full redraw if requested */
  if ( sdldisplay_force_full_refresh ) {
    num_rects = 1;

    updated_rects[0].x = 0;
    updated_rects[0].y = 0;
    updated_rects[0].w = image_width;
    updated_rects[0].h = image_height;
  }

#ifdef USE_WIDGET
  if ( !(widget_level >= 0) && num_rects == 0 ) return;
#else                   /* #ifdef USE_WIDGET */
  if ( num_rects == 0 ) return;
#endif                  /* #ifdef USE_WIDGET */

  if( SDL_MUSTLOCK( tmp_screen ) ) SDL_LockSurface( tmp_screen );

  tmp_screen_pitch = tmp_screen->pitch;

  last_rect = updated_rects + num_rects;

  for( r = updated_rects; r != last_rect; r++ ) {

    WORD *dest_base, *dest;
    size_t xx,yy;

    dest_base =
      (WORD*)( (BYTE*)tmp_screen->pixels + (r->x*2+2) +
	       (r->y+1)*tmp_screen_pitch );

    for( yy = r->y; yy < r->y + r->h; yy++ ) {

      for( xx = r->x, dest = dest_base; xx < r->x + r->w; xx++, dest++ )
	*dest = colour_values[ display_image[yy][xx] ];

      dest_base = (WORD*)( (BYTE*)dest_base + tmp_screen_pitch );
    }
	  
  }

  if( SDL_MUSTLOCK( gc ) ) SDL_LockSurface( gc );

  dstPitch = gc->pitch;

  for( r = updated_rects; r != last_rect; ++r ) {
    register int dst_y = r->y * sdldisplay_current_size;
    register int dst_h = r->h;

    scaler_proc(
     (BYTE*)tmp_screen->pixels + (r->x*2+2) + (r->y+1)*tmp_screen_pitch,
     tmp_screen_pitch, NULL,
     (BYTE*)gc->pixels + r->x*(BYTE)(2*sdldisplay_current_size) +
     dst_y*dstPitch, dstPitch, r->w, dst_h );

    /* Adjust rects for the destination rect size */
    r->x *= sdldisplay_current_size;
    r->y = dst_y;
    r->w *= sdldisplay_current_size;
    r->h = dst_h * sdldisplay_current_size;
  }

  if( SDL_MUSTLOCK( tmp_screen ) ) SDL_UnlockSurface( tmp_screen );
  if( SDL_MUSTLOCK( gc ) ) SDL_UnlockSurface( gc );

  /* Finally, blit all our changes to the screen */
  SDL_UpdateRects( gc, num_rects, updated_rects );

  num_rects = 0;
  sdldisplay_force_full_refresh = 0;
}

void
uidisplay_area( int x, int y, int width, int height )
{
  if ( sdldisplay_force_full_refresh )
    return;

  if( num_rects == MAX_UPDATE_RECT ) {
    sdldisplay_force_full_refresh = 1;
    return;
  }

  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. 2xSAI */
  if (scaler_mode_flags & UPDATE_EXPAND_1_PIXEL) {
    x--;
    y--;
    width+=2;   
    height+=2;
  } else if (scaler_mode_flags & UPDATE_EXPAND_2_Y_PIXELS) {
    y-=2;
    height+=4;
  }

  /* clip */
  if ( x < 0 ) { width+=x; x=0; }
  if ( y < 0 ) { height+=y; y=0; }
  if ( width > image_width - x )
    width = image_width - x;
  if ( height > image_height - y )
    height = image_height - y;

  updated_rects[num_rects].x = x;
  updated_rects[num_rects].y = y;
  updated_rects[num_rects].w = width;
  updated_rects[num_rects].h = height;

  num_rects++;
}

int
uidisplay_end( void )
{
  display_ui_initialised = 0;
  if ( tmp_screen ) {
    free(tmp_screen->pixels);
    SDL_FreeSurface(tmp_screen);
  }
  return 0;
}

#endif        /* #ifdef UI_SDL */
