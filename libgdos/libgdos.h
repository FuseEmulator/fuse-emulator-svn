/* libgdos.h: library for dealing with GDOS disk images
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

   E-mail: sdbrady@ntlworld.com

*/

#ifndef LIBGDOS_LIBGDOS_H
#define LIBGDOS_LIBGDOS_H

#ifdef __cplusplus
extern "C" {
#endif				/* #ifdef __cplusplus */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely used stuff from Windows
				   headers */
#include <windows.h>

#ifdef LIBGDOS_EXPORTS
#define WIN32_DLL __declspec( dllexport )
#else                           /* #ifdef LIBGDOS_EXPORTS */
#define WIN32_DLL __declspec( dllimport )
#endif                          /* #ifdef LIBGDOS_EXPORTS */

#else                           /* #ifdef _WIN32 */

#define WIN32_DLL

#endif                          /* #ifdef _WIN32 */

#ifdef __GNUC__
#define DEPRECATED __attribute__((deprecated))
#else                           /* #ifdef __GNUC__ */
#define DEPRECATED 
#endif                          /* #ifdef __GNUC__ */

#include <stdint.h>

typedef struct libgdos_disk libgdos_disk;
typedef struct libgdos_dir libgdos_dir;
typedef struct libgdos_dirent libgdos_dirent;
typedef struct libgdos_file libgdos_file;

enum libgdos_dirflag {
  libgdos_dirflag_skip_erased = 0,
  libgdos_dirflag_skip_hidden,
  libgdos_dirflag_zero_terminate,
};

enum libgdos_variant {
  libgdos_variant_gdos,
  libgdos_variant_betados,
  libgdos_variant_masterdos,
};

enum libgdos_ftype {
  libgdos_ftype_erased = 0,
  libgdos_ftype_zx_basic,
  libgdos_ftype_zx_num,
  libgdos_ftype_zx_str,
  libgdos_ftype_zx_code,
  libgdos_ftype_zx_snap48,
  libgdos_ftype_zx_microdrive,
  libgdos_ftype_zx_screen,
  libgdos_ftype_special,
  libgdos_ftype_zx_snap128,
  libgdos_ftype_opentype,
  libgdos_ftype_zx_execute,
  libgdos_ftype_unidos_subdir,
  libgdos_ftype_unidos_create,

  libgdos_ftype_sam_basic = 16,
  libgdos_ftype_sam_num,
  libgdos_ftype_sam_str,
  libgdos_ftype_sam_code,
  libgdos_ftype_sam_screen,
  libgdos_ftype_mdos_subdir,
};

enum libgdos_status {
  libgdos_status_protected = 1,
  libgdos_status_hidden = 2,
};

/* dirent, inode */
struct libgdos_dirent {
  libgdos_disk *disk;

  int slot;
  enum libgdos_status status;
  enum libgdos_ftype ftype;
  char filename[ 10 ]; /* not null-terminated */
  int numsectors;
  int track;
  int sector;
  uint8_t secmap[ 195 ];
  uint8_t ftypeinfo[ 46 ];
};

libgdos_disk * WIN32_DLL
libgdos_openimage( const char *filename );

int WIN32_DLL
libgdos_closeimage( libgdos_disk *disk );

int WIN32_DLL
libgdos_readsector( libgdos_disk *disk, uint8_t *buf, int track, int sector );

libgdos_dir * WIN32_DLL
libgdos_openrootdir( libgdos_disk *disk );

libgdos_dir * WIN32_DLL
libgdos_opensubdir( libgdos_dir *dir, const char *name );

libgdos_dir * WIN32_DLL
libgdos_opendir( libgdos_disk *disk, const char *name );

int WIN32_DLL
libgdos_readdir( libgdos_dir *dir, libgdos_dirent *entry );

void WIN32_DLL
libgdos_closedir( libgdos_dir *dir );

void WIN32_DLL
libgdos_set_dirflag( libgdos_dir *dir, enum libgdos_dirflag );

void WIN32_DLL
libgdos_reset_dirflag( libgdos_dir *dir, enum libgdos_dirflag );

int WIN32_DLL
libgdos_test_dirflag( libgdos_dir *dir, enum libgdos_dirflag );

int WIN32_DLL
libgdos_getnumslots( libgdos_dir *dir );

int WIN32_DLL
libgdos_getentnum( libgdos_dir *dir, int slot, libgdos_dirent *entry );

int WIN32_DLL
libgdos_scandir( libgdos_dir *dir, libgdos_dirent ***namelist,
		 int( *filter )( const libgdos_dirent *dir ),
		 int( *compar )( const libgdos_dirent **,
				 const libgdos_dirent ** ));

int WIN32_DLL
libgdos_alphasort( const void *a, const void *b );

libgdos_file * WIN32_DLL
libgdos_fopennum( libgdos_dir *dir, int n; );

libgdos_file * WIN32_DLL
libgdos_fopenent( libgdos_dirent *dirent );

int WIN32_DLL
libgdos_fclose( libgdos_file *file );

int WIN32_DLL
libgdos_fread( uint8_t *buf, int count, libgdos_file *file );

#ifdef __cplusplus
};
#endif                          /* #ifdef __cplusplus */

#endif                          /* #ifndef LIBGDOS_LIBGDOS_H */
