/* tape_block.c: one tape block
   Copyright (c) 2003 Philip Kendall

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


  case LIBSPECTRUM_TAPE_BLOCK_CONCAT: /* This should never occur */
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "Unknown block type %d", block->type );
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
