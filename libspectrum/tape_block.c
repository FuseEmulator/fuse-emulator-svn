/* tape_block.c: one tape block
   Copyright (c) 2003-2007 Philip Kendall

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

#include "tape_block.h"

/* The number of pilot pulses for the standard ROM loader NB: These
   disagree with the .tzx specification (they're 5 and 1 less
   respectively), but are correct. Entering the loop at #04D8 in the
   48K ROM with HL == #0001 will produce the first sync pulse, not a
   pilot pulse, which gives a difference of one in both cases. The
   further difference of 4 in the header count is just a screw-up in
   the .tzx specification AFAICT */
static const size_t LIBSPECTRUM_TAPE_PILOTS_HEADER = 0x1f7f;
static const size_t LIBSPECTRUM_TAPE_PILOTS_DATA   = 0x0c97;

/* Functions to initialise block types */

static libspectrum_error
rom_init( libspectrum_tape_rom_block *block );
static libspectrum_error
turbo_init( libspectrum_tape_turbo_block *block );
static libspectrum_error
pure_data_init( libspectrum_tape_pure_data_block *block );
static libspectrum_error
raw_data_init( libspectrum_tape_raw_data_block *block );

libspectrum_error
libspectrum_tape_block_alloc( libspectrum_tape_block **block,
			      libspectrum_tape_type type )
{
  (*block) = malloc( sizeof( libspectrum_tape_block ) );
  if( !(*block) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_tape_block_alloc: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  libspectrum_tape_block_set_type( *block, type );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free the memory used by one block */
libspectrum_error
libspectrum_tape_block_free( libspectrum_tape_block *block )
{
  size_t i;

  switch( block->type ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    free( block->types.rom.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    free( block->types.turbo.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    free( block->types.pulses.lengths );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    free( block->types.pure_data.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    free( block->types.raw_data.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    free( block->types.group_start.name );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    for( i=0; i<block->types.select.count; i++ ) {
      free( block->types.select.descriptions[i] );
    }
    free( block->types.select.descriptions );
    free( block->types.select.offsets );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    free( block->types.comment.text );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    free( block->types.message.text );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    for( i=0; i<block->types.archive_info.count; i++ ) {
      free( block->types.archive_info.strings[i] );
    }
    free( block->types.archive_info.ids );
    free( block->types.archive_info.strings );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    free( block->types.hardware.types  );
    free( block->types.hardware.ids    );
    free( block->types.hardware.values );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    free( block->types.custom.description );
    free( block->types.custom.data );
    break;

    /* Block types not present in .tzx follow here */

  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    free( block->types.rle_pulse.data );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CONCAT: /* This should never occur */
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "%s: unknown block type %d", __func__,
			     block->type );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  free( block );

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_tape_type
libspectrum_tape_block_type( libspectrum_tape_block *block )
{
  return block->type;
}

libspectrum_error
libspectrum_tape_block_set_type( libspectrum_tape_block *block,
				 libspectrum_tape_type type )
{
  block->type = type;
  return LIBSPECTRUM_ERROR_NONE;
}

/* Called when a new block is started to initialise its internal state */
libspectrum_error
libspectrum_tape_block_init( libspectrum_tape_block *block )
{
  switch( libspectrum_tape_block_type( block ) ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    return rom_init( &(block->types.rom) );
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    return turbo_init( &(block->types.turbo) );
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    block->types.pure_tone.edge_count = block->types.pure_tone.pulses;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    block->types.pulses.edge_count = 0;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    return pure_data_init( &(block->types.pure_data) );
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    return raw_data_init( &(block->types.raw_data) );
  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    block->types.rle_pulse.index = 0;
    return LIBSPECTRUM_ERROR_NONE;

  /* These blocks need no initialisation */
  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    return LIBSPECTRUM_ERROR_NONE;

  default:
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_tape_init_block: unknown block type 0x%02x",
      block->type
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rom_init( libspectrum_tape_rom_block *block )
{
  /* Initialise the number of pilot pulses */
  block->edge_count = block->length && block->data[0] & 0x80 ?
                      LIBSPECTRUM_TAPE_PILOTS_DATA           :
                      LIBSPECTRUM_TAPE_PILOTS_HEADER;

  /* And that we're just before the start of the data */
  block->bytes_through_block = -1; block->bits_through_byte = 7;
  block->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_init( libspectrum_tape_turbo_block *block )
{
  /* Initialise the number of pilot pulses */
  block->edge_count = block->pilot_pulses;

  /* And that we're just before the start of the data */
  block->bytes_through_block = -1; block->bits_through_byte = 7;
  block->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pure_data_init( libspectrum_tape_pure_data_block *block )
{
  libspectrum_error error;

  /* We're just before the start of the data */
  block->bytes_through_block = -1; block->bits_through_byte = 7;
  /* Set up the next bit */
  error = libspectrum_tape_pure_data_next_bit( block );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
raw_data_init( libspectrum_tape_raw_data_block *block )
{
  libspectrum_error error;

  /* We're just before the start of the data */
  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;
  block->bytes_through_block = -1; block->bits_through_byte = 7;
  block->last_bit = 0x80 & block->data[0];
  /* Set up the next bit */
  error = libspectrum_tape_raw_data_next_bit( block );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Does this block consist solely of metadata? */
int
libspectrum_tape_block_metadata( libspectrum_tape_block *block )
{
  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    return 0;

  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
  case LIBSPECTRUM_TAPE_BLOCK_CONCAT:
    return 1;
  }

  /* Should never happen */
  return -1;
}

libspectrum_error
libspectrum_tape_block_read_symbol_table_parameters(
  libspectrum_tape_block *block, int pilot, const libspectrum_byte **ptr )
{
  libspectrum_tape_generalised_data_block *generalised =
    &block->types.generalised_data;

  libspectrum_tape_generalised_data_symbol_table *table =
    pilot ? &generalised->pilot_table : &generalised->data_table;

  table->symbols_in_block = libspectrum_read_dword( ptr );
  table->max_pulses = (*ptr)[0];
  table->symbols_in_table = (*ptr)[1];
  (*ptr) += 2;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_block_read_symbol_table(
  libspectrum_tape_generalised_data_symbol_table *table,
  const libspectrum_byte **ptr, size_t length )
{
  if( table->symbols_in_block ) {
    
    libspectrum_tape_generalised_data_symbol *symbol;
    size_t i, j;

    /* Sanity check */
    if( length < ( 2 * table->max_pulses + 1 ) * table->symbols_in_table ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "%s: not enough data in buffer", __func__ );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }


    table->symbols = malloc( table->symbols_in_table *
			     sizeof( *table->symbols ) );
    if( !table->symbols ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY, "%s: out of memory",
			       __func__ );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    for( i = 0, symbol = table->symbols;
	 i < table->symbols_in_table;
	 i++, symbol++ ) {
      symbol->edge_type = **ptr; (*ptr)++;
      symbol->lengths = malloc( table->max_pulses * sizeof( *symbol->lengths ) );
      if( !symbol->lengths ) {
	/* Aargh. Unwind everything */
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      for( j = 0; j < table->max_pulses; j++ ) {
	symbol->lengths[ j ] = (*ptr)[0] + (*ptr)[1] * 0x100;
	(*ptr) += 2;
      }
    }

  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

