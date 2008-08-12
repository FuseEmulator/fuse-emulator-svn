/* parse.h: routines to parse a buffer to the specified type
   Copyright (c) 2004 Philip Kendall

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

#ifndef BASIC_PARSE_H
#define BASIC_PARSE_H

#include "numexp.h"
#include "program.h"

enum parse_target_type {

  PARSE_PROGRAM,
  PARSE_NUMEXP,
  PARSE_STREXP,

};

extern enum parse_target_type parse_target;

extern struct program *parse_program_target;
extern struct numexp *parse_numexp_target;
extern struct strexp *parse_strexp_target;

struct program* parse_program( const char *buffer, size_t length );
struct numexp* parse_numexp( const char *buffer, size_t length );
struct strexp *parse_strexp( const char *buffer, size_t length );

int parse_fill_buffer( char *buf, int *result, int max_size );

int yylex( void );
void yyerror( char *s );

#endif				/* #ifndef BASIC_PARSE_H */
