/* make-sed.c: Generate a sed script to create the libspectrum_* typedefs
   Copyright (c) 2002-2003 Philip Kendall, Darren Salt

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

#include <stdio.h>

int main(void)
{
  /*
   * Define the integer types
   */

  printf( "if( /LIBSPECTRUM_DEFINE_TYPES/ ) {\n\n  $_ = << \"CODE\";\n" );

#ifdef HAVE_STDINT_H

  printf( "#include <stdint.h>\n\n" );

  printf( "typedef  uint8_t libspectrum_byte;\n" );
  printf( "typedef   int8_t libspectrum_signed_byte;\n" );

  printf( "typedef uint16_t libspectrum_word;\n" );
  printf( "typedef  int16_t libspectrum_signed_word;\n" );

  printf( "typedef uint32_t libspectrum_dword;\n" );
  printf( "typedef  int32_t libspectrum_signed_dword;\n" );

  printf( "typedef uint64_t libspectrum_qword;\n" );
  printf( "typedef  int64_t libspectrum_signed_qword;\n" );

#else				/* #ifdef HAVE_STDINT_H */

  if( sizeof( char ) == 1 ) {
    printf( "typedef unsigned char libspectrum_byte;\n" );
    printf( "typedef   signed char libspectrum_signed_byte;\n" );
  } else if( sizeof( short ) == 1 ) {
    printf( "typedef unsigned short libspectrum_byte;\n" );
    printf( "typedef   signed short libspectrum_signed_byte;\n" );
  } else {
    fprintf( stderr, "No plausible 8 bit types found\n" );
    return 1;
  }

  if( sizeof( short ) == 2 ) {
    printf( "typedef unsigned short libspectrum_word;\n" );
    printf( "typedef   signed short libspectrum_signed_word;\n" );
  } else if( sizeof( int ) == 2 ) {
    printf( "typedef unsigned int libspectrum_word;\n" );
    printf( "typedef   signed int libspectrum_signed_word;\n" );
  } else {
    fprintf( stderr, "No plausible 16 bit types found\n" );
    return 1;
  }

  if( sizeof( int ) == 4 ) {
    printf( "typedef unsigned int libspectrum_dword;\n" );
    printf( "typedef   signed int libspectrum_signed_dword;\n" );
  } else if( sizeof( long ) == 4 ) {
    printf( "typedef unsigned long libspectrum_dword;\n" );
    printf( "typedef   signed long libspectrum_signed_dword;\n" );
  } else {
    fprintf( stderr, "No plausible 32 bit types found\n" );
    return 1;
  }

  if( sizeof( long ) == 8 ) {
    printf( "typedef unsigned long libspectrum_qword;\n" );
    printf( "typedef   signed long libspectrum_signed_qword;\n" );
  } else if( sizeof( long long ) == 8 ) {
    printf( "typedef unsigned long long libspectrum_qword;\n" );
    printf( "typedef   signed long long libspectrum_signed_qword;\n" );
  } else {
    fprintf( stderr, "No plausible 64 bit types found\n" );
    return 1;
  }

#endif				/* #ifdef HAVE_STDINT_H */

  printf( "CODE\n}\n\n" );

  /*
   * Get glib or our replacement for it
   */
   
  printf( "if( /LIBSPECTRUM_GLIB_REPLACEMENT/ ) {\n\n" );

#ifdef HAVE_LIB_GLIB		/* #ifdef HAVE_LIB_GLIB */

  printf( "  $_ = \"#include <glib.h>\\n\"\n\n" );

#else				/* #ifdef HAVE_LIB_GLIB */

  printf( "  $_ = << \"CODE\";\n"
"typedef int gint;\n"
"typedef unsigned int guint;\n"
"typedef const void * gconstpointer;\n"
"typedef void * gpointer;\n"
"\n"
"typedef struct _GSList GSList;\n"
"\n"
"struct _GSList {\n"
"    gpointer data;\n"
"    GSList *next;\n"
"};\n"
"\n"
"typedef void		(*GFunc)		(gpointer	data,\n"
"						 gpointer	user_data);\n"
"\n"
"typedef gint		(*GCompareFunc)		(gconstpointer	a,\n"
"						 gconstpointer	b);\n"
"\n"
"\n"
"GSList* g_slist_insert_sorted	(GSList		*list,\n"
"				 gpointer	 data,\n"
"				 GCompareFunc	 func);\n"
"\n"
"GSList* g_slist_append		(GSList		*list,\n"
"				 gpointer	 data);\n"
"\n"
"GSList* g_slist_remove		(GSList		*list,\n"
"				 gpointer	 data);\n"
"\n"
"void	g_slist_foreach		(GSList		*list,\n"
"				 GFunc		 func,\n"
"				 gpointer	 user_data);\n"
"\n"
"void	g_slist_free		(GSList		*list);\n"
"\n"
"GSList* g_slist_nth		(GSList		*list,\n"
"				 guint		n);\n"
"\n"
"GSList* g_slist_find_custom	(GSList		*list,\n"
"				 gpointer	data,\n"
"				 GCompareFunc	func );\n"
"\n"
"gint	g_slist_position	(GSList		*list,\n"
"				 GSList		*llink);\n"
"CODE\n" );

#endif				/* #ifdef HAVE_LIB_GLIB */

  printf( "}\n\n" );

  /*
   * If we have libgcrypt, include the header file and store the signature
   * parameters
   */

  printf( "if( /LIBSPECTRUM_INCLUDE_GCRYPT/ ) {\n\n" );

#ifdef HAVE_GCRYPT_H

  printf( "  $_ = \"#include <gcrypt.h>\\n\";\n\n" );

#else

  printf( "  $_ = '';\n\n" );

#endif

  printf( "}\n\n" );

  printf( "if( /LIBSPECTRUM_SIGNATURE_PARAMETERS/ ) {\n\n" );

#ifdef HAVE_GCRYPT_H

  printf( "  $_ = \"  /* The DSA signature parameters 'r' and 's' */\\n  GcryMPI r, s;\\n\";\n\n" );

#else

  printf( "  $_ = \"/* Signature parameters not stored as libgcrypt is not present */\n\";\n\n" );

#endif

  printf( "}\n\n" );

  return 0;
}
