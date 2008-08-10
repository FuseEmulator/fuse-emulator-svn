/* dump.c: dump out a BASIC program
   Copyright (C) 2002 Philip Kendall

   $Id: dump.c,v 1.10 2003-09-11 14:41:04 pak21 Exp $

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

#include "config.h"

#include <stdio.h>

#include <glib.h>

#include "basic.h"
#include "line.h"
#include "numexp.h"
#include "program.h"
#include "token.h"

static void show_line( gpointer data, gpointer user_data );
static void show_statement( gpointer data, gpointer user_data );
static void show_numexp( struct numexp *exp );

int
basic_dump( struct program *program )
{
  g_slist_foreach( program->lines, show_line, NULL );

  return BASIC_ERROR_NONE;
}

static void
show_line( gpointer data, gpointer user_data )
{
  struct line *line = (struct line*)data;

  printf( "%d ", line->line_number );

  g_slist_foreach( line->statements, show_statement, NULL );

  printf( "\n" );
}

static void
show_statement( gpointer data, gpointer user_data )
{
  struct statement *statement = (struct statement*)data;

  switch( statement->type ) {

  case STATEMENT_NO_ARGS_ID:
    switch( statement->types.no_args ) {
    case CLS:      printf( "CLS" );      break;
    case CONTINUE: printf( "CONTINUE" ); break;
    case COPY:     printf( "COPY" );     break;
    case NEW:      printf( "NEW" );      break;
    case RETURN:   printf( "RETURN" );   break;
    case STOP:     printf( "STOP" );     break;
    default: printf( "(unknown no args statement %d)",
		     statement->types.no_args );
    }
    break;

  case STATEMENT_NUMEXP_ARG_ID:
    switch( statement->types.numexp_arg.type ) {
    case BORDER: printf( "BORDER " ); break;
    case GOSUB:  printf( "GO SUB " ); break;
    case GOTO:   printf( "GO TO " );  break;
    case PAUSE:  printf( "PAUSE " );  break;
    default: printf( "(unknown numexp arg statement %d) ",
		     statement->types.numexp_arg.type );
    }
    show_numexp( statement->types.numexp_arg.exp );
    break;

  default:
    printf( "(unknown statement type %d)", statement->type );
    break;

  }

  printf( ": " );
}

static void
show_numexp( struct numexp *exp )
{
  switch( exp->type ) {
  case NUMEXP_NUMBER_ID: printf( "%f", exp->types.number ); break;

  case NUMEXP_NO_ARG_ID:
    switch( exp->types.no_arg ) {
    case PI:  printf( "PI" );  break;
    case RND: printf( "RND" ); break;
    default: printf( "(unknown no arg numexp %d)", exp->types.no_arg );
    }
    break;

  case NUMEXP_ONE_ARG_ID:
    switch( exp->types.one_arg.type ) {
    case SPECTRUM_ABS: printf( "ABS( " ); break;
    case ACS: printf( "ACS( " ); break;
    case ASN: printf( "ASN( " ); break;
    case ATN: printf( "ATN( " ); break;
    case COS: printf( "COS( " ); break;
    case EXP: printf( "EXP( " ); break;
    case IN:  printf( "IN( " );  break;
    case INT: printf( "INT( " ); break;
    case LEN: printf( "LEN( " ); break;
    case LN:  printf( "LN( " );  break;
    case NOT: printf( "NOT( " ); break;
    case PEEK: printf( "PEEK( " ); break;
    case SGN: printf( "SGN( " ); break;
    case SIN: printf( "SIN( " ); break;
    case SQR: printf( "SQR( " ); break;
    case TAN: printf( "TAN( " ); break;
    case USR: printf( "USR( " ); break;
    default: printf( "(unknown one arg numexp %d) ",
		     exp->types.one_arg.type );
    }
    show_numexp( exp->types.one_arg.exp );
    printf( " )" );
    break;

  case NUMEXP_TWO_ARG_ID:
    switch( exp->types.two_arg.type ) {
    case '+': case '-': case '*': case '/': case '^':
      printf( "( " ); show_numexp( exp->types.two_arg.exp1 );
      printf( " %c ", exp->types.two_arg.type );
      show_numexp( exp->types.two_arg.exp2 ); printf( " )" );
      break;
    default:
      printf( "(unknown two arg numexp %d) ",
	      exp->types.two_arg.type );
    }
    break;

  default: printf( "(unknown numexp type %d)", exp->type );
  }
}
