/* uidisplay.c: UI display functions
   Copyright (c) 2002-2003 Philip Kendall

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

#include "display.h"
#include "machine.h"
#include "ui/uidisplay.h"

void uidisplay_spectrum_screen( const BYTE *screen, int border )
{
  int x,y;
  BYTE attr,data; int ink, paper;

  int scale = machine_current->timex ? 2 : 1;

  for( y=0; y < DISPLAY_BORDER_HEIGHT; y++ ) {
    for( x=0; x < DISPLAY_ASPECT_WIDTH; x++ ) {
      display_putpixel( x, y, border );
      display_putpixel( x, y + DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT,
			border );
    }
  }

  for( y=0; y<DISPLAY_HEIGHT; y++ ) {

    for( x=0; x < DISPLAY_BORDER_WIDTH; x++ ) {
      display_putpixel( x, y + DISPLAY_BORDER_HEIGHT, border );
      display_putpixel( x + DISPLAY_ASPECT_WIDTH - DISPLAY_BORDER_ASPECT_WIDTH,
			y + DISPLAY_BORDER_HEIGHT, border );
    }

    for( x=0; x < DISPLAY_WIDTH_COLS; x++ ) {

      /* Get the attribute byte */
      attr = screen[ display_attr_start[y] + x ];
      
      /* Split it into (possibly bright) INK and PAPER */
      ink = (attr & 0x07) + ( (attr & 0x40) >> 3 );
      paper = (attr & ( 0x0f << 3 ) ) >> 3;

      data = screen[ display_line_start[y]+x ];

      display_plot8( x, y, data, ink, paper );
    }
  }

  uidisplay_area( 0, 0, scale * DISPLAY_ASPECT_WIDTH,
		  scale * DISPLAY_SCREEN_HEIGHT );
}
