/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
 * Copyright (C) 2003  Fredrick Meunier, Philip Kendall
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include <config.h>
#include <string.h>
#include "types.h"

#include "scaler.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

static void _2xSaI(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr, BYTE *dstPtr,
                   DWORD dstPitch, int width, int height);
static void Super2xSaI(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
	               BYTE *dstPtr, DWORD dstPitch, int width, int height);
static void SuperEagle(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
	               BYTE *dstPtr, DWORD dstPitch, int width, int height);
static void AdvMame2x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
static void Half(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
static void Normal1x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
static void Normal2x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
static void Normal3x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height);
static void TV2x(BYTE *srcPtr, DWORD srcPitch, BYTE *null,
               BYTE *dstPtr, DWORD dstPitch, int width, int height);
static void TimexTV(BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr,
               DWORD dstPitch, int width, int height);

static int scaler_supported[ SCALER_NUM ] = {0};

int scalers_registered = 0;

struct scaler_info {

  const char *name;
  const char *id;
  enum scaler_flags_t flags;
  ScalerProc *scaler;

};

/* Information on each of the available scalers. Make sure this array stays
   in the same order as scaler.h:scaler_type */
static struct scaler_info available_scalers[] = {

  { "Timex Half size", "half",       SCALER_FLAGS_NONE,	       Half       },
  { "Normal",	       "normal",     SCALER_FLAGS_NONE,	       Normal1x   },
  { "Double size",     "2x",	     SCALER_FLAGS_NONE,	       Normal2x   },
  { "Triple size",     "3x",	     SCALER_FLAGS_NONE,	       Normal3x   },
  { "2xSaI",	       "2xsai",	     SCALER_EXPAND_1_PIXEL,    _2xSaI     },
  { "Super 2xSaI",     "super2xsai", SCALER_EXPAND_1_PIXEL,    Super2xSaI },
  { "SuperEagle",      "supereagle", SCALER_EXPAND_1_PIXEL,    SuperEagle },
  { "AdvMAME 2x",      "advmame2x",  SCALER_EXPAND_1_PIXEL,    AdvMame2x  },
  { "TV 2x",	       "tv2x",	     SCALER_EXPAND_1_PIXEL,    TV2x       },
  { "Timex TV",	       "timextv",    SCALER_EXPAND_2_Y_PIXELS, TimexTV    },
};

scaler_type current_scaler = SCALER_NUM;
ScalerProc *scaler_proc;
scaler_flags_t scaler_flags;

int
scaler_select_scaler( scaler_type scaler )
{
  if( !scaler_is_supported( scaler ) ) return 1;

  if( current_scaler == scaler ) return 0;

  current_scaler = scaler;

  if( settings_current.start_scaler_mode ) free( settings_current.start_scaler_mode );
  settings_current.start_scaler_mode =
    strdup( available_scalers[current_scaler].id );
  if( !settings_current.start_scaler_mode ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  scaler_proc = scaler_get_proc( current_scaler );
  scaler_flags = scaler_get_flags( current_scaler );

  uidisplay_hotswap_gfx_mode();

  return 0;
}

int
scaler_select_id( const char *id )
{
  scaler_type i;

  for( i=0; i < SCALER_NUM; i++ ) {
    if( ! strcmp( available_scalers[i].id, id ) ) {
      scaler_select_scaler( i );
      return 0;
    }
  }

  ui_error( UI_ERROR_ERROR, "Scaler id '%s' unknown", id );
  return 1;
}

void
scaler_register_clear( void )
{
  scalers_registered = 0;
  memset( scaler_supported, 0, sizeof(int) * SCALER_NUM );
}

void
scaler_register( scaler_type scaler )
{
  if( scaler_supported[scaler] == 1 ) return;
  scalers_registered++;
  scaler_supported[scaler] = 1;
}

int
scaler_is_supported( scaler_type scaler )
{
  return ( scaler >= SCALER_NUM ? 0 : scaler_supported[scaler] );
}

const char *
scaler_name( scaler_type scaler )
{
  return available_scalers[scaler].name;
}

ScalerProc*
scaler_get_proc( scaler_type scaler )
{
  return available_scalers[scaler].scaler;
}

scaler_flags_t
scaler_get_flags( scaler_type scaler )
{
  return available_scalers[scaler].flags;
}

/* The actual code for the scalers starts here */

/* FIXME: do this in a more maintainable fashion */
#ifdef UI_GTK
#define SCALER_DATA_SIZE 4
#elif defined( UI_SDL ) || defined( UI_X ) /* #ifdef UI_GTK */
#define SCALER_DATA_SIZE 2
#else				/* #ifdef UI_GTK */
#error Unknown scaler data size for this user interface
#endif				/* #ifdef UI_GTK */

#if SCALER_DATA_SIZE == 2
typedef WORD scaler_data_type;
#elif SCALER_DATA_SIZE == 4	/* #if SCALER_DATA_SIZE == 2 */
typedef DWORD scaler_data_type;
#else				/* #if SCALER_DATA_SIZE == 2 or 4 */
#error Unknown scaler data size
#endif				/* #if SCALER_DATA_SIZE == 2 or 4 */

/********** 2XSAI Filter *****************/
static DWORD colorMask;
static DWORD lowPixelMask;
static DWORD qcolorMask;
static DWORD qlowpixelMask;
static DWORD redblueMask;
static DWORD greenMask;

int 
scaler_select_bitformat( DWORD BitFormat )
{
  switch( BitFormat ) {

  case 565:
    colorMask = 0x0000F7DE;
    lowPixelMask = 0x00000821;
    qcolorMask = 0x0000E79C;
    qlowpixelMask = 0x00001863;
    redblueMask = 0xF81F;
    greenMask = 0x7E0;
    break;

  case 555:
    colorMask = 0x00007BDE;
    lowPixelMask = 0x00000421;
    qcolorMask = 0x0000739C;
    qlowpixelMask = 0x00000C63;
    redblueMask = 0x7C1F;
    greenMask = 0x3E0;
    break;

  case 888:
    colorMask = 0xFEFEFE00;
    lowPixelMask = 0x01010100;
    qcolorMask = 0xFCFCFC00;
    qlowpixelMask = 0x03030300;
    redblueMask = 0xFF00FF00;
    greenMask = 0x00FF0000;
    break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown bitformat %d", BitFormat );
    return 1;

  }

  return 0;
}

static inline int 
GetResult1(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r += 1;
  if (y <= 1)
    r -= 1;
  return r;
}

static inline int 
GetResult2(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r -= 1;
  if (y <= 1)
    r += 1;
  return r;
}

static inline int 
GetResult(DWORD A, DWORD B, DWORD C, DWORD D)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r += 1;
  if (y <= 1)
    r -= 1;
  return r;
}

static inline DWORD 
INTERPOLATE(DWORD A, DWORD B)
{
  if (A != B) {
    return (((A & colorMask) >> 1) + ((B & colorMask) >> 1) + (A & B & lowPixelMask));
  } else
    return A;
}

static inline DWORD 
Q_INTERPOLATE(DWORD A, DWORD B, DWORD C, DWORD D)
{
  register DWORD x = ((A & qcolorMask) >> 2) +
  ((B & qcolorMask) >> 2) + ((C & qcolorMask) >> 2) + ((D & qcolorMask) >> 2);
  register DWORD y = (A & qlowpixelMask) +
  (B & qlowpixelMask) + (C & qlowpixelMask) + (D & qlowpixelMask);

  y = (y >> 2) & qlowpixelMask;
  return x + y;
}

#define BLUE_MASK565 0x001F001F
#define RED_MASK565 0xF800F800
#define GREEN_MASK565 0x07E007E0

#define BLUE_MASK555 0x001F001F
#define RED_MASK555 0x7C007C00
#define GREEN_MASK555 0x03E003E0

static void 
Super2xSaI(BYTE *srcPtr, DWORD srcPitch,
	BYTE *deltaPtr, BYTE *dstPtr, DWORD dstPitch, int width, int height)
{
  scaler_data_type *bP, *dP;

  {
    DWORD Nextline = srcPitch / sizeof( scaler_data_type );
    DWORD nextDstLine = dstPitch / sizeof( scaler_data_type );
    DWORD finish;

    while (height--) {
      bP = (scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;

      for( finish = width; finish; finish-- ) {
	DWORD color4, color5, color6;
	DWORD color1, color2, color3;
	DWORD colorA0, colorA1, colorA2, colorA3, colorB0, colorB1, colorB2,
	 colorB3, colorS1, colorS2;
	DWORD product1a, product1b, product2a, product2b;

/*---------------------------------------    B1 B2
                                           4  5  6 S2
                                           1  2  3 S1
	                                     A1 A2
*/

        colorB0 = *(bP - Nextline - 1);
	colorB1 = *(bP - Nextline);
	colorB2 = *(bP - Nextline + 1);
	colorB3 = *(bP - Nextline + 2);

	color4 = *(bP - 1);
	color5 = *(bP);
	color6 = *(bP + 1);
	colorS2 = *(bP + 2);

	color1 = *(bP + Nextline - 1);
	color2 = *(bP + Nextline);
	color3 = *(bP + Nextline + 1);
	colorS1 = *(bP + Nextline + 2);

	colorA0 = *(bP + Nextline + Nextline - 1);
	colorA1 = *(bP + Nextline + Nextline);
	colorA2 = *(bP + Nextline + Nextline + 1);
	colorA3 = *(bP + Nextline + Nextline + 2);

/*--------------------------------------*/
	if (color2 == color6 && color5 != color3) {
	  product2b = product1b = color2;
	} else if (color5 == color3 && color2 != color6) {
	  product2b = product1b = color5;
	} else if (color5 == color3 && color2 == color6) {
	  register int r = 0;

	  r += GetResult(color6, color5, color1, colorA1);
	  r += GetResult(color6, color5, color4, colorB1);
	  r += GetResult(color6, color5, colorA2, colorS1);
	  r += GetResult(color6, color5, colorB2, colorS2);

	  if (r > 0)
	    product2b = product1b = color6;
	  else if (r < 0)
	    product2b = product1b = color5;
	  else {
	    product2b = product1b = INTERPOLATE(color5, color6);
	  }
	} else {
	  if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
	    product2b = Q_INTERPOLATE(color3, color3, color3, color2);
	  else if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
	    product2b = Q_INTERPOLATE(color2, color2, color2, color3);
	  else
	    product2b = INTERPOLATE(color2, color3);

	  if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
	    product1b = Q_INTERPOLATE(color6, color6, color6, color5);
	  else if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
	    product1b = Q_INTERPOLATE(color6, color5, color5, color5);
	  else
	    product1b = INTERPOLATE(color5, color6);
	}

	if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
	  product2a = INTERPOLATE(color2, color5);
	else if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
	  product2a = INTERPOLATE(color2, color5);
	else
	  product2a = color2;

	if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
	  product1a = INTERPOLATE(color2, color5);
	else if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
	  product1a = INTERPOLATE(color2, color5);
	else
	  product1a = color5;

	*dP = product1a; *(dP+nextDstLine) = product2a; dP++;
	*dP = product1b; *(dP+nextDstLine) = product2b; dP++;

	bP++;
      }				/* end of for ( finish= width etc..) */

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
      deltaPtr += srcPitch;
    }				/* while (height--) */
  }
}

static void 
SuperEagle(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
	   BYTE *dstPtr, DWORD dstPitch, int width, int height)
{
  scaler_data_type *bP, *dP;

  {
    DWORD finish;
    DWORD Nextline = srcPitch / sizeof( scaler_data_type );
    DWORD nextDstLine = dstPitch / sizeof( scaler_data_type );

    while (height--) {
      bP = (scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;
      for( finish = width; finish; finish-- ) {
	DWORD color4, color5, color6;
	DWORD color1, color2, color3;
	DWORD colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
	DWORD product1a, product1b, product2a, product2b;

	colorB1 = *(bP - Nextline);
	colorB2 = *(bP - Nextline + 1);

	color4 = *(bP - 1);
	color5 = *(bP);
	color6 = *(bP + 1);
	colorS2 = *(bP + 2);

	color1 = *(bP + Nextline - 1);
	color2 = *(bP + Nextline);
	color3 = *(bP + Nextline + 1);
	colorS1 = *(bP + Nextline + 2);

	colorA1 = *(bP + Nextline + Nextline);
	colorA2 = *(bP + Nextline + Nextline + 1);

	/* -------------------------------------- */
	if (color2 == color6 && color5 != color3) {
	  product1b = product2a = color2;
	  if ((color1 == color2) || (color6 == colorB2)) {
	    product1a = INTERPOLATE(color2, color5);
	    product1a = INTERPOLATE(color2, product1a);
	  } else {
	    product1a = INTERPOLATE(color5, color6);
	  }

	  if ((color6 == colorS2) || (color2 == colorA1)) {
	    product2b = INTERPOLATE(color2, color3);
	    product2b = INTERPOLATE(color2, product2b);
	  } else {
	    product2b = INTERPOLATE(color2, color3);
	  }
	} else if (color5 == color3 && color2 != color6) {
	  product2b = product1a = color5;

	  if ((colorB1 == color5) || (color3 == colorS1)) {
	    product1b = INTERPOLATE(color5, color6);
	    product1b = INTERPOLATE(color5, product1b);
	  } else {
	    product1b = INTERPOLATE(color5, color6);
	  }

	  if ((color3 == colorA2) || (color4 == color5)) {
	    product2a = INTERPOLATE(color5, color2);
	    product2a = INTERPOLATE(color5, product2a);
	  } else {
	    product2a = INTERPOLATE(color2, color3);
	  }

	} else if (color5 == color3 && color2 == color6) {
	  register int r = 0;

	  r += GetResult(color6, color5, color1, colorA1);
	  r += GetResult(color6, color5, color4, colorB1);
	  r += GetResult(color6, color5, colorA2, colorS1);
	  r += GetResult(color6, color5, colorB2, colorS2);

	  if (r > 0) {
	    product1b = product2a = color2;
	    product1a = product2b = INTERPOLATE(color5, color6);
	  } else if (r < 0) {
	    product2b = product1a = color5;
	    product1b = product2a = INTERPOLATE(color5, color6);
	  } else {
	    product2b = product1a = color5;
	    product1b = product2a = color2;
	  }
	} else {
	  product2b = product1a = INTERPOLATE(color2, color6);
	  product2b = Q_INTERPOLATE(color3, color3, color3, product2b);
	  product1a = Q_INTERPOLATE(color5, color5, color5, product1a);

	  product2a = product1b = INTERPOLATE(color5, color3);
	  product2a = Q_INTERPOLATE(color2, color2, color2, product2a);
	  product1b = Q_INTERPOLATE(color6, color6, color6, product1b);
	}

	*dP = product1a; *(dP+nextDstLine) = product2a; dP++;
	*dP = product1b; *(dP+nextDstLine) = product2b; dP++;

	bP++;
      }				/* end of for ( finish= width etc..) */

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
      deltaPtr += srcPitch;
    }				/* endof: while (height--) */
  }
}

static void 
_2xSaI(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
       BYTE *dstPtr, DWORD dstPitch, int width, int height)
{
  BYTE *dP;
  WORD *bP;
  DWORD inc_bP;


  {
    DWORD Nextline = srcPitch >> 1;
    inc_bP = 1;

    while (height--) {
      DWORD finish;
      bP = (WORD *) srcPtr;
      dP = dstPtr;

      for (finish = width; finish; finish -= inc_bP) {

	register DWORD colorA, colorB;
	DWORD colorC, colorD, colorE, colorF, colorG, colorH, colorI, colorJ,
	 colorK, colorL, colorM, colorN, colorO, colorP;
	DWORD product, product1, product2;

/*---------------------------------------
   Map of the pixels:                    I|E F|J
                                         G|A B|K
                                         H|C D|L
                                         M|N O|P
*/
	colorI = *(bP - Nextline - 1);
	colorE = *(bP - Nextline);
	colorF = *(bP - Nextline + 1);
	colorJ = *(bP - Nextline + 2);

	colorG = *(bP - 1);
	colorA = *(bP);
	colorB = *(bP + 1);
	colorK = *(bP + 2);

	colorH = *(bP + Nextline - 1);
	colorC = *(bP + Nextline);
	colorD = *(bP + Nextline + 1);
	colorL = *(bP + Nextline + 2);

	colorM = *(bP + Nextline + Nextline - 1);
	colorN = *(bP + Nextline + Nextline);
	colorO = *(bP + Nextline + Nextline + 1);
	colorP = *(bP + Nextline + Nextline + 2);

	if ((colorA == colorD) && (colorB != colorC)) {
	  if (((colorA == colorE) && (colorB == colorL)) || ((colorA == colorC) && (colorA == colorF)
						       && (colorB != colorE)
						   && (colorB == colorJ))) {
	    product = colorA;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if (((colorA == colorG) && (colorC == colorO)) || ((colorA == colorB) && (colorA == colorH)
						       && (colorG != colorC)
						   && (colorC == colorM))) {
	    product1 = colorA;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	  product2 = colorA;
	} else if ((colorB == colorC) && (colorA != colorD)) {
	  if (((colorB == colorF) && (colorA == colorH)) || ((colorB == colorE) && (colorB == colorD)
						       && (colorA != colorF)
						   && (colorA == colorI))) {
	    product = colorB;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if (((colorC == colorH) && (colorA == colorF)) || ((colorC == colorG) && (colorC == colorD)
						       && (colorA != colorH)
						   && (colorA == colorI))) {
	    product1 = colorC;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	  product2 = colorB;
	} else if ((colorA == colorD) && (colorB == colorC)) {
	  if (colorA == colorB) {
	    product = colorA;
	    product1 = colorA;
	    product2 = colorA;
	  } else {
	    register int r = 0;

	    product1 = INTERPOLATE(colorA, colorC);
	    product = INTERPOLATE(colorA, colorB);

	    r += GetResult1(colorA, colorB, colorG, colorE, colorI);
	    r += GetResult2(colorB, colorA, colorK, colorF, colorJ);
	    r += GetResult2(colorB, colorA, colorH, colorN, colorM);
	    r += GetResult1(colorA, colorB, colorL, colorO, colorP);

	    if (r > 0)
	      product2 = colorA;
	    else if (r < 0)
	      product2 = colorB;
	    else {
	      product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);
	    }
	  }
	} else {
	  product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);

	  if ((colorA == colorC) && (colorA == colorF)
	      && (colorB != colorE) && (colorB == colorJ)) {
	    product = colorA;
	  } else if ((colorB == colorE) && (colorB == colorD)
		     && (colorA != colorF) && (colorA == colorI)) {
	    product = colorB;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if ((colorA == colorB) && (colorA == colorH)
	      && (colorG != colorC) && (colorC == colorM)) {
	    product1 = colorA;
	  } else if ((colorC == colorG) && (colorC == colorD)
		     && (colorA != colorH) && (colorA == colorI)) {
	    product1 = colorC;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	}

#ifdef WORDS_BIGENDIAN
	product = (colorA << 16) | product;
	product1 = (product1 << 16) | product2;
#else				/* #ifdef WORDS_BIGENDIAN */
	product = colorA | (product << 16);
	product1 = product1 | (product2 << 16);
#endif				/* #ifdef WORDS_BIGENDIAN */
	*((SDWORD *) dP) = product;
	*((DWORD *) (dP + dstPitch)) = product1;

	bP += inc_bP;
	dP += sizeof(DWORD);
      }				/* end of for ( finish= width etc..) */

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
      deltaPtr += srcPitch;
    }				/* endof: while (height--) */
  }
}

static DWORD 
Bilinear(DWORD A, DWORD B, DWORD x)
{
  unsigned long areaA, areaB;
  unsigned long result;

  if (A == B)
    return A;

  areaB = (x >> 11) & 0x1f;	/* reduce 16 bit fraction to 5 bits */
  areaA = 0x20 - areaB;

  A = (A & redblueMask) | ((A & greenMask) << 16);
  B = (B & redblueMask) | ((B & greenMask) << 16);

  result = ((areaA * A) + (areaB * B)) >> 5;

  return (result & redblueMask) | ((result >> 16) & greenMask);

}

static DWORD 
Bilinear4(DWORD A, DWORD B, DWORD C, DWORD D, DWORD x, DWORD y)
{
  unsigned long areaA, areaB, areaC, areaD;
  unsigned long result, xy;

  x = (x >> 11) & 0x1f;
  y = (y >> 11) & 0x1f;
  xy = (x * y) >> 5;

  A = (A & redblueMask) | ((A & greenMask) << 16);
  B = (B & redblueMask) | ((B & greenMask) << 16);
  C = (C & redblueMask) | ((C & greenMask) << 16);
  D = (D & redblueMask) | ((D & greenMask) << 16);

  areaA = 0x20 + xy - x - y;
  areaB = x - xy;
  areaC = y - xy;
  areaD = xy;

  result = ((areaA * A) + (areaB * B) + (areaC * C) + (areaD * D)) >> 5;

  return (result & redblueMask) | ((result >> 16) & greenMask);
}

void 
Scale_2xSaI(BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
	    BYTE *dstPtr, DWORD dstPitch,
	    DWORD dstWidth, DWORD dstHeight, int width, int height)
{
  BYTE *dP;
  WORD *bP;

  DWORD w;
  DWORD h;
  DWORD dw;
  DWORD dh;
  DWORD hfinish;
  DWORD wfinish;

  DWORD Nextline = srcPitch >> 1;

  wfinish = (width - 1) << 16;	/* convert to fixed point */
  dw = wfinish / (dstWidth - 1);
  hfinish = (height - 1) << 16;	/* convert to fixed point */
  dh = hfinish / (dstHeight - 1);

  for (h = 0; h < hfinish; h += dh) {
    DWORD y1, y2;

    y1 = h & 0xffff;		/* fraction part of fixed point */
    bP = (WORD *) (srcPtr + ((h >> 16) * srcPitch));
    dP = dstPtr;
    y2 = 0x10000 - y1;

    w = 0;

    for (; w < wfinish;) {
      DWORD A, B, C, D;
      DWORD E, F, G, H;
      DWORD I, J, K, L;
      DWORD x1, x2, a1, f1, f2;
      DWORD position, product1 = 0;

      position = w >> 16;
      A = bP[position];		/* current pixel */
      B = bP[position + 1];	/* next pixel */
      C = bP[position + Nextline];
      D = bP[position + Nextline + 1];
      E = bP[position - Nextline];
      F = bP[position - Nextline + 1];
      G = bP[position - 1];
      H = bP[position + Nextline - 1];
      I = bP[position + 2];
      J = bP[position + Nextline + 2];
      K = bP[position + Nextline + Nextline];
      L = bP[position + Nextline + Nextline + 1];

      x1 = w & 0xffff;		/* fraction part of fixed point */
      x2 = 0x10000 - x1;

      /* 0 */
      if (A == B && C == D && A == C)
	product1 = A;
      else
	/* 1 */
      if (A == D && B != C) {
	f1 = (x1 >> 1) + (0x10000 >> 2);
	f2 = (y1 >> 1) + (0x10000 >> 2);
	if (y1 <= f1 && A == J && A != E) {	/* close to B */
	  a1 = f1 - y1;
	  product1 = Bilinear(A, B, a1);
	} else if (y1 >= f1 && A == G && A != L) {	/* close to C */
	  a1 = y1 - f1;
	  product1 = Bilinear(A, C, a1);
	} else if (x1 >= f2 && A == E && A != J) {	/* close to B */
	  a1 = x1 - f2;
	  product1 = Bilinear(A, B, a1);
	} else if (x1 <= f2 && A == L && A != G) {	/* close to C */
	  a1 = f2 - x1;
	  product1 = Bilinear(A, C, a1);
	} else if (y1 >= x1) {	/* close to C */
	  a1 = y1 - x1;
	  product1 = Bilinear(A, C, a1);
	} else if (y1 <= x1) {	/* close to B */
	  a1 = x1 - y1;
	  product1 = Bilinear(A, B, a1);
	}
      } else
	/* 2 */
      if (B == C && A != D) {
	f1 = (x1 >> 1) + (0x10000 >> 2);
	f2 = (y1 >> 1) + (0x10000 >> 2);
	if (y2 >= f1 && B == H && B != F) {	/* close to A */
	  a1 = y2 - f1;
	  product1 = Bilinear(B, A, a1);
	} else if (y2 <= f1 && B == I && B != K) {	/* close to D */
	  a1 = f1 - y2;
	  product1 = Bilinear(B, D, a1);
	} else if (x2 >= f2 && B == F && B != H) {	/* close to A */
	  a1 = x2 - f2;
	  product1 = Bilinear(B, A, a1);
	} else if (x2 <= f2 && B == K && B != I) {	/* close to D */
	  a1 = f2 - x2;
	  product1 = Bilinear(B, D, a1);
	} else if (y2 >= x1) {	/* close to A */
	  a1 = y2 - x1;
	  product1 = Bilinear(B, A, a1);
	} else if (y2 <= x1) {	/* close to D */
	  a1 = x1 - y2;
	  product1 = Bilinear(B, D, a1);
	}
      }
      /* 3 */
      else {
	product1 = Bilinear4(A, B, C, D, x1, y1);
      }

/*end First Pixel */
      *(DWORD *) dP = product1;
      dP += 2;
      w += dw;
    }
    dstPtr += dstPitch;
  }
}

static void 
AdvMame2x(BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr, DWORD dstPitch,
	  int width, int height)
{
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  scaler_data_type *p = (scaler_data_type*) srcPtr;

  unsigned nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*) dstPtr;

  while (height--) {
    int i;
    for (i = 0; i < width; ++i) {
      /* short A = *(p + i - nextlineSrc - 1); */
      scaler_data_type B = *(p + i - nextlineSrc);
      /* short C = *(p + i - nextlineSrc + 1); */
      scaler_data_type D = *(p + i - 1);
      scaler_data_type E = *(p + i);
      scaler_data_type F = *(p + i + 1);
      /* short G = *(p + i + nextlineSrc - 1); */
      scaler_data_type H = *(p + i + nextlineSrc);
      /* short I = *(p + i + nextlineSrc + 1); */

      *(q + (i << 1)) = D == B && B != F && D != H ? D : E;
      *(q + (i << 1) + 1) = B == F && B != D && F != H ? F : E;
      *(q + (i << 1) + nextlineDst) = D == H && D != B && H != F ? D : E;
      *(q + (i << 1) + nextlineDst + 1) = H == F && D != H && B != F ? F : E;
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}

void 
Half(BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr, DWORD dstPitch,
	 int width, int height)
{
  scaler_data_type *r;

  while (height--) {
    int i;
    r = (scaler_data_type*) dstPtr;

    if( ( height & 1 ) == 0 ) {
      for (i = 0; i < width; i+=2, ++r) {
        scaler_data_type color1 = *(((scaler_data_type*) srcPtr) + i);
        scaler_data_type color2 = *(((scaler_data_type*) srcPtr) + i + 1);

        *r = INTERPOLATE(color1, color2);
      }
      dstPtr += dstPitch;
    }

    srcPtr += srcPitch;
  }
}

static void 
Normal1x( BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr,
	  DWORD dstPitch, int width, int height )
{
  scaler_data_type i, *s, *d;

  while( height-- ) {

    /* FIXME: should we use memcpy(3) here? For 32-bit data, this loop
       is as efficient as it's going to get, so we probably want to
       avoid the overhead of the function call. For 16-bit data, we'd
       really like to copy it in 32-bit chunks (and 1 16-bit chunk at
       the end if necessary), which memcpy will get right for us.

       Currently, this code is going to be at least as efficient as the
       old version, so I'll leave this for now
    */
    for( i = 0, s = (scaler_data_type*)srcPtr, d = (scaler_data_type*)dstPtr;
	 i < width;
	 i++ )
      *d++ = *s++;

    srcPtr += srcPitch;
    dstPtr += dstPitch;
  }
}

static void 
Normal2x( BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr,
	  DWORD dstPitch, int width, int height )
{
  scaler_data_type i, *s, *d, *d2;

  while( height-- ) {

    for( i = 0, s = (scaler_data_type*)srcPtr,
	   d = (scaler_data_type*)dstPtr,
	   d2 = (scaler_data_type*)(dstPtr + dstPitch);
	 i < width;
	 i++ ) {
      *d++ = *d2++ = *s; *d++ = *d2++ = *s++;
    }

    srcPtr += srcPitch;
    dstPtr += dstPitch << 1;
  }
}

static void 
Normal3x(BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr, DWORD dstPitch,
	 int width, int height)
{
  BYTE *r;
  DWORD dstPitch2 = dstPitch * 2;
  DWORD dstPitch3 = dstPitch * 3;

  while (height--) {
    int i;
    r = dstPtr;
    for (i = 0; i < width; ++i, r += 6) {
      WORD color = *(((WORD *) srcPtr) + i);

      *(WORD *) (r + 0) = color;
      *(WORD *) (r + 2) = color;
      *(WORD *) (r + 4) = color;
      *(WORD *) (r + 0 + dstPitch) = color;
      *(WORD *) (r + 2 + dstPitch) = color;
      *(WORD *) (r + 4 + dstPitch) = color;
      *(WORD *) (r + 0 + dstPitch2) = color;
      *(WORD *) (r + 2 + dstPitch2) = color;
      *(WORD *) (r + 4 + dstPitch2) = color;
    }
    srcPtr += srcPitch;
    dstPtr += dstPitch3;
  }
}

static void
TV2x(BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr, DWORD dstPitch, 
            int width, int height)
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  scaler_data_type *p = (scaler_data_type*)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*)dstPtr;

  while(height--) {
    for (i = 0, j = 0; i < width; ++i, j += 2) {
      scaler_data_type p1 = *(p + i);
      scaler_data_type p2 = *(p + i + nextlineSrc);
      scaler_data_type pi = ((INTERPOLATE(p1, p2) & colorMask) >> 1);

      *(q + j) = p1;
      *(q + j + 1) = p1;
      *(q + j + nextlineDst) = pi;
      *(q + j + nextlineDst + 1) = pi;
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}

static void
TimexTV(BYTE *srcPtr, DWORD srcPitch, BYTE *null, BYTE *dstPtr, DWORD dstPitch, 
            int width, int height)
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  scaler_data_type *p = (scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type *)dstPtr;

  while(height--) {
    if( ( height & 1 ) == 0 ) {
      for (i = 0, j = 0; i < width; ++i, j++ ) {
        scaler_data_type p1 = *(p + i);
        scaler_data_type p2 = *(p + i + nextlineSrc + nextlineSrc);
        scaler_data_type pi = ((INTERPOLATE(p1, p2) & colorMask) >> 1);

        *(q + j) = p1;
        *(q + j + nextlineDst) = pi;
      }
      q += nextlineDst << 1;
    }
    p += nextlineSrc;
  }
}
