/* sdl2display.c: Routines for dealing with the SDL2 display
   Copyright (c) 2000-2006 Philip Kendall, Matan Ziv-Av, Fredrick Meunier
   Copyright (c) 2014 Gergely Szasz

   $Id$

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

   E-mail: szaszg@hu.inter.net

*/

#include <config.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>

#ifdef USE_WM_ASPECT_X11
#include <X11/Xutil.h>
#include <SDL_syswm.h>
#endif

#include <libspectrum.h>

#include "display.h"
#include "fuse.h"
#include "machine.h"
#include "peripherals/scld.h"
#include "screenshot.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/scaler/scaler.h"
#include "ui/uidisplay.h"
#include "utils.h"

/* SDL2 rendering always use RGB565 texture
   Spectrum -> tmp_screen -> scl_screen -> texture -> (Blit) */

SDL_Window *sdlwin = NULL; /* Hardware screen */
SDL_Renderer *sdlren = NULL;
#ifdef USE_WM_ASPECT_X11
int sdldisplay_use_wm_aspect_hint = 0;
#endif

static SDL_Texture *sdltxt = NULL;
/* Temporary screen for scalers input */
static Uint16 tmp_screen[ 2 * ( DISPLAY_SCREEN_HEIGHT + 4 ) ]
                        [ 2 * ( DISPLAY_SCREEN_WIDTH  + 3 ) ];
static const int tmp_screen_pitch = sizeof( Uint16 ) *
                                    ( DISPLAY_SCREEN_WIDTH + 3 );
static Uint16 *scl_screen = NULL; /* Temporary screen for scalers output */
static int scl_screen_pitch = 0;


static Uint16 tmp_screen_backup[ 2 * ( DISPLAY_SCREEN_HEIGHT + 4 ) ]
                               [ 2 * ( DISPLAY_SCREEN_WIDTH  + 3 ) ];

static SDL_PixelFormat *sdlpix = NULL;

typedef struct {
  SDL_Texture *t;
  int w;
  int h;
} icon_t;

static icon_t red_cassette, green_cassette;
static icon_t red_mdr, green_mdr;
static icon_t red_disk, green_disk;

static ui_statusbar_state sdl_disk_state, sdl_mdr_state, sdl_tape_state;
static int sdl_status_updated;

static SDL_Color colour_palette[] = {
  {   0,   0,   0,   0 },
  {   0,   0, 192,   0 },
  { 192,   0,   0,   0 },
  { 192,   0, 192,   0 },
  {   0, 192,   0,   0 },
  {   0, 192, 192,   0 },
  { 192, 192,   0,   0 },
  { 192, 192, 192,   0 },
  {   0,   0,   0,   0 },
  {   0,   0, 255,   0 },
  { 255,   0,   0,   0 },
  { 255,   0, 255,   0 },
  {   0, 255,   0,   0 },
  {   0, 255, 255,   0 },
  { 255, 255,   0,   0 },
  { 255, 255, 255,   0 }
};

static Uint16 colour_values[16];
static Uint16 bw_values[16];

/* This is a rule of thumb for the maximum number of rects that can be updated
   each frame. If more are generated we just update the whole screen */
#define MAX_UPDATE_RECT 300
static SDL_Rect updated_rects[MAX_UPDATE_RECT];
static int num_rects = 0;
int sdldisplay_force_full_refresh = 1;

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT
   scale * 4, so 2 => 0.5, 6 => 1.5, 4 => 1.0 */
static int image_scale = -1;

static libspectrum_byte sdldisplay_is_full_screen = 0;

static int image_width;
static int image_height;

static int timex;

static void init_scalers( void );
static int sdldisplay_allocate_colours( int numColours, Uint16 *colour_values,
                                        Uint16 *bw_values );

static int sdldisplay_load_gfx_mode( void );

static void
init_scalers( void )
{
  scaler_register_clear();

  scaler_register( SCALER_NORMAL );
  scaler_register( SCALER_DOUBLESIZE );
  scaler_register( SCALER_TRIPLESIZE );
  scaler_register( SCALER_2XSAI );
  scaler_register( SCALER_SUPER2XSAI );
  scaler_register( SCALER_SUPEREAGLE );
  scaler_register( SCALER_ADVMAME2X );
  scaler_register( SCALER_ADVMAME3X );
  scaler_register( SCALER_DOTMATRIX );
  scaler_register( SCALER_TV2X );
  scaler_register( SCALER_TV3X );
  scaler_register( SCALER_PALTV );
  scaler_register( SCALER_PALTV2X );
  scaler_register( SCALER_PALTV3X );
  scaler_register( SCALER_HQ2X );
  scaler_register( SCALER_HQ3X );
  if( machine_current->timex ) {
    scaler_register( SCALER_HALF );
    scaler_register( SCALER_HALFSKIP );
    scaler_register( SCALER_TIMEXTV );
    scaler_register( SCALER_TIMEX1_5X );
  }

  if( scaler_is_supported( current_scaler ) ) {
    scaler_select_scaler( current_scaler );
  } else {
    scaler_select_scaler( SCALER_NORMAL );
  }
}

static int
sdl_convert_icon( SDL_Surface *source, icon_t *icon, int red )
{
  SDL_Surface *copy;   /* Copy with altered palette */
  SDL_Surface *temp;   /* Converted */
  int i;

  SDL_Color colors[ source->format->palette->ncolors ];

  copy = SDL_ConvertSurface( source, source->format, SDL_SWSURFACE );

  for( i = 0; i < copy->format->palette->ncolors; i++ ) {
    colors[i].r = red ? copy->format->palette->colors[i].r : 0;
    colors[i].g = red ? 0 : copy->format->palette->colors[i].g;
    colors[i].b = 0;
  }

  SDL_SetPaletteColors( copy->format->palette, colors, 0, i );

  temp = SDL_ConvertSurface( copy, sdlpix, SDL_SWSURFACE );
  SDL_FreeSurface( copy );

  icon->t = SDL_CreateTextureFromSurface( sdlren, temp );
  icon->w = temp->w; icon->h = temp->h;
  SDL_FreeSurface( temp );

  return 0;
}

static int
sdl_load_status_icon( const char *filename, icon_t *red, icon_t *green )
{
  char path[ PATH_MAX ];
  SDL_Surface *temp;    /* Copy of image as loaded */

  if( red->t ) {
    SDL_DestroyTexture( red->t );
    red->t = NULL;
  }
  if( green->t ) {
    SDL_DestroyTexture( green->t );
    green->t = NULL;
  }
  if( utils_find_file_path( filename, path, UTILS_AUXILIARY_LIB ) ) {
    fprintf( stderr, "%s: Error getting path for icons\n", fuse_progname );
    return -1;
  }

  if( ( temp = SDL_LoadBMP( path ) ) == NULL ) {
    fprintf( stderr, "%s: Error loading icon \"%s\" text:%s\n", fuse_progname,
             path, SDL_GetError() );
    return -1;
  }

  if( temp->format->palette == NULL ) {
    fprintf( stderr, "%s: Icon \"%s\" is not paletted\n", fuse_progname, path );
    return -1;
  }

  sdl_convert_icon( temp, red, 1 );
  sdl_convert_icon( temp, green, 0 );

  SDL_FreeSurface( temp );

  return 0;
}

#ifdef USE_WM_ASPECT_X11
static void
wm_setsizehints( void )
{
  XSizeHints *sizeHints;
  SDL_SysWMinfo wminfo;

  sdldisplay_use_wm_aspect_hint = 0;

  SDL_VERSION( &wminfo.version );
  if( !SDL_GetWindowWMInfo( sdlwin, &wminfo ) ||
      wminfo.subsystem != SDL_SYSWM_X11 ) {
    return;
  }

  if( !( sizeHints = XAllocSizeHints() ) ) {
    fprintf( stderr, "%s: failure allocating memory for X11 WMSizeHints\n",
             fuse_progname );
    return;
  }

  /* Set standard window properties */
  if( settings_current.aspect_hint ) {
    sizeHints->flags = PAspect;
    sizeHints->min_aspect.x = DISPLAY_ASPECT_WIDTH;
    sizeHints->min_aspect.y = DISPLAY_SCREEN_HEIGHT;
    sizeHints->max_aspect.x = DISPLAY_ASPECT_WIDTH;
    sizeHints->max_aspect.y = DISPLAY_SCREEN_HEIGHT;
    XSetWMNormalHints( wminfo.info.x11.display, wminfo.info.x11.window,
                       sizeHints );
  }
  XFree( sizeHints );

  sdldisplay_use_wm_aspect_hint = 1;
}
#endif                  /* ifdef USE_WM_ASPECT_X11 */

static void
sdldisplay_recreate_win_ren( int w, int h )
{
  int x, y;

  if( sdlwin ) {
    SDL_GetWindowPosition( sdlwin, &x, &y );
    SDL_DestroyRenderer( sdlren );
    sdlren = NULL;
    SDL_DestroyWindow( sdlwin );
    sdlwin = NULL;
  } else {
    x = y = SDL_WINDOWPOS_UNDEFINED;
  }

  sdlwin = SDL_CreateWindow( "Fuse", x, y, w, h, SDL_WINDOW_RESIZABLE );
  if( !sdlwin ) {
    fprintf( stderr, "%s: couldn't create SDL2 window\n", fuse_progname );
    fuse_abort();
  }

  sdlren = SDL_CreateRenderer( sdlwin, -1, 0 );
  if( !sdlren ) {
    fprintf( stderr, "%s: couldn't create SDL2 renderer\n", fuse_progname );
    fuse_abort();
  }

#ifdef USE_WM_ASPECT_X11
  wm_setsizehints();
#endif

  /* If we place this before wm_setsizehints(), there is no effect (on X11) */
  SDL_SetWindowMinimumSize( sdlwin, w, h );

  sdlpix = SDL_AllocFormat( SDL_PIXELFORMAT_RGB565 );
  sdldisplay_allocate_colours( 16, colour_values, bw_values );
  scaler_select_bitformat( 565 ); /* SDL2 always use 565 */

  sdl_load_status_icon( "cassette.bmp", &red_cassette, &green_cassette );
  sdl_load_status_icon( "microdrive.bmp", &red_mdr, &green_mdr );
  sdl_load_status_icon( "plus3disk.bmp", &red_disk, &green_disk );
}

int
uidisplay_init( int width, int height )
{
  image_width = width;
  image_height = height;

  SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY,
               settings_current.sdl_scale_quality ?
               settings_current.sdl_scale_quality : "2" );

  timex = machine_current->timex;

  init_scalers();

  if( !sdlwin ) {
    red_cassette.t = green_cassette.t = NULL;
    red_mdr.t = green_mdr.t = NULL;
    red_disk.t = green_disk.t = NULL;
    sdldisplay_recreate_win_ren( 0, 0 );
  }

  if( scaler_select_scaler( current_scaler ) )
    scaler_select_scaler( SCALER_NORMAL );

  image_scale = 0; /* force load_gfx_mode() */
  if( sdldisplay_load_gfx_mode() ) return 1;
  /* We can now output error messages to our output device */
  display_ui_initialised = 1;

  sdl_load_status_icon( "cassette.bmp", &red_cassette, &green_cassette );
  sdl_load_status_icon( "microdrive.bmp", &red_mdr, &green_mdr );
  sdl_load_status_icon( "plus3disk.bmp", &red_disk, &green_disk );

  return 0;
}

static int
sdldisplay_allocate_colours( int numColours, Uint16 *colour_values,
                             Uint16 *bw_values )
{
  int i;
  Uint8 red, green, blue, grey;

  for( i = 0; i < numColours; i++ ) {

      red = colour_palette[i].r;
    green = colour_palette[i].g;
     blue = colour_palette[i].b;

    /* Addition of 0.5 is to avoid rounding errors */
    grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

    colour_values[i] = SDL_MapRGB( sdlpix,  red, green, blue );
    bw_values[i]     = SDL_MapRGB( sdlpix, grey,  grey, grey );
  }

  return 0;
}

/* Called from uidisplay_init()->init_scalers(), uidisplay_init(),
   scaler_select_scaler() */
static int
sdldisplay_load_gfx_mode( void )
{
  int old_image_scale = image_scale;

  image_scale = 4 * scaler_get_scaling_factor( current_scaler );
  if( old_image_scale != image_scale ) {
    /* Free the old pixels */
    if( sdltxt ) {
      SDL_DestroyTexture( sdltxt );
      sdltxt = NULL;
    }
    if( scl_screen ) {
      free( scl_screen );
      scl_screen = NULL;
    }
    if( sdlwin ) { /* SDL2 windows and Renderer... initialised */
      sdldisplay_recreate_win_ren( image_width * image_scale >> 2,
                                   image_height * image_scale >> 2 );

      /* Create the temp screen and texture used for the graphics in 16 bit
         before scaling */
      scl_screen = malloc( image_width * image_height * sizeof( Uint16 ) *
                           image_scale * image_scale >> 4 );

      if( !scl_screen ) {
        fprintf( stderr, "%s: couldn't create SDL texture\n", fuse_progname );
        fuse_abort();
      }
      scl_screen_pitch = image_width * sizeof( Uint16 ) * image_scale >> 2;

      sdltxt = SDL_CreateTexture( sdlren,
                                  SDL_PIXELFORMAT_RGB565,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  image_width * image_scale >> 2,
                                  image_height * image_scale >> 2 );

      if( !sdltxt ) {
        fprintf( stderr, "%s: couldn't create SDL texture (%dx%d)\n",
                 fuse_progname, image_width * image_scale >> 2,
                 image_height * image_scale >> 2 );
        fuse_abort();
      }

      SDL_RenderSetLogicalSize( sdlren,
                                image_width * image_scale >> 2,
                                image_height * image_scale >> 2 );

    }
  }

  if( settings_current.full_screen ) {
    /* if( !SDL_SetWindowFullscreen( sdlwin, SDL_WINDOW_FULLSCREEN ) ) */
    SDL_SetWindowFullscreen( sdlwin, SDL_WINDOW_FULLSCREEN_DESKTOP );
  } else {
    SDL_SetWindowFullscreen( sdlwin, 0 );
  }

  sdldisplay_is_full_screen =
    settings_current.full_screen = !!( SDL_GetWindowFlags( sdlwin ) &
      ( SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP ) );

  /* Redraw the entire screen... */
  sdldisplay_force_full_refresh = 1;
  display_refresh_all();

  return 0;
}

int
uidisplay_hotswap_gfx_mode( void )
{
  fuse_emulation_pause();

  /* Setup the new GFX mode */
  if( sdldisplay_load_gfx_mode() ) return 1;

  /* Mac OS X resets the state of the cursor after a switch to full screen
     mode */
  if( settings_current.full_screen || ui_mouse_grabbed ) {
    SDL_ShowCursor( SDL_DISABLE );
    SDL_WarpMouseInWindow( sdlwin, 128, 128 );
  } else {
    SDL_ShowCursor( SDL_ENABLE );
  }

  fuse_emulation_unpause();

  return 0;
}

void
uidisplay_frame_save( void )
{
  memcpy( tmp_screen_backup, tmp_screen, sizeof( tmp_screen ) );
}

void
uidisplay_frame_restore( void )
{
  memcpy( tmp_screen, tmp_screen_backup, sizeof( tmp_screen ) );
  sdldisplay_force_full_refresh = 1;
}

static void
sdl_blit_icon( icon_t *icon, SDL_Rect *r )
{
  if( timex ) {
    r->x <<= 1;
    r->y <<= 1;
    r->w <<= 1;
    r->h <<= 1;
  }

  r->x = r->x * image_scale >> 2;
  r->y = r->y * image_scale >> 2;
  r->w = r->w * image_scale >> 2;
  r->h = r->h * image_scale >> 2;
  r->x++;
  r->y++;

  /* SDL2 renderer scale our icon... */
  SDL_RenderCopy( sdlren, icon->t, NULL, r );
}

static void
sdl_icon_overlay( int tmp_screen_pitch, Uint32 dstPitch )
{
  SDL_Rect r = { 243, 218, red_disk.w, red_disk.h };

  switch( sdl_disk_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    sdl_blit_icon( &green_disk, &r );
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
    sdl_blit_icon( &red_disk, &r );
    break;
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    break;
  }

  r.x = 264;
  r.y = 218;
  r.w = red_mdr.w;
  r.h = red_mdr.h;

  switch( sdl_mdr_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    sdl_blit_icon( &green_mdr, &r );
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
    sdl_blit_icon( &red_mdr, &r );
    break;
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    break;
  }

  r.x = 285;
  r.y = 220;
  r.w = red_cassette.w;
  r.h = red_cassette.h;

  switch( sdl_tape_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    sdl_blit_icon( &green_cassette, &r );
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    sdl_blit_icon( &red_cassette, &r );
    break;
  }

  sdl_status_updated = 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  libspectrum_word *dest_base, *dest;
  Uint16 *palette_values = settings_current.bw_tv ? bw_values :
                           colour_values;

  Uint16 palette_colour = palette_values[ colour ];

  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    dest_base = dest =
      (libspectrum_word *)( (libspectrum_byte *)tmp_screen +
                            ( x + 1 ) * 2 +
                            ( y + 1 ) * tmp_screen_pitch );

    *(dest++) = palette_colour;
    *(dest++) = palette_colour;
    dest = (libspectrum_word *)
           ( (libspectrum_byte *)dest_base + tmp_screen_pitch );
    *(dest++) = palette_colour;
    *(dest++) = palette_colour;
  } else {
    dest =
      (libspectrum_word *)( (libspectrum_byte *)tmp_screen +
                            ( x + 1 ) * 2 +
                            ( y + 1 ) * tmp_screen_pitch );

    *dest = palette_colour;
  }
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void
uidisplay_plot8( int x, int y, libspectrum_byte data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
  libspectrum_word *dest;
  Uint16 *palette_values = settings_current.bw_tv ? bw_values :
                           colour_values;

  Uint16 palette_ink = palette_values[ ink ];
  Uint16 palette_paper = palette_values[ paper ];

  if( machine_current->timex ) {
    int i;
    libspectrum_word *dest_base;

    x <<= 4; y <<= 1;

    dest_base =
      (libspectrum_word *)( (libspectrum_byte *)tmp_screen +
                            ( x + 1 ) * 2 +
                            ( y + 1 ) * tmp_screen_pitch );

    for( i=0; i<2; i++ ) {
      dest = dest_base;

      *(dest++) = ( data & 0x80 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x80 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x40 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x40 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x20 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x20 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x10 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x10 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x08 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x08 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x04 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x04 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x02 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x02 ) ? palette_ink : palette_paper;
      *(dest++) = ( data & 0x01 ) ? palette_ink : palette_paper;
      *dest     = ( data & 0x01 ) ? palette_ink : palette_paper;

      dest_base = (libspectrum_word *)
                  ( (libspectrum_byte *)dest_base + tmp_screen_pitch );
    }
  } else {
    x <<= 3;

    dest =
      (libspectrum_word *)( (libspectrum_byte *)tmp_screen +
                            ( x + 1 ) * 2 +
                            ( y + 1 ) * tmp_screen_pitch );

    *(dest++) = ( data & 0x80 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x40 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x20 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x10 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x08 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x04 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x02 ) ? palette_ink : palette_paper;
    *dest     = ( data & 0x01 ) ? palette_ink : palette_paper;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
uidisplay_plot16( int x, int y, libspectrum_word data,
                  libspectrum_byte ink, libspectrum_byte paper )
{
  libspectrum_word *dest_base, *dest;
  int i;
  Uint16 *palette_values = settings_current.bw_tv ? bw_values :
                           colour_values;
  Uint16 palette_ink = palette_values[ ink ];
  Uint16 palette_paper = palette_values[ paper ];
  x <<= 4; y <<= 1;

  dest_base =
    (libspectrum_word *)( (libspectrum_byte *)tmp_screen +
                          ( x + 1 ) * 2 +
                          ( y + 1 ) * tmp_screen_pitch );

  for( i=0; i<2; i++ ) {
    dest = dest_base;

    *(dest++) = ( data & 0x8000 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x4000 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x2000 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x1000 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0800 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0400 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0200 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0100 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0080 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0040 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0020 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0010 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0008 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0004 ) ? palette_ink : palette_paper;
    *(dest++) = ( data & 0x0002 ) ? palette_ink : palette_paper;
    *dest     = ( data & 0x0001 ) ? palette_ink : palette_paper;

    dest_base = (libspectrum_word *)
                ( (libspectrum_byte *)dest_base + tmp_screen_pitch );
  }
}

void
uidisplay_frame_end( void )
{
  SDL_Rect *r;
  SDL_Rect *last_rect;

  /* We check for a switch to fullscreen here to give systems with a
     windowed-only UI a chance to free menu etc. resources before
     the switch to fullscreen (e.g. Mac OS X) */
  if( sdldisplay_is_full_screen != settings_current.full_screen &&
      uidisplay_hotswap_gfx_mode() ) {
    fprintf( stderr, "%s: Error switching to fullscreen\n", fuse_progname );
    fuse_abort();
  }

  /* Force a full redraw if requested */
  if( sdldisplay_force_full_refresh ) {
    num_rects = 1;

    updated_rects[0].x = 0;
    updated_rects[0].y = 0;
    updated_rects[0].w = image_width;
    updated_rects[0].h = image_height;
  }

  if( !( ui_widget_level >= 0 ) && num_rects == 0 && !sdl_status_updated )
    return;

  last_rect = updated_rects + num_rects;

  for( r = updated_rects; r != last_rect; r++ ) {
    libspectrum_byte *pixel_data;

    int dst_y = r->y * image_scale >> 2;
    int dst_h = r->h;

    pixel_data = (libspectrum_byte *)scl_screen +
                 (int)( r->x * image_scale >> 1 ) +
                 dst_y * scl_screen_pitch;

    scaler_proc16(
      (libspectrum_byte *)tmp_screen +
                          ( r->x + 1 ) * 2 +
                          ( r->y + 1 ) * tmp_screen_pitch,
      tmp_screen_pitch, pixel_data,
      scl_screen_pitch, r->w, dst_h
    );

    /* Adjust rects for the destination rect size */
    r->x = r->x * image_scale >> 2;
    r->y = dst_y;
    r->w = r->w * image_scale >> 2;
    r->h = dst_h * image_scale >> 2;

    /* Blit our changes to the texture */
    SDL_UpdateTexture( sdltxt, r, pixel_data, scl_screen_pitch );
  }

  SDL_RenderClear( sdlren );
  SDL_RenderCopy( sdlren, sdltxt, NULL, NULL );

  if( settings_current.statusbar )
    sdl_icon_overlay( tmp_screen_pitch, scl_screen_pitch );

  SDL_RenderPresent( sdlren );

  num_rects = 0;
  sdldisplay_force_full_refresh = 0;
}

void
uidisplay_area( int x, int y, int width, int height )
{
  if( sdldisplay_force_full_refresh )
    return;

  if( num_rects == MAX_UPDATE_RECT ) {
    sdldisplay_force_full_refresh = 1;
    return;
  }

  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. 2xSAI */
  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &width, &height, image_width, image_height );

  updated_rects[num_rects].x = x;
  updated_rects[num_rects].y = y;
  updated_rects[num_rects].w = width;
  updated_rects[num_rects].h = height;

  num_rects++;
}

int
uidisplay_end( void )
{
  int i;

  display_ui_initialised = 0;

  for( i=0; i<2; i++ ) {
    if( red_cassette.t ) {
      SDL_DestroyTexture( red_cassette.t ); red_cassette.t = NULL;
    }
    if( green_cassette.t ) {
      SDL_DestroyTexture( green_cassette.t ); green_cassette.t = NULL;
    }
    if( red_mdr.t ) {
      SDL_DestroyTexture( red_mdr.t ); red_mdr.t = NULL;
    }
    if( green_mdr.t ) {
      SDL_DestroyTexture( green_mdr.t ); green_mdr.t = NULL;
    }
    if( red_disk.t ) {
      SDL_DestroyTexture( red_disk.t ); red_disk.t = NULL;
    }
    if( green_disk.t ) {
      SDL_DestroyTexture( green_disk.t ); green_disk.t = NULL;
    }
  }

  return 0;
}

/* The statusbar handling function */
int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  switch( item ) {

  case UI_STATUSBAR_ITEM_DISK:
    sdl_disk_state = state;
    sdl_status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_PAUSED:
    /* We don't support pausing this version of Fuse */
    return 0;

  case UI_STATUSBAR_ITEM_TAPE:
    sdl_tape_state = state;
    sdl_status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_MICRODRIVE:
    sdl_mdr_state = state;
    sdl_status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_MOUSE:
    /* We don't support showing a grab icon */
    return 0;

  }

  ui_error( UI_ERROR_ERROR, "Attempt to update unknown statusbar item %d",
            item );
  return 1;
}
