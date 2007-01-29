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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

static int get_type_from_string( libspectrum_id_t *type, const char *string );
static int read_tape( char *filename, libspectrum_id_t type,
		      libspectrum_tape **tape );
static int write_tape( libspectrum_id_t type, libspectrum_tape *tape );

char *progname;

int
main( int argc, char **argv )
{
  int c, error;
  char *input_type_string = NULL; libspectrum_id_t input_type;
  char *output_type_string = NULL; libspectrum_id_t output_type;
  libspectrum_tape *tzx;

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;

  /* Don't screw up people's terminals */
  if( isatty( STDOUT_FILENO ) ) {
    fprintf( stderr, "%s: won't output binary data to a terminal\n",
	     progname );
    return 1;
  }

  while( ( c = getopt( argc, argv, "i:o:" ) ) != -1 ) {

    switch( c ) {

    case 'i': input_type_string = optarg; break;
    case 'o': output_type_string = optarg; break;

    }
  }

  input_type = LIBSPECTRUM_ID_UNKNOWN;
  if( input_type_string &&
      get_type_from_string( &input_type, input_type_string ) ) return 1;

  output_type = LIBSPECTRUM_ID_TAPE_TZX;
  if( output_type_string &&
      get_type_from_string( &output_type, output_type_string ) ) return 1;

  if( argv[optind] == NULL ) {
    fprintf( stderr, "%s: no tape file given\n", progname );
    return 1;
  }

  if( read_tape( argv[optind], input_type, &tzx ) ) return 1;

  if( write_tape( output_type, tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  libspectrum_tape_free( tzx );

  return 0;
}

static int
get_type_from_string( libspectrum_id_t *type, const char *string )
{
  if( !strcmp( string, "tap" ) ) {
    *type = LIBSPECTRUM_ID_TAPE_TAP;
  } else if( !strcmp( string, "tzx" ) ) {
    *type = LIBSPECTRUM_ID_TAPE_TZX;
  } else {
    fprintf( stderr, "%s: unknown format `%s'\n", progname, string );
    return 1;
  }
  
  return 0;
}
  
static int
read_tape( char *filename, libspectrum_id_t type, libspectrum_tape **tape )
{
  libspectrum_byte *buffer; size_t length;

  if( mmap_file( filename, &buffer, &length ) ) return 1;

  if( libspectrum_tape_alloc( tape ) ) {
    munmap( buffer, length );
    return 1;
  }

  if( libspectrum_tape_read( *tape, buffer, length, type, filename ) ) {
    munmap( buffer, length );
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

static int
write_tape( libspectrum_id_t type, libspectrum_tape *tape )
{
  libspectrum_byte *buffer; size_t length;

  length = 0;

  switch( type ) {

  case LIBSPECTRUM_ID_TAPE_TAP:
    if( libspectrum_tap_write( &buffer, &length, tape ) ) return 1;
    break;

  case LIBSPECTRUM_ID_TAPE_TZX:
    if( libspectrum_tzx_write( &buffer, &length, tape ) ) return 1;
    break;
    
  default:
    fprintf( stderr, "%s: unknown output format %d\n", progname, type );
    return 1;

  }

  if( fwrite( buffer, 1, length, stdout ) != length ) {
    free( buffer );
    return 1;
  }

  free( buffer );

  return 0;
}
