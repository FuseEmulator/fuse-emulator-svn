/* ffmpeg.c: Routines for recording movie with ffmpeg libs
   Copyright (c) 2010 Gergely Szasz

   $Id:$

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

   szaszg@hu.inter.net

*/

/* Most of the ffmpeg part of the code deduced from FFMPEG's output-example.c.
   Here is the original copyright note:
*/

/*
 * Libavformat API example: Output a media file in any supported
 * libavformat format. The default codecs are used.
 *
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#ifdef USE_FFMPEG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <libspectrum.h>

#include "display.h"
/* #include "fuse.h"
#include "machine.h" */
#include "options.h"
#include "settings.h"
#include "scld.h"
#include "ui/ui.h"

static int sws_flags = SWS_BICUBIC;

static int yuv_pal[] = {
  0, 128, 128,
  22, 224, 112,
  57, 96, 224,
  79, 191, 208,
  112, 65, 48,
  134, 160, 32,
  169, 32, 144,
  191, 128, 128,
  0, 128, 128,
  29, 255, 107,
  76, 85, 255,
  105, 212, 235,
  150, 44, 21,
  179, 171, 0,
  226, 0, 149,
  255, 128, 128,
};

static int machine_timing[] = {
  50080,	/* A */
  50021,	/* B */
  60115,	/* C */
  50000,	/* D */
};

static int frame_time_tab[] = {
  19968,
  19992,
  16635,
  20000,
};

/* Should be synced with ui/options.dat */
static char *format_str[] = {
  "", "avi", "matroska", "mpeg", "mp4", "dvd", "flv", "ipod", NULL
};

/* Should be synced with ui/options.dat */
static char *vcodec_str[] = {
  "", "mpeg2video", "mpeg4", "msmpeg4v2", "h263p"
};

/* Should be synced with ui/options.dat */
static char *acodec_str[] = {
  "", "mp2", "libmp3lame", "aac", "pcm_s16be", "vorbis"
};

static int frate_tab[] = {
  -1, 0, 25000, 29970, 24000
};


static AVOutputFormat *fmt;
static AVFormatContext *oc;
static AVStream *audio_st, *video_st;

static uint8_t *audio_outbuf;
static int16_t *audio_inpbuf;
static int audio_outbuf_size;
static int audio_iframe_size;			/* size of an input audio frame in bytes */
static int audio_oframe_size;			/* size of an output audio frame in bytes */
static int audio_input_frames;			/* number of required audio frames for a packet */
static int audio_inpbuf_len;			/* actual number of audio frames */

static AVFrame *picture, *tmp_picture;
static uint8_t *video_outbuf;
static int video_outbuf_size;
static int video_timex;
static int frame_count, frame_rate, frame_time, frame_time_cur, frame_time_incr;

/**************************************************************/
/* audio output */
/*
 * add an audio output stream
 */

static int
add_audio_stream( enum CodecID codec_id, int freq, int stereo )
{
  AVCodecContext *c;

  audio_st = av_new_stream( oc, 1 );
  if( !audio_st ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: Could not allocate audio stream stream\n" );
    return 1;
  }

  c = audio_st->codec;
  c->codec_id = codec_id;
  c->codec_type = AVMEDIA_TYPE_AUDIO;

    /* put sample parameters */
  c->sample_fmt = SAMPLE_FMT_S16;
  if( settings_current.movie_arate > 0 )
    c->bit_rate = settings_current.movie_arate;
  c->sample_rate = freq;
  c->channels = stereo ? 2 : 1;

    /* some formats want stream headers to be separate */
  if( oc->oformat->flags & AVFMT_GLOBALHEADER )
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;

  return 0;
}

static int
open_audio( int freq, int stereo, int timing )
{
  AVCodecContext *c;
  AVCodec *codec;

  c = audio_st->codec;

    /* find the audio encoder */
  codec = avcodec_find_encoder( c->codec_id );
  if( !codec ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: audio codec not found\n" );
    return 1;
  }

    /* open it */
  if( avcodec_open( c, codec ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: could not open audio codec\n" );
    return 1;
  }

  if( codec->sample_fmts != NULL )
    c->sample_fmt = codec->sample_fmts[0];

  audio_oframe_size = ( av_get_bits_per_sample_format( c->sample_fmt ) + 7 ) / 8 * c->channels;
  audio_input_frames = c->frame_size;

  if( audio_input_frames <= 1 ) {
    audio_outbuf_size = freq * 1250 * audio_oframe_size / timing;
  } else {
    audio_outbuf_size = audio_input_frames * audio_oframe_size * 125 / 100;
    if( audio_outbuf_size < FF_MIN_BUFFER_SIZE ) audio_outbuf_size = FF_MIN_BUFFER_SIZE;
    audio_inpbuf = av_malloc( audio_input_frames * audio_iframe_size );
    audio_inpbuf_len = 0;
  }
  audio_outbuf = av_malloc( audio_outbuf_size );

  return 0;
}

/* add an audio frame to the stream */
void
movie_add_sound_ffmpeg( int16_t *buf, int len )
{
  AVCodecContext *c;
  AVPacket pkt;

  if( !audio_st ) return;

  c = audio_st->codec;

  if( audio_input_frames > 1 && audio_inpbuf_len + len < audio_input_frames ) {
  /* store the full sound buffer and wait for more...*/
    memcpy( (char *)audio_inpbuf + ( audio_inpbuf_len * audio_iframe_size ), buf, len * audio_iframe_size );
    audio_inpbuf_len += len;
    return;
  }

  av_init_packet( &pkt );

  if( audio_input_frames > 1 ) {
    memcpy( (char *)audio_inpbuf + ( audio_inpbuf_len * audio_iframe_size ), buf,
		( audio_input_frames - audio_inpbuf_len ) * audio_iframe_size );
    pkt.size= avcodec_encode_audio( c, audio_outbuf, audio_outbuf_size, audio_inpbuf );
    memcpy( audio_inpbuf, (char *)buf + ( audio_input_frames - audio_inpbuf_len ) * audio_iframe_size,
		( len - audio_input_frames + audio_inpbuf_len ) * audio_iframe_size );
    audio_inpbuf_len = len - audio_input_frames + audio_inpbuf_len;
  } else {
    pkt.size= avcodec_encode_audio( c, audio_outbuf, len * audio_oframe_size, buf );
  }
  if( c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE )
    pkt.pts= av_rescale_q( c->coded_frame->pts, c->time_base, audio_st->time_base );
  pkt.flags |= AV_PKT_FLAG_KEY;
  pkt.stream_index= audio_st->index;
  pkt.data= audio_outbuf;

    /* write the compressed frame in the media file */
  if( av_interleaved_write_frame( oc, &pkt ) != 0 ) {
    fprintf( stderr, "Error while writing audio frame\n" );
/*    exit( 1 ); */
  }
}

static void
close_audio()
{
  if( audio_st ) avcodec_close( audio_st->codec );
  if( audio_outbuf ) av_free( audio_outbuf );
}

/**************************************************************/
/* video output */

static int
check_framerate( const AVRational *frates, int timing )
{
  if( frates == NULL ) return -1;
  while( frates->den != 0 || frates->num != 0 ) {
    if( ( frates->num * 1000 * frame_rate / frates->den ) == timing ) {
      return 1;
    }
    frates++;
  }
  return 0;
}

/* add a video output stream */
static int
add_video_stream( enum CodecID codec_id, int w, int h, int timing )
{
  AVCodecContext *c;
  int vrate_sel  = option_enumerate_movieoptions_movie_vrate_sel();

  video_st = av_new_stream( oc, 0 );
  if( !video_st ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: Could not allocate video stream\n" );
    return 1;
  }

  c = video_st->codec;
  c->codec_id = codec_id;
  c->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
  if( settings_current.movie_vrate > 0 )
    c->bit_rate = settings_current.movie_vrate;
  else if( vrate_sel == 0 )
    c->bit_rate *= 2;

    /* resolution must be a multiple of two */
  c->width  = w * ( settings_current.movie_double ? 2 : 1 );
  c->height = h * ( settings_current.movie_double ? 2 : 1 );
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
  c->gop_size = 25; /* emit one intra frame every twelve frames at most */
    /* some formats want stream headers to be separate */
  if(oc->oformat->flags & AVFMT_GLOBALHEADER)
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;
  return 0;
}

static AVFrame *
alloc_picture( enum PixelFormat pix_fmt, int width, int height )
{
  AVFrame *picture;
  uint8_t *picture_buf;
  int size;

  picture = avcodec_alloc_frame();
  if( !picture ) return NULL;
  size = avpicture_get_size( pix_fmt, width, height );
  picture_buf = av_malloc( size );
  if( !picture_buf ) {
    av_free( picture );
    return NULL;
  }
  avpicture_fill( (AVPicture *)picture, picture_buf,
                  pix_fmt, width, height );
  return picture;
}

static int
open_video( int timing_code )
{
  AVCodec *codec;
  AVCodecContext *c;
  int i;
  int timing = machine_timing[timing_code - 'A'];
  int frate_sel  = option_enumerate_movieoptions_movie_frate_sel();

  c = video_st->codec;

    /* find the video encoder */
  codec = avcodec_find_encoder( c->codec_id );
  if( !codec ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: video codec not found\n" );
    return 1;
  }

  if( codec->pix_fmts == NULL )
     c->pix_fmt = PIX_FMT_YUV420P;
  else
    c->pix_fmt = codec->pix_fmts[0];

  frame_time_cur = 0;
  frame_time_incr = frame_time = frame_time_tab[timing_code - 'A'] / frame_rate;

  if( settings_current.movie_frate != NULL ) {
    AVRational fr;
    if( av_parse_video_frame_rate( &fr, (const char *)settings_current.movie_frate ) == 0 ) {
      c->time_base.den = fr.num;
      c->time_base.num = fr.den;
      frame_time = 1000000 * c->time_base.num / c->time_base.den;
    } else {
      ui_error( UI_ERROR_ERROR, "FFMPEG: Cannot parse the given frame rate! (%s)", settings_current.movie_frate );
      return 1;
    }
  } else {
    if( frate_sel > 1 )
      timing = frate_tab[frate_sel] * frame_rate;
    if( codec->supported_framerates == NULL ||
        check_framerate( codec->supported_framerates, timing ) ) {	/* codec support the selected frate */
      c->time_base.den = timing;
      c->time_base.num = 1000 * frame_rate;
      if( frate_sel > 1 )
        frame_time = 1000000 * c->time_base.num / c->time_base.den;
    } else {
      if( frate_sel == 1 ) {	/* default -- try common ones */
        for( i = 2; i < 5; i++ ) {
          if( check_framerate( codec->supported_framerates, frate_tab[i] * frame_rate ) )
            break;
        }
        if( i < 5 ) {
          c->time_base.den = frate_tab[i];
          c->time_base.num = 1000;
        } else {
          c->time_base.den = codec->supported_framerates[0].num;
          c->time_base.num = codec->supported_framerates[0].den;
        }
      /*  24 / 1001 --> 1000 * 1000 * 1000 / ( 24 / 1001 ) */
        frame_time = 1000000 * c->time_base.num / c->time_base.den;
/*      fprintf(stderr, " %d / %d \n", frame_time_incr, frame_time ); */
      } else {
        ui_error( UI_ERROR_ERROR, "FFMPEG: The selected video codec cannot use the required frame rate!" );
        return 1;
      }
    }
  }

    /* open the codec */
  if( avcodec_open( c, codec ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: could not open video codec\n" );
    return 1;
  }

  if( !( oc->oformat->flags & AVFMT_RAWPICTURE ) ) {
        /* allocate output buffer */
        /* XXX: API change will be done */
        /* buffers passed into lav* can be allocated any way you prefer,
           as long as they're aligned enough for the architecture, and
           they're freed appropriately (such as using av_free for buffers
           allocated with av_malloc) */
    video_outbuf_size = 200000;
    video_outbuf = av_malloc( video_outbuf_size );
  }

    /* allocate the encoded raw picture */
  picture = alloc_picture( c->pix_fmt, c->width, c->height );
  if( !picture ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: Could not allocate picture\n" );
    avcodec_close(video_st->codec);
    return 1;
  }

    /* if the output format is not YUV444, then a temporary YUV444
       picture is needed too. It is then converted to the required
       output format */
  tmp_picture = NULL;
  if( c->pix_fmt != PIX_FMT_YUV444P ) {
    tmp_picture = alloc_picture( PIX_FMT_YUV444P, c->width, c->height );
    if( !tmp_picture ) {
      ui_error( UI_ERROR_ERROR, "FFMPEG: Could not allocate temporary picture\n" );
      avcodec_close( video_st->codec );
      av_free( picture->data[0] );
      av_free( picture );
      return 1;
    }
  }
  return 0;
}

static void
fill_yuv_timex_image( AVFrame *pict, int inv, int x, int y, int w0, int h, int d )
{
  libspectrum_dword *dpoint;
  int i, j, w, idx, half;
  int Y, U, V;

  h *= 2;
  y *= 2;
  for( w = w0 ; h > 0; h--, y++, w = w0 ) {
    idx = y * pict->linesize[0] * d + x * 16 * d;	/* 16pixel/byte*/
    dpoint = &display_last_screen[x + 40 * ( y >> 1 )];
    for( ; w > 0; w--, dpoint++ ) {
      int px, fx, ix = ( *dpoint & 0xff00 ) >> 8;
      int bits2, bits = *dpoint & 0xff;
      scld mode_data;

      mode_data.byte = ( *dpoint & 0xff0000 ) >> 16;

      if( mode_data.name.hires ) {		/* 512x192 */
        bits2 = ix;
        ix = hires_convert_dec( mode_data.byte );
        half = 0;
      } else {					/* 256x192 normal/hicolor  */
      /* divide x by two to get the same value for adjacent pixels */
        half = 2;
      }

      fx = ( ix & 0x80 ) * inv;
      px = ( ( ix & 0x38 ) >> 3 ) + ( ( ix & 0x40 ) ? 8 : 0 );
      ix = ( ix & 0x07 ) + ( ( ix & 0x40 ) ? 8 : 0 );
      for( j = mode_data.name.hires ? 1 : 0; j >= 0; j-- ) {

      for( i = 8; i > 0; i-- ) {
        if( ( bits ^ fx ) & 128 ) {	/* ink */
          Y = yuv_pal[ ix * 3 + 0 ];
          U = yuv_pal[ ix * 3 + 1 ];
          V = yuv_pal[ ix * 3 + 2 ];
        } else {			/* paper */
          Y = yuv_pal[ px * 3 + 0 ];
          U = yuv_pal[ px * 3 + 1 ];
          V = yuv_pal[ px * 3 + 2 ];
        }
        if( half ) {
          pict->data[0][idx]   = Y;
          pict->data[1][idx]   = U;
          pict->data[2][idx++] = V;
        }
        pict->data[0][idx]   = Y;
        pict->data[1][idx]   = U;
        pict->data[2][idx++] = V;
        if( d > 1 ) {			/* double size */
          if( half ) {
            pict->data[0][idx]   = Y;
            pict->data[1][idx]   = U;
            pict->data[2][idx++] = V;
          }
          pict->data[0][idx] = Y;
          pict->data[1][idx] = U;
          pict->data[2][idx--] = V;
          idx += pict->linesize[0] - half;
          if( half ) {
            pict->data[0][idx]   = Y;
            pict->data[1][idx]   = U;
            pict->data[2][idx++] = V;
            pict->data[0][idx]   = Y;
            pict->data[1][idx]   = U;
            pict->data[2][idx++] = V;
          }
          pict->data[0][idx]   = Y;
          pict->data[1][idx]   = U;
          pict->data[2][idx++] = V;
          pict->data[0][idx]   = Y;
          pict->data[1][idx]   = U;
          pict->data[2][idx++] = V;
          idx -= pict->linesize[0];
        }
        bits <<= 1;
      }
      bits = bits2;
    }
    }
  }
}

static void
fill_yuv_image( AVFrame *pict, int frame_index, int x, int y, int w0, int h )
{
  libspectrum_dword *dpoint;
  int i, w, idx;
  int Y, U, V;
  int inv = ( frame_count * frame_rate % 32 ) > 15 ? 1 : 0;

  int d = settings_current.movie_double ? 2 : 1;
  if( video_timex ) {
    fill_yuv_timex_image( pict, inv, x, y, w0, h, d );
    return;
  }
  w = w0;
  for( ; h > 0; h--, y++, w = w0 ) {
    idx = y * pict->linesize[0] * d + x * 8 * d;	/* 8pixel/byte*/
    dpoint = &display_last_screen[x + 40 * y];
    for( ; w > 0; w--, dpoint++ ) {
      int px, fx, ix = ( *dpoint & 0xff00 ) >> 8;
      int bits = *dpoint & 0xff;

      fx = ( ix & 0x80 ) * inv;
      px = ( ( ix & 0x38 ) >> 3 ) + ( ( ix & 0x40 ) ? 8 : 0 );
      ix = ( ix & 0x07 ) + ( ( ix & 0x40 ) ? 8 : 0 );
      for( i = 8; i > 0; i-- ) {
        if( ( bits ^ fx ) & 128 ) {	/* ink */
          Y = yuv_pal[ ix * 3 + 0 ];
          U = yuv_pal[ ix * 3 + 1 ];
          V = yuv_pal[ ix * 3 + 2 ];
        } else {			/* paper */
          Y = yuv_pal[ px * 3 + 0 ];
          U = yuv_pal[ px * 3 + 1 ];
          V = yuv_pal[ px * 3 + 2 ];
        }
        pict->data[0][idx]   = Y;
        pict->data[1][idx]   = U;
        pict->data[2][idx++] = V;
        if( d > 1 ) {			/* double size */
          pict->data[0][idx] = Y;
          pict->data[1][idx] = U;
          pict->data[2][idx--] = V;
          idx += pict->linesize[0];
          pict->data[0][idx]   = Y;
          pict->data[1][idx]   = U;
          pict->data[2][idx++] = V;
          pict->data[0][idx]   = Y;
          pict->data[1][idx]   = U;
          pict->data[2][idx++] = V;
          idx -= pict->linesize[0];
        }
        bits <<= 1;
      }
    }
  }
}

void
movie_add_area_ffmpeg( int x, int y, int w, int h )
{
  AVCodecContext *c;
  static struct SwsContext *img_convert_ctx;

  if( !video_st ) return;

/* fprintf( stderr, "%d,%d %d,%d\n", x, y, w, h ); */
  c = video_st->codec;

  if( c->pix_fmt != PIX_FMT_YUV444P ) {
            /* as we only generate a YUV444P picture, we must convert it
               to the codec pixel format if needed */
    if( img_convert_ctx == NULL ) {
      img_convert_ctx = sws_getContext( c->width, c->height,
                                             PIX_FMT_YUV444P,
                                             c->width, c->height,
                                             c->pix_fmt,
                                             sws_flags, NULL, NULL, NULL );
      if( img_convert_ctx == NULL ) {
        fprintf( stderr, "Cannot initialize the conversion context\n" );
/*        exit( 1 ); */
      }
    }
    fill_yuv_image( tmp_picture, frame_count, x, y, w, h );
    sws_scale( img_convert_ctx, (const uint8_t * const*)tmp_picture->data, tmp_picture->linesize,
                0, c->height, picture->data, picture->linesize );
  } else {
    fill_yuv_image( picture, frame_count, x, y, w, h );
  }
}

void
movie_start_frame_ffmpeg()
{
  int out_size, ret;
  AVCodecContext *c;

  if( !video_st ) return;

  frame_count++;
  c = video_st->codec;

  frame_time_cur += frame_time_incr;
  while( frame_time_cur + ( frame_time_incr + 1 ) / 2 - frame_time >= 0 ) {
    frame_time_cur -= frame_time;

    if( oc->oformat->flags & AVFMT_RAWPICTURE ) {
        /* raw video case. The API will change slightly in the near
           future for that */
      AVPacket pkt;
      av_init_packet( &pkt );

      pkt.flags |= AV_PKT_FLAG_KEY;
      pkt.stream_index= video_st->index;
      pkt.data= (uint8_t *)picture;
      pkt.size= sizeof( AVPicture );

      ret = av_interleaved_write_frame( oc, &pkt );
    } else {
        /* encode the image */
      out_size = avcodec_encode_video( c, video_outbuf, video_outbuf_size, picture );
        /* if zero size, it means the image was buffered */
      if( out_size > 0 ) {
        AVPacket pkt;
        av_init_packet( &pkt );

        if( c->coded_frame->pts != AV_NOPTS_VALUE )
          pkt.pts = av_rescale_q( c->coded_frame->pts, c->time_base, video_st->time_base );
        if( c->coded_frame->key_frame )
          pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index= video_st->index;
        pkt.data = video_outbuf;
        pkt.size = out_size;

            /* write the compressed frame in the media file */
        ret = av_interleaved_write_frame( oc, &pkt );
      } else {
        ret = 0;
      }
    }
    if( ret != 0 ) {
      fprintf(stderr, "Error while writing video frame\n");
/*      exit( 1 ); */
    }
  }
}

static void
close_video()
{
  if( video_st )
    avcodec_close( video_st->codec );
  if( picture ) {
    av_free( picture->data[0] );
    av_free( picture );
  }
  if( tmp_picture ) {
    av_free( tmp_picture->data[0] );
    av_free( tmp_picture );
  }
  if( video_outbuf ) av_free( video_outbuf );
}

/**************************************************************/
/* media file output */

int
movie_start_ffmpeg( char *filename, int freq, int stereo, int timex, int timing, int frate )
{

  AVCodec *c;
  enum CodecID acodec, vcodec;
  int format_sel = option_enumerate_movieoptions_movie_format_sel();
  int acodec_sel = option_enumerate_movieoptions_movie_acodec_sel();
  int vcodec_sel = option_enumerate_movieoptions_movie_vcodec_sel();

  picture = NULL;
  tmp_picture = NULL;
  video_outbuf = NULL;
  audio_outbuf = NULL;

    /* initialize libavcodec, and register all codecs and formats */
  av_register_all();

    /* auto detect the output format from the name. default is
       mpeg. */
  if( settings_current.movie_format != NULL && *settings_current.movie_format != 0 ) {
    fmt = av_guess_format( NULL, NULL, settings_current.movie_format );		/* MIME */
    if( !fmt )
      fmt = av_guess_format( settings_current.movie_format, NULL, NULL );	/* Name */
    if( !fmt ) {
      ui_error( UI_ERROR_ERROR, "FFMPEG: Unknown output format '%s'.\n", settings_current.movie_format );
      return 1;
    }
  }

  if( !fmt && format_sel > 0 && format_str[format_sel] != NULL ) {	/* we select other than default */
    fmt = av_guess_format( format_str[format_sel], NULL, NULL );	/* Name */
    if( !fmt ) {
      ui_error( UI_ERROR_ERROR, "FFMPEG: Unknown output format '%s'.\n", format_str[format_sel] );
      return 1;
    }
  }

  if( !fmt )
    fmt = av_guess_format( NULL, filename, NULL );
  if( !fmt ) {
/*    printf( "Could not deduce output format from file extension: using AVI.\n" ); */
    fmt = av_guess_format( "avi", NULL, NULL );
  }
  if( !fmt ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: Could not find suitable output format\n" );
    return 1;
  }

    /* allocate the output media context */
  oc = avformat_alloc_context();
  if( !oc ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: Memory error\n" );
    return 1;
  }
  oc->oformat = fmt;
  snprintf( oc->filename, sizeof( oc->filename ), "%s", filename );

    /* add the audio and video streams using the default format codecs
       and initialize the codecs */
  video_st = NULL;
  audio_st = NULL;
  vcodec = fmt->video_codec;
  acodec = fmt->audio_codec;
  if ( vcodec != CODEC_ID_NONE ) {
    AVCodec *c;
    if( settings_current.movie_vcodec != NULL && *settings_current.movie_vcodec != 0 ) {
      c = avcodec_find_encoder_by_name( settings_current.movie_vcodec );
      if( c && c->type == AVMEDIA_TYPE_VIDEO ) {
        vcodec = c->id;
      } else {
        ui_error( UI_ERROR_ERROR, "FFMPEG: Unknown video encoder '%s'.\n", settings_current.movie_vcodec );
        return 1;
      }
    } else if( vcodec_sel > 0 ) {
      c = avcodec_find_encoder_by_name( vcodec_str[vcodec_sel] );
      if( c ) {
        vcodec = c->id;
      } else {
        ui_error( UI_ERROR_ERROR, "FFMPEG: Unknown video encoder '%s'.\n", vcodec_str[vcodec_sel] );
        return 1;
      }
    }
    video_timex = timex;
    frame_rate = frate;
    if( add_video_stream( vcodec, timex ? 640 : 320, timex ? 480 : 240,
	machine_timing[timing - 'A'] ) )
      return 1;
  }
  if ( acodec != CODEC_ID_NONE ) {
    if( settings_current.movie_acodec != NULL && *settings_current.movie_acodec != 0 ) {
      c = avcodec_find_encoder_by_name( settings_current.movie_acodec );
      if( c && c->type == AVMEDIA_TYPE_AUDIO ) {
        acodec = c->id;
      } else {
        ui_error( UI_ERROR_ERROR, "FFMPEG: Unknown audio encoder '%s'.\n", settings_current.movie_acodec );
      }
    } else if( acodec_sel > 0 ) {
      c = avcodec_find_encoder_by_name( acodec_str[acodec_sel] );
      if( c ) {
        acodec = c->id;
      } else {
        ui_error( UI_ERROR_ERROR, "FFMPEG: Unknown audio encoder '%s'.\n", acodec_str[acodec_sel] );
        return 1;
      }
    }
    if( add_audio_stream( acodec, freq, stereo ) ) {
      close_video();
      return 1;
    }
    audio_iframe_size = stereo ? 4 : 2;
  }

    /* set the output parameters (must be done even if no
       parameters). */
  if( av_set_parameters( oc, NULL ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "FFMPEG: Invalid output format parameters\n" );
    close_video();
    close_audio();
    return 1;
  }

/*  dump_format( oc, 0, filename, 1 ); */

    /* now that all the parameters are set, we can open the audio and
       video codecs and allocate the necessary encode buffers */
  if( video_st && open_video( timing ) ) {
    close_video();
    close_audio();
    return 1;
  }
  if( audio_st && open_audio( freq, stereo, machine_timing[timing - 'A'] ) ) {
    close_video();
    close_audio();
    return 1;
  }

    /* open the output file, if needed */
  if( !( fmt->flags & AVFMT_NOFILE ) ) {
    if( url_fopen( &oc->pb, filename, URL_WRONLY ) < 0 ) {
      ui_error( UI_ERROR_ERROR, "FFMPEG: Could not open '%s'\n", filename );
      return 1;
    }
  }
    /* write the stream header, if any */
  av_write_header( oc );
  movie_add_area_ffmpeg( 0, 0, 40, 240 );
  return 0;
}

void
movie_stop_ffmpeg()
{
  int i;
    /* write the trailer, if any.  the trailer must be written
     * before you close the CodecContexts open when you wrote the
     * header; otherwise write_trailer may try to use memory that
     * was freed on av_codec_close() */
  av_write_trailer( oc );

    /* close each codec */
  if( video_st )
    close_video();
  if( audio_st )
    close_audio();

    /* free the streams */
  for( i = 0; i < oc->nb_streams; i++ ) {
    av_freep( &oc->streams[i]->codec );
    av_freep( &oc->streams[i] );
  }

  if( !( fmt->flags & AVFMT_NOFILE ) ) {
        /* close the output file */
    url_fclose(oc->pb);
  }

    /* free the stream */
  av_free( oc );
}

/* util functions */
static void
show_formats()
{
  AVInputFormat *ifmt = NULL;
  AVOutputFormat *ofmt = NULL;
  const char *last_name, *mime_type;

  printf( "List of available FFMPEG file formats:\n"
          "  name       mime type        long name\n"
          "-------------------------------------------------------------------------------\n" );
  last_name= "000";
  for(;;){
    int encode=0;
    const char *name=NULL;
    const char *long_name=NULL;

    while( ( ofmt = av_oformat_next( ofmt ) ) ) {
      if( ( name == NULL || strcmp( ofmt->name, name ) < 0 ) &&
            strcmp( ofmt->name, last_name ) > 0 ) {
        name = ofmt->name;
        long_name = ofmt->long_name;
        mime_type = ofmt->mime_type;
        encode = 1;
      }
    }

    while( ( ifmt = av_iformat_next( ifmt ) ) ) {
      if( ( name == NULL || strcmp( ifmt->name, name ) < 0 ) &&
            strcmp( ifmt->name, last_name ) > 0 ) {
        name = ifmt->name;
        long_name = ifmt->long_name;
        encode = 0;
      }
    }

    if( name == NULL ) break;
    last_name = name;

    if( encode )
      printf(
            "  %-10s %-16s %s\n", name,
            mime_type ? mime_type:"---",
            long_name ? long_name:"---" );
    }
    printf( "-------------------------------------------------------------------------------\n\n" );
}

void
show_codecs( int what )
{
  AVCodec *p = NULL, *p2;
  const char *last_name;

  printf( "List of available FFMPEG %s codecs:\n"
          "  name             long name\n"
          "-------------------------------------------------------------------------------\n", what ? "video" : "audio" );
  last_name= "000";
  for(;;){
    int encode = 0;
    int ok;

    p2 = NULL;
    while( ( p = av_codec_next( p ) ) ) {
      if( ( p2 == NULL || strcmp( p->name, p2->name ) < 0 ) &&
            strcmp( p->name, last_name ) > 0 ) {
        p2 = p;
        encode = 0;
      }
      if( p2 && strcmp( p->name, p2->name ) == 0 ) {
        if( p->encode ) encode=1;
      }
    }
    if( p2 == NULL ) break;
    last_name = p2->name;

    ok = 0;
    switch( p2->type ) {
    case AVMEDIA_TYPE_VIDEO:
      if( encode && what ) ok = 1;
      break;
    case AVMEDIA_TYPE_AUDIO:
      if( encode && !what ) ok = 1;
      break;
    default:
      break;
    }
    if( ok )
      printf( "  %-16s %s\n", p2->name,
/*              p2->mime_type ? p2->mime_type : "---", */
              p2->long_name ? p2->long_name : "---" );
    }
    printf( "-------------------------------------------------------------------------------\n\n" );
}

void
movie_list_ffmpeg( int what )
{
  av_register_all();
  if( what == 0 ) show_formats();
  if( what == 1 ) show_codecs( 0 );
  if( what == 2 ) show_codecs( 1 );
}
#endif	/* ifdef USE_FFMPEG */
