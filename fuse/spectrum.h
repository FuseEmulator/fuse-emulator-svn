/* spectrum.h: Spectrum 48K specific routines
   Copyright (c) 1999-2004 Philip Kendall, Darren Salt

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

#ifndef FUSE_SPECTRUM_H
#define FUSE_SPECTRUM_H

#include <stdlib.h>

#include <libspectrum.h>

/* How many tstates have elapsed since the last interrupt? (or more
   precisely, since the ULA last pulled the /INT line to the Z80 low) */
extern libspectrum_dword tstates;

/* The last byte written to the ULA */
extern libspectrum_byte spectrum_last_ula;

int spectrum_ula_from_snapshot( libspectrum_snap *snap );
int spectrum_ula_to_snapshot( libspectrum_snap *snap );

/* Things relating to memory */

/* 272 Kb of RAM */
#define SPECTRUM_RAM_PAGES 17

extern libspectrum_byte RAM[ SPECTRUM_RAM_PAGES ][0x4000];

typedef int
  (*spectrum_port_contented_function)( libspectrum_word port );
typedef libspectrum_byte
  (*spectrum_contention_delay_function)( libspectrum_dword time );

typedef struct spectrum_raminfo {

  /* Is this port result supplied by the ULA? */
  spectrum_port_contented_function port_contended;

  /* What's the actual delay at the current tstate */
  spectrum_contention_delay_function contend_delay;

  int locked;			/* Is the memory configuration locked? */
  int current_page, current_rom; /* Current paged memory */

  libspectrum_byte last_byte;	/* The last byte sent to the 128K port */
  libspectrum_byte last_byte2;	/* The last byte sent to +3 port */

  int special;			/* Is a +3 special config in use? */

  int romcs;			/* Is the /ROMCS line low? */

} spectrum_raminfo;

/* How much contention do we get at every tstate? */
extern libspectrum_byte spectrum_contention[ 80000 ];

libspectrum_byte spectrum_ula_read( libspectrum_word port, int *attached );
void spectrum_ula_write( libspectrum_word port, libspectrum_byte b );

void spectrum_contend_port( libspectrum_word port );
libspectrum_byte spectrum_unattached_port( int offset );

/* Miscellaneous stuff */

int spectrum_frame( void );

#endif			/* #ifndef FUSE_SPECTRUM_H */
