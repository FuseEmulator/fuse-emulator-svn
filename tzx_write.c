/* tzx_write.c: Routines for writing .tzx files
   Copyright (c) 2001-2005 Philip Kendall

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

#include <stdio.h>
#include <string.h>

#include "internals.h"

/*** Local function prototypes ***/

static libspectrum_error
tzx_write_rom( libspectrum_tape_block *block, libspectrum_byte **buffer,
	       libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_turbo( libspectrum_tape_block *block, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_pure_tone( libspectrum_tape_block *block, libspectrum_byte **buffer,
		     libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_data( libspectrum_tape_block *block, libspectrum_byte **buffer,
		libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_generalised_data( libspectrum_tape_block *block,
			    libspectrum_byte **buffer,
			    libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_pulses( libspectrum_tape_block *block, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_pause( libspectrum_tape_block *block, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_group_start( libspectrum_tape_block *block,
		       libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length );
static libspectrum_error
tzx_write_jump( libspectrum_tape_block *block, libspectrum_byte **buffer,
		libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_loop_start( libspectrum_tape_block *block,
		      libspectrum_byte **buffer, libspectrum_byte **ptr,
		      size_t *length );
static libspectrum_error
tzx_write_select( libspectrum_tape_block *block, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_stop( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length );
static libspectrum_error
tzx_write_comment( libspectrum_tape_block *block, libspectrum_byte **buffer,
		   libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_message( libspectrum_tape_block *block, libspectrum_byte **buffer,
		   libspectrum_byte **ptr, size_t *length );
static libspectrum_error
tzx_write_archive_info( libspectrum_tape_block *block,
			libspectrum_byte **buffer, libspectrum_byte **ptr,
			size_t *length );
static libspectrum_error
tzx_write_hardware( libspectrum_tape_block *block, libspectrum_byte **buffer,
		    libspectrum_byte **ptr, size_t *length);
static libspectrum_error
tzx_write_custom( libspectrum_tape_block *block, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length );

static libspectrum_error
tzx_write_empty_block( libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length, libspectrum_tape_type id );

static libspectrum_error
tzx_write_bytes( libspectrum_byte **ptr, size_t length,
		 size_t length_bytes, libspectrum_byte *data );
static libspectrum_error
tzx_write_string( libspectrum_byte **ptr, char *string );

/*** Function definitions ***/

/* The main write function */

libspectrum_error
libspectrum_tzx_write( libspectrum_byte **buffer, size_t *length,
		       libspectrum_tape *tape )
{
  libspectrum_error error;
  libspectrum_tape_iterator iterator;
  libspectrum_tape_block *block;
  libspectrum_byte *ptr = *buffer;

  size_t signature_length = strlen( libspectrum_tzx_signature );

  /* First, write the .tzx signature and the version numbers */
  error = libspectrum_make_room( buffer, signature_length + 2, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  memcpy( ptr, libspectrum_tzx_signature, signature_length );
  ptr += signature_length;

  *ptr++ = 1;		/* Major version number */
  *ptr++ = 20;		/* Minor version number */

  for( block = libspectrum_tape_iterator_init( &iterator, tape );
       block;
       block = libspectrum_tape_iterator_next( &iterator )       )
  {
    switch( libspectrum_tape_block_type( block ) ) {

    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      error = tzx_write_rom( block, buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      error = tzx_write_turbo( block, buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      error = tzx_write_pure_tone( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      error = tzx_write_pulses( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      error = tzx_write_data( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
      error = tzx_write_generalised_data( block, buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      error = tzx_write_pause( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
      error = tzx_write_group_start( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
      error = tzx_write_empty_block( buffer, &ptr, length,
				     libspectrum_tape_block_type( block ) );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      error = tzx_write_jump( block, buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      error = tzx_write_loop_start( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
      error = tzx_write_empty_block( buffer, &ptr, length,
				     libspectrum_tape_block_type( block ) );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
      error = tzx_write_select( block, buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
      error = tzx_write_stop( buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      error = tzx_write_comment( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
      error = tzx_write_message( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
      error = tzx_write_archive_info( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
      error = tzx_write_hardware( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      error = tzx_write_custom( block, buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    default:
      free( *buffer );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
	"libspectrum_tzx_write: unknown block type 0x%02x",
	libspectrum_tape_block_type( block )
      );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
  }

  (*length) = ptr - *buffer;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_rom( libspectrum_tape_block *block, libspectrum_byte **buffer,
	       libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte, the pause, the length and the actual data */
  error = libspectrum_make_room(
    buffer, 5 + libspectrum_tape_block_data_length( block ), ptr, length
  );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the pause */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_ROM;
  libspectrum_write_word( ptr, libspectrum_tape_block_pause( block ) );

  /* Copy the data across */
  error = tzx_write_bytes( ptr, libspectrum_tape_block_data_length( block ),
			   2, libspectrum_tape_block_data( block ) );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_turbo( libspectrum_tape_block *block, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte, the metadata and the actual data */
  error = libspectrum_make_room(
    buffer, 19 + libspectrum_tape_block_data_length( block ), ptr, length
  );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the metadata */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_TURBO;
  libspectrum_write_word( ptr, libspectrum_tape_block_pilot_length( block ) );
  libspectrum_write_word( ptr, libspectrum_tape_block_sync1_length( block ) );
  libspectrum_write_word( ptr, libspectrum_tape_block_sync2_length( block ) );
  libspectrum_write_word( ptr, libspectrum_tape_block_bit0_length ( block ) );
  libspectrum_write_word( ptr, libspectrum_tape_block_bit1_length ( block ) );
  libspectrum_write_word( ptr, libspectrum_tape_block_pilot_pulses( block ) );
  *(*ptr)++ = libspectrum_tape_block_bits_in_last_byte( block );
  libspectrum_write_word( ptr, libspectrum_tape_block_pause       ( block ) );

  /* Copy the data across */
  error = tzx_write_bytes( ptr, libspectrum_tape_block_data_length( block ),
			   3, libspectrum_tape_block_data( block ) );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pure_tone( libspectrum_tape_block *block, libspectrum_byte **buffer,
		     libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte and the data */
  error = libspectrum_make_room( buffer, 5, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_PURE_TONE;
  libspectrum_write_word( ptr, libspectrum_tape_block_pulse_length( block ) );
  libspectrum_write_word( ptr, libspectrum_tape_block_count( block ) );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pulses( libspectrum_tape_block *block, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  size_t i, count, block_length;

  /* ID byte, count and 2 bytes for length of each pulse */
  count = libspectrum_tape_block_count( block );
  block_length = 2 + 2 * count;

  /* Make room for the ID byte, the count and the data */
  error = libspectrum_make_room( buffer, block_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_PULSES;
  *(*ptr)++ = count;
  for( i = 0; i < count; i++ )
    libspectrum_write_word( ptr,
			    libspectrum_tape_block_pulse_lengths( block, i ) );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_data( libspectrum_tape_block *block, libspectrum_byte **buffer,
		libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;
  size_t data_length;

  /* Make room for the ID byte, the metadata and the actual data */
  data_length = libspectrum_tape_block_data_length( block );
  error = libspectrum_make_room( buffer, 11 + data_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the metadata */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_PURE_DATA;
  libspectrum_write_word( ptr, libspectrum_tape_block_bit0_length( block ) );
  libspectrum_write_word( ptr, libspectrum_tape_block_bit1_length( block ) );
  *(*ptr)++ = libspectrum_tape_block_bits_in_last_byte( block );
  libspectrum_write_word( ptr, libspectrum_tape_block_pause( block ) );

  /* Copy the data across */
  error = tzx_write_bytes( ptr, data_length, 3,
			   libspectrum_tape_block_data( block ) );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static size_t
generalised_data_length( libspectrum_tape_block *block )
{
  libspectrum_tape_generalised_data_symbol_table *table;
  size_t symbol_count;

  size_t data_length = 14;	/* Minimum if no tables or symbols present */

  table = libspectrum_tape_block_pilot_table( block );
  
  symbol_count = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( table );
  if( symbol_count ) {

    size_t max_pulses = libspectrum_tape_generalised_data_symbol_table_max_pulses( table );
    size_t symbols_in_table = libspectrum_tape_generalised_data_symbol_table_symbols_in_table( table );

    data_length += ( 2 * max_pulses + 1 ) * symbols_in_table;
    data_length += 3 * symbol_count;

  }

  table = libspectrum_tape_block_data_table( block );

  symbol_count = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( table );
  if( symbol_count ) {

    size_t max_pulses = libspectrum_tape_generalised_data_symbol_table_max_pulses( table );
    size_t symbols_in_table = libspectrum_tape_generalised_data_symbol_table_symbols_in_table( table );

    size_t bits_per_symbol;

    data_length += ( 2 * max_pulses + 1 ) * symbols_in_table;

    bits_per_symbol = libspectrum_tape_block_bits_per_data_symbol( block );
    
    data_length += ( ( bits_per_symbol * symbol_count ) + 7 ) / 8;

  }

  return data_length;
}

static libspectrum_error
serialise_generalised_data_table( libspectrum_byte **ptr, libspectrum_tape_generalised_data_symbol_table *table )
{
  libspectrum_dword symbols_in_block;
  libspectrum_word symbols_in_table;

  symbols_in_block = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( table );

  if( !symbols_in_block ) return LIBSPECTRUM_ERROR_NONE;

  libspectrum_write_dword( ptr, symbols_in_block );
  *(*ptr)++ = libspectrum_tape_generalised_data_symbol_table_max_pulses( table );

  symbols_in_table = libspectrum_tape_generalised_data_symbol_table_symbols_in_table( table );

  if( symbols_in_table == 0 || symbols_in_table > 256 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID, "%s: invalid number of symbols in table: %d", __func__, symbols_in_table );
    return LIBSPECTRUM_ERROR_INVALID;
  } else if( symbols_in_table == 256 ) {
    symbols_in_table = 0;
  }

  *(*ptr)++ = symbols_in_table;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
serialise_generalised_data_symbols( libspectrum_byte **ptr, libspectrum_tape_generalised_data_symbol_table *table )
{
  libspectrum_word symbols_in_table = libspectrum_tape_generalised_data_symbol_table_symbols_in_table( table );
  libspectrum_byte max_pulses = libspectrum_tape_generalised_data_symbol_table_max_pulses( table );

  libspectrum_word i;
  libspectrum_byte j;

  if( !libspectrum_tape_generalised_data_symbol_table_symbols_in_block( table ) ) return LIBSPECTRUM_ERROR_NONE;

  for( i = 0; i < symbols_in_table; i++ ) {

    libspectrum_tape_generalised_data_symbol *symbol = libspectrum_tape_generalised_data_symbol_table_symbol( table, i );

    *(*ptr)++ = libspectrum_tape_generalised_data_symbol_type( symbol );

    for( j = 0; j < max_pulses; j++ )
      libspectrum_write_word( ptr, libspectrum_tape_generalised_data_symbol_pulse( symbol, j ) );

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_generalised_data( libspectrum_tape_block *block,
			    libspectrum_byte **buffer, libspectrum_byte **ptr,
			    size_t *length )
{
  size_t data_length, bits_per_symbol;
  libspectrum_error error;
  libspectrum_tape_generalised_data_symbol_table *pilot_table, *data_table;
  libspectrum_dword pilot_symbol_count, data_symbol_count, i;

  data_length = generalised_data_length( block );

  error = libspectrum_make_room( buffer, 5 + data_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA;
  libspectrum_write_dword( ptr, data_length );

  libspectrum_write_word( ptr, libspectrum_tape_block_pause( block ) );

  pilot_table = libspectrum_tape_block_pilot_table( block );
  data_table = libspectrum_tape_block_data_table( block );

  error = serialise_generalised_data_table( ptr, pilot_table );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = serialise_generalised_data_table( ptr, data_table );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = serialise_generalised_data_symbols( ptr, pilot_table );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  pilot_symbol_count = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( pilot_table );

  for( i = 0; i < pilot_symbol_count; i++ ) {
    *(*ptr)++ = libspectrum_tape_block_pilot_symbols( block, i );
    libspectrum_write_word( ptr, libspectrum_tape_block_pilot_repeats( block, i ) );
  }

  error = serialise_generalised_data_symbols( ptr, data_table );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  bits_per_symbol = libspectrum_tape_block_bits_per_data_symbol( block );
  data_symbol_count = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( data_table );

  data_length = ( ( bits_per_symbol * data_symbol_count ) + 7 ) / 8;

  memcpy( *ptr, libspectrum_tape_block_data( block ), data_length );
  (*ptr) += data_length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pause( libspectrum_tape_block *block, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte and the data */
  error = libspectrum_make_room( buffer, 3, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_PAUSE;
  libspectrum_write_word( ptr, libspectrum_tape_block_pause( block ) );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_group_start( libspectrum_tape_block *block,
		       libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length )
{
  libspectrum_error error;
  char *name; size_t name_length;

  name = libspectrum_tape_block_text( block );
  name_length = strlen( (char*)name );

  /* Make room for the ID byte, the length byte and the name */
  error = libspectrum_make_room( buffer, 2 + name_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_GROUP_START;
  
  error = tzx_write_string( ptr, name ); if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_jump( libspectrum_tape_block *block, libspectrum_byte **buffer,
		libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  int u_offset;

  /* Make room for the ID byte and the offset */
  error = libspectrum_make_room( buffer, 3, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_JUMP;

  u_offset = libspectrum_tape_block_offset( block );
  if( u_offset < 0 ) u_offset += 65536;
  libspectrum_write_word( ptr, u_offset );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_loop_start( libspectrum_tape_block *block, libspectrum_byte **buffer,
		      libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte and the count */
  error = libspectrum_make_room( buffer, 3, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_LOOP_START;

  libspectrum_write_word( ptr, libspectrum_tape_block_count( block ) );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_select( libspectrum_tape_block *block, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;
  size_t count, total_length, i;

  /* The id byte, the total length (2 bytes), the count byte,
     and ( 2 offset bytes and 1 length byte ) per selection */
  count = libspectrum_tape_block_count( block );
  total_length = 4 + 3 * count;

  for( i = 0; i < count; i++ )
    total_length += strlen( (char*)libspectrum_tape_block_texts( block, i ) );

  error = libspectrum_make_room( buffer, total_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_SELECT;
  libspectrum_write_word( ptr, total_length );
  *(*ptr)++ = count;

  for( i = 0; i < count; i++ ) {
    libspectrum_write_word( ptr, libspectrum_tape_block_offsets( block, i ) );
    error = tzx_write_string( ptr, libspectrum_tape_block_texts( block, i ) );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_stop( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte and four length bytes */
  error = libspectrum_make_room( buffer, 5, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_STOP48;
  *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0';

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_comment( libspectrum_tape_block *block, libspectrum_byte **buffer,
		   libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;
  char *comment; size_t comment_length;

  comment = libspectrum_tape_block_text( block );
  comment_length = strlen( (char*)comment );

  /* Make room for the ID byte, the length byte and the text */
  error = libspectrum_make_room( buffer, 2 + comment_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_COMMENT;

  error = tzx_write_string( ptr, comment ); if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_message( libspectrum_tape_block *block, libspectrum_byte **buffer,
		   libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;
  char *message; size_t text_length;

  message = libspectrum_tape_block_text( block );
  text_length = strlen( (char*)message );

  /* Make room for the ID byte, the time byte, length byte and the text */
  error = libspectrum_make_room( buffer, 3 + text_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_MESSAGE;
  *(*ptr)++ = libspectrum_tape_block_pause( block );

  error = tzx_write_string( ptr, message ); if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_archive_info( libspectrum_tape_block *block,
			libspectrum_byte **buffer, libspectrum_byte **ptr,
			size_t *length )
{
  libspectrum_error error;
  size_t i, count, total_length;

  count = libspectrum_tape_block_count( block );

  /* ID byte, 1 count byte, 2 bytes (ID and length) for
     every string */
  total_length = 2 + 2 * count;
  /* And then the length of all the strings */
  for( i = 0; i < count; i++ )
    total_length += strlen( (char*)libspectrum_tape_block_texts( block, i ) );

  /* Make room for all that, and two bytes storing the length */
  error = libspectrum_make_room( buffer, total_length + 2, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write out the metadata */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO;
  libspectrum_write_word( ptr, total_length );
  *(*ptr)++ = count;

  /* And the strings */
  for( i = 0; i < count; i++ ) {
    *(*ptr)++ = libspectrum_tape_block_ids( block, i );
    error = tzx_write_string( ptr, libspectrum_tape_block_texts( block, i ) );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_hardware( libspectrum_tape_block *block, libspectrum_byte **buffer,
		    libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;
  size_t i, count;

  /* We need one ID byte, one count byte and then three bytes for every
     entry */
  count = libspectrum_tape_block_count( block );
  error = libspectrum_make_room( buffer, 2 + 3 * count, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write out the metadata */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_HARDWARE;
  *(*ptr)++ = count;

  /* And the info */
  for( i = 0; i < count; i++ ) {
    *(*ptr)++ = libspectrum_tape_block_types( block, i );
    *(*ptr)++ = libspectrum_tape_block_ids  ( block, i );
    *(*ptr)++ = libspectrum_tape_block_values( block, i );
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_custom( libspectrum_tape_block *block, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;
  size_t data_length;

  data_length = libspectrum_tape_block_data_length( block );

  /* An ID byte, 16 description bytes, 4 length bytes and the data
     itself */
  error = libspectrum_make_room( buffer, 1 + 16 + 4 + data_length, ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_CUSTOM;
  memcpy( *ptr, libspectrum_tape_block_text( block ), 16 ); *ptr += 16;

  error = tzx_write_bytes( ptr, data_length, 4,
			   libspectrum_tape_block_data( block ) );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_empty_block( libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length, libspectrum_tape_type id )
{
  libspectrum_error error;

  /* Make room for the ID byte */
  error = libspectrum_make_room( buffer, 1, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = id;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_bytes( libspectrum_byte **ptr, size_t length,
		 size_t length_bytes, libspectrum_byte *data )
{
  size_t i, shifter;

  /* Write out the appropriate number of length bytes */
  for( i=0, shifter = length; i<length_bytes; i++, shifter >>= 8 )
    *(*ptr)++ = shifter & 0xff;

  /* And then the actual data */
  memcpy( *ptr, data, length ); (*ptr) += length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_string( libspectrum_byte **ptr, char *string )
{
  libspectrum_error error;

  size_t i, length = strlen( (char*)string );

  error = tzx_write_bytes( ptr, length, 1, (libspectrum_byte*)string );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Fix up line endings */
  (*ptr) -= length;
  for( i=0; i<length; i++, (*ptr)++ ) if( **ptr == '\x0a' ) **ptr = '\x0d';

  return LIBSPECTRUM_ERROR_NONE;
}
