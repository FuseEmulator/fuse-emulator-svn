/* fbkeyboard.h: routines for dealing with the buttons interface
   Copyright (c) 2000-2001 Philip Kendall, Matan Ziv-Av

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
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_FBKEYBOARD_H
#define FUSE_FBKEYBOARD_H

int fbkeyboard_init(void);

int fbkeyboard_keypress(int keysym);
void fbkeyboard_keyrelease(int keysym);

int fbkeyboard_end(void);

void keyboard_update(void);

#endif			/* #ifndef FUSE_FBKEYBOARD_H */
