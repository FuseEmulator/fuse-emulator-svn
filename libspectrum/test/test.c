#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "libspectrum.h"

const char *progname;
const char *LIBSPECTRUM_MIN_VERSION = "0.3.0.1";

typedef enum test_return_t {
  TEST_PASS,
  TEST_FAIL,
  TEST_INCOMPLETE,
} test_return_t;

typedef test_return_t (*test_fn)( void );

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int
read_file( libspectrum_byte **buffer, size_t *length, const char *filename )
{
  int fd;
  struct stat info;
  ssize_t bytes;

  fd = open( filename, O_RDONLY | O_BINARY );
  if( fd == -1 ) {
    fprintf( stderr, "%s: couldn't open `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return errno;
  }

  if( fstat( fd, &info ) ) {
    fprintf( stderr, "%s: couldn't stat `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return errno;
  }

  *length = info.st_size;

  *buffer = malloc( *length );
  if( !*buffer ) {
    fprintf( stderr, "%s: out of memory allocating %lu bytes at %s:%d\n",
	     progname, (unsigned long)*length, __func__, __LINE__ );
    return -ENOMEM;
  }

  bytes = read( fd, *buffer, *length );
  if( bytes == -1 ) {
    fprintf( stderr, "%s: error reading from `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return errno;
  } else if( bytes < *length ) {
    fprintf( stderr, "%s: read only %lu of %lu bytes from `%s'\n", progname,
	     (unsigned long)bytes, (unsigned long)*length, filename );
    return 1;
  }

  if( close( fd ) ) {
    fprintf( stderr, "%s: error closing `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return errno;
  }

  return 0;
}

/* Test for bugs #1479451 and #1706994: tape object incorrectly freed
   after reading invalid tape */
static test_return_t
test_1( void )
{
  libspectrum_byte *buffer = NULL;
  size_t filesize = 0;
  libspectrum_tape *tape;
  const char *filename = "invalid.tzx";

  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  if( libspectrum_tape_alloc( &tape ) ) {
    free( buffer );
    return TEST_INCOMPLETE;
  }

  if( libspectrum_tape_read( tape, buffer, filesize, LIBSPECTRUM_ID_UNKNOWN,
			     filename ) != LIBSPECTRUM_ERROR_UNKNOWN ) {
    fprintf( stderr, "%s: reading `%s' did not give expected result of LIBSPECTRUM_ERROR_UNKNOWN\n",
	     progname, filename );
    libspectrum_tape_free( tape );
    free( buffer );
    return TEST_INCOMPLETE;
  }

  free( buffer );

  if( libspectrum_tape_free( tape ) ) return TEST_INCOMPLETE;

  return TEST_PASS;
}

/* Test for bugs #1720238: TZX turbo blocks with zero pilot pulses and
   #1720270: freeing a turbo block with no data produces segfault */
static test_return_t
test_2( void )
{
  libspectrum_byte *buffer = NULL;
  size_t filesize = 0;
  libspectrum_tape *tape;
  const char *filename = "turbo-zeropilot.tzx";
  libspectrum_dword tstates;
  int flags;

  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  if( libspectrum_tape_alloc( &tape ) ) {
    free( buffer );
    return TEST_INCOMPLETE;
  }

  if( libspectrum_tape_read( tape, buffer, filesize, LIBSPECTRUM_ID_UNKNOWN,
			     filename ) ) {
    libspectrum_tape_free( tape );
    free( buffer );
    return TEST_INCOMPLETE;
  }

  free( buffer );

  if( libspectrum_tape_get_next_edge( &tstates, &flags, tape ) ) {
    libspectrum_tape_free( tape );
    return TEST_INCOMPLETE;
  }

  if( flags ) {
    fprintf( stderr, "%s: reading first edge of `%s' gave unexpected flags 0x%04x; expected 0x0000\n",
	     progname, filename, flags );
    libspectrum_tape_free( tape );
    return TEST_FAIL;
  }

  if( tstates != 667 ) {
    fprintf( stderr, "%s: first edge of `%s' was %d tstates; expected 667\n",
	     progname, filename, tstates );
    libspectrum_tape_free( tape );
    return TEST_FAIL;
  }

  if( libspectrum_tape_free( tape ) ) return TEST_INCOMPLETE;

  return TEST_PASS;
}

/* Test for bug #1725864: writing empty .tap file causes crash */
static test_return_t
test_3( void )
{
  libspectrum_tape *tape;
  libspectrum_byte *buffer = (libspectrum_byte*)1;
  size_t length = 0;

  if( libspectrum_tape_alloc( &tape ) ) return TEST_INCOMPLETE;

  if( libspectrum_tape_write( &buffer, &length, tape, LIBSPECTRUM_ID_TAPE_TAP ) ) {
    libspectrum_tape_free( tape );
    return TEST_INCOMPLETE;
  }

  /* `buffer' should now have been set to NULL */
  if( buffer ) {
    fprintf( stderr, "%s: `buffer' was not NULL after libspectrum_tape_write()\n", progname );
    libspectrum_tape_free( tape );
    return TEST_FAIL;
  }

  if( libspectrum_tape_free( tape ) ) return TEST_INCOMPLETE;

  return TEST_PASS;
}

/* Test for bug #1753279: invalid compressed file causes crash */
static test_return_t
test_4( void )
{
  libspectrum_byte *buffer = NULL; 
  size_t filesize;
  libspectrum_snap *snap;
  const char *filename = "invalid.gz";

  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  if( libspectrum_snap_alloc( &snap ) ) {
    free( buffer );
    return TEST_INCOMPLETE;
  }

  if( libspectrum_snap_read( snap, buffer, filesize, LIBSPECTRUM_ID_UNKNOWN,
			     filename ) != LIBSPECTRUM_ERROR_UNKNOWN ) {
    fprintf( stderr, "%s: reading `%s' did not give expected result of LIBSPECTRUM_ERROR_UNKNOWN\n",
	     progname, filename );
    libspectrum_snap_free( snap );
    free( buffer );
    return TEST_INCOMPLETE;
  }

  free( buffer );

  if( libspectrum_snap_free( snap ) ) return TEST_INCOMPLETE;

  return TEST_PASS;
}

/* Further test for bug #1753279: invalid compressed file causes crash */
static test_return_t
test_5( void )
{
  libspectrum_byte *buffer = NULL; 
  size_t filesize;
  libspectrum_snap *snap;
  const char *filename = "invalid.gz";

  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  if( libspectrum_snap_alloc( &snap ) ) {
    free( buffer );
    return TEST_INCOMPLETE;
  }

  if( libspectrum_snap_read( snap, buffer, filesize, LIBSPECTRUM_ID_UNKNOWN,
			     NULL ) != LIBSPECTRUM_ERROR_UNKNOWN ) {
    fprintf( stderr, "%s: reading `%s' did not give expected result of LIBSPECTRUM_ERROR_UNKNOWN\n",
	     progname, filename );
    libspectrum_snap_free( snap );
    free( buffer );
    return TEST_INCOMPLETE;
  }

  free( buffer );

  if( libspectrum_snap_free( snap ) ) return TEST_INCOMPLETE;

  return TEST_PASS;
}

/* Test for bug #1753938: pointer wraparound causes segfault */
static test_return_t
test_6( void )
{
  libspectrum_byte *buffer = NULL; 
  size_t filesize;
  libspectrum_snap *snap;
  const char *filename = "invalid.szx";

  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  if( libspectrum_snap_alloc( &snap ) ) {
    free( buffer );
    return TEST_INCOMPLETE;
  }

  if( libspectrum_snap_read( snap, buffer, filesize, LIBSPECTRUM_ID_SNAPSHOT_SZX,
			     filename ) != LIBSPECTRUM_ERROR_CORRUPT ) {
    fprintf( stderr, "%s: reading `%s' did not give expected result of LIBSPECTRUM_ERROR_CORRUPT\n",
	     progname, filename );
    libspectrum_snap_free( snap );
    free( buffer );
    return TEST_INCOMPLETE;
  }

  free( buffer );

  if( libspectrum_snap_free( snap ) ) return TEST_INCOMPLETE;

  return TEST_PASS;
}

/* Test for bug #1755124: lack of sanity check in GDB code */
static test_return_t
test_7( void )
{
  libspectrum_byte *buffer = NULL; 
  size_t filesize;
  libspectrum_tape *tape;
  const char *filename = "invalid-gdb.tzx";

  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  if( libspectrum_tape_alloc( &tape ) ) {
    free( buffer );
    return TEST_INCOMPLETE;
  }

  if( libspectrum_tape_read( tape, buffer, filesize, LIBSPECTRUM_ID_TAPE_TZX,
			     filename ) != LIBSPECTRUM_ERROR_CORRUPT ) {
    fprintf( stderr, "%s: reading `%s' did not give expected result of LIBSPECTRUM_ERROR_CORRUPT\n",
	     progname, filename );
    libspectrum_tape_free( tape );
    free( buffer );
    return TEST_INCOMPLETE;
  }

  free( buffer );

  if( libspectrum_tape_free( tape ) ) return TEST_INCOMPLETE;

  return TEST_PASS;
}

static test_fn tests[] = {
  test_1,
  test_2,
  test_3,
  test_4,
  test_5,
  test_6,
  test_7,
  NULL
};

int
main( int argc, char *argv[] )
{
  test_fn *test;
  int count;
  int pass = 0, fail = 0, incomplete = 0;

  progname = argv[0];

  if( libspectrum_check_version( LIBSPECTRUM_MIN_VERSION ) ) {
    if( libspectrum_init() ) return 2;
  } else {
    fprintf( stderr, "%s: libspectrum version %s found, but %s required",
	     progname, libspectrum_version(), LIBSPECTRUM_MIN_VERSION );
    return 2;
  }

  for( test = tests, count = 0;
       *test;
       test++, count++ ) {
    switch( (*test)() ) {
    case TEST_PASS:
      printf( "Test %d passed\n", count + 1 );
      pass++;
      break;
    case TEST_FAIL:
      printf( "Test %d FAILED\n", count + 1 );
      fail++;
      break;
    case TEST_INCOMPLETE:
      printf( "Test %d NOT COMPLETE\n", count + 1 );
      incomplete++;
      break;
    }
  }

  printf( "\n%3d tests run\n\n", count );
  printf( "%3d     passed (%6.2f%%)\n", pass, 100 * (float)pass/count );
  printf( "%3d     failed (%6.2f%%)\n", pass, 100 * (float)fail/count );
  printf( "%3d incomplete (%6.2f%%)\n", pass, 100 * (float)incomplete/count );

  if( fail == 0 && incomplete == 0 ) {
    return 0;
  } else {
    return 1;
  }
}
