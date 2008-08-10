/* basic.h: generic things
   Copyright (C) 2002-2004 Philip Kendall

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

#ifndef BASIC_BASIC_H
#define BASIC_BASIC_H

extern const char *progname;

typedef enum error_t {

  BASIC_ERROR_NONE = 0,
  BASIC_ERROR_LOGIC,
  BASIC_ERROR_MEMORY,
  BASIC_ERROR_UNKNOWN,

} error_t;

#endif				/* #ifndef BASIC_BASIC_H */
