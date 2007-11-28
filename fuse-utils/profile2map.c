/* profile2map.c: convert Fuse profiler output into the Z80 map format
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

   Philip: philip-fuse@shadowmagic.org.uk

*/

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <libspectrum.h>

#include "utils.h"

/* argv[0] */
char *progname;

libspectrum_byte map[ 0x10000 / 8 ];

int main( int argc, char **argv )
{
  char *profile, *mapfile;
  FILE *f;
  long address, count;
  int ret;

  progname = argv[0];

  if( argc != 3 ) {
    fprintf( stderr, "%s: usage: %s <profile> <map>\n", progname, progname );
    return 1;
  }

  profile = argv[1];
  mapfile = argv[2];

  f = fopen( profile, "r" );
  if( !f ) {
    fprintf( stderr, "%s: unable to open profile map '%s' for reading\n",
	     progname, profile );
    return 1;
  }

  memset( map, 0, sizeof( map ) );

  while( 1 ) {
    ret = fscanf( f, "%li,%li\n", &address, &count );
    if( ret == EOF ) break;

    if( count && address >= 0 && address < 0x10000 )
      map[ address / 8 ] |= ( 1 << ( address % 8 ) );
  }

  fclose( f );

  f = fopen( mapfile, "wb" );
  if( !f ) {
    fprintf( stderr, "%s: unable to open Z80 map '%s' for writing\n", progname,
	     mapfile );
    return 1;
  }

  if( fwrite( map, sizeof( map ), 1, f ) != 1 )
    fprintf( stderr, "%s: failed to write Z80 map '%s'\n", progname, mapfile );

  fclose( f );

  return 0;
}
