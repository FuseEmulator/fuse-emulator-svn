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
static int read_tape( char *filename, libspectrum_tape **tape );
static int write_tape( char *filename, libspectrum_tape *tape );

char *progname;

int
main( int argc, char **argv )
{
  int error;
  libspectrum_tape *tzx;

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;

  if( argc < 2 ) {
    fprintf( stderr,
             "%s: usage: %s <infile> <outfile>\n",
             progname,
	     progname );
    return 1;
  }

  if( read_tape( argv[1], &tzx ) ) return 1;

  if( write_tape( argv[2], tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  libspectrum_tape_free( tzx );

  return 0;
}

static int
get_type_from_string( libspectrum_id_t *type, const char *string )
{
  libspectrum_class_t class;
  int error;

  /* Work out what sort of file we want from the filename; default to
     .tzx if we couldn't guess */
  error = libspectrum_identify_file_with_class( type, &class, string, NULL,
						0 );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_TAPE || type == LIBSPECTRUM_ID_UNKNOWN )
    *type = LIBSPECTRUM_ID_TAPE_TZX;

  return 0;
}
  
static int
read_tape( char *filename, libspectrum_tape **tape )
{
  libspectrum_byte *buffer; size_t length;

  if( mmap_file( filename, &buffer, &length ) ) return 1;

  if( libspectrum_tape_alloc( tape ) ) {
    munmap( buffer, length );
    return 1;
  }

  if( libspectrum_tape_read( *tape, buffer, length, LIBSPECTRUM_ID_UNKNOWN,
                             filename ) ) {
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
write_tape( char *filename, libspectrum_tape *tape )
{
  libspectrum_byte *buffer; size_t length;
  FILE *f;
  libspectrum_id_t type;

  if( get_type_from_string( &type, filename ) ) return 1;

  length = 0;

  if( libspectrum_tape_write( &buffer, &length, tape, type ) ) return 1;

  f = fopen( filename, "wb" );
  if( !f ) {
    fprintf( stderr, "%s: couldn't open '%s': %s\n", progname, filename,
	     strerror( errno ) );
    free( buffer );
    return 1;
  }
    
  if( fwrite( buffer, 1, length, f ) != length ) {
    fprintf( stderr, "%s: error writing to '%s'\n", progname, filename );
    free( buffer );
    fclose( f );
    return 1;
  }

  free( buffer );

  if( fclose( f ) ) {
    fprintf( stderr, "%s: couldn't close '%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  return 0;
}
