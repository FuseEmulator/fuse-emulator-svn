/* file.c: various routines
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

libgdos_file *
libgdos_fopennum( libgdos_dir *dir, int n )
{
  libgdos_dirent entry;
  int error;

  error = libgdos_getentnum( dir, n, &entry );
  if( error ) return NULL;

  return libgdos_fopenent( &entry );
}

libgdos_file *
libgdos_fopenent( libgdos_dirent *entry )
{
  libgdos_file *file;

  if( entry->ftype == libgdos_ftype_unidos_subdir ||
      entry->ftype == libgdos_ftype_mdos_subdir      ) {
    return NULL;
  }

  file = malloc( sizeof *file );
  if( !file ) return NULL;

  file->disk = entry->disk;
  file->offset = 0;
  file->length = entry->numsectors * 0x1fe;
  file->track = entry->track;
  file->sector = entry->sector;
  file->secofs = 0;

  return file;
}

int
libgdos_fclose( libgdos_file *file )
{
  free( file );
  return 0;
}

int
libgdos_fread( uint8_t *buf, int count, libgdos_file *file )
{
  uint8_t sector[ 0x200 ];
  int toread;
  int numread = 0;

  if( file->offset + count > file->length )
    count = file->length - file->offset;

  while( count > 0 ) {
    if( libgdos_readsector( file->disk, sector, file->track, file->sector ) )
      break;

    if( file->secofs + count < 0x1fe ) {
      toread = count;
    } else {
      toread = 0x1fe - file->secofs;
    }

    memcpy( buf, sector + file->secofs, toread );
    file->offset += toread;
    file->secofs += toread;
    buf += toread;
    count -= toread;
    numread += toread;

    if( file->secofs == 0x1fe ) {
      file->track = sector[0x1fe];
      file->sector = sector[0x1ff];
      file->secofs = 0;
    }
  }

  return numread;
}
