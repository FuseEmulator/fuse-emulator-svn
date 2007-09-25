#include "test.h"

test_return_t
check_edges( const char *filename, test_edge_sequence_t *edges )
{
  libspectrum_byte *buffer = NULL;
  size_t filesize = 0;
  libspectrum_tape *tape;
  test_return_t r = TEST_FAIL;
  test_edge_sequence_t *ptr = edges;

  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  if( libspectrum_tape_alloc( &tape ) ) {
    free( buffer );
    return TEST_INCOMPLETE;
  }

  if( libspectrum_tape_read( tape, buffer, filesize, LIBSPECTRUM_ID_UNKNOWN,
			     filename ) != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_tape_free( tape );
    free( buffer );
    return TEST_INCOMPLETE;
  }

  free( buffer );

  while( 1 ) {

    libspectrum_dword tstates;
    int flags;
    libspectrum_error e;

    e = libspectrum_tape_get_next_edge( &tstates, &flags, tape );
    if( e ) {
      libspectrum_tape_free( tape );
      return TEST_INCOMPLETE;
    }

    if( flags & LIBSPECTRUM_TAPE_FLAGS_STOP ) {
      if( ptr->length == 0 ) {
	r = TEST_PASS;
      } else {
	fprintf( stderr, "%s: expected end of tape, got %d tstates\n",
		 progname, tstates );
      }
      break;
    }

    if( tstates != ptr->length ) {
      fprintf( stderr, "%s: expected %d tstates, got %d tstates\n", progname,
	       ptr->length, tstates );
      break;
    }

    if( --ptr->count == 0 ) ptr++;
  }

  if( libspectrum_tape_free( tape ) ) return TEST_INCOMPLETE;

  return r;
}

  
