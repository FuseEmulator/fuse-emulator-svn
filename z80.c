/* z80.c: Routines for handling .z80 snapshots
   Copyright (c) 2001-2004 Philip Kendall, Darren Salt, Fredrick Meunier

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "internals.h"

/* Length of the basic .z80 headers */
static const int LIBSPECTRUM_Z80_HEADER_LENGTH = 30;

/* Length of the v2 extensions */
#define LIBSPECTRUM_Z80_V2_LENGTH 23

/* Length of the v3 extensions */
#define LIBSPECTRUM_Z80_V3_LENGTH 54

/* Length of xzx's extensions */
#define LIBSPECTRUM_Z80_V3X_LENGTH 55

/* The constants used for each machine type */
enum {

  /* v1 constants */
  Z80_JOYSTICK_CURSOR_V1 = 0,
  Z80_JOYSTICK_KEMPSTON_V1 = 1,
  Z80_JOYSTICK_SINCLAIR_LEFT_V1 = 2,
  Z80_JOYSTICK_SINCLAIR_RIGHT_V1 = 3,

  /* v2 constants */
  Z80_MACHINE_48_V2 = 0,
  Z80_MACHINE_48_IF1_V2 = 1,
  Z80_MACHINE_48_SAMRAM_V2 = 2,
  Z80_MACHINE_128_V2 = 3,
  Z80_MACHINE_128_IF1_V2 = 4,

  /* v3 constants */
  Z80_MACHINE_48 = 0,
  Z80_MACHINE_48_IF1 = 1,
  Z80_MACHINE_48_SAMRAM = 2,
  Z80_MACHINE_48_MGT = 3,
  Z80_MACHINE_128 = 4,
  Z80_MACHINE_128_IF1 = 5,
  Z80_MACHINE_128_MGT = 6,

  Z80_JOYSTICK_CURSOR_V3 = 0,
  Z80_JOYSTICK_KEMPSTON_V3 = 1,
  Z80_JOYSTICK_USER_DEFINED_V3 = 2,
  Z80_JOYSTICK_SINCLAIR_RIGHT_V3 = 3,

  /* Extensions */
  Z80_MACHINE_PLUS3 = 7,
  Z80_MACHINE_PLUS3_XZX_ERROR = 8,
  Z80_MACHINE_PENTAGON = 9,
  Z80_MACHINE_SCORPION = 10,
  Z80_MACHINE_PLUS2 = 12,
  Z80_MACHINE_PLUS2A = 13,
  Z80_MACHINE_TC2048 = 14,
  Z80_MACHINE_TC2068 = 15,
  Z80_MACHINE_TS2068 = 128,

  /* The first extension ID; anything here or greater applies to both
     v2 and v3 files */
  Z80_MACHINE_FIRST_EXTENSION = Z80_MACHINE_PLUS3,
};

/* Constants representing a Sinclair Interface II port 1 joystick as a .z80
   user defined joystick */
static const libspectrum_word if2_left_map_l = 0x0f03;
static const libspectrum_word if2_left_map_r = 0x0803;
static const libspectrum_word if2_left_map_d = 0x0403;
static const libspectrum_word if2_left_map_u = 0x0203;
static const libspectrum_word if2_left_map_f = 0x0103;
static const libspectrum_word if2_left_l     = '1';
static const libspectrum_word if2_left_r     = '2';
static const libspectrum_word if2_left_d     = '3';
static const libspectrum_word if2_left_u     = '4';
static const libspectrum_word if2_left_f     = '5';

/* The signature used to designate the .slt extensions */
static libspectrum_byte slt_signature[] = "\0\0\0SLT";
static size_t slt_signature_length = 6;

static libspectrum_error
read_header( const libspectrum_byte *buffer, libspectrum_snap *snap,
	     const libspectrum_byte **data, int *version, int *compressed );
static libspectrum_error
get_machine_type( libspectrum_snap *snap, libspectrum_byte type, int version );
static libspectrum_error
get_machine_type_v2( libspectrum_snap *snap, libspectrum_byte type );
static libspectrum_error
get_machine_type_v3( libspectrum_snap *snap, libspectrum_byte type );
static libspectrum_error
get_machine_type_extension( libspectrum_snap *snap, libspectrum_byte type );
static libspectrum_error
get_joystick_type( libspectrum_snap *snap, 
                   const libspectrum_byte *custom_joystick_base,
                   libspectrum_byte type, int version );
static libspectrum_error
get_joystick_type_v1( libspectrum_snap *snap, libspectrum_byte type );
static libspectrum_error
get_joystick_type_v3( libspectrum_snap *snap,
                      const libspectrum_byte *custom_joystick_base,
                      libspectrum_byte type );
static libspectrum_error
read_blocks( const libspectrum_byte *buffer, size_t buffer_length,
	     libspectrum_snap *snap, int version, int compressed );
static libspectrum_error
read_slt( libspectrum_snap *snap, const libspectrum_byte **next_block,
	  const libspectrum_byte *end );
static libspectrum_error
read_block( const libspectrum_byte *buffer, libspectrum_snap *snap,
	    const libspectrum_byte **next_block, const libspectrum_byte *end,
	    int version, int compressed );
static libspectrum_error
read_v1_block( const libspectrum_byte *buffer, int is_compressed,
	       libspectrum_byte **uncompressed,
	       const libspectrum_byte **next_block,
	       const libspectrum_byte *end );
static libspectrum_error
read_v2_block( const libspectrum_byte *buffer, libspectrum_byte **block,
	       size_t *length, int *page, const libspectrum_byte **next_block,
	       const libspectrum_byte *end );

static libspectrum_error
write_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
	      size_t *length, int *flags, libspectrum_snap *snap );
static libspectrum_error
write_base_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length, int *flags, libspectrum_snap *snap );
static libspectrum_error
write_extended_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length, int *flags, libspectrum_snap *snap );
static libspectrum_error
write_pages( libspectrum_byte **buffer, libspectrum_byte **ptr,
	     size_t *length, libspectrum_snap *snap, int compress );
static libspectrum_error
write_page( libspectrum_byte **buffer, libspectrum_byte **ptr,
	    size_t *length, int page_num, libspectrum_byte *page,
	    int compress );
static libspectrum_error
write_slt( libspectrum_byte **buffer, libspectrum_byte **ptr,
	   size_t *length, libspectrum_snap *snap );
static libspectrum_error
write_slt_entry( libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length, libspectrum_word type, libspectrum_word id,
		 libspectrum_dword slt_length );

static libspectrum_error
compress_block( libspectrum_byte **dest, size_t *dest_length,
		const libspectrum_byte *src, size_t src_length);
static libspectrum_error
uncompress_block( libspectrum_byte **dest, size_t *dest_length,
		  const libspectrum_byte *src, size_t src_length);

/* The various things which can appear in the .slt data */
enum slt_type {

  LIBSPECTRUM_SLT_TYPE_END = 0,
  LIBSPECTRUM_SLT_TYPE_LEVEL,
  LIBSPECTRUM_SLT_TYPE_INSTRUCTIONS,
  LIBSPECTRUM_SLT_TYPE_SCREEN,
  LIBSPECTRUM_SLT_TYPE_PICTURE,
  LIBSPECTRUM_SLT_TYPE_POKE,

};

libspectrum_error
libspectrum_z80_read( libspectrum_snap *snap,
	              const libspectrum_byte *buffer, size_t buffer_length )
{
  return internal_z80_read( snap, buffer, buffer_length );
}

libspectrum_error
internal_z80_read( libspectrum_snap *snap,
		   const libspectrum_byte *buffer, size_t buffer_length )
{
  libspectrum_error error;
  const libspectrum_byte *data;
  int version, compressed = 1;

  error = read_header( buffer, snap, &data, &version, &compressed );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = read_blocks( data, buffer_length - (data - buffer), snap,
		       version, compressed );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  libspectrum_snap_set_beta_paged( snap, 0 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_header( const libspectrum_byte *buffer, libspectrum_snap *snap,
	     const libspectrum_byte **data, int *version, int *compressed )
{
  const libspectrum_byte *header = buffer;
  int capabilities;
  size_t i;
  libspectrum_error error;

  libspectrum_snap_set_a  ( snap, header[ 0] );
  libspectrum_snap_set_f  ( snap, header[ 1] );
  libspectrum_snap_set_bc ( snap, header[ 2] + header[ 3]*0x100 );
  libspectrum_snap_set_de ( snap, header[13] + header[14]*0x100 );
  libspectrum_snap_set_hl ( snap, header[ 4] + header[ 5]*0x100 );
  libspectrum_snap_set_a_ ( snap, header[21] );
  libspectrum_snap_set_f_ ( snap, header[22] );
  libspectrum_snap_set_bc_( snap, header[15] + header[16]*0x100 );
  libspectrum_snap_set_de_( snap, header[17] + header[18]*0x100 );
  libspectrum_snap_set_hl_( snap, header[19] + header[20]*0x100 );
  libspectrum_snap_set_ix ( snap, header[25] + header[26]*0x100 );
  libspectrum_snap_set_iy ( snap, header[23] + header[24]*0x100 );
  libspectrum_snap_set_i  ( snap, header[10] );
  libspectrum_snap_set_r  ( snap, (   header[11] & 0x7f ) +
			          ( ( header[12] & 0x01 ) << 7 ) );
  libspectrum_snap_set_pc ( snap, header[ 6] + header[ 7]*0x100 );
  libspectrum_snap_set_sp ( snap, header[ 8] + header[ 9]*0x100 );

  libspectrum_snap_set_iff1( snap, header[27] ? 1 : 0 );
  libspectrum_snap_set_iff2( snap, header[28] ? 1 : 0 );
  libspectrum_snap_set_im( snap, header[29] & 0x03 );
  libspectrum_snap_set_issue2( snap, header[29] & 0x04 );

  libspectrum_snap_set_out_ula( snap, (header[12]&0x0e) >> 1 );

  if( libspectrum_snap_pc( snap ) == 0 ) { /* PC == 0x0000 => v2 or greater */

    size_t extra_length;
    const libspectrum_byte *extra_header;
    libspectrum_dword quarter_tstates;

    extra_length = header[ LIBSPECTRUM_Z80_HEADER_LENGTH     ] +
                   header[ LIBSPECTRUM_Z80_HEADER_LENGTH + 1 ] * 0x100;

    switch( extra_length ) {
    case LIBSPECTRUM_Z80_V2_LENGTH:
      *version = 2;
      break;
    case LIBSPECTRUM_Z80_V3_LENGTH:
    case LIBSPECTRUM_Z80_V3X_LENGTH:
      *version = 3;
      break;
    default:
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
        "libspectrum_read_z80_header: unknown header length %d",
	(int)extra_length
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
      
    }

    extra_header = buffer + LIBSPECTRUM_Z80_HEADER_LENGTH + 2;

    error = get_joystick_type( snap,
                               extra_header + 32,
                               ( header[29] & 0xc0 ) >> 6,
                               *version
                             );
    if( error ) return error;

    libspectrum_snap_set_pc( snap, extra_header[0] + extra_header[1] * 0x100 );

    error = get_machine_type( snap, extra_header[2], *version );
    if( error ) return error;

    if( *version >= 3 ) {

      quarter_tstates =
	libspectrum_timings_tstates_per_frame(
	  libspectrum_snap_machine( snap ) 
	) / 4;
      
      /* This is correct, even if it does disagree with Z80 v3.05's
	 `z80dump'; thanks to Pedro Gimeno for pointing this out when
	 this code was part of SnapConv */
      libspectrum_snap_set_tstates( snap,
        ( ( ( extra_header[25] + 1 ) % 4 ) + 1 ) * quarter_tstates -
	( extra_header[23] + extra_header[24] * 0x100 + 1 )
      );
    
      /* Stop broken files from causing trouble... */
      if( libspectrum_snap_tstates( snap ) >= quarter_tstates * 4 )
	libspectrum_snap_set_tstates( snap, 0 );
    }

    /* I don't like this any more than you do but here is support for the
       Spectaculator extensions:

       Support for 16k/+2/+2A snapshots

       If bit 7 of byte 37 is set, this modifies the value of the hardware
       field (for both v2 and v3 snapshots) such that:
         a.. Any valid 48k identifier should be taken as 16k
         b.. Any valid 128k identifier should be taken as +2
         c.. Any valid +3 identifier (7 or 8) should be taken as +2A

       Support for Fuller Box / AY sound in 48k mode (not implemented)
       
       Spectaculator recognises xzx's extension of setting bit 2 of byte 37 to
       specify AY sound in 48k mode. In addition, if this and also bit 6 is
       set, this specifies Fuller Box emulation.
    */

    if( extra_header[5] & 0x80 ) {

      switch( libspectrum_snap_machine( snap ) ) {

      case LIBSPECTRUM_MACHINE_48:
	libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_16 ); break;
      case LIBSPECTRUM_MACHINE_128:
	libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2 ); break;
      case LIBSPECTRUM_MACHINE_PLUS3:
	libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2A );
	break;
      default:
	break;			/* Do nothing */

      }
    }

    capabilities =
      libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
      libspectrum_snap_set_out_128_memoryport( snap, extra_header[3] );
    } else if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ) {
      libspectrum_snap_set_out_scld_hsr( snap, extra_header[3] );
    }

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO ) {
      libspectrum_snap_set_out_scld_dec( snap, extra_header[4] );
    }

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
      libspectrum_snap_set_out_ay_registerport( snap, extra_header[ 6] );
      for( i = 0; i < 16; i++ ) {
	libspectrum_snap_set_ay_registers( snap, i, extra_header[ 7 + i ] );
      }
    }

    if( extra_length == LIBSPECTRUM_Z80_V3X_LENGTH ) {
      if( ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) ||
	  ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY )    ) {
	libspectrum_snap_set_out_plus3_memoryport( snap, extra_header[54] );
      }
    }

    (*data) = buffer + LIBSPECTRUM_Z80_HEADER_LENGTH + 2 + extra_length;

  } else {	/* v1 .z80 file */

    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
    *version = 1;

    libspectrum_snap_set_joystick_active_count( snap, 1 );
    error = get_joystick_type_v1( snap, ( header[29] & 0xc0 ) >> 6 );
    if( error ) return error;

    /* Need to flag this for later */
    *compressed = ( header[12] & 0x20 ) ? 1 : 0;

    (*data) = buffer + LIBSPECTRUM_Z80_HEADER_LENGTH;

  }

  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
get_machine_type( libspectrum_snap *snap, libspectrum_byte type, int version )
{
  libspectrum_error error;

  if( type < Z80_MACHINE_FIRST_EXTENSION ) {

    switch( version ) {

    case 2:
      error = get_machine_type_v2( snap, type ); if( error ) return error;
      break;

    case 3:
      error = get_machine_type_v3( snap, type ); if( error ) return error;
      break;

    default:
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			       "%s:get_machine_type: unknown version %d",
			       __FILE__, version );
      return LIBSPECTRUM_ERROR_LOGIC;
    }

  } else {
    error = get_machine_type_extension( snap, type ); if( error ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_machine_type_v2( libspectrum_snap *snap, libspectrum_byte type )
{
  switch( type ) {

  case Z80_MACHINE_48_V2:
  case Z80_MACHINE_48_IF1_V2:
  case Z80_MACHINE_48_SAMRAM_V2:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 ); break;
  case Z80_MACHINE_128_V2:
  case Z80_MACHINE_128_IF1_V2:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128 ); break;
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:get_machine_type: unknown v2 machine type %d",
			     __FILE__, type );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_machine_type_v3( libspectrum_snap *snap, libspectrum_byte type )
{
  switch( type ) {
    
  case Z80_MACHINE_48:
  case Z80_MACHINE_48_IF1:
  case Z80_MACHINE_48_SAMRAM:
  case Z80_MACHINE_48_MGT:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 ); break;
  case Z80_MACHINE_128:
  case Z80_MACHINE_128_IF1:
  case Z80_MACHINE_128_MGT:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128 ); break;
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:get_machine_type: unknown v3 machine type %d",
			     __FILE__, type );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_machine_type_extension( libspectrum_snap *snap, libspectrum_byte type )
{
  switch( type ) {

  case Z80_MACHINE_PLUS3:
  case Z80_MACHINE_PLUS3_XZX_ERROR:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS3 ); break;
  case Z80_MACHINE_PENTAGON:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PENT ); break;
  case Z80_MACHINE_SCORPION:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_SCORP ); break;
  case Z80_MACHINE_PLUS2:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2 ); break;
  case Z80_MACHINE_PLUS2A:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2A ); break;
  case Z80_MACHINE_TC2048:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_TC2048 ); break;
  case Z80_MACHINE_TC2068:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_TC2068 ); break;
  case Z80_MACHINE_TS2068:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_TS2068 ); break;
  default:
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:get_machine_type: unknown extension machine type %d", __FILE__, type
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_joystick_type( libspectrum_snap *snap, 
                   const libspectrum_byte *custom_joystick_base,
                   libspectrum_byte type, int version )
{
  libspectrum_error error;

  libspectrum_snap_set_joystick_active_count( snap, 1 );
  libspectrum_snap_set_joystick_inputs( snap, 0,
                                        LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD |
                                        LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );

  switch( version ) {

  case 1:
  case 2:
    error = get_joystick_type_v1( snap, type ); if( error ) return error;
    break;
  case 3:
    error = get_joystick_type_v3( snap, custom_joystick_base, type );
    if( error ) return error;
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                             "%s:get_joystick_type: unknown version %d",
                             __FILE__, version );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_joystick_type_v1( libspectrum_snap *snap, libspectrum_byte type )
{
  switch( type ) {

  case Z80_JOYSTICK_CURSOR_V1:
    libspectrum_snap_set_joystick_list( snap, 0, LIBSPECTRUM_JOYSTICK_CURSOR );
    break;
  case Z80_JOYSTICK_KEMPSTON_V1:
    libspectrum_snap_set_joystick_list( snap, 0, LIBSPECTRUM_JOYSTICK_KEMPSTON );
    break;
  case Z80_JOYSTICK_SINCLAIR_LEFT_V1:
    libspectrum_snap_set_joystick_list( snap, 0, LIBSPECTRUM_JOYSTICK_SINCLAIR_2 );
    break;
  case Z80_JOYSTICK_SINCLAIR_RIGHT_V1:
    libspectrum_snap_set_joystick_list( snap, 0, LIBSPECTRUM_JOYSTICK_SINCLAIR_1 );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:get_machine_type: unknown v1 joystick type %d",
			     __FILE__, type );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_joystick_type_v3( libspectrum_snap *snap,
                      const libspectrum_byte *custom_keys_base,
                      libspectrum_byte type )
{
  switch( type ) {

  case Z80_JOYSTICK_CURSOR_V3:
    libspectrum_snap_set_joystick_list( snap, 0, LIBSPECTRUM_JOYSTICK_CURSOR );
    break;
  case Z80_JOYSTICK_KEMPSTON_V3:
    libspectrum_snap_set_joystick_list( snap, 0, LIBSPECTRUM_JOYSTICK_KEMPSTON );
    break;
  case Z80_JOYSTICK_USER_DEFINED_V3:
    /* User defined mapping, check to see if keys match Sinclair Interface 2
       left port and return that if possible */
    /* First 10 bytes are spectrum keyboard mappings for user defined keys
       in this order: left, right, down, up, fire */
    if( custom_keys_base[0] + custom_keys_base[1] * 0x100 == if2_left_map_l &&
        custom_keys_base[2] + custom_keys_base[3] * 0x100 == if2_left_map_r &&
        custom_keys_base[4] + custom_keys_base[5] * 0x100 == if2_left_map_d &&
        custom_keys_base[6] + custom_keys_base[7] * 0x100 == if2_left_map_u &&
        custom_keys_base[8] + custom_keys_base[9] * 0x100 == if2_left_map_f ) {
      libspectrum_snap_set_joystick_list( snap, 0,
                                          LIBSPECTRUM_JOYSTICK_SINCLAIR_2 );
    } else {
      libspectrum_snap_set_joystick_active_count( snap, 0 );
      libspectrum_snap_set_joystick_inputs( snap, 0, 0 );
    }
    break;
  case Z80_JOYSTICK_SINCLAIR_RIGHT_V3:
    libspectrum_snap_set_joystick_list( snap, 0,
                                        LIBSPECTRUM_JOYSTICK_SINCLAIR_1 );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:get_machine_type: unknown v3 joystick type %d",
			     __FILE__, type );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_blocks( const libspectrum_byte *buffer, size_t buffer_length,
	     libspectrum_snap *snap, int version, int compressed )
{
  const libspectrum_byte *end, *next_block;

  end = buffer + buffer_length; next_block = buffer;

  while( next_block < end ) {

    libspectrum_error error;

    error = read_block( next_block, snap, &next_block, end, version,
			compressed );

    /* If it looks like some .slt data, try and parse that. That should
       then be the end of the file */
    if( error == LIBSPECTRUM_ERROR_SLT ) {
      error = read_slt( snap, &next_block, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      
      /* If we haven't reached the end, return with error */
      if( next_block != end ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
				 "read_blocks: .slt data does not end file" );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      /* If we have reached the end, that's OK */
      return LIBSPECTRUM_ERROR_NONE;
    }

    /* Return immediately with any other errors */
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_slt( libspectrum_snap *snap, const libspectrum_byte **next_block,
	  const libspectrum_byte *end )
{
  size_t slt_length[256], offsets[256];
  size_t whence = 0;

  size_t screen_length = 0, screen_offset = 0;

  int i;
  libspectrum_error error;

  /* Zero all lengths to imply `not present' */
  for( i=0; i<256; i++ ) slt_length[i]=0;

  while( 1 ) {

    int type, level, length;

    /* Check we've got enough data left */
    if( *next_block + 8 > end ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "read_slt: out of data in directory" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    type =   (*next_block)[0] + (*next_block)[1] * 0x100;
    level =  (*next_block)[2] + (*next_block)[3] * 0x100;
    (*next_block) += 4;
    length = libspectrum_read_dword( next_block );

    /* If this ends the table, exit */
    if( type == LIBSPECTRUM_SLT_TYPE_END ) break;

    /* Reset the pointer back to the start of the block */
    (*next_block) -= 8;

    switch( type ) {

    case LIBSPECTRUM_SLT_TYPE_LEVEL:	/* Level data */

      if( level >= 0x100 ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
				 "read_slt: unexpected level number %d",
				 level );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      /* Each level should appear once only */
      if( slt_length[ level ] ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
				 "read_slt: level %d is duplicated", level );
        return LIBSPECTRUM_ERROR_CORRUPT;
      }

      offsets[ level ] = whence; slt_length[ level ] = length;

      break;

    case LIBSPECTRUM_SLT_TYPE_SCREEN:	/* Loading screen */

      /* Allow only one loading screen per .slt file */
      if( screen_length != 0 ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
				 "read_slt: duplicated loading screen" );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      libspectrum_snap_set_slt_screen_level( snap, level );
      screen_length = length; screen_offset = whence;

      break;

    default:

      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "read_slt: unknown data type %d", type );
      return LIBSPECTRUM_ERROR_UNKNOWN;

    }

    /* and move both pointers along to the next block */
    *next_block += 8; whence += length;

  }

  /* Read in the data for each level */
  for( i=0; i<256; i++ ) {

    libspectrum_byte *buffer; size_t length;

    if( slt_length[i] ) {

      /* Check this data actually exists */
      if( *next_block + offsets[i] + slt_length[i] > end ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
				 "read_slt: out of data reading level %d", i );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      length = 0;	/* Tell uncompress_block to allocate memory for us */
      error =
	uncompress_block( &buffer, &length,
			  *next_block + offsets[i], slt_length[i] );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;

      libspectrum_snap_set_slt( snap, i, buffer );
      libspectrum_snap_set_slt_length( snap, i, length );

    }
  }

  /* And the screen data */
  if( screen_length ) {

    libspectrum_byte *buffer;

    /* Should expand to 6912 bytes, so give me a buffer that long */
    buffer = malloc( 6912 * sizeof( libspectrum_byte ) );
    if( !buffer ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "read_slt: out of memory" );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    if( screen_length == 6912 ) {	/* Not compressed */

      memcpy( buffer, (*next_block) + screen_offset, 6912 );

    } else {				/* Compressed */
      
      error = uncompress_block( &buffer, &screen_length,
				(*next_block) + screen_offset, screen_length );

      /* A screen should be 6912 bytes long */
      if( screen_length != 6912 ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
				 "read_slt: screen is not 6912 bytes long" );
	free( buffer );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }
    }

    libspectrum_snap_set_slt_screen( snap, buffer );
  }

  /* Move past the data */
  *next_block += whence;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_block( const libspectrum_byte *buffer, libspectrum_snap *snap,
	    const libspectrum_byte **next_block, const libspectrum_byte *end,
	    int version, int compressed )
{
  libspectrum_error error;
  libspectrum_byte *uncompressed;

  int capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );
  
  if( version == 1 ) {

    error = read_v1_block( buffer, compressed, &uncompressed, next_block,
			   end );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    libspectrum_split_to_48k_pages( snap, uncompressed );

    free( uncompressed );

  } else {

    size_t length;
    int page;

    error = read_v2_block( buffer, &uncompressed, &length, &page, next_block,
			   end );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    if( page <= 0 || page > 18 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "read_block: unknown page %d", page );
      free( uncompressed );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    /* If it's a ROM page, just throw it away */
    if( page < 3 ) {
      free( uncompressed );
      return LIBSPECTRUM_ERROR_NONE;
    }

    /* Page 11 is the Multiface ROM unless we're emulating something
       Scorpion-like */
    if( page == 11 &&
	!( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY ) ) {
      free( uncompressed );
      return LIBSPECTRUM_ERROR_NONE;
    }

    /* Deal with 48K snaps -- first, throw away page 3, as it's a ROM.
       Then remap the numbers slightly */
    if( !( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) ) {

      switch( page ) {

      case 3:
	free( uncompressed );
	return LIBSPECTRUM_ERROR_NONE;
      case 4:
	page=5;	break;
      case 5:
	page=3;	break;

      }
    }

    /* Now map onto RAM page numbers */
    page -= 3;

    if( libspectrum_snap_pages( snap, page ) == NULL ) {
      libspectrum_snap_set_pages( snap, page, uncompressed );
    } else {
      free( uncompressed );
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "read_block: page %d duplicated", page );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

  }    

  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
read_v1_block( const libspectrum_byte *buffer, int is_compressed, 
	       libspectrum_byte **uncompressed,
	       const libspectrum_byte **next_block,
	       const libspectrum_byte *end )
{
  if( is_compressed ) {
    
    const libspectrum_byte *ptr;
    int state,error;
    size_t uncompressed_length = 0;

    state = 0; ptr = buffer;

    while( state != 4 ) {

      if( ptr == end ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
				 "read_v1_block: end marker not found" );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      switch( state ) {
      case 0:
	switch( *ptr++ ) {
	case 0x00: state = 1; break;
	  default: state = 0; break;
	}
	break;
      case 1:
	switch( *ptr++ ) {
	case 0x00: state = 1; break;
	case 0xed: state = 2; break;
	  default: state = 0; break;
	}
	break;
      case 2:
	switch( *ptr++ ) {
	case 0x00: state = 1; break;
	case 0xed: state = 3; break;
	  default: state = 0; break;
	}
	break;
      case 3:
	switch( *ptr++ ) {
	case 0x00: state = 4; break;
	  default: state = 0; break;
	}
	break;
      default:
	libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
				 "read_v1_block: unknown state %d", state );
	return LIBSPECTRUM_ERROR_LOGIC;
      }

    }

    /* Length passed here is reduced by 4 to remove the end marker */
    error = uncompress_block( uncompressed, &uncompressed_length, buffer,
			      ( ptr - buffer - 4 ) );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    /* Uncompressed data must be exactly 48Kb long */
    if( uncompressed_length != 0xc000 ) {
      free( (*uncompressed) );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "read_v1_block: data does not uncompress to 48Kb"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    (*next_block) = ptr;

  } else {	/* Snap isn't compressed */

    /* Check we've got enough bytes to read */
    if( end - (*next_block) < 0xc000 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "read_v1_block: not enough data in buffer" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    (*uncompressed) = (libspectrum_byte*)malloc( 0xc000 * sizeof(libspectrum_byte) );
    if( (*uncompressed) == NULL ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "read_v1_block: out of memory" );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    memcpy( (*uncompressed), buffer, 0xc000 );

    (*next_block) = buffer + 0xc000;

  }

  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
read_v2_block( const libspectrum_byte *buffer, libspectrum_byte **block,
	       size_t *length, int *page, const libspectrum_byte **next_block,
	       const libspectrum_byte *end )
{
  size_t length2;

  length2 = buffer[0] + buffer[1] * 0x100;
  (*page) = buffer[2];

  if (length2 == 0 && *page == 0) {
    if (buffer + 8 < end
	&& !memcmp( buffer, slt_signature, slt_signature_length ) )
    {
      /* Ah, we have what looks like SLT data... */
      *next_block = buffer + 6;
      return LIBSPECTRUM_ERROR_SLT;
    }
  }

  /* A length of 0xffff => 16384 bytes of uncompressed data */ 
  if( length2 != 0xffff ) {

    libspectrum_error error;

    /* Check we're not going to run over the end of the buffer */
    if( buffer + 3 + length2 > end ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "read_v2_block: not enough data in buffer" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    (*length)=0;
    error = uncompress_block( block, length, buffer+3, length2 );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    (*next_block) = buffer + 3 + length2;

  } else { /* Uncompressed block */

    /* Check we're not going to run over the end of the buffer */
    if( buffer + 3 + 0x4000 > end ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "read_v2_block: not enough data in buffer" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    (*block) = (libspectrum_byte*)malloc( 0x4000 * sizeof(libspectrum_byte) );
    if( (*block) == NULL ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "read_v2_block: out of memory" );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    memcpy( (*block), buffer+3, 0x4000 );

    (*length) = 0x4000;
    (*next_block) = buffer + 3 + 0x4000;

  }

  return LIBSPECTRUM_ERROR_NONE;

}

/* DEPRECATED: use libspectrum_snap_write() instead */
libspectrum_error
libspectrum_z80_write( libspectrum_byte **buffer, size_t *length,
		       libspectrum_snap *snap )
{
  int out_flags;

  return libspectrum_z80_write2( buffer, length, &out_flags, snap, 0 );
}

libspectrum_error
libspectrum_z80_write2( libspectrum_byte **buffer, size_t *length,
			int *out_flags, libspectrum_snap *snap, int in_flags )
{
  libspectrum_error error;
  libspectrum_byte *ptr = *buffer;

  *out_flags = 0;

  /* .z80 format doesn't store the 'last instruction EI' or 'halted' state */
  if( libspectrum_snap_last_instruction_ei( snap ) ) 
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
  if( libspectrum_snap_halted( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;

  /* .z80 format doesn't store the ZXATASP or ZXCF info */
  if( libspectrum_snap_zxatasp_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
  if( libspectrum_snap_zxcf_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* .z80 format may be able to store an IF2 ROM instead of the 48K ROM but
     not for now */
  if( libspectrum_snap_interface2_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* .z80 format doesn't store the Timex Dock cart */
  if( libspectrum_snap_dock_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  error = write_header( buffer, &ptr, length, out_flags, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = write_pages(
    buffer, &ptr, length, snap,
    !( in_flags & LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION) +
    ( in_flags & LIBSPECTRUM_FLAG_SNAPSHOT_ALWAYS_COMPRESS ));
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  (*length) = ptr - *buffer;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
	      size_t *length, int *flags, libspectrum_snap *snap )
{
  libspectrum_error error;

  error = write_base_header( buffer, ptr, length, flags, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = write_extended_header( buffer, ptr, length, flags, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_base_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length, int *flags, libspectrum_snap *snap )
{
  libspectrum_error error;
  libspectrum_byte joystick_flags = 0;

  error = libspectrum_make_room( buffer, LIBSPECTRUM_Z80_HEADER_LENGTH, ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = libspectrum_snap_a( snap ); 
  *(*ptr)++ = libspectrum_snap_f( snap );
  libspectrum_write_word( ptr, libspectrum_snap_bc( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_hl( snap ) );

  libspectrum_write_word( ptr, 0 ); /* PC; zero denotes >= v2 */

  libspectrum_write_word( ptr, libspectrum_snap_sp( snap ) );
  *(*ptr)++ = libspectrum_snap_i( snap );
  *(*ptr)++ = ( libspectrum_snap_r( snap ) & 0x7f );

  *(*ptr)++ = ( libspectrum_snap_r( snap ) >> 7 ) +
              ( ( libspectrum_snap_out_ula( snap ) & 0x07 ) << 1 );

  libspectrum_write_word( ptr, libspectrum_snap_de( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_bc_( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_de_( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_hl_( snap ) );
  *(*ptr)++ = libspectrum_snap_a_( snap );
  *(*ptr)++ = libspectrum_snap_f_( snap );
  libspectrum_write_word( ptr, libspectrum_snap_iy( snap ) );
  libspectrum_write_word( ptr, libspectrum_snap_ix( snap ) );

  *(*ptr)++ = libspectrum_snap_iff1( snap ) ? 0xff : 0x00;
  *(*ptr)++ = libspectrum_snap_iff2( snap ) ? 0xff : 0x00;

  /* Exactly one active joystick can be represented in a .z80 file */
  if( libspectrum_snap_joystick_active_count( snap ) != 1 )
    *flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;

  switch( libspectrum_snap_joystick_list( snap, 1 ) ) {
  case LIBSPECTRUM_JOYSTICK_CURSOR:
    joystick_flags = Z80_JOYSTICK_CURSOR_V3;
    break;
  case LIBSPECTRUM_JOYSTICK_KEMPSTON:
    joystick_flags = Z80_JOYSTICK_KEMPSTON_V3;
    break;
  case LIBSPECTRUM_JOYSTICK_SINCLAIR_1:
    joystick_flags = Z80_JOYSTICK_SINCLAIR_RIGHT_V3;
    break;
  case LIBSPECTRUM_JOYSTICK_SINCLAIR_2:
    /* Write Z80_JOYSTICK_USER_DEFINED_V3 and custom key mapping corresponding
       to Sinclair Interface II right port */
    joystick_flags = Z80_JOYSTICK_USER_DEFINED_V3;
    break;

  default:
    /* LIBSPECTRUM_JOYSTICK_NONE,
       LIBSPECTRUM_JOYSTICK_TIMEX_1,
       LIBSPECTRUM_JOYSTICK_TIMEX_2,
       LIBSPECTRUM_JOYSTICK_FULLER, */
    *flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
  }
  joystick_flags <<= 6;

  *(*ptr)++ = ( libspectrum_snap_im( snap ) & 0x03 ) +
              ( libspectrum_snap_issue2( snap ) ? 0x04 : 0x00 ) +
              joystick_flags;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_extended_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length, int *flags, libspectrum_snap *snap )
{
  int i, bottom_16kb_ram; libspectrum_error error;

  libspectrum_dword quarter_states;

  int capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  /* +2 here to deal with the two length bytes */
  error = libspectrum_make_room( buffer, LIBSPECTRUM_Z80_V3_LENGTH + 2, ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  libspectrum_write_word( ptr, LIBSPECTRUM_Z80_V3_LENGTH );
  
  libspectrum_write_word( ptr, libspectrum_snap_pc( snap ) );

  switch( libspectrum_snap_machine( snap ) ) {
  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
    *(*ptr)++ = Z80_MACHINE_48; break;
  case LIBSPECTRUM_MACHINE_SE:
    *flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
    /* fall through */
  case LIBSPECTRUM_MACHINE_128:
    *(*ptr)++ = Z80_MACHINE_128; break;
  case LIBSPECTRUM_MACHINE_PLUS3E:
    *flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
    /* fall through */
  case LIBSPECTRUM_MACHINE_PLUS3:
    *(*ptr)++ = Z80_MACHINE_PLUS3; break;
  case LIBSPECTRUM_MACHINE_PENT:
    *(*ptr)++ = Z80_MACHINE_PENTAGON; break;
  case LIBSPECTRUM_MACHINE_SCORP:
    *(*ptr)++ = Z80_MACHINE_SCORPION; break;
  case LIBSPECTRUM_MACHINE_PLUS2:
    *(*ptr)++ = Z80_MACHINE_PLUS2; break;
  case LIBSPECTRUM_MACHINE_PLUS2A:
    *(*ptr)++ = Z80_MACHINE_PLUS2A; break;
  case LIBSPECTRUM_MACHINE_TC2048:
    *(*ptr)++ = Z80_MACHINE_TC2048; break;
  case LIBSPECTRUM_MACHINE_TC2068:
    *(*ptr)++ = Z80_MACHINE_TC2068; break;
  case LIBSPECTRUM_MACHINE_TS2068:
    *(*ptr)++ = Z80_MACHINE_TS2068; break;

  case LIBSPECTRUM_MACHINE_UNKNOWN:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:write_extended_header: machine type unknown",
			     __FILE__ );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    *(*ptr)++ = libspectrum_snap_out_128_memoryport( snap );
  } else if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ) {
    *(*ptr)++ = libspectrum_snap_out_scld_hsr( snap );
  } else {
    *(*ptr)++ = '\0';
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO ) {
    *(*ptr)++ = libspectrum_snap_out_scld_dec( snap );
  } else {
    *(*ptr)++ = '\0';		/* IF1 disabled */
  }

  /* Support 16K snapshots via Spectaculator's extension; see the
     comment in read_header for details */
  if( libspectrum_snap_machine( snap ) == LIBSPECTRUM_MACHINE_16 ) {
    *(*ptr)++ = 0x80;
  } else {
    *(*ptr)++ = '\0';		/* No special emulation features */
  }

  *(*ptr)++ = libspectrum_snap_out_ay_registerport( snap );
  for( i = 0; i < 16; i++ )
    *(*ptr)++ = libspectrum_snap_ay_registers( snap, i );

  quarter_states =
    libspectrum_timings_tstates_per_frame(
      libspectrum_snap_machine( snap ) 
    ) / 4;

  libspectrum_write_word(
    ptr,
    quarter_states - ( libspectrum_snap_tstates( snap ) % quarter_states ) - 1
  );

  *(*ptr)++ =
    ( ( libspectrum_snap_tstates( snap ) / quarter_states ) + 3 ) % 4;

  /* Spectator, MGT and Multiface disabled */
  *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0';

  /* Is 0x0000 to 0x3fff RAM? True if we're in a +3 64Kb RAM
     configuration, or a Scorpion configuration with page 0 mapped in */
  bottom_16kb_ram = 0;

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) {

    if( libspectrum_snap_out_plus3_memoryport( snap ) & 0x01 )
      bottom_16kb_ram = 1;

  } else if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY ) {

    if( libspectrum_snap_out_plus3_memoryport( snap ) & 0x01 )
      bottom_16kb_ram = 1;
  }

  if( bottom_16kb_ram ) {
    *(*ptr)++ = 0xff; *(*ptr)++ = 0xff;
  } else {
    *(*ptr)++ = 0x00; *(*ptr)++ = 0x00;
  }

  /* Joystick settings */
  if( libspectrum_snap_joystick_list( snap, 1 ) ==
        LIBSPECTRUM_JOYSTICK_SINCLAIR_2 ) {
    /* First 10 bytes are spectrum keyboard mappings for user defined keys
       in this order: left, right, down, up, fire */
    libspectrum_write_word( ptr, if2_left_map_l );
    libspectrum_write_word( ptr, if2_left_map_r );
    libspectrum_write_word( ptr, if2_left_map_d );
    libspectrum_write_word( ptr, if2_left_map_u );
    libspectrum_write_word( ptr, if2_left_map_f );
    /* Second 10 bytes are ascii words corresponding to keys */
    libspectrum_write_word( ptr, if2_left_l );
    libspectrum_write_word( ptr, if2_left_r );
    libspectrum_write_word( ptr, if2_left_d );
    libspectrum_write_word( ptr, if2_left_u );
    libspectrum_write_word( ptr, if2_left_f );
  } else {
    for( i=32; i<52; i++ ) *(*ptr)++ = '\0';
  }

  /* etc */
  for( i=52; i<=LIBSPECTRUM_Z80_V3_LENGTH; i++ ) *(*ptr)++ = '\0';

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_pages( libspectrum_byte **buffer, libspectrum_byte **ptr, size_t *length,
	     libspectrum_snap *snap, int compress )
{
  int i; libspectrum_error error;
  int do_slt;

  int capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  if( !( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) ) {

    error = write_page( buffer, ptr, length, 4,
			libspectrum_snap_pages( snap, 2 ), compress );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    error = write_page( buffer, ptr, length, 5,
			libspectrum_snap_pages( snap, 0 ), compress );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    error = write_page( buffer, ptr, length, 8,
			libspectrum_snap_pages( snap, 5 ), compress );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  } else {

    for( i=0; i<8; i++ ) {
      if( libspectrum_snap_pages( snap, i ) ) {
	error = write_page( buffer, ptr, length, i+3,
			    libspectrum_snap_pages( snap, i ), compress );
	if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      }
    }

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY ) {
      for( i = 8; i < 16; i++ ) {
        if( libspectrum_snap_pages( snap, i ) ) {
          error = write_page( buffer, ptr, length, i + 3,
                              libspectrum_snap_pages( snap, i ), compress );
          if( error != LIBSPECTRUM_ERROR_NONE ) return error;
        }
      }
    }

  }

  /* Do we want to write .slt data? Definitely if we've got a loading
     screen */
  do_slt = libspectrum_snap_slt_screen( snap ) != NULL;

  /* If not, see if any level data exists */
  for( i=0; !do_slt && i<256; i++ )
    if( libspectrum_snap_slt_length( snap, i ) ) do_slt = 1;

  /* Write the data if we've got any */
  if( do_slt ) {
    error = write_slt( buffer, ptr, length, snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
write_page( libspectrum_byte **buffer, libspectrum_byte **ptr, size_t *length,
	    int page_num, libspectrum_byte *page, int compress )
{
  libspectrum_byte *compressed = NULL; size_t compressed_length;
  libspectrum_error error;

  if( compress ) {

    compressed_length = 0;
    error = compress_block( &compressed, &compressed_length, page, 0x4000 );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  }

  if( !compress || ( !(compress & LIBSPECTRUM_FLAG_SNAPSHOT_ALWAYS_COMPRESS) &&
                      compressed_length >= 0x4000 ) ) {

    error = libspectrum_make_room( buffer, 3 + 0x4000, ptr, length );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    libspectrum_write_word( ptr, 0xffff );
    *(*ptr)++ = page_num;
    memcpy( *ptr, page, 0x4000 ); *ptr += 0x4000;

  } else {

    libspectrum_make_room( buffer, 3 + compressed_length, ptr, length );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    libspectrum_write_word( ptr, compressed_length );
    *(*ptr)++ = page_num;
    memcpy( *ptr, compressed, compressed_length ); *ptr += compressed_length;

  }

  if( compressed ) free( compressed );

  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
write_slt( libspectrum_byte **buffer, libspectrum_byte **ptr, size_t *length,
	   libspectrum_snap *snap )
{
  int i,j;

  size_t compressed_length[256];
  libspectrum_byte* compressed_data[256];

  size_t compressed_screen_length; libspectrum_byte* compressed_screen;

  libspectrum_error error;

  /* Make room for the .slt signature */
  error = libspectrum_make_room( buffer, slt_signature_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  memcpy( *ptr, slt_signature, slt_signature_length );
  *ptr += slt_signature_length;

  /* Now write out the .slt directory, compressing the data along
     the way */
  for( i=0; i<256; i++ ) {
    if( libspectrum_snap_slt_length( snap, i ) ) {

      /* Zero the compressed length so it will be allocated memory
	 by compress_block */
      compressed_length[i] = 0;

      error = compress_block( &compressed_data[i], &compressed_length[i],
			      libspectrum_snap_slt( snap, i ),
			      libspectrum_snap_slt_length( snap, i ) );
      if( error != LIBSPECTRUM_ERROR_NONE ) {

	for( j = 0; j < i; j++ )
	  if( libspectrum_snap_slt_length( snap, j ) )
	    free( compressed_data[j] );

	return error;
      }

      error = write_slt_entry( buffer, ptr, length,
			       LIBSPECTRUM_SLT_TYPE_LEVEL, i,
			       compressed_length[i] );
      if( error != LIBSPECTRUM_ERROR_NONE ) {

	for( j = 0; j < i; j++ )
	  if( libspectrum_snap_slt_length( snap, j ) )
	    free( compressed_data[j] );

	return error;
      }
    }
  }

  /* Write the directory entry for the loading screen out if we've got one */
  if( libspectrum_snap_slt_screen( snap ) ) {

    compressed_screen_length = 0;
    error = compress_block( &compressed_screen, &compressed_screen_length,
			    libspectrum_snap_slt_screen( snap ), 6912 );
    if( error != LIBSPECTRUM_ERROR_NONE ) {

      for( i = 0; i < 256; i++ )
	if( libspectrum_snap_slt_length( snap, i ) )
	  free( compressed_data[i] );

      return error;
    }

    /* If length >= 6912, write out uncompressed */
    if( compressed_screen_length >= 6912 ) {
      compressed_screen_length = 6912;
      memcpy( compressed_screen, libspectrum_snap_slt_screen( snap ), 6912 );
    }

    /* Write the directory entry */
    error = write_slt_entry( buffer, ptr, length,
			     LIBSPECTRUM_SLT_TYPE_SCREEN,
			     libspectrum_snap_slt_screen_level( snap ),
			     compressed_screen_length );
    if( error != LIBSPECTRUM_ERROR_NONE ) {

      free( compressed_screen );
      for( i = 0; i < 256; i++ )
	if( libspectrum_snap_slt_length( snap, i ) )
	  free( compressed_data[i] );

      return error;
    }
  }

  /* and the directory end marker */
  error = write_slt_entry( buffer, ptr, length, LIBSPECTRUM_SLT_TYPE_END, 0,
			   0 );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    if( libspectrum_snap_slt_screen( snap ) ) free( compressed_screen );
    for( i = 0; i < 256; i++ )
      if( libspectrum_snap_slt_length( snap, i ) )
	free( compressed_data[i] );
    return error;
  }

  /* Then write the actual level data */
  for( i=0; i<256; i++ ) {
    if( libspectrum_snap_slt_length( snap, i ) ) {
      
      /* Make room for the data */
      error = libspectrum_make_room( buffer, compressed_length[i], ptr,
				     length );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	if( libspectrum_snap_slt_screen( snap ) ) free( compressed_screen );
	for( j = 0; j < 256; i++ )
	  if( libspectrum_snap_slt_length( snap, i ) )
	    free( compressed_data[i] );
	return error;
      }

      /* And copy it across */
      memcpy( *ptr, compressed_data[i], compressed_length[i] );
      *ptr += compressed_length[i];
    }
  }

  /* And the loading screen */
  if( libspectrum_snap_slt_screen( snap ) ) {

    /* Make room */
    error = libspectrum_make_room( buffer, compressed_screen_length, ptr,
				   length );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      if( libspectrum_snap_slt_screen( snap ) ) free( compressed_screen );
      for( i = 0; i < 256; i++ )
	if( libspectrum_snap_slt_length( snap, i ) )
	  free( compressed_data[i] );
      return error;
    }

    /* Copy the data */
    memcpy( *ptr, compressed_screen, compressed_screen_length );
    *ptr += compressed_screen_length;
  }

  /* Free up the compressed data */
  if( libspectrum_snap_slt_screen( snap ) ) free( compressed_screen );
  for( i = 0; i < 256; i++ )
    if( libspectrum_snap_slt_length( snap, i ) )
      free( compressed_data[i] );

  /* That's your lot */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_slt_entry( libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length, libspectrum_word type, libspectrum_word id,
		 libspectrum_dword slt_length )
{
  libspectrum_error error;

  /* We need 8 bytes of space */
  error = libspectrum_make_room( buffer, 8, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  libspectrum_write_word( ptr, type );
  libspectrum_write_word( ptr, id );
  libspectrum_write_word( ptr, slt_length & 0xffff );
  libspectrum_write_word( ptr, slt_length >> 16 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
compress_block( libspectrum_byte **dest, size_t *dest_length,
		const libspectrum_byte *src, size_t src_length)
{
  const libspectrum_byte *in_ptr;
  libspectrum_byte *out_ptr;
  int last_char_ed = 0;

  /* Allocate memory for dest if requested */
  if( *dest_length == 0 ) {
    *dest_length = src_length/2;
    *dest =
      (libspectrum_byte*)malloc( *dest_length * sizeof(libspectrum_byte) );
  }
  if( *dest == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "compress_block: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  in_ptr = src;
  out_ptr = *dest;

  /* Now loop over the entire input block */
  while( in_ptr < src + src_length ) {

    /* If we're pointing at the last byte, just copy it across
       and exit */
    if( in_ptr == src + src_length - 1 ) {
      if( libspectrum_make_room( dest, 1, &out_ptr, dest_length ) ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "compress_block: out of memory" );
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      *out_ptr++ = *in_ptr++;
      continue;
    }

    /* Now see if we're pointing to a run of identical bytes, and
       the last thing output wasn't a single 0xed */
    if( *in_ptr == *(in_ptr+1) && !last_char_ed ) {

      libspectrum_byte repeated;
      size_t run_length;
      
      /* Reset the `last character was a 0xed' flag */
      last_char_ed = 0;

      /* See how long the run is */
      repeated = *in_ptr;
      in_ptr += 2;
      run_length = 2;

      /* Find the length of the run (but cap it at 255 bytes) */
      while( in_ptr < src + src_length && *in_ptr == repeated && 
	     run_length < 0xff ) {
	run_length++;
	in_ptr++;
      }

      if( run_length >= 5 || repeated == 0xed ) {
	/* Output this in compressed form if it's of length 5 or longer,
	   _or_ if it's a run of 0xed */

	if( libspectrum_make_room( dest, 4, &out_ptr, dest_length ) ) {
	  libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				   "compress_block: out of memory" );
	  return LIBSPECTRUM_ERROR_MEMORY;
	}

	*out_ptr++ = 0xed;
	*out_ptr++ = 0xed;
	*out_ptr++ = run_length;
	*out_ptr++ = repeated;

      } else {

	/* If not, just output the bytes */
	if( libspectrum_make_room( dest, run_length, &out_ptr, dest_length )) {
	  libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				   "compress_block: out of memory" );
	  return LIBSPECTRUM_ERROR_MEMORY;
	}
	while(run_length--) *out_ptr++ = repeated;

      }

    } else {

      /* Not a repeated character, so just output the byte */
      last_char_ed = ( *in_ptr == 0xed ) ? 1 : 0;
      if( libspectrum_make_room( dest, 1, &out_ptr, dest_length ) ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "compress_block: out of memory" );
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      *out_ptr++ = *in_ptr++;

    }
      
  }

  (*dest_length) = out_ptr - *dest;

  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
uncompress_block( libspectrum_byte **dest, size_t *dest_length,
		  const libspectrum_byte *src, size_t src_length)
{
  const libspectrum_byte *in_ptr;
  libspectrum_byte *out_ptr;

  /* Allocate memory for dest if requested */
  if( *dest_length == 0 ) {
    *dest_length = src_length/2;
    *dest =
      (libspectrum_byte*)malloc( *dest_length * sizeof(libspectrum_byte) );
  }
  if( *dest == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "uncompress_block: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  in_ptr = src;
  out_ptr = *dest;

  while( in_ptr < src + src_length ) {

    /* If we're pointing at the last byte, just copy it across
       and exit */
    if( in_ptr == src + src_length - 1 ) {
      if( libspectrum_make_room( dest, 1, &out_ptr, dest_length ) ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "uncompress_block: out of memory" );
        return LIBSPECTRUM_ERROR_MEMORY;
      }
      *out_ptr++ = *in_ptr++;
      continue;
    }

    /* If we're pointing at two successive 0xed bytes, that's
       a run. If not, just copy the byte across */
    if( *in_ptr == 0xed && *(in_ptr+1) == 0xed ) {

      size_t run_length;
      libspectrum_byte repeated;
      
      in_ptr+=2;
      run_length = *in_ptr++;
      repeated = *in_ptr++;

      if( libspectrum_make_room( dest, run_length, &out_ptr, dest_length ) ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "uncompress_block: out of memory" );
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      while(run_length--) *out_ptr++ = repeated;

    } else {

      if( libspectrum_make_room( dest, 1, &out_ptr, dest_length ) ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "uncompress_block: out of memory" );
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      *out_ptr++ = *in_ptr++;

    }

  }

  (*dest_length) = out_ptr - *dest;

  return LIBSPECTRUM_ERROR_NONE;

}
