/* zxatasp.c: ZXATASP interface routines
   Copyright (c) 2003-2004 Garry Lancaster,
		 2004 Philip Kendall

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

   E-mail: Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <compat.h>

#include <libspectrum.h>

#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "settings.h"
#include "zxatasp.h"

/*
  TBD: Allow save/load of memory
  TBD: Allow memory size selection (128K/512K)
  TBD: Should support for secondary channel be removed?
       No software ever supported it, and v2.0+ boards don't have it.
*/


/* Private function prototypes */

static libspectrum_byte zxatasp_portA_read( libspectrum_word port,
					    int *attached );
static void zxatasp_portA_write( libspectrum_word port,
				 libspectrum_byte data );
static libspectrum_byte zxatasp_portB_read( libspectrum_word port,
					    int *attached );
static void zxatasp_portB_write( libspectrum_word port,
				 libspectrum_byte data );
static libspectrum_byte zxatasp_portC_read( libspectrum_word port,
					    int *attached );
static void zxatasp_portC_write( libspectrum_word port,
				 libspectrum_byte data );
static libspectrum_byte zxatasp_control_read( libspectrum_word port,
					      int *attached );
static void zxatasp_control_write( libspectrum_word port,
				   libspectrum_byte data );
static void zxatasp_resetports( void );
static void set_zxatasp_bank( int bank );

static void zxatasp_readide( libspectrum_ide_channel *chn,
			     libspectrum_ide_register idereg );
static void zxatasp_writeide( libspectrum_ide_channel *chn,
			      libspectrum_ide_register idereg );

/* Data */

const periph_t zxatasp_peripherals[] = {
  { 0x039f, 0x009f, zxatasp_portA_read, zxatasp_portA_write },
  { 0x039f, 0x019f, zxatasp_portB_read, zxatasp_portB_write },
  { 0x039f, 0x029f, zxatasp_portC_read, zxatasp_portC_write },
  { 0x039f, 0x039f, zxatasp_control_read, zxatasp_control_write },
};

const size_t zxatasp_peripherals_count =
  sizeof( zxatasp_peripherals ) / sizeof( periph_t );

static libspectrum_byte zxatasp_control;
static libspectrum_byte zxatasp_portA;
static libspectrum_byte zxatasp_portB;
static libspectrum_byte zxatasp_portC;

static libspectrum_ide_channel *zxatasp_idechn0;
static libspectrum_ide_channel *zxatasp_idechn1;

static libspectrum_byte ZXATASPMEM[ 32 ][ 0x4000 ];

/* We're ignoring all mode bits and only emulating mode 0, basic I/O */
#define MC8255_PORT_C_LOW_IO  0x01
#define MC8255_PORT_B_IO      0x02
#define MC8255_PORT_C_HI_IO   0x08
#define MC8255_PORT_A_IO      0x10
#define MC8255_SETMODE        0x80

#define ZXATASP_IDE_REG       0x07
#define ZXATASP_RAM_BANK      0x1f
#define ZXATASP_IDE_WR        0x08
#define ZXATASP_IDE_RD        0x10
#define ZXATASP_IDE_PRIMARY   0x20
#define ZXATASP_RAM_LATCH     0x40
#define ZXATASP_RAM_DISABLE   0x80
#define ZXATASP_IDE_SECONDARY 0x80

#define ZXATASP_READ_PRIMARY( x )     ( ( x & 0x78 ) == 0x30 )
#define ZXATASP_WRITE_PRIMARY( x )    ( ( x & 0x78 ) == 0x28 )
#define ZXATASP_READ_SECONDARY( x )   ( ( x & 0xd8 ) == 0x90 )
#define ZXATASP_WRITE_SECONDARY( x )  ( ( x & 0xd8 ) == 0x88 )

/* Housekeeping functions */

int
zxatasp_init( void )
{
  int error;

  error = libspectrum_ide_alloc( &zxatasp_idechn0, LIBSPECTRUM_IDE_DATA16 );
  if( error ) return error;
  error = libspectrum_ide_alloc( &zxatasp_idechn1, LIBSPECTRUM_IDE_DATA16 );
  if( error ) return error;
  
  error = libspectrum_ide_insert( zxatasp_idechn0, LIBSPECTRUM_IDE_MASTER,
                                  settings_current.zxatasp_master_file );
  if( error ) return error;

  error = libspectrum_ide_insert( zxatasp_idechn0, LIBSPECTRUM_IDE_SLAVE,
                                  settings_current.zxatasp_slave_file );
                                  
  return error;
}

int
zxatasp_end( void )
{
  int error;
  
  error = libspectrum_ide_free( zxatasp_idechn0 );
  error = libspectrum_ide_free( zxatasp_idechn1 ) || error;

  return error;
}

void
zxatasp_reset( void )
{
  set_zxatasp_bank( 0 );
  zxatasp_control = 0x9b;
  zxatasp_resetports();

  libspectrum_ide_reset( zxatasp_idechn0 );
  libspectrum_ide_reset( zxatasp_idechn1 );
}

int
zxatasp_insert( const char *filename, libspectrum_ide_unit unit )
{
  int error;

  switch( unit ) {
  case LIBSPECTRUM_IDE_MASTER:
    error = settings_set_string( &settings_current.zxatasp_master_file,
				 filename );
    break;
    
  case LIBSPECTRUM_IDE_SLAVE:
    error = settings_set_string( &settings_current.zxatasp_slave_file,
				 filename );
    break;
    
  default:
    error = 1;
  }
  
  if( error ) return error;

  error = libspectrum_ide_insert( zxatasp_idechn0, unit, filename );

  return error;
}

int
zxatasp_commit( libspectrum_ide_unit unit )
{
  int error;

  error = libspectrum_ide_commit( zxatasp_idechn0, unit );

  return error;
}

int
zxatasp_eject( libspectrum_ide_unit unit )
{
  int error = 0;
  
  switch( unit ) {
  case LIBSPECTRUM_IDE_MASTER:
    if( settings_current.zxatasp_master_file )
      free( settings_current.zxatasp_master_file );
    settings_current.zxatasp_master_file = NULL;
    break;
    
  case LIBSPECTRUM_IDE_SLAVE:
    if( settings_current.zxatasp_slave_file )
      free( settings_current.zxatasp_slave_file );
    settings_current.zxatasp_slave_file = NULL;
    break;
    
  default:
    error = 1;
  }
  
  if( error ) return error;
  
  error = libspectrum_ide_eject( zxatasp_idechn0, unit );
  
  return error;
}

/* Port read/writes */

static libspectrum_byte
zxatasp_portA_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !settings_current.zxatasp_active ) return 0xff;
  
  *attached = 1;
  
  return zxatasp_portA;
}

static void
zxatasp_portA_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( !settings_current.zxatasp_active ) return;
  
  if( !( zxatasp_control & MC8255_PORT_A_IO ) ) zxatasp_portA = data;
}

static libspectrum_byte
zxatasp_portB_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !settings_current.zxatasp_active ) return 0xff;
  
  *attached = 1;
  
  return zxatasp_portB;
}

static void
zxatasp_portB_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( !settings_current.zxatasp_active ) return;
  
  if( !( zxatasp_control & MC8255_PORT_B_IO ) ) zxatasp_portB = data;
}

static libspectrum_byte
zxatasp_portC_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !settings_current.zxatasp_active ) return 0xff;
  
  *attached = 1;
  
  return zxatasp_portC;
}

static void
zxatasp_portC_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  libspectrum_byte oldC = zxatasp_portC;
  libspectrum_byte newC;
  
  if( !settings_current.zxatasp_active ) return;
  
  /* Determine new port C value, dependent on I/O modes */
  newC = ( zxatasp_control & MC8255_PORT_C_LOW_IO )
            ? ( oldC & 0x0f ) : ( data & 0x0f );
            
  newC |= ( zxatasp_control & MC8255_PORT_C_HI_IO )
            ? ( oldC & 0xf0 ) : ( data & 0xf0 );
            
  /* Set the new port value */
  zxatasp_portC = newC;
  
  /* No action can occur if high part of port C is in input mode */
  if( zxatasp_control & MC8255_PORT_C_HI_IO ) return;
  
  /* Check for any I/O action */
  if(  ( ZXATASP_READ_PRIMARY( newC ) ) &
      !( ZXATASP_READ_PRIMARY( oldC ) )   ) {
    zxatasp_readide( zxatasp_idechn0, ( newC & ZXATASP_IDE_REG ) );
    return;
  }
  
  if(  ( ZXATASP_READ_SECONDARY( newC ) ) &
      !( ZXATASP_READ_SECONDARY( oldC ) )   ) {
    zxatasp_readide( zxatasp_idechn1, ( newC & ZXATASP_IDE_REG ) );
    return;
  }
  
  if(  ( ZXATASP_WRITE_PRIMARY( newC ) ) &
      !( ZXATASP_WRITE_PRIMARY( oldC ) )   ) {
    zxatasp_writeide( zxatasp_idechn0, ( newC & ZXATASP_IDE_REG ) );
    return;
  }
  
  if(  ( ZXATASP_WRITE_SECONDARY( newC ) ) &
      !( ZXATASP_WRITE_SECONDARY( oldC ) )   ) {
    zxatasp_writeide( zxatasp_idechn1, ( newC & ZXATASP_IDE_REG ) );
    return;
  }

  if( newC & ZXATASP_RAM_LATCH ) {
    set_zxatasp_bank( newC & ZXATASP_RAM_BANK );
    machine_current->ram.romcs = !( newC & ZXATASP_RAM_DISABLE );

    machine_current->memory_map();
    return;
  }
}

static libspectrum_byte
zxatasp_control_read( libspectrum_word port, int *attached )
{
  if( !settings_current.zxatasp_active ) return 0xff;
  
  *attached = 1;
  
  return zxatasp_control;
}

static void
zxatasp_control_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( !settings_current.zxatasp_active ) return;
  
  if( data & MC8255_SETMODE ) {
    /* Set the control word and reset the ports */
    zxatasp_control = data;
    zxatasp_resetports();

  } else {

    /* Set or reset a bit of port C */
    libspectrum_byte bit = (data >> 1) & 7;
    libspectrum_byte newC = zxatasp_portC;
      
    if( data & 1 ) {
      newC |=  ( 1 << bit );
    } else {
      newC &= ~( 1 << bit );
    }
    
    zxatasp_portC_write( 0, newC );
  }
}

static void
zxatasp_resetports( void )
{
  /* In input mode, ports will return 0xff unless IDE is active,
     which it won't be just after a reset. Output ports are always
     reset to zero.
  */
  zxatasp_portA = (zxatasp_control & MC8255_PORT_A_IO) ? 0xff : 0x00;
  zxatasp_portB = (zxatasp_control & MC8255_PORT_B_IO) ? 0xff : 0x00;
  zxatasp_portC = (zxatasp_control & MC8255_PORT_C_LOW_IO) ? 0x0f : 0x00;
  zxatasp_portC |= (zxatasp_control & MC8255_PORT_C_HI_IO) ? 0xf0 : 0x00;
}

static void
set_zxatasp_bank( int bank )
{
  memory_page *page;
  size_t i, offset;

  for( i = 0; i < 2; i++ ) {

    page = &memory_map_romcs[i];
    offset = i & 1 ? MEMORY_PAGE_SIZE : 0x0000;

    page->page = &ZXATASPMEM[ bank ][ offset ];
    page->writable = !settings_current.zxatasp_wp;
    page->contended = 0;
    
    page->page_num = bank;
    page->offset = offset;
  }
}

/* IDE access */

static void
zxatasp_readide(libspectrum_ide_channel *chn,
                libspectrum_ide_register idereg)
{
  libspectrum_byte dataHi, dataLo;
  
  dataLo = libspectrum_ide_read( chn, idereg );
  
  if( idereg == LIBSPECTRUM_IDE_REGISTER_DATA ) {
    dataHi = libspectrum_ide_read( chn, idereg );
  } else {
    dataHi = 0xff;
  }

  if( zxatasp_control & MC8255_PORT_A_IO ) zxatasp_portA = dataLo;
  if( zxatasp_control & MC8255_PORT_B_IO ) zxatasp_portB = dataHi;
}

static void
zxatasp_writeide(libspectrum_ide_channel *chn,
                 libspectrum_ide_register idereg)
{
  libspectrum_byte dataHi, dataLo;

  dataLo = ( zxatasp_control & MC8255_PORT_A_IO ) ? 0xff : zxatasp_portA;
  dataHi = ( zxatasp_control & MC8255_PORT_B_IO ) ? 0xff : zxatasp_portB;
  
  libspectrum_ide_write( chn, idereg, dataLo );
  
  if( idereg == LIBSPECTRUM_IDE_REGISTER_DATA )
    libspectrum_ide_write( chn, idereg, dataHi );
}
