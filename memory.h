/* memory.h: memory access routines
   Copyright (c) 2003 Philip Kendall

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

#ifndef FUSE_MEMORY_H
#define FUSE_MEMORY_H

#include <libspectrum.h>

#include "spectrum.h"

typedef enum memory_bank {

  MEMORY_BANK_NONE,

  MEMORY_BANK_HOME,
  MEMORY_BANK_DOCK,
  MEMORY_BANK_EXROM,
  
} memory_bank;

typedef struct memory_page {

  libspectrum_byte *page;	/* The data for this page */
  int writable;			/* Can we write to this data? */
  int contended;		/* Are reads/writes to this page contended? */

  memory_bank bank;		/* Which bank is mapped in here */
  int page_num;			/* Which page from the bank */
  libspectrum_word offset;	/* How far into the page this chunk starts */

} memory_page;

#define MEMORY_PAGE_SIZE 0x2000

/* Each 8Kb RAM chunk accessible by the Z80 */
extern memory_page memory_map[8];

/* Two 8Kb memory chunks accessible by the Z80 when /ROMCS is low */
extern memory_page memory_map_romcs[2];

extern memory_page *memory_map_home[8];
extern memory_page *memory_map_dock[8];
extern memory_page *memory_map_exrom[8];

extern memory_page memory_map_ram[ 2 * SPECTRUM_RAM_PAGES ];
extern memory_page memory_map_rom[ 8];

/* Which RAM page contains the current screen */
extern int memory_current_screen;

/* Which bits to look at when working out where the screen is */
extern libspectrum_word memory_screen_mask;

int memory_init( void );
libspectrum_byte *memory_pool_allocate( size_t length );
void memory_pool_free( void );

const char *memory_bank_name( memory_page *page );

/* Map in alternate bank if ROMCS is set */
void memory_romcs_map( void );

libspectrum_byte readbyte( libspectrum_word address );

/* Use a macro for performance in the main core, but a function for
   flexibility in the core tester */

#ifndef CORETEST

#define readbyte_internal( address ) \
  memory_map[ (libspectrum_word)(address) >> 13 ].page[ (address) & 0x1fff ]

#else				/* #ifndef CORETEST */

libspectrum_byte readbyte_internal( libspectrum_word address );

#endif				/* #ifndef CORETEST */

void writebyte( libspectrum_word address, libspectrum_byte b );
void writebyte_internal( libspectrum_word address, libspectrum_byte b );

#endif				/* #ifndef FUSE_MEMORY_H */
