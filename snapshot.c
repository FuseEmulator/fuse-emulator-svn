/* snapshot.c: Snapshot handling routines
   Copyright (c) 2001-2003 Philip Kendall, Darren Salt

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

#include <string.h>

#include "internals.h"

/* Some flags which may be given to libspectrum_snap_write() */
const int LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION = 1 << 0;

/* Some flags which may be returned from libspectrum_snap_write() */
const int LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS = 1 << 0;
const int LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS = 1 << 1;

/* Free all memory used by a libspectrum_snap structure (destructor...) */
libspectrum_error
libspectrum_snap_free( libspectrum_snap *snap )
{
  int i;

  for( i = 0; i < 8; i++ )
    if( libspectrum_snap_pages( snap, i ) )
	free( libspectrum_snap_pages( snap, i ) );

  for( i = 0; i < 256; i++ ) {
    if( libspectrum_snap_slt_length( snap, i ) ) {
      free( libspectrum_snap_slt( snap, i ) );
      libspectrum_snap_set_slt_length( snap, i, 0 );
    }
  }
  if( libspectrum_snap_slt_screen( snap ) )
  free( libspectrum_snap_slt_screen( snap ) );

  for( i = 0; i < 64; i++ ) free( libspectrum_snap_zxcf_ram( snap, i ) );

  free( libspectrum_snap_interface2_rom( snap, 0 ) );

  free( snap );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Read in a snapshot, optionally guessing what type it is */
libspectrum_error
libspectrum_snap_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		       size_t length, libspectrum_id_t type,
		       const char *filename )
{
  libspectrum_id_t raw_type;
  libspectrum_class_t class;
  libspectrum_byte *new_buffer;
  libspectrum_error error;
  int uncompressed;

  /* If we don't know what sort of file this is, make a best guess */
  if( type == LIBSPECTRUM_ID_UNKNOWN ) {
    error = libspectrum_identify_file( &type, filename, buffer, length );
    if( error ) return error;

    /* If we still can't identify it, give up */
    if( type == LIBSPECTRUM_ID_UNKNOWN ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
	"libspectrum_snap_read: couldn't identify file"
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
  }

  error = libspectrum_identify_class( &class, type );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "libspectrum_snap_read: not a snapshot file" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Find out if this file needs decompression */
  uncompressed = 0; new_buffer = NULL;

  error = libspectrum_identify_file_raw( &raw_type, filename, buffer, length );
  if( error ) return error;

  error = libspectrum_identify_class( &class, raw_type );
  if( error ) return error;

  if( class == LIBSPECTRUM_CLASS_COMPRESSED ) {

    size_t new_length;

    error = libspectrum_uncompress_file( &new_buffer, &new_length, NULL,
					 raw_type, buffer, length, NULL );
    buffer = new_buffer; length = new_length;
    uncompressed = 1;
  }

  switch( type ) {

  case LIBSPECTRUM_ID_SNAPSHOT_PLUSD:
    error = libspectrum_plusd_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_SNA:
    error = libspectrum_sna_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_SNP:
    error = libspectrum_snp_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_SP:
    error = libspectrum_sp_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_SZX:
    error = libspectrum_szx_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_Z80:
    error = libspectrum_z80_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_ZXS:
    error = libspectrum_zxs_read( snap, buffer, length ); break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_snap_read: unknown snapshot type %d",
			     type );
    free( new_buffer );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  free( new_buffer );
  return error;
}

libspectrum_error
libspectrum_snap_write( libspectrum_byte **buffer, size_t *length,
			int *out_flags, libspectrum_snap *snap,
	 		libspectrum_id_t type, libspectrum_creator *creator,
			int in_flags )
{
  libspectrum_class_t class;
  libspectrum_error error;

  error = libspectrum_identify_class( &class, type );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID,
			     "libspectrum_snap_write: not a snapshot format" );
    return LIBSPECTRUM_ERROR_INVALID;
  }

  switch( type ) {

  case LIBSPECTRUM_ID_SNAPSHOT_SNA:
    return libspectrum_sna_write( buffer, length, out_flags, snap, in_flags );

  case LIBSPECTRUM_ID_SNAPSHOT_SZX:
    return libspectrum_szx_write( buffer, length, out_flags, snap, creator,
				  in_flags );

  case LIBSPECTRUM_ID_SNAPSHOT_Z80:
    return libspectrum_z80_write2( buffer, length, out_flags, snap, in_flags );

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "libspectrum_snap_write: format not supported" );
    return LIBSPECTRUM_ERROR_UNKNOWN;

  }
}

/* Given a 48K memory dump `data', place it into the
   appropriate bits of `snap' for a 48K machine */
int
libspectrum_split_to_48k_pages( libspectrum_snap *snap,
				const libspectrum_byte* data )
{
  libspectrum_byte *buffer[3];
  size_t i, j;

  /* If any of the three pages are already occupied, barf */
  if( libspectrum_snap_pages( snap, 5 ) ||
      libspectrum_snap_pages( snap, 2 ) ||
      libspectrum_snap_pages( snap, 0 )    ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_split_to_48k_pages: RAM page already in use"
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  for( i = 0; i < 3; i++ ) {
    buffer[i] = malloc( 0x4000 * sizeof( libspectrum_byte ) );

    if( !buffer[i] ) {
      for( j = 0; j < i; j++ ) free( buffer[i] );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_MEMORY,
	"libspectrum_split_to_48k_pages: out of memory"
      );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

  }

  libspectrum_snap_set_pages( snap, 5, buffer[0] );
  libspectrum_snap_set_pages( snap, 2, buffer[1] );
  libspectrum_snap_set_pages( snap, 0, buffer[2] );

  /* Finally, do the copies... */
  memcpy( libspectrum_snap_pages( snap, 5 ), &data[0x0000], 0x4000 );
  memcpy( libspectrum_snap_pages( snap, 2 ), &data[0x4000], 0x4000 );
  memcpy( libspectrum_snap_pages( snap, 0 ), &data[0x8000], 0x4000 );

  return LIBSPECTRUM_ERROR_NONE;
}
