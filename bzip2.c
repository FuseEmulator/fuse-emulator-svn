/* bzip2.c: routines for bzip2 decompression of data
   Copyright (c) 2003-2004 Philip Kendall

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

#ifdef HAVE_LIBBZ2

#include <stdio.h>		/* Needed by bzlib.h < v 1.0.2 */
#include <stdlib.h>

#include <bzlib.h>

#include "internals.h"

libspectrum_error
libspectrum_bzip2_inflate( const libspectrum_byte *bzptr, size_t bzlength,
			   libspectrum_byte **outptr, size_t *outlength )
{
  int error;
  unsigned int length2;

  /* Known length, so we can use the easy method */
  if( *outlength ) {

    (*outptr) = malloc( *outlength );
    if( !(*outptr) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "out of memory at %s:%d", __FILE__, __LINE__ );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    length2 = *outlength;

    /* The cast here is necessary as libbz2 doesn't declare its input
       buffers as being const */
    error = BZ2_bzBuffToBuffDecompress( *outptr, &length2,
					(libspectrum_byte*)bzptr, bzlength,
					0, 0 );
    if( error != BZ_OK ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			       "error decompressing bzip data" );
      return LIBSPECTRUM_ERROR_LOGIC;
    }

    *outlength = length2;

    return LIBSPECTRUM_ERROR_NONE;

  } else {			/* Unknown length, have to stream */

    bz_stream stream;
    libspectrum_byte *ptr; size_t length;

    length = bzlength;

    *outptr = malloc( length );
    if( !(*outptr) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "out of memory at %s:%d", __FILE__, __LINE__ );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    /* Use standard memory allocation/free routines */
    stream.bzalloc = NULL; stream.bzfree = NULL; stream.opaque = NULL;

    error = BZ2_bzDecompressInit( &stream, 0, 0 );
    if( error != BZ_OK ) {
      switch( error ) {

      case BZ_MEM_ERROR:
	libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				 "out of memory at %s:%d",
				 __FILE__, __LINE__ );
	free( *outptr );
	return LIBSPECTRUM_ERROR_MEMORY;

      default:
	libspectrum_print_error(
          LIBSPECTRUM_ERROR_LOGIC,
	  "bzip2_inflate: serious error from BZ2_bzDecompressInit: %d", error
	);
	free( *outptr );
	return LIBSPECTRUM_ERROR_LOGIC;

      }
    }

    /* Cast necessary as libbz2 doesn't declare its input buffers as const */
    stream.next_in = (libspectrum_byte*)bzptr; stream.avail_in = bzlength;
    stream.next_out = *outptr; stream.avail_out = bzlength;

    while( 1 ) {

      error = BZ2_bzDecompress( &stream );

      switch( error ) {

      case BZ_STREAM_END:	/* Finished decompression */
	error = BZ2_bzDecompressEnd( &stream );
	if( error ) {
	  libspectrum_print_error(
	    LIBSPECTRUM_ERROR_LOGIC,
	    "bzip2_inflate: error from BZ2_bzDecompressEnd: %d", error
	  );
	  free( *outptr );
	  return LIBSPECTRUM_ERROR_LOGIC;
	}
	*outlength = stream.total_out_lo32;
	*outptr = realloc( *outptr, *outlength );
	return LIBSPECTRUM_ERROR_NONE;

      case BZ_OK:		/* More output space required */

	length += bzlength;
	ptr = realloc( *outptr, length );
	if( !ptr ) {
	  libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
				   "out of memory at %s:%d",
				   __FILE__, __LINE__ );
	  BZ2_bzDecompressEnd( &stream );
	  free( *outptr );
	  return LIBSPECTRUM_ERROR_MEMORY;
	}
	*outptr = ptr;
	stream.next_out = *outptr + stream.total_out_lo32;
	stream.avail_out += bzlength;
	break;

      default:
	libspectrum_print_error(
	  LIBSPECTRUM_ERROR_LOGIC,
	  "bzip2_inflate: serious error from BZ2_bzDecompress: %d", error
	);
	BZ2_bzDecompressEnd( &stream );
	free( *outptr );
	return LIBSPECTRUM_ERROR_LOGIC;
      }
    }				/* Matches while( 1 ) { ... } */

  }				/* Matches if( *outlength ) { ... } */
}

#endif				/* #ifdef HAVE_LIBBZ2 */
