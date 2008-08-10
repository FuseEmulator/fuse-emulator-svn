/* program.h: routines for dealing with an entire program
   Copyright (C) 2002-2004 Philip Kendall

   $Id: program.h,v 1.26 2004-05-03 14:24:38 pak21 Exp $

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

#ifndef BASIC_PROGRAM_H
#define BASIC_PROGRAM_H

#include <glib.h>

struct program;

#include "line.h"
#include "spectrum-string.h"

enum program_errors {

  BASIC_PROGRAM_ERROR_NONE = 0,

  /* NB: BASIC `error' 0 does *not* have the value 0 */
  BASIC_PROGRAM_ERROR_0,	/* 0 OK */
  BASIC_PROGRAM_ERROR_1,	/* 1 NEXT without FOR */
  BASIC_PROGRAM_ERROR_2,	/* 2 Variable not found */
  BASIC_PROGRAM_ERROR_3,	/* 3 Subscript wrong */
  BASIC_PROGRAM_ERROR_7,	/* 7 RETURN without GOSUB */
  BASIC_PROGRAM_ERROR_9,	/* 9 Stop statement */
  BASIC_PROGRAM_ERROR_A,	/* A Invalid argument */
  BASIC_PROGRAM_ERROR_B,	/* B Integer out of range */
  BASIC_PROGRAM_ERROR_C,	/* C Nonsense in BASIC */
  BASIC_PROGRAM_ERROR_I,	/* I FOR without NEXT */
  BASIC_PROGRAM_ERROR_P,	/* P FN without DEF */
  BASIC_PROGRAM_ERROR_Q,	/* Q Parameter error */

};

/* A symbol table */
struct program_symbol_table {

  GHashTable *numeric;		/* Numeric variables */
  GHashTable *string;		/* String variables */

  GHashTable *numeric_array;	/* Numeric arrays */

};

/* A function definition */
struct program_function {

  struct explist *arguments;
  struct numexp *definition;

};

struct program {

  GSList *lines;		/* A linked list of program lines */

  GSList *current_line;		/* The line we're about to execute */
  GSList *current_statement;	/* And the statement on that line */

  enum program_errors error;	/* A Spectrum BASIC error */

  struct program_symbol_table variables; /* Global variables */
  GSList *scratchpads;		/* Lexical variables */

  GArray *gosub_stack;		/* The GO SUB return stack */

  GHashTable *functions;	/* Defined functions */
  
};

error_t program_initialise( struct program *program );
error_t program_add_line( struct program* program, struct line *line );

error_t program_find_functions( struct program *program );

error_t program_execute_statement( struct program *program );

error_t program_insert_numeric( struct program *program, const char *name,
				float value );
error_t program_lookup_numeric( struct program *program, const char *name,
				float *value );
error_t program_insert_string( struct program *program, const char *name,
			       struct string *string );
error_t program_lookup_string( struct program *program, const char *name,
			       struct string *string );

error_t program_create_numeric_array( struct program *program,
				      const char *name, size_t *dimensions,
				      size_t count );
error_t program_set_numeric_array_element( struct program *program,
					   const char *name,
					   struct explist *subscripts,
					   float value );
error_t program_get_numeric_array_element( struct program *program,
					   const char *name,
					   struct explist *subscripts,
					   float *value );

error_t program_push_scratchpad( struct program *program,
				 struct explist *names,
				 struct explist *values );
error_t program_pop_scratchpad( struct program *program );

error_t program_clear_variables( struct program *program );

error_t program_set_line( struct program *program, float line );
error_t program_gosub( struct program *program, float line );
error_t program_gosub_return( struct program *program );

error_t program_initialise_for_loop( struct program *program,
				     const char *control, float start,
				     float end, float step );
error_t program_for_loop_next( struct program *program, const char *control );

const char* program_strerror( enum program_errors program_error );

error_t program_free( struct program *program );

#endif				/* #ifndef BASIC_PROGRAM_H */
