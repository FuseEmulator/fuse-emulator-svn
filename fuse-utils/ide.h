/* ide.h: constants etc used for IDE emulation
   Copyright (c) 2004-2005 Philip Kendall

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

   Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_UTILS_IDE_H
#define FUSE_UTILS_IDE_H

#include <stdlib.h>

#define HDF_HEADER_LENGTH 0x80

extern const char *HDF_SIGNATURE;
extern const size_t HDF_SIGNATURE_LENGTH;
extern const char HDF_VERSION;
extern const size_t HDF_DATA_OFFSET;

enum {

  HDF_SIGNATURE_OFFSET   = 0x00,
  HDF_VERSION_OFFSET     = 0x07,
  HDF_COMPACT_OFFSET     = 0x08,
  HDF_DATA_OFFSET_OFFSET = 0x09,
  HDF_IDENTITY_OFFSET    = 0x16,

};

enum {

  IDENTITY_CYLINDERS_OFFSET    = 2,
  IDENTITY_HEADS_OFFSET        = 6,
  IDENTITY_SECTORS_OFFSET      = 12,
  IDENTITY_MODEL_NUMBER_OFFSET = 54,
  IDENTITY_CAPABILITIES_OFFSET = 98,

};

#endif				/* #ifndef FUSE_UTILS_IDE_H */
