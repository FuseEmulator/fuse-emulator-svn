/* tapeconf.c: Convert .tzx files to .tap files
   Copyright (c) 2002-2003 Philip Kendall

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
#include <sys/mman.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

static int read_tape( char *filename, libspectrum_tape **tape );

char *progname;

int
main( int argc, char **argv )
{
  libspectrum_byte *buffer; size_t length;

  libspectrum_tape *tzx;

  progname = argv[0];

  if( argc < 2 ) {
    fprintf( stderr, "%s: no .tzx file given\n", progname );
    return 1;
  }

  /* Don't screw up people's terminals */
  if( isatty( STDOUT_FILENO ) ) {
    fprintf( stderr, "%s: won't output binary data to a terminal\n",
	     progname );
    return 1;
  }

  if( read_tape( argv[1], &tzx ) ) return 1;

  length = 0;
  if( libspectrum_tap_write( &buffer, &length, tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  if( fwrite( buffer, 1, length, stdout ) != length ) {
    free( buffer ); libspectrum_tape_free( tzx );
    return 1;
  }

  free( buffer ); libspectrum_tape_free( tzx );

  return 0;
}

static int
read_tape( char *filename, libspectrum_tape **tape )
{
  libspectrum_byte *buffer; size_t length;
  libspectrum_id_t type;

  if( mmap_file( filename, &buffer, &length ) ) return 1;

  if( libspectrum_identify_file( &type, filename, buffer, length ) ) {
    munmap( buffer, length );
    return 1;
  }

  if( libspectrum_tape_alloc( tape ) ) {
    munmap( buffer, length );
    return 1;
  }

  switch( type ) {

  case LIBSPECTRUM_ID_TAPE_TAP:
    if( libspectrum_tap_read( *tape, buffer, length ) ) {
      munmap( buffer, length ); libspectrum_tape_free( *tape );
      return 1;
    }
    break;

  case LIBSPECTRUM_ID_TAPE_TZX:
    if( libspectrum_tzx_read( *tape, buffer, length ) ) {
      munmap( buffer, length ); libspectrum_tape_free( *tape );
      return 1;
    }
    break;

  case LIBSPECTRUM_ID_UNKNOWN:
    fprintf( stderr, "%s: couldn't identify file type of `%s'\n", progname,
	     filename );
    munmap( buffer, length ); libspectrum_tape_free( *tape );
    return 1;

  default:
    fprintf( stderr, "%s: `%s' is not a tape file\n", progname, filename );
    munmap( buffer, length ); libspectrum_tape_free( *tape );
    return 1;

  }

  if( munmap( buffer, length ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, filename,
	     strerror( errno ) );
    libspectrum_tape_free( *tape );
    return 1;
  }

  return 0;
}
