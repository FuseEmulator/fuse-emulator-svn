/* internals.h: functions which need to be called inter-file by libspectrum
                routines, but not by user code
   Copyright (c) 2001-2002 Philip Kendall, Darren Salt

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

#ifndef LIBSPECTRUM_INTERNALS_H
#define LIBSPECTRUM_INTERNALS_H

#ifdef HAVE_LIB_GLIB		/* Only if we've got the real glib */
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#ifndef LIBSPECTRUM_LIBSPECTRUM_H
#include "libspectrum.h"
#endif				/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */

#ifdef __GNUC__
#define GCC_UNUSED __attribute__ ((unused))
#else				/* #ifdef __GNUC__ */
#define GCC_UNUSED
#endif				/* #ifdef __GNUC__ */

/* Win32 systems have _strcmpi, but not strcasecmp */
#if !defined(HAVE_STRCASECMP) && defined(HAVE__STRCMPI)
#define strcasecmp _strcmpi
#endif		/* #if !defined(HAVE_STRCASECMP) && defined(HAVE__STRCMPI) */

/* Print an error to stdout */
libspectrum_error libspectrum_print_error( const char *format, ... );

/* Acquire more memory for a buffer */
int libspectrum_make_room( libspectrum_byte **dest, size_t requested,
			   libspectrum_byte **ptr, size_t *allocated );

/* Read and write (d)words */
libspectrum_dword libspectrum_read_dword( const libspectrum_byte **buffer );
int libspectrum_write_word( libspectrum_byte **buffer, libspectrum_word w );
int libspectrum_write_dword( libspectrum_byte **buffer, libspectrum_dword d );

/* zlib (de)compression routines */

libspectrum_error
libspectrum_zlib_inflate( const libspectrum_byte *gzptr, size_t gzlength,
			  libspectrum_byte **outptr, size_t *outlength );
libspectrum_error
libspectrum_zlib_compress( const libspectrum_byte *data, size_t length,
			   libspectrum_byte **gzptr, size_t *gzlength );

/* Convert a 48K memory dump into separate RAM pages */

int libspectrum_split_to_48k_pages( libspectrum_snap *snap,
				    const libspectrum_byte* data );

/* Crypto functions */

libspectrum_error
libspectrum_sign_data( libspectrum_byte **signature, size_t *signature_length,
		       libspectrum_byte *data, size_t data_length,
		       libspectrum_rzx_dsa_key *key );
libspectrum_error
libspectrum_verify_signature( const libspectrum_byte *signature,
			      size_t signature_length,
			      const libspectrum_byte *data, size_t length,
			      libspectrum_rzx_dsa_key *key );

#endif				/* #ifndef LIBSPECTRUM_INTERNALS_H */
