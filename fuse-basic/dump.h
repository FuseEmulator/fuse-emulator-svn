/* dump.h: dump out a BASIC program
   Copyright (C) 2002 Philip Kendall

   $Id: dump.h,v 1.2 2002-07-19 16:05:02 pak21 Exp $

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

   E-mail: pak21@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef BASIC_DUMP_H
#define BASIC_DUMP_H

#include <glib.h>

#ifndef BASIC_PROGRAM_H
#include "program.h"
#endif				/* #ifndef BASIC_PROGRAM_H */

int basic_dump( struct program *program );

#endif				/* #ifndef BASIC_DUMP_H */
