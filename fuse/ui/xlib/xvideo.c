/* xvideo.c: Routines for using the XVideo extension
   Copyright (c) 2007 Gergely Szász, Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#ifdef X_USE_XV

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>

#include "display.h"
#include "settings.h"
#include "ui/ui.h"
#include "xdisplay.h"
#include "xui.h"

XvPortID xvport;
int xvid = -1;
int xv_scaler = 0;
XvImage *xvimage = NULL;

int xvideo_usable = 0;

typedef enum {
  XV_NONE = -1,
  XV_YUY2,
  XV_UYVY,
  XV_RGB2,		/* 16 bit packed RGB */
  XV_RGBT,		/* 15 bit packed RGB */
} xvtype_t;

typedef enum {
  XV_ATTR_BRIGHT = 0,
  XV_ATTR_CONTR,
  XV_ATTR_SATUR,
  XV_ATTR_COLOR,
  XV_ATTR_HUE,
  XV_ATTR_GAMMA,
  XV_ATTR_RED,
  XV_ATTR_GREEN,
  XV_ATTR_BLUE,
  XV_ATTR_CRT,
  XV_ATTR_NONE,
} xv_attr_t;

static xvtype_t xvtype = XV_NONE;
static int xdisplay_xvport_set = 0;
static int xv_redM = 0, xv_greenM = 0, xv_blueM = 0;
static int xv_lx, xv_ly;
static unsigned int xv_lw, xv_lh;

int xvideo_dw, xvideo_dh, xvideo_isize = -1;

typedef struct {
  int avail;
  char *name;
  int min;
  int max;
  int val;
  
  int saved;
} xv_attrib_t;

#define XV_STR_BRIGHT "XV_BRIGHTNESS"
#define XV_STR_CONTR "XV_CONTRAST"
#define XV_STR_SATUR "XV_SATURATION"

#define XV_STR_COLOR "XV_COLOR"
#define XV_STR_HUE "XV_HUE"
#define XV_STR_GAMMA "XV_GAMMA"

#define XV_STR_RED "XV_RED_INTENSITY"
#define XV_STR_GREEN "XV_GREEN_INTENSITY"
#define XV_STR_BLUE "XV_BLUE_INTENSITY"

#define XV_STR_CRT "XV_SWITCHCRT"

static xv_attrib_t xv_attr[] = {
  { 0, XV_STR_BRIGHT, 0, 0, 0, 0 },
  { 0, XV_STR_CONTR, 0, 0, 0, 0 },
  { 0, XV_STR_SATUR, 0, 0, 0, 0 },
  { 0, XV_STR_COLOR, 0, 0, 0, 0 },
  { 0, XV_STR_HUE, 0, 0, 0, 0 },
  { 0, XV_STR_GAMMA, 0, 0, 0, 0 },
  { 0, XV_STR_RED, 0, 0, 0, 0 },
  { 0, XV_STR_GREEN, 0, 0, 0, 0 },
  { 0, XV_STR_BLUE, 0, 0, 0, 0 },
  { 0, XV_STR_CRT, 0, 0, 0, 0 },
};

static void
get_max_isize( void )
{
  XvEncodingInfo *encodings;
  unsigned long xv_max_width = 0;
  unsigned long xv_max_height = 0;
  unsigned int num_encodings, idx;

  XvQueryEncodings( display, xvport, &num_encodings, &encodings );
  
  xvideo_isize = 1;		/* at least... */
  if( encodings ) {
    for( idx = 0; idx < num_encodings; ++idx ) {
      if( strcmp( encodings[idx].name, "XV_IMAGE" ) == 0 ) {
	xv_max_width  = encodings[idx].width;
        xv_max_height = encodings[idx].height;
        break;
      }
    }
    for( idx = 3; idx > 1; idx-- ) {
      if( xv_max_width  >= idx * DISPLAY_ASPECT_WIDTH &&
	  xv_max_height >= idx * DISPLAY_SCREEN_HEIGHT ) {
	xvideo_isize = idx;
	break;
      }
    }
  }
}

static void
find_attributes( void )
{
  int num;
  int k;
  int cur_val;
  Atom val_atom;
  XvAttribute *xvattr;
  xv_attr_t id;

  xvattr = XvQueryPortAttributes( display, xvport, &num);
  for(k = 0; k < num; k++) {
    if( !( xvattr[k].flags & XvGettable && xvattr[k].flags & XvSettable ) )
      continue;
    for( id = XV_ATTR_BRIGHT; id < XV_ATTR_NONE; id++ ) {
      if( strcmp( xvattr[k].name, xv_attr[id].name ) == 0 )
        break;
    }
    val_atom = XInternAtom( display, xvattr[k].name, False );
    if( id == XV_ATTR_NONE || 
	XvGetPortAttribute( display, xvport, val_atom, &cur_val) != Success ||
	XvSetPortAttribute( display, xvport, val_atom, cur_val) != Success
    ) continue;

    xv_attr[id].avail = 1;
    xv_attr[id].min = xvattr[k].min_value;
    xv_attr[id].max = xvattr[k].max_value;
    xv_attr[id].val = id == XV_ATTR_CRT ? cur_val :
				( cur_val - xv_attr[id].min ) * 100 /
					( xv_attr[id].max - xv_attr[id].min );
    xv_attr[id].saved = cur_val;
  }
}

/*
YUY2 packed => Y U Y V  Y U Y V
UYVY packed => U Y V Y  U Y V Y
*/
void
xvideo_find( void )
{
  int i, j, num_frm, num_prt, num_ifrm;
  unsigned int num_adp;
  unsigned int xv_ver, xv_rev, xv_reb, xv_evb, xv_erb;
  xvtype_t xvt = XV_NONE;
  XvAdaptorInfo *ai;
  XvImageFormatValues *ifrm;
  XvFormat *afrm;
  XvPortID port;

  if( XvQueryExtension( display, &xv_ver, &xv_rev, &xv_reb,
				&xv_evb, &xv_erb ) ) {
    fprintf( stderr, "Sorry, no XVideo extension found.\n");
    xvideo_usable = 0;
    return;
  }
  
  /* Get adaptor info */
  if( XvQueryAdaptors( display, xui_mainWindow, &num_adp, &ai )
      != Success ) {
    fprintf( stderr, "XvQueryAdaptors failure\n" );
    return;
  }
  if( settings_current.xv_adaptor >= 0 && 
	settings_current.xv_adaptor < num_adp ) {
    i = settings_current.xv_adaptor;
    num_adp = 1;
  } else {
    i = 0;
  }
  /* For each adapter, get image ifrm available on each port */
  for(; i < num_adp; i++ ) {
    num_prt = ai[i].num_ports;
    afrm = ai[i].formats;
    num_frm = ai[i].num_formats;
    if( settings_current.xv_port >= ai[i].base_id && 
	settings_current.xv_port < ai[i].base_id + num_prt ) {
      port = settings_current.xv_port;
      num_prt = 1;
    } else {
    }
    for( port = ai[i].base_id; port < ai[i].base_id + num_prt; port++ ) {
      ifrm = XvListImageFormats( display, port, &num_ifrm );
      for( j = 0; j < num_ifrm; j++ ) {
	/* Only packed modes YUV and RGB */
	xvt = strcmp( ifrm[j].component_order, "RGB" ) == 0 ?  
			XV_RGB2 : (
			strcmp( ifrm[j].component_order, "ARGB" ) == 0 ? 
			    XV_RGBT : (
			strcmp( ifrm[j].component_order, "YUYV" ) == 0 ? 
			    XV_YUY2 : (
			strcmp( ifrm[j].component_order, "UYVY" ) == 0 ?
			    XV_UYVY : XV_NONE ) ) );
        if( ifrm[j].format != XvPacked || ifrm[j].bits_per_pixel != 16 ||
	    xvt == XV_NONE )
	  continue;
	if( !xdisplay_xvport_set ||
	    xvt == XV_YUY2
	) {
	  xvideo_usable = 1;
	  xvport = port;
	  xdisplay_xvport_set = 1;
	  xvtype = xvt;
	  if( xvtype == XV_RGB2 || xvtype == XV_RGBT ) {
	    xv_redM   = ifrm[j].red_mask;
	    xv_greenM = ifrm[j].green_mask;
	    xv_blueM  = ifrm[j].blue_mask;;
	  } else {
	    xv_redM = xv_greenM =  xv_blueM = 0;
	  }
	  xvid = ifrm[j].id;
	}
      }
    }
  }
  if( xdisplay_xvport_set ) {
    get_max_isize();
    find_attributes();
  }
}

void
xvideo_acquire( void )
{
  if( xvideo_usable && settings_current.xv_scaler ) {
    if( XvGrabPort( display, xvport, CurrentTime ) == Success )
      xv_scaler = settings_current.xv_scaler;
    else {
      settings_current.xv_scaler = 0;
      ui_error( UI_ERROR_WARNING,
        "xdisplay: cannot switch to XVideo scaler (cannot grab port: 0x%02x),\n"
	"  may other application use it (e.g. a video/dvd player)\n",
	  (unsigned int)xvport );
    }
  }
}

#define XV_SET_ATTR(d) \
  if( xv_attr[id].avail ) { \
    val = settings_current.d; \
    if( val > 100 ) \
      val = 100; \
    if( val < 0 ) {\
      settings_current.d = xv_attr[id].val; \
    } else {\
      xv_attr[id].val = val; \
      val = ( xv_attr[id].max - xv_attr[id].min ) * val / 100 + xv_attr[id].min; \
      val_atom = XInternAtom( display, xv_attr[id].name, False ); \
      XvSetPortAttribute( display, xvport, val_atom, val ); \
    } \
  } \
  id++

#define XV_SET_ATTR_2(d) \
  if( xv_attr[id].avail ) { \
    val = settings_current.d; \
    if( val > xv_attr[id].max ) \
      val = xv_attr[id].max; \
    if( val < 0 ) {\
      settings_current.d = xv_attr[id].val; \
    } else {\
      xv_attr[id].val = val; \
      val_atom = XInternAtom( display, xv_attr[id].name, False ); \
      XvSetPortAttribute( display, xvport, val_atom, xv_attr[id].val ); \
    } \
  } \
  id++

static void
set_attributes( void )
{
  xv_attr_t id = XV_ATTR_BRIGHT;
  Atom val_atom;
  int val;
  
  XV_SET_ATTR( xv_bright );
  XV_SET_ATTR( xv_contr );
  XV_SET_ATTR( xv_satur );

  XV_SET_ATTR( xv_color );
  XV_SET_ATTR( xv_hue );
  XV_SET_ATTR( xv_gamma );

  XV_SET_ATTR( xv_red );
  XV_SET_ATTR( xv_green );
  XV_SET_ATTR( xv_blue );

  XV_SET_ATTR_2( xv_crt );
  XSync( display, False );
}

#define RGB_TO_Y(r, g, b)   ( ( 2449L * r + 4809L * g +  934L * b + 1024 ) >> 13 )
#define RGB_TO_U(r, g, b)   ( ( 4096L * b - 1383L * r - 2713L * g + 1024 ) >> 13 )
#define RGB_TO_V(r, g, b)   ( ( 4096L * r - 3430L * g -  666L * b + 1024 ) >> 13 )

#define RGB_TO_R(c)   ( ( c >> 11 )          * 255 / 31 )
#define RGB_TO_G(c) ( ( ( c >>  5 ) & 0x3f ) * 255 / 63 )
#define RGB_TO_B(c) ( (   c         & 0x1f ) * 255 / 31 )

#define XvPutPixel(i,x,y,a,b) *((i)->data + (i)->offsets[0] + (i)->pitches[0] * (y) + 2 * (x))     = a; \
			      *((i)->data + (i)->offsets[0] + (i)->pitches[0] * (y) + 2 * (x) + 1) = b

#define XvPutRGBPixel(i,x,y,a) *(libspectrum_word *)((i)->data + (i)->offsets[0] + (i)->pitches[0] * (y) + 2 * (x)) = a

/* x1 x2 => two pixel per put!!!! 565 RGB */
static void
putpixel_uyvy( int x, int y, libspectrum_word *colour )
{
  unsigned long yy, uv;

  if( x % 2 ) return;	/* return on odd */

  yy = RGB_TO_Y( RGB_TO_R( *colour ), RGB_TO_G( *colour ), RGB_TO_B( *colour ) );
  uv = ( ( RGB_TO_U( RGB_TO_R( *colour ), RGB_TO_G( *colour ), RGB_TO_B( *colour ) )
         + RGB_TO_U( RGB_TO_R( *(colour + 1) ), RGB_TO_G( *(colour + 1) ), RGB_TO_B( *(colour + 1) ) ) ) >> 1 );
  XvPutPixel( xvimage, x, y, uv + 128, yy );
  colour++;
  x++;
  yy = RGB_TO_Y( RGB_TO_R( *colour ), RGB_TO_G( *colour ), RGB_TO_B( *colour ) );
  uv = ( ( RGB_TO_V( RGB_TO_R( *colour ), RGB_TO_G( *colour ), RGB_TO_B( *colour ) )
         + RGB_TO_V( RGB_TO_R( *(colour - 1) ), RGB_TO_G( *(colour - 1) ), RGB_TO_B( *(colour - 1) ) ) ) >> 1 );
  XvPutPixel( xvimage, x, y, uv + 128, yy );
}

static void
putpixel_yuy2( int x, int y, libspectrum_word *colour )
{
  unsigned long yy, uv;

  if( x % 2 ) return;	/* return on odd */

  yy = RGB_TO_Y( RGB_TO_R( *colour ), RGB_TO_G( *colour ), RGB_TO_B( *colour ) );
  uv = ( ( RGB_TO_U( RGB_TO_R( *colour ), RGB_TO_G( *colour ), RGB_TO_B( *colour ) )
         + RGB_TO_U( RGB_TO_R( *(colour + 1) ), RGB_TO_G( *(colour + 1) ), RGB_TO_B( *(colour + 1) ) ) ) >> 1 );
  XvPutPixel( xvimage, x, y, yy, uv + 128 );
  colour++;
  x++;
  yy = RGB_TO_Y( RGB_TO_R( *colour ), RGB_TO_G( *colour ), RGB_TO_B( *colour ) );
  uv = ( ( RGB_TO_V( RGB_TO_R( *colour ), RGB_TO_G( *colour ), RGB_TO_B( *colour ) )
         + RGB_TO_V( RGB_TO_R( *(colour - 1) ), RGB_TO_G( *(colour - 1) ), RGB_TO_B( *(colour - 1) ) ) ) >> 1 );
  XvPutPixel( xvimage, x, y, yy, uv + 128 );
}

/* RGBT 15bit color + 1 bit transparency */
static void
putpixel_rgbt( int x, int y, libspectrum_word *colour)
{
  unsigned long c = *colour;

  c = 0x8000 + ( c & 0x1f ) + ( ( c >>  1 ) & 0xffe0 );
  XvPutRGBPixel( xvimage, x, y, c );
}

static void
putpixel_rgb2( int x, int y, libspectrum_word *colour )
{
  XvPutRGBPixel( xvimage, x, y, *colour );
}

void
xvideo_hotswap_gfx_mode( void )
{
  Window w;
  static int last_state = -1;	/* 0, 1, 2 */
  int d1;			/* dummy */
  unsigned int d;		/* dummy */

  if( xvideo_usable ) {
    if( settings_current.xv_scaler != xv_scaler ) {
      if( !settings_current.xv_scaler || 
        XvGrabPort( display, xvport, CurrentTime ) == Success ) {
        xv_scaler = settings_current.xv_scaler;
	/* 1st we have to destroy the old image */
	xdisplay_destroy_image();
	/* 2nd we have to allocate a new image */
	xdisplay_allocate_image();
      } else if ( settings_current.xv_scaler ) {
        settings_current.xv_scaler = 0;
	ui_error( UI_ERROR_WARNING,
    	    "xdisplay: cannot switch to XVideo scaler (cannot grab port: 0x%02x); "
	    "it may be in use by another application",
	    (unsigned int)xvport );
      }
    }
    if( xv_scaler ) {	/* if we want to use the xvideo */
      xui_setsizehints( 1 );
      set_attributes();
      switch( xvtype ) {
      case XV_RGB2:
    	xdisplay_putpixel = putpixel_rgb2;
    	break;
      case XV_RGBT:
    	xdisplay_putpixel = putpixel_rgbt;
    	break;
      case XV_YUY2:
    	xdisplay_putpixel = putpixel_yuy2;
    	break;
      case XV_UYVY:
    	xdisplay_putpixel = putpixel_uyvy;
    	break;
      default:
    	break;
      }
      if( settings_current.full_screen ) {
	if( last_state != 3 ) {		/* save pos and size */
	  XTranslateCoordinates( display, xui_mainWindow,
				XDefaultRootWindow( display ),
		0, 0, &xv_lx, &xv_ly, &w );
	  xv_lw = xvideo_dw;
	  xv_lh = xvideo_dh;
	}
	xvideo_dw = DisplayWidth( display, xui_screenNum );
	xvideo_dh = DisplayHeight( display, xui_screenNum );
	  /* resize to fullscreen */
        XMoveResizeWindow( display, xui_mainWindow, 0, 0, xvideo_dw, xvideo_dh );
	last_state = 3;	/* xv fullscreen */
      } else {
	if( last_state != 2 && xv_lw != 0 ) {	/* restore xv pos and size */
	  xvideo_dw = xv_lw;
	  xvideo_dh = xv_lh;
          XMoveResizeWindow( display, xui_mainWindow, xv_lx, xv_ly, xvideo_dw, xvideo_dh );
	}
	last_state = 2;	/* xv */
      }
    } else {	/* => x11 */
      if( last_state == 3 ) {
        XMoveWindow( display, xui_mainWindow, xv_lx, xv_ly );
      } else if( last_state == 2 ) {	/* save XV size */
	XTranslateCoordinates( display, xui_mainWindow,
				XDefaultRootWindow( display ),
				0, 0, &xv_lx, &xv_ly, &w );
	XGetGeometry( display, xui_mainWindow, &w, &d1, &d1, &xv_lw, &xv_lh, &d, &d );
      }
      xui_setsizehints( 0 );
    /* resize back to scaler... */
      if( last_state != 1 )
        xdisplay_current_size = 0;	/* force size change */
      xdisplay_setup_rgb_putpixel();
      last_state = 1;	/* x11 */
    }
  } else {
    xdisplay_setup_rgb_putpixel();
  }
  XFlush( display );
}

static void
restore_attributes( void )
{
  xv_attr_t id;
  Atom val_atom;
  
  for( id = XV_ATTR_BRIGHT; id < XV_ATTR_NONE; id++ ) {
    if( !xv_attr[id].avail ) continue;
    val_atom = XInternAtom( display, xv_attr[id].name, False );
    XvSetPortAttribute( display, xvport, val_atom, xv_attr[id].saved );
  }
  XSync( display, False );
}

void
xvideo_end( void )
{
  if( xv_scaler ) restore_attributes();
}

int
xvideo_scaler_in_use( void )
{
  return xv_scaler;
}

void
xvideo_register_scalers( void )
{
  if( machine_current->timex ) {
    scaler_register( SCALER_HALF ); 
    scaler_register( SCALER_HALFSKIP );
    if( xvideo_isize > 1 ) {
      scaler_register( SCALER_NORMAL );
      scaler_register( SCALER_PALTV );
    }
    if( xvideo_isize > 2 ) {
      scaler_register( SCALER_TIMEXTV );
      scaler_register( SCALER_TIMEX1_5X );
    }
  } else {
    scaler_register( SCALER_NORMAL );
    scaler_register( SCALER_PALTV );
    if( xvideo_isize > 1 ) {
      scaler_register( SCALER_DOUBLESIZE );
      scaler_register( SCALER_2XSAI );
      scaler_register( SCALER_SUPER2XSAI );
      scaler_register( SCALER_SUPEREAGLE );
      scaler_register( SCALER_ADVMAME2X );
      scaler_register( SCALER_TV2X );
      scaler_register( SCALER_DOTMATRIX );
      scaler_register( SCALER_PALTV2X );
    }
    if( xvideo_isize > 2 ) {
      scaler_register( SCALER_TRIPLESIZE );
      scaler_register( SCALER_ADVMAME3X );
      scaler_register( SCALER_TV3X );
      scaler_register( SCALER_PALTV3X );
    }
  }
}

void
xvideo_create_image( void )
{
  xvimage = XvCreateImage( display, xvport, xvid,
			   NULL, 
			   xvideo_isize * DISPLAY_ASPECT_WIDTH,
			   xvideo_isize * DISPLAY_SCREEN_HEIGHT );
}

void
xvideo_set_size( int width, int height )
{
  if( !settings_current.full_screen ) {
    xvideo_dw = width; xvideo_dh = height;
  }
}

#else		/* #ifdef X_USE_XV */

/* Stub routines if Xv not available */

#include "fuse.h"

int
xvideo_scaler_in_use( void )
{
  return 0;
}

void
xvideo_find( void )
{
}

void
xvideo_acquire( void )
{
}

void
xvideo_end( void )
{
}

void
xvideo_create_image( void )
{
  /* Should never be called */
  fuse_abort();
}

void
xvideo_register_scalers( void )
{
  /* Should never be called */
  fuse_abort();
}

void
xvideo_set_size( int width, int height )
{
  /* Should never be called */
  fuse_abort();
}

#endif		/* #ifdef X_USE_XV */
