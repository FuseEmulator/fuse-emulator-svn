/* utils.c: useful utility functions
   Copyright (c) 2002-2007 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/utsname.h>

#include <libspectrum.h>

#include "compat.h"
#include "utils.h"

extern char *progname;

/* The minimum version of libspectrum we need */
static const char *LIBSPECTRUM_MIN_VERSION = "0.2.0";

int
init_libspectrum( void )
{
  if( libspectrum_check_version( LIBSPECTRUM_MIN_VERSION ) ) {
    if( libspectrum_init() ) return 1;
  } else {
    fprintf( stderr, "libspectrum version %s found, but %s required",
	     libspectrum_version(), LIBSPECTRUM_MIN_VERSION );
    return 1;
  }

  return 0;
}

int
get_creator( libspectrum_creator **creator, const char *program )
{
  char *custom;
  int version[4] = { 0, 0, 0, 0 };
  struct utsname buf;
  libspectrum_error error; int sys_error;
  size_t i;

  sys_error = uname( &buf );
  if( sys_error == -1 ) {
    fprintf( stderr, "%s: error getting system information: %s\n", progname,
	     strerror( errno ) );
    return 1;
  }

  error = libspectrum_creator_alloc( creator );
  if( error ) return error;

  error = libspectrum_creator_set_program( *creator, program );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  sscanf( VERSION, "%u.%u.%u.%u",
	  &version[0], &version[1], &version[2], &version[3] );
  for( i=0; i<4; i++ ) if( version[i] > 0xff ) version[i] = 0xff;

  error = libspectrum_creator_set_major( *creator,
					 version[0] * 0x100 + version[1] );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  error = libspectrum_creator_set_minor( *creator,
					 version[2] * 0x100 + version[3] );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  custom = malloc( 256 );
  if( !custom ) {
    fprintf( stderr, "%s: out of memory at %s:%d\n", progname,
	     __FILE__, __LINE__ );
    libspectrum_creator_free( *creator );
    return 1;
  }

  snprintf( custom, 256, "libspectrum: %s\nuname: %s %s %s\n",
	    libspectrum_version(),
	    buf.sysname, buf.machine, buf.release );

  error = libspectrum_creator_set_custom( *creator,
					  (libspectrum_byte*)custom,
					  strlen( custom ) );
  if( error ) {
    free( custom ); libspectrum_creator_free( *creator );
    return error;
  }

  return 0;
}

int
mmap_file( const char *filename, unsigned char **buffer, size_t *length )
{
  int fd; struct stat file_info;
  
  if( ( fd = open( filename, O_RDONLY | O_BINARY ) ) == -1 ) {
    fprintf( stderr, "%s: couldn't open `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    fprintf( stderr, "%s: couldn't stat `%s': %s\n", progname, filename,
	     strerror( errno ) );
    close(fd);
    return 1;
  }

  (*length) = file_info.st_size;

  (*buffer) = mmap( 0, *length, PROT_READ, MAP_SHARED, fd, 0 );
  if( (*buffer) == (void*)-1 ) {
    fprintf( stderr, "%s: couldn't mmap `%s': %s\n", progname, filename,
	     strerror( errno ) );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    fprintf( stderr, "%s: couldn't close `%s': %s\n", progname, filename,
	     strerror( errno ) );
    munmap( *buffer, *length );
    return 1;
  }
  
  return 0;
}
