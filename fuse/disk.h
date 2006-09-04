/* disk.h: Routines for handling disk images
   Copyright (c) 2004 Stuart Brady

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

#ifndef FUSE_DISK_H
#define FUSE_DISK_H

typedef struct disk_info {
  int fd;
  char filename[ PATH_MAX ];
  int numlayers;
  int numtracks;
  int numsectors;
  int sectorsize;
  int alternatesides;
  int readonly;

  libspectrum_byte *buffer;
  libspectrum_byte *dirty;
  libspectrum_byte *present;
} disk_info;

int disk_read_byte( disk_info *disk, int layer, int track, int sector,
                    int offset );
int disk_write_byte( disk_info *disk, int layer, int track, int sector,
                     int offset );

int disk_phys_to_linear( disk_info *disk, int layer, int track, int sector );
int disk_sector_size( disk_info *disk, int layer, int track, int sector );

libspectrum_byte *disk_read_sector( disk_info *disk,
                                    int layer, int track, int sector );

disk_info *disk_image_write( disk_info *disk, const char *filename );

#endif                  /* #ifndef FUSE_DISK_H */
