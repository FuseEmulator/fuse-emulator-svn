/* tzx.c: Routines for handling .tzx files
   Copyright (c) 2001 Philip Kendall

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

#include <stdio.h>
#include <string.h>

#include "tape.h"

/* The .tzx file signature (first 8 bytes) */
static const libspectrum_byte *signature = "ZXTape!\x1a";

/*** Local function prototypes ***/

static libspectrum_error
tzx_read_rom_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end );
static libspectrum_error
tzx_read_turbo_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end );
static libspectrum_error
tzx_read_pure_tone( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end );
static libspectrum_error
tzx_read_pulses_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end );
static libspectrum_error
tzx_read_pure_data( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end );
static libspectrum_error
tzx_read_pause( libspectrum_tape *tape, const libspectrum_byte **ptr,
		const libspectrum_byte *end );
static libspectrum_error
tzx_read_group_start( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end );
static libspectrum_error
tzx_read_jump( libspectrum_tape *tape, const libspectrum_byte **ptr,
	       const libspectrum_byte *end );
static libspectrum_error
tzx_read_loop_start( libspectrum_tape *tape, const libspectrum_byte **ptr,
		     const libspectrum_byte *end );
static libspectrum_error
tzx_read_select( libspectrum_tape *tape, const libspectrum_byte **ptr,
		 const libspectrum_byte *end );
static libspectrum_error
tzx_read_stop( libspectrum_tape *tape, const libspectrum_byte **ptr,
	       const libspectrum_byte *end );
static libspectrum_error
tzx_read_comment( libspectrum_tape *tape, const libspectrum_byte **ptr,
		  const libspectrum_byte *end );
static libspectrum_error
tzx_read_message( libspectrum_tape *tape, const libspectrum_byte **ptr,
		  const libspectrum_byte *end );
static libspectrum_error
tzx_read_archive_info( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end );
static libspectrum_error
tzx_read_hardware( libspectrum_tape *tape, const libspectrum_byte **ptr,
		   const libspectrum_byte *end );
static libspectrum_error
tzx_read_custom( libspectrum_tape *tape, const libspectrum_byte **ptr,
		 const libspectrum_byte *end );
static libspectrum_error
tzx_read_concat( const libspectrum_byte **ptr, const libspectrum_byte *end );

static libspectrum_error
tzx_read_empty_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end, libspectrum_tape_type id );

static libspectrum_error
tzx_read_data( const libspectrum_byte **ptr, const libspectrum_byte *end,
	       size_t *length, int bytes, libspectrum_byte **data );
static libspectrum_error
tzx_read_string( const libspectrum_byte **ptr, const libspectrum_byte *end,
		 libspectrum_byte **dest );

static libspectrum_error
tzx_write_rom( libspectrum_tape_rom_block *rom_block,
	       libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_turbo( libspectrum_tape_turbo_block *rom_block,
		 libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_pure_tone( libspectrum_tape_pure_tone_block *tone_block,
		     libspectrum_byte **buffer, size_t *offset,
		     size_t *length );
static libspectrum_error
tzx_write_data( libspectrum_tape_pure_data_block *data_block,
		libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_pulses( libspectrum_tape_pulses_block *pulses_block,
		  libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_pause( libspectrum_tape_pause_block *pause_block,
		 libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_group_start( libspectrum_tape_group_start_block *start_block,
		       libspectrum_byte **buffer, size_t *offset,
		       size_t *length );
static libspectrum_error
tzx_write_jump( libspectrum_tape_jump_block *block,
		libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_loop_start( libspectrum_tape_loop_start_block *block,
		      libspectrum_byte **buffer, size_t *offset,
		      size_t *length );
static libspectrum_error
tzx_write_select( libspectrum_tape_select_block *block,
		  libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_stop( libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_comment( libspectrum_tape_comment_block *comment_block,
		   libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_message( libspectrum_tape_message_block *message_block,
		   libspectrum_byte **buffer, size_t *offset, size_t *length );
static libspectrum_error
tzx_write_archive_info( libspectrum_tape_archive_info_block *info_block,
			libspectrum_byte **buffer, size_t *offset,
			size_t *length );
static libspectrum_error
tzx_write_hardware( libspectrum_tape_hardware_block *hardware_block,
		    libspectrum_byte **buffer, size_t *offset, size_t *length);
static libspectrum_error
tzx_write_custom( libspectrum_tape_custom_block *block,
		  libspectrum_byte **buffer, size_t *offset, size_t *length );

static libspectrum_error
tzx_write_empty_block( libspectrum_byte **buffer, size_t *offset,
		       size_t *length, libspectrum_tape_type id );

static libspectrum_error
tzx_write_bytes( libspectrum_byte **ptr, size_t length,
		 size_t length_bytes, libspectrum_byte *data );
static libspectrum_error
tzx_write_string( libspectrum_byte **ptr, libspectrum_byte *string );

/*** Function definitions ***/

/* The main load function */

libspectrum_error
libspectrum_tzx_create( libspectrum_tape *tape, const libspectrum_byte *buffer,
			const size_t length )
{

  libspectrum_error error;

  const libspectrum_byte *ptr, *end;

  ptr = buffer; end = buffer + length;

  /* Must be at least as many bytes as the signature, and the major/minor
     version numbers */
  if( length < strlen(signature) + 2 ) {
    libspectrum_print_error(
      "libspectrum_tzx_create: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Now check the signature */
  if( memcmp( ptr, signature, strlen( signature ) ) ) {
    libspectrum_print_error( "libspectrum_tzx_create: wrong signature\n" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }
  ptr += strlen( signature );
  
  /* Just skip the version numbers */
  ptr += 2;

  while( ptr < end ) {

    /* Get the ID of the next block */
    libspectrum_tape_type id = *ptr++;

    switch( id ) {
    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      error = tzx_read_rom_block( tape, &ptr, end );
       if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      error = tzx_read_turbo_block( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      error = tzx_read_pure_tone( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      error = tzx_read_pulses_block( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      error = tzx_read_pure_data( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      error = tzx_read_pause( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
      error = tzx_read_group_start( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
      error = tzx_read_empty_block( tape, &ptr, end, id );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      error = tzx_read_jump( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      error = tzx_read_loop_start( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
      error = tzx_read_empty_block( tape, &ptr, end, id );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
      error = tzx_read_select( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
      error = tzx_read_stop( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      error = tzx_read_comment( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
      error = tzx_read_message( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
      error = tzx_read_archive_info( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
      error = tzx_read_hardware( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      error = tzx_read_custom( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CONCAT:
      error = tzx_read_concat( &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;

    default:	/* For now, don't handle anything else */
      libspectrum_tape_free( tape );
      libspectrum_print_error(
        "libspectrum_tzx_create: unknown block type 0x%02x\n", id
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
  }

  /* And we're pointing to the first block */
  tape->current_block = tape->blocks;

  /* Which we should then initialise */
  error = libspectrum_tape_init_block(
            (libspectrum_tape_block*)tape->current_block->data
          );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_rom_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_rom_block *rom_block;
  libspectrum_error error;

  /* Check there's enough left in the buffer for the pause and the
     data length */
  if( end - (*ptr) < 4 ) {
    libspectrum_print_error(
      "tzx_read_rom_block: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_rom_block: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a standard ROM loader */
  block->type = LIBSPECTRUM_TAPE_BLOCK_ROM;
  rom_block = &(block->types.rom);

  /* Get the pause length and data length */
  rom_block->pause  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

  /* Copy the data across */
  error = tzx_read_data( ptr, end,
			 &(rom_block->length), 2, &(rom_block->data) );
  if( error ) { free( block ); return error; }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
tzx_read_turbo_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_turbo_block *turbo_block;
  libspectrum_error error;

  /* Check there's enough left in the buffer for all the metadata */
  if( end - (*ptr) < 18 ) {
    libspectrum_print_error(
      "tzx_read_turbo_block: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_turbo_block: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a turbo loader */
  block->type = LIBSPECTRUM_TAPE_BLOCK_TURBO;
  turbo_block = &(block->types.turbo);

  /* Get the metadata */
  turbo_block->pilot_length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->sync1_length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->sync2_length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->bit0_length  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->bit1_length  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->pilot_pulses = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->bits_in_last_byte = **ptr; (*ptr)++;
  turbo_block->pause        = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

  /* Read the data in */
  error = tzx_read_data( ptr, end,
			 &(turbo_block->length), 3, &(turbo_block->data) );
  if( error ) { free( block ); return error; }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
tzx_read_pure_tone( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_pure_tone_block *tone_block;

  /* Check we've got enough bytes */
  if( end - (*ptr) < 4 ) {
    libspectrum_print_error(
      "tzx_read_pure_tone: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_pure_tone: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a turbo loader */
  block->type = LIBSPECTRUM_TAPE_BLOCK_PURE_TONE;
  tone_block = &(block->types.pure_tone);

  /* Read in the data, and move along */
  tone_block->length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  tone_block->pulses = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  
  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_pulses_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_pulses_block *pulses_block;

  int i;

  /* Check the count byte exists */
  if( (*ptr) == end ) {
    libspectrum_print_error(
      "tzx_read_pulses_block: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_pulses_block: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a `list of pulses' block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_PULSES;
  pulses_block = &(block->types.pulses);

  /* Get the count byte */
  pulses_block->count = **ptr; (*ptr)++;

  /* Check enough data exists for every pulse */
  if( end - (*ptr) < 2 * pulses_block->count ) {
    free( block );
    libspectrum_print_error(
      "tzx_read_pulses_block: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory for the actual list of lengths */
  pulses_block->lengths = 
    (libspectrum_dword*)malloc( pulses_block->count*sizeof(libspectrum_dword));
  if( pulses_block->lengths == NULL ) {
    free( block );
    libspectrum_print_error( "tzx_read_pulses_block: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Copy the data across */
  for( i=0; i<pulses_block->count; i++ ) {
    pulses_block->lengths[i] = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  }

  /* Put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_pure_data( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_pure_data_block *data_block;
  libspectrum_error error;

  /* Check there's enough left in the buffer for all the metadata */
  if( end - (*ptr) < 10 ) {
    libspectrum_print_error(
      "tzx_read_pure_data: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_pure_data: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a pure data block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_PURE_DATA;
  data_block = &(block->types.pure_data);

  /* Get the metadata */
  data_block->bit0_length  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  data_block->bit1_length  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  data_block->bits_in_last_byte = **ptr; (*ptr)++;
  data_block->pause        = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

  /* And the actual data */
  error = tzx_read_data( ptr, end,
			 &(data_block->length), 3, &(data_block->data) );
  if( error ) { free( block ); return error; }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
tzx_read_pause( libspectrum_tape *tape, const libspectrum_byte **ptr,
		const libspectrum_byte *end )
{
  libspectrum_tape_block *block;
  libspectrum_tape_pause_block *pause_block;

  /* Check the pause actually exists */
  if( end - (*ptr) < 2 ) {
    libspectrum_print_error(
      "tzx_read_pause: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_pause: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a pause block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_PAUSE;
  pause_block = &(block->types.pause);

  /* Get the pause length */
  pause_block->length  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

  /* Put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* And return */
  return LIBSPECTRUM_ERROR_NONE;
}
  
static libspectrum_error
tzx_read_group_start( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end )
{
  libspectrum_tape_block *block;
  libspectrum_tape_group_start_block *start_block;
  
  libspectrum_error error;

  /* Check the length byte exists */
  if( (*ptr) == end ) {
    libspectrum_print_error(
      "tzx_read_group_start: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_group_start: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a group start block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_GROUP_START;
  start_block = &(block->types.group_start);

  /* Read in the description of the group */
  error = tzx_read_string( ptr, end, &(start_block->name) );
  if( error ) { free( block ); return error; }
			  
  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_jump( libspectrum_tape *tape, const libspectrum_byte **ptr,
	       const libspectrum_byte *end )
{
  libspectrum_tape_block *block;
  libspectrum_tape_jump_block *jump_block;
  
  /* Check the offset exists */
  if( end - (*ptr) < 2 ) {
    libspectrum_print_error(
      "tzx_read_jump: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_jump: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a jump block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_JUMP;
  jump_block = &(block->types.jump);

  /* Get the offset */
  jump_block->offset = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  if( jump_block->offset >= 32768 ) jump_block->offset -= 65536;

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_loop_start( libspectrum_tape *tape, const libspectrum_byte **ptr,
		     const libspectrum_byte *end )
{
  libspectrum_tape_block *block;
  
  /* Check the count exists */
  if( end - (*ptr) < 2 ) {
    libspectrum_print_error(
      "tzx_read_loop_start: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_loop_start: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a jump block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_LOOP_START;

  /* Get the offset */
  block->types.loop_start.count = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_select( libspectrum_tape *tape, const libspectrum_byte **ptr,
		 const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_select_block *select_block;
  libspectrum_error error;

  size_t length; int i,j;

  /* Check there's enough left in the buffer for the length and the count
     byte */
  if( end - (*ptr) < 3 ) {
    libspectrum_print_error(
      "tzx_read_select: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get the length, and check we've got that many bytes in the buffer */
  length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  if( end - (*ptr) < length ) {
    libspectrum_print_error(
      "tzx_read_select: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_select: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is an select block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_SELECT;
  select_block = &(block->types.select);

  /* Get the number of selections */
  select_block->count = **ptr; (*ptr)++;

  /* Allocate memory */
  select_block->offsets = (int*)malloc( select_block->count * sizeof(int) );
  if( select_block->offsets == NULL ) {
    free( block );
    libspectrum_print_error( "tzx_read_select: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
  select_block->descriptions = 
    (libspectrum_byte**)malloc(select_block->count*sizeof(libspectrum_byte*));
  if( select_block->descriptions == NULL ) {
    free( select_block->offsets );
    free( block );
    libspectrum_print_error( "tzx_read_select: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Read in the data */
  for( i=0; i<select_block->count; i++ ) {

    /* Check we've got the offset and a length byte */
    if( end - (*ptr) < 3 ) {
      for( j=0; j<i; j++) free( select_block->descriptions[j] );
      free( select_block->descriptions );
      free( select_block->offsets );
      libspectrum_print_error(
	"tzx_read_select: not enough data in buffer\n"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Get the offset */
    select_block->offsets[i] = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

    /* Get the description of this selection */
    error = tzx_read_string( ptr, end, &(select_block->descriptions[i] ) );
    if( error ) {
      for( j=0; j<i; j++) free( select_block->descriptions[j] );
      free( select_block->descriptions );
      free( select_block->offsets );
      return error;
    }

  }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_stop( libspectrum_tape *tape, const libspectrum_byte **ptr,
	       const libspectrum_byte *end )
{
  libspectrum_tape_block *block;

  /* Check the length field exists */
  if( end - (*ptr) < 4 ) {
    libspectrum_print_error(
      "tzx_read_stop: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* But then just skip over it, as I don't care what it is */
  (*ptr) += 4;

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_stop: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is an `stop tape if in 48K mode' block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_STOP48;

  /* Put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}  

static libspectrum_error
tzx_read_comment( libspectrum_tape *tape, const libspectrum_byte **ptr,
		  const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_comment_block *comment_block;

  libspectrum_error error;

  /* Check the length byte exists */
  if( (*ptr) == end ) {
    libspectrum_print_error(
      "tzx_read_comment: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_comment: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a comment block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_COMMENT;
  comment_block = &(block->types.comment);

  /* Get the actual comment */
  error = tzx_read_string( ptr, end, &(comment_block->text) );
  if( error ) { free( block ); return error; }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_message( libspectrum_tape *tape, const libspectrum_byte **ptr,
		  const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_message_block *message_block;
  libspectrum_error error;

  /* Check the time and length byte exists */
  if( end - (*ptr) < 2 ) {
    libspectrum_print_error(
      "tzx_read_message: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_message: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a message block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_MESSAGE;
  message_block = &(block->types.message);

  /* Get the time */
  message_block->time = **ptr; (*ptr)++;

  /* Get the message itself */
  error = tzx_read_string( ptr, end, &(message_block->text) );
  if( error ) { free( block ); return error; }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_archive_info( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_archive_info_block *info_block;
  libspectrum_error error;

  int i;

  /* Check there's enough left in the buffer for the length and the count
     byte */
  if( end - (*ptr) < 3 ) {
    libspectrum_print_error(
      "tzx_read_archive_info: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_archive_info: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is an archive info block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO;
  info_block = &(block->types.archive_info);

  /* Skip the length, as I don't care about it */
  (*ptr) += 2;

  /* Get the number of string */
  info_block->count = **ptr; (*ptr)++;

  /* Allocate memory */
  info_block->ids = (int*)malloc( info_block->count * sizeof( int ) );
  if( info_block->ids == NULL ) {
    free( block );
    libspectrum_print_error( "tzx_read_archive_info: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
  info_block->strings =
    (libspectrum_byte**)malloc( info_block->count * sizeof(libspectrum_byte*));
  if( info_block->strings == NULL ) {
    free( info_block->ids );
    free( block );
    libspectrum_print_error( "tzx_read_archive_info: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  for( i=0; i<info_block->count; i++ ) {

    /* Must be ID byte and length byte */
    if( end - (*ptr) < 2 ) {
      int j;
      for( j=0; j<i; j++ ) free( info_block->strings[i] );
      free( info_block->strings ); free( info_block->ids );
      free( block );
      libspectrum_print_error(
        "tzx_read_archive_info: not enough data in buffer\n"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Get the ID byte */
    info_block->ids[i] = **ptr; (*ptr)++;

    /* Read in the string itself */
    error = tzx_read_string( ptr, end, &(info_block->strings[i] ) );
    if( error ) {
      int j; for( j=0; j<i; j++ ) free( info_block->strings[i] );
      free( info_block->strings ); free( info_block->ids );
      free( block );
      return error;
    }

  }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_hardware( libspectrum_tape *tape, const libspectrum_byte **ptr,
		   const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_hardware_block *hardware_block;

  int i;

  /* Check there's enough left in the buffer for the count byte */
  if( (*ptr) == end ) {
    libspectrum_print_error(
      "tzx_read_hardware: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_hardware: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is an archive info block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_HARDWARE;
  hardware_block = &(block->types.hardware);

  /* Get the number of string */
  hardware_block->count = **ptr; (*ptr)++;

  /* Check there's enough data in the buffer for all the data */
  if( end - (*ptr) < 3*hardware_block->count ) {
    libspectrum_print_error(
      "tzx_read_hardware: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory */
  hardware_block->types  = (int*)malloc( hardware_block->count * sizeof(int) );
  if( hardware_block->types == NULL ) {
    free( block );
    libspectrum_print_error( "tzx_read_hardware: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
  hardware_block->ids    = (int*)malloc( hardware_block->count * sizeof(int) );
  if( hardware_block->ids == NULL ) {
    free( hardware_block->types );
    free( block );
    libspectrum_print_error( "tzx_read_hardware: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
  hardware_block->values = (int*)malloc( hardware_block->count * sizeof(int) );
  if( hardware_block->values == NULL ) {
    free( hardware_block->ids   );
    free( hardware_block->types );
    free( block );
    libspectrum_print_error( "tzx_read_hardware: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Actually read in all the data */
  for( i=0; i<hardware_block->count; i++ ) {
    hardware_block->types[i]  = **ptr; (*ptr)++;
    hardware_block->ids[i]    = **ptr; (*ptr)++;
    hardware_block->values[i] = **ptr; (*ptr)++;
  }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_custom( libspectrum_tape *tape, const libspectrum_byte **ptr,
		 const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_custom_block *custom_block;
  libspectrum_error error;

  /* Check the description (16) and length bytes (4) exist */
  if( end - (*ptr) < 20 ) {
    libspectrum_print_error(
      "tzx_read_custom: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_custom: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* This is a custom info block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_CUSTOM;
  custom_block = &(block->types.custom);

  /* Get the description */
  memcpy( custom_block->description, *ptr, 16 ); (*ptr) += 16;
  custom_block->description[16] = '\0';

  /* Read in the data */
  error = tzx_read_data( ptr, end,
			 &(custom_block->length), 4, &(custom_block->data) );
  if( error ) { free( block ); return error; }

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Concatenation block: just skip it entirely as it serves no useful
   purpose */
static libspectrum_error
tzx_read_concat( const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  /* Check there's enough data left; the -1 here is because we've already
     read the first byte of the signature as the block ID */
  if( end - (*ptr) < strlen( signature ) + 2 - 1 ) {
    libspectrum_print_error(
      "tzx_read_concat: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Skip the data */
  (*ptr) += strlen( signature ) + 2 - 1;

  return LIBSPECTRUM_ERROR_NONE;
}
  
static libspectrum_error
tzx_read_empty_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end, libspectrum_tape_type id )
{
  libspectrum_tape_block *block;

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) {
    libspectrum_print_error( "tzx_read_empty_block: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Set the type of block */
  block->type = id;

  /* Put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  return LIBSPECTRUM_ERROR_NONE;
}  

static libspectrum_error
tzx_read_data( const libspectrum_byte **ptr, const libspectrum_byte *end,
	       size_t *length, int bytes, libspectrum_byte **data )
{
  int i; libspectrum_dword multiplier = 0x01;
  size_t padding;

  if( bytes < 0 ) {
    bytes = -bytes; padding = 1;
  } else {
    padding = 0;
  }

  (*length) = 0;
  for( i=0; i<bytes; i++, multiplier *= 0x100 ) {
    *length += **ptr * multiplier; (*ptr)++;
  }

  /* Have we got enough bytes left in buffer? */
  if( ( end - (*ptr) ) < (*length) ) {
    libspectrum_print_error(
      "tzx_read_data: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory for the data */
  (*data) =
    (libspectrum_byte*)malloc( (*length+padding) * sizeof(libspectrum_byte) );
  if( (*data) == NULL ) {
    libspectrum_print_error( "tzx_read_data: out of memory\n" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Copy the block data across, and move along */
  memcpy( (*data), (*ptr), (*length) ); (*ptr) += (*length);

  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
tzx_read_string( const libspectrum_byte **ptr, const libspectrum_byte *end,
		 libspectrum_byte **dest )
{
  size_t length;
  libspectrum_error error;
  libspectrum_byte *ptr2;

  error = tzx_read_data( ptr, end, &length, -1, dest );
  if( error ) return error;
  
  /* Null terminate the string */
  (*dest)[length] = '\0';

  /* Translate line endings */
  for( ptr2 = (*dest); *ptr2; ptr2++ ) if( *ptr2 == '\r' ) *ptr2 = '\n';

  return LIBSPECTRUM_ERROR_NONE;
}

/* The main write function */

libspectrum_error
libspectrum_tzx_write( libspectrum_tape *tape,
		       libspectrum_byte **buffer, size_t *length )
{
  size_t offset; libspectrum_error error;

  GSList *ptr;

  /* First, write the .tzx signature and the version numbers */
  error = libspectrum_make_room( buffer, strlen(signature)+2, buffer, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  memcpy( (*buffer), signature, strlen( signature ) );
  offset = strlen( signature );
  (*buffer)[ offset++ ] = 1;	/* Major version number */
  (*buffer)[ offset++ ] = 13;	/* Minor version number */

  for( ptr = tape->blocks; ptr; ptr = ptr->next ) {
    libspectrum_tape_block *block = (libspectrum_tape_block*)ptr->data;

    switch( block->type ) {

    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      error = tzx_write_rom( &(block->types.rom), buffer, &offset, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      error = tzx_write_turbo( &(block->types.turbo), buffer, &offset, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      error = tzx_write_pure_tone( &(block->types.pure_tone), buffer, &offset,
				   length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      error = tzx_write_pulses( &(block->types.pulses), buffer, &offset,
				length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      error = tzx_write_data( &(block->types.pure_data), buffer, &offset,
			      length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      error = tzx_write_pause( &(block->types.pause), buffer, &offset, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
      error = tzx_write_group_start( &(block->types.group_start), buffer,
				     &offset, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
      error = tzx_write_empty_block( buffer, &offset, length, block->type );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      error = tzx_write_jump( &(block->types.jump), buffer, &offset, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      error = tzx_write_loop_start( &(block->types.loop_start), buffer,
				    &offset, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
      error = tzx_write_empty_block( buffer, &offset, length, block->type );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
      error = tzx_write_select( &(block->types.select), buffer, &offset,
				length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
      error = tzx_write_stop( buffer, &offset, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      error = tzx_write_comment( &(block->types.comment), buffer, &offset,
				 length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
      error = tzx_write_message( &(block->types.message), buffer, &offset,
				 length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
      error = tzx_write_archive_info( &(block->types.archive_info),
				      buffer, &offset, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
      error = tzx_write_hardware( &(block->types.hardware),
				  buffer, &offset, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      error = tzx_write_custom( &(block->types.custom), buffer, &offset,
				length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    default:
      free( *buffer );
      libspectrum_print_error(
        "libspectrum_tzx_write: unknown block type 0x%02x\n", block->type
      );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
  }

  (*length) = offset;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_rom( libspectrum_tape_rom_block *rom_block,
	       libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* Make room for the ID byte, the pause, the length and the actual data */
  error = libspectrum_make_room( buffer, (*offset) + 5 + rom_block->length,
				 &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the pause */
  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_ROM;
  libspectrum_write_word( ptr, rom_block->pause  ); ptr += 2;

  /* Copy the data across */
  error = tzx_write_bytes( &ptr, rom_block->length, 2, rom_block->data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  
  /* And update our offset */
  (*offset) += 5 + rom_block->length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_turbo( libspectrum_tape_turbo_block *turbo_block,
		 libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* Make room for the ID byte, the metadata and the actual data */
  error = libspectrum_make_room( buffer, (*offset) + 19 + turbo_block->length,
				 &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the metadata */
  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_TURBO;
  libspectrum_write_word( ptr, turbo_block->pilot_length ); ptr += 2;
  libspectrum_write_word( ptr, turbo_block->sync1_length ); ptr += 2;
  libspectrum_write_word( ptr, turbo_block->sync2_length ); ptr += 2;
  libspectrum_write_word( ptr, turbo_block->bit0_length  ); ptr += 2;
  libspectrum_write_word( ptr, turbo_block->bit1_length  ); ptr += 2;
  libspectrum_write_word( ptr, turbo_block->pilot_pulses ); ptr += 2;
  *ptr++ = turbo_block->bits_in_last_byte;
  libspectrum_write_word( ptr, turbo_block->pause        ); ptr += 2;

  /* Copy the data across */
  error = tzx_write_bytes( &ptr, turbo_block->length, 3, turbo_block->data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* And update our offset */
  (*offset) += 19 + turbo_block->length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pure_tone( libspectrum_tape_pure_tone_block *tone_block,
		     libspectrum_byte **buffer, size_t *offset,
		     size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* Make room for the ID byte and the data */
  error = libspectrum_make_room( buffer, (*offset) + 5, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_PURE_TONE;
  libspectrum_write_word( ptr, tone_block->length ); ptr += 2;
  libspectrum_write_word( ptr, tone_block->pulses ); ptr += 2;
  
  (*offset) += 5;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pulses( libspectrum_tape_pulses_block *pulses_block,
		  libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  size_t i;

  /* ID byte, count and 2 bytes for length of each pulse */
  size_t block_length = 2 + 2 * pulses_block->count;

  /* Make room for the ID byte, the count and the data */
  error = libspectrum_make_room( buffer, (*offset) + block_length, &ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_PULSES;
  *ptr++ = pulses_block->count;
  for( i=0; i<pulses_block->count; i++ ) {
    libspectrum_write_word( ptr, pulses_block->lengths[i] ); ptr += 2;
  }
  
  (*offset) += block_length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_data( libspectrum_tape_pure_data_block *data_block,
		libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* Make room for the ID byte, the metadata and the actual data */
  error = libspectrum_make_room( buffer, (*offset) + 11 + data_block->length,
				 &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the metadata */
  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_PURE_DATA;
  libspectrum_write_word( ptr, data_block->bit0_length  ); ptr += 2;
  libspectrum_write_word( ptr, data_block->bit1_length  ); ptr += 2;
  *ptr++ = data_block->bits_in_last_byte;
  libspectrum_write_word( ptr, data_block->pause        ); ptr += 2;

  /* Copy the data across */
  error = tzx_write_bytes( &ptr, data_block->length, 3, data_block->data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* And update our offset */
  (*offset) += 11 + data_block->length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pause( libspectrum_tape_pause_block *pause_block,
		 libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* Make room for the ID byte and the data */
  error = libspectrum_make_room( buffer, (*offset) + 3, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_PAUSE;
  libspectrum_write_word( ptr, pause_block->length ); ptr += 2;
  
  (*offset) += 3;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_group_start( libspectrum_tape_group_start_block *start_block,
		       libspectrum_byte **buffer, size_t *offset,
		       size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  size_t name_length = strlen( start_block->name );

  /* Make room for the ID byte, the length byte and the name */
  error = libspectrum_make_room( buffer, (*offset) + 2 + name_length, &ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_GROUP_START;
  
  error = tzx_write_string( &ptr, start_block->name );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  (*offset) += 2 + name_length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_jump( libspectrum_tape_jump_block *block,
		libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  int u_offset;

  /* Make room for the ID byte and the offset */
  error = libspectrum_make_room( buffer, (*offset) + 3, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_JUMP;

  u_offset = block->offset; if( u_offset < 0 ) u_offset += 65536;
  libspectrum_write_word( ptr, u_offset ); ptr += 2;
  
  (*offset) += 3;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_loop_start( libspectrum_tape_loop_start_block *block,
		      libspectrum_byte **buffer, size_t *offset,
		      size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* Make room for the ID byte and the count */
  error = libspectrum_make_room( buffer, (*offset) + 3, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_LOOP_START;

  libspectrum_write_word( ptr, block->count ); ptr += 2;
  
  (*offset) += 3;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_select( libspectrum_tape_select_block *block,
		  libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  size_t total_length; int i;

  /* The count byte, and ( 2 offset bytes and 1 length byte ) per selection */
  total_length = 1 + 3 * block->count;

  for( i=0; i<block->count; i++ )
    total_length += strlen( block->descriptions[i] );

  /* On top of that, we need an id byte and 2 length bytes */
  error = libspectrum_make_room( buffer, (*offset) + total_length + 3, &ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_SELECT;
  libspectrum_write_word( ptr, total_length ); ptr += 2;
  *ptr++ = block->count;

  for( i=0; i<block->count; i++ ) {
    libspectrum_write_word( ptr, block->offsets[i] ); ptr += 2;
    error = tzx_write_string( &ptr, block->descriptions[i] );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  (*offset) += total_length + 3;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_stop( libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* Make room for the ID byte and four length bytes */
  error = libspectrum_make_room( buffer, (*offset) + 5, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_STOP48;
  *ptr++ = *ptr++ = *ptr++ = *ptr++ = '\0';

  (*offset) += 5;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_comment( libspectrum_tape_comment_block *comment_block,
		   libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  size_t comment_length = strlen( comment_block->text );

  /* Make room for the ID byte, the length byte and the name */
  error = libspectrum_make_room( buffer, (*offset) + 2 + comment_length, &ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_COMMENT;

  error = tzx_write_string( &ptr, comment_block->text );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  (*offset) += 2 + comment_length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_message( libspectrum_tape_message_block *message_block,
		   libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  size_t text_length = strlen( message_block->text );

  /* Make room for the ID byte, the time byte, length byte and the name */
  error = libspectrum_make_room( buffer, (*offset) + 3 + text_length, &ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_MESSAGE;
  *ptr++ = message_block->time;

  error = tzx_write_string( &ptr, message_block->text );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  (*offset) += 3 + text_length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_archive_info( libspectrum_tape_archive_info_block *info_block,
			libspectrum_byte **buffer, size_t *offset,
			size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  size_t i, total_length;

  /* ID byte, 1 count byte, 2 bytes (ID and length) for
     every string */
  total_length = 2 + 2 * info_block->count;
  /* And then the length of all the strings */
  for( i=0; i<info_block->count; i++ )
    total_length += strlen( info_block->strings[i] );

  /* Make room for all that, and two bytes storing the length */
  error = libspectrum_make_room( buffer, (*offset) + total_length + 2,
				 &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write out the metadata */
  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO;
  libspectrum_write_word( ptr, total_length ); ptr += 2;
  *ptr++ = info_block->count;

  /* And the strings */
  for( i=0; i<info_block->count; i++ ) {
    *ptr++ = info_block->ids[i];
    error = tzx_write_string( &ptr, info_block->strings[i] );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  /* Update offset and return */
  (*offset) += total_length + 2;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_hardware( libspectrum_tape_hardware_block *block,
		    libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  size_t i;

  /* We need one ID byte, one count byte and then three bytes for every
     entry */
  error = libspectrum_make_room( buffer, (*offset) + 3 * block->count + 2,
				 &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write out the metadata */
  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_HARDWARE;
  *ptr++ = block->count;

  /* And the info */
  for( i=0; i<block->count; i++ ) {
    *ptr++ = block->types[i];
    *ptr++ = block->ids[i];
    *ptr++ = block->values[i];
  }

  /* Update offset and return */
  (*offset) += 3 * block->count + 2;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_custom( libspectrum_tape_custom_block *block,
		  libspectrum_byte **buffer, size_t *offset, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* An ID byte, 16 description bytes, 4 length bytes and the data
     itself */
  size_t total_length = 1 + 16 + 4 + block->length;

  /* Make room for the block */
  error = libspectrum_make_room( buffer, (*offset) + total_length, &ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = LIBSPECTRUM_TAPE_BLOCK_CUSTOM;
  memcpy( ptr, block->description, 16 ); ptr += 16;

  error = tzx_write_bytes( &ptr, block->length, 4, block->data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  (*offset) += total_length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_empty_block( libspectrum_byte **buffer, size_t *offset,
		       size_t *length, libspectrum_tape_type id )
{
  libspectrum_error error;
  libspectrum_byte *ptr = (*buffer) + (*offset);

  /* Make room for the ID byte */
  error = libspectrum_make_room( buffer, (*offset) + 1, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *ptr++ = id;

  (*offset)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_bytes( libspectrum_byte **ptr, size_t length,
		 size_t length_bytes, libspectrum_byte *data )
{
  int i;
  size_t shifter;

  /* Write out the appropriate number of length bytes */
  for( i=0, shifter = length; i<length_bytes; i++, shifter >>= 8 )
    *(*ptr)++ = shifter & 0xff;

  /* And then the actual data */
  memcpy( *ptr, data, length ); (*ptr) += length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_string( libspectrum_byte **ptr, libspectrum_byte *string )
{
  libspectrum_error error;

  size_t i, length = strlen( string );

  error = tzx_write_bytes( ptr, length, 1, string );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Fix up line endings */
  (*ptr) -= length;
  for( i=0; i<length; i++, (*ptr)++ ) if( **ptr == '\x0a' ) **ptr = '\x0d';

  return LIBSPECTRUM_ERROR_NONE;
}
