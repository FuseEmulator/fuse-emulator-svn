/* scorpion.h: Scorpion 256K specific routines
   Copyright (c) 1999-2004 Philip Kendall and Fredrick Meunier
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

#ifndef FUSE_SCORPION_H
#define FUSE_SCORPION_H

#ifndef FUSE_MACHINE_H
#include "machine.h"
#endif			/* #ifndef FUSE_MACHINE_H */

int scorpion_init( fuse_machine_info *machine );
void scorpion_memoryport2_write( libspectrum_word port, libspectrum_byte b );

#endif			/* #ifndef FUSE_SCORPION_H */
