/* xui.c: Routines for dealing with the Xlib user interface
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

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifndef HAVE_LIBGTK		/* Use this iff we're not using GTK+ */

#include <stdio.h>

#include "fuse.h"
#include "x.h"
#include "sdisplay.h"
#include "xui.h"

int xui_init(int argc, char **argv, int width, int height)
{
  xdisplay_init(width, height);

  return 0;
}

int xui_end(void)
{
  return 0;
}

#endif				/* #ifndef HAVE_LIBGTK */
