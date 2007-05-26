/* rzxcheck.c: Check the signature on an RZX file
   Copyright (c) 2002-2003 Philip Kendall

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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

char *progname;			/* argv[0] */

int
main( int argc, char **argv )
{
  unsigned char *buffer; size_t length;

  const char *rzxfile;

  libspectrum_rzx *rzx;

  libspectrum_error error;
  libspectrum_dword keyid = 0;
  libspectrum_signature signature;
  struct rzx_key *key;

  progname = argv[0];

  if( init_libspectrum() ) return 16;

  if( argc < 2 ) {
    fprintf( stderr, "%s: usage: %s <rzxfile>\n", progname, progname );
    return 2;
  }
  rzxfile = argv[1];

  if( libspectrum_rzx_alloc( &rzx ) ) return 16;

  if( mmap_file( rzxfile, &buffer, &length ) ) return 16;

  if( libspectrum_rzx_read( rzx, buffer, length ) ) {
    munmap( buffer, length );
    return 16;
  }

  keyid = libspectrum_rzx_get_keyid( rzx );
  if( !keyid ) {
    printf( "%s: no key ID found in '%s'\n", progname, rzxfile );
    libspectrum_rzx_free( rzx );
    munmap( buffer, length );
    return 16;
  }

  for( key = known_keys; key->id; key++ )
    if( keyid == key->id ) break;

  if( !key->id ) {
    printf( "%s: don't know anything about key ID %08x\n", progname,
	    keyid );
    libspectrum_rzx_free( rzx );
    munmap( buffer, length );
    return 16;
  }

  error = libspectrum_rzx_get_signature( rzx, &signature );
  if( error ) {
    libspectrum_rzx_free( rzx );
    munmap( buffer, length );
    return 16;
  }

  error = libspectrum_verify_signature( &signature, &( key->key ) );
  if( error && error != LIBSPECTRUM_ERROR_SIGNATURE ) {
    libspectrum_signature_free( &signature );
    libspectrum_rzx_free( rzx );
    munmap( buffer, length );
    return 16;
  }

  if( munmap( buffer, length ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, rzxfile,
	     strerror( errno ) );
    libspectrum_signature_free( &signature );
    libspectrum_rzx_free( rzx );
    return 16;
  }

  libspectrum_rzx_free( rzx ); free( rzx );
  libspectrum_signature_free( &signature );

  if( error == LIBSPECTRUM_ERROR_SIGNATURE ) {
    printf( "%s: BAD signature with key %08x (%s) in '%s'\n", progname,
	    key->id, key->description, rzxfile );
    return 1;
  } else {
    printf( "%s: good signature with key %08x (%s) in '%s'\n", progname,
	    key->id, key->description, rzxfile );
    return 0;
  }

}
