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
  uint8_t sector[ 0x200 ];

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

  d->allocmap = NULL;

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

  libgdos_readsector( d, sector, 0, 1 );
  if( sector[ 0xff ] <= 35 ) {
    d->extra_dir_tracks = sector[ 0xff ];
    if( sector[ 15 ] & 0x01 ) /* track 4, sector 1 is/was allocated */
      d->variant = libgdos_variant_masterdos;
    else
      d->variant = libgdos_variant_betados;
  } else {
    d->extra_dir_tracks = 0;
    d->variant = libgdos_variant_gdos;
  }

  return d;
}

int
libgdos_closeimage( libgdos_disk *d )
{
  if( d->allocmap) free( d->allocmap );
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

int
libgdos_genallocmap( libgdos_disk *disk ) {
  libgdos_dir *dir;
  int i, n;
  int error;
  libgdos_dirent entry;

  if( !disk->allocmap ) {
    disk->allocmap = calloc( 195, 1 );
    if( !disk->allocmap ) return 1;
  } else {
    memset( disk->allocmap, 0, 195 );
  }

  dir = libgdos_openrootdir( disk );
  if( !dir ) {
    free( disk->allocmap );
    disk->allocmap = NULL;
    return 1;
  }

  libgdos_reset_dirflag( dir, libgdos_dirflag_skip_erased );
  libgdos_reset_dirflag( dir, libgdos_dirflag_skip_hidden );
  libgdos_reset_dirflag( dir, libgdos_dirflag_zero_terminate );

  n = 0;

  while( 1 ) {
    error = libgdos_readdir( dir, &entry );
    if( error ) {
      if( n == libgdos_getnumslots( dir ) )
	break;
      free( disk->allocmap );
      disk->allocmap = NULL;
      libgdos_closedir( dir );
      return error;
    }
    n++;

    if( entry.ftype != libgdos_ftype_erased ) {
      for( i = 0; i < 195; i++ ) {
	disk->allocmap[ i ] |= entry.secmap[ i ];
      }
    }
  }

  libgdos_closedir( dir );
  return 0;
}

int
libgdos_findfreesector( libgdos_disk *disk, int *track, int *sector )
{
  int i, t, s;

  if( !disk->allocmap ) return -1;

  if( disk->variant == libgdos_variant_masterdos &&
      !( disk->allocmap[ 0 ] & 0x01 ) ) {
    /* track 4, sector 1 is used for the boot sector under Master DOS,
     * but not under Beta DOS */
    *track = 4;
    *sector = 1;
    return 0;
  }

  t = 4 + disk->extra_dir_tracks;
  i = ( t - 4 ) * 10;
  while( t < 208 ) {
    for( s = 1; s <= 10; s++ ) {
      if( !( disk->allocmap[ i / 8 ] & ( 1 << ( i % 8 ) ) ) ) {
	*track = t;
	*sector = s;
	return 0;
      }
      i++;
    }
    t++;
    if( t == 80 ) t = 128;
  }

  return 1;
}

int
libgdos_numfreesectors( libgdos_disk *disk )
{
  int i, j, s, numfree;

  if( !disk->allocmap ) return -1;

  numfree = 0;

  if( disk->extra_dir_tracks &&
      disk->variant == libgdos_variant_masterdos &&
      !( disk->allocmap[ 0 ] & 0x01 ) ) {
    /* track 4, sector 1 is used for the boot sector under Master DOS,
     * but not under Beta DOS */
    numfree++;
  }

  s = disk->extra_dir_tracks * 10;
  i = s / 8;
  j = s % 8;
  while( i < 195 ) {
    while( j < 8 ) {
      if( !( disk->allocmap[ i ] & ( 1 << j ) ) ) {
	numfree++;
      }
      j++;
    }
    i++;
  }

  return numfree;
}
