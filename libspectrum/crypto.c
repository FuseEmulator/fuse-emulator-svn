/* crypto.c: crytography-related functions
   Copyright (c) 2002 Philip Kendall

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

#ifdef HAVE_GCRYPT_H

#include <gcrypt.h>

#include "internals.h"

static const char *private_key_format =
  "(key-data (private-key (dsa (p %m) (q %m) (g %m) (y %m) (x %m))))";
static const char *public_key_format =
  "(key-data (public-key (dsa (p %m) (q %m) (g %m) (y %m))))";
static const char *signature_format = "(sig-val (dsa (r %m) (s %m)))";

#define HASH_ALGORITHM GCRY_MD_SHA1
#define MPI_COUNT 5

static libspectrum_error
get_hash( GcrySexp *hash, const libspectrum_byte *data, size_t data_length );
static libspectrum_error
create_key( GcrySexp *s_key, libspectrum_rzx_dsa_key *key, const char *format);
static void free_mpis( GcryMPI *mpis, size_t n );
static libspectrum_error get_mpi( GcryMPI *mpi, GcrySexp signature,
				  const char *token );
static libspectrum_error
serialise_mpis( libspectrum_byte **signature, size_t *signature_length,
		GcryMPI r, GcryMPI s );

libspectrum_error
libspectrum_sign_data( libspectrum_byte **signature, size_t *signature_length,
		       libspectrum_byte *data, size_t data_length,
		       libspectrum_rzx_dsa_key *key )
{
  int error;
  GcrySexp hash, s_key, s_signature;
  GcryMPI r, s;

  error = get_hash( &hash, data, data_length ); if( error ) return error;

  error = create_key( &s_key, key, private_key_format );
  if( error ) { gcry_sexp_release( hash ); return error; }

  error = gcry_pk_sign( &s_signature, hash, s_key );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_sign_data: error signing data: %s",
			     gcry_strerror( error ) );
    gcry_sexp_release( s_key ); gcry_sexp_release( hash );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  gcry_sexp_release( s_key ); gcry_sexp_release( hash );

  error = get_mpi( &r, s_signature, "r" );
  if( error ) { gcry_sexp_release( s_signature ); return error; }
  error = get_mpi( &s, s_signature, "s" );
  if( error ) {
    gcry_sexp_release( s_signature ); gcry_mpi_release( r );
    return error;
  }

  gcry_sexp_release( s_signature );

  error = serialise_mpis( signature, signature_length, r, s );
  if( error ) { gcry_mpi_release( r ); gcry_mpi_release( s ); return error; }

  gcry_mpi_release( r ); gcry_mpi_release( s );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_hash( GcrySexp *hash, const libspectrum_byte *data, size_t data_length )
{
  int error;
  char *digest; size_t digest_length;
  GcryMPI hash_mpi;
  
  digest_length = gcry_md_get_algo_dlen( HASH_ALGORITHM );
  digest = malloc( digest_length );
  if( !digest ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "get_hash: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  gcry_md_hash_buffer( HASH_ALGORITHM, digest, data, data_length );

  error = gcry_mpi_scan( &hash_mpi, GCRYMPI_FMT_USG, digest, &digest_length );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_hash: error creating hash MPI: %s",
			     gcry_strerror( error )
    );
    free( digest );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  free( digest );

  error = gcry_sexp_build( hash, NULL, "(%m)", hash_mpi );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_hash: error creating hash sexp: %s",
			     gcry_strerror( error )
    );
    gcry_mpi_release( hash_mpi );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  gcry_mpi_release( hash_mpi );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
create_key( GcrySexp *s_key, libspectrum_rzx_dsa_key *key, const char *format )
{
  int error;
  size_t i;
  GcryMPI mpis[MPI_COUNT];

  for( i=0; i<MPI_COUNT; i++ ) mpis[i] = NULL;

               error = gcry_mpi_scan( &mpis[0], GCRYMPI_FMT_HEX, key->p, NULL);
  if( !error ) error = gcry_mpi_scan( &mpis[1], GCRYMPI_FMT_HEX, key->q, NULL);
  if( !error ) error = gcry_mpi_scan( &mpis[2], GCRYMPI_FMT_HEX, key->g, NULL);
  if( !error ) error = gcry_mpi_scan( &mpis[3], GCRYMPI_FMT_HEX, key->y, NULL);
  if( !error && key->x )
               error = gcry_mpi_scan( &mpis[4], GCRYMPI_FMT_HEX, key->x, NULL);

  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "create_key: error creating MPIs: %s",
			     gcry_strerror( error ) );
    free_mpis( mpis, MPI_COUNT );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  
  error = gcry_sexp_build( s_key, NULL, format,
			   mpis[0], mpis[1], mpis[2], mpis[3], mpis[4] );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "create_key: error creating key: %s",
			     gcry_strerror( error ) );
    free_mpis( mpis, MPI_COUNT );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  free_mpis( mpis, MPI_COUNT );

  /* FIXME: Use gcry_pk_testkey once it is able to check public keys
     as well as private keys */

  return LIBSPECTRUM_ERROR_NONE;
}

static void
free_mpis( GcryMPI *mpis, size_t n )
{
  size_t i;
  for( i=0; i<n; i++ ) if( mpis[i] ) gcry_mpi_release( mpis[i] );
}

static libspectrum_error
get_mpi( GcryMPI *mpi, GcrySexp sexp, const char *token )
{
  GcrySexp pair;

  pair = gcry_sexp_find_token( sexp, token, strlen( token ) );
  if( !pair ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_mpis: couldn't find '%s'", token );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  *mpi = gcry_sexp_nth_mpi( pair, 1, GCRYMPI_FMT_STD );
  if( !(*mpi) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_mpis: couldn't create MPI '%s'", token );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
serialise_mpis( libspectrum_byte **signature, size_t *signature_length,
		GcryMPI r, GcryMPI s )
{
  int error;
  size_t length, length_s;
  char *ptr;

  error = gcry_mpi_print( GCRYMPI_FMT_PGP, NULL, &length, r );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "serialise_mpis: length of r: %s",
			     gcry_strerror( error ) );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  error = gcry_mpi_print( GCRYMPI_FMT_PGP, NULL, &length_s, s );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "serialise_mpis: length of s: %s",
			     gcry_strerror( error ) );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  length += length_s; *signature_length = length;

  *signature = malloc( length );
  if( signature == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "serialise_mpis: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  error = gcry_mpi_print( GCRYMPI_FMT_PGP, *signature, &length, r );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "serialise_mpis: printing r: %s",
			     gcry_strerror( error ) );
    free( *signature );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  ptr = *signature + length; length = *signature_length - length;
  error = gcry_mpi_print( GCRYMPI_FMT_PGP, ptr, &length, s );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "serialise_mpis: printing s: %s",
			     gcry_strerror( error ) );
    free( *signature );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_verify_signature( libspectrum_rzx_signature *signature,
			      libspectrum_rzx_dsa_key *key )
{
  libspectrum_error error;
  GcrySexp hash, key_sexp, signature_sexp;

  error = get_hash( &hash, signature->start, signature->length );
  if( error ) return error;

  error = create_key( &key_sexp, key, public_key_format );
  if( error ) { gcry_sexp_release( hash ); return error; }

  error = gcry_sexp_build( &signature_sexp, NULL, signature_format,
			   signature->r, signature->s );

  if( error ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "create_signature: error building signature sexp: %s",
      gcry_strerror( error )
    );
    gcry_sexp_release( key_sexp ); gcry_sexp_release( hash );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  error = gcry_pk_verify( signature_sexp, hash, key_sexp );

  gcry_sexp_release( signature_sexp );
  gcry_sexp_release( key_sexp ); gcry_sexp_release( hash );

  if( error ) {
    if( error == GCRYERR_BAD_SIGNATURE ) {
      return LIBSPECTRUM_ERROR_SIGNATURE;
    } else {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
	"libgcrypt error verifying signature: %s",
	gcry_strerror( error )
      );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

#endif				/* #ifdef HAVE_GCRYPT_H */
