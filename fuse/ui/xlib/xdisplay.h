/* xdisplay.h: Routines for dealing with the X display
   Copyright (c) 2000 Philip Kendall

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

#ifndef FUSE_XDISPLAY_H
#define FUSE_XDISPLAY_H

int xdisplay_init( void );
int xdisplay_end( void );

int xdisplay_configure_notify(int width, int height);
void xdisplay_area(int x, int y, int width, int height);

int xdisplay_allocate_image( void );
void xdisplay_destroy_image( void );

typedef void xdisplay_putpixel_t( int x, int y, libspectrum_word *color );
extern xdisplay_putpixel_t *xdisplay_putpixel;
void xdisplay_setup_rgb_putpixel( void );

/* Are we expecting an X error to occur? */
extern int xerror_expecting;

/* Which error occurred? */
extern int xerror_error;

/* The X error handler */
int xerror_handler( Display *display, XErrorEvent *error );

extern int xdisplay_current_size;

#endif			/* #ifndef FUSE_XDISPLAY_H */
