/* rzx.c: routines for dealing with .rzx files
   Copyright (c) 2002 Philip Kendall

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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "internals.h"

/* The block types which can appear in RZX files */
typedef enum libspectrum_rzx_block_t {

  LIBSPECTRUM_RZX_CREATOR_BLOCK = 0x10,
  LIBSPECTRUM_RZX_SNAPSHOT_BLOCK = 0x30,
  LIBSPECTRUM_RZX_INPUT_BLOCK = 0x80,

} libspectrum_rzx_block_t;

typedef struct libspectrum_rzx_frame_t {

  size_t instructions;

  size_t count;
  libspectrum_byte* in_bytes;

  int repeat_last;			/* Set if we should use the last
					   frame's IN bytes */

} libspectrum_rzx_frame_t;

struct libspectrum_rzx {

  libspectrum_rzx_frame_t *frames;
  size_t count;
  size_t allocated;

  size_t tstates;

  /* Playback variables */
  size_t current_frame;		/* The number of the current playback frame */
  size_t in_count;		/* How many INs done in current frame */
  libspectrum_rzx_frame_t *data_frame;
				/* Which frame we're reading INs from */

};

static libspectrum_error
rzx_read_header( const libspectrum_byte **ptr, const libspectrum_byte *end );
static libspectrum_error
rzx_read_creator( const libspectrum_byte **ptr, const libspectrum_byte *end );
static libspectrum_error
rzx_read_snapshot( const libspectrum_byte **ptr, const libspectrum_byte *end,
		   libspectrum_snap **snap );
static libspectrum_error
rzx_read_input( libspectrum_rzx *rzx,
		const libspectrum_byte **ptr, const libspectrum_byte *end );
static libspectrum_error
rzx_read_frames( libspectrum_rzx *rzx,
		 const libspectrum_byte **ptr, const libspectrum_byte *end );

static libspectrum_error
rzx_write_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length );
static libspectrum_error
rzx_write_creator( libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length, const char *program, libspectrum_word major,
		   libspectrum_word minor );
static libspectrum_error
rzx_write_snapshot( libspectrum_byte **buffer, libspectrum_byte **ptr,
		    size_t *length, libspectrum_snap *snap, int compress );
static libspectrum_error
rzx_write_input( libspectrum_rzx *rzx, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length, int compress );

/* The signature used to identify .rzx files */
const libspectrum_byte *signature = "RZX!";

/* The IN count used to signify 'repeat last frame' */
const libspectrum_word libspectrum_rzx_repeat_frame = 0xffff;

libspectrum_error
libspectrum_rzx_alloc( libspectrum_rzx **rzx )
{
  (*rzx) = (libspectrum_rzx*)malloc( sizeof( libspectrum_rzx ) );
  if( !(*rzx) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_rzx_alloc: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
  (*rzx)->count = (*rzx)->allocated = 0;
  (*rzx)->frames = NULL;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_store_frame( libspectrum_rzx *rzx, size_t instructions,
			     size_t count, libspectrum_byte *in_bytes )
{
  libspectrum_rzx_frame_t *frame;

  /* Get more space if we need it; allocate twice as much as we currently
     have, with a minimum of 50 */
  if( rzx->count == rzx->allocated ) {

    libspectrum_rzx_frame_t *ptr; size_t new_allocated;

    new_allocated = rzx->allocated >= 25 ? 2 * rzx->allocated : 50;
    ptr =
      (libspectrum_rzx_frame_t*)realloc(
        rzx->frames, new_allocated * sizeof(libspectrum_rzx_frame_t)
      );
    if( ptr == NULL ) return LIBSPECTRUM_ERROR_MEMORY;

    rzx->frames = ptr;
    rzx->allocated = new_allocated;
  }

  frame = &rzx->frames[ rzx->count ];

  frame->instructions = instructions;

  /* Check for repeated frames */
  if( rzx->count != 0 && count != 0 &&
      count == rzx->frames[ rzx->count - 1].count &&
      !memcmp( in_bytes, rzx->frames[ rzx->count - 1].in_bytes, count ) ) {
	
    frame->repeat_last = 1;

  } else {

    frame->repeat_last = 0;
    frame->count = count;

    frame->in_bytes = (libspectrum_byte*)
      malloc( count * sizeof( libspectrum_byte ) );
    if( frame->in_bytes == NULL ) return LIBSPECTRUM_ERROR_MEMORY;

    memcpy( frame->in_bytes, in_bytes, count * sizeof( libspectrum_byte ) );
  }

  /* Move along to the next frame */
  rzx->count++;

  return 0;
}

libspectrum_error
libspectrum_rzx_start_playback( libspectrum_rzx *rzx )
{
  rzx->current_frame = 0; rzx->in_count = 0;
  rzx->data_frame = rzx->frames;
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_playback_frame( libspectrum_rzx *rzx, int *finished )
{
  *finished = 0;

  /* Check we read the correct number of INs during this frame */
  if( rzx->in_count != rzx->data_frame->count ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_rzx_playback_frame: wrong number of INs in frame %lu: expected %lu, got %lu",
      (unsigned long)rzx->current_frame,
      (unsigned long)rzx->data_frame->count, (unsigned long)rzx->in_count
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Increment the frame count and see if we've finished with this file */
  if( ++rzx->current_frame >= rzx->count ) {
    *finished = 1;
    return LIBSPECTRUM_ERROR_NONE;
  }

  /* Move the data frame pointer along, unless we're supposed to be
     repeating the last frame */
  if( !rzx->frames[ rzx->current_frame ].repeat_last )
    rzx->data_frame = &rzx->frames[ rzx->current_frame ];

  /* And start with the first byte of the new frame */
  rzx->in_count = 0;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_playback( libspectrum_rzx *rzx, libspectrum_byte *byte )
{
  /* Check we're not trying to read off the end of the array */
  if( rzx->in_count >= rzx->data_frame->count ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_rzx_playback: more INs during frame %lu than stored in RZX file (%lu)",
      (unsigned long)rzx->current_frame, (unsigned long)rzx->data_frame->count
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  *byte = rzx->data_frame->in_bytes[ rzx->in_count++ ];
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_free( libspectrum_rzx *rzx )
{
  size_t i;

  for( i=0; i<rzx->count; i++ )
    if( !rzx->frames[i].repeat_last ) free( rzx->frames[i].in_bytes );
  
  free( rzx->frames ); rzx->frames = NULL;
  rzx->count = rzx->allocated = 0;
  return LIBSPECTRUM_ERROR_NONE;
}

size_t
libspectrum_rzx_tstates( libspectrum_rzx *rzx )
{
  return rzx->tstates;
}

size_t
libspectrum_rzx_set_tstates( libspectrum_rzx *rzx, size_t tstates )
{
  return( rzx->tstates = tstates );
}

size_t
libspectrum_rzx_instructions( libspectrum_rzx *rzx )
{
  return rzx->frames[ rzx->current_frame ].instructions;
}

libspectrum_error
libspectrum_rzx_read( libspectrum_rzx *rzx, libspectrum_snap **snap,
	              const libspectrum_byte *buffer, const size_t length )
{
  libspectrum_error error;
  const libspectrum_byte *ptr, *end;

  ptr = buffer; end = buffer + length;

  error = rzx_read_header( &ptr, end );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  while( ptr < end ) {

    libspectrum_byte id;

    id = *ptr++;

    switch( id ) {

    case LIBSPECTRUM_RZX_CREATOR_BLOCK:
      error = rzx_read_creator( &ptr, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      break;
      
    case LIBSPECTRUM_RZX_SNAPSHOT_BLOCK:
      error = rzx_read_snapshot( &ptr, end, snap );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      break;

    case LIBSPECTRUM_RZX_INPUT_BLOCK:
      error = rzx_read_input( rzx, &ptr, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      break;

    default:
      libspectrum_print_error(
	LIBSPECTRUM_ERROR_UNKNOWN,
        "libspectrum_rzx_read: unknown RZX block ID 0x%02x", id
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_header( const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  /* Check the header exists */
  if( end - (*ptr) < 10 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_header: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Check the RZX signature exists */
  if( memcmp( *ptr, signature, strlen( signature ) ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_SIGNATURE,
			     "rzx_read_header: RZX signature not found" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  (*ptr) += 10;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_creator( const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  size_t length;

  /* Check we've got enough data for the block */
  if( end - (*ptr) < 28 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_creator: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get the length */
  length = libspectrum_read_dword( ptr );

  /* Check there's still enough data (the -5 is because we've already read
     the block ID and the length) */
  if( end - (*ptr) < (ptrdiff_t)length - 5 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_creator: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  (*ptr) += length - 5;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_snapshot( const libspectrum_byte **ptr, const libspectrum_byte *end,
		   libspectrum_snap **snap )
{
  size_t blocklength, snaplength; libspectrum_error error;
  libspectrum_dword flags;
  const libspectrum_byte *snap_ptr;

  /* For deflated snapshot data: */
  int compressed;
  libspectrum_byte *gzsnap = NULL; size_t uncompressed_length = 0;

  if( end - (*ptr) < 16 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_snapshot: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  blocklength = libspectrum_read_dword( ptr );

  if( end - (*ptr) < (ptrdiff_t)blocklength - 5 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_snapshot: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* See if we want a compressed snap */
  flags = libspectrum_read_dword( ptr );
  compressed = flags & 0x02;

  /* How long is the (uncompressed) snap? */
  (*ptr) += 4;
  snaplength = libspectrum_read_dword( ptr );
  (*ptr) -= 8;

  /* If compressed, uncompress the data */
  if( compressed ) {

#ifdef HAVE_ZLIB_H

    error = libspectrum_zlib_inflate( (*ptr) + 8, blocklength - 17,
				      &gzsnap, &uncompressed_length );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    if( uncompressed_length != snaplength ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "rzx_read_snapshot: compressed snapshot has wrong length"
      );
      free( gzsnap );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }   
    snap_ptr = gzsnap;

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "rzx_read_snapshot: zlib needed for decompression\n"
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    /* If not compressed, check things are consistent */
    if( blocklength != snaplength + 17 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "rzx_read_snapshot: inconsistent snapshot lengths"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }
    snap_ptr = (*ptr) + 8;
    uncompressed_length = snaplength;

  }

  /* Initialise the snap */
  error = libspectrum_snap_alloc( snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    if( compressed ) free( gzsnap );
    libspectrum_snap_free( *snap );
    return error;
  }

  if( !strcasecmp( *ptr, "Z80" ) ) {
    error = libspectrum_z80_read( (*snap), snap_ptr, uncompressed_length );
  } else if( !strcasecmp( *ptr, "SNA" ) ) {
    error = libspectrum_sna_read( (*snap), snap_ptr, uncompressed_length );
  } else {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "rzx_read_snapshot: unrecognised snapshot format"
    );
    if( compressed ) free( gzsnap );
    libspectrum_snap_free( *snap );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  if( error != LIBSPECTRUM_ERROR_NONE ) {
    if( compressed ) free( gzsnap );
    libspectrum_snap_free( *snap );
    return error;
  }

  /* Free the decompressed data (if we created it) */
  if( compressed ) free( gzsnap );

  /* Skip over the data */
  (*ptr) += blocklength - 9;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_input( libspectrum_rzx *rzx,
		const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  size_t blocklength;
  libspectrum_dword flags; int compressed;
  libspectrum_error error;

  /* Check we've got enough data for the block */
  if( end - (*ptr) < 18 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_input: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get the length and number of frames */
  blocklength = libspectrum_read_dword( ptr );
  rzx->count = libspectrum_read_dword( ptr );

  /* Frame size is undefined, so just skip it */
  (*ptr)++;

  /* Allocate memory for the frames */
  rzx->frames =
    (libspectrum_rzx_frame_t*)malloc( rzx->count *
				      sizeof( libspectrum_rzx_frame_t) );
  if( rzx->frames == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "rzx_read_input: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
  rzx->allocated = rzx->count;

  /* Fetch the T-state counter and the flags */
  rzx->tstates = libspectrum_read_dword( ptr );

  flags = libspectrum_read_dword( ptr );
  compressed = flags & 0x02;

  if( compressed ) {

#ifdef HAVE_ZLIB_H

    libspectrum_byte *data; const libspectrum_byte *data_ptr;
    size_t data_length = 0;

    /* Discount the block intro */
    blocklength -= 18;

    /* Check that we've got enough compressed data */
    if( end - (*ptr) < (ptrdiff_t)blocklength ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "rzx_read_input: not enough data in buffer" );
      libspectrum_rzx_free( rzx );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    error = libspectrum_zlib_inflate( *ptr, blocklength, &data, &data_length );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_rzx_free( rzx );
      return error;
    }

    *ptr += blocklength;

    data_ptr = data;
    error = rzx_read_frames( rzx, &data_ptr, data + data_length );
    if( error ) {
      libspectrum_rzx_free( rzx ); free( data );
      return error;
    }

#else				/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "rzx_read_input: zlib needed for decompression" );
    libspectrum_rzx_free( rzx );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif				/* #ifdef HAVE_ZLIB_H */

  } else {			/* Data not compressed */

    error = rzx_read_frames( rzx, ptr, end );
    if( error ) {
      libspectrum_rzx_free( rzx );
      return error;
    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_frames( libspectrum_rzx *rzx,
		 const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  size_t i, j;

  /* And read in the frames */
  for( i=0; i < rzx->count; i++ ) {

    /* Check the two length bytes exist */
    if( end - (*ptr) < 4 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "rzx_read_frames: not enough data in buffer" );
      for( j=0; j<i; j++ ) {
	if( !rzx->frames[i].repeat_last ) free( rzx->frames[j].in_bytes );
      }
      libspectrum_rzx_free( rzx );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    rzx->frames[i].instructions = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
    rzx->frames[i].count        = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

    /* FIXME: is this is right thing to do? Or should we copy the data
       across from the previous frame? */
    if( rzx->frames[i].count == libspectrum_rzx_repeat_frame ) {
      rzx->frames[i].repeat_last = 1;
      continue;
    }

    rzx->frames[i].repeat_last = 0;

    if( end - (*ptr) < (ptrdiff_t)rzx->frames[i].count ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "rzx_read_frames: not enough data in buffer" );
      for( j=0; j<i; j++ ) {
	if( !rzx->frames[i].repeat_last ) free( rzx->frames[j].in_bytes );
      }
      libspectrum_rzx_free( rzx );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    rzx->frames[i].in_bytes =
      (libspectrum_byte*)malloc( rzx->frames[i].count *
				 sizeof( libspectrum_byte ) );
    if( rzx->frames[i].in_bytes == NULL ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "rzx_read_input: out of memory" );
      for( j=0; j<i; j++ ) {
	if( !rzx->frames[i].repeat_last ) free( rzx->frames[j].in_bytes );
      }
      libspectrum_rzx_free( rzx );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    memcpy( rzx->frames[i].in_bytes, *ptr, rzx->frames[i].count );
    (*ptr) += rzx->frames[i].count;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_write( libspectrum_byte **buffer, size_t *length,
		       libspectrum_rzx *rzx, libspectrum_snap *snap,
		       const char *program, libspectrum_word major,
		       libspectrum_word minor, int compress )
{
  libspectrum_error error;
  libspectrum_byte *ptr = *buffer;

  error = rzx_write_header( buffer, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = rzx_write_creator( buffer, &ptr, length, program, major, minor );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  if( snap ) {
    error = rzx_write_snapshot( buffer, &ptr, length, snap, compress );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  error = rzx_write_input( rzx, buffer, &ptr, length, compress );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  
  /* *length is the allocated size; we want to return how much is used */
  *length = ptr - *buffer;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length )
{
  libspectrum_error error;

  error = libspectrum_make_room( buffer, strlen(signature) + 6, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "rzx_write_header: out of memory" );
    return error;
  }

  strcpy( *ptr, signature ); (*ptr) += strlen( signature );
  *(*ptr)++ = 0;		/* Minor version number */
  *(*ptr)++ = 12;		/* Major version number */

  /* 'Reserved' flags */
  *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0';

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_creator( libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length, const char *program, libspectrum_word major,
		   libspectrum_word minor )
{
  libspectrum_error error;

  error = libspectrum_make_room( buffer, 29, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_print_error( error, "rzx_write_creator: out of memory" );
    return error;
  }

  *(*ptr)++ = LIBSPECTRUM_RZX_CREATOR_BLOCK;
  libspectrum_write_dword( ptr, 29 );	/* Block length */

  strncpy( *ptr, program, 19 ); (*ptr) += 19;
  *(*ptr)++ = '\0';

  libspectrum_write_word( ptr, major );
  libspectrum_write_word( ptr, minor );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_snapshot( libspectrum_byte **buffer, libspectrum_byte **ptr,
		    size_t *length, libspectrum_snap *snap, int compress )
{
  libspectrum_error error;
  libspectrum_byte *snap_buffer; size_t snap_length;
  libspectrum_byte *gzsnap = NULL; size_t gzlength;

  snap_length = 0;
  error = libspectrum_z80_write( &snap_buffer, &snap_length, snap );
  if( error ) return error;

  if( compress ) {

    error = libspectrum_zlib_compress( snap_buffer, snap_length,
				       &gzsnap, &gzlength );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      free( snap_buffer );
      return error;
    }

    error = libspectrum_make_room( buffer, 17 + gzlength, ptr, length );
  } else {
    error = libspectrum_make_room( buffer, 17 + snap_length, ptr, length );
  }

  if( error != LIBSPECTRUM_ERROR_NONE ) {
    if( gzsnap ) free( gzsnap ); free( snap_buffer );
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "rzx_write_snapshot: out of memory" );
    return error;
  }

  *(*ptr)++ = LIBSPECTRUM_RZX_SNAPSHOT_BLOCK;
  if( compress ) {			/* Block length and flags */
    libspectrum_write_dword( ptr, 17 + gzlength );
    libspectrum_write_dword( ptr, 2 );
  } else {
    libspectrum_write_dword( ptr, 17 + snap_length );
    libspectrum_write_dword( ptr, 0 );
  }
  strcpy( *ptr, "Z80" ); (*ptr) += 4;	/* Snapshot type */
  libspectrum_write_dword( ptr, snap_length );	/* Snapshot length */

  if( compress ) {
    memcpy( *ptr, gzsnap, gzlength ); (*ptr) += gzlength;
    free( gzsnap );
  } else {
    memcpy( *ptr, snap, snap_length ); (*ptr) += snap_length;
  }

  free( snap_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_input( libspectrum_rzx *rzx, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length, int compress )
{
  libspectrum_error error;
  size_t i, size;
  size_t length_offset, data_offset, flags_offset;
  libspectrum_byte *length_ptr; 

  error = libspectrum_make_room( buffer, 18, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_print_error( error, "rzx_write_input: out of memory" );
    return error;
  }

  *(*ptr)++ = LIBSPECTRUM_RZX_INPUT_BLOCK;

  /* The length bytes: for uncompressed data, 18 for the block introduction
     and 4 per frame; the number of bytes in every frame is added in below.
     If compression is requested (and makes the data shorter), this will be
     overwritten with the compressed length */
  size = 18 + 4 * rzx->count;

  /* Store where the length will be written, and skip over those byes */
  length_offset = *ptr - *buffer; (*ptr) += 4;

  /* How many frames? */
  libspectrum_write_dword( ptr, rzx->count );

  /* Each frame has an undefined length, so write a zero */
  *(*ptr)++ = 0;

  /* T-state counter */
  libspectrum_write_dword( ptr, rzx->tstates );

  /* Flags */
  flags_offset = *ptr - *buffer;
  libspectrum_write_dword( ptr, compress ? 0x02 : 0 );

  /* Write the frames */
  data_offset = *ptr - *buffer;
  for( i=0; i<rzx->count; i++ ) {

    libspectrum_rzx_frame_t *frame = &rzx->frames[i];

    error = libspectrum_make_room( buffer, 4, ptr, length );
    if( error ) {
      libspectrum_print_error( error, "rzx_write_input: out of memory" );
      return error;
    }

    libspectrum_write_word( ptr, frame->instructions );

    if( frame->repeat_last ) {
      libspectrum_write_word( ptr, libspectrum_rzx_repeat_frame );
    } else {
      error = libspectrum_make_room( buffer, frame->count, ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	libspectrum_print_error( error, "rzx_write_input: out of memory" );
	return error;
      }

      libspectrum_write_word( ptr, frame->count );
      memcpy( *ptr, frame->in_bytes, frame->count ); (*ptr) += frame->count;
      size += frame->count;			/* Keep track of the size */
    }
  }

  /* Write the length in */
  length_ptr = *buffer + length_offset;
  libspectrum_write_dword( &length_ptr, size ); length_ptr -= 4;

  if( compress ) {

#ifdef HAVE_ZLIB_H

    /* Compress the data the simple way. Really, we should stream the data */
    libspectrum_byte *gzsnap = NULL; size_t gzlength;
    libspectrum_byte *data_ptr = *buffer + data_offset;

    error = libspectrum_zlib_compress( data_ptr, *ptr - data_ptr,
				       &gzsnap, &gzlength );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    if( (ptrdiff_t)gzlength >= *ptr - data_ptr ) { /* Compression made it
						      bigger :-( */
      *(*buffer + flags_offset) &= ~0x02; /* Clear `compressed' bit */
    } else {
      /* Write the compressed data in */
      memcpy( data_ptr, gzsnap, gzlength );

      /* Correct the length word and the buffer length */
      libspectrum_write_dword( &length_ptr, 18 + gzlength );
      *ptr = *buffer + data_offset + gzlength;
    }

    free( gzsnap );

#else				/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "rzx_write_input: compression needs zlib" );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif				/* #ifdef HAVE_ZLIB_H */

  }

  return LIBSPECTRUM_ERROR_NONE;
}
