/* microdrive.c: Routines for handling microdrive images
   Copyright (c) 2004 Philip Kendall

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
     Post: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England
*/

#include <config.h>

#include "internals.h"

/* The type itself */

struct libspectrum_microdrive {

  libspectrum_byte data[ LIBSPECTRUM_MICRODRIVE_DATA_LENGTH ];
  int write_protect;

};

const static size_t MDR_LENGTH = LIBSPECTRUM_MICRODRIVE_DATA_LENGTH + 1;

/* Constructor/destructor */

/* Allocate a microdrive image */
libspectrum_error
libspectrum_microdrive_alloc( libspectrum_microdrive **microdrive )
{
  *microdrive = malloc( sizeof( **microdrive ) );
  if( !*microdrive ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_MEMORY,
      "libspectrum_microdrive_alloc: out of memory at %s:%d", __FILE__,
      __LINE__
    );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free a microdrive image */
libspectrum_error
libspectrum_microdrive_free( libspectrum_microdrive *microdrive )
{
  free( microdrive );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Accessors */

libspectrum_byte
libspectrum_microdrive_data( const libspectrum_microdrive *microdrive,
			     size_t which )
{
  return microdrive->data[ which ];
}

void
libspectrum_microdrive_set_data( libspectrum_microdrive *microdrive,
				 size_t which, libspectrum_byte data )
{
  microdrive->data[ which ] = data;
}

int
libspectrum_microdrive_write_protect( const libspectrum_microdrive *microdrive )
{
  return microdrive->write_protect;
}

void
libspectrum_microdrive_set_write_protect( libspectrum_microdrive *microdrive,
					  int write_protect )
{
  microdrive->write_protect = write_protect;
}

/* .mdr format routines */

libspectrum_error
libspectrum_microdrive_mdr_read( libspectrum_microdrive *microdrive,
				 libspectrum_byte *buffer, size_t length )
{
  if( length < MDR_LENGTH ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_microdrive_mdr_read: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( microdrive->data, buffer, MDR_LENGTH ); buffer += MDR_LENGTH;

  libspectrum_microdrive_set_write_protect( microdrive, *buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_microdrive_mdr_write( const libspectrum_microdrive *microdrive,
				  libspectrum_byte **buffer, size_t *length )
{
  *buffer = malloc( MDR_LENGTH );
  if( !*buffer ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_MEMORY,
      "libspectrum_microdrive_mdr_write: out of memory at %s:%d", __FILE__,
      __LINE__
    );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  memcpy( *buffer, microdrive->data, LIBSPECTRUM_MICRODRIVE_DATA_LENGTH );

  
  (*buffer)[ LIBSPECTRUM_MICRODRIVE_DATA_LENGTH ] = microdrive->write_protect;

  return LIBSPECTRUM_ERROR_NONE;
}
