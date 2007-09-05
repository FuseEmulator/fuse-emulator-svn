## Process this file with automake to produce Makefile.in
## Copyright (c) 1999-2002 Philip Kendall

## $Id$

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk

AUTOMAKE_OPTIONS = foreign

SUBDIRS = hacking \
	man

bin_PROGRAMS = @AUDIOTOOLS@ \
	       createhdf \
	       listbasic \
	       raw2hdf \
	       @RZXCHECK@ \
	       rzxdump \
	       rzxtool \
	       scl2trd \
	       snap2tzx \
	       snapconv \
	       tapeconv \
	       tzxlist

EXTRA_PROGRAMS = rzxcheck audio2tape tape2wav

INCLUDES = @LIBSPEC_CFLAGS@ @AUDIOFILE_CFLAGS@

createhdf_SOURCES = ide.c createhdf.c

listbasic_SOURCES = listbasic.c utils.c
listbasic_LDADD = @LIBSPEC_LIBS@

raw2hdf_SOURCES = ide.c raw2hdf.c

rzxcheck_SOURCES = rzxcheck.c keys.c utils.c
rzxcheck_LDADD = @LIBSPEC_LIBS@

rzxdump_SOURCES = rzxdump.c utils.c
rzxdump_LDADD = @LIBSPEC_LIBS@

rzxtool_SOURCES = rzxtool.c utils.c
rzxtool_LDADD = @LIBSPEC_LIBS@

scl2trd_SOURCES = scl2trd.c

snap2tzx_SOURCES = snap2tzx.c utils.c
snap2tzx_LDADD = @LIBSPEC_LIBS@

snapconv_SOURCES = snapconv.c utils.c
snapconv_LDADD = @LIBSPEC_LIBS@

tapeconv_SOURCES = tapeconv.c utils.c
tapeconv_LDADD = @LIBSPEC_LIBS@

tzxlist_SOURCES = tzxlist.c utils.c
tzxlist_LDADD = @LIBSPEC_LIBS@

audio2tape_SOURCES = audio2tape.c utils.c
audio2tape_LDADD = @LIBSPEC_LIBS@ @AUDIOFILE_LIBS@

tape2wav_SOURCES = tape2wav.c utils.c
tape2wav_LDADD = @LIBSPEC_LIBS@ @AUDIOFILE_LIBS@

noinst_HEADERS = ide.h utils.h