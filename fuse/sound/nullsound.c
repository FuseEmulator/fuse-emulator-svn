/* nullsound.c: dummy sound routines
   Copyright (c) 2003-2004 Philip Kendall

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

*/

#include <config.h>

#include "lowlevel.h"

/* Dummy functions for when we don't have a sound device; should never be
   called, so just abort if they are */

#ifndef HAVE_SOUND

#include "fuse.h"

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  fuse_abort();
}

void
sound_lowlevel_end( void )
{
  fuse_abort();
}

void
sound_lowlevel_frame( unsigned char *data, int len )
{
  fuse_abort();
}

#endif			/* #ifndef HAVE_SOUND */
