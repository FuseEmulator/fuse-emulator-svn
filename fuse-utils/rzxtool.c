/* rzxtool.c: (Un)compress RZX data and add, remove or extract embedded snaps
   Copyright (c) 2002-2004 Philip Kendall

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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>		/* Needed for strncasecmp() on QNX6 */
#endif				/* #ifdef HAVE_STRINGS_H */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

struct options {

  char *rzxfile;		/* The RZX file we'll operate on */

  char *add;			/* The snapshot to add */
  int remove;			/* Shall we remove the existing snap? */

  int uncompress;		/* Shall we uncompress the data? */

  int extract;			/* Shall we extract the snapshot? */

};

char *progname;			/* argv[0] */

void init_options( struct options *options );
int parse_options( int argc, char **argv, struct options *options );
int write_snapshot( libspectrum_snap *snap );
int make_snapshot( unsigned char **buffer, size_t *length,
		   libspectrum_snap *snap );

int
main( int argc, char **argv )
{
  unsigned char *buffer; size_t length;
  int error;

  libspectrum_rzx *rzx;
  libspectrum_creator *creator;

  struct options options;

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;

  init_options( &options );
  if( parse_options( argc, argv, &options ) ) return 1;

  /* Don't screw up people's terminals */
  if( isatty( STDOUT_FILENO ) ) {
    fprintf( stderr, "%s: won't output binary data to a terminal\n",
	     progname );
    return 1;
  }

  if( libspectrum_rzx_alloc( &rzx ) ) return 1;

  if( mmap_file( options.rzxfile, &buffer, &length ) ) return 1;

  if( libspectrum_rzx_read( rzx, buffer, length ) ) {
    munmap( buffer, length );
    return 1;
  }

  if( munmap( buffer, length ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname,
	     options.rzxfile, strerror( errno ) );
    return 1;
  }

  if( options.extract ) {

    libspectrum_snap *snap;

    error = libspectrum_rzx_start_playback( rzx, 0, &snap );
    if( error ) { libspectrum_rzx_free( rzx ); return error; }

    if( !snap ) {
      fprintf( stderr, "%s: no snapshot in `%s' to extract\n", progname,
	       options.rzxfile );
      libspectrum_rzx_free( rzx );
      return 1;
    }
    if( write_snapshot( snap ) ) {
      libspectrum_rzx_free( rzx );
      libspectrum_snap_free( snap );
      return 1;
    }

  } else {

    if( options.remove ) {
      /* FIXME: do something here */
    }

    if( options.add ) {
      /* FIXME: do something here */
    }

    if( get_creator( &creator, "rzxtool" ) ) {
      libspectrum_rzx_free( rzx );
      return 1;
    }

    length = 0;
    if( libspectrum_rzx_write( &buffer, &length, rzx, LIBSPECTRUM_ID_UNKNOWN,
			       creator, !options.uncompress, NULL ) ) {
      libspectrum_creator_free( creator );
      libspectrum_rzx_free( rzx );
      return 1;
    }

    libspectrum_creator_free( creator );

    /* And (finally!) write it */
    if( fwrite( buffer, 1, length, stdout ) != length ) {
      free( buffer );
      libspectrum_rzx_free( rzx );
      return 1;
    }

    free( buffer );

  }

  libspectrum_rzx_free( rzx );

  return 0;
}

void
init_options( struct options *options )
{
  options->rzxfile = NULL;

  options->add = NULL;
  options->remove = 0;
  options->uncompress = 0;
  options->extract = 0;
}

int
parse_options( int argc, char **argv, struct options *options )
{
  /* Defined by getopt */
  extern char *optarg;
  extern int optind;

  int c;

  int unknown = 0, output_rzx = 0, output_snapshot = 0;

  while( ( c = getopt( argc, argv, "a:ceru" ) ) != EOF ) 
    switch( c ) {
    case 'a': options->add = optarg;   break;
    case 'e': options->extract = 1;    break;
    case 'r': options->remove = 1;     break;
    case 'u': options->uncompress = 1; break;
    case '?': unknown = 1;	       break;
    }

  if( unknown ) return 1;

  if( options->add || options->remove || options->uncompress ) output_rzx = 1;
  if( options->extract ) output_snapshot = 1;

  if( output_rzx && output_snapshot ) {
    fprintf( stderr, "%s: can't output both a snapshot and a RZX file\n",
	     progname );
    return 1;
  }

  if( argv[optind] == NULL ) {
    fprintf( stderr, "%s: no RZX file given\n", progname );
    return 1;
  }

  options->rzxfile = argv[optind];

  return 0;
}

int
write_snapshot( libspectrum_snap *snap )
{
  unsigned char *buffer = NULL; size_t length = 0;
  int flags;

  if( libspectrum_snap_write( &buffer, &length, &flags, snap,
			      LIBSPECTRUM_ID_SNAPSHOT_Z80, NULL, 0 ) )
    return 1;
  
  if( fwrite( buffer, 1, length, stdout ) != length ) {
    fprintf( stderr, "%s: error writing output: %s\n", progname,
	     strerror( errno ) );
    free( buffer );
    return 1;
  }

  free( buffer );

  return 0;
}
