/* printlist.h: a list of items to a PRINT command
   Copyright (C) 2002 Philip Kendall

   $Id: printlist.h,v 1.2 2003-09-30 10:24:02 pak21 Exp $

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

#ifndef BASIC_PRINTLIST_H
#define BASIC_PRINTLIST_H

#include <glib.h>

#ifndef BASIC_BASIC_H
#include "basic.h"
#endif				/* #ifndef BASIC_BASIC_H */

#ifndef BASIC_TOKEN_H
#include "token.h"
#endif				/* #ifndef BASIC_TOKEN_H */

/* First a 'printitem': something which can appear in a printlist */

enum printitem_type {

  PRINTITEM_NUMEXP,	/* A numerical expression */
  PRINTITEM_STREXP,	/* A string expression */
  PRINTITEM_SEPARATOR,	/* A separator: one of { ; , ' } */

};

struct printitem {

  enum printitem_type type;

  union {

    struct numexp *numexp;
    struct strexp *strexp;
    enum basic_token separator;

  } types;

};

error_t printitem_new_numexp( struct printitem **printitem,
			      struct numexp *numexp );
error_t printitem_new_strexp( struct printitem **printitem,
			      struct strexp *strexp );
error_t printitem_new_separator( struct printitem **printitem,
				 enum basic_token separator );

/* And then a printlist itself: a list of printitems */

struct printlist {

  GSList *items;

};

error_t printlist_new( struct printlist **list );
error_t printlist_append( struct printlist *list, struct printitem *item );
error_t printlist_free( struct printlist *printlist );

#endif				/* #ifndef BASIC_PRINTLIST_H */
