/* simpleide.c: Simple 8-bit IDE interface routines
   Copyright (c) 2003-2004 Garry Lancaster,
		 2004 Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <libspectrum.h>

#include "ide.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "simpleide.h"
#include "ui/ui.h"

/* Private function prototypes */

static libspectrum_byte simpleide_read( libspectrum_word port, int *attached );
static void simpleide_write( libspectrum_word port, libspectrum_byte data );

/* Data */

const periph_t simpleide_peripherals[] = {
  { 0x0010, 0x0000, simpleide_read, simpleide_write },
};

const size_t simpleide_peripherals_count =
  sizeof( simpleide_peripherals ) / sizeof( periph_t );

static libspectrum_ide_channel *simpleide_idechn;

static module_info_t simpleide_module_info = {

  simpleide_reset,
  NULL,
  NULL,
  NULL,
  NULL,

};

/* Housekeeping functions */

int
simpleide_init( void )
{
  int error;

  error = libspectrum_ide_alloc( &simpleide_idechn, LIBSPECTRUM_IDE_DATA8 );
  if( error ) return error;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT, 0 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT, 0 );

  if( settings_current.simpleide_master_file ) {
    error = libspectrum_ide_insert( simpleide_idechn, LIBSPECTRUM_IDE_MASTER,
				    settings_current.simpleide_master_file );
    if( error ) return error;
    ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT, 1 );
  }

  if( settings_current.simpleide_slave_file ) {
    error = libspectrum_ide_insert( simpleide_idechn, LIBSPECTRUM_IDE_SLAVE,
				    settings_current.simpleide_slave_file );
    if( error ) return error;
    ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT, 1 );
  }

  module_register( &simpleide_module_info );

  return 0;
}

int
simpleide_end( void )
{
  return libspectrum_ide_free( simpleide_idechn );
}

void
simpleide_reset( int hard_reset GCC_UNUSED )
{
  libspectrum_ide_reset( simpleide_idechn );
}

int
simpleide_insert( const char *filename, libspectrum_ide_unit unit )
{
  char **setting;
  ui_menu_item item;

  switch( unit ) {

  case LIBSPECTRUM_IDE_MASTER:
    setting = &settings_current.simpleide_master_file;
    item = UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT;
    break;
    
  case LIBSPECTRUM_IDE_SLAVE:
    setting = &settings_current.simpleide_slave_file;
    item = UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT;
    break;
    
    default: return 1;
  }

  return ide_insert( filename, simpleide_idechn, unit, simpleide_commit,
		     setting, item );
}

int
simpleide_commit( libspectrum_ide_unit unit )
{
  int error;

  error = libspectrum_ide_commit( simpleide_idechn, unit );

  return error;
}

int
simpleide_eject( libspectrum_ide_unit unit )
{
  char **setting;
  ui_menu_item item;

  switch( unit ) {
  case LIBSPECTRUM_IDE_MASTER:
    setting = &settings_current.simpleide_master_file;
    item = UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT;
    break;
    
  case LIBSPECTRUM_IDE_SLAVE:
    setting = &settings_current.simpleide_slave_file;
    item = UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT;
    break;
    
  default: return 1;
  }

  return ide_eject( simpleide_idechn, unit, simpleide_commit, setting, item );
}

/* Port read/writes */

static libspectrum_byte
simpleide_read( libspectrum_word port, int *attached )
{
  libspectrum_ide_register idereg;
  
  if( !settings_current.simpleide_active ) return 0xff;
  
  *attached = 1;
  
  idereg = ( ( port >> 8 ) & 0x01 ) | ( ( port >> 11 ) & 0x06 );
  
  return libspectrum_ide_read( simpleide_idechn, idereg ); 
}  

static void
simpleide_write( libspectrum_word port, libspectrum_byte data )
{
  libspectrum_ide_register idereg;
  
  if( !settings_current.simpleide_active ) return;
   
  idereg = ( ( port >> 8 ) & 0x01 ) | ( ( port >> 11 ) & 0x06 );
  
  libspectrum_ide_write( simpleide_idechn, idereg, data ); 
}
