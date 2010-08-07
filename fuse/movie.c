/* movie.c: Routines for creating 'movie' with border
   Copyright (c) 2006 Gergely Szasz

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
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <libspectrum.h>
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

#include "display.h"
#include "fuse.h"
#include "machine.h"
#include "screenshot.h"
#include "settings.h"
#include "scld.h"
#include "ui/ui.h"

#undef MOVIE_DEBUG_PRINT

#ifdef USE_FFMPEG
int movie_start_ffmpeg( char *filename, int freq, int stereo, int timex, int timing, int frate );
void movie_stop_ffmpeg( void );
void movie_add_area_ffmpeg( int x, int y, int w, int h );
void movie_add_sound_ffmpeg( libspectrum_signed_word *buf, int len );
void movie_start_frame_ffmpeg();
void movie_list_ffmpeg( int what );
#endif
/*
    FMF - Fuse Movie File

	Fuse Movie File:
	    Magic header: "FMF_" 
	    Version: "V#"	# 1 with sound
	    Endianness: "e" - little, "E" - big
	    Compression "U" - uncompressed, "Z" - Zlib compressed

	    Screen
	    Every frame start with 'N' + frame_rate + screen_type + frame_timing,
	    where 'frame_rate' is 1:#, where 1:1 is 50 1/s, 1:2 is 25 1/s...,
	    where 'screen_type'  $ - standard screen,
				 R - HiRes,
				 C - HiColor,
				 X - standard screen on Timex (double size)
	    where 'frame_timing' A - 16, 48, TC2048, TC2068, Scorpion, SE
				 B - 128, +2, +2A, +3, +3E
				 C - TS2068, Pentagon
	    in a frame there are no, one or several rectangle, changed from
		the previouse frame.
	    '$', x, yl, yh, w, hl, hh, runlength encoded data 
	        x and w is in columns (real_x / 8), y and h in lines.
		Data bytes:
		  abcdefghbb#gg# two identical byte plus len code a #+2 len
		  run...
		  bitmap1; attrib1		ZX$/TX$
		  bitmap1; bitmap2; attrib	HiR
		  bitmap1; attrib1		HiC

	    Sound:
		Every frame start with 'S' + type + freql freqh + stereo +
		    lenl lenh in frame,
		    type is 'P' - PCM linear
			    'A' - A-law
			    'U' - u-law
		    freq(l+h) is 16bit Hz
		    stereo is M or S
		    len(l+h) is 16bit in frame-1 (1-65536)
*/

#define U_BIAS 0x0084
#define U_CLIP 0x7f7b
static int frame_no, slice_no;

static FILE *of = NULL;	/* out file */
static int fmf_screen;
static libspectrum_byte head[7];
static int freq = 0;
static char stereo = 'M';
static char format = '?';
static int framesiz = 4;

#ifdef USE_FFMPEG
static int ffmpeg_movie;		/* if 1 we record movie trough ffmpeg avcodec/... */
#endif

static libspectrum_byte sbuff[4096];
#ifdef HAVE_ZLIB_H
#define ZBUF_SIZE 8192
static int fmf_compr = -1;
static z_stream zstream;
static unsigned char zbuf_o[ZBUF_SIZE];
#endif	/* HAVE_ZLIB_H */

#include "movie_tables.c"

static char
get_timing()
{
  return    machine_current->machine ==  LIBSPECTRUM_MACHINE_16 ||
	    machine_current->machine ==  LIBSPECTRUM_MACHINE_48 ||
	    machine_current->machine ==  LIBSPECTRUM_MACHINE_TC2048 ||
	    machine_current->machine ==  LIBSPECTRUM_MACHINE_TC2068 ||
	    machine_current->machine ==  LIBSPECTRUM_MACHINE_SCORP ||
	    machine_current->machine ==  LIBSPECTRUM_MACHINE_SE ? 'A' : (
	      machine_current->machine ==  LIBSPECTRUM_MACHINE_128 ||
	      machine_current->machine ==  LIBSPECTRUM_MACHINE_PLUS2 ||
	      machine_current->machine ==  LIBSPECTRUM_MACHINE_PLUS2A ||
	      machine_current->machine ==  LIBSPECTRUM_MACHINE_PLUS3 ||
	      machine_current->machine ==  LIBSPECTRUM_MACHINE_PLUS3E ? 'B' : (
	        machine_current->machine ==  LIBSPECTRUM_MACHINE_TS2068 ? 'C' : (
	          machine_current->machine ==  LIBSPECTRUM_MACHINE_PENT ? 'D' : '?'
		)
	      )
	    );
}

#ifdef HAVE_ZLIB_H
static void
fwrite_compr( void *b, size_t n, size_t m, FILE *f )
{
  int s;			/* zlib status */

  if( fmf_compr == 0 ) {
    fwrite( b, n, m, f );
  } else {
    zstream.avail_in = n * m;
    zstream.next_in = b;
    zstream.avail_out = ZBUF_SIZE;
    zstream.next_out = zbuf_o;
    do {
      s = deflate( &zstream, Z_NO_FLUSH );
      while( zstream.avail_out != ZBUF_SIZE ) {
        fwrite( zbuf_o, ZBUF_SIZE - zstream.avail_out, 1, of );
	zstream.avail_out = ZBUF_SIZE;
	zstream.next_out = zbuf_o;
        deflate( &zstream, Z_NO_FLUSH );
      }
    } while ( zstream.avail_in != 0 );
  }
}
#else	/* HAVE_ZLIB_H */
#define fwrite_compr fwrite
#endif	/* HAVE_ZLIB_H */

static void
movie_compress_area( int x, int y, int w, int h, int s )
{
  libspectrum_dword *dpoint, *dline;
  libspectrum_byte d, d1, *b;
  libspectrum_byte buff[960];
  int w0, h0, l;

  dline = &display_last_screen[x + 40 * y];
  b = buff; l = -1;
  d1 = ( ( *dline >> s ) & 0xff ) + 1;		/* *d1 != dpoint :-) */
  
  for( h0 = h; h0 > 0; h0--, dline += 40 ) {
    dpoint = dline;
    for( w0 = w; w0 > 0; w0--, dpoint++) {
      d = ( *dpoint >> s ) & 0xff;	/* bitmask1 */
      if( d != d1 ) {
        if( l > -1 ) {			/* save running length 0-255 */
	  *b++ = l;
	  l = -1;			/* reset l */
	}
        *b++ = d1 = d;
      } else if( l >= 0 ) {
	if( l == 255 ) {		/* close run, and may start a new? */
	  *b++ = l; *b++ = d; l = -1;
	} else {
    	  l++;
	}
      } else {
        *b++ = d;
	l++;
      }
/*      d1 = d;				*/
    }
    if( b - buff > 960 - 128 ) {	/* worst case 40*1.5 per line */
      fwrite_compr( buff, b - buff, 1, of );
      b = buff;
    }
  }
  if( l > -1 ) {		/* save running length 0-255 */
    *b++ = l;
  }
  if( b != buff ) {	/* dump remain */
    fwrite_compr( buff, b - buff, 1, of );
  }
}
/* Fetch pixel (x, y). On a Timex this will be a point on a 640x480 canvas,
   on a Sinclair/Amstrad/Russian clone this will be a point on a 320x240
   canvas */

/* x: 0 - 39; y: 0 - 239 */

/* abcdefghijkl... cc# where # mean cc + # c char*/

void
movie_add_area( int x, int y, int w, int h )
{
#ifdef USE_FFMPEG
  if( ffmpeg_movie ) {
    movie_add_area_ffmpeg( x, y, w, h );
    return;
  }
#endif
  head[0] = '$';			/* RLE compressed data... */
  head[1] = x;
  head[2] = y & 0xff;
  head[3] = y >> 8;
  head[4] = w;
  head[5] = h & 0xff;
  head[6] = h >> 8;
  fwrite_compr( head, 7, 1, of );
  movie_compress_area( x, y, w, h, 0 );	/* Bitmap1 */
  movie_compress_area( x, y, w, h, 8 );	/* Attrib/B2 */
  if( fmf_screen == 'R' ) {
    movie_compress_area( x, y, w, h, 16 );	/* HiRes attrib */
  }
  slice_no++;
}

static void
movie_start_fmf( char *name )
{
    if( ( of = fopen(name, "wb") ) == NULL ) {	/* trunc old file ? or append ?*/
    /*** FIXME error handling */
    }
#ifdef WORDS_BIGENDIAN
    fwrite( "FMF_V1E", 7, 1, of );	/* write magic header Fuse Movie File*/
#else	/* WORDS_BIGENDIAN */
    fwrite( "FMF_V1e", 7, 1, of );	/* write magic header Fuse Movie File*/
#endif	/* WORDS_BIGENDIAN */
#ifdef HAVE_ZLIB_H
    if( settings_current.movie_fmf_compr == 0 ) {
      fmf_compr = 0;
      fwrite( "U", 1, 1, of );		/* not compressed */
    } else {
      if( settings_current.movie_fmf_compr < Z_NO_COMPRESSION || 
    	  settings_current.movie_fmf_compr > Z_BEST_COMPRESSION )
        fmf_compr = Z_DEFAULT_COMPRESSION;
      else
        fmf_compr = settings_current.movie_fmf_compr;
      fwrite( "Z", 1, 1, of );		/* compressed */
    }
    if( fmf_compr != 0 ) {
      zstream.zalloc = Z_NULL;
      zstream.zfree = Z_NULL;
      zstream.opaque = Z_NULL;
      zstream.avail_in = 0;
      zstream.next_in = Z_NULL;
      deflateInit( &zstream, fmf_compr );
    }
#else	/* HAVE_ZLIB_H */
    fwrite( "U", 1, 1, of );		/* cannot be compressed */
#endif	/* HAVE_ZLIB_H */
    movie_add_area( 0, 0, 40, 240 );
}

void
movie_start( char *name, int type )	/* some init, open file (name)*/
{
  frame_no = slice_no = 0;
  if( name == NULL || *name == '\0' )
    name = "fuse.fmf";			/* fuse movie file */

  if( type == 3 ) {			/* fuse movie file */
#ifdef USE_FFMPEG
    if( !settings_current.movie_fmf ) {
      ffmpeg_movie = 1;
      if( movie_start_ffmpeg( name, freq, ( stereo == 'M' ? 0 : 1 ),
				machine_current->timex,
				get_timing(), settings_current.frame_rate ) ) {
	ffmpeg_movie = 0;
	screenshot_movie_record = 0;
	return;
      }
    } else {
      ffmpeg_movie = 0;
#endif
      movie_start_fmf( name );
#ifdef USE_FFMPEG
    }
#endif
  } else {
    snprintf( screenshot_movie_file, PATH_MAX-SCREENSHOT_MOVIE_FILE_MAX,
		"%s", name );
  }
  screenshot_movie_record = type;
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIES_RECORDING, type );
}

void
movie_stop( void )
{
  if( screenshot_movie_record != 3 ) return;

#ifdef USE_FFMPEG
  if( ffmpeg_movie ) {
    movie_stop_ffmpeg();
  } else {
#endif
#ifdef HAVE_ZLIB_H
    int s;

    if( fmf_compr != 0 ) {		/* close zlib */
      zstream.avail_in = 0;
      do {
        zstream.avail_out = ZBUF_SIZE;
        zstream.next_out = zbuf_o;
        s = deflate( &zstream, Z_SYNC_FLUSH );
        if( zstream.avail_out != ZBUF_SIZE )
          fwrite( zbuf_o, ZBUF_SIZE - zstream.avail_out, 1, of );
      } while ( zstream.avail_out != ZBUF_SIZE );
      deflateEnd( &zstream );
      fmf_compr = -1;
    }
#endif	/* HAVE_ZLIB_H */
    format = '?';
    if( of ) {
      fclose( of );
      of = NULL;
    }
#ifdef USE_FFMPEG
  }
#endif
#ifdef MOVIE_DEBUG_PRINT
  fprintf( stderr, "Debug movie: saved %d.%d frame(.slice)\n", frame_no, slice_no );
#endif 	/* MOVIE_DEBUG_PRINT */
  screenshot_movie_record = 0;
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIES_RECORDING, 0 );

}

void
movie_init_sound( int f, int s )
{
/* initialise sound format */
  if( format == '?' )
    format = settings_current.movie_fmf_sound == 1 ? 'U' : ( 
		settings_current.movie_fmf_sound == 2 ? 'A' : 'P' );
  freq = f;
  stereo = ( s ? 'S' : 'M' );
  framesiz = ( s ? 2 : 1 ) * ( format == 'P' ? 2 : 1 );
}

static inline void
write_alaw( libspectrum_signed_word *buff, int len )
{
  int i = 0;
  while( len-- ) {  
    if( *buff >= 0)
      sbuff[i++] = alaw_table[*buff >> 4];
    else
      sbuff[i++] = 0x7f & alaw_table [- *buff >> 4];
    buff++;
    if( i == 4096 ) {
      i = 0;
      fwrite_compr( sbuff, 4096, 1, of );	/* write frame */
    }
  }
  if( i )
    fwrite_compr( sbuff, i, 1, of );	/* write remaind */
}

static inline void
write_ulaw( libspectrum_signed_word *buff, int len )
{
  int i = 0;

  while( len-- ) {  
    if( *buff >= 0)
      sbuff[i++] = ulaw_table[*buff >> 2];
    else
      sbuff[i++] = 0x7f & ulaw_table [- *buff >> 2];
    buff++;
    if( i == 4096 ) {
      i = 0;
      fwrite_compr( sbuff, 4096, 1, of );	/* write frame */
    }
  }
  if( i )
    fwrite_compr( sbuff, i, 1, of );	/* write remaind */
}

static void
add_sound( libspectrum_signed_word *buff, int len )
{

  head[0] = 'S';	/* sound frame */
  head[1] = format;	/* sound format */
  head[2] = freq & 0xff;
  head[3] = freq >> 8;
  head[4] = stereo;
  len--;		/*len - 1*/
  head[5] = len & 0xff;
  head[6] = len >> 8;
  len++;		/* len :-) */
  fwrite_compr( head, 7, 1, of );	/* Sound frame */
  if( format == 'P' )
    fwrite_compr( buff, len * framesiz , 1, of );	/* write frame */
  else if( format == 'U' )
    write_ulaw( buff, len * framesiz );
  else if( format == 'A' )
    write_alaw( buff, len * framesiz );
}

void
movie_add_sound( libspectrum_signed_word *buff, int len )
{
#ifdef USE_FFMPEG
  if( ffmpeg_movie ) {
    movie_add_sound_ffmpeg( buff, len );
  } else {
#endif
    while( len ) {
      add_sound( buff, len > 65536 ? 65536 : len );
      buff += len > 65536 ? 65536 : len;
      len -= len > 65536 ? 65536 : len;
    }
#ifdef USE_FFMPEG
  }
#endif
}

void
movie_start_frame( void )
{
#ifdef USE_FFMPEG
  if( ffmpeg_movie ) {
    movie_start_frame_ffmpeg();
    return;
  }
#endif
		/* $ - ZX$, T - TX$, C - HiCol, R - HiRes */
  head[0] = 'N';
  head[1] = settings_current.frame_rate;
  if( machine_current->timex ) { /* ALTDFILE and default */
    if( scld_last_dec.name.hires )
      fmf_screen = 'R';	/* HIRES screen */
    else if( scld_last_dec.name.b1 )
      fmf_screen = 'C';	/* HICOLOR screen */
    else
      fmf_screen = 'X';	/* STANDARD screen on timex machine */
  } else {
    fmf_screen = '$';	/* STANDARD screen */
  }
  head[2] = fmf_screen;
  head[3] = get_timing();
  fwrite_compr( head, 4, 1, of );	/* New frame! */
  frame_no++;
}

void
movie_init()
{
#ifdef USE_FFMPEG
  if( settings_current.movie_format && (
      strcmp( settings_current.movie_format, "?" ) == 0 ||
      strcmp( settings_current.movie_format, "list" ) == 0 ||
      strcmp( settings_current.movie_format, "help" ) ) == 0 ) {	/* list the available formats etc... */
    movie_list_ffmpeg( 0 );
    fuse_exiting = 1;
  }
  if( settings_current.movie_acodec && (
      strcmp( settings_current.movie_acodec, "?" ) == 0 ||
      strcmp( settings_current.movie_acodec, "list" ) == 0 || 
      strcmp( settings_current.movie_acodec, "help" ) == 0 ) ) {	/* list the available formats etc... */
    movie_list_ffmpeg( 1 );
    fuse_exiting = 1;
  }
  if( settings_current.movie_vcodec && (
      strcmp( settings_current.movie_vcodec, "?" ) == 0 ||
      strcmp( settings_current.movie_vcodec, "list" ) == 0 ||
      strcmp( settings_current.movie_vcodec, "help" ) == 0 ) ) {	/* list the available formats etc... */
    movie_list_ffmpeg( 2 );
    fuse_exiting = 1;
  }
#endif
  if( settings_current.movie_start )		/* start movie recording if user requested... */
    movie_start( settings_current.movie_start, 3 );
}
