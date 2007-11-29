/* gdosread.c: read a file from a GDOS disk image

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

   Stuart: sdbrady@ntlworld.com

*/

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <libgdos.h>

const char *progname;

int
main( int argc, const char **argv )
{
  libgdos_disk *disk;
  libgdos_dir *dir;
  libgdos_dirent entry;
  libgdos_file *file;
  uint8_t buf[ 0x1fe ];
  int numread, toread, length, error;
  long slot;

  progname = argv[0];

  if( argc != 3 ) {
    fprintf( stderr, "%s: usage: %s <file> <slot>\n", progname, progname );
    return 1;
  }

  errno = 0;
  slot = strtol( argv[2], NULL, 10 );
  if( errno != 0 ) {
    fprintf( stderr, "%s: invalid slot number: %s\n", progname, argv[2] );
    return 1;
  }

  disk = libgdos_openimage( argv[1] );
  if( !disk ) {
    /* XXX */
    fprintf( stderr, "%s: failed to open image\n", progname );
    return 1;
  }

  dir = libgdos_openrootdir( disk );
  if( !dir ) {
    fprintf( stderr, "%s: failed to open root directory\n", progname );
    libgdos_closeimage( disk );
    return 1;
  }

  error = libgdos_getentnum( dir, slot, &entry );
  if( error ) {
    fprintf( stderr, "%s: error reading slot %li\n", progname, slot );
    libgdos_closedir( dir );
    libgdos_closeimage( disk );
    return 1;
  }

  libgdos_closedir( dir );

  file = libgdos_fopenent( &entry );
  if( !file ) {
    fprintf( stderr, "%s: failed to open file\n", progname );
    libgdos_closeimage( disk );
    return 1;
  }

  /* Write out the register dump for snapshots */
  if( entry.ftype == libgdos_ftype_zx_snap48 ||
      entry.ftype == libgdos_ftype_zx_snap128 )
    fwrite( entry.ftypeinfo + 10, 1, 22, stdout );

  switch( entry.ftype ) {

  case libgdos_ftype_zx_snap48:
    length = 0xc000;
    break;

  case libgdos_ftype_zx_snap128:
    length = 0x20001;
    break;

  case libgdos_ftype_zx_screen:
    length = 0x1b00;
    numread = libgdos_fread( buf, 9, file );
    if( numread != 9 ) break;
    break;

  default:
    length = -1;
    break;

  }

  while( length != 0 ) {

    if( length > 0 && length < 0x1fe )
      toread = length;
    else
      toread = 0x1fe;

    numread = libgdos_fread( buf, toread, file );
    if( !numread ) break;

    fwrite( buf, 1, numread, stdout );
    if( length > 0 )
      length -= numread;

  }

  libgdos_fclose( file );
  libgdos_closeimage( disk );

  return 0;
}
