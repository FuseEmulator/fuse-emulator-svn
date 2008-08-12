/* strexp.c: A string expression
   Copyright (c) 2002-2004 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef BASIC_STREXP_H
#define BASIC_STREXP_H

#include "program.h"
#include "token.h"

/* A string slicer */

struct slicer {

  struct numexp *start, *end;

};

error_t slicer_new( struct slicer **slicer, struct numexp *start,
		    struct numexp *end );

/* A string expression */

enum strexp_type {

  /* A simple string */
  STREXP_STRING_ID,

  /* A string variable */
  STREXP_VARIABLE_ID,

  /* A strexp which takes one numexp as an argument */
  STREXP_NUMEXP_ID,

  /* A strexp which takes one strexp as an argument */
  STREXP_STREXP_ID,

  /* A strexp which takes two strexps as arguments */
  STREXP_TWOSTREXP_ID,

  /* A `strexp AND numexp' expression */
  STREXP_AND_ID,

  /* A sliced string expression */
  STREXP_SLICER_ID,

};

/* A strexp which takes one numexp as an argument */
struct strexp_numexp {

  int type;
  struct numexp *exp;

};

/* A strexp which takes one strexp as an argument */
struct strexp_strexp {

  int type;
  struct strexp *exp;

};

/* A strexp which takes two strexps as arguments */
struct strexp_twostrexp {

  int type;
  struct strexp *exp1, *exp2;

};

/* A `strexp AND numexp' expression */
struct strexp_and {

  struct strexp *exp1;
  struct numexp *exp2;

};

/* A sliced string expression */
struct strexp_slicer {
  
  struct strexp *exp;
  struct slicer *slicer;

};

/* The top level strexp structure */
struct strexp {

  enum strexp_type type;

  union {

    /* A simple string */
    struct string *string;

    /* A variable reference */
    char *name;

    /* A strexp which takes one numexp as an argument */
    struct strexp_numexp numexp;

    /* A strexp which takes one numexp as an argument */
    struct strexp_strexp strexp;

    /* A strexp which takes two strexps as arguments */
    struct strexp_twostrexp twostrexp;

    /* A `strexp AND numexp' expression */
    struct strexp_and and;

    /* A sliced string expression */
    struct strexp_slicer slicer;

  } types;

};

error_t strexp_new_variable( struct strexp **numexp, const char *name );
error_t strexp_new_string( struct strexp **strexp, struct string *string );
error_t strexp_new_numexp( struct strexp **strexp, enum basic_token type,
			   struct numexp *exp );
error_t strexp_new_strexp( struct strexp **strexp, enum basic_token type,
			   struct strexp *exp );
error_t strexp_new_twostrexp( struct strexp **strexp, int type,
			      struct strexp *exp1, struct strexp *exp2 );
error_t strexp_new_and( struct strexp **strexp,
			struct strexp *exp1, struct numexp *exp2 );
error_t strexp_new_slicer( struct strexp **strexp, struct strexp *exp,
			   struct slicer *slicer );

error_t strexp_eval( struct program *program, struct strexp *strexp,
		     struct string **string );
error_t strexp_free( struct strexp *strexp );

#endif				/* #ifndef BASIC_STREXP_H */
