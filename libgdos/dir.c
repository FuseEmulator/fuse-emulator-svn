/* dir.c: directory handling routines
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

#include "internals.h"

libgdos_dir *
libgdos_openrootdir( libgdos_disk *disk )
{
  libgdos_dir *dir;

  dir = malloc( sizeof( *dir ) );
  if( !dir ) {
    return NULL;
  }

  dir->disk = disk;
  dir->length = 80 + disk->extra_dir_tracks * 20;
  /* track 4, sector 1 is reserved for the boot sector under Master DOS */
  if( dir->length > 80 && disk->variant == libgdos_variant_masterdos )
    dir->length -= 2;
  dir->current = 0;
  dir->track = 0;
  dir->sector = 1;
  dir->half = 0;

  dir->flags = 0;
  libgdos_set_dirflag( dir, libgdos_dirflag_skip_erased );
  libgdos_set_dirflag( dir, libgdos_dirflag_skip_hidden );
  libgdos_set_dirflag( dir, libgdos_dirflag_zero_terminate );

  return dir;
}

int
libgdos_readdir( libgdos_dir *dir, libgdos_dirent *entry )
{
  uint8_t sector[ 0x200 ];
  uint8_t *buf;

  if( !entry ) return 1;

  if( dir->current >= dir->length ) return 1;

  while( 1 ) {
    dir->current++;

    if( libgdos_readsector( dir->disk, sector, dir->track, dir->sector ) ) {
      return 1;
    }

    buf = sector + ( dir->half * 0x100 );

    dir->half++;
    if( dir->half > 1 ) {
      dir->half = 0;
      dir->sector++;
      if( dir->sector > 10 ) {
        /* next track */
	dir->sector = 1;
	dir->track++;
	if( ( dir->track & 0x7f ) >= 80 ) {
	  /* next side */
	  dir->track &= 0x80;
	  dir->track += 0x80;
	  if( dir->track >= 0x100 ) {
	    dir->track = 0;
	  }
	} else if( dir->track == 4 && dir->sector == 1 &&
		   dir->disk->variant == libgdos_variant_masterdos ) {
	  /* track 4, sector 1 is reserved for the boot sector
	   * under Master DOS */
	  dir->sector++;
	}
      }
    }

    entry->ftype = buf[0] & 0x3f;
    memcpy( entry->filename, &buf[1], 10 );

    if( entry->ftype == libgdos_ftype_erased ) {
      if( libgdos_test_dirflag( dir, libgdos_dirflag_zero_terminate ) &&
	  entry->filename[0] == '\0' ) {
	return 1;
      }

      if( libgdos_test_dirflag( dir, libgdos_dirflag_skip_erased ) ) {
	if( dir->current >= dir->length ) {
	  return 1;
	}
	continue;
      }
    }

    if( libgdos_test_dirflag( dir, libgdos_dirflag_skip_hidden ) &&
	entry->status & 0x02 ) {
      continue;
    }

    entry->disk = dir->disk;
    entry->slot = dir->current;
    entry->status = ( buf[0] >> 6 ) & 3;
    entry->numsectors = ( buf[11] << 8 ) | buf[12];
    entry->track = buf[13];
    entry->sector = buf[14];
    memcpy( entry->secmap, &buf[15], 195 );
    memcpy( entry->ftypeinfo, &buf[210], 46 );

    break;
  }

  return 0;
}

void
libgdos_closedir( libgdos_dir *dir )
{
  free( dir );
}

int
libgdos_getnumslots( libgdos_dir *dir )
{
  return dir->length;
}

void
libgdos_set_dirflag( libgdos_dir *dir, enum libgdos_dirflag flag )
{
  dir->flags |= ( 1 << flag );
}

void
libgdos_reset_dirflag( libgdos_dir *dir, enum libgdos_dirflag flag )
{
  dir->flags &= ~( 1 << flag );
}

int
libgdos_test_dirflag( libgdos_dir *dir, enum libgdos_dirflag flag )
{
  return !!( dir->flags & ( 1 << flag ) );
}

int
libgdos_getentnum( libgdos_dir *dir, int slot, libgdos_dirent *entry )
{
  int error;

  error = libgdos_readdir( dir, entry );
  if( error ) return error;
  while( entry->slot < slot ) {
    error = libgdos_readdir( dir, entry );
    if( error ) return error;
  }

  if( entry->slot != slot ) return 1;

  return 0;
}
