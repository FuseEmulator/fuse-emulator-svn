/* plusd.c: Routines for handling +D snapshots
   Copyright (c) 1998,2003 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include "internals.h"

#define PLUSD_HEADER_LENGTH 22

libspectrum_error
libspectrum_plusd_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
			size_t length )
{
  libspectrum_byte i, iff; const libspectrum_byte *ptr;
  libspectrum_word sp;
  libspectrum_error error;

  /* Length must be at least the header plus 48K of RAM */
  if( length < PLUSD_HEADER_LENGTH + 0xc000 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_plusd_read: not enough bytes for header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* All +D snaps are of the 48K machine */
  libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );

  libspectrum_snap_set_iy ( snap, buffer[ 0] + buffer[ 1] * 0x100 );
  libspectrum_snap_set_ix ( snap, buffer[ 2] + buffer[ 3] * 0x100 );
  libspectrum_snap_set_de_( snap, buffer[ 4] + buffer[ 5] * 0x100 );
  libspectrum_snap_set_bc_( snap, buffer[ 6] + buffer[ 7] * 0x100 );
  libspectrum_snap_set_hl_( snap, buffer[ 8] + buffer[ 9] * 0x100 );
  libspectrum_snap_set_f_ ( snap, buffer[10] );
  libspectrum_snap_set_a_ ( snap, buffer[11] );
  libspectrum_snap_set_de ( snap, buffer[12] + buffer[13] * 0x100 );
  libspectrum_snap_set_bc ( snap, buffer[14] + buffer[15] * 0x100 );
  libspectrum_snap_set_hl ( snap, buffer[16] + buffer[17] * 0x100 );
  i = buffer[18]; libspectrum_snap_set_i( snap, i );
  /* Header offset 19 is 'rubbish' */
  sp = buffer[20] + buffer[21] * 0x100;

  /* Make a guess at the interrupt mode depending on what I was set to */
  libspectrum_snap_set_im( snap, ( i == 0 || i == 63 ) ? 1 : 2 );

  buffer += PLUSD_HEADER_LENGTH; length -= PLUSD_HEADER_LENGTH;

  /* We must have 0x4000 <= SP <= 0xfffa so we can rescue the stacked
     registers */
  if( sp < 0x4000 || sp > 0xfffa ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_plusd_read: SP invalid (0x%04x)", sp
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }
    
  /* R, IFF, AF and PC are stored on the stack */
  ptr = &buffer[ sp - 0x4000 ];

  libspectrum_snap_set_r   ( snap, ptr[0] );
  iff = ptr[1] & 0x04;
  libspectrum_snap_set_iff1( snap, iff );
  libspectrum_snap_set_iff2( snap, iff );
  libspectrum_snap_set_a   ( snap, ptr[2] );
  libspectrum_snap_set_f   ( snap, ptr[3] );
  libspectrum_snap_set_pc  ( snap, ptr[4] + ptr[5] * 0x100 );

  /* Store SP + 6 to account for those unstacked values */
  libspectrum_snap_set_sp( snap, sp + 6 );

  /* Split the RAM into separate pages */
  error = libspectrum_split_to_48k_pages( snap, buffer );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}
