/* statement.h: a BASIC statement
   Copyright (c) 2002-2004 Philip Kendall

   $Id: statement.h,v 1.21 2004-04-30 15:43:00 pak21 Exp $

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

#ifndef BASIC_STATEMENT_H
#define BASIC_STATEMENT_H

#ifndef BASIC_EXPLIST_H
#include "explist.h"
#endif				/* #ifndef BASIC_EXPLIST_H */

#ifndef BASIC_TOKEN_H
#include "token.h"
#endif				/* #ifndef BASIC_TOKEN_H */

enum statement_type {

  /* A statement which takes no arguments */
  STATEMENT_NO_ARGS_ID,

  /* A statement which takes one numerical argument */
  STATEMENT_NUMEXP_ARG_ID,

  /* A statement which takes zero or one numerical arguments */
  STATEMENT_OPTNUMEXP_ARG_ID,

  /* A statement which takes two numerical arguments */
  STATEMENT_TWONUMEXP_ARG_ID,

  /* A statement which takes one string argument */
  STATEMENT_STREXP_ARG_ID,

  /* A numeric let statement */
  STATEMENT_LET_NUMERIC_ID,

  /* An assignment to a numeric array element */
  STATEMENT_LET_NUMERIC_ARRAY_ID,

  /* A string let statement */
  STATEMENT_LET_STRING_ID,

  /* A print statement */
  STATEMENT_PRINT_ID,

  /* An IF statement */
  STATEMENT_IF_ID,

  /* A FOR loop */
  STATEMENT_FOR_ID,

  /* A NEXT statement */
  STATEMENT_NEXT_ID,

  /* A DEF FN statement */
  STATEMENT_DEFFN_ID,

  /* A DIM statement creating a number array */
  STATEMENT_DIM_NUMERIC_ID,

  /* An INPUT statement */
  STATEMENT_INPUT_ID,

};

/* A statement which takes one (optional) numerical argument */
struct statement_numexp_arg {

  enum basic_token type;

  int expression_present;
  struct numexp *exp;

};

/* A statement which takes two numerical arguments */
struct statement_twonumexp_arg {

  enum basic_token type;
  struct numexp *exp1, *exp2;

};

/* A statement which takes one string argument */
struct statement_strexp_arg {

  enum basic_token type;
  struct strexp *exp;

};

/* A numeric let statement */
struct statement_let_numeric {

  char *name;
  struct numexp *exp;

};

/* An assignment to a numeric array element */
struct statement_let_numeric_array {

  char *name;
  struct explist *subscripts;
  struct numexp *exp;

};

/* A string let statement */
struct statement_let_string {

  char *name;
  struct strexp *exp;
  struct slicer *slicer;

};

/* An IF statement */
struct statement_if {

  struct numexp *condition;
  struct statement *true_clause;

};

/* A FOR loop */
struct statement_for {

  char *control;
  struct numexp *start, *end, *step;

};

/* A DEF FN statement */
struct statement_deffn {

  char *name;
  struct explist *arguments;
  struct numexp *definition;

};

/* A DIM statement creating a number array */
struct statement_dim_numeric {

  char *name;
  struct explist *dimensions;

};

/* The top-level statement type */
struct statement {

  enum statement_type type;

  union {

    int no_args;
    struct statement_numexp_arg numexp_arg;
    struct statement_twonumexp_arg twonumexp_arg;
    struct statement_strexp_arg strexp_arg;
    struct statement_let_numeric let_numeric;
    struct statement_let_numeric_array let_numeric_array;
    struct statement_let_string let_string;
    struct printlist *printlist;
    struct statement_if if_statement;
    struct statement_for for_loop;
    char *next;
    struct statement_deffn deffn;
    struct statement_dim_numeric dim_numeric;

  } types;

};

error_t statement_new_noarg( struct statement **statement, int type );
error_t statement_new_numexp( struct statement **statement, int type,
			      struct numexp *exp );
error_t statement_new_optnumexp( struct statement **statement, int type,
				 struct numexp *exp );
error_t statement_new_twonumexp( struct statement **statement, int type,
				 struct numexp *exp1, struct numexp *exp2 );
error_t statement_new_strexp( struct statement **statement, int type,
			      struct strexp *strexp );
error_t statement_new_let_numeric( struct statement **statement,
				   const char *name, struct numexp *exp );
error_t statement_new_let_numeric_array( struct statement **statement,
					 const char *name,
					 struct explist *subscripts,
					 struct numexp *exp );
error_t statement_new_let_string( struct statement **statement,
				  const char *name, struct strexp *exp,
				  struct slicer *slicer );
error_t statement_new_print( struct statement **statement,
			     struct printlist *printlist );
error_t statement_new_if( struct statement **statement,
			  struct numexp *condition,
			  struct statement *true_clause );
error_t statement_new_for( struct statement **statement,
			   const char *control, struct numexp *start,
			   struct numexp *end, struct numexp *step );
error_t statement_new_next( struct statement **statement,
			    const char *control );
error_t statement_new_deffn( struct statement **statement, const char *name,
			     struct explist *arguments,
			     struct numexp *definition );
error_t statement_new_dim_numeric( struct statement **statement,
				   const char *name,
				   struct explist *dimensions );
error_t statement_new_input( struct statement **statement,
			     struct printlist *printlist );

error_t statement_execute( struct program *program,
			   struct statement *statement );

void statement_free( gpointer data, gpointer user_data );

#endif				/* #ifndef BASIC_STATEMENT_H */
