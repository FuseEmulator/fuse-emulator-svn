/* disciple.c: Routines for handling the +D/DISCiPLE interface
   Copyright (c) 2002-2005 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Dmitry Sanarin

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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>            /* Needed for strncasecmp() on QNX6 */
#endif                          /* #ifdef HAVE_STRINGS_H */
#include <limits.h>
#include <sys/stat.h>

#include <libspectrum.h>

#include "compat.h"
#include "disciple.h"
#include "disk.h"
#include "event.h"
#include "machine.h"
#include "settings.h"
#include "ui/ui.h"
#include "utils.h"
#include "wd1770.h"
#include "z80/z80.h"

#define DEBUG    0
#define MEMDEBUG 0

#if DEBUG == 1
#define DPRINT printf
#else
#define DPRINTF(str, ...) {}
#endif

#if MEMDEBUG == 1
#define MDPRINT printf
#else
#define MDPRINTF(str, ...) {}
#endif

#define SWAPPAGING 0
#define SWAPROMBANK 0
#define SWAPMEMSWAP 0
#define INITROMBANK 0
#define INITMEMSWAP 0

int disciple_available = 0;
int disciple_isplusd = 0;
int disciple_active = 0;
int disciple_index_pulse = 0;

static int disciple_memswap = 0;        /* Are the ROM and RAM pages swapped? */
static int disciple_rombank = 0;        /* ROM bank that is paged in */

wd1770_drive *disciple_current;
wd1770_drive disciple_drives[ DISCIPLE_NUM_DRIVES ];

void
disciple_page( void )
{
  MDPRINTF( "disciple_page()\n" );
  disciple_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
}

void
disciple_unpage( void )
{
  MDPRINTF( "disciple_unpage()\n" );
  disciple_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
}

void
disciple_memory_map( void )
{
  MDPRINTF( "disciple_memory_map()\n" );
  if( !disciple_memswap ) {
    memory_map_read[ 0 ] = memory_map_write[ 0 ] =
      memory_map_romcs[ disciple_rombank != SWAPROMBANK ];
    memory_map_read[ 1 ] = memory_map_write[ 1 ] = memory_map_ram[ 16 * 2 ];
  } else {
    memory_map_read[ 0 ] = memory_map_write[ 0 ] = memory_map_ram[ 16 * 2 ];
    memory_map_read[ 1 ] = memory_map_write[ 1 ] =
      memory_map_romcs[ disciple_rombank != SWAPROMBANK ];
  }
}

void
disciple_set_cmdint( wd1770_drive * d )
{
  z80_interrupt();
}

const periph_t disciple_peripherals[] = {
  /* ---- ---- 0001 1011 */
  { 0x00ff, 0x001b, disciple_sr_read, disciple_cr_write },
  /* ---- ---- 0101 1011  */
  { 0x00ff, 0x005b, disciple_tr_read, disciple_tr_write },
  /* ---- ---- 1001 1011 */
  { 0x00ff, 0x009b, disciple_sec_read, disciple_sec_write },
  /* ---- ---- 1110 1011 */
  { 0x00ff, 0x00db, disciple_dr_read, disciple_dr_write },

  /* ---- ---- 0001 1111 */
  { 0x00ff, 0x001f, disciple_joy_read, disciple_cn_write },
  /* ---- ---- 0011 1011 */
  { 0x00ff, 0x003b, disciple_net_read, NULL },
  /* ---- ---- 0111 1011 */
  { 0x00ff, 0x007b, disciple_boot_read, disciple_boot_write },
  /* ---- ---- 1011 1011 */
  { 0x00ff, 0x00bb, disciple_mem_read, disciple_mem_write },
  /* 0000 0000 1111 1011 */
  { 0xffff, 0x00fb, NULL, disciple_print_write },
  /* 0000 0000 1111 1110 ?? */
  { 0xffff, 0x00fe, disciple_joy2_read, NULL },
};

const size_t disciple_peripherals_count =
  sizeof( disciple_peripherals ) / sizeof( periph_t );

const periph_t plusd_peripherals[] = {
  /* ---- ---- 1110 0011 */
  { 0x00ff, 0x00e3, disciple_sr_read, disciple_cr_write },
  /* ---- ---- 1110 1011 */
  { 0x00ff, 0x00eb, disciple_tr_read, disciple_tr_write },
  /* ---- ---- 1111 0011 */
  { 0x00ff, 0x00f3, disciple_sec_read, disciple_sec_write },
  /* ---- ---- 1111 1011 */
  { 0x00ff, 0x00fb, disciple_dr_read, disciple_dr_write },

  /* ---- ---- 1110 1111 */
  { 0x00ff, 0x00ef, disciple_joy_read, disciple_cn_write },
  /* ---- ---- 1110 0111 */
  { 0x00ff, 0x00e7, disciple_mem_read, disciple_mem_write },
  /* 0000 0000 1111 0111 */
  { 0x00ff, 0x00f7, disciple_print_read, disciple_print_write },
};

const size_t plusd_peripherals_count =
  sizeof( plusd_peripherals ) / sizeof( periph_t );

int
disciple_init( void )
{
  int i;
  wd1770_drive *d;

  disciple_current = &disciple_drives[ 0 ];
  for( i = 0; i < DISCIPLE_NUM_DRIVES; i++ ) {
    d = &disciple_drives[ i ];
    d->disk = malloc( sizeof( *d->disk ) );
    if( !d->disk )
      return 1;

    d->disk->fd = -1;
    d->disk->alternatesides = 0;
    d->disk->numlayers = 2;
    d->disk->numtracks = 80;
    d->disk->numsectors = 10;
    d->disk->sectorsize = 512;

    d->disk->buffer = calloc( d->disk->numlayers * d->disk->numtracks *
      d->disk->numsectors * d->disk->sectorsize, sizeof( libspectrum_byte ) );
    d->disk->dirty = calloc( d->disk->numlayers * d->disk->numtracks *
      d->disk->numsectors, sizeof( libspectrum_byte ) );
    d->disk->present = calloc( d->disk->numlayers * d->disk->numtracks *
      d->disk->numsectors, sizeof( libspectrum_byte ) );
    if( !( d->disk->buffer && d->disk->dirty && d->disk->present ) ) {
      if( d->disk->buffer ) free( d->disk->buffer );
      if( d->disk->dirty ) free( d->disk->dirty );
      if( d->disk->present ) free( d->disk->present );
      d->disk->buffer = NULL;
      d->disk->dirty = NULL;
      d->disk->present = NULL;
      free( d->disk );
      d->disk = NULL;
      return 1;
    }

    d->rates[ 0 ] = 6;
    d->rates[ 1 ] = 12;
    d->rates[ 2 ] = 2;
    d->rates[ 3 ] = 3;

    d->set_cmdint = &disciple_set_cmdint;
    d->reset_cmdint = NULL;
    d->set_datarq = NULL;
    d->reset_datarq = NULL;
    d->iface = NULL;
  }
  return 0;
}

void
disciple_reset_drive( wd1770_drive * d ) {
  d->index_pulse = 0;
  d->index_interrupt = 0;
  d->spin_cycles = 0;
  d->track = 0;
  d->side = 0;
  d->direction = 0;

  d->state = wd1770_state_none;
  d->status_type = wd1770_status_type1;

  d->status_register = 0;
  if( d->track == 0 )
    d->status_register |= WD1770_SR_LOST;
  d->track_register = 0;
  d->sector_register = 0;
  d->data_register = 0;
}

int
disciple_reset( void )
{
  int i;
  wd1770_drive *d;
  int error;

  disciple_available = 0;

  if( !periph_disciple_active )
    return 0;

  if( !disciple_isplusd )
    error = machine_load_rom_bank( memory_map_romcs, 0, 0,
                                   settings_current.rom_disciple, 0x4000 );
  else
    error = machine_load_rom_bank( memory_map_romcs, 0, 0,
                                   settings_current.rom_plusd, 0x2000 );
  if( error ) return error;

  memory_map_romcs[0].source = MEMORY_SOURCE_PERIPHERAL;

  machine_current->ram.romcs = 0;

  memory_map_romcs[ 0 ].writable = 0;
  memory_map_romcs[ 1 ].writable = 0;
  memory_map_ram[ 16 * 2 ].writable = 1;

  disciple_available = 1;
  disciple_active = 0;
  disciple_index_pulse = 0;
  disciple_memswap = INITMEMSWAP;
  disciple_rombank = INITROMBANK;

  /* Hard reset: */
  for( i = 0; i < 8192; i++ )
    memory_map_ram[ 16 * 2 ].page[ i ] = 0;

  for( i = 0; i < DISCIPLE_NUM_DRIVES; i++ ) {
    d = &disciple_drives[ i ];
    disciple_reset_drive( d );
  }

  /* We can eject disks only if they are currently present */
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_EJECT,
                    disciple_drives[ DISCIPLE_DRIVE_1 ].disk->fd != -1 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_EJECT,
                    disciple_drives[ DISCIPLE_DRIVE_2 ].disk->fd != -1 );

  disciple_current = &disciple_drives[ 0 ];
  machine_current->memory_map();
  disciple_event_index( 0 );

  return 0;
}

void
disciple_inhibit( void )
{
}

void
disciple_end( void )
{
  disciple_available = 0;
}

libspectrum_byte
disciple_sr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;
  return wd1770_sr_read( disciple_current );
}

void
disciple_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd1770_cr_write( disciple_current, b );
}

libspectrum_byte
disciple_tr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;
  return wd1770_tr_read( disciple_current );
}

void
disciple_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd1770_tr_write( disciple_current, b );
}

libspectrum_byte
disciple_sec_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;
  return wd1770_sec_read( disciple_current );
}

void
disciple_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd1770_sec_write( disciple_current, b );
}

libspectrum_byte
disciple_dr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;
  return wd1770_dr_read( disciple_current );
}

void
disciple_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd1770_dr_write( disciple_current, b );
}

libspectrum_byte
disciple_joy_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  DPRINTF( "disciple_%s\n", "joy_read" );
  return 0;
}

void
disciple_cn_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  int drive, side;
  DPRINTF( "disciple_%s(%i)\n", "cn_write", ( int ) b );

  if( !disciple_isplusd ) {
    drive = ( b & 0x01 ) ? 0 : 1;
    side  = ( b & 0x02 ) ? 1 : 0;
    /* disciple_rombank = ( b & 0x08 ) ? 1 : 0; */
    machine_current->memory_map();
    if( b & 0x10 )
      disciple_inhibit();
  } else {
    drive = b & 0x03;
    side = (b & 0x80) ? 1 : 0;
  }
  disciple_current = &disciple_drives[ side ];
  disciple_current->side = side;
}

libspectrum_byte
disciple_net_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  DPRINTF( "disciple_%s\n", "net_read" );
  return 0;
}

libspectrum_byte
disciple_boot_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  MDPRINTF( "disciple_%s\n", "boot_read" );

  disciple_memswap = SWAPMEMSWAP ? 1 : 0;
  machine_current->memory_map();
  return 0;
}

void
disciple_boot_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  MDPRINTF( "disciple_%s\n", "boot_write" );

  disciple_memswap = SWAPMEMSWAP ? 0 : 1;
  machine_current->memory_map();
}

libspectrum_byte
disciple_mem_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  MDPRINTF( "disciple_%s\n", "mem_read" );

  if( !SWAPPAGING )
    disciple_page();
  else
    disciple_unpage();
  return 0;
}

void
disciple_mem_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  MDPRINTF( "disciple_%s\n", "mem_write" );

  if( !SWAPPAGING )
    disciple_unpage();
  else
    disciple_page();
}

libspectrum_byte
disciple_print_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  return 0;
}

void
disciple_print_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  DPRINTF( "disciple_%s\n", "print_write" );
}

libspectrum_byte
disciple_joy2_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  DPRINTF( "disciple_%s\n", "joy2_read" );
  return 0;
}

int
disciple_disk_insert_default_autoload( disciple_drive_number which,
                                       const char *filename )
{
  return disciple_disk_insert( which, filename, settings_current.auto_load );
}

int
disciple_disk_insert( disciple_drive_number which, const char *filename,
                      int autoload )
{
  int l;
  wd1770_drive *d;

  if( which >= DISCIPLE_NUM_DRIVES )
    return 1;

  d = &disciple_drives[ which ];
  if( !( d->disk && d->disk->buffer && d->disk->dirty && d->disk->present ) )
    return 1;

  l = strlen( filename );
  if( l >= 5 ) {
    if( !strcmp( filename + ( l - 4 ), ".dsk" ) )
      d->disk->alternatesides = 1;
    else if( !strcmp( filename + ( l - 4 ), ".mgt" ) )
      d->disk->alternatesides = 1;
    else if( !strcmp( filename + ( l - 4 ), ".img" ) )
      d->disk->alternatesides = 0;
    else
      return 1;
  }

  if( d->disk->fd != -1 ) {
    if( disciple_disk_eject( which, 0 ) )
      return 1;
  }

  d->disk->readonly = 0;
  if( ( d->disk->fd = open( filename, O_RDWR ) ) == -1 ) {
    if( errno != EACCES ||
        ( d->disk->fd = open( filename, O_RDONLY ) ) == -1 ) {
      fprintf( stderr, "disciple_disk_insert: open() failed: %s\n",
               strerror( errno ) );
      return 1;
    }
    d->disk->readonly = 1;
  }

  memset( d->disk->present, 0, 2 * d->disk->numtracks * d->disk->numsectors );
  memset( d->disk->dirty, 0, 2 * d->disk->numtracks * d->disk->numsectors );

  strncpy( d->disk->filename, filename, PATH_MAX );
  d->disk->filename[ PATH_MAX - 1 ] = '\0';

  /* Set the 'eject' item active */
  switch( which ) {
  case DISCIPLE_DRIVE_1:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_EJECT, 1 );
    break;
  case DISCIPLE_DRIVE_2:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_EJECT, 1 );
    break;
  case DISCIPLE_DRIVE_3:
/*  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_3_EJECT, 1 ); */
    break;
  case DISCIPLE_DRIVE_4:
/*  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_4_EJECT, 1 ); */
    break;
  }
  return 0;
}

int
disciple_disk_eject( disciple_drive_number which, int write )
{
  wd1770_drive *d;

  if( which >= DISCIPLE_NUM_DRIVES )
    return 1;

  d = &disciple_drives[ which ];

  if( !d->disk || d->disk->fd == -1 )
    return 1;

  if( write )
    disciple_disk_write( which, d->disk->filename );

/* ui_disciple_disk_write( which ); */

  close( d->disk->fd );
  d->disk->fd = -1;

  /* Set the 'eject' item inactive */
  switch( which ) {
  case DISCIPLE_DRIVE_1:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_EJECT, 0 );
    break;
  case DISCIPLE_DRIVE_2:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_EJECT, 0 );
    break;
  case DISCIPLE_DRIVE_3:
/*  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_3_EJECT, 0 ); */
    break;
  case DISCIPLE_DRIVE_4:
/*  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_4_EJECT, 0 ); */
    break;
  }
  return 0;
}

int
disciple_disk_write( disciple_drive_number which, const char *filename )
{
  wd1770_drive *d = &disciple_drives[ which ];
  disk_info *newdisk;

  newdisk = disk_image_write( d->disk, filename );
  if( newdisk ) {
    close( d->disk->fd );
    free( d->disk );
    d->disk = newdisk;
    return 0;
  } else {
    return 1;
  }
}

int
disciple_event_cmd_done( libspectrum_dword last_tstates )
{
  disciple_current->status_register &= ~WD1770_SR_BUSY;
  return 0;
}

int
disciple_event_index( libspectrum_dword last_tstates )
{
  int error;
  int next_tstates;
  int i;

  disciple_index_pulse = !disciple_index_pulse;
  for( i = 0; i < DISCIPLE_NUM_DRIVES; i++ ) {
    wd1770_drive *d = &disciple_drives[ i ];

    d->index_pulse = disciple_index_pulse;
    if( !disciple_index_pulse && d->index_interrupt ) {
      wd1770_set_cmdint( d );
      d->index_interrupt = 0;
    }
  }
  next_tstates = ( disciple_index_pulse ? 10 : 190 ) *
    machine_current->timings.processor_speed / 1000;
  error = event_add( last_tstates + next_tstates, EVENT_TYPE_DISCIPLE_INDEX );
  if( error )
    return error;
  return 0;
}

int
disciple_from_snapshot( libspectrum_snap *snap, int capabilities )
{
#if 0
  if( !disciple_available ) return 0;

  disciple_active = libspectrum_snap_disciple_paged( snap );

  if( disciple_active ) {
    disciple_page();
  } else {
    disciple_unpage();
  }

  trdos_direction = libspectrum_snap_beta_direction( snap );

  disciple_cr_write ( 0x001f, libspectrum_snap_beta_status( snap ) );
  disciple_tr_write ( 0x003f, libspectrum_snap_beta_track ( snap ) );
  disciple_sec_write( 0x005f, libspectrum_snap_beta_sector( snap ) );
  disciple_dr_write ( 0x007f, libspectrum_snap_beta_data  ( snap ) );
  disciple_sp_write ( 0x00ff, libspectrum_snap_beta_system( snap ) );
#endif

  return 0;
}

int
disciple_to_snapshot( libspectrum_snap *snap )
{
#if 0
  libspectrum_snap_set_beta_paged( snap, disciple_active );
  libspectrum_snap_set_beta_direction( snap, disciple_direction );
  libspectrum_snap_set_beta_status( snap, disciple_status_register );
  libspectrum_snap_set_beta_track ( snap, disciple_system_register );
  libspectrum_snap_set_beta_sector( snap, disciple_track_register );
  libspectrum_snap_set_beta_data  ( snap, disciple_sector_register );
  libspectrum_snap_set_beta_system( snap, disciple_data_register );
#endif

  return 0;
}
