/* tc2048.c: Timex TC2048 specific routines
   Copyright (c) 1999-2002 Philip Kendall
   Copyright (c) 2002 Fredrick Meunier

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
#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "printer.h"
#include "snapshot.h"
#include "sound.h"
#include "tc2048.h"
#include "spectrum.h"
#include "scld.h"
#include "z80/z80.h"
#include "ui/ui.h"

static DWORD tc2048_contend_delay( void );

spectrum_port_info tc2048_peripherals[] = {
  { 0x00e0, 0x0000, joystick_kempston_read, spectrum_port_nowrite },
  { 0x00ff, 0x00f4, scld_hsr_read, scld_hsr_write },
  { 0x00ff, 0x00fb, printer_zxp_read, printer_zxp_write }, /* TS2040/Alphacom printer */

  /* Lower 8 bits of Timex ports are fully decoded */
  { 0x00ff, 0x00fe, spectrum_ula_read, spectrum_ula_write },

  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};


static BYTE tc2048_unattached_port( void )
{
  /* TC2048 does not have floating ULA values on any port (despite rumours to */
  /* the contrary), it returns 255 on unattached ports */
  return 255;
}

BYTE tc2048_readbyte(WORD address)
{
  WORD bank;

  if(address<0x4000) return ROM[0][address];
  bank=address/0x4000; address-=(bank*0x4000);
  switch(bank) {
    case 1: return RAM[5][address]; break;
    case 2: return RAM[2][address]; break;
    case 3: return RAM[0][address]; break;
    default:
      ui_error( UI_ERROR_ERROR, "access to impossible bank %d at %s:%d\n",
		bank, __FILE__, __LINE__ );
      fuse_abort();
  }
  return 0; /* Keep gcc happy */
}

BYTE tc2048_read_screen_memory(WORD offset)
{
  return RAM[5][offset];
}

void tc2048_writebyte(WORD address, BYTE b)
{
  if(address>=0x4000) {		/* 0x4000 = 1st byte of RAM */
    WORD bank=address/0x4000,offset=address-(bank*0x4000);
    switch(bank) {
    case 1: RAM[5][offset]=b; break;
    case 2: RAM[2][offset]=b; break;
    case 3: RAM[0][offset]=b; break;
    default:
      ui_error( UI_ERROR_ERROR, "access to impossible bank %d at %s:%d\n",
		bank, __FILE__, __LINE__ );
      fuse_abort();
    }
    display_dirty( address );	/* Replot necessary pixels */
  }
}

DWORD tc2048_contend_memory( WORD address )
{
  /* Contention occurs only in the lowest 16Kb of RAM */
  if( address < 0x4000 || address > 0x7fff ) return 0;

  return tc2048_contend_delay();
}

DWORD tc2048_contend_port( WORD port )
{
  /* Contention occurs for ports FE and F4 (SCLD and HSR) */
  /* Contention occurs for port FF (SCLD DCE) */
  if( ( port & 0xff ) == 0xf4 ||
      ( port & 0xff ) == 0xfe ||
      ( port & 0xff ) == 0xff    ) return tc2048_contend_delay();

  return 0;
}

static DWORD tc2048_contend_delay( void )
{
  WORD tstates_through_line;
  
  /* No contention in the upper border */
  if( tstates < machine_current->line_times[ DISPLAY_BORDER_HEIGHT ] )
    return 0;

  /* Or the lower border */
  if( tstates >= machine_current->line_times[ DISPLAY_BORDER_HEIGHT + 
                                              DISPLAY_HEIGHT          ] )
    return 0;

  /* Work out where we are in this line */
  tstates_through_line =
    ( tstates + machine_current->timings.left_border_cycles ) %
    machine_current->timings.cycles_per_line;

  /* No contention if we're in the left border */
  if( tstates_through_line < machine_current->timings.left_border_cycles - 1 ) 
    return 0;

  /* Or the right border or retrace */
  if( tstates_through_line >= machine_current->timings.left_border_cycles +
                              machine_current->timings.screen_cycles - 1 )
    return 0;

  /* We now know the ULA is reading the screen, so put in the appropriate
     delay */
  switch( tstates_through_line % 8 ) {
    case 7: return 6; break;
    case 0: return 5; break;
    case 1: return 4; break;
    case 2: return 3; break;
    case 3: return 2; break;
    case 4: return 1; break;
    case 5: return 0; break;
    case 6: return 0; break;
  }

  return 0;	/* Shut gcc up */
}

int tc2048_init( fuse_machine_info *machine )
{
  int error;

  machine->machine = SPECTRUM_MACHINE_2048;
  machine->description = "Timex TC2048";
  machine->id = "2048";

  machine->reset = tc2048_reset;

  machine_set_timings( machine, 3.5e6, 24, 128, 24, 48, 312, 8936 );

  machine->timex = 1;
  machine->ram.read_memory    = tc2048_readbyte;
  machine->ram.read_screen    = tc2048_read_screen_memory;
  machine->ram.write_memory   = tc2048_writebyte;
  machine->ram.contend_memory = tc2048_contend_memory;
  machine->ram.contend_port   = tc2048_contend_port;

  error = machine_allocate_roms( machine, 1 );
  if( error ) return error;
  error = machine_read_rom( machine, 0, "tc2048.rom" );
  if( error ) return error;

  machine->peripherals = tc2048_peripherals;
  machine->unattached_port = tc2048_unattached_port;

  machine->ay.present = 0;

  machine->shutdown = NULL;

  return 0;

}

int tc2048_reset(void)
{
  z80_reset();
  sound_ay_reset();	/* should happen for *all* resets */
  snapshot_flush_slt();
  printer_zxp_reset();

  return 0;
}
