dnl Process this file with autoconf to produce a configure script.
dnl Copyright (c) 2002-2003 Philip Kendall

dnl $Id$

dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License along
dnl with this program; if not, write to the Free Software Foundation, Inc.,
dnl 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
dnl
dnl Author contact information:
dnl
dnl E-mail: philip-fuse@shadowmagic.org.uk

AC_INIT(basic.c)
AM_CONFIG_HEADER(config.h)

dnl Use automake to produce `Makefile.in'
AM_INIT_AUTOMAKE(basic, 0.0.2)

dnl Checks for programs.
AC_PROG_CC
AM_PROG_LEX
AC_PROG_YACC

AC_PATH_PROG(PERL,perl)
AC_ARG_VAR(PERL,[path to a working Perl executable])

dnl Check for libraries
AC_CHECK_LIB(m,cos)

dnl Generate basicy.h
YFLAGS="$YFLAGS -d"
AC_SUBST(YFLAGS)

dnl Turn on warnings if we're using gcc
if test "$ac_cv_prog_cc_g" = yes; then
    CFLAGS="$CFLAGS -Wall"
fi

AC_OUTPUT(Makefile)
