/* rzx.c: .rzx files
   Copyright (c) 2002-2003 Philip Kendall

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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#endif				/* #ifdef WIN32 */

#include "event.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "rzx.h"
#include "settings.h"
#include "snapshot.h"
#include "timer.h"
#include "ui/ui.h"
#include "utils.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The offset used to get the count of instructions from the R register;
   (instruction count) = R + rzx_instructions_offset */
int rzx_instructions_offset;

/* The number of bytes read via IN during the current frame */
size_t rzx_in_count;

/* And the values of those bytes */
libspectrum_byte *rzx_in_bytes;

/* How big is the above array? */
size_t rzx_in_allocated;

/* Are we currently recording a .rzx file? */
int rzx_recording;

/* Is the .rzx file being recorded in competition mode? */
int rzx_competition_mode;

/* The filename we'll save this recording into */
static char *rzx_filename;

/* The snapshot taken when we started the recording */
static libspectrum_snap *rzx_snap;

/* Are we currently playing back a .rzx file? */
int rzx_playback;

/* The number of instructions in the current .rzx playback frame */
size_t rzx_instruction_count;

/* The current RZX data */
libspectrum_rzx *rzx;

/* Fuse's DSA key */
libspectrum_rzx_dsa_key rzx_key = {
  "A9E3BD74E136A9ABD41E614383BB1B01EB24B2CD7B920ED6A62F786A879AC8B00F2FF318BF96F81654214B1A064889FF6D8078858ED00CF61D2047B2AAB7888949F35D166A2BBAAE23A331BD4728A736E76901D74B195B68C4A2BBFB9F005E3655BDE8256C279A626E00C7087A2D575F78D7DC5CA6E392A535FFE47A816BA503", /* p */
  "FE8D540EED2CAE1983690E2886259F8956FB5A19",	       /* q */
  "9680ABFFB98EF2021945ADDF86C21D6EE3F7C8777FB0A0220AB59E9DFA3A3338611B32CFD1F22F8F26547858754ED93BFBDD87DC13C09F42B42A36B2024467D98EB754DEB2847FCA7FC60C81A99CF95133847EA38AD9D037AFE9DD189E9F0EE47624848CEE840D7E3724A39681E71B97ECF777383DC52A48C0A2C93BADA93F4C", /* g */
  "46605F0514D56BC0B4207A350367A5038DBDD4DD62B7C997D26D0ADC5BE42D01F852C199E34553BCBCE5955FF80E3B402B55316606D7E39C0F500AE5EE41A7B7A4DCE78EC19072C21FCC7BA48DFDC830C17B72BCAA2B2D70D9DFC0AAD9B7E73F7AEB6241E54D55C33E41AB749CAAFBE7AB00F2D74C500E5F5DD63BD299C65778", /* y */
  "948744AA7A1D1BE9EE65150B0A95A678B4181F0E"	       /* x */
};

/* The time we started recording this RZX file */
static timer_type start_time;

/* How many 1/50ths of a second do we expect to have elapsed since we
   started recording? */
static int expected_time;

/* By how much is the speed allowed to deviate from 100% whilst recording
   a competition mode RZX file */
static const float SPEED_TOLERANCE = 0.05;

static int start_playback( libspectrum_rzx *rzx, libspectrum_snap *snap );
static int recording_frame( void );
static int playback_frame( void );
static int counter_reset( void );

int rzx_init( void )
{
  rzx_recording = rzx_playback = 0;

  rzx_in_bytes = NULL;
  rzx_in_allocated = 0;
  rzx_snap = NULL;

  return 0;
}

int rzx_start_recording( const char *filename, int embed_snapshot )
{
  int error; libspectrum_error libspec_error;

  if( rzx_playback ) return 1;

  error = libspectrum_rzx_alloc( &rzx ); if( error ) return error;

  /* Store the filename */
  rzx_filename = strdup( filename );
  if( rzx_filename == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory in rzx_start_recording" );
    return 1;
  }

  /* If we're embedding a snapshot, create it now */
  if( embed_snapshot ) {
    
    libspec_error = libspectrum_snap_alloc( &rzx_snap );
    if( libspec_error != LIBSPECTRUM_ERROR_NONE ) return 1;

    error = snapshot_copy_to( rzx_snap );
    if( error ) {
      libspectrum_snap_free( rzx_snap ); rzx_snap = NULL;
      return 1;
    }
  } else {
    rzx_snap = NULL;
  }

  /* Start the count of instruction fetches here */
  counter_reset(); rzx_in_count = 0;

  libspectrum_rzx_set_tstates( rzx, tstates );

  /* Note that we're recording */
  rzx_recording = 1;

  ui_menu_activate( UI_MENU_ITEM_RECORDING, 1 );

  if( settings_current.competition_mode ) {

    if( !libspectrum_gcrypt_version() )
      ui_error( UI_ERROR_INFO,
		"gcrypt not available: recording will NOT be signed" );

    expected_time = 0;
    settings_current.emulation_speed = 100;
    timer_count = 0.0;
    rzx_competition_mode = 1;
    
  } else {

    rzx_competition_mode = 0;

  }
    
  return 0;
}

int rzx_stop_recording( void )
{
  libspectrum_byte *buffer; size_t length;
  libspectrum_error libspec_error; int error;

  if( !rzx_recording ) return 0;

  /* Stop recording data */
  rzx_recording = 0;

  ui_menu_activate( UI_MENU_ITEM_RECORDING, 0 );

  libspectrum_creator_set_competition_code(
    fuse_creator, settings_current.competition_code
  );

  length = 0;
  libspec_error =
    libspectrum_rzx_write( &buffer, &length, rzx, rzx_snap, fuse_creator,
			   settings_current.rzx_compression,
			   rzx_competition_mode ? &rzx_key : NULL
			 );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_rzx_free( rzx );
    if( rzx_snap ) libspectrum_snap_free( rzx_snap );
    return libspec_error;
  }

  if( rzx_snap ) libspectrum_snap_free( rzx_snap );

  error = utils_write_file( rzx_filename, buffer, length );
  free( rzx_filename );
  if( error ) { free( buffer ); libspectrum_rzx_free( rzx ); return error; }

  free( buffer );

  libspec_error = libspectrum_rzx_free( rzx );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) return libspec_error;

  return 0;
}

int rzx_start_playback( const char *filename, int (*load_snap)(void) )
{
  utils_file file;
  libspectrum_error libspec_error; int error;
  libspectrum_snap *snap;

  if( rzx_recording ) return 1;

  error = libspectrum_rzx_alloc( &rzx ); if( error ) return error;

  error = utils_read_file( filename, &file );
  if( error ) return error;

  libspec_error = libspectrum_rzx_read( rzx, &snap, file.buffer, file.length,
					NULL );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
    utils_close_file( &file );
    return libspec_error;
  }

  if( utils_close_file( &file ) ) {
    libspectrum_rzx_free( rzx );
    if( snap ) libspectrum_snap_free( snap );
    return 1;
  }

  if( !snap && load_snap ) {
    error = load_snap();
    if( error ) { libspectrum_rzx_free( rzx ); return 1; }
  }

  error = start_playback( rzx, snap );
  if( error ) {
    if( snap ) libspectrum_snap_free( snap );
    libspectrum_rzx_free( rzx );
    return error;
  }

  if( snap ) {
    error = libspectrum_snap_free( snap );
    if( error ) { libspectrum_rzx_free( rzx ); return error; }
  }

  return 0;
}

int
rzx_start_playback_from_buffer( const unsigned char *buffer, size_t length )
{
  libspectrum_snap *snap;
  int error;

  if( rzx_recording ) return 0;

  error = libspectrum_rzx_alloc( &rzx ); if( error ) return error;

  error = libspectrum_rzx_read( rzx, &snap, buffer, length, NULL );
  if( error ) return error;

  error = start_playback( rzx, snap );
  if( error ) {
    if( snap ) libspectrum_snap_free( snap );
    libspectrum_rzx_free( rzx );
    return error;
  }

  if( snap ) {
    error = libspectrum_snap_free( snap );
    if( error ) { libspectrum_rzx_free( rzx ); return error; }
  }

  return 0;
}

static int
start_playback( libspectrum_rzx *rzx, libspectrum_snap *snap )
{
  int error;

  if( snap ) {
    error = snapshot_copy_from( snap ); if( error ) return error;
  }
  
  /* End of frame will now be generated by the RZX code */
  error = event_remove_type( EVENT_TYPE_FRAME );
  if( error ) return error;

  /* We're now playing this RZX file */
  if( libspectrum_rzx_start_playback( rzx ) ) return 1;

  tstates = libspectrum_rzx_tstates( rzx );
  rzx_instruction_count = libspectrum_rzx_instructions( rzx );
  rzx_playback = 1;
  counter_reset();

  ui_menu_activate( UI_MENU_ITEM_RECORDING, 1 );

  return 0;
}

int rzx_stop_playback( int add_interrupt )
{
  libspectrum_error libspec_error; int error;

  rzx_playback = 0;

  ui_menu_activate( UI_MENU_ITEM_RECORDING, 0 );

  /* We've now finished with the RZX file, so add an end of frame
     event if we've been requested to do so; we don't if we just run
     out of frames, as this occurs just before a normal end of frame
     and everything works normally as rzx_playback is now zero again */
  if( add_interrupt ) {
    error = event_add( machine_current->timings.tstates_per_frame,
		       EVENT_TYPE_FRAME );
    if( error ) return error;
  }

  libspec_error = libspectrum_rzx_free( rzx );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) return libspec_error;

  return 0;
}  

int rzx_frame( void )
{
  if( rzx_recording ) return recording_frame();
  if( rzx_playback  ) return playback_frame();
  return 0;
}

static int recording_frame( void )
{
  libspectrum_error error;

  error = libspectrum_rzx_store_frame( rzx, R + rzx_instructions_offset,
				       rzx_in_count, rzx_in_bytes );
  if( error ) return error;

  /* Reset the instruction counter */
  rzx_in_count = 0; counter_reset();

  /* If we're in competition mode, check we're running at close to 100%
     speed */
  if( rzx_competition_mode ) {

    expected_time++;

    /* We wait one second before starting time measurements at all.
       This is to allow any initial speed fluctuations caused by the
       menu system to settle down (notably, with a widget UI and OSS
       sound, the first few frames run fast as the sound buffers fill
       up) */
    if( expected_time == 50 ) {

      error = timer_get_real_time( &start_time );
      if( error ) {
	ui_error( UI_ERROR_ERROR, "stopping competition mode recording" );
	rzx_stop_recording();
	return error;
      }
      
    /* Measuring small time intervals will be inaccurate, so don't
       start checking for speed violations until at least one second
       of measured time has elapsed (ie two seconds into recording) */
    } else if( expected_time > 100 ) {

      timer_type current_time;
      float elapsed_seconds, expected_seconds;

      error = timer_get_real_time( &current_time );
      if( error ) {
	ui_error( UI_ERROR_ERROR, "stopping competition mode recording" );
	rzx_stop_recording();
	return error;
      }

      /* -1 to account for the fact we don't time the first second */
      expected_seconds = ( expected_time * 0.02 ) - 1;

      elapsed_seconds =
	timer_get_time_difference( &current_time, &start_time );

      if( fabs( expected_seconds / elapsed_seconds - 1 ) > SPEED_TOLERANCE ) {

	rzx_stop_recording();

	ui_error(
	  UI_ERROR_INFO,
	  "emulator speed is %d%%: stopping competition mode RZX recording",
	  (int)( 100 * ( expected_seconds / elapsed_seconds ) )
	);

      }
    }

  }

  return 0;
}

static int playback_frame( void )
{
  int error, finished;

  error = libspectrum_rzx_playback_frame( rzx, &finished );
  if( error ) return rzx_stop_playback( 0 );

  if( finished ) {
    ui_error( UI_ERROR_INFO, "Finished RZX playback" );
    return rzx_stop_playback( 0 );
  }

  /* If we've got another frame to do, fetch the new instruction count and
     continue */
  rzx_instruction_count = libspectrum_rzx_instructions( rzx );
  counter_reset();

  return 0;
}

/* Reset the RZX counter; also, take this opportunity to normalise the
   R register */
static int counter_reset( void )
{
  R &= 0x7f;		/* Clear all but the 7 lowest bits of the R register */
  rzx_instructions_offset = -R; /* Gives us a zero count */

  return 0;
}

int rzx_store_byte( libspectrum_byte value )
{
  /* Get more space if we need it; allocate twice as much as we currently
     have, with a minimum of 50 */
  if( rzx_in_count == rzx_in_allocated ) {

    libspectrum_byte *ptr; size_t new_allocated;

    new_allocated = rzx_in_allocated >= 25 ? 2 * rzx_in_allocated : 50;
    ptr =
      (libspectrum_byte*)realloc(
        rzx_in_bytes, new_allocated * sizeof(libspectrum_byte)
      );
    if( ptr == NULL ) {
      ui_error( UI_ERROR_ERROR, "Out of memory in rzx_store_byte\n" );
      return 1;
    }

    rzx_in_bytes = ptr;
    rzx_in_allocated = new_allocated;
  }

  rzx_in_bytes[ rzx_in_count++ ] = value;

  return 0;
}

int rzx_end( void )
{
  if( rzx_recording ) return rzx_stop_recording();
  if( rzx_playback  ) return rzx_stop_playback( 0 );

  return 0;
}
