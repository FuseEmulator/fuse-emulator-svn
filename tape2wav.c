/* tape2wav.c: Convert tape files (tzx, tap, etc.) to .wav files
   Copyright (c) 2007 Fredrick Meunier

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

   E-mail: fredm@spamcop.net

*/

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libspectrum.h>

#include <audiofile.h>

#include "utils.h"

static int read_tape( char *filename, libspectrum_tape **tape );
static int write_tape( char *filename, libspectrum_tape *tape );

char *progname;
int sample_rate = 41000;

int
main( int argc, char **argv )
{
  int c, error;
  libspectrum_tape *tzx;

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;

  while( ( c = getopt( argc, argv, "r:" ) ) != -1 ) {

    switch( c ) {

    case 'r': sample_rate = abs( atol( optarg ) ); break;

    }

  }

  argc -= optind;
  argv += optind;

  if( argc < 2 ) {
    fprintf( stderr,
             "%s: usage: %s <infile> <outfile>\n",
             progname,
	     progname );
    return 1;
  }

  if( read_tape( argv[0], &tzx ) ) return 1;

  if( write_tape( argv[1], tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  libspectrum_tape_free( tzx );

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
  size_t tape_length = 0;
  libspectrum_error error;
  short level = 0; /* The last level output to this block */
  libspectrum_dword pulse_tstates = 0;
  libspectrum_dword balance_tstates = 0;
  int flags = 0;
  int framesWritten;
  AFfilehandle file;

  AFfilesetup setup = afNewFileSetup();

  unsigned int scale = 3500000/sample_rate;

  length = 8192;
  buffer = malloc( length );

  if( !buffer ) {
    fprintf( stderr, "%s: unable to allocate memory for conversion buffer\n",
             progname );
    return 1;
  }

  while( !(flags & LIBSPECTRUM_TAPE_FLAGS_STOP) ) {
    libspectrum_dword pulse_length = 0;
    size_t i;

    error = libspectrum_tape_get_next_edge( &pulse_tstates, &flags, tape );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      free( buffer );
      return 1;
    }

    balance_tstates += pulse_tstates;

    if( flags & LIBSPECTRUM_TAPE_FLAGS_NO_EDGE ) continue;

    pulse_length = balance_tstates / scale;
    balance_tstates = balance_tstates % scale;

    if( tape_length + pulse_length > length ) {
      length *= 2;
      buffer = realloc( buffer, length );
      if( !buffer ) {
        fprintf( stderr, "%s: unable to allocate memory for conversion buffer\n",
                 progname );
        return 1;
      }
    }

    for( i = 0; i < pulse_length; i++ ) {
      buffer[ tape_length++ ] = level ? 0xff : 0x00;
    }

    level = !level;
  }

  afInitFileFormat( setup, AF_FILE_WAVE );
  afInitChannels( setup, AF_DEFAULT_TRACK, 1 );
  afInitSampleFormat( setup, AF_DEFAULT_TRACK, AF_SAMPFMT_UNSIGNED, 8 );
  afInitRate( setup, AF_DEFAULT_TRACK, sample_rate );

  file = afOpenFile( filename, "w", setup );
  if( file == AF_NULL_FILEHANDLE ) {
    fprintf( stderr, "%s: unable to open file '%s' for writing\n", progname,
             filename );
    free( buffer );
    afFreeFileSetup( setup );
    return 1;
  }

  framesWritten = afWriteFrames( file, AF_DEFAULT_TRACK, buffer, tape_length );
  if(framesWritten != tape_length ) {
    fprintf( stderr,
             "%s: number of frames written does not match number of frames"
             " requested\n",
             progname );
    free( buffer );
    afFreeFileSetup( setup );
    return 1;
  }

  if( afCloseFile( file ) ) {
    fprintf( stderr, "%s: error closing wav file\n", progname );
    free( buffer );
    afFreeFileSetup( setup );
    return 1;
  }

  free( buffer );
  afFreeFileSetup( setup );

  return 0;
}
