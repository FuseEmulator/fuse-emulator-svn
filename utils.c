/* utils.c: useful utility functions
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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <libspectrum.h>

extern char *progname;

int
get_creator( libspectrum_creator **creator, const char *program )
{
  libspectrum_error error;

  error = libspectrum_creator_alloc( creator );
  if( error ) return error;

  error = libspectrum_creator_set_program( *creator, program );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  error = libspectrum_creator_set_major( *creator, 0x0006 );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  error = libspectrum_creator_set_minor( *creator, 0x0100 );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  return 0;
}

int
mmap_file( const char *filename, unsigned char **buffer, size_t *length )
{
  int fd; struct stat file_info;
  
  if( ( fd = open( filename, O_RDONLY ) ) == -1 ) {
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
