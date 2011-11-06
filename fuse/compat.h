/* compat.h: various compatibility bits
   Copyright (c) 2003-2008 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_COMPAT_H
#define FUSE_COMPAT_H

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>

/* Remove the gcc-specific incantations if we're not using gcc */
#ifdef __GNUC__

#define GCC_UNUSED __attribute__ ((unused))
#define GCC_PRINTF( fmtstring, args ) __attribute__ ((format( printf, fmtstring, args )))
#define GCC_NORETURN __attribute__ ((noreturn))

#else				/* #ifdef __GNUC__ */

#define GCC_UNUSED
#define GCC_PRINTF( fmtstring, args )
#define GCC_NORETURN

#endif				/* #ifdef __GNUC__ */

#ifndef HAVE_DIRNAME
char *dirname( char *path );
#endif				/* #ifndef HAVE_DIRNAME */

#if !defined HAVE_GETOPT_LONG && !defined AMIGA && !defined __MORPHOS__
#include "compat/getopt.h"
#endif				/* #ifndef HAVE_GETOPT_LONG */

#ifndef HAVE_MKSTEMP
int mkstemp( char *templ );
#endif				/* #ifndef HAVE_MKSTEMP */

/* That which separates components in a path name */
#ifdef WIN32
#define FUSE_DIR_SEP_CHR '\\'
#define FUSE_DIR_SEP_STR "\\"
#else
#define FUSE_DIR_SEP_CHR '/'
#define FUSE_DIR_SEP_STR "/"
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

int compat_osname( char *buffer, size_t length );
const char* compat_get_temp_path( void );
const char* compat_get_home_path( void );
int compat_is_absolute_path( const char *path );

typedef FILE* compat_fd;

#ifndef GEKKO
typedef DIR* compat_dir;
#else                           /* #ifndef GEKKO */
typedef DIR_ITER* compat_dir;
#endif                          /* #ifndef GEKKO */

extern const compat_fd COMPAT_FILE_OPEN_FAILED;

/* File handling */

struct utils_file;

compat_fd compat_file_open( const char *path, int write );
off_t compat_file_get_length( compat_fd fd );
int compat_file_read( compat_fd fd, struct utils_file *file );
int compat_file_write( compat_fd fd, const unsigned char *buffer,
                       size_t length );
int compat_file_close( compat_fd fd );
int compat_file_exists( const char *path );

/* Directory handling */

typedef enum compat_dir_result_t {
  COMPAT_DIR_RESULT_OK,
  COMPAT_DIR_RESULT_END,
  COMPAT_DIR_RESULT_ERROR,
} compat_dir_result_t;

compat_dir compat_opendir( const char *path );
compat_dir_result_t compat_readdir( compat_dir directory, char *name,
				    size_t length );
int compat_closedir( compat_dir directory );

/* Timing routines */

double compat_timer_get_time( void );
void compat_timer_sleep( int ms );

/* TUN/TAP handling */

int compat_get_tap( const char *interface_name );

#endif				/* #ifndef FUSE_COMPAT_H */
