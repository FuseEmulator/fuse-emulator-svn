/* disciple.h: Routines for handling the +D/DISCiPLE interface
   Copyright (c) 2005 Stuart Brady

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
     Post: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

   Stuart: sdbrady@ntlworld.com

*/

#ifndef FUSE_DISCIPLE_H
#define FUSE_DISCIPLE_H

#include <libspectrum.h>
#include "wd1770.h"
#include "periph.h"

extern int disciple_available;  /* Is DISCiPLE available for use? */
extern int disciple_isplusd;    /* Do we have a +D, or a DISCiPLE? */
extern int disciple_active;     /* DISCiPLE enabled? */
extern int disciple_index_pulse;

extern const periph_t disciple_peripherals[];
extern const size_t disciple_peripherals_count;

extern const periph_t plusd_peripherals[];
extern const size_t plusd_peripherals_count;

int disciple_init( void );

int disciple_reset( void );

void disciple_end( void );

void disciple_page( void );
void disciple_unpage( void );
void disciple_memory_map( void );

int disciple_from_snapshot( libspectrum_snap *snap, int capabilities );
int disciple_to_snapshot( libspectrum_snap *snap );

libspectrum_byte disciple_sr_read( libspectrum_word port, int *attached );
void disciple_cr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte disciple_tr_read( libspectrum_word port, int *attached );
void disciple_tr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte disciple_sec_read( libspectrum_word port, int *attached );
void disciple_sec_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte disciple_dr_read( libspectrum_word port, int *attached );
void disciple_dr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte disciple_joy_read( libspectrum_word port, int *attached );
void disciple_cn_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte disciple_net_read( libspectrum_word port, int *attached );

libspectrum_byte disciple_boot_read( libspectrum_word port, int *attached );
void disciple_boot_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte disciple_mem_read( libspectrum_word port, int *attached );
void disciple_mem_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte disciple_print_read( libspectrum_word port, int *attached );
void disciple_print_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte disciple_joy2_read( libspectrum_word port, int *attached );

/* DISCiPLE supports two drives.
 * +D supports four drives.
 */
typedef enum disciple_drive_number {
  DISCIPLE_DRIVE_1 = 0,
  DISCIPLE_DRIVE_2,
  DISCIPLE_DRIVE_3,
  DISCIPLE_DRIVE_4,
} disciple_drive_number;

#define DISCIPLE_NUM_DRIVES 4
extern wd1770_drive disciple_drives[ DISCIPLE_NUM_DRIVES ];
extern wd1770_drive *disciple_current;

int disciple_disk_insert( disciple_drive_number which, const char *filename,
                          int autoload );
int disciple_disk_insert_default_autoload( disciple_drive_number which,
                                           const char *filename );
int disciple_disk_eject( disciple_drive_number which, int write );
int disciple_disk_write( disciple_drive_number which, const char *filename );
int disciple_event_cmd_done( libspectrum_dword last_tstates );
int disciple_event_index( libspectrum_dword last_tstates );

#endif                  /* #ifndef FUSE_DISCIPLE_H */
