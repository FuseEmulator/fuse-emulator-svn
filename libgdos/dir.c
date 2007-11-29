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
  dir->length = 80;
  dir->current = 0;
  dir->track = 0;
  dir->sector = 1;
  dir->half = 0;

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
	dir->sector = 1;
	dir->track++;
	if( ( dir->track & 0x7f ) >= 80 ) {
	  dir->track &= 0x80;
	  dir->track += 0x80;
	  if( dir->track >= 0x100 ) {
	    dir->track = 0;
	  }
	}
      }
    }

    entry->ftype = buf[0] & 0x3f;
    memcpy( entry->filename, &buf[1], 10 );

    if( entry->ftype == libgdos_ftype_erased ) {
      if( /* dir->nullterminate && */ entry->filename[0] == '\0' ) {
	return 1;
      }

      if( /* !dir->readerased */ 1 ) {
	if( dir->current >= dir->length ) {
	  return 1;
	}
	continue;
      }
    }

    entry->disk = dir->disk;
    entry->slot = dir->current;
    entry->status = ( buf[0] >> 6 ) & 3;
    entry->numsectors = ( buf[11] << 8 ) | buf[12];
    entry->track = buf[13];
    entry->sector = buf[14];
    memcpy( entry->sab, &buf[15], 195 );
    memcpy( entry->ftypeinfo, &buf[220], 36 );

    break;
  }

  return 0;
}

void
libgdos_closedir( libgdos_dir *dir )
{
  free( dir );
}
