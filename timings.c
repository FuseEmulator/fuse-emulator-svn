/* timings.c: Timing routines
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

#include <config.h>

#include <string.h>

#include "internals.h"

/* The actual data from which the full timings are constructed */
libspectrum_timings base_timings[] = {

  /* FIXME: These are almost certainly wrong in many places, but I don't know
            what the correct values are. Corrections very welcome */

                             /* /- Horizonal -\  /-- Vertical -\ */
  { LIBSPECTRUM_MACHINE_16,     3500000,
				24, 128, 24, 48, 48, 192, 48, 24, 14336 },
  { LIBSPECTRUM_MACHINE_48,     3500000,
				24, 128, 24, 48, 48, 192, 48, 24, 14336 },
  { LIBSPECTRUM_MACHINE_TC2048, 3528000,
				24, 128, 24, 48, 48, 192, 48, 24, 14336 },
  { LIBSPECTRUM_MACHINE_128,    3546900,
				24, 128, 24, 52, 48, 192, 48, 23, 14361 },
  { LIBSPECTRUM_MACHINE_PLUS2,  3546900,
				24, 128, 24, 52, 48, 192, 48, 23, 14361 },
  { LIBSPECTRUM_MACHINE_PENT,   3546900,
				36, 128, 28, 32, 64, 192, 48, 16, 17988 },
  { LIBSPECTRUM_MACHINE_PLUS2A, 3546900,
				24, 128, 24, 52, 48, 192, 48, 23, 14361 },
  { LIBSPECTRUM_MACHINE_PLUS3,  3546900,
				24, 128, 24, 52, 48, 192, 48, 23, 14361 },

  { -1 } /* End marker */
};

libspectrum_error
libspectrum_machine_timings( libspectrum_timings *timings,
			     libspectrum_machine machine )
{
  libspectrum_timings *ptr;

  for( ptr = base_timings; ptr->machine != -1; ptr++ ) {

    if( ptr->machine == machine ) {

      memcpy( timings, ptr, sizeof( libspectrum_timings ) );

      /* Calculate derived fields */
      timings->tstates_per_line =
	timings->left_border + timings->horizontal_screen +
	timings->right_border + timings->horizontal_retrace;

      timings->lines_per_frame = 
	timings->top_border + timings->vertical_screen +
	timings->bottom_border + timings->vertical_retrace;

      timings->tstates_per_frame =
	timings->tstates_per_line *
	(libspectrum_dword)timings->lines_per_frame;

      return LIBSPECTRUM_ERROR_NONE;
    }
  }

  libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			   "Unknown machine type `%d'", machine );
  return LIBSPECTRUM_ERROR_UNKNOWN;

}
