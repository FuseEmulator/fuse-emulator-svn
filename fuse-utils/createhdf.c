/* createhdf.c: Create an empty .hdf file
   Copyright (c) 2004 Philip Kendall

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

   Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *progname;

#define HDF_HEADER_LENGTH 0x80

enum {

  HDF_SIGNATURE_OFFSET   = 0x00,
  HDF_VERSION_OFFSET     = 0x07,
  HDF_COMPACT_OFFSET     = 0x08,
  HDF_DATA_OFFSET_OFFSET = 0x09,
  HDF_IDENTITY_OFFSET    = 0x16,

};

static const char *HDF_SIGNATURE = "RS-IDE\x1a";
static const size_t HDF_SIGNATURE_LENGTH = 7;
static const char HDF_VERSION = '\x10';
static const size_t HDF_DATA_OFFSET = 0x80;

enum {

  IDENTITY_CYLINDERS_OFFSET = 0x02,
  IDENTITY_HEADS_OFFSET     = 0x06,
  IDENTITY_SECTORS_OFFSET   = 0x0c,

};

#define CHUNK_LENGTH 1 << 20

int
parse_options( int argc, char **argv, size_t *cylinders, size_t *heads,
	       size_t *sectors, int *compact, int *sparse,
	       const char **filename )
{
  int c;

  while( ( c = getopt( argc, argv, "cs" ) ) != -1 ) {

    switch( c ) {

    case 'c': *compact = 1; break;
    case 's': *sparse = 0; break;

    }
  }

  if( argc - optind < 4 ) {
    fprintf( stderr,
	     "%s: usage: %s [-c] [-s] <cylinders> <heads> <sectors> <hdf>\n",
	     progname, progname );
    return 1;
  }

  *cylinders = atoi( argv[ optind++ ] );
  *heads     = atoi( argv[ optind++ ] );
  *sectors   = atoi( argv[ optind++ ] );
  *filename  = argv[ optind++ ];

  return 0;
}

int
write_header( FILE *f, size_t cylinders, size_t heads, size_t sectors,
	      int compact, const char *filename )
{
  char hdf_header[ HDF_HEADER_LENGTH ], *identity;
  size_t bytes;

  memset( hdf_header, 0, HDF_HEADER_LENGTH );

  memcpy( &hdf_header[ HDF_SIGNATURE_OFFSET ], HDF_SIGNATURE,
	  HDF_SIGNATURE_LENGTH );
  hdf_header[ HDF_VERSION_OFFSET ] = HDF_VERSION;
  hdf_header[ HDF_COMPACT_OFFSET ] = compact;
  hdf_header[ HDF_DATA_OFFSET_OFFSET     ] = ( HDF_DATA_OFFSET      ) & 0xff;
  hdf_header[ HDF_DATA_OFFSET_OFFSET + 1 ] = ( HDF_DATA_OFFSET >> 8 ) & 0xff;

  identity = &hdf_header[ HDF_IDENTITY_OFFSET ];

  identity[ IDENTITY_CYLINDERS_OFFSET     ] = ( cylinders      ) & 0xff;
  identity[ IDENTITY_CYLINDERS_OFFSET + 1 ] = ( cylinders >> 8 ) & 0xff;

  identity[ IDENTITY_HEADS_OFFSET         ] = ( heads          ) & 0xff;
  identity[ IDENTITY_HEADS_OFFSET     + 1 ] = ( heads     >> 8 ) & 0xff;

  identity[ IDENTITY_SECTORS_OFFSET       ] = ( sectors        ) & 0xff;
  identity[ IDENTITY_SECTORS_OFFSET   + 1 ] = ( sectors   >> 8 ) & 0xff;

  bytes = fwrite( hdf_header, 1, HDF_HEADER_LENGTH, f );
  if( bytes != HDF_HEADER_LENGTH ) {
    fprintf( stderr,
	     "%s: could write only %lu header bytes out of %lu to '%s'\n",
	     progname, (unsigned long)bytes, (unsigned long)HDF_HEADER_LENGTH,
	     filename );
    return 1;
  }

  return 0;
}

int
write_data( FILE *f, size_t cylinders, size_t heads, size_t sectors,
	    int compact, int sparse, const char *filename )
{
  size_t data_size;

  data_size = cylinders * heads * sectors * ( compact ? 0x100 : 0x200 );

  if( sparse ) {

    if( fseek( f, data_size - 1, SEEK_CUR ) ) {
      fprintf( stderr, "%s: error seeking in '%s': %s\n", progname, filename,
	       strerror( errno ) );
      return 1;
    }

    if( fputc( '\0', f ) == EOF ) {
      fprintf( stderr, "%s: error terminating null to '%s'\n", progname,
	       filename );
      return 1;
    }

  } else {

    char buffer[ CHUNK_LENGTH ];
    size_t total_written;

    memset( buffer, 0, CHUNK_LENGTH );
    total_written = 0;

    while( data_size ) {

      char buffer[ CHUNK_LENGTH ];
      size_t bytes_to_write, bytes_written;

      bytes_to_write = data_size > CHUNK_LENGTH ? CHUNK_LENGTH : data_size;

      bytes_written = fwrite( buffer, 1, bytes_to_write, f );
      if( bytes_written != bytes_to_write ) {
	fprintf( stderr, "%s: could write only %lu bytes out of %lu to '%s'",
		 progname, (unsigned long)total_written + bytes_written,
		 (unsigned long)data_size, filename );
	return 1;
      }

      total_written += bytes_to_write;
      data_size -= bytes_to_write;
    }
  }

  return 0;
}

int
main( int argc, char **argv )
{
  size_t cylinders, heads, sectors;
  int compact, sparse;
  const char *filename;
  FILE *f;
  int error;

  progname = argv[0];

  compact = 0;
  sparse = 1;
  error = parse_options( argc, argv, &cylinders, &heads, &sectors, &compact,
			 &sparse, &filename );
  if( error ) return error;

  f = fopen( filename, "wb" );
  if( !f ) {
    fprintf( stderr, "%s: error opening '%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  error = write_header( f, cylinders, heads, sectors, compact, filename );
  if( error ) return error;

  error = write_data( f, cylinders, heads, sectors, compact, sparse,
		      filename );
  if( error ) return error;

  return 0;
}
