/* image.c: disk image handling routines
   Copyright (c) 2007 Stuart Brady

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

   E-mail: sdbrady@ntlworld.com

*/

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <libdsk.h>

#include "internals.h"

libgdos_disk *
libgdos_openimage( const char *filename )
{
  libgdos_disk *d;
  int l;

  dsk_format_t fmt;
  dsk_err_t dsk_error;

  l = strlen( filename );
  if( l >= 5 ) {
    if( !strcasecmp( filename + ( l - 4 ), ".dsk" ) ||
	!strcasecmp( filename + ( l - 4 ), ".mgt" )    ) {
      fmt = FMT_800K;
    } else if( !strcasecmp( filename + ( l - 4 ), ".img" ) ) {
      fmt = FMT_MGT800;
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }

  d = malloc( sizeof(*d) );
  if( !d ) return NULL;

  /* First, try the "logical" driver. */
  dsk_error = dsk_open( &d->driver, filename, "logical", NULL );

  /* If it's not available, and we're using FMT_800K (an out-and-out format)
   * try the "raw" driver. */
  if( dsk_error != DSK_ERR_OK && fmt == FMT_800K ) {
    dsk_error = dsk_open( &d->driver, filename, "raw", NULL );
  }

  if( dsk_error != DSK_ERR_OK ) {
    free( d );
    return NULL;
  }

  dsk_error = dg_stdformat( &d->geom, fmt, NULL, NULL );
  if( dsk_error != DSK_ERR_OK ) {
    dsk_close( &d->driver );
    free( d );
    return NULL;
  }

  return d;
}

int
libgdos_closeimage( libgdos_disk *d )
{
  dsk_close( &d->driver );
  free( d );
  return 0;
}


int
libgdos_readsector( libgdos_disk *disk, uint8_t *buffer,
		    int track, int sector ) {
  int error;
  error = dsk_pread( disk->driver, &disk->geom, buffer,
		     track & 0x7f, !!( track & 0x80 ), sector );

  return error != DSK_ERR_OK;
}
