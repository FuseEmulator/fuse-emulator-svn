/* libspectrum.c: Some general routines
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internals.h"

/* The function to call on errors */
libspectrum_error_function_t libspectrum_error_function =
  libspectrum_default_error_function;

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

libspectrum_error
libspectrum_print_error( libspectrum_error error, const char *format, ... )
{
  va_list ap;

  /* If we don't have an error function, do nothing */
  if( !libspectrum_error_function ) return LIBSPECTRUM_ERROR_NONE;

  /* Otherwise, call that error function */
  va_start( ap, format );
  libspectrum_error_function( error, format, ap );
  va_end( ap );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Default error action is just to print a message to stderr */
libspectrum_error
libspectrum_default_error_function( libspectrum_error error,
				    const char *format, va_list ap )
{
   fprintf( stderr, "libspectrum error: " );
  vfprintf( stderr, format, ap );
   fprintf( stderr, "\n" );

  if( error == LIBSPECTRUM_ERROR_LOGIC ) abort();

  return LIBSPECTRUM_ERROR_NONE;
}

/* Get the name of a specific machine type */
const char *
libspectrum_machine_name( libspectrum_machine type )
{
  switch( type ) {
  case LIBSPECTRUM_MACHINE_48:     return "Spectrum 48K";
  case LIBSPECTRUM_MACHINE_TC2048: return "Timex TC2048";
  case LIBSPECTRUM_MACHINE_128:    return "Spectrum 128K";
  case LIBSPECTRUM_MACHINE_PLUS2:  return "Spectrum +2";
  case LIBSPECTRUM_MACHINE_PENT:   return "Pentagon";
  case LIBSPECTRUM_MACHINE_PLUS2A: return "Spectrum +2A";
  case LIBSPECTRUM_MACHINE_PLUS3:  return "Spectrum +3";
  default:			   return "unknown";
  }
}

/* The various capabilities of the different machines */
const int LIBSPECTRUM_MACHINE_CAPABILITY_AY           = 1 << 0; /* AY-3-8192 */
const int LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY   = 1 << 1;
                                                  /* 128-style memory paging */
const int LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY = 1 << 2;
                                                   /* +3-style memory paging */
const int LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK   = 1 << 3;
                                                      /* +3-style disk drive */
const int LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY = 1 << 4;
                                            /* TC20[46]8-style memory paging */
const int LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO  = 1 << 5;
                                              /* TC20[46]8-style video modes */

/* Given a machine type, what features does it have? */
int
libspectrum_machine_capabilities( libspectrum_machine type )
{
  int capabilities = 0;

  /* AY-3-8912 */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_128: case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_PLUS2A: case LIBSPECTRUM_MACHINE_PLUS3:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_AY; break;
  default:
    break;
  }

  /* 128K Spectrum-style 0x7ffd memory paging */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_128: case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_PLUS2A: case LIBSPECTRUM_MACHINE_PLUS3:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY; break;
  default:
    break;
  }

  /* +3 Spectrum-style 0x1ffd memory paging */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_PLUS2A: case LIBSPECTRUM_MACHINE_PLUS3:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY; break;
  default:
    break;
  }

  /* +3 Spectrum-style disk */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_PLUS3:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK; break;
  default:
    break;
  }

  /* TC20[46]8-style 0x00fd memory paging */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_TC2048:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY; break;
  default:
    break;
  }

  /* TC20[46]8-style 0x00ff video mode selection */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_TC2048:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO; break;
  default:
    break;
  }

  return capabilities;
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

/* Ensure there is room for `requested' characters after the current
   position `ptr' in `buffer'. If not, realloc() and update the
   pointers as necessary */
int
libspectrum_make_room( libspectrum_byte **dest, size_t requested,
		       libspectrum_byte **ptr, size_t *allocated )
{
  size_t current_length;

  current_length = *ptr - *dest;

  if( *allocated == 0 ) {

    (*allocated) = requested;

    *dest = (libspectrum_byte*)malloc( requested * sizeof(libspectrum_byte) );
    if( *dest == NULL ) return 1;

  } else {

    /* If there's already enough room here, just return */
    if( current_length + requested <= (*allocated) ) return 0;

    /* Make the new size the maximum of the new needed size and the
     old allocated size * 2 */
    (*allocated) =
      current_length + requested > 2 * (*allocated) ?
      current_length + requested :
      2 * (*allocated);

    *dest = (libspectrum_byte*)
      realloc( *dest, (*allocated) * sizeof( libspectrum_byte ) );
    if( *dest == NULL ) return 1;

  }

  /* Update the secondary pointer to the block */
  *ptr = *dest + current_length;

  return 0;

}

/* Read an LSB dword from buffer */
libspectrum_dword
libspectrum_read_dword( const libspectrum_byte **buffer )
{
  libspectrum_dword value;

  value = (*buffer)[0]             +
          (*buffer)[1] *     0x100 +
	  (*buffer)[2] *   0x10000 +
          (*buffer)[3] * 0x1000000 ;

  (*buffer) += 4;

  return value;
}

/* Write an (LSB) word to buffer */
int
libspectrum_write_word( libspectrum_byte **buffer, libspectrum_word w )
{
  *(*buffer)++ = w & 0xff;
  *(*buffer)++ = w >> 8;
  return LIBSPECTRUM_ERROR_NONE;
}

/* Write an LSB dword to buffer */
int libspectrum_write_dword( libspectrum_byte **buffer, libspectrum_dword d )
{
  *(*buffer)++ = ( d & 0x000000ff )      ;
  *(*buffer)++ = ( d & 0x0000ff00 ) >>  8;
  *(*buffer)++ = ( d & 0x00ff0000 ) >> 16;
  *(*buffer)++ = ( d & 0xff000000 ) >> 24;
  return LIBSPECTRUM_ERROR_NONE;
}
