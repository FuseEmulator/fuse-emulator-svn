/* z80em.c: Routines for handling Z80Em raw audio files
   Copyright (c) 2002 Darren Salt
   Based on tap.c, copyright (c) 2001 Philip Kendall

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

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>
#include <string.h>

#include "internals.h"
#include "tape_block.h"

libspectrum_error
libspectrum_z80em_read( libspectrum_tape *tape,
			const libspectrum_byte *buffer, size_t length )
{
  libspectrum_tape_block *block;
  libspectrum_tape_rle_pulse_block *z80em_block;
  libspectrum_error error;

  static const char id[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0Raw tape sample";

  if( length < sizeof( id ) ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_z80em_read: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( memcmp( id, buffer, sizeof( id ) ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_SIGNATURE,
			     "libspectrum_z80em_read: wrong signature" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  /* Claim memory for the block */
  block = malloc( sizeof( *block ) );
  if( !block ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_z80em_read: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Set the block type */
  block->type = LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE;
  z80em_block = &block->types.rle_pulse;
  z80em_block->scale = 7; /* 1 time unit == 7 clock ticks */

  buffer += sizeof( id );
  length -= sizeof( id );

  /* Claim memory for the data (it's one big lump) */
  z80em_block->length = length;
  z80em_block->data = malloc( length );
  if( !z80em_block->data ) {
    free( block );
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_z80em_read: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Copy the data across */
  memcpy( z80em_block->data, buffer, length );

  /* Put the block into the block list */
  error = libspectrum_tape_append_block( tape, block );
  if( error ) {
    free( z80em_block->data );
    libspectrum_tape_block_free( block );
    return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_csw_read( libspectrum_tape *tape,
		      const libspectrum_byte *buffer, size_t length )
{
  libspectrum_tape_block *block = NULL;
  libspectrum_tape_rle_pulse_block *csw_block;
  libspectrum_error error;

  int compressed;

  static const char id[23] = "Compressed Square Wave\x1a";

  if( length < sizeof( id ) + 2 ) goto csw_short;

  if( memcmp( id, buffer, sizeof( id ) ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_SIGNATURE,
			     "libspectrum_csw_read: wrong signature" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  /* Claim memory for the block */
  block = malloc( sizeof( *block ) );
  if( !block ) goto csw_nomem;

  /* Set the block type */
  block->type = LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE;
  csw_block = &block->types.rle_pulse;

  buffer += sizeof( id );
  length -= sizeof( id );

  switch( buffer[0] ) {

  case 1:
    if( length < 9 ) goto csw_short;
    csw_block->scale = buffer[2] | buffer[3] << 8;
    if( buffer[4] != 1 ) goto csw_bad_compress;
    compressed = 0;
    buffer += 9;
    length -= 9;
    break;

  case 2:
    if( length < 29 ) goto csw_short;

    csw_block->scale =
      buffer[2]       |
      buffer[3] <<  8 |
      buffer[4] << 16 |
      buffer[5] << 24;
    compressed = buffer[10] - 1;

    if( compressed != 0 && compressed != 1 ) goto csw_bad_compress;

    length -= 29 - buffer[12];
    if( length < 0 ) goto csw_short;
    buffer += 29 + buffer[12];

    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_csw_read: unknown CSW version" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  if (csw_block->scale)
    csw_block->scale = 3500000 / csw_block->scale; /* approximate CPU speed */

  if( csw_block->scale < 0 || csw_block->scale >= 0x80000 ) {
    libspectrum_print_error (LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_csw_read: bad sample rate" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  if( compressed ) {
    /* Compressed data... */
#ifdef HAVE_ZLIB_H
    csw_block->data = NULL;
    csw_block->length = 0;
    error = libspectrum_zlib_inflate( buffer, length, &csw_block->data,
				      &csw_block->length );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
#else
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zlib not available to decompress gzipped file" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
#endif
  }
  else
  {
    /* Claim memory for the data (it's one big lump) */
    csw_block->length = length;
    csw_block->data = malloc( length );
    if( !csw_block->data ) goto csw_nomem;

    /* Copy the data across */
    memcpy( csw_block->data, buffer, length );
  }

  /* Put the block into the block list */
  error = libspectrum_tape_append_block( tape, block );
  if( error ) {
    free (csw_block->data);
    libspectrum_tape_block_free( block );
    return error;
  }

  /* Successful completion */
  return LIBSPECTRUM_ERROR_NONE;

  /* Error returns */

 csw_bad_compress:
  free( block );
  libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			   "libspectrum_csw_read: unknown compression type" );
  return LIBSPECTRUM_ERROR_CORRUPT;

 csw_nomem:
  free( block );
  libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			   "libspectrum_csw_read: out of memory" );
  return LIBSPECTRUM_ERROR_MEMORY;

 csw_short:
  free( block );
  libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			   "libspectrum_csw_read: not enough data in buffer" );
  return LIBSPECTRUM_ERROR_CORRUPT;
}
