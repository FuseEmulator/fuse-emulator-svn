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

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

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
rom_init( libspectrum_tape_rom_block *block,
          libspectrum_tape_rom_block_state *state );
static libspectrum_error
turbo_init( libspectrum_tape_turbo_block *block,
            libspectrum_tape_turbo_block_state *state );
static libspectrum_error
pure_data_init( libspectrum_tape_pure_data_block *block,
                libspectrum_tape_pure_data_block_state *state );
static libspectrum_error
raw_data_init( libspectrum_tape_raw_data_block *block,
               libspectrum_tape_raw_data_block_state *state );
static libspectrum_error
generalised_data_init( libspectrum_tape_generalised_data_block *block,
                       libspectrum_tape_generalised_data_block_state *state );

libspectrum_tape_block*
libspectrum_tape_block_alloc( libspectrum_tape_type type )
{
  libspectrum_tape_block *block = libspectrum_malloc( sizeof( *block ) );
  libspectrum_tape_block_set_type( block, type );
  return block;
}

static void
free_symbol_table( libspectrum_tape_generalised_data_symbol_table *table )
{
  size_t i;

  if( table->symbols ) {
    for( i = 0; i < table->symbols_in_table; i++ )
      libspectrum_free( table->symbols[ i ].lengths );

    libspectrum_free( table->symbols );
  }
}

/* Free the memory used by one block */
libspectrum_error
libspectrum_tape_block_free( libspectrum_tape_block *block )
{
  size_t i;

  switch( block->type ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    libspectrum_free( block->types.rom.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    libspectrum_free( block->types.turbo.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    libspectrum_free( block->types.pulses.lengths );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    libspectrum_free( block->types.pure_data.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    libspectrum_free( block->types.raw_data.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    free_symbol_table( &block->types.generalised_data.pilot_table );
    free_symbol_table( &block->types.generalised_data.data_table );
    libspectrum_free( block->types.generalised_data.pilot_symbols );
    libspectrum_free( block->types.generalised_data.pilot_repeats );
    libspectrum_free( block->types.generalised_data.data );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    libspectrum_free( block->types.group_start.name );
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
      libspectrum_free( block->types.select.descriptions[i] );
    }
    libspectrum_free( block->types.select.descriptions );
    libspectrum_free( block->types.select.offsets );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    libspectrum_free( block->types.comment.text );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    libspectrum_free( block->types.message.text );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    for( i=0; i<block->types.archive_info.count; i++ ) {
      libspectrum_free( block->types.archive_info.strings[i] );
    }
    libspectrum_free( block->types.archive_info.ids );
    libspectrum_free( block->types.archive_info.strings );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    libspectrum_free( block->types.hardware.types  );
    libspectrum_free( block->types.hardware.ids    );
    libspectrum_free( block->types.hardware.values );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    libspectrum_free( block->types.custom.description );
    libspectrum_free( block->types.custom.data );
    break;

    /* Block types not present in .tzx follow here */

  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    libspectrum_free( block->types.rle_pulse.data );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CONCAT: /* This should never occur */
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "%s: unknown block type %d", __func__,
			     block->type );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  libspectrum_free( block );

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
libspectrum_tape_block_init( libspectrum_tape_block *block,
                             libspectrum_tape_block_state *state )
{
  if( !block ) return LIBSPECTRUM_ERROR_NONE;

  switch( libspectrum_tape_block_type( block ) ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    return rom_init( &(block->types.rom), &(state->block_state.rom) );
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    return turbo_init( &(block->types.turbo), &(state->block_state.turbo) );
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    state->block_state.pure_tone.edge_count = block->types.pure_tone.pulses;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    state->block_state.pulses.edge_count = 0;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    return pure_data_init( &(block->types.pure_data),
                           &(state->block_state.pure_data) );
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    return raw_data_init( &(block->types.raw_data),
                          &(state->block_state.raw_data) );
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    return generalised_data_init( &(block->types.generalised_data),
                                  &(state->block_state.generalised_data) );
  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    state->block_state.rle_pulse.index = 0;
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
rom_init( libspectrum_tape_rom_block *block,
          libspectrum_tape_rom_block_state *state )
{
  /* Initialise the number of pilot pulses */
  state->edge_count = block->length && block->data[0] & 0x80 ?
                      LIBSPECTRUM_TAPE_PILOTS_DATA           :
                      LIBSPECTRUM_TAPE_PILOTS_HEADER;

  /* And that we're just before the start of the data */
  state->bytes_through_block = -1; state->bits_through_byte = 7;
  state->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_init( libspectrum_tape_turbo_block *block,
            libspectrum_tape_turbo_block_state *state )
{
  /* Initialise the number of pilot pulses */
  state->edge_count = block->pilot_pulses;

  /* And that we're just before the start of the data */
  state->bytes_through_block = -1; state->bits_through_byte = 7;
  state->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pure_data_init( libspectrum_tape_pure_data_block *block,
                libspectrum_tape_pure_data_block_state *state )
{
  libspectrum_error error;

  /* We're just before the start of the data */
  state->bytes_through_block = -1; state->bits_through_byte = 7;
  /* Set up the next bit */
  error = libspectrum_tape_pure_data_next_bit( block, state );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
raw_data_init( libspectrum_tape_raw_data_block *block,
               libspectrum_tape_raw_data_block_state *state )
{
  libspectrum_error error;

  if( block->data ) {

    /* We're just before the start of the data */
    state->state = LIBSPECTRUM_TAPE_STATE_DATA1;
    state->bytes_through_block = -1; state->bits_through_byte = 7;
    state->last_bit = 0x80 & block->data[0];
    /* Set up the next bit */
    error = libspectrum_tape_raw_data_next_bit( block, state );
    if( error ) return error;

  } else {

    state->state = LIBSPECTRUM_TAPE_STATE_PAUSE;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
generalised_data_init( libspectrum_tape_generalised_data_block *block,
                       libspectrum_tape_generalised_data_block_state *state )
{
  state->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  state->run = 0;
  state->symbols_through_run = 0;
  state->edges_through_symbol = 0;

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
  if( !table->symbols_in_table ) table->symbols_in_table = 256;

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


    table->symbols = libspectrum_malloc( table->symbols_in_table * sizeof( *table->symbols ) );

    for( i = 0, symbol = table->symbols;
	 i < table->symbols_in_table;
	 i++, symbol++ ) {
      symbol->edge_type = **ptr; (*ptr)++;
      symbol->lengths = libspectrum_malloc( table->max_pulses * sizeof( *symbol->lengths ) );
      for( j = 0; j < table->max_pulses; j++ ) {
	symbol->lengths[ j ] = (*ptr)[0] + (*ptr)[1] * 0x100;
	(*ptr) += 2;
      }
    }

  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

void
libspectrum_tape_block_zero( libspectrum_tape_block *block )
{
  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    block->types.generalised_data.pilot_table.symbols = NULL;
    block->types.generalised_data.data_table.symbols = NULL;
    block->types.generalised_data.pilot_symbols = NULL;
    block->types.generalised_data.pilot_repeats = NULL;
    block->types.generalised_data.data = NULL;
    break;
  default:
    break;
  }
}

