/* snapshot.c: Snapshot handling routines
   Copyright (c) 2001-2002 Philip Kendall, Darren Salt

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

/* Initialise a libspectrum_snap structure (constructor!) */
libspectrum_error
libspectrum_snap_alloc( libspectrum_snap **snap )
{
  int i;

  (*snap) = (libspectrum_snap*)malloc( sizeof( libspectrum_snap ) );
  if( !(*snap) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_snap_alloc: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  for( i=0; i<8; i++ ) (*snap)->pages[i]=NULL;
  for( i=0; i<256; i++ ) { (*snap)->slt[i]=NULL; (*snap)->slt_length[i]=0; }
  (*snap)->slt_screen = NULL;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free all memory used by a libspectrum_snap structure (destructor...) */
libspectrum_error
libspectrum_snap_free( libspectrum_snap *snap )
{
  int i;

  for( i=0; i<8; i++ ) if( snap->pages[i] ) free( snap->pages[i] );
  for( i=0; i<256; i++ ) {
    if( snap->slt_length[i] ) {
      free( snap->slt[i] );
      snap->slt_length[i] = 0;
    }
  }
  if( snap->slt_screen ) { free( snap->slt_screen ); }

  free( snap );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Read in a snapshot, optionally guessing what type it is */
libspectrum_error
libspectrum_snap_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		       size_t length, libspectrum_id_t type,
		       const char *filename )
{
  libspectrum_error error;

  /* If we don't know what sort of file this is, make a best guess */
  if( type == LIBSPECTRUM_ID_UNKNOWN ) {
    error = libspectrum_identify_file( &type, filename, buffer, length );
    if( error ) return error;

    /* If we still can't identify it, give up */
    if( type == LIBSPECTRUM_ID_UNKNOWN ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
	"libspectrum_snap_read: couldn't identify file"
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
  }

  switch( type ) {

  case LIBSPECTRUM_ID_SNAPSHOT_SNA:
    return libspectrum_sna_read( snap, buffer, length );

  case LIBSPECTRUM_ID_SNAPSHOT_Z80:
    return libspectrum_z80_read( snap, buffer, length );

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "libspectrum_snap_read: not a snapshot file" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Should never happen */
  abort();
}

/* Given a 48K memory dump `data', place it into the
   appropriate bits of `snap' for a 48K machine */
int
libspectrum_split_to_48k_pages( libspectrum_snap *snap,
				const libspectrum_byte* data )
{
  /* If any of the three pages are already occupied, barf */
  if( snap->pages[5] || snap->pages[2] || snap->pages[0] ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_split_to_48k_pages: RAM page already in use"
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  /* Allocate memory for the three pages */
  snap->pages[5] =
    (libspectrum_byte*)malloc( 0x4000 * sizeof( libspectrum_byte ) );
  if( snap->pages[5] == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_split_to_48k_pages: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  snap->pages[2] =
    (libspectrum_byte*)malloc( 0x4000 * sizeof( libspectrum_byte ) );
  if( snap->pages[2] == NULL ) {
    free( snap->pages[5] ); snap->pages[5] = NULL;
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_split_to_48k_pages: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
    
  snap->pages[0] =
    (libspectrum_byte*)malloc( 0x4000 * sizeof( libspectrum_byte ) );
  if( snap->pages[0] == NULL ) {
    free( snap->pages[5] ); snap->pages[5] = NULL;
    free( snap->pages[2] ); snap->pages[2] = NULL;
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_split_to_48k_pages: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Finally, do the copies... */
  memcpy( snap->pages[5], &data[0x0000], 0x4000 );
  memcpy( snap->pages[2], &data[0x4000], 0x4000 );
  memcpy( snap->pages[0], &data[0x8000], 0x4000 );

  return LIBSPECTRUM_ERROR_NONE;
    
}
