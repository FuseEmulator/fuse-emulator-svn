## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2004 Philip Kendall

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

libspectrum_la_SOURCES = bzip2.c \
			 creator.c \
			 crypto.c \
			 dck.c \
			 ide.c \
			 libspectrum.c \
			 microdrive.c \
			 plusd.c \
			 rzx.c \
			 sna.c \
			 snp.c \
			 snapshot.c \
			 snap_accessors.c \
			 sp.c \
			 symbol_table.c \
			 szx.c \
			 tap.c \
			 tape.c \
			 tape_accessors.c \
			 tape_block.c \
			 tape_set.c \
		 	 timings.c \
			 tzx_read.c \
			 tzx_write.c \
			 warajevo_read.c \
			 z80.c \
			 z80em.c \
			 zlib.c \
			 zxs.c

libspectrum_la_LDFLAGS = -version-info 4:0:2 -no-undefined

libspectrum_la_LIBADD = myglib/libmyglib.la -lm

BUILT_SOURCES = libspectrum.h snap_accessors.c tape_accessors.c tape_set.c

nodist_include_HEADERS = libspectrum.h
noinst_HEADERS = internals.h

AM_CFLAGS = -DLIBSPECTRUM_EXPORTS

make-perl$(EXEEXT): make-perl.c
	$(CC_FOR_BUILD) -I. -o $@ $<

libspectrum.h: libspectrum.h.in generate.pl snap_accessors.txt tape_accessors.txt config.h
	@PERL@ -p generate.pl libspectrum.h.in > $@

generate.pl: make-perl$(EXEEXT) generate.pl.in
	./make-perl$(EXEEXT) > $@
	cat generate.pl.in >> $@

snap_accessors.c: accessor.pl snap_accessors.txt
	@PERL@ accessor.pl snap_accessors.txt > $@

tape_accessors.c: tape_accessors.pl tape_accessors.txt
	@PERL@ tape_accessors.pl tape_accessors.txt > $@

tape_set.c: tape_set.pl tape_accessors.txt
	@PERL@ tape_set.pl tape_accessors.txt > $@

INCLUDES = @GLIB_CFLAGS@

EXTRA_DIST = accessor.pl \
	     generate.pl.in \
	     make-perl.c \
	     tape_accessors.pl \
	     tape_accessors.txt \
	     tape_block.h \
	     tape_set.pl \
	     libspectrum.h.in \
	     snap_accessors.txt


SUBDIRS = doc \
	  myglib
