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

/* I've had to make some assumptions about the 'right' way to do some
   things in the .szx format due to the documentation
   <http://www.spectaculator.com/docs/zx-state/intro.shtml> not being
   up to scratch:

   * http://www.spectaculator.com/docs/zx-state/header.shtml says
     "chMinorVersion
      Minor version number of the file format. Currently 1."

     despite the current version of the format being 1.2. libspectrum
     writes .szx files with a minor version number of 2.

   * The ZXSTSPECREGS block says that the ch1ffd member should be set
     to zero for machines other than the +2A/+3. libspectrum makes
     this field non-zero for Scorpion emulation.

   * In a ZXSTRAMPAGE block, the Timex machines should save the same
     pages as the 48K machine, and RAM pages 8-15 are valid for
     Scorpion emulation.

   Places where these points are used in the code are marked with
   [Assumption].

   I've mentioned these points to Jonathan Needle ('maintainer' of the
   .szx format), but he hasn't replied :-(

*/

/* The machine numbers used in the .szx format */

typedef enum szx_machine_type {

  SZX_MACHINE_16 = 0,
  SZX_MACHINE_48,
  SZX_MACHINE_128,
  SZX_MACHINE_PLUS2,
  SZX_MACHINE_PLUS2A,
  SZX_MACHINE_PLUS3,
  SZX_MACHINE_PLUS3E,
  SZX_MACHINE_PENTAGON,
  SZX_MACHINE_TC2048,
  SZX_MACHINE_TC2068,
  SZX_MACHINE_SCORPION,
  SZX_MACHINE_SE,

} szx_machine_type;

static const char *signature = "ZXST";
static const size_t signature_length = 4;

#define ZXSTDP_RAM              0x02
#define ZXSTDP_EXROMDOCK        0x04

static libspectrum_error
read_chunk( libspectrum_snap *snap, libspectrum_word version,
	    const libspectrum_byte **buffer, const libspectrum_byte *end );

typedef libspectrum_error (*read_chunk_fn)( libspectrum_snap *snap,
					    libspectrum_word version,
					    const libspectrum_byte **buffer,
					    const libspectrum_byte *end,
					    size_t data_length );

static libspectrum_error
write_file_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
	      size_t *length, int *out_flags, libspectrum_snap *snap );

static libspectrum_error
write_crtr_chunk( libspectrum_byte **bufer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_creator *creator );
static libspectrum_error
write_z80r_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_spcr_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_ram_pages( libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length, libspectrum_snap *snap, int compress );
static libspectrum_error
write_ramp_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap, int page,
		  int compress );
static libspectrum_error
write_ram_page( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length, const char *id, const libspectrum_byte *data,
		size_t data_length, int page, int compress, int extra_flags );
static libspectrum_error
write_ay_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_scld_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_b128_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_zxat_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_atrp_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap, int page,
		  int compress );
static libspectrum_error
write_zxcf_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_cfrp_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap, int page,
		  int compress );
static libspectrum_error
write_if2r_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_dock_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap, int exrom_dock,
                  const libspectrum_byte *data, int page, int writeable,
                  int compress );

static libspectrum_error
write_chunk_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		    size_t *length, const char *id,
		    libspectrum_dword block_length );

static libspectrum_error
read_ram_page( libspectrum_byte **data, size_t *page,
	       const libspectrum_byte **buffer, size_t data_length,
	       libspectrum_word *flags )
{
  size_t uncompressed_length;

  libspectrum_error error;

  if( data_length < 3 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_ram_page: length %lu too short",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  *flags = libspectrum_read_word( buffer );

  *page = **buffer; (*buffer)++;

  if( *flags & 0x01 ) {

    uncompressed_length = 0x4000;

    error = libspectrum_zlib_inflate( *buffer, data_length - 3, data,
				      &uncompressed_length );
    if( error ) return error;

    *buffer += data_length - 3;

  } else {

    if( data_length < 3 + 0x4000 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_ram_page: length %lu too short",
			       __FILE__, (unsigned long)data_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *data = malloc( 0x4000 * sizeof( libspectrum_byte ) );
    if( !( *data ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "%s:read_ram_page: out of memory at %d",
			       __FILE__, __LINE__ );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    memcpy( *data, *buffer, 0x4000 ); *buffer += 0x4000;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_atrp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;

  error = read_ram_page( &data, &page, buffer, data_length, &flags );
  if( error ) return error;

  if( page >= SNAPSHOT_ZXATASP_PAGES ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_atrp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_zxatasp_ram( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_ay_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
	       const libspectrum_byte **buffer,
	       const libspectrum_byte *end GCC_UNUSED, size_t data_length )
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
read_b128_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  libspectrum_dword flags;

  if( data_length < 10 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_b128_chunk: length %lu too short",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_beta_paged( snap, flags & 0x04 );
  libspectrum_snap_set_beta_direction( snap, !( flags & 0x20 ) );

  (*buffer)++;		/* Skip the number of drives */
  libspectrum_snap_set_beta_system( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_track ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_sector( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_data  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_status( snap, **buffer ); (*buffer)++;

  /* Skip any extra data (most likely a custom ROM) */
  *buffer += data_length - 10;

  return LIBSPECTRUM_ERROR_NONE;
}
     
static libspectrum_error
read_cfrp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;

  error = read_ram_page( &data, &page, buffer, data_length, &flags );
  if( error ) return error;

  if( page >= SNAPSHOT_ZXCF_PAGES ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_cfrp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_zxcf_ram( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_ramp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;


  error = read_ram_page( &data, &page, buffer, data_length, &flags );
  if( error ) return error;

  if( page > 15 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_ramp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_pages( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_scld_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  if( data_length != 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_scld_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_out_scld_hsr( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_out_scld_dec( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_spcr_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  libspectrum_byte out_ula;

  if( data_length != 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "szx_read_spcr_chunk: unknown length %lu", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  out_ula = **buffer & 0x07; (*buffer)++;

  libspectrum_snap_set_out_128_memoryport( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_out_plus3_memoryport( snap, **buffer ); (*buffer)++;

  if( version >= 0x0101 ) out_ula |= **buffer & 0xf8;
  (*buffer)++;

  libspectrum_snap_set_out_ula( snap, out_ula );

  *buffer += 4;			/* Skip 'reserved' data */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_z80r_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
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

  if( version >= 0x0101 ) {
    (*buffer)++;		/* Skip dwHoldIntReqCycles */
    
    /* Flags; ignore the 'last instruction EI' flag for now */
    libspectrum_snap_set_halted( snap, **buffer & 0x02 ? 1 : 0 );
    (*buffer)++;

    (*buffer)++;		/* Skip the hidden register */
    (*buffer)++;		/* Skip the reserved byte */

  } else {
    *buffer += 4;		/* Skip the reserved dword */
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_zxat_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  if( data_length != 8 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_zxat_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_zxatasp_active( snap, 1 );

  libspectrum_snap_set_zxatasp_upload( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_writeprotect( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_port_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_port_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_port_c( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_control( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_pages( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_current_page( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_zxcf_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  if( data_length != 3 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "read_zxcf_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_zxcf_active( snap, 1 );
  libspectrum_snap_set_zxcf_upload( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxcf_memctl( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxcf_pages( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_if2r_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  libspectrum_byte *buffer2;

  size_t compressed_length;
  size_t uncompressed_length;

  libspectrum_error error;

  if( data_length < 4 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "szx_read_if2r_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  compressed_length = libspectrum_read_dword( buffer );

  uncompressed_length = 0x4000;

  error = libspectrum_zlib_inflate( *buffer, data_length - 4, &buffer2,
                                    &uncompressed_length );
  if( error ) return error;

  *buffer += data_length - 4;

  libspectrum_snap_set_interface2_active( snap, 1 );

  libspectrum_snap_set_interface2_rom( snap, 0, buffer2 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_dock_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;
  libspectrum_byte writeable;

  error = read_ram_page( &data, &page, buffer, data_length, &flags );
  if( error ) return error;

  if( page > 7 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_dock_chunk: unknown page number %ld",
			     __FILE__, (unsigned long)page );
    free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_dock_active( snap, 1 );

  writeable = flags & ZXSTDP_RAM;

  if( flags & ZXSTDP_EXROMDOCK ) {
    libspectrum_snap_set_dock_ram( snap, page, writeable );
    libspectrum_snap_set_dock_cart( snap, page, data );
  } else {
    libspectrum_snap_set_exrom_ram( snap, page, writeable );
    libspectrum_snap_set_exrom_cart( snap, page, data );
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_chunk( libspectrum_snap *snap GCC_UNUSED,
	    libspectrum_word version GCC_UNUSED,
	    const libspectrum_byte **buffer,
	    const libspectrum_byte *end GCC_UNUSED,
	    size_t data_length )
{
  *buffer += data_length;
  return LIBSPECTRUM_ERROR_NONE;
}

struct read_chunk_t {

  const char *id;
  read_chunk_fn function;

};

static struct read_chunk_t read_chunks[] = {

  { "+3\0\0", skip_chunk      },
  { "AMXM",   skip_chunk      },
  { "ATRP",   read_atrp_chunk },
  { "AY\0\0", read_ay_chunk   },
  { "B128",   read_b128_chunk },
  { "CFRP",   read_cfrp_chunk },
  { "CRTR",   skip_chunk      },
  { "DOCK",   read_dock_chunk },
  { "DRUM",   skip_chunk      },
  { "DSK\0",  skip_chunk      },
  { "IF1\0",  skip_chunk      },
  { "IF2R",   read_if2r_chunk },
  { "JOY\0",  skip_chunk      },
  { "KEYB",   skip_chunk      },
  { "MDRV",   skip_chunk      },
  { "MFCE",   skip_chunk      },
  { "RAMP",   read_ramp_chunk },
  { "ROM\0",  skip_chunk      },
  { "SCLD",   read_scld_chunk },
  { "SPCR",   read_spcr_chunk },
  { "TAPE",   skip_chunk      },
  { "USPE",   skip_chunk      },
  { "Z80R",   read_z80r_chunk },
  { "ZXAT",   read_zxat_chunk },
  { "ZXCF",   read_zxcf_chunk },
  { "ZXPR",   skip_chunk      },

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
read_chunk( libspectrum_snap *snap, libspectrum_word version,
	    const libspectrum_byte **buffer, const libspectrum_byte *end )
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
      error = read_chunks[i].function( snap, version, buffer, end,
				       data_length );
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
  libspectrum_word version;

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
  buffer += signature_length;

  version = (*buffer++) << 8; version |= *buffer++;

  switch( *buffer ) {

  case SZX_MACHINE_16:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_16 );
    break;

  case SZX_MACHINE_48:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
    break;

  case SZX_MACHINE_128:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128 );
    break;

  case SZX_MACHINE_PLUS2:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2 );
    break;

  case SZX_MACHINE_PLUS2A:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2A );
    break;

  case SZX_MACHINE_PLUS3:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS3 );
    break;

  case SZX_MACHINE_PLUS3E:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS3E );
    break;

  case SZX_MACHINE_PENTAGON:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PENT );
    break;

  case SZX_MACHINE_TC2048:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_TC2048 );
    break;

  case SZX_MACHINE_TC2068:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_TC2068 );
    break;

  case SZX_MACHINE_SCORPION:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_SCORP );
    break;

  case SZX_MACHINE_SE:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_SE );
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
    error = read_chunk( snap, version, &buffer, end );
    if( error ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_szx_write( libspectrum_byte **buffer, size_t *length,
		       int *out_flags, libspectrum_snap *snap,
		       libspectrum_creator *creator, int in_flags )
{
  libspectrum_byte *ptr = *buffer;
  int capabilities, compress;
  libspectrum_error error;
  size_t i;

  *out_flags = 0;

  capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  compress = !( in_flags & LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION );

  error = write_file_header( buffer, &ptr, length, out_flags, snap );
  if( error ) return error;

  if( creator ) {
    error = write_crtr_chunk( buffer, &ptr, length, creator );
    if( error ) return error;
  }

  error = write_z80r_chunk( buffer, &ptr, length, snap );
  if( error ) return error;

  error = write_spcr_chunk( buffer, &ptr, length, snap );
  if( error ) return error;

  error = write_ram_pages( buffer, &ptr, length, snap, compress );
  if( error ) return error;

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
    error = write_ay_chunk( buffer, &ptr, length, snap );
    if( error ) return error;
  }

  if( capabilities & ( LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY |
                       LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) ) {
    error = write_scld_chunk( buffer, &ptr, length, snap );
    if( error ) return error;
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK ) {
    error = write_b128_chunk( buffer, &ptr, length, snap );
    if( error ) return error;
  }

  if( libspectrum_snap_zxatasp_active( snap ) ) {
    error = write_zxat_chunk( buffer, &ptr, length, snap );
    if( error ) return error;

    for( i = 0; i < libspectrum_snap_zxatasp_pages( snap ); i++ ) {
      error = write_atrp_chunk( buffer, &ptr, length, snap, i, compress );
      if( error ) return error;
    }
  }

  if( libspectrum_snap_zxcf_active( snap ) ) {
    error = write_zxcf_chunk( buffer, &ptr, length, snap );
    if( error ) return error;

    for( i = 0; i < libspectrum_snap_zxcf_pages( snap ); i++ ) {
      error = write_cfrp_chunk( buffer, &ptr, length, snap, i, compress );
      if( error ) return error;
    }
  }

  if( libspectrum_snap_interface2_active( snap ) ) {
#ifdef HAVE_ZLIB_H
    error = write_if2r_chunk( buffer, &ptr, length, snap );
    if( error ) return error;
#else
    /* IF2R blocks only support writing compressed images */
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
#endif                         /* #ifdef HAVE_ZLIB_H */
  }

  if( libspectrum_snap_dock_active( snap ) ) {
    for( i = 0; i < 8; i++ ) {
      if( libspectrum_snap_exrom_cart( snap, i ) ) {
        error = write_dock_chunk( buffer, &ptr, length, snap, 0,
                                  libspectrum_snap_exrom_cart( snap, i ), i,
                                  libspectrum_snap_exrom_ram( snap, i ),
                                  compress );
        if( error ) return error;
      }
      if( libspectrum_snap_dock_cart( snap, i ) ) {
        error = write_dock_chunk( buffer, &ptr, length, snap, 1,
                                  libspectrum_snap_dock_cart( snap, i ), i,
                                  libspectrum_snap_dock_ram( snap, i ),
                                  compress );
        if( error ) return error;
      }
    }
  }

  /* Set length to be actual length, not allocated length */
  *length = ptr - *buffer;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_file_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length, int *out_flags, libspectrum_snap *snap )
{
  libspectrum_error error;

  error = libspectrum_make_room( buffer, 8, ptr, length );
  if( error ) return error;

  memcpy( *ptr, signature, 4 ); *ptr += 4;
  
  /* [Assumption] We currently write version 1.2 files (major, minor) */
  *(*ptr)++ = 0x01; *(*ptr)++ = 0x02;

  switch( libspectrum_snap_machine( snap ) ) {

  case LIBSPECTRUM_MACHINE_16:     **ptr = SZX_MACHINE_16; break;
  case LIBSPECTRUM_MACHINE_48:     **ptr = SZX_MACHINE_48; break;
  case LIBSPECTRUM_MACHINE_128:    **ptr = SZX_MACHINE_128; break;
  case LIBSPECTRUM_MACHINE_PLUS2:  **ptr = SZX_MACHINE_PLUS2; break;
  case LIBSPECTRUM_MACHINE_PLUS2A: **ptr = SZX_MACHINE_PLUS2A; break;
  case LIBSPECTRUM_MACHINE_PLUS3:  **ptr = SZX_MACHINE_PLUS3; break;
  case LIBSPECTRUM_MACHINE_PLUS3E: **ptr = SZX_MACHINE_PLUS3E; break;
  case LIBSPECTRUM_MACHINE_PENT:   **ptr = SZX_MACHINE_PENTAGON; break;
  case LIBSPECTRUM_MACHINE_TC2048: **ptr = SZX_MACHINE_TC2048; break;
  case LIBSPECTRUM_MACHINE_TC2068: **ptr = SZX_MACHINE_TC2068; break;
  case LIBSPECTRUM_MACHINE_SCORP:  **ptr = SZX_MACHINE_SCORPION; break;
  case LIBSPECTRUM_MACHINE_SE: **ptr = SZX_MACHINE_SE; break;

  case LIBSPECTRUM_MACHINE_UNKNOWN:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "Emulated machine type is set to 'unknown'!" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  (*ptr)++;

  /* Reserved byte */
  *(*ptr)++ = '\0';

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_crtr_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_creator *creator )
{
  libspectrum_error error;
  size_t custom_length;

  custom_length = libspectrum_creator_custom_length( creator );

  error = write_chunk_header( buffer, ptr, length, "CRTR",
			      36 + custom_length );
  if( error ) return error;

  memcpy( *ptr, libspectrum_creator_program( creator ), 32 ); *ptr += 32;
  libspectrum_write_word( ptr, libspectrum_creator_major( creator ) );
  libspectrum_write_word( ptr, libspectrum_creator_minor( creator ) );

  if( custom_length ) {
    memcpy( *ptr, libspectrum_creator_custom( creator ), custom_length );
    *ptr += custom_length;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_z80r_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap )
{
  libspectrum_dword tstates;
  libspectrum_byte flags;
  libspectrum_error error;

  error = write_chunk_header( buffer, ptr, length, "Z80R", 37 );
  if( error ) return error;

  *(*ptr)++ = libspectrum_snap_a ( snap );
  *(*ptr)++ = libspectrum_snap_f ( snap );
  libspectrum_write_word( ptr, libspectrum_snap_bc  ( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_de  ( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_hl  ( snap ) );

  *(*ptr)++ = libspectrum_snap_a_( snap );
  *(*ptr)++ = libspectrum_snap_f_( snap );
  libspectrum_write_word( ptr, libspectrum_snap_bc_ ( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_de_ ( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_hl_ ( snap ) );

  libspectrum_write_word( ptr, libspectrum_snap_ix  ( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_iy  ( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_sp  ( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_pc  ( snap ) );

  *(*ptr)++ = libspectrum_snap_i   ( snap );
  *(*ptr)++ = libspectrum_snap_r   ( snap );
  *(*ptr)++ = libspectrum_snap_iff1( snap );
  *(*ptr)++ = libspectrum_snap_iff2( snap );
  *(*ptr)++ = libspectrum_snap_im  ( snap );

  tstates = libspectrum_snap_tstates( snap );

  libspectrum_write_dword( ptr, tstates );

  /* Number of tstates remaining in which an interrupt can occur */
  if( tstates < 48 ) {
    *(*ptr)++ = (unsigned char)(48 - tstates);
  } else {
    *(*ptr)++ = '\0';
  }

  flags = '\0';
  if( libspectrum_snap_last_instruction_ei( snap ) ) flags |= 0x01;
  if( libspectrum_snap_halted( snap ) ) flags |= 0x02;
  *(*ptr)++ = flags;

  /* Hidden register not supported */
  *(*ptr)++ = '\0';

  /* Reserved byte */
  *(*ptr)++ = '\0';

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_spcr_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap )
{
  libspectrum_error error;
  int capabilities;

  error = write_chunk_header( buffer, ptr, length, "SPCR", 8 );
  if( error ) return error;

  capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  /* Border colour */
  *(*ptr)++ = libspectrum_snap_out_ula( snap ) & 0x07;

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    *(*ptr)++ = libspectrum_snap_out_128_memoryport( snap );
  } else {
    *(*ptr)++ = '\0';
  }
  
  /* [Assumption] The Scorpion port 0x1ffd should be written here */
  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY || 
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY    ) {
    *(*ptr)++ = libspectrum_snap_out_plus3_memoryport( snap );
  } else {
    *(*ptr)++ = '\0';
  }

  *(*ptr)++ = libspectrum_snap_out_ula( snap );

  /* Reserved bytes */
  libspectrum_write_dword( ptr, 0 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_ram_pages( libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length, libspectrum_snap *snap, int compress )
{
  libspectrum_machine machine;
  int capabilities; 
  libspectrum_error error;

  machine = libspectrum_snap_machine( snap );
  capabilities = libspectrum_machine_capabilities( machine );

  error = write_ramp_chunk( buffer, ptr, length, snap, 5, compress );
  if( error ) return error;

  /* [Assumption] This is the right way to write Timex machine RAM */
  if( machine != LIBSPECTRUM_MACHINE_16 ) {
    error = write_ramp_chunk( buffer, ptr, length, snap, 2, compress );
    if( error ) return error;
    error = write_ramp_chunk( buffer, ptr, length, snap, 0, compress );
    if( error ) return error;
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    error = write_ramp_chunk( buffer, ptr, length, snap, 1, compress );
    if( error ) return error;
    error = write_ramp_chunk( buffer, ptr, length, snap, 3, compress );
    if( error ) return error;
    error = write_ramp_chunk( buffer, ptr, length, snap, 4, compress );
    if( error ) return error;
    error = write_ramp_chunk( buffer, ptr, length, snap, 6, compress );
    if( error ) return error;
    error = write_ramp_chunk( buffer, ptr, length, snap, 7, compress );
    if( error ) return error;

    /* [Assumption] RAM pages 8-15 are valid here */
    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY ) {
      int i;
      for( i = 8; i < 16; i++ ) {
        error = write_ramp_chunk( buffer, ptr, length, snap, i, compress );
        if( error ) return error;
      }
    }

  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) {
    error = write_ramp_chunk( buffer, ptr, length, snap, 8, compress );
    if( error ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_ramp_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap, int page,
		  int compress )
{
  libspectrum_error error;
  const libspectrum_byte *data;

  data = libspectrum_snap_pages( snap, page );

  error = write_ram_page( buffer, ptr, length, "RAMP", data, 0x4000, page,
			  compress, 0x00 );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_ram_page( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length, const char *id, const libspectrum_byte *data,
		size_t data_length, int page, int compress, int extra_flags )
{
  libspectrum_error error;
  libspectrum_byte *block_length, *flags, *compressed_data;
  size_t compressed_length;
  int use_compression;

  /* 8 for the chunk header, 3 for the flags and the page number */
  error = libspectrum_make_room( buffer, 8 + 3, ptr, length );
  if( error ) return error;

  memcpy( *ptr, id, 4 ); (*ptr) += 4;

  /* Store this location for later */
  block_length = *ptr; *ptr += 4;

  /* And this one */
  flags = *ptr; *ptr += 2;

  *(*ptr)++ = (libspectrum_byte)page;

  use_compression = 0;
  compressed_data = NULL;

#ifdef HAVE_ZLIB_H

  if( compress ) {

    error = libspectrum_zlib_compress( data, data_length,
				       &compressed_data, &compressed_length );
    if( error ) return error;

    if( compressed_length < data_length ) {
      use_compression = 1;
      data = compressed_data;
      data_length = compressed_length;
    }
  }

#endif				/* #ifdef HAVE_ZLIB_H */

  libspectrum_write_dword( &block_length, 3 + data_length );
  libspectrum_write_word( &flags,
                          extra_flags | (use_compression ? 0x01 : 0x00) );

  error = libspectrum_make_room( buffer, data_length, ptr, length );
  if( error ) { if( compressed_data ) free( compressed_data ); return error; }

  memcpy( *ptr, data, data_length ); *ptr += data_length;

  if( compressed_data ) free( compressed_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_ay_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length, libspectrum_snap *snap )
{
  libspectrum_error error;
  size_t i;

  error = write_chunk_header( buffer, ptr, length, "AY\0\0", 18 );
  if( error ) return error;

  *(*ptr)++ = '\0';			/* Flags */
  *(*ptr)++ = libspectrum_snap_out_ay_registerport( snap );

  for( i = 0; i < 16; i++ )
    *(*ptr)++ = libspectrum_snap_ay_registers( snap, i );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_scld_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length, libspectrum_snap *snap )
{
  libspectrum_error error;

  error = write_chunk_header( buffer, ptr, length, "SCLD", 2 );
  if( error ) return error;

  *(*ptr)++ = libspectrum_snap_out_scld_hsr( snap );
  *(*ptr)++ = libspectrum_snap_out_scld_dec( snap );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_b128_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap )
{
  libspectrum_error error;
  libspectrum_dword flags;

  error = write_chunk_header( buffer, ptr, length, "B128", 10 );
  if( error ) return error;

  flags = 0x01;		/* Betadisk interface connected */
  if( libspectrum_snap_beta_paged( snap ) ) flags |= 0x04;
  if( !libspectrum_snap_beta_direction( snap ) ) flags |= 0x20;
  libspectrum_write_dword( ptr, flags );

  *(*ptr)++ = 2;	/* 2 drives connected */
  *(*ptr)++ = libspectrum_snap_beta_system( snap );
  *(*ptr)++ = libspectrum_snap_beta_track ( snap );
  *(*ptr)++ = libspectrum_snap_beta_sector( snap );
  *(*ptr)++ = libspectrum_snap_beta_data  ( snap );
  *(*ptr)++ = libspectrum_snap_beta_status( snap );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_zxat_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap )
{
  libspectrum_error error;

  error = write_chunk_header( buffer, ptr, length, "ZXAT", 8 );

  *(*ptr)++ = libspectrum_snap_zxatasp_upload( snap );
  *(*ptr)++ = libspectrum_snap_zxatasp_writeprotect( snap );
  *(*ptr)++ = libspectrum_snap_zxatasp_port_a( snap );
  *(*ptr)++ = libspectrum_snap_zxatasp_port_b( snap );
  *(*ptr)++ = libspectrum_snap_zxatasp_port_c( snap );
  *(*ptr)++ = libspectrum_snap_zxatasp_control( snap );
  *(*ptr)++ = libspectrum_snap_zxatasp_pages( snap );
  *(*ptr)++ = libspectrum_snap_zxatasp_current_page( snap );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_atrp_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap, int page,
		  int compress )
{
  libspectrum_error error;
  const libspectrum_byte *data;

  data = libspectrum_snap_zxatasp_ram( snap, page );

  error = write_ram_page( buffer, ptr, length, "ATRP", data, 0x4000, page,
			  compress, 0x00 );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_zxcf_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap )
{
  libspectrum_error error;

  error = write_chunk_header( buffer, ptr, length, "ZXCF", 3 );

  *(*ptr)++ = libspectrum_snap_zxcf_upload( snap );
  *(*ptr)++ = libspectrum_snap_zxcf_memctl( snap );
  *(*ptr)++ = libspectrum_snap_zxcf_pages( snap );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_cfrp_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap, int page,
		  int compress )
{
  libspectrum_error error;
  const libspectrum_byte *data;

  data = libspectrum_snap_zxcf_ram( snap, page );

  error = write_ram_page( buffer, ptr, length, "CFRP", data, 0x4000, page,
			  compress, 0x00 );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

#ifdef HAVE_ZLIB_H

static libspectrum_error
write_if2r_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length, libspectrum_snap *snap )
{
  libspectrum_error error;
  libspectrum_byte *block_length, *data, *cart_size, *compressed_data;
  size_t data_length, compressed_length;

  /* 8 for the chunk header, 4 for the compressed cart size */
  error = libspectrum_make_room( buffer, 8 + 4, ptr, length );
  if( error ) return error;

  memcpy( *ptr, "IF2R", 4 ); (*ptr) += 4;

  /* Store this location for later */
  block_length = *ptr; *ptr += 4;

  /* And this one */
  cart_size = *ptr; *ptr += 4;

  data = libspectrum_snap_interface2_rom( snap, 0 ); data_length = 0x4000;
  compressed_data = NULL;

  error = libspectrum_zlib_compress( data, data_length,
                                     &compressed_data, &compressed_length );
  if( error ) return error;

  libspectrum_write_dword( &block_length, 4 + compressed_length );
  libspectrum_write_dword( &cart_size, compressed_length );

  error = libspectrum_make_room( buffer, compressed_length, ptr, length );
  if( error ) { if( compressed_data ) free( compressed_data ); return error; }

  memcpy( *ptr, compressed_data, compressed_length ); *ptr += compressed_length;

  if( compressed_data ) free( compressed_data );

  return LIBSPECTRUM_ERROR_NONE;
}

#endif                         /* #ifdef HAVE_ZLIB_H */

static libspectrum_error
write_dock_chunk( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length, libspectrum_snap *snap, int exrom_dock,
                  const libspectrum_byte *data, int page, int writeable,
                  int compress )
{
  libspectrum_error error;
  libspectrum_byte extra_flags;

  extra_flags  = (writeable ? ZXSTDP_RAM : 0x00);
  extra_flags |= (exrom_dock ? ZXSTDP_EXROMDOCK : 0x00);

  error = write_ram_page( buffer, ptr, length, "DOCK", data, 0x2000, page,
			  compress, extra_flags );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_chunk_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		    size_t *length, const char *id,
		    libspectrum_dword block_length )
{
  libspectrum_error error;

  error = libspectrum_make_room( buffer, 8 + block_length, ptr, length );
  if( error ) return error;

  memcpy( *ptr, id, 4 ); *ptr += 4;
  libspectrum_write_dword( ptr, block_length );

  return LIBSPECTRUM_ERROR_NONE;
}
