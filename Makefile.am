## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2003 Philip Kendall

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
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##
## Author contact information:
##
## E-mail: pak21-fuse@srcf.ucam.org
## Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libspectrum.la

libspectrum_la_SOURCES = dck.c \
			 libspectrum.c \
			 myglib.c \
			 rzx.c \
			 sna.c \
			 snapshot.c \
			 snap_accessors.c \
			 tap.c \
			 tape.c \
		 	 timings.c \
			 tzx_read.c \
			 tzx_write.c \
			 warajevo_read.c \
			 z80.c \
			 zlib.c

libspectrum_la_LDFLAGS = -version-info 1:0:1

BUILT_SOURCES = libspectrum.h snap_accessors.c

include_HEADERS = libspectrum.h
noinst_HEADERS = internals.h

noinst_PROGRAMS = make-perl

libspectrum.h: libspectrum.h.in generate.pl
	@PERL@ -p generate.pl libspectrum.h.in > libspectrum.h

generate.pl: make-perl
	./make-perl > generate.pl

snap_accessors.c: accessor.pl snap_accessors.txt
	@PERL@ accessor.pl snap_accessors.txt > snap_accessors.c

INCLUDES = @GLIB_CFLAGS@

EXTRA_DIST = libspectrum.h.in doc/libspectrum.txt
