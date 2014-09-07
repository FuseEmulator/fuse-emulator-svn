/* ulaplus.c: Routines for handling the ULAplus
   Copyright (c) 2009 Gilberto Almeida
   Copyright (c) 2014 Fredrick Meunier, Segio Baldoví

   $Id$

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

#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "display.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "ula.h"
#include "ulaplus.h"

int ulaplus_available = 0;
int ulaplus_palette_enabled = 0;
libspectrum_byte ulaplus_palette[ ULAPLUS_CLUT_MAX_COLOURS ];
static libspectrum_byte ulaplus_current_register = 0;

static void ulaplus_reset( int hard_reset );
static void ulaplus_enabled_snapshot( libspectrum_snap *snap );
static void ulaplus_from_snapshot( libspectrum_snap *snap );
static void ulaplus_to_snapshot( libspectrum_snap *snap );

static void ulaplus_registerport_write( libspectrum_word port,
                                        libspectrum_byte value );
static libspectrum_byte ulaplus_dataport_read( libspectrum_word port,
                                               int *attached );
static void ulaplus_dataport_write( libspectrum_word port,
                                    libspectrum_byte value );

static module_info_t ulaplus_module_info = {
  ulaplus_reset,
  NULL,
  ulaplus_enabled_snapshot,
  ulaplus_from_snapshot,
  ulaplus_to_snapshot
};

static const periph_port_t ulaplus_ports[] = {
  { 0xffff, 0xbf3b, NULL, ulaplus_registerport_write },
  { 0xffff, 0xff3b, ulaplus_dataport_read, ulaplus_dataport_write },
  { 0, 0, NULL, NULL }
};

static const periph_t ulaplus_periph = {
  &settings_current.ulaplus,
  ulaplus_ports,
  1,
  NULL
};

void
ulaplus_init( void )
{
  module_register( &ulaplus_module_info );
  periph_register( PERIPH_TYPE_ULAPLUS, &ulaplus_periph );
}

static void
ulaplus_reset( int hard_reset GCC_UNUSED )
{
  if( !periph_is_active( PERIPH_TYPE_ULAPLUS ) ) {
    ulaplus_available = 0;
    ulaplus_palette_enabled = 0;
    return;
  }

  ulaplus_available = 1;

  ulaplus_palette_enabled = 0;
  ulaplus_current_register = 0;
  memset( ulaplus_palette, 0, ULAPLUS_CLUT_MAX_COLOURS );

  if( display_write_if_dirty == display_write_if_dirty_sinclair )
    display_write_if_dirty = display_write_if_dirty_sinclair_ulaplus;

  if( display_write_if_dirty == display_write_if_dirty_timex )
    display_write_if_dirty = display_write_if_dirty_timex_ulaplus;

  if( display_dirty_flashing == display_dirty_flashing_sinclair )
    display_dirty_flashing = display_dirty_flashing_sinclair_ulaplus;
}

/* ULAplus registerport write:
   values 0 to 63 set palette group and the current palette entry,
   value 64 set mode group,
   all other values are reserved. */
void
ulaplus_registerport_write( libspectrum_word port GCC_UNUSED,
                            libspectrum_byte b )
{
  ulaplus_current_register = b;
}

/* ULAplus dataport read
   If the current register is in the range 0 to 63 (palette group),
   returns the palette value (in GGGRRRBB format);
   If the current register is 64 (mode group), returns the mode
   (0 - normal mode, 1 - 64 colour mode).
   Register range 65-255 is reserved. */
libspectrum_byte
ulaplus_dataport_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( ulaplus_current_register <= 63 ) {
    *attached = 1;
    return ulaplus_palette[ ulaplus_current_register ];
  } else if( ulaplus_current_register == 64 ) {
    *attached = 1;
    return ulaplus_palette_enabled;
  }

  return 255;
}

/* ULAplus dataport write
   If the current register is in the range 0 to 63 (palette group),
   store the palette value for that register (in GGGRRRBB format);
   If the current register is 64 (mode group), set the mode according to bit 0
   (0 - normal mode, 1 - 64 colour mode).
   Register range 65-255 is reserved. */
void
ulaplus_dataport_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  /* If we changed the palette or enabled ULAplus drawing, update the critical
     region and mark the entire display file as dirty so we redraw it on the
     next pass. Note that palette changes and ULAplus mode changes can also 
     change the border state */
  if( ulaplus_current_register <= 63 ) {

    if( ulaplus_palette[ ulaplus_current_register ] != b ) {

      if( ulaplus_palette_enabled ) {
        display_update_critical( 0, 0 );
        display_refresh_main_screen();

        ulaplus_palette[ ulaplus_current_register ] = b;

        display_set_lores_border( ula_last_byte() & 7 );
      } else {
        ulaplus_palette[ ulaplus_current_register ] = b;
      }

    }

  } else if( ulaplus_current_register == 64 ) {

    b = b & 1;

    if( ulaplus_palette_enabled != b ) {
      display_update_critical( 0, 0 );
      display_refresh_main_screen();

      ulaplus_palette_enabled = b;

      display_set_lores_border( ula_last_byte() & 7 );
    }

  }

}

static void
ulaplus_enabled_snapshot( libspectrum_snap *snap )
{
  if( libspectrum_snap_ulaplus_active( snap ) )
    settings_current.ulaplus = 1;
}

static void
ulaplus_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_ulaplus_active( snap ) ) return;

  if( periph_is_active( PERIPH_TYPE_ULAPLUS ) ) {
    ulaplus_palette_enabled = libspectrum_snap_ulaplus_palette_enabled( snap );

    ulaplus_current_register =
      libspectrum_snap_ulaplus_current_register( snap );

    memcpy( ulaplus_palette, libspectrum_snap_ulaplus_palette( snap, 0 ),
            ULAPLUS_CLUT_MAX_COLOURS );

    display_set_lores_border( ula_last_byte() & 7 );
    display_refresh_all();
  }

}

static void
ulaplus_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;

  int active = periph_is_active( PERIPH_TYPE_ULAPLUS );
  libspectrum_snap_set_ulaplus_active( snap, active );

  if( !active )
    return;

  libspectrum_snap_set_ulaplus_palette_enabled( snap, ulaplus_palette_enabled );

  libspectrum_snap_set_ulaplus_current_register( snap,
                                                 ulaplus_current_register );

  buffer =
    libspectrum_malloc( ULAPLUS_CLUT_MAX_COLOURS * sizeof( libspectrum_byte ) );

  memcpy( buffer, ulaplus_palette, ULAPLUS_CLUT_MAX_COLOURS );
  libspectrum_snap_set_ulaplus_palette( snap, 0, buffer );
}
