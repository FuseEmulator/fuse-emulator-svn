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

#include <stdio.h>
#include <string.h>

#include "internals.h"

/* The function to call on errors */
libspectrum_error_function_t libspectrum_error_function =
  libspectrum_default_error_function;

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
  case LIBSPECTRUM_MACHINE_16:     return "Spectrum 16K";
  case LIBSPECTRUM_MACHINE_48:     return "Spectrum 48K";
  case LIBSPECTRUM_MACHINE_TC2048: return "Timex TC2048";
  case LIBSPECTRUM_MACHINE_TC2068: return "Timex TC2068";
  case LIBSPECTRUM_MACHINE_128:    return "Spectrum 128K";
  case LIBSPECTRUM_MACHINE_PLUS2:  return "Spectrum +2";
  case LIBSPECTRUM_MACHINE_PENT:   return "Pentagon 128K";
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
const int LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK   = 1 << 6;
                                                    /* TRDOS-style disk drive */

/* Given a machine type, what features does it have? */
int
libspectrum_machine_capabilities( libspectrum_machine type )
{
  int capabilities = 0;

  /* AY-3-8912 */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_128: case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_PLUS2A: case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_TC2068:
  case LIBSPECTRUM_MACHINE_PENT:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_AY; break;
  default:
    break;
  }

  /* 128K Spectrum-style 0x7ffd memory paging */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_128: case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_PLUS2A: case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PENT:
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
  case LIBSPECTRUM_MACHINE_TC2068:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY; break;
  default:
    break;
  }

  /* TC20[46]8-style 0x00ff video mode selection */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_TC2048:
  case LIBSPECTRUM_MACHINE_TC2068:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO; break;
  default:
    break;
  }

  /* TRDOS-style disk */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_PENT:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK; break;
  default:
    break;
  }

  return capabilities;
}

/* Given a buffer and optionally a filename, make a best guess as to
   what sort of file this is */
libspectrum_error
libspectrum_identify_file( libspectrum_id_t *type, const char *filename,
			   const unsigned char *buffer, size_t length )
{
  struct type {

    int type;

    char *extension; int extension_score;

    char *signature; size_t offset, length; int sig_score;
  };

  struct type *ptr,
    types[] = {
      
      { LIBSPECTRUM_ID_RECORDING_RZX, "rzx", 3, "RZX!",		    0, 4, 4 },

      { LIBSPECTRUM_ID_SNAPSHOT_SNA,  "sna", 3, NULL,		    0, 0, 0 },
      { LIBSPECTRUM_ID_SNAPSHOT_SNP,  "snp", 3, NULL,		    0, 0, 0 },
      { LIBSPECTRUM_ID_SNAPSHOT_Z80,  "z80", 3, "\0\0",		    6, 2, 1 },
      /* .slt files also dealt with by the .z80 loading code */
      { LIBSPECTRUM_ID_SNAPSHOT_Z80,  "slt", 3, "\0\0",		    6, 2, 1 },
      { LIBSPECTRUM_ID_SNAPSHOT_ZXS,  "zxs", 3, "SNAP",		    8, 4, 4 },

      { LIBSPECTRUM_ID_CARTRIDGE_DCK, "dck", 3, NULL,		    0, 0, 0 },

      { LIBSPECTRUM_ID_TAPE_TAP,      "tap", 3, "\x13\0\0",	    0, 3, 1 },
      { LIBSPECTRUM_ID_TAPE_TZX,      "tzx", 3, "ZXTape!",	    0, 7, 4 },
      { LIBSPECTRUM_ID_TAPE_WARAJEVO, "tap", 2, "\xff\xff\xff\xff", 8, 4, 2 },

      { LIBSPECTRUM_ID_DISK_DSK,      "dsk", 3, NULL,		    0, 0, 0 },
      { LIBSPECTRUM_ID_DISK_SCL,      "scl", 3, "SINCLAIR",         0, 8, 4 },
      { LIBSPECTRUM_ID_DISK_TRD,      "trd", 3, NULL,		    0, 0, 0 },

      { -1 }, /* End marker */

    };

  const char *extension = NULL;
  int score, best_score, best_guess, duplicate_best;

  /* Get the filename extension, if it exists */
  if( filename ) {
    extension = strrchr( filename, '.' ); if( extension ) extension++;
  }

  best_guess = LIBSPECTRUM_ID_UNKNOWN; best_score = 0; duplicate_best = 0;

  /* Compare against known extensions and signatures */
  for( ptr = types; ptr->type != -1; ptr++ ) {

    score = 0;

    if( extension && ptr->extension &&
	!strcasecmp( extension, ptr->extension ) )
      score += ptr->extension_score;

    if( ptr->signature && length >= ptr->offset + ptr->length &&
	!memcmp( &buffer[ ptr->offset ], ptr->signature, ptr->length ) )
      score += ptr->sig_score;

    if( score > best_score ) {
      best_guess = ptr->type; best_score = score; duplicate_best = 0;
    } else if( score == best_score && ptr->type != best_guess ) {
      duplicate_best = 1;
    }
  }

  /* If two things were equally good, we can't identify this. Otherwise,
     return our best guess */
  if( duplicate_best ) {
    *type = LIBSPECTRUM_ID_UNKNOWN;
  } else {
    *type = best_guess;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* What generic 'class' of file is this file */
libspectrum_error
libspectrum_identify_class( libspectrum_class_t *class, libspectrum_id_t type )
{
  switch( type ) {

  case LIBSPECTRUM_ID_UNKNOWN:
    *class = LIBSPECTRUM_CLASS_UNKNOWN; return 0;
  
  case LIBSPECTRUM_ID_CARTRIDGE_DCK:
    *class = LIBSPECTRUM_CLASS_CARTRIDGE_TIMEX; return 0;

  case LIBSPECTRUM_ID_DISK_DSK:
    *class = LIBSPECTRUM_CLASS_DISK_PLUS3; return 0;

  case LIBSPECTRUM_ID_DISK_SCL:
  case LIBSPECTRUM_ID_DISK_TRD:
    *class = LIBSPECTRUM_CLASS_DISK_TRDOS; return 0;

  case LIBSPECTRUM_ID_RECORDING_RZX:
    *class = LIBSPECTRUM_CLASS_RECORDING; return 0;

  case LIBSPECTRUM_ID_SNAPSHOT_PLUSD:
  case LIBSPECTRUM_ID_SNAPSHOT_SNA:
  case LIBSPECTRUM_ID_SNAPSHOT_SNP:
  case LIBSPECTRUM_ID_SNAPSHOT_Z80:
  case LIBSPECTRUM_ID_SNAPSHOT_ZXS:
    *class = LIBSPECTRUM_CLASS_SNAPSHOT; return 0;

  case LIBSPECTRUM_ID_TAPE_TAP:
  case LIBSPECTRUM_ID_TAPE_TZX:
  case LIBSPECTRUM_ID_TAPE_WARAJEVO:
    *class = LIBSPECTRUM_CLASS_TAPE; return 0;

  }

  libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			   "Unknown file type %d", type );
  return LIBSPECTRUM_ERROR_UNKNOWN;
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

/* Read an LSB word from 'buffer' */
libspectrum_word
libspectrum_read_word( const libspectrum_byte **buffer )
{
  libspectrum_word value;

  value = (*buffer)[0] + (*buffer)[1] * 0x100;

  (*buffer) += 2;

  return value;
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
