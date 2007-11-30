/* fuse-mgt.c: Filesystem in USErspace driver for MGT disk images
   Copyright (c) 2005-7 Philip Kendall

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

   E-mail: philip@shadowmagic.org.uk

*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fuse.h>

static int mgt_fd = -1;

enum mgt_dirent_attributes {

  MGT_DIRENT_ATTRIBUTE_ERASED = 0,
  MGT_DIRENT_ATTRIBUTE_BASIC,
  MGT_DIRENT_ATTRIBUTE_NUMERIC_ARRAY,
  MGT_DIRENT_ATTRIBUTE_CHARACTER_ARRAY,
  MGT_DIRENT_ATTRIBUTE_CODE,
  MGT_DIRENT_ATTRIBUTE_48K_SNAP,
  MGT_DIRENT_ATTRIBUTE_MICRODRIVE,
  MGT_DIRENT_ATTRIBUTE_SCREEN,
  MGT_DIRENT_ATTRIBUTE_SPECIAL,
  MGT_DIRENT_ATTRIBUTE_128K_SNAP,
  MGT_DIRENT_ATTRIBUTE_STREAM,
  MGT_DIRENT_ATTRIBUTE_EXECUTE,
  MGT_DIRENT_ATTRIBUTE_DIRECTORY,
  MGT_DIRENT_ATTRIBUTE_CREATE,

};
  
struct mgt_dirent {

  /* Where this entry is stored 'on disk' */
  int side;
  int track;
  int entry;

  /* Properties of the directory entry */
  unsigned char attributes;
  char filename[ 11 ];
  unsigned char first_track;
  unsigned char first_sector;
  int size;
};

typedef int (*mgt_dirent_find_fn)( struct mgt_dirent *dirent, void *opaque );

#define SECTOR_SIZE 512
#define TRACK_SIZE ( 10 * SECTOR_SIZE )

static off_t
get_offset( int side, int track )
{
  return ( 2 * track + side ) * TRACK_SIZE;
}

static int
get_track( char *buffer, int side, int track )
{
  off_t start = get_offset( side, track );

  if( lseek( mgt_fd, start, SEEK_SET ) == -1 ) return -EIO;
  if( read( mgt_fd, buffer, TRACK_SIZE ) != TRACK_SIZE ) return -EIO;

  return 0;
}

static int
get_sector( char *buffer, int side, int track, int sector )
{
  off_t start = get_offset( side, track ) + 0x200 * ( sector - 1 );

  if( lseek( mgt_fd, start, SEEK_SET ) == -1 ) return -EIO;
  if( read( mgt_fd, buffer, SECTOR_SIZE ) != SECTOR_SIZE ) return -EIO;

  return 0;
}

static int
get_dirent( struct mgt_dirent *dirent, mgt_dirent_find_fn finder,
	    void *opaque )
{
  int track;
  for( track = 0; track < 4; track++ )
  {
    char buffer[ TRACK_SIZE ];

    int res = get_track( buffer, 0, track );
    if( res ) return res;

    int i;
    for( i = 0; i < 20; i++ )
    {
      const char *ptr = &buffer[ 0x100 * i ];

      dirent->side = 0;
      dirent->track = track;
      dirent->entry = i;

      dirent->attributes = *ptr;
      memcpy( dirent->filename, ptr + 1, 10 ); dirent->filename[ 10 ] = 0;
      dirent->first_track = *( ptr + 13 );
      dirent->first_sector = *( ptr + 14 );
      dirent->size = (unsigned char)*( ptr + 212 ) + 0x100 * (unsigned char)*( ptr + 213 );

      if( finder( dirent, opaque ) )
      {
	return 1;
      }
    }
  }

  return 0;
}

static int
find_by_filename( struct mgt_dirent *dirent, void *opaque )
{
  if( dirent->attributes == MGT_DIRENT_ATTRIBUTE_ERASED ) return 0;

  return strcmp( dirent->filename, opaque ) == 0;
}

static int
mgt_getattr( const char *path, struct stat *stbuf )
{
  memset( stbuf, 0, sizeof( struct stat ) );

  if( strcmp( path, "/" ) == 0 )
  {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  struct mgt_dirent dirent;

  if( get_dirent( &dirent, find_by_filename, (void*)path + 1 ) )
  {
    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = dirent.size;
    return 0;
  }

  return -ENOENT;
}

static int
find_empty_dirent( struct mgt_dirent *dirent, void *opaque )
{
  return dirent->attributes == MGT_DIRENT_ATTRIBUTE_ERASED;
}

static int
write_lsb_word( int fd, unsigned w )
{
  unsigned char buffer[2];

  buffer[0] = w & 0xff;
  buffer[1] = w >> 8;

  if( write( fd, buffer, 2 ) != 2 ) return -EIO;

  return 0;
}

static int
write_dirent( struct mgt_dirent *dirent )
{
  off_t offset = get_offset( dirent->side, dirent->track );

  offset += dirent->entry * 0x100;

  if( lseek( mgt_fd, offset, SEEK_SET ) == -1 ) return -EIO;

  if( write( mgt_fd, &( dirent->attributes ),    1 ) !=  1 ) return -EIO;
  if( write( mgt_fd, &( dirent->filename ),     10 ) != 10 ) return -EIO;
  if( lseek( mgt_fd,   2, SEEK_CUR ) == -1 ) return -EIO;
  if( write( mgt_fd, &( dirent->first_track ),   1 ) !=  1 ) return -EIO;
  if( write( mgt_fd, &( dirent->first_sector ),  1 ) !=  1 ) return -EIO;
  if( lseek( mgt_fd, 197, SEEK_CUR ) == -1 ) return -EIO;
 
  int e = write_lsb_word( mgt_fd, dirent->size );
  if( e ) return e;

  return 0;
}

static int
mgt_mknod( const char *path, mode_t mode, dev_t rdev )
{
  if( !S_ISREG( mode ) ) return -EPERM;

  if( strcmp( path, "/" ) == 0 ) return -EEXIST;

  struct mgt_dirent dirent;

  if( get_dirent( &dirent, find_by_filename, (void*)path + 1 ) )
    return -EEXIST;

  if( !get_dirent( &dirent, find_empty_dirent, NULL ) )
    return -ENOSPC;

  dirent.attributes = MGT_DIRENT_ATTRIBUTE_SPECIAL;
  strncpy( dirent.filename, path + 1, 10 );
  dirent.first_track = dirent.first_sector = 0;
  dirent.size = 0;

  write_dirent( &dirent );

  return 0;
}

static int
mgt_open( const char *path, struct fuse_file_info *fi )
{
  struct mgt_dirent dirent;
  if( get_dirent( &dirent, find_by_filename, (void*)path + 1 ) )
  {
    return 0;
  }

  return -ENOENT;
}

struct read_context
{
  int eof;

  char buffer[ SECTOR_SIZE ];
  size_t offset;
};

static int
read_context_init( struct read_context *ctx, int track, int sector )
{
  if( track || sector )
  {
    int side = track & 0x80 ? 1 : 0;

    int res = get_sector( ctx->buffer, side, track % 80, sector );
    if( res ) return res;

    ctx->eof = 0;
    ctx->offset = 0;
  }
  else
  {
    ctx->eof = 1;
  }

  return 0;
}

static int
is_tape_type( struct mgt_dirent *dirent )
{
  switch( dirent->attributes )
  {
  case MGT_DIRENT_ATTRIBUTE_BASIC:
  case MGT_DIRENT_ATTRIBUTE_NUMERIC_ARRAY:
  case MGT_DIRENT_ATTRIBUTE_CHARACTER_ARRAY:
  case MGT_DIRENT_ATTRIBUTE_CODE:
  case MGT_DIRENT_ATTRIBUTE_SCREEN:
    return 1;

  case MGT_DIRENT_ATTRIBUTE_ERASED:
  case MGT_DIRENT_ATTRIBUTE_48K_SNAP:
  case MGT_DIRENT_ATTRIBUTE_MICRODRIVE:
  case MGT_DIRENT_ATTRIBUTE_SPECIAL:
  case MGT_DIRENT_ATTRIBUTE_128K_SNAP:
  case MGT_DIRENT_ATTRIBUTE_STREAM:
  case MGT_DIRENT_ATTRIBUTE_EXECUTE:
  case MGT_DIRENT_ATTRIBUTE_DIRECTORY:
  case MGT_DIRENT_ATTRIBUTE_CREATE:
    return 0;
  }

  /* Should never be reached */
  return 0;
}

static ssize_t
read_bytes( char *buffer, struct read_context *ctx, size_t count )
{
  if( ctx->eof ) return 0;

  if( ctx->offset + count > SECTOR_SIZE - 2 )
  {
    count = SECTOR_SIZE - 2 - ctx->offset;
  }

  if( buffer )
  {
    memcpy( buffer, ctx->buffer + ctx->offset, count );
  }

  ctx->offset += count;

  if( ctx->offset == SECTOR_SIZE - 2 )
  {
    int res = read_context_init( ctx, ctx->buffer[ SECTOR_SIZE - 2 ],
				 ctx->buffer[ SECTOR_SIZE - 1 ] );
    if( res ) return res;
  }

  return count;
}

static int
mgt_read( const char *path, char *buf, size_t size, off_t offset,
	  struct fuse_file_info *fi )
{
  struct mgt_dirent dirent;
  if( get_dirent( &dirent, find_by_filename, (void*)path + 1 ) )
  {
    struct read_context ctx;

    if( offset > dirent.size ) return 0;
    if( offset + size > dirent.size ) size = dirent.size - offset;

    int res = read_context_init( &ctx, dirent.first_track,
				 dirent.first_sector );
    if( res ) return res;

    if( is_tape_type( &dirent ) ) ctx.offset += 9;

    while( offset )
    {
      ssize_t bytes = read_bytes( NULL, &ctx, offset );
      offset -= bytes;

      if( bytes < 0 ) return bytes;
      if( bytes == 0 ) return 0;
    }

    char *ptr = buf;
    size_t total = 0;

    while( size )
    {
      ssize_t bytes = read_bytes( ptr, &ctx, size );
      total += bytes;
      ptr += bytes;
      size -= bytes;

      if( bytes < 0 ) return bytes;
      if( bytes == 0 ) return total;
    }

    return total;
  }

  return -ENOENT;
}

static int
mgt_readdir( const char *path, void *buf, fuse_fill_dir_t filler,
	     off_t offset, struct fuse_file_info *fi )
{
  if( strcmp(path, "/") != 0 ) return -ENOENT;

  filler( buf, ".", NULL, 0 );
  filler( buf, "..", NULL, 0 );

  int track;
  for( track = 0; track < 4; track++ )
  {
    char buffer[ TRACK_SIZE ];

    int res = get_track( buffer, 0, track );
    if( res ) return res;

    int i;
    for( i = 0; i < 20; i++ )
    {
      const char *ptr = &buffer[ 0x100 * i ];

      if( *ptr == MGT_DIRENT_ATTRIBUTE_ERASED ) continue;

      char filename[11];
      memcpy( filename, ptr + 1, 10 ); filename[ 10 ] = 0;

      filler( buf, filename, NULL, 0 );
    }
  }

  return 0;
}

static int
mgt_unlink( const char *path )
{
  if( strcmp( path, "/" ) == 0 ) return -EISDIR;

  struct mgt_dirent dirent;

  if( get_dirent( &dirent, find_by_filename, (void*)path + 1 ) )
  {
    dirent.attributes = MGT_DIRENT_ATTRIBUTE_ERASED;
    return write_dirent( &dirent );
  }

  return -ENOENT;
}

static int
mgt_write( const char *path, const char *buf, size_t size,
	   off_t offset, struct fuse_file_info *fi )
{
  struct mgt_dirent dirent;

  if( !get_dirent( &dirent, find_by_filename, (void*)path + 1 ) )
  {
    return -ENOENT;
  }

  return size;
}

static struct fuse_operations mgt_operations =
{
  .getattr = mgt_getattr,
  .mknod   = mgt_mknod,
  .open    = mgt_open,
  .read    = mgt_read,
  .readdir = mgt_readdir,
  .unlink  = mgt_unlink,
  .write   = mgt_write,
};

static const char *filename = "Games5.mgt";
static const off_t MGT_SIZE = 2 * 80 * TRACK_SIZE;

int
main( int argc, char *argv[] )
{
  mgt_fd = open( filename, O_RDWR );
  if( mgt_fd == -1 )
  {
    fprintf( stderr, "%s: couldn't open \"%s\": %s\n", argv[0], filename,
	     strerror( errno ) );
    return 1;
  }

  struct stat file_info;
  if( fstat( mgt_fd, &file_info ) )
  {
    fprintf( stderr, "%s: couldn't stat \"%s\": %s\n", argv[0], filename,
	     strerror( errno ) );
    close( mgt_fd );
    return 1;
  }

  if( file_info.st_size != MGT_SIZE )
  {
    fprintf( stderr, "%s: \"%s\" is not %lu bytes long\n", argv[0], filename,
	     (unsigned long)MGT_SIZE );
    close( mgt_fd );
    return 1;
  }

  return fuse_main( argc, argv, &mgt_operations );
}
