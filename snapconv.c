/* snapconv.c: Convert snapshot formats
   Copyright (c) 2003-2005 Philip Kendall

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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

#include <libspectrum.h>

#include "utils.h"

char *progname;

int
main( int argc, char **argv )
{
  libspectrum_snap *snap;
  libspectrum_id_t type; libspectrum_class_t class;
  unsigned char *buffer; size_t length;
  libspectrum_creator *creator;
  int flags;
  int compress = 0;
  FILE *f;

  int error;
  int c;

  progname = argv[0];

  while( ( c = getopt( argc, argv, "cn" ) ) != -1 ) {

    switch( c ) {

    case 'c': compress = LIBSPECTRUM_FLAG_SNAPSHOT_ALWAYS_COMPRESS;
      break;

    case 'n': compress = LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION;
      break;

    }
  }
  argc -= optind;
  argv += optind;

  if( argc < 2 ) {
    fprintf( stderr, "%s: usage: %s [-c] [-n] <infile> <outfile>\n", progname,
	     progname );
    return 1;
  }

  error = init_libspectrum(); if( error ) return error;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  if( mmap_file( argv[0], &buffer, &length ) ) {
    libspectrum_snap_free( snap );
    return 1;
  }

  error = libspectrum_snap_read( snap, buffer, length, LIBSPECTRUM_ID_UNKNOWN,
				 argv[0] );
  if( error ) {
    libspectrum_snap_free( snap ); munmap( buffer, length );
    return error;
  }

  if( munmap( buffer, length ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap '%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    libspectrum_snap_free( snap );
    return 1;
  }

  error = libspectrum_identify_file_with_class( &type, &class, argv[1], NULL,
                                                0 );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT ) {
    fprintf( stderr, "%s: '%s' is not a snapshot file\n", progname, argv[1] );
    libspectrum_snap_free( snap );
    return 1;
  }

  error = get_creator( &creator, "snapconv" );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  length = 0;
  error = libspectrum_snap_write( &buffer, &length, &flags, snap, type,
                                  creator, compress );
  if( error ) {
    libspectrum_creator_free( creator ); libspectrum_snap_free( snap );
    return error;
  }

  if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS ) {
    fprintf( stderr,
	     "%s: warning: major information loss during conversion\n",
	     progname );
  } else if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS ) {
    fprintf( stderr,
	     "%s: warning: minor information loss during conversion\n",
	     progname );
  }
  error = libspectrum_creator_free( creator );
  if( error ) { free( buffer ); libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap );
  if( error ) { free( buffer ); return error; }

  f = fopen( argv[1], "w" );
  if( !f ) {
    fprintf( stderr, "%s: couldn't open '%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    free( buffer );
    return 1;
  }
    
  if( fwrite( buffer, 1, length, f ) != length ) {
    fprintf( stderr, "%s: error writing to '%s'\n", progname, argv[1] );
    free( buffer );
    return 1;
  }

  free( buffer );
   
  return 0;
}
