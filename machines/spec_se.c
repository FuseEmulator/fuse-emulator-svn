/* spec_se.c: ZX Spectrum SE specific routines
   Copyright (c) 2003 Darren Salt
   Copyright (c) 2004 Fredrick Meunier, Philip Kendall
   Based on tc2048.c
   Copyright (c) 1999-2002 Philip Kendall
   Copyright (c) 2002-2003 Fredrick Meunier

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

#include "dck.h"
#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "machines.h"
#include "memory.h"
#include "printer.h"
#include "snapshot.h"
#include "sound.h"
#include "spec128.h"
#include "settings.h"
#include "spectrum.h"
#include "scld.h"
#include "tc2068.h"
#include "ui/ui.h"
#include "ula.h"
#include "if1.h"

static libspectrum_byte spec_se_contend_delay( libspectrum_dword time );
static int dock_exrom_reset( void );
static int spec_se_reset( void );
static int spec_se_memory_map( void );

const static periph_t peripherals[] = {
  { 0x00e0, 0x0000, joystick_kempston_read, NULL },
  { 0x00ff, 0x00f4, scld_hsr_read, scld_hsr_write },

  /* TS2040/Alphacom printer */
  { 0x00ff, 0x00fb, printer_zxp_read, printer_zxp_write },

  /* FIXME: The SE has an 8k SRAM attached to its AY dataport */
  { 0xffff, 0xfffd, ay_registerport_read, ay_registerport_write },
  { 0xffff, 0xbffd, NULL, ay_dataport_write },

  { 0xffff, 0x7ffd, NULL, spec128_memoryport_write },

  /* Lower 8 bits of Timex ports are fully decoded */
  { 0x00ff, 0x00fe, ula_read, ula_write },

  /* Interface 1 */
  { 0x0018, 0x0010, if1_port_in, if1_port_out },
  { 0x0018, 0x0008, if1_port_in, if1_port_out },
  { 0x0018, 0x0000, if1_port_in, if1_port_out },

  /* FIXME: The SE has an 8k SRAM attached to its AY dataport */
  { 0x00ff, 0x00f5, ay_registerport_read, ay_registerport_write },
  { 0x00ff, 0x00f6, NULL, ay_dataport_write },

  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
};

const static size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

static libspectrum_byte
spec_se_contend_delay ( libspectrum_dword time )
{
  /* Contention is as for a 128 in odd-numbered HOME banks.
   * Else it is as a TC2048 or TC2068.
   */
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
spec_se_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_SE;
  machine->id = "se";

  machine->reset = spec_se_reset;

  machine->timex = 1;
  machine->ram.port_contended = tc2048_port_contended;
  machine->ram.contend_delay = spec_se_contend_delay;

  machine->unattached_port = tc2068_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec_se_memory_map;

  return 0;
}

static int
dock_exrom_reset( void )
{
  memory_map_home[0] = &memory_map_rom[ 0];
  memory_map_home[1] = &memory_map_rom[ 1];

  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  memory_map_home[4] = &memory_map_ram[16];
  memory_map_home[5] = &memory_map_ram[17];

  memory_map_home[6] = &memory_map_ram[ 0];
  memory_map_home[7] = &memory_map_ram[ 1];

  /* The dock is always active on the SE */
  dck_active = 1;

  return 0;
}

int
spec_se_reset( void )
{
  int error;
  size_t i;

  error = dock_exrom_reset(); if( error ) return error;

  error = machine_load_rom( 0, 0, settings_current.rom_spec_se_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, 1, settings_current.rom_spec_se_1, 0x4000 );
  if( error ) return error;

  error = periph_setup( peripherals, peripherals_count,
                        PERIPH_PRESENT_OPTIONAL,
			PERIPH_PRESENT_OPTIONAL,
                        PERIPH_PRESENT_OPTIONAL );
  if( error ) return error;

  for( i = 0; i < 8; i++ ) {

    timex_dock[i] = memory_map_ram[ i + 18 ];
    timex_dock[i].bank = MEMORY_BANK_DOCK;
    timex_dock[i].page_num = i;
    timex_dock[i].contended = 0;
    timex_dock[i].source = MEMORY_SOURCE_SYSTEM;
    memory_map_dock[i] = &timex_dock[i];

    timex_exrom[i] = memory_map_ram[ i + 26 ];
    timex_exrom[i].bank = MEMORY_BANK_EXROM;
    timex_exrom[i].page_num = i;
    timex_exrom[i].contended = 0;
    timex_exrom[i].source = MEMORY_SOURCE_SYSTEM;
    memory_map_exrom[i] = &timex_exrom[i];
  }

  /* The dock and exrom aren't cleared by the reset routine, so do
     so manually (only really necessary to keep snapshot sizes down) */
  for( i = 0; i < 8; i++ ) {
    memset( memory_map_dock[i]->page,  0, MEMORY_PAGE_SIZE );
    memset( memory_map_exrom[i]->page, 0, MEMORY_PAGE_SIZE );
  }

  /* Similarly for 0x8000 to 0xbfff (RAM page 8) */
  memset( memory_map_home[4]->page, 0, MEMORY_PAGE_SIZE );
  memset( memory_map_home[5]->page, 0, MEMORY_PAGE_SIZE );

  /* RAM pages 1, 3, 5 and 7 contended */
  for( i = 0; i < 8; i++ ) 
    memory_map_ram[ 2 * i ].contended =
      memory_map_ram[ 2 * i + 1 ].contended = i & 1;

  machine_current->ram.locked=0;
  machine_current->ram.last_byte = 0;

  machine_current->ram.current_page=0;
  machine_current->ram.current_rom=0;

  memory_current_screen = 5;
  memory_screen_mask = 0xdfff;

  scld_dec_write( 0x00ff, 0x80 );
  scld_dec_write( 0x00ff, 0x00 );
  scld_hsr_write( 0x00f4, 0x00 );

  return 0;
}

static int
spec_se_memory_map( void )
{
  memory_page **exrom_dock, **bank;

  /* Spectrum SE memory paging is just a combination of the 128K
     0x7ffd and TimexDOCK/EXROM paging schemes with one exception */
  spec128_memory_map();
  scld_memory_map();

  /* Exceptions apply if an odd bank is paged in via 0x7ffd */
  if( machine_current->ram.current_page & 0x01 ) {

  /* If so, bits 2 and 3 of 0xf4 also control whether the DOCK/EXROM
     is paged in at 0xc000 and 0xe000 respectively */
    exrom_dock = 
      scld_last_dec.name.altmembank ? memory_map_exrom : memory_map_dock;

    bank = scld_last_hsr & ( 1 << 2 ) ? exrom_dock : memory_map_home;
    memory_map_read[6] = memory_map_write[6] = *bank[6];

    bank = scld_last_hsr & ( 1 << 3 ) ? exrom_dock : memory_map_home;
    memory_map_read[7] = memory_map_write[7] = *bank[7];
  }

  memory_romcs_map();

  return 0;
}
