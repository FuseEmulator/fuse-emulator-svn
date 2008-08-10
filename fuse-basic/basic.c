/* main.c: placeholder */

#include "config.h"

#include <stdio.h>

#include <glib.h>

#include "basic.h"
#include "parse.h"
#include "program.h"
#include "utils.h"

const char *progname;

static int
interpret_program( struct program *program )
{
  if( program_find_functions( program ) ) {
    fprintf( stderr, "%s: error finding functions\n", progname );
    return 1;
  }

  while( !program->error ) {
    if( program_execute_statement( program ) ) {
      program_free( program );
      return 1;
    }
  }

  printf( "%s\n", program_strerror( program->error ) );

  return 0;
}

int
main( int argc, char **argv )
{
  char *buffer; size_t length;
  struct program *basic_program;
  int error;

  progname = argv[0];

  if( argc < 2 ) {
    fprintf( stderr, "%s: usage: %s <basic file>\n", progname, progname );
    return 1;
  }

  error = utils_read_file( &buffer, &length, argv[1] );
  if( error ) return error;

  basic_program = parse_program( buffer, length );
  if( !basic_program ) return 1;

  free( buffer );

  error = interpret_program( basic_program );
  if( error ) { program_free( basic_program ); return error; }

  if( program_free( basic_program ) ) {
    fprintf( stderr, "%s: error freeing program\n", progname );
    return 1;
  }

  return 0;
}
