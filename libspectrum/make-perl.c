/* make-sed.c: Generate a sed script to create the libspectrum_* typedefs
   Copyright (c) 2002 Philip Kendall, Darren Salt

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

  printf( "CODE\n}\n" );

  printf( "\nif( /LIBSPECTRUM_GLIB_REPLACEMENT/ ) {\n\n" );

#ifdef HAVE_LIB_GLIB

  printf( "  $_ = \"#include <glib.h>\";\n\n" );

#else				/* #ifdef HAVE_LIB_GLIB */

  printf( "  $_ = << \"CODE\";\n"
"/* Glib emulation */\n"
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

  printf( "}\n" );

  printf( "\nif( /LIBSPECTRUM_SNAP_ACCESSORS/ ) {\n\n"
"  open( DATAFILE, \"< snap_accessors.txt\" ) or die \"Couldn't open `snap_accessors.txt': $!\";\n\n"
"  $_ = '';\n"
"  while( <DATAFILE> ) {\n\n"
"    next if /^\\s*$/; next if /^\\s*#/;\n\n"
"    my( $type, $name, $indexed ) = split;\n\n"
"    if( $indexed ) {\n\n"
"	print << \"CODE\";\n"
"$type libspectrum_snap_$name( libspectrum_snap *snap, int idx );\n"
"void libspectrum_snap_set_$name( libspectrum_snap *snap, int idx, $type $name );\n"
"CODE\n\n"
"    } else {\n\n"
"	print << \"CODE\";\n"
"$type libspectrum_snap_$name( libspectrum_snap *snap );\n"
"void libspectrum_snap_set_$name( libspectrum_snap *snap, $type $name );\n"
"CODE\n\n"
"    }\n"
"  }\n"
"}\n" );

  return 0;
}
