/* audio2tape.c: Convert audio files (wav, voc, etc.) to tape files (.tzx,.csw)
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

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: fredm@spamcop.net

*/

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include <audiofile.h>

#include "utils.h"

typedef  libspectrum_byte (*glm) ( libspectrum_byte byte );

static int read_tape( glm get_level_method, char *filename, libspectrum_tape **tape );
static int write_tape( char *filename, libspectrum_tape *tape );

static void collect_stats( libspectrum_byte byte );
static void display_stats( void );

char *progname;

int show_stats = 0;

libspectrum_byte simple_crossover_point = 127;

static libspectrum_byte
get_level_simple( libspectrum_byte byte )
{
  return byte > simple_crossover_point;
}

libspectrum_byte schmitt_low2high_crossover_point = 0xc3;
libspectrum_byte schmitt_high2low_crossover_point = 0x3c;

static libspectrum_byte
get_level_schmitt( libspectrum_byte byte )
{
  static libspectrum_byte last_level = 0;

  switch( last_level ) {
  case 0:
    if( byte >= schmitt_low2high_crossover_point )
      last_level = 1;
    break;
  case 1:
    if( byte < schmitt_high2low_crossover_point )
      last_level = 0;
    break;
  }

  return last_level;
}

const char *schmitt = "schmitt";
const char *simple = "simple";

static int
set_trigger_type_from_string( glm* get_level_method, const char *string )
{
  if( strncmp( string, simple, strlen(simple) ) ) {
    *get_level_method = get_level_simple;
  } else if( strncmp( string, schmitt, strlen(schmitt) ) ) {
    *get_level_method = get_level_schmitt;
  } else {
    fprintf( stderr, "%s: unrecognised trigger type: %s\n", progname, string );
    return 1;
  }
  
  return 0;
}

int
main( int argc, char **argv )
{
  int c, error;
  char *trigger_type_string = NULL; glm get_level_method;
  libspectrum_tape *tzx;

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;

  get_level_method = get_level_simple;

  while( ( c = getopt( argc, argv, "t:sp:h:l:" ) ) != -1 ) {

    switch( c ) {

    case 't': trigger_type_string = optarg; break;
    case 's': show_stats = 1; break;
    case 'p': simple_crossover_point = atol( optarg ); break;
    case 'h': schmitt_low2high_crossover_point = atol( optarg ); break;
    case 'l': schmitt_high2low_crossover_point = atol( optarg ); break;

    }
  }

  get_level_method = get_level_simple;
  if( trigger_type_string &&
      set_trigger_type_from_string( &get_level_method, trigger_type_string ) )
    return 1;

  argc -= optind;
  argv += optind;

  if( argc < 2 ) {
    fprintf( stderr,
             "%s: usage: %s [-t <trigger type>] <infile> <outfile>\n",
             progname,
	     progname );
    return 1;
  }

  if( read_tape( get_level_method, argv[0], &tzx ) ) return 1;

  if( write_tape( argv[1], tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  libspectrum_tape_free( tzx );

  return 0;
}

unsigned int amplitude_frequency_table[256];

static void
collect_stats( libspectrum_byte byte )
{
  amplitude_frequency_table[byte]++;
}

static void
display_stats( void )
{
  int i, j;
  printf("Frequency table:\n");
  for( i = 0; i < 32; i++ ) {
    int subtotal = 0;
    for( j = 0; j < 8; j++ ) {
      subtotal += amplitude_frequency_table[i*8+j];
    }
    printf( "%3d-%3d | %d\n", i*8, i*8+7, subtotal );
  }
}

static int
read_tape( glm get_level_method, char *filename, libspectrum_tape **tape )
{
  libspectrum_byte *buffer; size_t length;
  libspectrum_byte *tape_buffer; size_t tape_length;
  libspectrum_error error;

  /* Our filehandle from libaudiofile */
  AFfilehandle handle;

  /* The track we're using in the file */
  int track = AF_DEFAULT_TRACK; 

  libspectrum_tape_alloc( tape );

  handle = afOpenFile( filename, "r", NULL );
  if( handle == AF_NULL_FILEHANDLE ) {
    libspectrum_tape_free( *tape );
    return 1;
  }

  if( afSetVirtualSampleFormat( handle, track, AF_SAMPFMT_UNSIGNED, 8 ) ) {
    libspectrum_tape_free( *tape );
    afCloseFile( handle );
    return 1;
  }

  length = afGetFrameCount( handle, track );

  tape_length = length;
  if( tape_length%8 ) tape_length += 8 - (tape_length%8);

  buffer = calloc( tape_length, 1);
  if( !buffer ) {
    libspectrum_tape_free( *tape );
    afCloseFile( handle );
    return 1;
  }

  int frames = afReadFrames( handle, track, buffer, length );
  if( frames == -1 ) {
    fprintf( stderr, "Can't calculate number of frames\n" );
    free( buffer );
    libspectrum_tape_free( *tape );
    afCloseFile( handle );
    return 1;
  }

  if( !length ) {
    fprintf( stderr, "Empty audiofile, nothing to convert\n" );
    free( buffer );
    libspectrum_tape_free( *tape );
    afCloseFile( handle );
    return 1;
  }

  if( frames != length ) {
    fprintf( stderr, "Read %d frames, but expected %lu\n", frames,
	     (unsigned long)length );
    free( buffer );
    libspectrum_tape_free( *tape );
    afCloseFile( handle );
    return 1;
  }

  /* Get a new block */
  libspectrum_tape_block* block;
  libspectrum_tape_block_alloc( &block, LIBSPECTRUM_TAPE_BLOCK_RAW_DATA );

  /* 44100 Hz 79 t-states 22050 Hz 158 t-states */
  libspectrum_tape_block_set_bit_length( block,
                                         3500000/afGetRate( handle, track ) );
  libspectrum_tape_block_set_pause     ( block, 0 );
  libspectrum_tape_block_set_bits_in_last_byte( block, length%8 ? length%8 : 8 );
  libspectrum_tape_block_set_data_length( block, tape_length/8 );

  tape_buffer = calloc( tape_length/8, 1 );

  libspectrum_byte *from = buffer;
  libspectrum_byte *to = tape_buffer;
  length = tape_length;
  do {
    libspectrum_byte val = 0;
    int i;
    for( i = 7; i >= 0; i-- ) {
      collect_stats( *from );
      if( get_level_method( *from++ ) ) val |= 1 << i;
    }
    *to++ = val;
  } while ((length -= 8) > 0);

  if( show_stats ) display_stats();

  libspectrum_tape_block_set_data( block, tape_buffer );

  /* Finally, put the block into the block list */
  error = libspectrum_tape_append_block( *tape, block );
  if( error ) {
    libspectrum_tape_block_free( block );
    return error;
  }

  if( afCloseFile( handle ) ) {
    free( buffer );
    return 1;
  }

  free( buffer );

  return 0;
}

static int
write_tape( char *filename, libspectrum_tape *tape )
{
  libspectrum_byte *buffer; size_t length;
  FILE *f;
  libspectrum_id_t type;
  libspectrum_class_t class;
  int error;

  /* Work out what sort of file we want from the filename; default to
     .tzx if we couldn't guess */
  error = libspectrum_identify_file_with_class( &type, &class, filename, NULL,
						0 );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_TAPE || type == LIBSPECTRUM_ID_UNKNOWN )
    type = LIBSPECTRUM_ID_TAPE_TZX;

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
