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

/* The frame timings of a machine */
typedef struct timings_t {

  /* Processor speed in Hz */
  libspectrum_dword processor_speed;

  /* AY clock speed in Hz */
  libspectrum_dword ay_speed;

  /* Line timings in tstates */
  libspectrum_word left_border, horizontal_screen, right_border,
	           horizontal_retrace;

  /* Frame timings in lines */
  libspectrum_word top_border, vertical_screen, bottom_border,
	           vertical_retrace;

  /* How long after interrupt is the top-level pixel of the main screen
     displayed */
  libspectrum_dword top_left_pixel;

} timings_t;

/* The actual data from which the full timings are constructed */
static timings_t base_timings[] = {

  /* FIXME: These are almost certainly wrong in many places, but I don't know
            what the correct values are. Corrections very welcome */

                   /* /- Horizonal -\  /-- Vertical -\ */
  { 3500000,       0, 24, 128, 24, 48, 48, 192, 48, 24, 14336 }, /* 48K */
  { 3528000,       0, 24, 128, 24, 48, 48, 192, 48, 27, 14336 }, /* TC2048 */
  { 3546900, 1773400, 24, 128, 24, 52, 48, 192, 48, 23, 14361 }, /* 128K */
  { 3546900, 1773400, 24, 128, 24, 52, 48, 192, 48, 23, 14361 }, /* +2 */
  { 3546900, 1773400, 36, 128, 28, 32, 64, 192, 48, 16, 17988 }, /* Pentagon */
  { 3546900, 1773400, 24, 128, 24, 52, 48, 192, 48, 23, 14361 }, /* +2A */
  { 3546900, 1773400, 24, 128, 24, 52, 48, 192, 48, 23, 14361 }, /* +3 */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },			  /* Unknown machine */
  { 3500000,       0, 24, 128, 24, 48, 48, 192, 48, 24, 14336 }, /* 16K */
  { 3528000, 1764750, 24, 128, 24, 48, 48, 192, 48, 27, 14336 }, /* TC2068 */
  { 3546900, 1773400, 24, 128, 32, 40, 48, 192, 48, 24, 14336 }, /* Scorpion */
  { 3546900, 1773400, 24, 128, 24, 52, 48, 192, 48, 23, 14361 }, /* +3e */
};

libspectrum_dword
libspectrum_timings_processor_speed( libspectrum_machine machine )
{
  return base_timings[ machine ].processor_speed;
}

libspectrum_dword
libspectrum_timings_ay_speed( libspectrum_machine machine )
{
  return base_timings[ machine ].ay_speed;
}

libspectrum_word
libspectrum_timings_left_border( libspectrum_machine machine )
{
  return base_timings[ machine ].left_border;
}

libspectrum_word
libspectrum_timings_horizontal_screen( libspectrum_machine machine )
{
  return base_timings[ machine ].horizontal_screen;
}

libspectrum_word
libspectrum_timings_right_border( libspectrum_machine machine )
{
  return base_timings[ machine ].right_border;
}

libspectrum_word
libspectrum_timings_horizontal_retrace( libspectrum_machine machine )
{
  return base_timings[ machine ].horizontal_retrace;
}

libspectrum_word
libspectrum_timings_top_border( libspectrum_machine machine )
{
  return base_timings[ machine ].top_border;
}

libspectrum_word
libspectrum_timings_vertical_screen( libspectrum_machine machine )
{
  return base_timings[ machine ].vertical_screen;
}

libspectrum_word
libspectrum_timings_bottom_border( libspectrum_machine machine )
{
  return base_timings[ machine ].bottom_border;
}

libspectrum_word
libspectrum_timings_vertical_retrace( libspectrum_machine machine )
{
  return base_timings[ machine ].vertical_retrace;
}

libspectrum_word
libspectrum_timings_top_left_pixel( libspectrum_machine machine )
{
  return base_timings[ machine ].top_left_pixel;
}

libspectrum_word
libspectrum_timings_tstates_per_line( libspectrum_machine machine )
{
  return base_timings[ machine ].left_border        +
         base_timings[ machine ].horizontal_screen  +
         base_timings[ machine ].right_border       +
	 base_timings[ machine ].horizontal_retrace;
}

libspectrum_word
libspectrum_timings_lines_per_frame( libspectrum_machine machine )
{
  return base_timings[ machine ].top_border       +
	 base_timings[ machine ].vertical_screen  +
	 base_timings[ machine ].bottom_border    +
	 base_timings[ machine ].vertical_retrace;
}

libspectrum_dword
libspectrum_timings_tstates_per_frame( libspectrum_machine machine )
{
  return libspectrum_timings_tstates_per_line( machine ) *
    ( (libspectrum_dword)libspectrum_timings_lines_per_frame( machine ) );
}
