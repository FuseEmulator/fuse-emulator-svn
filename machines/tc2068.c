/* tc2068.c: Timex TC2068 specific routines
   Copyright (c) 1999-2004 Philip Kendall
   Copyright (c) 2002-2004 Fredrick Meunier
   Copyright (c) 2003 Witold Filipczyk
   Copyright (c) 2003 Darren Salt

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

#include <string.h>

#include <libspectrum.h>

#include "dck.h"
#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "periph.h"
#include "printer.h"
#include "scld.h"
#include "settings.h"
#include "ui/ui.h"

static libspectrum_byte tc2068_ay_registerport_read( libspectrum_word port,
						     int *attached );
static libspectrum_byte tc2068_ay_dataport_read( libspectrum_word port,
						 int *attached );
static libspectrum_byte tc2068_contend_delay( libspectrum_dword time );
static int dock_exrom_reset( void );

static int tc2068_reset( void );
static int tc2068_memory_map( void );

const static periph_t peripherals[] = {
  { 0x00ff, 0x00f4, scld_hsr_read, scld_hsr_write },

  /* TS2040/Alphacom printer */
  { 0x00ff, 0x00fb, printer_zxp_read, printer_zxp_write },

  /* Lower 8 bits of Timex ports are fully decoded */
  { 0x00ff, 0x00fe, spectrum_ula_read, spectrum_ula_write },

  { 0x00ff, 0x00f5, tc2068_ay_registerport_read, ay_registerport_write },
  { 0x00ff, 0x00f6, tc2068_ay_dataport_read, ay_dataport_write },

  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
};

const static size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

static libspectrum_byte fake_bank[ MEMORY_PAGE_SIZE ];
static memory_page fake_mapping;

static libspectrum_byte
tc2068_ay_registerport_read( libspectrum_word port, int *attached )
{
  if( machine_current->ay.current_register == 14 ) return 0xff;

  return ay_registerport_read( port, attached );
}

static libspectrum_byte
tc2068_ay_dataport_read( libspectrum_word port, int *attached )
{
  if (machine_current->ay.current_register != 14) {
    return ay_registerport_read( port, attached );
  } else {

    libspectrum_byte ret;

    /* In theory, we may need to distinguish cases where some data
       is returned here and were it isn't. In practice, this doesn't
       matter for the TC2068 as it doesn't have a floating bus, so we'll
       get 0xff in both cases anwyay */
    *attached = 1;

    ret =   machine_current->ay.registers[7] & 0x40
	  ? machine_current->ay.registers[14]
	  : 0xff;

    if( port & 0x0100 ) ret &= ~joystick_timex_read( port, 0 );
    if( port & 0x0200 ) ret &= ~joystick_timex_read( port, 1 );

    return ret;
  }
}

static libspectrum_byte
tc2068_unattached_port( void )
{
  /* TC2068 does not have floating ULA values on any port (despite
     rumours to the contrary), it returns 0xff on unattached ports */
  return 0xff;
}

static libspectrum_byte
tc2068_contend_delay( libspectrum_dword time )
{
  libspectrum_word tstates_through_line;
  
  /* No contention in the upper border */
  if( time < machine_current->line_times[ DISPLAY_BORDER_HEIGHT ] )
    return 0;

  /* Or the lower border */
  if( time >= machine_current->line_times[ DISPLAY_BORDER_HEIGHT + 
					   DISPLAY_HEIGHT          ] )
    return 0;

  /* Work out where we are in this line */
  tstates_through_line =
    ( time + machine_current->timings.left_border ) %
    machine_current->timings.tstates_per_line;

  /* No contention if we're in the left border */
  if( tstates_through_line < machine_current->timings.left_border - 1 ) 
    return 0;

  /* Or the right border or retrace */
  if( tstates_through_line >= machine_current->timings.left_border +
                              machine_current->timings.horizontal_screen - 1 )
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

int
tc2068_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_TC2068;
  machine->id = "2068";

  machine->reset = tc2068_reset;

  machine->timex = 1;
  machine->ram.port_contended	     = tc2048_port_contended;
  machine->ram.contend_delay	     = tc2068_contend_delay;

  memset( fake_bank, 0xff, MEMORY_PAGE_SIZE );

  fake_mapping.page = fake_bank;
  fake_mapping.writable = 0;
  fake_mapping.contended = 0;
  fake_mapping.bank = MEMORY_BANK_DOCK;
  fake_mapping.offset = 0x0000;

  machine->unattached_port = tc2068_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = tc2068_memory_map;

  return 0;
}

static int
dock_exrom_reset( void )
{
  memory_map_home[0] = &memory_map_rom[ 0];
  memory_map_home[1] = &memory_map_rom[ 1];

  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  memory_map_home[4] = &memory_map_ram[ 4];
  memory_map_home[5] = &memory_map_ram[ 5];

  memory_map_home[6] = &memory_map_ram[ 0];
  memory_map_home[7] = &memory_map_ram[ 1];

  return 0;
}

static int
tc2068_reset( void )
{
  size_t i;
  int error;

  error = dock_exrom_reset(); if( error ) return error;

  error = machine_load_rom( 0, settings_current.rom_tc2068_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, settings_current.rom_tc2068_1, 0x2000 );
  if( error ) return error;

  error = periph_setup( peripherals, peripherals_count,
                        PERIPH_PRESENT_OPTIONAL,
                        PERIPH_PRESENT_OPTIONAL );
  if( error ) return error;

  for( i = 0; i < 8; i++ ) {

    timex_dock[i] = fake_mapping;
    timex_dock[i].page_num = i;
    memory_map_dock[i] = &timex_dock[i];

    timex_exrom[i] = memory_map_rom[2];
    timex_exrom[i].page_num = i;
    memory_map_exrom[i] = &timex_exrom[i];

  }

  if( settings_current.dck_file ) {
    error = dck_read( settings_current.dck_file );
    if( error ) {
      ui_error( UI_ERROR_INFO, "Ignoring Timex dock file '%s'",
		settings_current.dck_file );
    }
  }

  memory_current_screen = 5;
  memory_screen_mask = 0xdfff;

  scld_dec_write( 0x00ff, 0x80 );
  scld_dec_write( 0x00ff, 0x00 );
  scld_hsr_write( 0x00f4, 0x00 );

  return 0;
}

static int
tc2068_memory_map( void )
{
  scld_memory_map();

  memory_romcs_map();

  return 0;
}
