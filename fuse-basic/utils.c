/* utils.c: generic utility routines
   Copyright (c) 2004 Philip Kendall

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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "basic.h"
#include "utils.h"

int
utils_read_file( char **buffer, size_t *length, const char *filename )
{
  int fd;
  struct stat file_info;

  fd = open( filename, O_RDONLY );
  if( fd == -1 ) {
    fprintf( stderr, "%s: couldn't open '%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  if( fstat( fd, &file_info ) ) {
    fprintf( stderr, "%s: couldn't stat '%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  *length = file_info.st_size;
  *buffer = malloc( *length * sizeof **buffer );
  if( !*buffer ) {
    fprintf( stderr, "%s: out of memory at %s:%d\n", progname, __FILE__,
	     __LINE__ );
    return 1;
  }

  if( read( fd, *buffer, *length ) != *length ) {
    fprintf( stderr, "%s: error reading from '%s'\n", progname, filename );
    return 1;
  }

  if( close( fd ) ) {
    fprintf( stderr, "%s: error closing '%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  return 0;
}

