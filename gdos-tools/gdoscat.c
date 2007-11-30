/* gdoscat.c: list the directory of a GDOS disk image

   Copyright (c) 2007 Stuart Brady

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   Stuart: sdbrady@ntlworld.com

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libgdos.h>

const char *progname;

void
printdir( libgdos_dirent *entry )
{
  char *typeinfo;
  char infobuf[ 80 ];
  int start=-1, length=-1;
  int i;

  typeinfo = infobuf;

  switch( entry->ftype ) {

  case libgdos_ftype_erased:
    typeinfo = "Erased";
    break;
  case libgdos_ftype_zx_basic:
    if( !( entry->ftypeinfo[9] & 0xc0 ) ) {
      snprintf( infobuf, sizeof( infobuf ),
		"ZX BASIC  %5i",
		entry->ftypeinfo[8] + entry->ftypeinfo[9] * 0x100 );
      typeinfo = infobuf;
    } else {
      typeinfo = "ZX BASIC";
    }
    break;
  case libgdos_ftype_zx_num:
    typeinfo = "ZX Num Arr";
    break;
  case libgdos_ftype_zx_str:
    typeinfo = "ZX Str Arr";
    break;
  case libgdos_ftype_zx_code:
    length = entry->ftypeinfo[2] + entry->ftypeinfo[3] * 0x100;
    start = entry->ftypeinfo[4] + entry->ftypeinfo[5] * 0x100;
    snprintf( infobuf, sizeof( infobuf ),
	      "ZX CODE  %6i,%i", start, length );
    typeinfo = infobuf;
    break;
  case libgdos_ftype_zx_snap48:
    typeinfo = "ZX SNP 48K";
    break;
  case libgdos_ftype_zx_microdrive:
    typeinfo = "Microdrive";
    break;
  case libgdos_ftype_zx_screen:
    typeinfo = "ZX SCREEN$";
    break;
  case libgdos_ftype_special:
    typeinfo = "Special";
    break;
  case libgdos_ftype_zx_snap128:
    typeinfo = "ZX SNP 128K";
    break;
  case libgdos_ftype_opentype:
    typeinfo = "Opentype";
    break;
  case libgdos_ftype_zx_execute:
    typeinfo = "ZX Execute";
    break;
  case libgdos_ftype_unidos_subdir:
    typeinfo = "UniDOS Subdir";
    break;
  case libgdos_ftype_unidos_create:
    typeinfo = "UniDOS Create";
    break;
  case libgdos_ftype_sam_basic:
    if( !entry->ftypeinfo[32] ) {
      snprintf( infobuf, sizeof( infobuf ),
		"SAM BASIC %5i",
		entry->ftypeinfo[33] + entry->ftypeinfo[34] * 0x100 );
      typeinfo = infobuf;
    } else {
      typeinfo = "SAM BASIC";
    }
    break;
  case libgdos_ftype_sam_num:
    typeinfo = "SAM Num Arr";
    break;
  case libgdos_ftype_sam_str:
    typeinfo = "SAM Str Arr";
    break;
  case libgdos_ftype_sam_code:
    length = entry->ftypeinfo[29] * 0x4000 +
	     entry->ftypeinfo[30] + entry->ftypeinfo[31] * 0x100;
    start = 0x4000 + ( entry->ftypeinfo[26] & 0x1f ) * 0x4000 +
	    entry->ftypeinfo[27] + ( entry->ftypeinfo[28] & 0x3f ) * 0x100;
    snprintf( infobuf, sizeof( infobuf ),
	      "SAM CODE %6i,%i", start, length );
    typeinfo = infobuf;
    break;
  case libgdos_ftype_sam_screen:
    typeinfo = "SAM SCREEN$";
    break;
  case libgdos_ftype_mdos_subdir:
    typeinfo = "Master DOS Subdir";
    break;
  default:
    typeinfo = "Unknown";
    break;

  }

  printf( "%3i %-10s%3i %s", entry->slot, entry->filename,
			     entry->numsectors, typeinfo );

  /* Master DOS timestamps */
  if( entry->ftypeinfo[35] >= 1 && entry->ftypeinfo[35] <= 31 &&
      entry->ftypeinfo[36] >= 1 && entry->ftypeinfo[36] <= 12 &&
      entry->ftypeinfo[37] < 100 &&
      entry->ftypeinfo[38] < 60 &&
      entry->ftypeinfo[39] < 60 ) {
    for( i = strlen( typeinfo ); i < 24; i++ )
      putchar(' ');
    printf( "%02i/%02i/%02i %02i:%02i",
	    entry->ftypeinfo[35], entry->ftypeinfo[36],
	    entry->ftypeinfo[37],
	    entry->ftypeinfo[38],
	    entry->ftypeinfo[39] );
  }

  putchar('\n');
}

int
main( int argc, const char **argv )
{
  libgdos_disk *disk;
  libgdos_dir *dir;
  libgdos_dirent entry;
  int numfiles = 0;
  int numsectors = 0;
  int length;
  int error;
  int freeslots;

  progname = argv[0];

  if( argc != 2 ) {
    fprintf( stderr, "%s: usage: %s <file>\n", progname, progname );
    return 1;
  }

  disk = libgdos_openimage( argv[1] );
  if( !disk ) {
    /* XXX */
    fprintf( stderr, "%s: failed to open image\n", progname );
    return 1;
  }

  dir = libgdos_openrootdir( disk );
  if( !dir ) {
    fprintf( stderr, "%s: failed to open root directory\n", progname );
    libgdos_closeimage( disk );
    return 1;
  }

  length = 80;
  numsectors += ( length + 1 ) / 2;

  printf( "* GDOS-TOOLS  DISC CATALOGUE *\n\n" );

  error = libgdos_readdir( dir, &entry );
  while( !error ) {
    numfiles++;
    numsectors += entry.numsectors;
    printdir( &entry );
    error = libgdos_readdir( dir, &entry );
  }

  printf( "\nNumber of Free K-Bytes = %i\n",
	  800 - ( ( numsectors + 1 ) / 2 ) );
  freeslots = length - numfiles;
  printf( "%i File%s, %i Free Slot%s\n",
	  numfiles,
	  numfiles != 1 ? "s" : "",
	  freeslots,
	  freeslots != 1 ? "s" : "" );

  libgdos_closedir( dir );

  libgdos_closeimage( disk );

  return 0;
}
