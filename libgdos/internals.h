/* internals.h:
   Copyright (c) 2007 Stuart Brady

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

   E-mail: sdbrady@ntlworld.com

*/

#ifndef LIBGDOS_INTERNALS_H
#define LIBGDOS_INTERNALS_H

#include <stdio.h>
#include <libdsk.h>

#include "libgdos.h"

struct libgdos_disk {
  DSK_PDRIVER driver;
  DSK_GEOMETRY geom;
};

struct libgdos_dir {
  libgdos_disk *disk;

  int length;
  int current;
  int track;
  int sector;
  int half;
};

struct zx_basic_info {
  int length;
  int start;
  int len_novars;
  int autostart;
};

struct zx_array_info {
  int length;
  int start;
  char name[2];
};

/* also used for the screen$, execute and unidos create types */
struct zx_code_info {
  int length;
  int start;
  int autorun;
};

/* 48K and 128K snapshots */
struct zx_snapshot_info {
  int iy;
  int ix;
  int de_;
  int bc_;
  int hl_;
  int af_;
  int de;
  int bc;
  int hl;
  int i;
  int sp;
};

struct opentype_info {
  int blocks;
  int modlen;
};

struct unidos_subdir_info {
  int blocks;
  int modlen;
  int capacity;
};

struct libgdos_file {
  libgdos_disk *disk;

  int offset;
  int length;

  int track;
  int sector;
  int secofs;

  int basetrack;
  int basesector;
};

#endif                          /* #ifndef LIBGDOS_INTERNALS_H */
