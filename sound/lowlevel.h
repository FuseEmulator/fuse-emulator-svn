/* lowlevel.h: work out which lowlevel sound routines to use
   Copyright (c) 2004 Philip Kendall

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

*/

/* This header file should define HAVE_SOUND if any form of sound API
   is available. If so, it should define one of the SOUND_xxx defines. */

#ifndef FUSE_SOUND_LOWLEVEL_H
#define FUSE_SOUND_LOWLEVEL_H

#if defined UI_SDL

#define HAVE_SOUND
#define SOUND_SDL

#elif defined USE_LIBAO	/* #if defined UI_SDL */

#define HAVE_SOUND
#define SOUND_AO

#elif defined HAVE_SYS_AUDIO_H		/* #if defined UI_SDL */

#include <sys/audio.h>

#if defined AUDIO_SETINFO

#define HAVE_SOUND
#define SOUND_SUN

#elif defined AUDIO_FORMAT_LINEAR16BIT	/* #if defined AUDIO_SETINFO */

#define HAVE_SOUND
#define SOUND_HP

#endif					/* #if defined AUDIO_SETINFO */

#elif defined HAVE_SYS_SOUNDCARD_H	/* #if defined UI_SDL */

#define HAVE_SOUND
#define SOUND_OSS

/* TODO: does OpenBSD have <sys/audio.h>? Solaris does, so the above
   check will do there */
#elif defined HAVE_SYS_AUDIOIO_H	/* #if defined UI_SDL */

#define HAVE_SOUND
#define SOUND_SUN

#elif defined HAVE_DSOUND_H		/* #if defined UI_SDL */

#define HAVE_SOUND
#define SOUND_DX

#endif					/* #if defined UI_SDL */

#endif			/* #ifndef FUSE_SOUND_LOWLEVEL_H */   
