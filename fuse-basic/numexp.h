/* numexp.h: a BASIC numerical expression
   Copyright (c) 2002-2004 Philip Kendall

   $Id: numexp.h,v 1.14 2004-04-25 22:29:52 pak21 Exp $

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

#ifndef BASIC_NUMEXP_H
#define BASIC_NUMEXP_H

#include "program.h"
#include "token.h"

enum numexp_type {

  /* A simple number */
  NUMEXP_NUMBER_ID,

  /* A reference to a simple variable */
  NUMEXP_VARIABLE_ID,

  /* A reference to an array variable */
  NUMEXP_ARRAY_ID,

  /* A function which takes no arguments */
  NUMEXP_NO_ARG_ID,

  /* A function which takes one numexp argument */
  NUMEXP_ONE_ARG_ID,

  /* A function which takes two numexp arguments */
  NUMEXP_TWO_ARG_ID,

  /* A function which takes one strexp argument */
  NUMEXP_STREXP_ID,

  /* A FN call */
  NUMEXP_FN_ID,

};

/* A reference to an array variable */
struct numexp_array {

  char *name;
  struct explist *subscripts;

};

/* A function which takes one numexp argument */
struct numexp_one_arg {

  enum basic_token type;
  struct numexp *exp;

};

/* A function which takes two numexp arguments */
struct numexp_two_arg {

  enum basic_token type;
  struct numexp *exp1, *exp2;

};

/* A function which takes one strexp argument */
struct numexp_strexp {

  enum basic_token type;
  struct strexp *exp;

};

/* A FN call */
struct numexp_fn {

  char *name;
  struct explist *arguments;

};

/* The top-level numerical expression */
struct numexp {

  enum numexp_type type;

  union {

    /* A simple number */
    float number;

    /* A reference to a simple variable */
    char *name;

    /* A reference to an array variable */
    struct numexp_array array;

    /* A function which takes no arguments */
    int no_arg;

    /* A function which takes one numexp argument */
    struct numexp_one_arg one_arg;

    /* A function which takes two numexp arguments */
    struct numexp_two_arg two_arg;

    /* A function which takes one strexp argument */
    struct numexp_strexp strexp;

    /* A FN call */
    struct numexp_fn function;

  } types;

};

error_t numexp_new_variable( struct numexp **numexp, const char *name );
error_t numexp_new_array( struct numexp **numexp, const char *name,
			  struct explist *subscripts );
error_t numexp_new_noarg( struct numexp **numexp, int type );
error_t numexp_new_onearg( struct numexp **numexp, int type,
			   struct numexp *exp );
error_t numexp_new_twoarg( struct numexp **numexp, int type,
			   struct numexp *exp1, struct numexp *exp2 );
error_t numexp_new_strexp( struct numexp **numexp, int type,
			   struct strexp *exp );
error_t numexp_new_fn( struct numexp **numexp, const char *name,
		       struct explist *explist );

error_t numexp_eval( struct program *program, struct numexp *numexp,
		     float *value );
error_t numexp_free( struct numexp *numexp );

#endif				/* #ifndef BASIC_NUMEXP_H */
