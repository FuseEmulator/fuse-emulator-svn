/* xvideo.h: Routines for using the XVideo extension
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

#ifndef FUSE_XVIDEO_H
#define FUSE_XVIDEO_H

#ifdef X_USE_XV

extern XvPortID xvport;
extern XvImage *xvimage;
extern int xvid;
extern int xvideo_usable;
extern int xv_scaler;

extern int xvideo_dw, xvideo_dh, xvideo_isize;

#endif			/* #ifdef X_USE_XV */

void xvideo_find( void );
void xvideo_acquire( void );
void xvideo_hotswap_gfx_mode( void );
void xvideo_end( void );
int xvideo_scaler_in_use( void );
void xvideo_register_scalers( void );
void xvideo_create_image( void );
void xvideo_set_size( int width, int height );

#endif			/* #ifndef FUSE_XVIDEO_H */
