/* ggiui.c: Routines for dealing with the GGI user interface
   Copyright (c) 2003 Catalin Mihaila <catalin@idgrup.ro>

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

   Catalin: catalin@idgrup.ro

*/

#include "config.h"

#ifdef UI_GGI

#include <ggi/ggi.h>

#include "ui/uidisplay.h"
#include "ggi_internals.h"

int
ui_init( int *argc, char ***argv )
{
  return 0;
}

int
ui_event( void )
{
  uiggi_event();
  return 0;
}

int
ui_end( void )
{
  int error;

  error = uidisplay_end();
  if( error ) return error;

  ggiExit();

  return 0;
}

#endif				/* #ifdef UI_GGI */
