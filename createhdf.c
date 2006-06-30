/* createhdf.c: Create an empty .hdf file
   Copyright (c) 2004-2006 Philip Kendall

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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ide.h"

const char *progname;

#define CHUNK_LENGTH 1 << 20

int
parse_options( int argc, char **argv, size_t *cylinders, size_t *heads,
	       size_t *sectors, int *compact, int *sparse,
	       const char **filename, enum hdf_version_t *version )
{
  int c;

  while( ( c = getopt( argc, argv, "csv:" ) ) != -1 ) {

    switch( c ) {

    case 'c': *compact = 1; break;
    case 's': *sparse = 0; break;
    case 'v':
      if( strcasecmp( optarg, "1.0" ) == 0 ) {
	*version = HDF_VERSION_10;
      } else if( strcasecmp( optarg, "1.1" ) == 0 ) {
	*version = HDF_VERSION_11;
      }
      break;

    }
  }

  if( argc - optind < 4 ) {
    fprintf( stderr,
	     "%s: usage: %s [-c] [-s] [-v<version>] <cylinders> <heads> <sectors> <hdf>\n",
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
write_header( int fd, size_t cylinders, size_t heads, size_t sectors,
	      int compact, const char *filename, enum hdf_version_t version )
{
  char *hdf_header, *identity;
  size_t bytes, data_offset;

  data_offset = hdf_data_offset( version );

  hdf_header = calloc( 1, data_offset );
  if( !hdf_header ) {
    fprintf( stderr, "%s: out of memory at line %d\n", progname, __LINE__ );
    return 1;
  }

  memcpy( &hdf_header[ HDF_SIGNATURE_OFFSET ], HDF_SIGNATURE,
	  HDF_SIGNATURE_LENGTH );
  hdf_header[ HDF_VERSION_OFFSET ] = hdf_version( version );
  hdf_header[ HDF_COMPACT_OFFSET ] = compact;
  hdf_header[ HDF_DATA_OFFSET_OFFSET     ] = ( data_offset      ) & 0xff;
  hdf_header[ HDF_DATA_OFFSET_OFFSET + 1 ] = ( data_offset >> 8 ) & 0xff;

  identity = &hdf_header[ HDF_IDENTITY_OFFSET ];

  identity[ IDENTITY_CYLINDERS_OFFSET     ] = ( cylinders      ) & 0xff;
  identity[ IDENTITY_CYLINDERS_OFFSET + 1 ] = ( cylinders >> 8 ) & 0xff;

  identity[ IDENTITY_HEADS_OFFSET         ] = ( heads          ) & 0xff;
  identity[ IDENTITY_HEADS_OFFSET     + 1 ] = ( heads     >> 8 ) & 0xff;

  identity[ IDENTITY_SECTORS_OFFSET       ] = ( sectors        ) & 0xff;
  identity[ IDENTITY_SECTORS_OFFSET   + 1 ] = ( sectors   >> 8 ) & 0xff;

  bytes = write( fd, hdf_header, data_offset );
  if( bytes != data_offset ) {
    fprintf( stderr,
	     "%s: could write only %lu header bytes out of %lu to '%s'\n",
	     progname, (unsigned long)bytes, (unsigned long)data_offset,
	     filename );
    free( hdf_header );
    return 1;
  }

  free( hdf_header );

  return 0;
}

int
write_data( int fd, size_t cylinders, size_t heads, size_t sectors,
	    int compact, int sparse, const char *filename,
	    enum hdf_version_t version )
{
  size_t data_size;

  data_size = cylinders * heads * sectors * ( compact ? 0x100 : 0x200 );

  if( sparse ) {

    if( ftruncate( fd, data_size + hdf_data_offset( version ) ) ) {
      fprintf( stderr, "%s: error truncating '%s': %s\n", progname, filename,
	       strerror( errno ) );
      return 1;
    }

  } else {

    char buffer[ CHUNK_LENGTH ];
    size_t total_written;

    memset( buffer, 0, CHUNK_LENGTH );
    total_written = 0;

    while( data_size ) {

      size_t bytes_to_write, bytes_written;

      bytes_to_write = data_size > CHUNK_LENGTH ? CHUNK_LENGTH : data_size;

      bytes_written = write( fd, buffer, bytes_to_write );
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
  enum hdf_version_t version;
  const char *filename;
  int fd;
  int error;

  progname = argv[0];

  compact = 0;
  sparse = 1;
  version = HDF_VERSION_11;
  error = parse_options( argc, argv, &cylinders, &heads, &sectors, &compact,
			 &sparse, &filename, &version );
  if( error ) return error;

  fd = creat( filename, S_IRUSR | S_IWUSR |
			S_IRGRP | S_IWGRP |
			S_IROTH | S_IWOTH   );
  if( fd == -1 ) {
    fprintf( stderr, "%s: error opening '%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  error = write_header( fd, cylinders, heads, sectors, compact, filename,
			version );
  if( error ) return error;

  error = write_data( fd, cylinders, heads, sectors, compact, sparse,
		      filename, version );
  if( error ) return error;

  return 0;
}
