/* tc2068.h: Timex TC2068 specific routines
   Copyright (c) 2004 Fredrick Meunier

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

#ifndef FUSE_TS2068_H
#define FUSE_TS2068_H

#include <libspectrum.h>

#include "machine.h"

int tc2068_init( fuse_machine_info *machine );
int tc2068_tc2048_common_reset( void );
libspectrum_byte tc2068_unattached_port( void );

libspectrum_byte tc2068_ay_registerport_read( libspectrum_word port,
                                              int *attached );
libspectrum_byte tc2068_ay_dataport_read( libspectrum_word port,
                                          int *attached );
libspectrum_byte tc2068_contend_delay( libspectrum_dword time );

int tc2068_memory_map( void );

extern libspectrum_byte fake_bank[ MEMORY_PAGE_SIZE ];
extern memory_page fake_mapping;

extern const periph_t tc2068_peripherals[];
extern const size_t tc2068_peripherals_count;

#endif			/* #ifndef FUSE_TS2068_H */
