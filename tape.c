/* tape.c: Routines for handling tape files
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

#include <stdio.h>
#include <string.h>

#include "internals.h"
#include "tape_block.h"

/* The tape type itself */
struct libspectrum_tape {

  /* All the blocks */
  GSList* blocks;

  /* The current block */
  GSList* current_block;

  /* Where to return to after a loop, and how many iterations of the loop
     to do */
  GSList* loop_block;
  size_t loop_count;

};

/*** Constants ***/

/* The timings for the standard ROM loader */
static const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_PILOT = 2168; /*Pilot*/
static const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC1 =  667; /*Sync 1*/
static const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC2 =  735; /*Sync 2*/
static const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA0 =  855; /*Reset*/
static const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA1 = 1710; /*Set*/

/*** Local function prototypes ***/

/* Free the memory used by a block */
static void
block_free( gpointer data, gpointer user_data );

/* Functions to get the next edge */

static libspectrum_error
rom_edge( libspectrum_tape_rom_block *block, libspectrum_dword *tstates,
	  int *end_of_block );
static libspectrum_error
rom_next_bit( libspectrum_tape_rom_block *block );

static libspectrum_error
turbo_edge( libspectrum_tape_turbo_block *block, libspectrum_dword *tstates,
	    int *end_of_block );
static libspectrum_error
turbo_next_bit( libspectrum_tape_turbo_block *block );

static libspectrum_error
tone_edge( libspectrum_tape_pure_tone_block *block, libspectrum_dword *tstates,
	   int *end_of_block );

static libspectrum_error
pulses_edge( libspectrum_tape_pulses_block *block, libspectrum_dword *tstates,
	     int *end_of_block );

static libspectrum_error
pure_data_edge( libspectrum_tape_pure_data_block *block,
		libspectrum_dword *tstates, int *end_of_block );

static libspectrum_error
raw_data_edge( libspectrum_tape_raw_data_block *block,
	       libspectrum_dword *tstates, int *end_of_block );

static libspectrum_error
jump_blocks( libspectrum_tape *tape, int offset );

/*** Function definitions ****/

/* Allocate a list of blocks */
libspectrum_error
libspectrum_tape_alloc( libspectrum_tape **tape )
{
  (*tape) = (libspectrum_tape*)malloc( sizeof( libspectrum_tape ) );
  if( !(*tape) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_tape_alloc: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  (*tape)->current_block = (*tape)->blocks = NULL;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free the memory used by a list of blocks, but not the object itself */
libspectrum_error
libspectrum_tape_clear( libspectrum_tape *tape )
{
  g_slist_foreach( tape->blocks, block_free, NULL );
  g_slist_free( tape->blocks );
  tape->current_block = tape->blocks = NULL;  

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free up a list of blocks */
libspectrum_error
libspectrum_tape_free( libspectrum_tape *tape )
{
  libspectrum_error error;

  error = libspectrum_tape_clear( tape );
  if( error ) return error;

  free( tape );
  
  return LIBSPECTRUM_ERROR_NONE;
}

/* Free the memory used by one block */
static void
block_free( gpointer data, gpointer user_data GCC_UNUSED )
{
  libspectrum_tape_block_free( data );
}

/* Read in a tape file, optionally guessing what sort of file it is */
libspectrum_error
libspectrum_tape_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
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
	"libspectrum_tape_read: couldn't identify file"
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
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

  case LIBSPECTRUM_ID_TAPE_TAP:
    error = libspectrum_tap_read( tape, buffer, length ); break;

  case LIBSPECTRUM_ID_TAPE_TZX:
    error = libspectrum_tzx_read( tape, buffer, length ); break;

  case LIBSPECTRUM_ID_TAPE_WARAJEVO:
    error = libspectrum_warajevo_read( tape, buffer, length ); break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "libspectrum_tape_read: not a tape file" );
    free( new_buffer );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  free( new_buffer );
  return error;
}

libspectrum_error
libspectrum_tape_write( libspectrum_byte **buffer, size_t *length,
			libspectrum_tape *tape, libspectrum_id_t type )
{
  libspectrum_class_t class;
  libspectrum_error error;

  error = libspectrum_identify_class( &class, type );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_TAPE ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID,
			     "libspectrum_tape_write: not a tape format" );
    return LIBSPECTRUM_ERROR_INVALID;
  }

  switch( type ) {

  case LIBSPECTRUM_ID_TAPE_TAP:
    return libspectrum_tap_write( buffer, length, tape );

  case LIBSPECTRUM_ID_TAPE_TZX:
    return libspectrum_tzx_write( buffer, length, tape );

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "libspectrum_tape_write: format not supported" );
    return LIBSPECTRUM_ERROR_UNKNOWN;

  }
}

/* Does this tape structure actually contain a tape? */
int
libspectrum_tape_present( const libspectrum_tape *tape )
{
  return tape->blocks != NULL;
}

/* Some flags which may be set after calling libspectrum_tape_get_next_edge */
const int LIBSPECTRUM_TAPE_FLAGS_BLOCK  = 1 << 0; /* End of block */
const int LIBSPECTRUM_TAPE_FLAGS_STOP   = 1 << 1; /* Stop tape */
const int LIBSPECTRUM_TAPE_FLAGS_STOP48 = 1 << 2; /* Stop tape if in
						     48K mode */

/* The main function: called with a tape object and returns the number of
   t-states until the next edge, and a marker if this was the last edge
   on the tape */
libspectrum_error
libspectrum_tape_get_next_edge( libspectrum_dword *tstates, int *flags,
	                        libspectrum_tape *tape )
{
  int error;

  libspectrum_tape_block *block =
    (libspectrum_tape_block*)tape->current_block->data;

  /* Has this edge ended the block? */
  int end_of_block = 0;

  /* After getting a new block, do we want to advance to the next one? */
  int no_advance = 0;

  /* Assume no special flags by default */
  *flags = 0;

  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    error = rom_edge( &(block->types.rom), tstates, &end_of_block );
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    error = turbo_edge( &(block->types.turbo), tstates, &end_of_block );
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    error = tone_edge( &(block->types.pure_tone), tstates, &end_of_block );
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    error = pulses_edge( &(block->types.pulses), tstates, &end_of_block );
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    error = pure_data_edge( &(block->types.pure_data), tstates, &end_of_block);
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    error = raw_data_edge( &(block->types.raw_data), tstates, &end_of_block );
    if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    *tstates = ( block->types.pause.length * 69888 ) / 20; end_of_block = 1;
    /* 0 ms pause => stop tape */
    if( *tstates == 0 ) { *flags |= LIBSPECTRUM_TAPE_FLAGS_STOP; }
    break;

  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    error = jump_blocks( tape, block->types.jump.offset );
    if( error ) return error;
    *tstates = 0; end_of_block = 1; no_advance = 1;
    break;

  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    tape->loop_block = tape->current_block->next;
    tape->loop_count = block->types.loop_start.count;
    *tstates = 0; end_of_block = 1;
    break;

  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    if( --tape->loop_count ) {
      tape->current_block = tape->loop_block;
      no_advance = 1;
    }
    *tstates = 0; end_of_block = 1;
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    *tstates = 0; *flags |= LIBSPECTRUM_TAPE_FLAGS_STOP48; end_of_block = 1;
    break;

  /* For blocks which contain no Spectrum-readable data, return zero
     tstates and set end of block set so we instantly get the next block */
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START: 
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    *tstates = 0; end_of_block = 1;
    break;

  default:
    *tstates = 0;
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_tape_get_next_edge: unknown block type 0x%02x",
      block->type
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  /* If that ended the block, move onto the next block */
  if( end_of_block ) {

    *flags |= LIBSPECTRUM_TAPE_FLAGS_BLOCK;

    /* Advance to the next block, unless we've been told not to */
    if( !no_advance ) {

      tape->current_block = tape->current_block->next;

      /* If we've just hit the end of the tape, stop the tape (and
	 then `rewind' to the start) */
      if( tape->current_block == NULL ) {
	*flags |= LIBSPECTRUM_TAPE_FLAGS_STOP;
	tape->current_block = tape->blocks;
      }
    }

    /* Initialise the new block */
    error = libspectrum_tape_block_init( tape->current_block->data );
    if( error ) return error;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rom_edge( libspectrum_tape_rom_block *block, libspectrum_dword *tstates,
	  int *end_of_block )
{
  int error;

  switch( block->state ) {

  case LIBSPECTRUM_TAPE_STATE_PILOT:
    /* The next edge occurs in one pilot edge timing */
    *tstates = LIBSPECTRUM_TAPE_TIMING_PILOT;
    /* If that was the last pilot edge, change state */
    if( --(block->edge_count) == 0 )
      block->state = LIBSPECTRUM_TAPE_STATE_SYNC1;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC1:
    /* The first short sync pulse */
    *tstates = LIBSPECTRUM_TAPE_TIMING_SYNC1;
    /* Followed by the second sync pulse */
    block->state = LIBSPECTRUM_TAPE_STATE_SYNC2;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC2:
    /* The second short sync pulse */
    *tstates = LIBSPECTRUM_TAPE_TIMING_SYNC2;
    /* Followed by the first bit of data */
    error = rom_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    /* The first edge for a bit of data */
    *tstates = block->bit_tstates;
    /* Followed by the second edge */
    block->state = LIBSPECTRUM_TAPE_STATE_DATA2;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA2:
    /* The second edge for a bit of data */
    *tstates = block->bit_tstates;
    /* Followed by the next bit of data (or the end of data) */
    error = rom_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = (block->pause * 69888)/20; /* FIXME: should vary with tstates
					     per frame */
    *end_of_block = 1;
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "rom_edge: unknown state %d", block->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rom_next_bit( libspectrum_tape_rom_block *block )
{
  int next_bit;

  /* Have we finished the current byte? */
  if( ++(block->bits_through_byte) == 8 ) {

    /* If so, have we finished the entire block? If so, all we've got
       left after this is the pause at the end */
    if( ++(block->bytes_through_block) == block->length ) {
      block->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
      return LIBSPECTRUM_ERROR_NONE;
    }
    
    /* If we've finished the current byte, but not the entire block,
       get the next byte */
    block->current_byte = block->data[ block->bytes_through_block ];
    block->bits_through_byte = 0;
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = block->current_byte & 0x80;
  block->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  block->bit_tstates = ( next_bit ? LIBSPECTRUM_TAPE_TIMING_DATA1
			          : LIBSPECTRUM_TAPE_TIMING_DATA0 );
  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_edge( libspectrum_tape_turbo_block *block, libspectrum_dword *tstates,
	    int *end_of_block )
{
  int error;

  switch( block->state ) {

  case LIBSPECTRUM_TAPE_STATE_PILOT:
    /* The next edge occurs in one pilot edge timing */
    *tstates = block->pilot_length;
    /* If that was the last pilot edge, change state */
    if( --(block->edge_count) == 0 )
      block->state = LIBSPECTRUM_TAPE_STATE_SYNC1;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC1:
    /* The first short sync pulse */
    *tstates = block->sync1_length;
    /* Followed by the second sync pulse */
    block->state = LIBSPECTRUM_TAPE_STATE_SYNC2;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC2:
    /* The second short sync pulse */
    *tstates = block->sync2_length;
    /* Followed by the first bit of data */
    error = turbo_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    /* The first edge for a bit of data */
    *tstates = block->bit_tstates;
    /* Followed by the second edge */
    block->state = LIBSPECTRUM_TAPE_STATE_DATA2;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA2:
    /* The second edge for a bit of data */
    *tstates = block->bit_tstates;
    /* Followed by the next bit of data (or the end of data) */
    error = turbo_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = (block->pause * 69888)/20; /* FIXME: should vary with tstates
					     per frame */
    *end_of_block = 1;
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "turbo_edge: unknown state %d", block->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_next_bit( libspectrum_tape_turbo_block *block )
{
  int next_bit;

  /* Have we finished the current byte? */
  if( ++(block->bits_through_byte) == 8 ) {

    /* If so, have we finished the entire block? If so, all we've got
       left after this is the pause at the end */
    if( ++(block->bytes_through_block) == block->length ) {
      block->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
      return LIBSPECTRUM_ERROR_NONE;
    }
    
    /* If we've finished the current byte, but not the entire block,
       get the next byte */
    block->current_byte = block->data[ block->bytes_through_block ];

    /* If we're looking at the last byte, take account of the fact it
       may have less than 8 bits in it */
    if( block->bytes_through_block == block->length-1 ) {
      block->bits_through_byte = 8 - block->bits_in_last_byte;
    } else {
      block->bits_through_byte = 0;
    }
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = block->current_byte & 0x80;
  block->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  block->bit_tstates = ( next_bit ? block->bit1_length : block->bit0_length );
  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tone_edge( libspectrum_tape_pure_tone_block *block, libspectrum_dword *tstates,
	   int *end_of_block )
{
  /* The next edge occurs in one pilot edge timing */
  *tstates = block->length;
  /* If that was the last edge, the block is finished */
  if( --(block->edge_count) == 0 ) (*end_of_block) = 1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pulses_edge( libspectrum_tape_pulses_block *block, libspectrum_dword *tstates,
	     int *end_of_block )
{
  /* Get the length of this edge */
  *tstates = block->lengths[ block->edge_count ];
  /* Was that the last edge? */
  if( ++(block->edge_count) == block->count ) (*end_of_block) = 1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pure_data_edge( libspectrum_tape_pure_data_block *block,
		libspectrum_dword *tstates, int *end_of_block )
{
  int error;

  switch( block->state ) {

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    /* The first edge for a bit of data */
    *tstates = block->bit_tstates;
    /* Followed by the second edge */
    block->state = LIBSPECTRUM_TAPE_STATE_DATA2;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA2:
    /* The second edge for a bit of data */
    *tstates = block->bit_tstates;
    /* Followed by the next bit of data (or the end of data) */
    error = libspectrum_tape_pure_data_next_bit( block );
    if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = (block->pause * 69888)/20; /* FIXME: should vary with tstates
					     per frame */
    *end_of_block = 1;
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "pure_data_edge: unknown state %d",
			     block->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_pure_data_next_bit( libspectrum_tape_pure_data_block *block )
{
  int next_bit;

  /* Have we finished the current byte? */
  if( ++(block->bits_through_byte) == 8 ) {

    /* If so, have we finished the entire block? If so, all we've got
       left after this is the pause at the end */
    if( ++(block->bytes_through_block) == block->length ) {
      block->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
      return LIBSPECTRUM_ERROR_NONE;
    }
    
    /* If we've finished the current byte, but not the entire block,
       get the next byte */
    block->current_byte = block->data[ block->bytes_through_block ];

    /* If we're looking at the last byte, take account of the fact it
       may have less than 8 bits in it */
    if( block->bytes_through_block == block->length-1 ) {
      block->bits_through_byte = 8 - block->bits_in_last_byte;
    } else {
      block->bits_through_byte = 0;
    }
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = block->current_byte & 0x80;
  block->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  block->bit_tstates = ( next_bit ? block->bit1_length : block->bit0_length );
  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
raw_data_edge( libspectrum_tape_raw_data_block *block,
	       libspectrum_dword *tstates, int *end_of_block )
{
  int error;

  switch (block->state) {
  case LIBSPECTRUM_TAPE_STATE_DATA1:
    *tstates = block->bit_tstates;
    error = libspectrum_tape_raw_data_next_bit( block );
    if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = ( block->pause * 69888 )/20; /* FIXME: should vary with tstates
					       per frame */
    *end_of_block = 1;
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "raw_edge: unknown state %d", block->state );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_raw_data_next_bit( libspectrum_tape_raw_data_block *block )
{
  int length = 0;

  if( block->bytes_through_block == block->length ) {
    block->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
    return LIBSPECTRUM_ERROR_NONE;
  }

  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  /* Step through the data until we find an edge */
  do {
    length++;
    if( ++block->bits_through_byte == 8 ) {
      if( ++block->bytes_through_block == block->length - 1 ) {
	block->bits_through_byte = 8 - block->bits_in_last_byte;
      } else {
	block->bits_through_byte = 0;
      }
      if( block->bytes_through_block == block->length )
	break;
    }
  } while( (block->data[block->bytes_through_block] << block->bits_through_byte
            & 0x80 ) != block->last_bit) ;

  block->bit_tstates = length * block->bit_length;
  block->last_bit ^= 0x80;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
jump_blocks( libspectrum_tape *tape, int offset )
{
  gint current_position; GSList *new_block;

  current_position = g_slist_position( tape->blocks, tape->current_block );
  if( current_position == -1 ) return LIBSPECTRUM_ERROR_LOGIC;

  new_block = g_slist_nth( tape->blocks, current_position + offset );
  if( new_block == NULL ) return LIBSPECTRUM_ERROR_CORRUPT;

  tape->current_block = new_block;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Get the current block */
libspectrum_tape_block*
libspectrum_tape_current_block( libspectrum_tape *tape )
{
  return tape->current_block ? tape->current_block->data : NULL;
}

/* Peek at the next block on the tape */
libspectrum_tape_block*
libspectrum_tape_peek_next_block( libspectrum_tape *tape )
{
  if( !tape->current_block ) return NULL;

  return tape->current_block->next ? tape->current_block->next->data
				   : tape->blocks->data;
}

/* Cause the next block on the tape to be active, initialise it
   and return it */
libspectrum_tape_block*
libspectrum_tape_select_next_block( libspectrum_tape *tape )
{
  
  if( !tape->current_block ) return NULL;

  tape->current_block = tape->current_block->next;
  if( !tape->current_block ) tape->current_block = tape->blocks;

  if( libspectrum_tape_block_init( tape->current_block->data ) ) return NULL;

  return tape->current_block->data;
}
  
/* Get the position on the tape of the current block */
libspectrum_error
libspectrum_tape_position( int *n, libspectrum_tape *tape )
{
  *n = g_slist_position( tape->blocks, tape->current_block );

  if( *n == -1 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_tape_position: current block is not in tape!"
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* Select the nth block on the tape */
libspectrum_error
libspectrum_tape_nth_block( libspectrum_tape *tape, int n )
{
  GSList *new_block;
  libspectrum_error error;

  new_block = g_slist_nth( tape->blocks, n );
  if( !new_block ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_tape_nth_block: tape does not have block %d", n
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  tape->current_block = new_block;

  error = libspectrum_tape_block_init( tape->current_block->data );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_append_block( libspectrum_tape *tape,
			       libspectrum_tape_block *block )
{
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* If we previously didn't have a tape loaded ( implied by
     tape->current_block == NULL ), set up so that we point to the
     start of the tape */
  if( !tape->current_block ) {
    tape->current_block = tape->blocks;
    libspectrum_tape_block_init( tape->blocks->data );
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_block_description( char *buffer, size_t length,
	                            libspectrum_tape_block *block )
{
  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    strncpy( buffer, "Standard Speed Data", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    strncpy( buffer, "Turbo Speed Data", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    strncpy( buffer, "Pure Tone", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    strncpy( buffer, "List of Pulses", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    strncpy( buffer, "Pure Data", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    strncpy( buffer, "Raw Data", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    strncpy( buffer, "Pause", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    strncpy( buffer, "Group Start", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    strncpy( buffer, "Group End", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    strncpy( buffer, "Jump", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    strncpy( buffer, "Loop Start Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    strncpy( buffer, "Loop End", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    strncpy( buffer, "Select", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    strncpy( buffer, "Stop Tape If In 48K Mode", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    strncpy( buffer, "Comment", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    strncpy( buffer, "Message", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    strncpy( buffer, "Archive Info", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    strncpy( buffer, "Hardware Information", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    strncpy( buffer, "Custom Info", length );
    break;

  default:
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_tape_block_description: unknown block type 0x%02x",
      block->type
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  buffer[ length-1 ] = '\0';
  return LIBSPECTRUM_ERROR_NONE;
}

/* Given a tape file, attempt to guess which sort of hardware it should run
   on by looking for a hardware block (0x33).

   Deliberately not mentioned in libspectrum.h(.in) as I'm really not
   sure this function is actually useful.
*/
libspectrum_error
libspectrum_tape_guess_hardware( libspectrum_machine *machine,
				 const libspectrum_tape *tape )
{
  GSList *ptr; int score, current_score; size_t i;

  *machine = LIBSPECTRUM_MACHINE_UNKNOWN; current_score = 0;

  if( !libspectrum_tape_present( tape ) ) return LIBSPECTRUM_ERROR_NONE;

  for( ptr = tape->blocks; ptr; ptr = ptr->next ) {

    libspectrum_tape_block *block = (libspectrum_tape_block*)ptr->data;
    libspectrum_tape_hardware_block *hardware;

    if( block->type != LIBSPECTRUM_TAPE_BLOCK_HARDWARE ) continue;

    hardware = &( block->types.hardware );

    for( i=0; i<hardware->count; i++ ) {

      /* Only interested in which computer types this tape runs on */
      if( hardware->types[i] != 0 ) continue;

      /* Skip if the tape doesn't run on this hardware */
      if( hardware->values[i] == 3 ) continue;

      /* If the tape uses the special hardware, choose that preferentially.
	 If it doesn't (or is unknown), it's a possibility */
      if( hardware->values[i] == 1 ) { score = 2; } else { score = 1; }

      if( score <= current_score ) continue;

      switch( hardware->ids[i] ) {

      case 0: /* 16K Spectrum */
	*machine = LIBSPECTRUM_MACHINE_16; current_score = score;
	break;

      case 1: /* 48K Spectrum */
      case 2: /* 48K Issue 1 Spectrum */
	*machine = LIBSPECTRUM_MACHINE_48; current_score = score;
	break;

      case 3: /* 128K Spectrum */
	*machine = LIBSPECTRUM_MACHINE_128; current_score = score;
	break;

      case 4: /* +2 */
	*machine = LIBSPECTRUM_MACHINE_PLUS2; current_score = score;
	break;

      case 5: /* +2A and +3. Gee, thanks to whoever designed the TZX format
		 for distinguishing those :-( */
	*machine = LIBSPECTRUM_MACHINE_PLUS3; current_score = score;
	break;

      case 6: /* TC2048 */
	*machine = LIBSPECTRUM_MACHINE_TC2048; current_score = score;
	break;

      }
    }
  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

/*
 * Tape iterator functions
 */

libspectrum_tape_block*
libspectrum_tape_iterator_init( libspectrum_tape_iterator *iterator,
				libspectrum_tape *tape )
{
  *iterator = tape->blocks;
  return *iterator ? (*iterator)->data : NULL;
}

libspectrum_tape_block*
libspectrum_tape_iterator_next( libspectrum_tape_iterator *iterator )
{
  *iterator = (*iterator)->next;
  return *iterator ? (*iterator)->data : NULL;
}
