/* spec16.c: Spectrum 16K specific routines
   Copyright (c) 1999-2005 Philip Kendall

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

#include <libspectrum.h>

#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "memory.h"
#include "periph.h"
#include "printer.h"
#include "settings.h"
#include "spec48.h"
#include "ula.h"
#include "if1.h"

static int spec16_reset( void );

static libspectrum_byte empty_chunk[ MEMORY_PAGE_SIZE ];
static memory_page empty_mapping;

const static periph_t peripherals[] = {
  { 0x0001, 0x0000, ula_read, ula_write },
  { 0x0018, 0x0010, if1_port_in, if1_port_out },
  { 0x0018, 0x0008, if1_port_in, if1_port_out },
  { 0x0018, 0x0000, if1_port_in, if1_port_out },
  { 0x0004, 0x0000, printer_zxp_read, printer_zxp_write },
  { 0x00e0, 0x0000, joystick_kempston_read, NULL },
};

const static size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

static libspectrum_byte
spec16_unattached_port( void )
{
  return spectrum_unattached_port( 1 );
}

int spec16_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_16;
  machine->id = "16";

  machine->reset = spec16_reset;

  machine->timex = 0;
  machine->ram.port_contended = spec48_port_contended;
  machine->ram.contend_delay  = spec48_contend_delay;

  memset( empty_chunk, 0xff, MEMORY_PAGE_SIZE );

  empty_mapping.page = empty_chunk;
  empty_mapping.writable = 0;
  empty_mapping.contended = 0;
  empty_mapping.bank = MEMORY_BANK_NONE;
  empty_mapping.source = MEMORY_SOURCE_SYSTEM;

  machine->unattached_port = spec16_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec48_memory_map;

  return 0;

}

static int
spec16_reset( void )
{
  int error;
  size_t i;

  error = machine_load_rom( 0, 0, settings_current.rom_16, 0x4000 );
  if( error ) return error;

  error = periph_setup( peripherals, peripherals_count,
			PERIPH_PRESENT_OPTIONAL,
			PERIPH_PRESENT_OPTIONAL,
			PERIPH_PRESENT_OPTIONAL );
  if( error ) return error;

  /* ROM 0, RAM 5, nothing, nothing */
  memory_map_home[0] = &memory_map_rom[ 0];
  memory_map_home[1] = &memory_map_rom[ 1];

  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  memory_map_home[4] = memory_map_home[5] = &empty_mapping;
  memory_map_home[6] = memory_map_home[7] = &empty_mapping;

  /* The RAM page is present/writable and contended */
  for( i = 2; i <= 3; i++ ) {
    memory_map_home[ i ]->writable = 1;
    memory_map_home[ i ]->contended = 1;
  }

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  return 0;
}
