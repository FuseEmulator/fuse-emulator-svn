/* tape.c: tape handling routines
   Copyright (c) 1999-2004 Philip Kendall, Darren Salt, Witold Filipczyk

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libspectrum.h>

#include "event.h"
#include "fuse.h"
#include "machine.h"
#include "memory.h"
#include "scld.h"
#include "settings.h"
#include "sound.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "trdos.h"
#include "ui/ui.h"
#include "utils.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The current tape */
static libspectrum_tape *tape;

/* Has the current tape been modified since it was last loaded/saved? */
int tape_modified;

/* Is the emulated tape deck playing? */
static int tape_playing;

/* Is there a high input to the EAR socket? */
int tape_microphone;

/* Function prototypes */

static int tape_autoload( libspectrum_machine hardware );
static int trap_load_block( libspectrum_tape_block *block );
int trap_check_rom( void );
static void make_name( char *name, const char *data );

/* Function defintions */

int tape_init( void )
{
  libspectrum_error error;

  error = libspectrum_tape_alloc( &tape );
  if( !tape ) return error;

  tape_modified = 0;

  /* Don't call tape_stop() here as the UI hasn't been initialised yet,
     so we can't update the statusbar */
  tape_playing = 0;
  tape_microphone = 0;
  if( settings_current.sound_load ) sound_beeper( 1, tape_microphone );
  return 0;
}

/* Open a tape using the current option for whether to autoload or not */
int
tape_open_default_autoload( const char *filename, void *data )
{
  return tape_open( filename, settings_current.auto_load );
}

int tape_open( const char *filename, int autoload )
{
  utils_file file;
  int error;

  error = utils_read_file( filename, &file );
  if( error ) return error;

  error = tape_read_buffer( file.buffer, file.length, LIBSPECTRUM_ID_UNKNOWN,
			    filename, autoload );
  if( error ) { utils_close_file( &file ); return error; }

  utils_close_file( &file );

  return 0;
}

/* Use an already open tape file as the current tape */
int
tape_read_buffer( unsigned char *buffer, size_t length, libspectrum_id_t type,
		  const char *filename, int autoload )
{
  int error;

  if( libspectrum_tape_present( tape ) ) {
    error = tape_close(); if( error ) return error;
  }

  error = libspectrum_tape_read( tape, buffer, length, type, filename );
  if( error ) return error;

  tape_modified = 0;
  ui_tape_browser_update();

  if( autoload ) {
    error = tape_autoload( machine_current->machine );
    if( error ) return error;
  }

  return 0;
}

/* Load a snap to start the current tape autoloading */
static int
tape_autoload( libspectrum_machine hardware )
{
  int error; const char *id; int fd;
  char filename[80];
  utils_file snap;

  id = machine_get_id( hardware );
  if( !id ) {
    ui_error( UI_ERROR_ERROR, "Unknown machine type %d!", hardware );
    return 1;
  }

  snprintf( filename, 80, "tape_%s.z80", id );
  fd = utils_find_auxiliary_file( filename, UTILS_AUXILIARY_LIB );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR,
	      "Couldn't find autoload snap for machine type '%s'", id );
    return 1;
  }

  error = utils_read_fd( fd, filename, &snap );
  if( error ) return error;

  error = snapshot_read_buffer( snap.buffer, snap.length,
				LIBSPECTRUM_ID_SNAPSHOT_Z80 );
  if( error ) { utils_close_file( &snap ); return error; }

  if( utils_close_file( &snap ) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't munmap '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }
    
  return 0;
}

/* Close the active tape file */
int tape_close( void )
{
  int error;
  ui_confirm_save_t confirm;

  /* If the tape has been modified, check if we want to do this */
  if( tape_modified ) {

    confirm = ui_confirm_save( "Tape has been modified" );
    switch( confirm ) {

    case UI_CONFIRM_SAVE_SAVE:
      error = ui_tape_write(); if( error ) return error;
      break;

    case UI_CONFIRM_SAVE_DONTSAVE: break;
    case UI_CONFIRM_SAVE_CANCEL: return 1;

    }
  }

  /* Stop the tape if it's currently playing */
  if( tape_playing ) {
    error = tape_stop();
    if( error ) return error;
  }

  /* And then remove it from memory */
  error = libspectrum_tape_clear( tape );
  if( error ) return error;

  tape_modified = 0;
  ui_tape_browser_update();

  return 0;
}

/* Select the nth block on the tape; 0 => 1st block */
int
tape_select_block( size_t n )
{
  int error;

  error = tape_select_block_no_update( n ); if( error ) return error;

  ui_tape_browser_update();

  return 0;
}

/* The same, but without updating the browser display */
int
tape_select_block_no_update( size_t n )
{
  return libspectrum_tape_nth_block( tape, n );
}

/* Which block is current? */
int
tape_get_current_block( void )
{
  int n;
  libspectrum_error error;

  if( !libspectrum_tape_present( tape ) ) return -1;

  error = libspectrum_tape_position( &n, tape );
  if( error ) return -1;

  return n;
}

/* Write the current in-memory tape file out to disk */
int tape_write( const char* filename )
{
  libspectrum_id_t type;
  libspectrum_class_t class;
  libspectrum_byte *buffer; size_t length;

  int error;

  /* Work out what sort of file we want from the filename; default to
     .tzx if we couldn't guess */
  error = libspectrum_identify_file_with_class( &type, &class, filename, NULL,
						0 );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_TAPE || type == LIBSPECTRUM_ID_UNKNOWN )
    type = LIBSPECTRUM_ID_TAPE_TZX;

  length = 0;

  error = libspectrum_tape_write( &buffer, &length, tape, type );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = utils_write_file( filename, buffer, length );
  if( error ) { free( buffer ); return error; }

  tape_modified = 0;
  ui_tape_browser_update();

  return 0;

}

/* Load the next tape block into memory; returns 0 if a block was
   loaded (even if it had an tape loading error or equivalent) or
   non-zero if there was an error at the emulator level, or tape traps
   are not active */
int tape_load_trap( void )
{
  libspectrum_tape_block *block, *next_block;
  int error;

  /* Do nothing if tape traps aren't active, or the tape is already playing */
  if( ! settings_current.tape_traps || tape_playing ) return 2;

  /* Do nothing if we're not in the correct ROM */
  if( ! trap_check_rom() ) return 3;

  /* Return with error if no tape file loaded */
  if( !libspectrum_tape_present( tape ) ) return 1;

  block = libspectrum_tape_current_block( tape );

  /* If this block isn't a ROM loader, start it playing
     Then return with `error' so that we actually do whichever instruction
     it was that caused the trap to hit */
  if( libspectrum_tape_block_type( block ) != LIBSPECTRUM_TAPE_BLOCK_ROM ) {

    error = tape_play();
    if( error ) return 3;

    return -1;
  }

  /* All returns made via the RET at #05E2, except on Timex 2068 at #0136 */
  if ( machine_current->machine == LIBSPECTRUM_MACHINE_TC2068 ) {
    PC = 0x0136;
  } else {
    PC = 0x05e2;
  }

  error = trap_load_block( block );
  if( error ) return error;

  /* Peek at the next block. If it's a ROM block, move along, initialise
     the block, and return */
  next_block = libspectrum_tape_peek_next_block( tape );

  if( libspectrum_tape_block_type(next_block) == LIBSPECTRUM_TAPE_BLOCK_ROM ) {

    next_block = libspectrum_tape_select_next_block( tape );
    if( !next_block ) return 1;

    ui_tape_browser_update();

    return 0;
  }

  /* If the next block isn't a ROM block, set ourselves up such that the
     next thing to occur is the pause at the end of the current block */
  libspectrum_tape_block_set_state( block, LIBSPECTRUM_TAPE_STATE_PAUSE );

  /* And start the tape playing */
  error = tape_play();
  /* On error, still return without error as we did sucessfully do
     the tape trap, and so don't want to do the trigger instruction */
  if( error ) return 0;

  return 0;

}

static int
trap_load_block( libspectrum_tape_block *block )
{
  libspectrum_byte parity, *data;
  int loading, i;

  /* If the block's too short, give up and go home (with carry reset
     to indicate error. The +2 is to deal with the party and checksum
     bytes which aren't accounted for in DE */
  if( libspectrum_tape_block_data_length( block ) < DE + 2 ) { 
    F = ( F & ~FLAG_C );
    return 0;
  }

  data = libspectrum_tape_block_data( block );
  parity = *data;

  /* If the flag byte (stored in A') does not match, reset carry and return */
  if( *data++ != A_ ) {
    F = ( F & ~FLAG_C );
    return 0;
  }

  /* Loading or verifying determined by the carry flag of F' */
  loading = ( F_ & FLAG_C );

  if( loading ) {
    for( i=0; i<DE; i++ ) {
      writebyte_internal( IX+i, *data );
      parity ^= *data++;
    }
  } else {		/* verifying */
    for( i=0; i<DE; i++) {
      parity ^= *data;
      if( *data++ != readbyte_internal(IX+i) ) {
	F = ( F & ~FLAG_C );
	return 0;
      }
    }
  }

  /* If the parity byte does not match, reset carry and return */
  if( *data++ != parity ) {
    F = ( F & ~FLAG_C );
    return 0;
  }

  /* Else return with carry set */
  F |= FLAG_C;

  return 0;
}

/* Append to the current tape file in memory; returns 0 if a block was
   saved or non-zero if there was an error at the emulator level, or tape
   traps are not active */
int tape_save_trap( void )
{
  libspectrum_tape_block *block;
  libspectrum_byte parity, *data;
  size_t length;

  int i; libspectrum_error error;

  /* Do nothing if tape traps aren't active */
  if( ! settings_current.tape_traps ) return 2;

  /* Check we're in the right ROM */
  if( ! trap_check_rom() ) return 3;

  /* Get a new block to store this data in */
  error = libspectrum_tape_block_alloc( &block, LIBSPECTRUM_TAPE_BLOCK_ROM );
  if( error ) return error;
  
  /* The +2 here is for the flag and parity bytes */
  length = DE + 2;
  libspectrum_tape_block_set_data_length( block, length );

  data = malloc( length * sizeof(libspectrum_byte) );
  if( !data ) { free( block ); return 1; }
  libspectrum_tape_block_set_data( block, data );

  /* First, store the flag byte (and initialise the parity counter) */
  data[0] = parity = A;

  /* then the main body of the data, counting parity along the way */
  for( i=0; i<DE; i++) {
    libspectrum_byte b = readbyte_internal( IX+i );
    parity ^= b;
    data[i+1] = b;
  }

  /* And finally the parity byte */
  data[ DE+1 ] = parity;

  /* Give a 1 second pause after this block */
  libspectrum_tape_block_set_pause( block, 1000 );

  /* Add the block to the current tape file */
  error = libspectrum_tape_append_block( tape, block );
  if( error ) return error;

  tape_modified = 1;
  ui_tape_browser_update();

  /* And then return via the RET at #053E, except on Timex 2068 at #00E4 */
  if ( machine_current->machine == LIBSPECTRUM_MACHINE_TC2068 ) {
    PC = 0x00e4;
  } else {
    PC = 0x053e;
  }

  return 0;

}

/* Check whether we're actually in the right ROM when a tape trap hit */
int trap_check_rom( void )
{
  switch( machine_current->machine ) {
  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_TC2048:
    return 1;		/* Always OK here */

  case LIBSPECTRUM_MACHINE_TC2068:
    /* OK if we're in the EXROM (location of the tape routines) */
    return( memory_map[0].page == timex_exrom[0].page );

  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
    /* OK if we're in ROM 1 */
    return( machine_current->ram.current_rom == 1 );

  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
    /* OK if we're not in a 64Kb RAM configuration and we're in
       ROM 3 */
    return( ! machine_current->ram.special &&
	    machine_current->ram.current_rom == 3 );

  case LIBSPECTRUM_MACHINE_PENT:
  case LIBSPECTRUM_MACHINE_SCORP:
    /* OK if we're in ROM 1  and TRDOS is not active */
    return( machine_current->ram.current_rom == 1 && !trdos_active );

  case LIBSPECTRUM_MACHINE_UNKNOWN:	/* should never happen */
    ui_error( UI_ERROR_ERROR,
	      "trap_check_rom: machine type is LIBSPECTRUM_MACHINE_UNKNOWN" );
    fuse_abort();

  }

  ui_error( UI_ERROR_ERROR, "trap_check_rom: unknown machine type %d",
	    machine_current->machine );
  fuse_abort();
}

int tape_play( void )
{
  libspectrum_tape_block* block;

  int error;

  if( !libspectrum_tape_present( tape ) ) return 1;
  
  block = libspectrum_tape_current_block( tape );

  /* If tape traps are active and the current block is a ROM block, do
     nothing, _unless_ the ROM block has already reached the pause at
     its end which (hopefully) means we're in the magic state involving
     starting slow loading whilst tape traps are active */
  if( settings_current.tape_traps &&
      libspectrum_tape_block_type( block ) == LIBSPECTRUM_TAPE_BLOCK_ROM &&
      libspectrum_tape_block_state( block ) != LIBSPECTRUM_TAPE_STATE_PAUSE )
    return 0;

  /* Otherwise, start the tape going */
  tape_playing = 1;
  tape_microphone = 0;

  /* Update the status bar */
  ui_statusbar_update( UI_STATUSBAR_ITEM_TAPE, UI_STATUSBAR_STATE_ACTIVE );

  /* Timex machines have no loading noise */
  if( ( !( machine_current->timex ) ) && settings_current.sound_load )
    sound_beeper( 1, tape_microphone );

  error = tape_next_edge( tstates ); if( error ) return error;

  return 0;
}

int tape_toggle_play( void )
{
  if( tape_playing ) {
    return tape_stop();
  } else {
    return tape_play();
  }
}

int tape_stop( void )
{
  tape_playing = 0;
  ui_statusbar_update( UI_STATUSBAR_ITEM_TAPE, UI_STATUSBAR_STATE_INACTIVE );
  return 0;
}

int
tape_next_edge( libspectrum_dword last_tstates )
{
  int error; libspectrum_error libspec_error;
  libspectrum_tape_block *block;

  libspectrum_dword edge_tstates;
  int flags;

  /* If the tape's not playing, just return */
  if( ! tape_playing ) return 0;

  /* Get the time until the next edge */
  libspec_error = libspectrum_tape_get_next_edge( &edge_tstates, &flags,
						  tape );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) return libspec_error;

  /* Invert the microphone state */
  if( edge_tstates || ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP ) ) {
    tape_microphone = !tape_microphone;
    /* Timex machines have no loading noise */
    if( !machine_current->timex && settings_current.sound_load )
      sound_beeper( 1, tape_microphone );
  }

  /* If we've been requested to stop the tape, do so and then
     return without stacking another edge */
  if( ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP ) ||
      ( ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP48 ) && 
	( !( libspectrum_machine_capabilities( machine_current->machine ) &
	     LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY
	   )
	)
      )
    )
  {
    error = tape_stop(); if( error ) return error;
    return 0;
  }

  /* If that was the end of a block, update the browser */
  if( flags & LIBSPECTRUM_TAPE_FLAGS_BLOCK ) {

    ui_tape_browser_update();

    /* If tape traps are active _and_ the new block is a ROM loader,
     return without putting another event into the queue */
    block = libspectrum_tape_current_block( tape );
    if( settings_current.tape_traps &&
	libspectrum_tape_block_type( block ) == LIBSPECTRUM_TAPE_BLOCK_ROM
      ) {
      error = tape_stop(); if( error ) return error;
      return 0;
    }
  }

  /* Otherwise, put this into the event queue; remember that this edge
     should occur 'edge_tstates' after the last edge, not after the
     current time (these will be slightly different as we only process
     events between instructions). */
  error = event_add( last_tstates + edge_tstates, EVENT_TYPE_EDGE );
  if( error ) return error;

  return 0;
}

/* Call a user-supplied function for every block in the current tape */
int
tape_foreach( void (*function)( libspectrum_tape_block *block,
				void *user_data),
	      void *user_data )
{
  libspectrum_tape_block *block;
  libspectrum_tape_iterator iterator;

  for( block = libspectrum_tape_iterator_init( &iterator, tape );
       block;
       block = libspectrum_tape_iterator_next( &iterator ) )
    function( block, user_data );

  return 0;
}

int
tape_block_details( char *buffer, size_t length,
		    libspectrum_tape_block *block )
{
  libspectrum_byte *data;
  const char *type; char name[11];
  int offset;

  switch( libspectrum_tape_block_type( block ) ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    /* See if this looks like a standard Spectrum header and if so
       display some extra data */
    if( libspectrum_tape_block_data_length( block ) != 19 ) goto normal;

    data = libspectrum_tape_block_data( block );

    /* Flag byte is 0x00 for headers */
    if( data[0] != 0x00 ) goto normal;

    switch( data[1] ) {
    case 0x00: type = "Program"; break;
    case 0x01: type = "Number array"; break;
    case 0x02: type = "Character array"; break;
    case 0x03: type = "Bytes"; break;
    default: goto normal;
    }
    
    make_name( name, &data[2] );

    snprintf( buffer, length, "%s: \"%s\"", type, name );

    break;

  normal:
    snprintf( buffer, length, "%lu bytes",
	      (unsigned long)libspectrum_tape_block_data_length( block ) );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    snprintf( buffer, length, "%lu bytes",
	      (unsigned long)libspectrum_tape_block_data_length( block ) );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    snprintf( buffer, length, "%lu tstates",
	      (unsigned long)libspectrum_tape_block_pulse_length( block ) );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    snprintf( buffer, length, "%lu pulses",
	      (unsigned long)libspectrum_tape_block_count( block ) );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    snprintf( buffer, length, "%lu ms",
	      (unsigned long)libspectrum_tape_block_pause( block ) );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    snprintf( buffer, length, "%s", libspectrum_tape_block_text( block ) );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    offset = libspectrum_tape_block_offset( block );
    if( offset > 0 ) {
      snprintf( buffer, length, "Forward %d blocks", offset );
    } else {
      snprintf( buffer, length, "Backward %d blocks", -offset );
    }
    break;

  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    snprintf( buffer, length, "%lu iterations",
	      (unsigned long)libspectrum_tape_block_count( block ) );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    snprintf( buffer, length, "%lu options",
	      (unsigned long)libspectrum_tape_block_count( block ) );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  default:
    buffer[0] = '\0';
    break;

  }

  return 0;
}

static void
make_name( char *name, const char *data )
{
  size_t i;

  for( i = 0; i < 10; i++, name++, data++ ) {
    if( *data >= 32 && *data < 127 ) {
      *name = *data;
    } else {
      *name = '?';
    }
  }

  *name = '\0';
}

