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

#include <stdio.h>

int main(void)
{
  printf( "s/LIBSPECTRUM_DEFINE_BYTE/typedef " );
  if( sizeof( unsigned char ) == 1 ) {
    printf( "unsigned  char" );
  } else if( sizeof( unsigned short ) == 1 ) {
    printf( "unsigned short" );
  } else {
    fprintf( stderr, "No plausible 8 bit types found\n" );
    return 1;
  }
  printf( " libspectrum_byte;/\n" );

  printf( "s/LIBSPECTRUM_DEFINE_WORD/typedef " );
  if( sizeof( unsigned short ) == 2 ) {
    printf( "unsigned short" );
  } else if( sizeof( unsigned int ) == 2 ) {
    printf( "unsigned   int" );
  } else {
    fprintf( stderr, "No plausible 16 bit types found\n" );
    return 1;
  }
  printf( " libspectrum_word;/\n" );

  printf( "s/LIBSPECTRUM_DEFINE_DWORD/typedef " );
  if( sizeof( unsigned int ) == 4 ) {
    printf( "unsigned   int" );
  } else if( sizeof( unsigned long ) == 4 ) {
    printf( "unsigned  long" );
  } else {
    fprintf( stderr, "No plausible 32 bit types found\n" );
    return 1;
  }
  printf( " libspectrum_dword;/\n" );

  return 0;
}
