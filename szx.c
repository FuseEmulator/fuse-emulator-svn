/* szx.c: Routines for .szx snapshots
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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <string.h>

#include "internals.h"

const static char *signature = "ZXST";
const static size_t signature_length = 4;

static libspectrum_error
read_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
	    const libspectrum_byte *end );

typedef libspectrum_error (*read_chunk_fn)( libspectrum_snap *snap,
					    const libspectrum_byte **buffer,
					    const libspectrum_byte *end,
					    size_t data_length );

static libspectrum_error
read_ay_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
	       const libspectrum_byte *end, size_t data_length )
{
  size_t i;

  if( data_length != 18 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_ay_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  (*buffer)++;			/* Skip the flags */

  libspectrum_snap_set_out_ay_registerport( snap, **buffer ); (*buffer)++;

  for( i = 0; i < 16; i++ ) {
    libspectrum_snap_set_ay_registers( snap, i, **buffer ); (*buffer)++;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_ramp_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
		 const libspectrum_byte *end, size_t data_length )
{
  libspectrum_word flags;
  libspectrum_byte page, *buffer2;

  size_t uncompressed_length;

  libspectrum_error error;

  if( data_length < 3 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "szx_read_ramp_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );

  page = **buffer; (*buffer)++;
  if( page > 7 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "szx_read_ramp_chunk: unknown page number %d", page
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( flags & 0x01 ) {

    uncompressed_length = 0x4000;

    error = libspectrum_zlib_inflate( *buffer, data_length - 3, &buffer2,
				      &uncompressed_length );
    if( error ) return error;

    *buffer += data_length - 3;

  } else {

    if( data_length < 3 + 0x4000 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
	"szx_read_ramp_chunk: length %lu too short", (unsigned long)data_length
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    buffer2 = malloc( 0x4000 * sizeof( libspectrum_byte ) );
    if( !buffer2 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_MEMORY,
	"szx_read_ramp_chunk: out of memory at %s:%d", __FILE__, __LINE__
      );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    memcpy( buffer2, *buffer, 0x4000 ); *buffer += 0x4000;
  }

  libspectrum_snap_set_pages( snap, page, buffer2 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_spcr_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
		 const libspectrum_byte *end, size_t data_length )
{
  if( data_length != 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "szx_read_spcr_chunk: unknown length %lu", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_out_ula( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_out_128_memoryport( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_out_plus3_memoryport( snap, **buffer ); (*buffer)++;

  *buffer += 5;			/* Skip 'reserved' data */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_z80r_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
		 const libspectrum_byte *end, size_t data_length )
{
  if( data_length != 37 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_z80r_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_a   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_f   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_bc  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_de  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_hl  ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_a_  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_f_  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_bc_ ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_de_ ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_hl_ ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_ix  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_iy  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_sp  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_pc  ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_i   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_r   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_iff1( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_iff2( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_im  ( snap, **buffer ); (*buffer)++;

  libspectrum_snap_set_tstates( snap, libspectrum_read_dword( buffer ) );

  *buffer += 4;	/* Skip dwHoldIntReqCycles as I don't know what it does */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
	    const libspectrum_byte *end, size_t data_length )
{
  *buffer += data_length;
  return LIBSPECTRUM_ERROR_NONE;
}

struct read_chunk_t {

  char *id;
  read_chunk_fn function;

};

static struct read_chunk_t read_chunks[] = {

  { "AMXM",   skip_chunk      },
  { "AY\0\0", read_ay_chunk   },
  { "CRTR",   skip_chunk      },
  { "DRUM",   skip_chunk      },
  { "DSK\0",  skip_chunk      },
  { "IF1\0",  skip_chunk      },
  { "IF2R",   skip_chunk      },
  { "JOY\0",  skip_chunk      },
  { "KEYB",   skip_chunk      },
  { "MDRV",   skip_chunk      },
  { "MFCE",   skip_chunk      },
  { "RAMP",   read_ramp_chunk },
  { "ROM\0",  skip_chunk      },
  { "SPCR",   read_spcr_chunk },
  { "TAPE",   skip_chunk      },
  { "USPE",   skip_chunk      },
  { "Z80R",   read_z80r_chunk },
  { "ZXPR",   skip_chunk      },
  { "+3\0\0", skip_chunk      },

};

static size_t read_chunks_count =
  sizeof( read_chunks ) / sizeof( struct read_chunk_t );

static libspectrum_error
read_chunk_header( char *id, libspectrum_dword *data_length, 
		   const libspectrum_byte **buffer,
		   const libspectrum_byte *end )
{
  if( end - *buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "szx_read_chunk_header: not enough data for chunk header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( id, *buffer, 4 ); id[4] = '\0'; *buffer += 4;
  *data_length = libspectrum_read_dword( buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
	    const libspectrum_byte *end )
{
  char id[5];
  libspectrum_dword data_length;
  libspectrum_error error;
  size_t i; int done;

  error = read_chunk_header( id, &data_length, buffer, end );
  if( error ) return error;

  if( *buffer + data_length > end ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "szx_read_chunk: chunk length goes beyond end of file"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  done = 0;

  for( i = 0; !done && i < read_chunks_count; i++ ) {

    if( !memcmp( id, read_chunks[i].id, 4 ) ) {
      error = read_chunks[i].function( snap, buffer, end, data_length );
      if( error ) return error;
      done = 1;
    }

  }

  if( !done ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_chunk: unknown chunk id '%s'", id );
    *buffer += data_length;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_szx_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		      size_t length )
{
  libspectrum_error error;
  const libspectrum_byte *end = buffer + length;

  if( end - buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_szx_read: not enough data for SZX header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( memcmp( buffer, signature, signature_length ) ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_SIGNATURE,
      "libspectrum_szx_read: wrong signature"
    );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  /* Skip the signature and the two version bytes */
  buffer += signature_length + 2;

  switch( *buffer ) {

  case 0:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_16 );
    break;

  case 1:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
    break;

  case 2:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128 );
    break;

  case 3:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2 );
    break;

  case 4:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2A );
    break;

  case 5:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS3 );
    break;

  /* case 6: +3e: not supported yet */

  case 7:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PENT );
    break;

  default:
    libspectrum_print_error(
      LIBSPECTRUM_MACHINE_UNKNOWN,
      "libspectrum_szx_read: unknown machine type %d", (int)*buffer
    );
    return LIBSPECTRUM_MACHINE_UNKNOWN;
  }

  /* Skip to the end of the header */
  buffer += 2;

  while( buffer < end ) {
    error = read_chunk( snap, &buffer, end );
    if( error ) {

      /* Tidy up any RAM pages we may have allocated */
      size_t i;

      for( i = 0; i < 8; i++ ) {
	libspectrum_byte *page = libspectrum_snap_pages( snap, i );
	if( page ) {
	  free( page );
	  libspectrum_snap_set_pages( snap, i, NULL );
	}
      }

      return error;
    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}
