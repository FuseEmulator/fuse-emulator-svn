/* sna.c: Routines for handling .sna snapshots
   Copyright (c) 2001-2002 Philip Kendall

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

#define LIBSPECTRUM_SNA_HEADER_LENGTH 27
#define LIBSPECTRUM_SNA_128_HEADER_LENGTH 4

static int identify_machine( size_t buffer_length, libspectrum_snap *snap );
static int libspectrum_sna_read_header( const libspectrum_byte *buffer,
					size_t buffer_length,
					libspectrum_snap *snap );
static int libspectrum_sna_read_data( const libspectrum_byte *buffer,
				      size_t buffer_length,
				      libspectrum_snap *snap );
static int libspectrum_sna_read_128_header( const libspectrum_byte *buffer,
					    size_t buffer_length,
					    libspectrum_snap *snap );
static int libspectrum_sna_read_128_data( const libspectrum_byte *buffer,
					  size_t buffer_length,
					  libspectrum_snap *snap );

libspectrum_error
libspectrum_sna_read( libspectrum_snap *snap,
	              const libspectrum_byte *buffer, size_t buffer_length )
{
  int error;

  error = identify_machine( buffer_length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_sna_read_header( buffer, buffer_length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_sna_read_data(
    &buffer[LIBSPECTRUM_SNA_HEADER_LENGTH],
    buffer_length - LIBSPECTRUM_SNA_HEADER_LENGTH, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static int
identify_machine( size_t buffer_length, libspectrum_snap *snap )
{
  switch( buffer_length ) {
  case 49179:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
    break;
  case 131103:
  case 147487:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128 );
    break;
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "libspectrum_sna_identify: unknown length" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_header( const libspectrum_byte *buffer,
			     size_t buffer_length, libspectrum_snap *snap )
{
  int iff;

  if( buffer_length < LIBSPECTRUM_SNA_HEADER_LENGTH ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_sna_read_header: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_a  ( snap, buffer[22] );
  libspectrum_snap_set_f  ( snap, buffer[21] );
  libspectrum_snap_set_bc ( snap, buffer[13] + buffer[14]*0x100 );
  libspectrum_snap_set_de ( snap, buffer[11] + buffer[12]*0x100 );
  libspectrum_snap_set_hl ( snap, buffer[ 9] + buffer[10]*0x100 );
  libspectrum_snap_set_a_ ( snap, buffer[ 8] );
  libspectrum_snap_set_f_ ( snap, buffer[ 7] );
  libspectrum_snap_set_bc_( snap, buffer[ 5] + buffer[ 6]*0x100 );
  libspectrum_snap_set_de_( snap, buffer[ 3] + buffer[ 4]*0x100 );
  libspectrum_snap_set_hl_( snap, buffer[ 1] + buffer[ 2]*0x100 );
  libspectrum_snap_set_ix ( snap, buffer[17] + buffer[18]*0x100 );
  libspectrum_snap_set_iy ( snap, buffer[15] + buffer[16]*0x100 );
  libspectrum_snap_set_i  ( snap, buffer[ 0] );
  libspectrum_snap_set_r  ( snap, buffer[20] );
  libspectrum_snap_set_pc ( snap, buffer[ 6] + buffer[ 7]*0x100 );
  libspectrum_snap_set_sp ( snap, buffer[23] + buffer[24]*0x100 );

  iff = ( buffer[19] & 0x04 ) ? 1 : 0;
  libspectrum_snap_set_iff1( snap, iff );
  libspectrum_snap_set_iff2( snap, iff );
  libspectrum_snap_set_im( snap, buffer[25] & 0x03 );

  libspectrum_snap_set_out_ula( snap, buffer[26] & 0x07 );

  /* A bit before an interrupt. Why this value? Because it's what
     z80's `convert' uses :-) */
  libspectrum_snap_set_tstates( snap, 69664 );

  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_data( const libspectrum_byte *buffer,
			   size_t buffer_length, libspectrum_snap *snap )
{
  int error;
  int offset; int page;
  int i,j;

  if( buffer_length < 0xc000 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_sna_read_data: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  switch( libspectrum_snap_machine( snap ) ) {

  case LIBSPECTRUM_MACHINE_48:

    /* Rescue PC from the stack */
    offset = libspectrum_snap_sp( snap ) - 0x4000;
    libspectrum_snap_set_pc( snap, buffer[offset] + 0x100 * buffer[offset+1] );

    /* Increase SP as PC has been unstacked */
    libspectrum_snap_set_sp( snap, libspectrum_snap_sp( snap ) + 2 );

    /* And split the pages up */
    error = libspectrum_split_to_48k_pages( snap, buffer );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    break;

  case LIBSPECTRUM_MACHINE_128:
    
    for( i=0; i<8; i++ ) {

      libspectrum_byte *ram;

      ram = malloc( 0x4000 * sizeof( libspectrum_byte ) );
      if( ram == NULL ) {
	for( j = 0; j < i; j++ ) {
	  free( libspectrum_snap_pages( snap, i ) );
	  libspectrum_snap_set_pages( snap, i, NULL );
	}
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "libspectrum_sna_read_data: out of memory" );
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      libspectrum_snap_set_pages( snap, i, ram );
    }

    memcpy( libspectrum_snap_pages( snap, 5 ), &buffer[0x0000], 0x4000 );
    memcpy( libspectrum_snap_pages( snap, 2 ), &buffer[0x4000], 0x4000 );

    buffer += 0xc000; buffer_length -= 0xc000;
    error = libspectrum_sna_read_128_header( buffer + 0xc000,
					     buffer_length - 0xc000, snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    page = libspectrum_snap_out_ula( snap ) & 0x07;
    if( page == 5 || page == 2 ) {
      if( memcmp( libspectrum_snap_pages( snap, page ),
		  &buffer[0x8000], 0x4000 ) ) {
	libspectrum_print_error(
          LIBSPECTRUM_ERROR_CORRUPT,
	  "libspectrum_sna_read_data: duplicated page not identical"
	);
	return LIBSPECTRUM_ERROR_CORRUPT;
      }
    } else {
      memcpy( libspectrum_snap_pages( snap, page ), &buffer[0x8000], 0x4000 );
    }

    buffer += 0xc000 + LIBSPECTRUM_SNA_128_HEADER_LENGTH;
    buffer_length -= 0xc000 - LIBSPECTRUM_SNA_128_HEADER_LENGTH;
    error = libspectrum_sna_read_128_data( buffer, buffer_length, snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_sna_read_data: unknown machine" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_128_header( const libspectrum_byte *buffer,
				 size_t buffer_length, libspectrum_snap *snap )
{
  if( buffer_length < 4 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_sna_read_128_header: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_pc( snap, buffer[0] + 0x100 * buffer[1] );
  libspectrum_snap_set_out_ula( snap, buffer[2] );

  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_128_data( const libspectrum_byte *buffer,
			       size_t buffer_length, libspectrum_snap *snap )
{
  int i, page;

  page = libspectrum_snap_out_ula( snap ) & 0x07;

  for( i=0; i<=7; i++ ) {

    if( i==2 || i==5 || i==page ) continue; /* Already got this page */

    /* Check we've still got some data to read */
    if( buffer_length < 0x4000 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_sna_read_128_data: not enough data in buffer"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }
    
    /* Copy the data across */
    memcpy( libspectrum_snap_pages( snap, i ), buffer, 0x4000 );
    
    /* And update what we're looking at here */
    buffer += 0x4000; buffer_length -= 0x4000;

  }

  return LIBSPECTRUM_ERROR_NONE;
}
