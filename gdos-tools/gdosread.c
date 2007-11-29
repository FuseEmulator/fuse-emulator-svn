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
  libgdos_file *file;
  uint8_t buf[ 0x1fe ];
  int numread;
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

  file = libgdos_fopennum( dir, slot );
  if( !file ) {
    fprintf( stderr, "%s: failed to open file\n", progname );
    libgdos_closedir( dir );
    libgdos_closeimage( disk );
    return 1;
  }

  libgdos_closedir( dir );

  numread = libgdos_fread( buf, 0x1fe, file );
  while( numread ) {
    fwrite( buf, 1, numread, stdout );
    numread = libgdos_fread( buf, 0x1fe, file );
  }

  libgdos_fclose( file );
  libgdos_closeimage( disk );

  return 0;
}
