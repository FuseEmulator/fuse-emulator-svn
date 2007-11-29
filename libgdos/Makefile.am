## Process this file with automake to produce Makefile.in
## Copyright (c) 2007 Stuart Brady

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
## E-mail: sdbrady@ntlworld.com

AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libgdos.la

libgdos_la_SOURCES = dir.c \
		     file.c \
		     image.c

libgdos_la_LDFLAGS = -version-info 1:0:0 -no-undefined

libgdos_la_LIBADD = -lm

include_HEADERS = libgdos.h
noinst_HEADERS = internals.h

noinst_PROGRAMS =
noinst_DATA =

AM_CFLAGS = -DLIBGDOS_EXPORTS

EXTRA_DIST = README

include doc/Makefile.am
include hacking/Makefile.am
