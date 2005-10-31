/* raw2hdf.c: Create an .hdf file from a raw binary image
   Copyright (c) 2005 Matthew Westcott

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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ide.h"

const char *progname;

static const char *MODEL_NUMBER = "rCaeet dybr wah2fd                      ";
static const size_t MODEL_NUMBER_LENGTH = 40;

#define CHUNK_LENGTH 1 << 20

int
parse_options( int argc, char **argv, const char **raw_filename,
  const char **hdf_filename )
{

  if( argc != 3 ) {
    fprintf( stderr, "%s: usage: %s <raw-filename> <hdf-filename>\n",
             progname, progname );
    return 1;
  }

  *raw_filename  = argv[1];
  *hdf_filename  = argv[2];

  return 0;
}

int
write_header( FILE *f, size_t byte_count, const char *filename )
{

  char hdf_header[ HDF_HEADER_LENGTH ], *identity;
  size_t bytes, total_sectors, head_count, cyl_count, sector_count;
  size_t sectors_per_head;

  memset( hdf_header, 0, HDF_HEADER_LENGTH );

  memcpy( &hdf_header[ HDF_SIGNATURE_OFFSET ], HDF_SIGNATURE,
          HDF_SIGNATURE_LENGTH );
  hdf_header[ HDF_VERSION_OFFSET ] = HDF_VERSION;
  hdf_header[ HDF_COMPACT_OFFSET ] = 0;
  hdf_header[ HDF_DATA_OFFSET_OFFSET     ] = ( HDF_DATA_OFFSET      ) & 0xff;
  hdf_header[ HDF_DATA_OFFSET_OFFSET + 1 ] = ( HDF_DATA_OFFSET >> 8 ) & 0xff;

  identity = &hdf_header[ HDF_IDENTITY_OFFSET ];

  /* invent suitable geometry to match the file length */
  if( byte_count & 0x1f ) {
    fprintf( stderr,
      "Warning: image is not a whole number of sectors in length\n" );
  }
  total_sectors = byte_count >> 9;
  if( total_sectors >= 16514064 ) {
    /* image > 8GB; use dummy 'large disk' CHS values */
    cyl_count = 16383;
    head_count = 16;
    sector_count = 63;
  } else {
    /* find largest factor <= 16 to use as the head count */
    for( head_count = 16; head_count > 1; head_count-- ) {
      if( total_sectors % head_count == 0 ) break;
    }
    sectors_per_head = total_sectors / head_count;
    /* find largest factor <= 63 to use as the sector count */
    for( sector_count = 63; sector_count > 1; sector_count-- ) {
      if( sectors_per_head % sector_count == 0 ) break;
    }
    cyl_count = sectors_per_head / sector_count;
    if( cyl_count > 16384 ) {
      /* attempt to factorise into sensible CHS values failed dismally;
         fall back on dummy 'large disk' values */
      cyl_count = 16383;
      head_count = 16;
      sector_count = 63;
    }
  }

  identity[ IDENTITY_CYLINDERS_OFFSET     ] = ( cyl_count      ) & 0xff;
  identity[ IDENTITY_CYLINDERS_OFFSET + 1 ] = ( cyl_count >> 8 ) & 0xff;

  identity[ IDENTITY_HEADS_OFFSET     ] = ( head_count      ) & 0xff;
  identity[ IDENTITY_HEADS_OFFSET + 1 ] = ( head_count >> 8 ) & 0xff;

  identity[ IDENTITY_SECTORS_OFFSET     ] = ( sector_count      ) & 0xff;
  identity[ IDENTITY_SECTORS_OFFSET + 1 ] = ( sector_count >> 8 ) & 0xff;

  /* set 'LBA supported' flag */
  identity[ IDENTITY_CAPABILITIES_OFFSET + 1 ] = 0x02;

  memcpy( &identity[ IDENTITY_MODEL_NUMBER_OFFSET ], MODEL_NUMBER,
          MODEL_NUMBER_LENGTH );  

  rewind( f );
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
copy_data( FILE *from, FILE *to, size_t *byte_count, const char *from_filename,
  const char *to_filename )
{
  char buffer[ CHUNK_LENGTH ];
  size_t bytes_read;

  if( fseek( to, HDF_DATA_OFFSET, SEEK_SET ) ) {
    fprintf( stderr, "%s: error seeking in '%s': %s\n", progname,
             to_filename, strerror( errno ) );
    return 1;
  }
  
  *byte_count = 0;
  
  while( !feof( from ) ) {
    bytes_read = fread( buffer, 1, CHUNK_LENGTH, from );
    if( ferror( from ) ) {
      fprintf( stderr, "%s: error reading from '%s': %s\n", progname,
               from_filename, strerror( errno ) );
      return 1;
    }
    *byte_count += bytes_read;
    
    fwrite( buffer, 1, bytes_read, to );
    if( ferror( to ) ) {
      fprintf( stderr, "%s: error writing to '%s': %s\n", progname,
               to_filename, strerror( errno ) );
      return 1;
    }
  }

  return 0;
}

int
main( int argc, char **argv )
{
  const char *raw_filename, *hdf_filename;
  FILE *raw, *hdf;
  int error;
  size_t byte_count;

  progname = argv[0];

  error = parse_options( argc, argv, &raw_filename, &hdf_filename );
  if( error ) return error;

  raw = fopen( raw_filename, "rb" );
  if( !raw ) {
    fprintf( stderr, "%s: error opening '%s': %s\n", progname, raw_filename,
	     strerror( errno ) );
    return 1;
  }

  hdf = fopen( hdf_filename, "wb" );
  if( !hdf ) {
    fprintf( stderr, "%s: error opening '%s': %s\n", progname, hdf_filename,
	     strerror( errno ) );
    return 1;
  }

  error = copy_data( raw, hdf, &byte_count, raw_filename, hdf_filename );
  if( error ) return error;

  fclose( raw );

  error = write_header( hdf, byte_count, hdf_filename );
  if( error ) return error;

  fclose( hdf );

  return 0;
}
