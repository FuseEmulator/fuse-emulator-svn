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

/* We have a fairly huge impedance mismatch between libspectrum and
   zlib here: libspectrum likes to deal with everything in memory,
   whilst zlib will only deal with gzip data in files on disk. Hence
   we have to write the data to a temporary file, and then read the
   data back from that again. This is particularly silly if we started
   with a simple file before feeding this to libspectrum... */

/* There is another way to do this. The zlib functions have an
   undocumented option (setting the window size negative) to assume
   that there is no zlib header on the file. Would using that be nicer
   than doing this? (see gzopen and inflateInit2 in zlib for more
   details). */

/* FIXME: need a better error code to deal with OS errors */

libspectrum_error
libspectrum_gzip_inflate( const libspectrum_byte *gzptr, size_t gzlength,
			  libspectrum_byte **outptr, size_t *outlength )
{
  int fd;
  char tempfile[ 256 ];
  gzFile f; int gzcount;
  ssize_t count; off_t offset;
  size_t length_to_read, allocated;

  strcpy( tempfile, "/tmp/libspectrumXXXXXX" );

  fd = mkstemp( tempfile );
  if( fd == -1 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "couldn't open temporary gzip file: %s",
			     strerror( errno ) );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  unlink( tempfile );

  count = write( fd, gzptr, gzlength );
  if( count == -1 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "error writing to temporary gzip file: %s",
			     strerror( errno ) );
    close( fd );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  offset = lseek( fd, 0, SEEK_SET );
  if( offset == (off_t)-1 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "error seeking in temporary gzip file: %s",
			     strerror( errno ) );
    close( fd );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  f = gzdopen( fd, "rb" );
  if( !f ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "out of memory at %s:%d", __FILE__, __LINE__ );
    close( fd );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  length_to_read = *outlength ? *outlength : gzlength;

  allocated = 0; *outptr = NULL;

  do {
    
    libspectrum_byte *buffer, *ptr;

    buffer = realloc( *outptr, allocated + length_to_read );
    if( !buffer ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "out of memory at %s:%d", __FILE__, __LINE__ );
      free( *outptr ); gzclose( f );
      return LIBSPECTRUM_ERROR_MEMORY;
    }
    (*outptr) = buffer; ptr = buffer + allocated;

    allocated += length_to_read;

    gzcount = gzread( f, ptr, length_to_read );

  } while( gzcount == length_to_read );

  if( gzcount == -1 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "error reading from temporary gzip file: %s",
			     strerror( errno ) );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  /* Trim the buffer down to fit */
  *outlength = allocated - length_to_read + gzcount;
  *outptr = realloc( *outptr, *outlength );

  gzclose( f );

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
