/* spec48_ntsc.c: NTSC Spectrum 48K specific routines
   Copyright (c) 1999-2011 Philip Kendall

   $Id: spec48.c 3566 2008-03-18 12:59:16Z pak21 $

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

#include <stdio.h>

#include <libspectrum.h>

#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "printer.h"
#include "settings.h"
#include "spec48.h"
#include "spectrum.h"
#include "ula.h"
#include "if1.h"

static int spec48_ntsc_reset( void );

static const periph_t peripherals[] = {
  { 0x0004, 0x0000, printer_zxp_read, printer_zxp_write },
};

static const size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

int spec48_ntsc_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_48_NTSC;
  machine->id = "48_ntsc";

  machine->reset = spec48_ntsc_reset;

  machine->timex = 0;
  machine->ram.port_from_ula         = spec48_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;

  machine->unattached_port = spectrum_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec48_memory_map;

  return 0;

}

static int
spec48_ntsc_reset( void )
{
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_48,
                            settings_default.rom_48, 0x4000 );
  if( error ) return error;

  error = periph_setup( peripherals, peripherals_count );
  if( error ) return error;

  spec48_common_peripherals();
  periph_update();

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  spec48_common_display_setup();

  return spec48_common_reset();
}
