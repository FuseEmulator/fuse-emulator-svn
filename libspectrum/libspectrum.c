/* libspectrum.c: Some general routines
   Copyright (c) 2001-2003 Philip Kendall, Darren Salt

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
#ifdef HAVE_STRINGS_H
#include <strings.h>		/* Needed for strcasecmp() on QNX6 */
#endif				/* #ifdef HAVE_STRINGS_H */

#ifdef HAVE_GCRYPT_H

#include <gcrypt.h>

/* The version of libgcrypt that we need */
static const char *MIN_GCRYPT_VERSION = "1.1.42";

#endif				/* #ifdef HAVE_GCRYPT_H */

static const char *gcrypt_version;

#include "internals.h"

/* The function to call on errors */
libspectrum_error_function_t libspectrum_error_function =
  libspectrum_default_error_function;

/* Initialise the library */
libspectrum_error
libspectrum_init( void )
{
#ifdef HAVE_GCRYPT_H

  if( !gcry_control( GCRYCTL_ANY_INITIALIZATION_P ) ) {

    gcrypt_version = gcry_check_version( MIN_GCRYPT_VERSION );
    if( !gcrypt_version ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
	"libspectrum_init: found libgcrypt %s, but need %s",
	gcry_check_version( NULL ), MIN_GCRYPT_VERSION
      );
      return LIBSPECTRUM_ERROR_LOGIC;	/* FIXME: better error code */
    }

    /* Give me some 'secure' memory */
    gcry_control( GCRYCTL_INIT_SECMEM, 16384 );

    /* But it's not very secure as we're not setuid root. However, this
       isn't critical here, so don't warn about it */
    gcry_control( GCRYCTL_SUSPEND_SECMEM_WARN );

    gcry_control( GCRYCTL_INITIALIZATION_FINISHED );
  }

#else				/* #ifdef HAVE_GCRYPT_H */

  gcrypt_version = NULL;

#endif				/* #ifdef HAVE_GCRYPT_H */

  return LIBSPECTRUM_ERROR_NONE;
}

int
libspectrum_check_version( const char *version )
{
  size_t i;

  int actual_version[4]   = { 0, 0, 0, 0 },
      required_version[4] = { 0, 0, 0, 0 };

  sscanf( VERSION, "%d.%d.%d.%d",
	  &actual_version[0], &actual_version[1],
	  &actual_version[2], &actual_version[3] );
  sscanf( version, "%d.%d.%d.%d",
	  &required_version[0], &required_version[1],
	  &required_version[2], &required_version[3] );

  for( i = 0; i < 4; i++ ) {
    
    if( actual_version[i] < required_version[i] ) return 0;
    if( actual_version[i] > required_version[i] ) return 1;

  }

  /* All version numbers exactly equal, so return OK */
  return 1;
}

const char *
libspectrum_version( void )
{
  return VERSION;
}

const char *
libspectrum_gcrypt_version( void )
{
  return gcrypt_version;
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
const int LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK   = 1 << 7;
                                           /* T[SC]2068-style cartridge port */
const int LIBSPECTRUM_MACHINE_CAPABILITY_SINCLAIR_JOYSTICK = 1 << 8;
                                            /* Sinclair-style joystick ports */
const int LIBSPECTRUM_MACHINE_CAPABILITY_KEMPSTON_JOYSTICK = 1 << 9;
                                            /* Kempston-style joystick ports */

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

  /* T[SC]2068-style cartridge port */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_TC2068:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK; break;
  default:
    break;
  }

  /* Sinclair-style joystick ports */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_PLUS2: case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_SINCLAIR_JOYSTICK; break;
  default:
    break;
  }

  /* Kempston-style joystick ports */
  switch( type ) {
  case LIBSPECTRUM_MACHINE_TC2048:
    capabilities |= LIBSPECTRUM_MACHINE_CAPABILITY_KEMPSTON_JOYSTICK; break;
  default:
    break;
  }

  return capabilities;
}

/* Given a buffer and optionally a filename, make a best guess as to
   what sort of file this is */
libspectrum_error
libspectrum_identify_file_with_class(
  libspectrum_id_t *type, libspectrum_class_t *class,
  const char *filename, const unsigned char *buffer, size_t length )
{
  libspectrum_error error;
  char *new_filename; unsigned char *new_buffer; size_t new_length;

  error = libspectrum_identify_file_raw( type, filename, buffer, length );
  if( error ) return error;

  error = libspectrum_identify_class( class, *type );
  if( error ) return error;

  if( *class != LIBSPECTRUM_CLASS_COMPRESSED ) return LIBSPECTRUM_ERROR_NONE;

  error = libspectrum_uncompress_file( &new_buffer, &new_length, &new_filename,
				       *type, buffer, length, filename );
  if( error ) return error;

  error = libspectrum_identify_file_with_class( type, class, new_filename,
						new_buffer, new_length );
  if( error ) return error;

  free( new_filename ); free( new_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Identify a file, but without worrying about its class */
libspectrum_error
libspectrum_identify_file( libspectrum_id_t *type, const char *filename,
			   const unsigned char *buffer, size_t length )
{
  libspectrum_class_t class;

  return libspectrum_identify_file_with_class( type, &class, filename,
					       buffer, length );
}

/* Identify a file without attempting to decompress it */
libspectrum_error
libspectrum_identify_file_raw( libspectrum_id_t *type, const char *filename,
			       const unsigned char *buffer, size_t length )
{
  struct type {

    int type;

    const char *extension; int extension_score;

    const char *signature; size_t offset, length; int sig_score;
  };

  struct type *ptr,
    types[] = {
      
      { LIBSPECTRUM_ID_RECORDING_RZX, "rzx", 3, "RZX!",		    0, 4, 4 },

      { LIBSPECTRUM_ID_SNAPSHOT_SNA,  "sna", 3, NULL,		    0, 0, 0 },
      { LIBSPECTRUM_ID_SNAPSHOT_SNP,  "snp", 3, NULL,		    0, 0, 0 },
      { LIBSPECTRUM_ID_SNAPSHOT_SP,   "sp",  3, "\x53\x50\0",	    0, 3, 1 },
      { LIBSPECTRUM_ID_SNAPSHOT_SZX,  "szx", 3, "ZXST",		    0, 4, 4 },
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

      { LIBSPECTRUM_ID_COMPRESSED_BZ2,"bz2", 3, "BZh",		    0, 3, 4 },
      { LIBSPECTRUM_ID_COMPRESSED_GZ, "gz",  3, "\x1f\x8b",	    0, 2, 4 },

      { -1, NULL, 0, NULL, 0, 0, 0 }, /* End marker */

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

  case LIBSPECTRUM_ID_COMPRESSED_BZ2:
  case LIBSPECTRUM_ID_COMPRESSED_GZ:
    *class = LIBSPECTRUM_CLASS_COMPRESSED; return 0;

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
  case LIBSPECTRUM_ID_SNAPSHOT_SP:
  case LIBSPECTRUM_ID_SNAPSHOT_SZX:
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

libspectrum_error
libspectrum_uncompress_file( unsigned char **new_buffer, size_t *new_length,
			     char **new_filename, libspectrum_id_t type,
			     const unsigned char *old_buffer,
			     size_t old_length, const char *old_filename )
{
  libspectrum_class_t class;
  libspectrum_error error;

  error = libspectrum_identify_class( &class, type );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_COMPRESSED ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "file type %d is not a compressed type", type );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  if( new_filename && old_filename ) {
    *new_filename = strdup( old_filename );
    if( !*new_filename ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "out of memory at %s:%d", __FILE__, __LINE__ );
      return LIBSPECTRUM_ERROR_MEMORY;
    }
  }

  /* Tells the inflation routines to allocate memory for us */
  *new_length = 0;
  
  switch( type ) {

  case LIBSPECTRUM_ID_COMPRESSED_BZ2:

#ifdef HAVE_BZLIB_H

    if( new_filename ) {
      if( strlen( *new_filename ) >= 4 &&
	  !strcasecmp( &(*new_filename)[ strlen( *new_filename ) - 4 ],
		       ".bz2" ) )
	(*new_filename)[ strlen( *new_filename ) - 4 ] = '\0';
    }

    error = libspectrum_bzip2_inflate( old_buffer, old_length,
				       new_buffer, new_length );
    if( error ) { free( new_filename ); return error; }

#else				/* #ifdef HAVE_BZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "libbz2 not available to decompress bzipped file"
    );
    free( new_filename );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif				/* #ifdef HAVE_BZLIB_H */

    break;

  case LIBSPECTRUM_ID_COMPRESSED_GZ:

#ifdef HAVE_ZLIB_H

    if( new_filename ) {
      if( strlen( *new_filename ) >= 3 &&
	  !strcasecmp( &(*new_filename)[ strlen( *new_filename ) - 3 ],
		       ".gz" ) )
	(*new_filename)[ strlen( *new_filename ) - 3 ] = '\0';
    }
      
    error = libspectrum_gzip_inflate( old_buffer, old_length,
				      new_buffer, new_length );
    if( error ) { free( new_filename ); return error; }

#else				/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zlib not available to decompress gzipped file" );
    free( new_filename );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif				/* #ifdef HAVE_ZLIB_H */

    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "unknown compressed type %d", type );
    free( new_filename );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

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
