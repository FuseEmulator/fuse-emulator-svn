/* profile.c: Z80 profiler
   Copyright (c) 2005 Philip Kendall
		 
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

   E-mail: Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdlib.h>

#include <libspectrum.h>

#include "ui/ui.h"
#include "z80/z80.h"

int profile_active = 0;

static int total_tstates[ 0x10000 ];
static libspectrum_word profile_last_pc;
static libspectrum_dword profile_last_tstates;

void
profile_start( void )
{
  memset( total_tstates, 0, sizeof( total_tstates ) );

  profile_active = 1;
  profile_last_pc = z80.pc.w;
  profile_last_tstates = tstates;
}

void
profile_map( libspectrum_word pc )
{
  if( tstates - profile_last_tstates > 256 ) abort();

  total_tstates[ profile_last_pc ] += tstates - profile_last_tstates;

  profile_last_pc = z80.pc.w;
  profile_last_tstates = tstates;
}

void
profile_frame( libspectrum_dword frame_length )
{
  profile_last_tstates -= frame_length;
}

void
profile_finish( const char *filename )
{
  FILE *f;
  size_t i;

  f = fopen( filename, "wb" );
  if( !f ) {
    ui_error( UI_ERROR_ERROR, "unable to open profile map '%s' for writing",
	      filename );
    return;
  }

  for( i = 0; i < 0x10000; i++ ) {

    if( !total_tstates[ i ] ) continue;

    fprintf( f, "0x%04x,%d\n", i, total_tstates[ i ] );

  }

  profile_active = 0;
}
