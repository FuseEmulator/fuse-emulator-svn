/* creator.c: simple type for storing creator information
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

struct libspectrum_creator {

  libspectrum_byte program[32];
  libspectrum_word major, minor;

};

libspectrum_error
libspectrum_creator_alloc( libspectrum_creator **creator )
{
  *creator = malloc( sizeof( libspectrum_creator ) );
  if( !(*creator) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "out of memory in libspectrum_creator_alloc" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_creator_free( libspectrum_creator *creator )
{
  free( creator );

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_creator_set_program( libspectrum_creator *creator,
				 const libspectrum_byte *program )
{
  memcpy( creator->program, program, 32 );
  return LIBSPECTRUM_ERROR_NONE;
}

const libspectrum_byte*
libspectrum_creator_program( libspectrum_creator *creator )
{
  return creator->program;
}

libspectrum_error libspectrum_creator_set_major( libspectrum_creator *creator,
						 libspectrum_word major )
{
  creator->major = major;
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_word
libspectrum_creator_major( libspectrum_creator *creator )
{
  return creator->major;
}

libspectrum_error libspectrum_creator_set_minor( libspectrum_creator *creator,
						 libspectrum_word minor )
{
  creator->minor = minor;
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_word
libspectrum_creator_minor( libspectrum_creator *creator )
{
  return creator->minor;
}
