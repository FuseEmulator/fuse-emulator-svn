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

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

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

/* Fuse's DSA key */
libspectrum_rzx_dsa_key rzx_key = {
  "9E140C4CEA9CA011AA8AD17443CB5DC18DC634908474992D38AB7D4A27038CBB209420BA2CAB8508CED35ADF8CBD31A0A034FC082A168A0E190FFC4CCD21706F", /* p */
  "C52E9CA1804BD021FFAD30E8FB89A94437C2E4CB",	       /* q */
  "90E56D9493DE80E1A35F922007357888A1A47805FD365AD27BC5F184601EBC74E44F576AA4BF8C5244D202BBAE697C4F9132DFB7AD0A56892A414C96756BD21A", /* g */
  "7810A35AC94EA5750934FB9C922351EE597C71E2B83913C121C6655EA25CE7CBE2C259FA3168F8475B2510AA29C5FEB50ACAB25F34366C2FFC93B3870A522232", /* y */
  NULL						       /* x */
};

int
main( int argc, char **argv )
{
  unsigned char *buffer; size_t length;

  const char *rzxfile;

  libspectrum_rzx *rzx;
  libspectrum_snap *snap = NULL;
  libspectrum_rzx_signature signature;
  libspectrum_error error;

  progname = argv[0];

  if( libspectrum_init() ) return 16;

  if( argc < 2 ) {
    fprintf( stderr, "%s: usage: %s <rzxfile>\n", progname, progname );
    return 2;
  }
  rzxfile = argv[1];

  if( libspectrum_rzx_alloc( &rzx ) ) return 16;

  if( mmap_file( rzxfile, &buffer, &length ) ) return 16;

  if( libspectrum_rzx_read( rzx, &snap, buffer, length, &signature ) ) {
    munmap( buffer, length );
    return 16;
  }

  if( snap ) libspectrum_snap_free( snap );

  if( !signature.start ) {
    printf( "%s: no signature found in '%s'\n", progname, rzxfile );
    libspectrum_rzx_free( rzx );
    munmap( buffer, length );
    return 0;
  }

  if( signature.key_id != 0x0a522232 ) {
    printf( "%s: don't know anything about key ID 0x%08x\n", progname,
	    signature.key_id );
    libspectrum_rzx_free( rzx );
    munmap( buffer, length );
    return 0;
  }

  error = libspectrum_verify_signature( &signature, &rzx_key );
  if( error && error != LIBSPECTRUM_ERROR_SIGNATURE ) {
    libspectrum_rzx_free( rzx ); return 16;
  }

  if( munmap( buffer, length ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, rzxfile,
	     strerror( errno ) );
    libspectrum_rzx_free( rzx );
    return 16;
  }

  libspectrum_rzx_free( rzx );

  if( error == LIBSPECTRUM_ERROR_SIGNATURE ) {
    printf( "%s: BAD signature with key 0x%08x on '%s'\n", progname,
	    signature.key_id, rzxfile );
    return 1;
  } else {
    printf( "%s: good signature with key 0x%08x on '%s'\n", progname,
	    signature.key_id, rzxfile );
    return 0;
  }

}
