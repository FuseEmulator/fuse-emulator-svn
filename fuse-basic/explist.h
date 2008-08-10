/* explist.h: a list of expressions
   Copyright (c) 2002-2004 Philip Kendall

   $Id: explist.h,v 1.4 2004-04-25 22:29:52 pak21 Exp $

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

#ifndef BASIC_EXPLIST_H
#define BASIC_EXPLIST_H

#include <stdlib.h>

#include "numexp.h"
#include "strexp.h"

enum expression_type {

  EXPRESSION_NUMEXP,
  EXPRESSION_STREXP,

};

struct expression {

  enum expression_type type;

  union {

    struct numexp *numexp;
    struct strexp *strexp;

  } types;

};

/* And a list of expressions */

struct explist {

  GSList *items;

};

error_t expression_new_numexp( struct expression **expression,
			       struct numexp *numexp );
error_t expression_new_strexp( struct expression **expression,
			       struct strexp *strexp );

error_t explist_new( struct explist **list );

error_t explist_append_numexp( struct explist *list, struct numexp *numexp );
error_t explist_append_strexp( struct explist *list, struct strexp *strexp );

/* Put variable references into the expression list; used for creating
   a list of variables as used by DEF FN */
error_t explist_append_forvar( struct explist *list, const char *name );
error_t explist_append_strvar( struct explist *list, const char *name );

/* Convert a list of numeric expressions into an array of integer values */
error_t explist_to_integer_array( struct program *program,
				  struct explist *list,
				  size_t **values, size_t *length );

error_t explist_free( struct explist *explist );

#endif				/* #ifndef BASIC_EXPLIST_H */
