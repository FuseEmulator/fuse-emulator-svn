/* display.h: Routines for printing the Spectrum's screen
   Copyright (c) 1999-2000 Philip Kendall

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

#ifndef FUSE_DISPLAY_H
#define FUSE_DISPLAY_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

/* The width and height of the Speccy's screen */
#define DISPLAY_WIDTH_COLS  32
#define DISPLAY_HEIGHT_ROWS 24

/* The width and height of the Speccy's screen */
/* Each main screen column can produce 16 pixels in hires mode */
#define DISPLAY_WIDTH         ( DISPLAY_WIDTH_COLS * 16 )
/* Each main screen row can produce only 8 pixels in any mode */
#define DISPLAY_HEIGHT        ( DISPLAY_HEIGHT_ROWS * 8 )

/* The width and height of the (emulated) border */
#define DISPLAY_BORDER_WIDTH_COLS  4
#define DISPLAY_BORDER_HEIGHT_COLS 3

/* The width and height of the (emulated) border */
/* Each main screen column can produce 16 pixels in hires mode */
#define DISPLAY_BORDER_WIDTH  ( DISPLAY_BORDER_WIDTH_COLS * 16 )
/* Aspect corrected border width */
#define DISPLAY_BORDER_ASPECT_WIDTH  ( DISPLAY_BORDER_WIDTH_COLS * 8 )
/* Each main screen column can produce only 8 pixels in any mode */
#define DISPLAY_BORDER_HEIGHT ( DISPLAY_BORDER_HEIGHT_COLS * 8 )

/* The width and height of the window we'll be displaying */
#define DISPLAY_SCREEN_WIDTH  ( DISPLAY_WIDTH + 2 * DISPLAY_BORDER_WIDTH  )
#define DISPLAY_SCREEN_HEIGHT ( DISPLAY_HEIGHT + 2 * DISPLAY_BORDER_HEIGHT )

/* The aspect ratio corrected display width */
#define DISPLAY_ASPECT_WIDTH  ( DISPLAY_SCREEN_WIDTH / 2 )

extern BYTE display_lores_border;
extern BYTE display_hires_border;

/* Offsets as to where the data and the attributes for each pixel
   line start */
extern WORD display_line_start[DISPLAY_HEIGHT];
extern WORD display_attr_start[DISPLAY_HEIGHT];

int display_init(int *argc, char ***argv);
void display_line(void);

void display_dirty( WORD address );
void display_plot8(int x, int y, BYTE data, BYTE ink, BYTE paper);
void display_plot16(int x, int y, WORD data, BYTE ink, BYTE paper);

void display_parse_attr(BYTE attr, BYTE *ink, BYTE *paper);

void display_set_lores_border(int colour);
void display_set_hires_border(int colour);
int display_dirty_border(void);

int display_frame(void);
void display_refresh_all(void);

#endif			/* #ifndef FUSE_DISPLAY_H */
