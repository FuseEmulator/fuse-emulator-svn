/* screenshot.h: Routines for saving .png screenshots
   Copyright (c) 2002 Philip Kendall

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

#ifndef FUSE_SCREENSHOT_H
#define FUSE_SCREENSHOT_H

#ifndef SCALER_H
#include "ui/scaler/scaler.h"
#endif				/* #ifndef SCALER_H */

#ifdef USE_LIBPNG

int screenshot_save( void );
int screenshot_write( const char *filename, scaler_type scaler );
int screenshot_write_fast( const char *filename, scaler_type scaler );
int screenshot_available_scalers( scaler_type scaler );

#endif				/* #ifdef USE_LIBPNG */

int screenshot_scr_write( const char *filename );
int screenshot_scr_read( const char *filename );

extern char screenshot_movie_name[];
extern char screenshot_movie_file[];
extern long int screenshot_movie_frame;
extern int screenshot_movie_record;
extern scaler_type screenshot_movie_scaler;

#endif				/* #ifndef FUSE_SCREENSHOT_H */
