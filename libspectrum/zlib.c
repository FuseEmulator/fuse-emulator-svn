/* zlib.c: routines for zlib (de)compression of data
   Copyright (c) 2002 Darren Salt, Philip Kendall

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

   Darren: E-mail: linux@youmustbejoking.demon.co.uk

   Philip: E-mail: pak21-fuse@srcf.ucam.org
             Post: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef HAVE_ZLIB_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <zlib.h>

#include "internals.h"

static libspectrum_error
skip_gzip_header( const libspectrum_byte **gzptr, size_t *gzlength );
static libspectrum_error
skip_null_terminated_string( const libspectrum_byte **ptr, size_t *length,
			     char *name );

libspectrum_error 
libspectrum_zlib_inflate( const libspectrum_byte *gzptr, size_t gzlength,
			  libspectrum_byte **outptr, size_t *outlength )
/* Inflates a block of data.
 * Input:	gzptr		-> source (deflated) data
 *		*gzlength	== source data length
 * Output:	*outptr		-> inflated data (malloced in this fn)
 *		*outlength	== length of the inflated data
 * Returns:	error flag (libspectrum_error)
 */
{
  int known_length;

  if( *outlength ) {
    known_length = 1;
  } else {
    known_length = 0; *outlength = 16384;
  }

  if( !known_length ) *outlength = 16384;

  *outptr = malloc( *outlength );
  if (!*outptr) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_zlib_inflate: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  if( known_length ) {

    /* We were given the size of the output data; use the fast method */

    uLongf dl = *outlength;

    switch( uncompress( *outptr, &dl, gzptr, gzlength ) ) {

    case Z_OK:			/* successfully decompressed */
      *outlength = dl;
      return LIBSPECTRUM_ERROR_NONE;
    case Z_MEM_ERROR:		/* out of memory */
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_MEMORY,
	"libspectrum_zlib_inflate: out of memory in zlib"
      );
      return LIBSPECTRUM_ERROR_MEMORY;
    default:			/* corrupt data or short length */
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "libspectrum_zlib_inflate: corrupt data" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

  } else {

    /* We don't know how big the output data is; use the slow method */
    z_stream stream;

    /* Cast to remove warning about losing constness */
    stream.next_in = (libspectrum_byte*)gzptr;
    stream.avail_in = gzlength;
    stream.next_out = *outptr;
    stream.avail_out = *outlength;

    stream.zalloc = 0;
    stream.zfree = 0;
    stream.opaque = 0;

    switch( inflateInit (&stream) ) {

    case Z_OK:			/* initialised OK */
      break;
    case Z_MEM_ERROR:		/* out of memory */
      inflateEnd( &stream );
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "libspectrum_zlib_inflate: out of memory" );
      return LIBSPECTRUM_ERROR_MEMORY;
    case Z_VERSION_ERROR:	/* unrecognised version */
      inflateEnd( &stream );
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "libspectrum_zlib_inflate: unknown version" );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    default:			/* some other error */
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "libspectrum_zlib_inflate: %s", stream.msg );
      inflateEnd( &stream );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    while( 1 ) {

      switch (inflate (&stream, Z_SYNC_FLUSH)) {

      case Z_OK:		/* more to decompress */
	break;
      case Z_STREAM_END:	/* successfully decompressed */
	*outlength = stream.next_out - *outptr;
	*outptr = realloc( *outptr, *outlength );	/* truncate to fit */
	inflateEnd( &stream );
	return LIBSPECTRUM_ERROR_NONE;
      case Z_BUF_ERROR:		/* need more buffer space? */
	{
	  libspectrum_byte *new_out =
	    realloc( *outptr, (*outlength += 16384) );
	  if( !new_out ) {
	    inflateEnd( &stream );
	    libspectrum_print_error(
              LIBSPECTRUM_ERROR_MEMORY,
	      "libspectrum_zlib_inflate: out of memory"
            );
	    return LIBSPECTRUM_ERROR_MEMORY;
	  }

	  /* Adjust from n bytes into *outptr to n bytes into new_out */
	  stream.next_out += new_out - *outptr;
	  *outptr = new_out;
	  stream.avail_out += 16384;
	}
	break;
      case Z_MEM_ERROR:		/* out of memory */
	inflateEnd( &stream );
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "libspectrum_zlib_inflate: out of memory" );
	return LIBSPECTRUM_ERROR_MEMORY;
      default:			/* corrupt data or needs dictionary */
	libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
				 "libspectrum_zlib_inflate: %s", stream.msg);
	inflateEnd( &stream );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }
    }
  }

  return LIBSPECTRUM_ERROR_LOGIC;	/* This can't happen */
}

libspectrum_error
libspectrum_gzip_inflate( const libspectrum_byte *gzptr, size_t gzlength,
			  libspectrum_byte **outptr, size_t *outlength )
{
  z_stream stream;
  libspectrum_error libspec_error;
  int error, simple;

  libspec_error = skip_gzip_header( &gzptr, &gzlength );
  if( libspec_error ) return libspec_error;

  /* Use default memory management */
  stream.zalloc = Z_NULL; stream.zfree = Z_NULL; stream.opaque = Z_NULL;

  /* Cast needed to avoid warning about losing const */
  stream.next_in = (libspectrum_byte*)gzptr; stream.avail_in = gzlength;

  /*
   * HACK ALERT (comment from zlib 1.1.14:gzio.c:143)
   *
   * windowBits is passed < 0 to tell that there is no zlib header.
   * Note that in this case inflate *requires* an extra "dummy" byte
   * after the compressed stream in order to complete decompression
   * and return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes
   * are present after the compressed stream.
   *
   */
  error = inflateInit2( &stream, -15 );
  switch( error ) {

  case Z_OK: break;

  case Z_MEM_ERROR: 
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "out of memory at %s:%d", __FILE__, __LINE__ );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_MEMORY;

  case Z_STREAM_ERROR:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "error from inflateInit2: %s", stream.msg );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_MEMORY;

  }

  /* If we know the size of the output data, use the quick method;
     otherwise we must inflate the data in chunks */
  simple = *outlength;

  if( simple ) {

    stream.next_out = *outptr; stream.avail_out = *outlength;

    error = inflate( &stream, Z_FINISH );

  } else {

    *outptr = stream.next_out = NULL;
    *outlength = stream.avail_out = 0;

    do {

      libspectrum_byte *ptr;

      *outlength += 16384; stream.avail_out += 16384;
      ptr = realloc( *outptr, *outlength );
      if( !ptr ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "out of memory at %s:%d",
				 __FILE__, __LINE__ );
	inflateEnd( &stream );
	free( *outptr );
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      
      stream.next_out = ptr + ( stream.next_out - *outptr );
      *outptr = ptr;

      error = inflate( &stream, 0 );

    } while( error == Z_OK );

  }

  *outlength = stream.next_out - *outptr;
  *outptr = realloc( *outptr, *outlength );

  switch( error ) {

  case Z_STREAM_END: break;

  case Z_NEED_DICT:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "gzip inflation needs dictionary" );
    if( !simple ) free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_UNKNOWN;

  case Z_DATA_ERROR:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT, "corrupt gzip data" );
    if( !simple ) free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_CORRUPT;

  case Z_MEM_ERROR:
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "out of memory at %s:%d", __FILE__, __LINE__ );
    if( !simple ) free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_MEMORY;

  case Z_BUF_ERROR:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "not enough space in gzip output buffer" );
    if( !simple ) free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_CORRUPT;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "gzip error from inflate: %s",
			     stream.msg );
    if( !simple ) free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  error = inflateEnd( &stream );
  if( error != Z_OK ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "gzip error from inflateEnd: %s", stream.msg );
    if( !simple ) free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_gzip_header( const libspectrum_byte **gzptr, size_t *gzlength )
{
  libspectrum_byte flags;
  libspectrum_error error;

  if( *gzlength < 10 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "not enough data for gzip header" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( (*gzptr)[0] != 0x1f || (*gzptr)[1] != 0x8b ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "gzip header missing" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( (*gzptr)[2] != 8 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "unknown gzip compression method %d",
			     (*gzptr)[2] );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = (*gzptr)[3];

  (*gzptr) += 10; (*gzlength) -= 10;

  if( flags & 0x04 ) {		/* extra header present */

    size_t length;

    if( *gzlength < 2 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
	"not enough data for gzip extra header length"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    length = (*gzptr)[0] + (*gzptr)[1] * 0x100;
    (*gzptr) += 2; (*gzlength) -= 2;

    if( *gzlength < length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "not enough data for gzip extra header" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

  }

  if( flags & 0x08 ) {		/* original file name present */
    error = skip_null_terminated_string( gzptr, gzlength, "original name" );
    if( error ) return error;
  }

  if( flags & 0x10 ) {		/* comment present */
    error = skip_null_terminated_string( gzptr, gzlength, "comment" );
    if( error ) return error;
  }

  if( flags & 0x02 ) {		/* header CRC present */

    if( *gzlength < 2 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "not enough data for gzip header CRC" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Could check the header CRC if we really wanted to */
    (*gzptr) += 2; (*gzptr) -= 2;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_null_terminated_string( const libspectrum_byte **ptr, size_t *length,
			     char *name )
{
  while( **ptr && *length ) { (*ptr)++; (*length)--; }

  if( !( *length ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "not enough data for gzip %s", name );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Skip the null as well */
  (*ptr)++; (*length)--;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_zlib_compress( const libspectrum_byte *data, size_t length,
			   libspectrum_byte **gzptr, size_t *gzlength )
/* Deflates a block of data.
 * Input:	data		-> source data
 *		length		== source data length
 * Output:	*gzptr		-> deflated data (malloced in this fn),
 *		*gzlength	== length of the deflated data
 * Returns:	error flag (libspectrum_error)
 */
{
  uLongf gzl = ( length * 1.001 ) + 12;
  int gzret;

  *gzptr = malloc( gzl );
  if( !*gzptr ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_zlib_compress: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  gzret = compress2( *gzptr, &gzl, data, length, Z_BEST_COMPRESSION );

  switch (gzret) {

  case Z_OK:			/* initialised OK */
    *gzlength = gzl;
    return LIBSPECTRUM_ERROR_NONE;

  case Z_MEM_ERROR:		/* out of memory */
    free( *gzptr ); *gzptr = 0;
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_zlib_compress: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;

  case Z_VERSION_ERROR:		/* unrecognised version */
    free( *gzptr ); *gzptr = 0;
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "libspectrum_zlib_compress: unknown version" );
    return LIBSPECTRUM_ERROR_UNKNOWN;

  case Z_BUF_ERROR:		/* Not enough space in output buffer.
				   Shouldn't happen */
    free( *gzptr ); *gzptr = 0;
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_zlib_compress: out of space?" ); 
    return LIBSPECTRUM_ERROR_LOGIC;

  default:			/* some other error */
    free( *gzptr ); *gzptr = 0;
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_zlib_compress: unexpected error?" ); 
    return LIBSPECTRUM_ERROR_LOGIC;
  }
}

#endif				/* #ifdef HAVE_ZLIB_H */
