/* spectrum.c: Generic Spectrum routines
   Copyright (c) 1999-2002 Philip Kendall, Darren Salt

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

#include "display.h"
#include "event.h"
#include "fuse.h"
#include "keyboard.h"
#include "printer.h"
#include "rzx.h"
#include "settings.h"
#include "sound.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus2.h"
#include "specplus3.h"
#include "spectrum.h"
#include "tape.h"
#include "timer.h"
#include "ui/ui.h"
#include "z80/z80.h"

BYTE **ROM;
BYTE RAM[8][0x4000];
DWORD tstates;

/* Level data from .slt files */

BYTE *slt[256];
size_t slt_length[256];

int spectrum_interrupt(void)
{
  /* Reduce the t-state count of both the processor and all the events
     scheduled to occur. Done slightly differently if RZX playback is
     occuring
  */
  if( rzx_playback ) {
    if( event_interrupt( tstates ) ) return 1;
    tstates = 0;
  } else {
    if(event_interrupt( machine_current->timings.cycles_per_frame )) return 1;
    tstates -= machine_current->timings.cycles_per_frame;
  }

  if(sound_enabled) {
    sound_frame();
  } else {
    timer_sleep();
    timer_count--;
  }

  if(display_frame()) return 1;
  printer_frame();
  z80_interrupt();

  /* Add an interrupt unless they're being generated by .rzx playback */
  if( ! rzx_playback ) {
    if(event_add(machine_current->timings.cycles_per_frame,
		 EVENT_TYPE_INTERRUPT) ) return 1;
  }

  return 0;
}

BYTE readport(WORD port)
{
  spectrum_port_info *ptr;

  BYTE return_value = 0xff;
  int attached = 0;		/* Is this port attached to anything? */

  /* If we're doing RZX playback, get a byte from the RZX file */
  if( rzx_playback ) {

    /* Check we're not trying to read off the end of the array */
    if( rzx_in_count >= rzx.frames[ rzx_current_frame ].count ) {
      ui_error( UI_ERROR_ERROR,
		"More INs during frame than stored in RZX file" );
      rzx_end();
      /* And get the byte normally */
      return readport( port );
    }

    /* Otherwise, just return the next RZX byte */
    return rzx.frames[ rzx_current_frame ].in_bytes[ rzx_in_count++ ];
  }

  /* If we're not doing RZX playback, get the byte normally */
  for( ptr = machine_current->peripherals; ptr->mask; ptr++ ) {
    if( ( port & ptr->mask ) == ptr->data ) {
      return_value &= ptr->read(port); attached = 1;
    }
  }

  if( !attached ) return_value = machine_current->unattached_port();

  /* If we're RZX recording, store this byte */
  if( rzx_recording ) rzx_store_byte( return_value );

  return return_value;

}

void writeport(WORD port, BYTE b)
{
  
  spectrum_port_info *ptr;

  for( ptr = machine_current->peripherals; ptr->mask; ptr++ ) {
    if( ( port & ptr->mask ) == ptr->data ) {
      ptr->write(port, b);
    }
  }

}

/* A dummy function for non-readable ports */
BYTE spectrum_port_noread(WORD port)
{
  return 0xff;
}

/* What do we get if we read from the ULA? */
BYTE spectrum_ula_read(WORD port)
{
  return ( keyboard_read( port >> 8 ) ^ ( tape_microphone ? 0x40 : 0x00 ) );
}

/* What happens when we write to the ULA? */
void spectrum_ula_write(WORD port, BYTE b)
{
  display_set_border( b & 0x07 );
  sound_beeper( 0, b & 0x10 );

  if( settings_current.issue2 ) {
    if( b & 0x18 ) {
      keyboard_default_value=0xff;
    } else {
      keyboard_default_value=0xbf;
    }
  } else {
    if( b & 0x10 ) {
      keyboard_default_value=0xff;
    } else {
      keyboard_default_value=0xbf;
    }
  }
}

/* What happens if we read from an unattached port? */
BYTE spectrum_unattached_port( int offset )
{
  int line, tstates_through_line, column;

  /* Return 0xff (idle bus) if we're in the top border */
  if( tstates < machine_current->line_times[ DISPLAY_BORDER_HEIGHT ] )
    return 0xff;

  /* Work out which line we're on, relative to the top of the screen */
  line = ( (SDWORD)tstates -
	   (SDWORD)machine_current->line_times[ DISPLAY_BORDER_HEIGHT ] ) /
    machine_current->timings.cycles_per_line;

  /* Idle bus if we're in the lower or upper borders */
  if( line >= DISPLAY_HEIGHT ) return 0xff;

  /* Work out where we are in this line */
  tstates_through_line = tstates -
    machine_current->line_times[ DISPLAY_BORDER_HEIGHT + line ];

  /* Idle bus if we're in the left border */
  if( tstates_through_line <
      machine_current->timings.left_border_cycles - offset )
    return 0xff;

  /* Or the right border or retrace */
  if( tstates_through_line >= machine_current->timings.left_border_cycles +
                              machine_current->timings.screen_cycles - offset )
    return 0xff;

  column = ( ( tstates_through_line + 1 -
	       machine_current->timings.left_border_cycles ) / 8) * 2;

  switch( tstates_through_line % 8 ) {

    /* FIXME: 25% of these should be screen data, 25% attribute bytes
       and 50% idle bus, but the actual distribution is unknown. Also,
       in each 8 T-state block, 16 pixels are displayed; when each of
       these is read is also unknown. Thanks to Ian Greenway for this
       information */

    /* FIXME: Arkanoid doesn't work properly with the below */

    /* Attribute bytes */
    case 1: column++;
    case 0:
      return read_screen_memory( display_attr_start[line] + column );

    /* Screen data */
    case 3: column++;
    case 2:
      return read_screen_memory( display_line_start[line] + column );

    /* Idle bus */
    case 4: case 5: case 6: case 7:
      return 0xff;

  }

  return 0;		/* Keep gcc happy */
}
