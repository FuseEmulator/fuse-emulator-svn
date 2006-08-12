/* scld.c: Routines for handling the Timex SCLD
   Copyright (c) 2002-2006 Fredrick Meunier, Philip Kendall, Witold Filipczyk

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

   Philip: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

   Fred: fredm@spamcop.net

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "dck.h"
#include "display.h"
#include "machine.h"
#include "memory.h"
#include "scld.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "z80/z80.h"

scld scld_last_dec;                 /* The last byte sent to Timex DEC port */

libspectrum_byte scld_last_hsr = 0; /* The last byte sent to Timex HSR port */

memory_page timex_exrom[8];
memory_page timex_dock[8];

libspectrum_byte
scld_dec_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return scld_last_dec.byte;
}

void
scld_dec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  scld old_dec = scld_last_dec;
  libspectrum_byte ink,paper;

  scld_last_dec.byte = b;

  /* If we just reenabled interrupts, check for a retriggered interrupt */
  if( old_dec.name.intdisable && !scld_last_dec.name.intdisable )
    z80_interrupt();

  if( scld_last_dec.name.altmembank != old_dec.name.altmembank )
    machine_current->memory_map();

  display_parse_attr( hires_get_attr(), &ink, &paper );
  display_set_hires_border( paper );

  if( scld_last_dec.name.altdfile != old_dec.name.altdfile ||
      scld_last_dec.name.hires != old_dec.name.hires )
    display_refresh_main_screen();
}

void
scld_reset(void)
{
  scld_last_dec.byte = 0;
}

void
scld_hsr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  scld_last_hsr = b;

  machine_current->memory_map();
}

libspectrum_byte
scld_hsr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return scld_last_hsr;
}

libspectrum_byte
hires_get_attr( void )
{
  return( hires_convert_dec( scld_last_dec.byte ) );
}

libspectrum_byte
hires_convert_dec( libspectrum_byte attr )
{
  scld colour;

  colour.byte = attr;

  switch ( colour.mask.hirescol )
  {
    case BLACKWHITE:   return 0x47;
    case BLUEYELLOW:   return 0x4e;
    case REDCYAN:      return 0x55;
    case MAGENTAGREEN: return 0x5c;
    case GREENMAGENTA: return 0x63;
    case CYANRED:      return 0x6a;
    case YELLOWBLUE:   return 0x71;
    default:	       return 0x78; /* WHITEBLACK */
  }
}

void
scld_memory_map( void )
{
  int i;
  memory_page **exrom_dock;
  
  exrom_dock =
    scld_last_dec.name.altmembank ? memory_map_exrom : memory_map_dock;

  for( i = 0; i < 8; i++ ) {
    if( scld_last_hsr & ( 1 << i ) ) {
      memory_map_read[i] = memory_map_write[i] = *exrom_dock[i];
    } else {
      memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];
    }
  }
}

int
scld_from_snapshot( libspectrum_snap *snap, int capabilities )
{
  size_t i;

  if( capabilities & ( LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY |
      LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) )
    scld_hsr_write( 0x00fd, libspectrum_snap_out_scld_hsr( snap ) );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO )
    scld_dec_write( 0x00ff, libspectrum_snap_out_scld_dec( snap ) );

  if( libspectrum_snap_dock_active( snap ) ) {

    dck_active = 1;

    for( i = 0; i < 8; i++ ) {

      if( libspectrum_snap_dock_cart( snap, i ) ) {
        if( !memory_map_dock[i]->page ) {
          memory_map_dock[i]->offset = 0;
          memory_map_dock[i]->page_num = 0;
          memory_map_dock[i]->writable = libspectrum_snap_dock_ram( snap, i );
          memory_map_dock[i]->source = MEMORY_SOURCE_CARTRIDGE;
          memory_map_dock[i]->page = memory_pool_allocate(
                                      MEMORY_PAGE_SIZE *
                                      sizeof( libspectrum_byte ) );
          if( !memory_map_dock[i]->page ) {
            ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                      __LINE__ );
            return 1;
          }
        }

        memcpy( memory_map_dock[i]->page, libspectrum_snap_dock_cart( snap, i ),
                MEMORY_PAGE_SIZE );
      }

      if( libspectrum_snap_exrom_cart( snap, i ) ) {
        if( !memory_map_dock[i]->page ) {
          memory_map_exrom[i]->offset = 0;
          memory_map_exrom[i]->page_num = 0;
          memory_map_exrom[i]->writable = libspectrum_snap_exrom_ram( snap, i );
          memory_map_exrom[i]->source = MEMORY_SOURCE_CARTRIDGE;
          memory_map_exrom[i]->page = memory_pool_allocate(
                                      MEMORY_PAGE_SIZE *
                                      sizeof( libspectrum_byte ) );
          if( !memory_map_exrom[i]->page ) {
            ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                      __LINE__ );
            return 1;
          }
        }

        memcpy( memory_map_exrom[i]->page,
                libspectrum_snap_exrom_cart( snap, i ), MEMORY_PAGE_SIZE );
      }

    }

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK )
      ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT, 1 );

    machine_current->memory_map();
  }

  return 0;
}

int
scld_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  libspectrum_snap_set_out_scld_hsr( snap, scld_last_hsr );
  libspectrum_snap_set_out_scld_dec( snap, scld_last_dec.byte );

  if( dck_active ) {

    libspectrum_snap_set_dock_active( snap, 1 );

    for( i = 0; i < 8; i++ ) {

      if( memory_map_exrom[i]->source == MEMORY_SOURCE_CARTRIDGE ||
	  memory_map_exrom[i]->writable ) {
        buffer = malloc( MEMORY_PAGE_SIZE * sizeof( libspectrum_byte ) );
        if( !buffer ) {
          ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                    __LINE__ );
          return 1;
        }

        libspectrum_snap_set_exrom_ram( snap, i,
                                        memory_map_exrom[i]->writable );
        memcpy( buffer, memory_map_exrom[i]->page, MEMORY_PAGE_SIZE );
        libspectrum_snap_set_exrom_cart( snap, i, buffer );
      }

      if( memory_map_dock[i]->source == MEMORY_SOURCE_CARTRIDGE ||
	  memory_map_dock[i]->writable ) {
        buffer = malloc( MEMORY_PAGE_SIZE * sizeof( libspectrum_byte ) );
        if( !buffer ) {
          ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                    __LINE__ );
          return 1;
        }

        libspectrum_snap_set_dock_ram( snap, i, memory_map_dock[i]->writable );
        memcpy( buffer, memory_map_dock[i]->page, MEMORY_PAGE_SIZE );
        libspectrum_snap_set_dock_cart( snap, i, buffer );
      }

    }

  }

  return 0;
}
