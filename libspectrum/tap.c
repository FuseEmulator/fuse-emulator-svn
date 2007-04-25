/* tape.c: Routines for handling .tap files
   Copyright (c) 2001-2003 Philip Kendall

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

#include <stddef.h>
#include <string.h>

#include "internals.h"

#define DESCRIPTION_LENGTH 256

static libspectrum_error
write_rom( libspectrum_tape_block *block, libspectrum_byte **buffer,
	   libspectrum_byte **ptr, size_t *length );
static libspectrum_error
write_turbo( libspectrum_tape_block *block, libspectrum_byte **buffer,
	     libspectrum_byte **ptr, size_t *length );
static libspectrum_error
write_pure_data( libspectrum_tape_block *block, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length );

static libspectrum_error
write_tap_block( libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length, libspectrum_byte *data, size_t data_length );
static libspectrum_error
skip_block( libspectrum_tape_block *block, const char *message );

libspectrum_error
libspectrum_tap_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
		      const size_t length )
{
  libspectrum_tape_block *block;
  libspectrum_error error;
  size_t data_length; libspectrum_byte *data;

  const libspectrum_byte *ptr, *end;

  ptr = buffer; end = buffer + length;

  while( ptr < end ) {
    
    /* If we've got less than two bytes for the length, something's
       gone wrong, so gone home */
    if( ( end - ptr ) < 2 ) {
      libspectrum_tape_clear( tape );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_tap_read: not enough data in buffer"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Get memory for a new block */
    error = libspectrum_tape_block_alloc( &block, LIBSPECTRUM_TAPE_BLOCK_ROM );
    if( error ) return error;

    /* Get the length, and move along the buffer */
    data_length = ptr[0] + ptr[1] * 0x100;
    libspectrum_tape_block_set_data_length( block, data_length );
    ptr += 2;

    /* Have we got enough bytes left in buffer? */
    if( end - ptr < (ptrdiff_t)data_length ) {
      libspectrum_tape_clear( tape );
      free( block );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_tap_create: not enough data in buffer"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Allocate memory for the data */
    data = malloc( data_length * sizeof( libspectrum_byte ) );
    if( !data ) {
      libspectrum_tape_clear( tape );
      free( block );
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "libspectrum_tap_create: out of memory" );
      return LIBSPECTRUM_ERROR_MEMORY;
    }
    libspectrum_tape_block_set_data( block, data );

    /* Copy the block data across, and move along */
    memcpy( data, ptr, data_length ); ptr += data_length;

    /* Give a 1s pause after each block */
    libspectrum_tape_block_set_pause( block, 1000 );

    /* Finally, put the block into the block list */
    error = libspectrum_tape_append_block( tape, block );
    if( error ) { libspectrum_tape_block_free( block ); return error; }

  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tap_write( libspectrum_byte **buffer, size_t *length,
		       libspectrum_tape *tape )
{
  libspectrum_tape_iterator iterator;
  libspectrum_tape_block *block;
  libspectrum_error error;

  libspectrum_byte *ptr = *buffer;

  for( block = libspectrum_tape_iterator_init( &iterator, tape );
       block;
       block = libspectrum_tape_iterator_next( &iterator ) )
  {
    switch( libspectrum_tape_block_type( block ) ) {

    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      error = write_rom( block, buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      error = write_turbo( block, buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      error = write_pure_data( block, buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      error = skip_block( block, "conversion almost certainly won't work" );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return 1; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      error = skip_block( block, "conversion may not work" );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return 1; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      error = skip_block( block, NULL );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return 1; }
      break;

    default:
      if( *length ) free( *buffer );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
        "libspectrum_tap_write: unknown block type 0x%02x",
	libspectrum_tape_block_type( block )
      );
      return LIBSPECTRUM_ERROR_LOGIC;

    }
  }

  (*length) = ptr - *buffer;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_rom( libspectrum_tape_block *block, libspectrum_byte **buffer,
	   libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  error = write_tap_block( buffer, ptr, length,
			   libspectrum_tape_block_data( block ),
			   libspectrum_tape_block_data_length( block ) );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_turbo( libspectrum_tape_block *block, libspectrum_byte **buffer,
	     libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  /* Print out a warning about converting a turbo block */
  libspectrum_print_error(
    LIBSPECTRUM_ERROR_WARNING,
    "write_turbo: converting Turbo Speed Data Block (ID 0x11); conversion may well not work"
  );

  error = write_tap_block( buffer, ptr, length,
			   libspectrum_tape_block_data( block ),
			   libspectrum_tape_block_data_length( block ) );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_pure_data( libspectrum_tape_block *block, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  /* Print out a warning about converting a pure data block */
  libspectrum_print_error(
    LIBSPECTRUM_ERROR_WARNING,
    "write_pure_data: converting Pure Data Block (ID 0x14); conversion almost certainly won't work"
  );

  error = write_tap_block( buffer, ptr, length,
			   libspectrum_tape_block_data( block ),
			   libspectrum_tape_block_data_length( block ) );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_tap_block( libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length, libspectrum_byte *data, size_t data_length )
{
  libspectrum_error error;

  error = libspectrum_make_room( buffer, 2 + data_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write out the length and the data */
  *(*ptr)++ =   data_length & 0x00ff;
  *(*ptr)++ = ( data_length & 0xff00 ) >> 8;
  memcpy( *ptr, data, data_length ); (*ptr) += data_length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_block( libspectrum_tape_block *block, const char *message )
{
  char description[ DESCRIPTION_LENGTH ];
  libspectrum_error error;

  error = libspectrum_tape_block_description( description, DESCRIPTION_LENGTH,
					      block );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  if( message ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_WARNING,
			     "skip_block: skipping %s (ID 0x%02x); %s",
			     description, libspectrum_tape_block_type( block ),
			     message );
  } else {
    libspectrum_print_error( LIBSPECTRUM_ERROR_WARNING,
			     "skip_block: skipping %s (ID 0x%02x)",
			     description,
			     libspectrum_tape_block_type( block ) );
  }

  return LIBSPECTRUM_ERROR_NONE;
}
