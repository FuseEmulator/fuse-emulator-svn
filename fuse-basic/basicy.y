/* basicy.y: ZX Spectrum BASIC grammar
   Copyright (c) 1998,2002-2004 Philip Kendall

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

%{

#include "config.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basic.h"
#include "explist.h"
#include "line.h"
#include "numexp.h"
#include "parse.h"
#include "printlist.h"
#include "program.h"
#include "statement.h"
#include "strexp.h"
#include "token.h"

%}

%union {

  int integer;
  float real;
  struct string *string;
  char *varname;

  struct slicer *slicer;

  enum basic_token token;

  struct numexp *numexp;
  struct strexp *strexp;

  struct expression *expression;
  struct explist *explist;

  struct printitem *printitem;
  struct printlist *printlist;

  struct statement *statement;

  struct line *line;
  struct program *program;

}

/* Simple tokens */

%token <integer>   LINENUM	/* An integer in the range 0-9999 */
%token <real>      NUM		/* Any other number */

%token <string>	   STRING	/* A simple string */

%token <varname>   FORVAR	/* A one letter variable name */
%token <varname>   NUMVAR	/* A longer variable name */
%token <varname>   STRVAR	/* A string variable name */

%token <token>	   TIMES_DIVIDE	/* '*' or '/' */
%token <token>	   NOARG_COMMAND /* A command which takes no arguments */
%token <token>	   NUMEXP_COMMAND /* One which takes one numeric arg */
%token <token>	   OPTNUMEXP_COMMAND /* Optional numeric arg */
%token <token>	   STREXP_COMMAND /* One string arg */
%token <token>     TWONUMEXP_COMMAND /* Two numeric args */

/* Some unique commands */
%token		   DEFFN_TOKEN
%token		   DIM_TOKEN
%token		   FOR_TOKEN
%token		   INPUT_TOKEN
%token		   LET_TOKEN
%token		   NEXT_TOKEN
%token		   PRINT_TOKEN
%token		   IF_TOKEN

%token		   THEN_TOKEN
%token		   STEP_TOKEN
%token		   FN_TOKEN
%token		   TO_TOKEN

/* Operators */

%token <token>	   NUMEXP_NO_ARG /* A numerical operator which takes no args */
%token <token>     NUMEXP_ONE_ARG /* One which takes one numerical argument */
%token <token>     NUMEXP_TWO_ARG /* Two numerical arguments */
%token <token>	   NUMEXP_STREXP /* One string argument */
%token <token>	   COMPARISON    /* Comparison operator */

%token <token>	   STREXP_NUMEXP /* A strexp which takes one numexp arg */
%token <token>	   STREXP_STREXP /* A strexp which takes one strexp arg */

/* Derived types */

%type <real>       number

%type <varname>	   numericvar
%type <numexp>     numexp
%type <numexp>     optnumexp

%type <slicer>	   slicer
%type <slicer>     optslicer

%type <strexp>	   strexp

%type <explist>	   explist
%type <explist>	   optexplist

%type <numexp>     optstepexp
%type <explist>    numexplist
%type <explist>	   optsvarlist

%type <token>	   printsep

%type <printitem>  printitem

%type <printlist>  printlist
%type <printlist>  printlist1
%type <printlist>  printlist2

%type <statement>  command

%type <line>	   commandlist
%type <line>	   line

%type <program>	   program
%type <program>	   input

/* Operator precedences */

/* Low precedence */

%left OR_TOKEN
%left AND_TOKEN
%left NOT_TOKEN
%left COMPARISON '='
%left '+' '-'
%left TIMES_DIVIDE
%left NEG		/* Unary minus; also used for unary plus */
%right '^'
%left NUMEXP_NO_ARG NUMEXP_ONE_ARG NUMEXP_TWO_ARG NUMEXP_STREXP STREXP_NUMEXP

/* High precedence */

%%

input:   /* empty */ { ; }
       | program     { if( parse_target != PARSE_PROGRAM ) YYERROR;
		       parse_program_target = $1; }
       | numexp { if( parse_target != PARSE_NUMEXP ) YYERROR;
                  parse_numexp_target = $1; }
       | strexp { if( parse_target != PARSE_STREXP ) YYERROR;
                  parse_strexp_target = $1; }
;

program:    line { $$ = malloc( sizeof( *$$ ) );
		   program_initialise( $$ ); 
		   program_add_line( $$, $1 ); }
          | program line { program_add_line( $1, $2 ); }
;

line:   '\n' { $$ = NULL; }
      | LINENUM commandlist '\n' { $$ = $2; $$->line_number = $1; }
;

commandlist:   command           { $$ = malloc( sizeof *$$ );
                                   $$->statements = NULL;
                                   line_append_statement( $$, $1 ); }
             | commandlist ':' command
                                 { $$ = $1;
				   line_append_statement( $$, $3 ); }
;

command:   NOARG_COMMAND { if( statement_new_noarg( &($$), $1 ) !=
			       BASIC_ERROR_NONE )
			     YYERROR;
	                 }
	 | NUMEXP_COMMAND numexp
           { if( statement_new_numexp( &($$), $1, $2 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
         | OPTNUMEXP_COMMAND optnumexp
           { if( statement_new_optnumexp( &($$), $1, $2 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
         | TWONUMEXP_COMMAND numexp ',' numexp
           { if( statement_new_twonumexp( &($$), $1, $2, $4 ) !=
		 BASIC_ERROR_NONE )
	       YYERROR;
	   }
         | LET_TOKEN numericvar '=' numexp
	   { if( statement_new_let_numeric( &($$), $2, $4 ) !=
		 BASIC_ERROR_NONE ) {
	       YYERROR;
	       free( $2 );
	     }
	     free( $2 );
	   }
	 | LET_TOKEN FORVAR '(' explist ')' '=' numexp
	   { if( statement_new_let_numeric_array( &($$), $2, $4, $7 ) !=
		 BASIC_ERROR_NONE ) {
	       free( $2 );
	       YYERROR;
	     }
	     free( $2 );
	   }
         | LET_TOKEN STRVAR optslicer '=' strexp
	   { if( statement_new_let_string( &($$), $2, $5, $3 ) !=
		 BASIC_ERROR_NONE ) {
	       free( $2 );
	       YYERROR;
	     }
	     free( $2 );
	   }
	 | STREXP_COMMAND strexp
	   { if( statement_new_strexp( &($$), $1, $2 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
	 | PRINT_TOKEN printlist
	   { if( statement_new_print( &($$), $2 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
	 | IF_TOKEN numexp THEN_TOKEN command
	   { if( statement_new_if( &($$), $2, $4 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
	 | FOR_TOKEN FORVAR '=' numexp TO_TOKEN numexp optstepexp
	   { if( statement_new_for( &($$), $2, $4, $6, $7 ) !=
		 BASIC_ERROR_NONE ) {
	       free( $2 );
	       YYERROR;
	     }
	     free( $2 );
	   }
	 | NEXT_TOKEN FORVAR
	   { if( statement_new_next( &($$), $2 ) != BASIC_ERROR_NONE ) {
	       free( $2 );
	       YYERROR;
	     }
	     free( $2 );
	   }
	 | DEFFN_TOKEN FORVAR '(' optsvarlist ')' '=' numexp
	   { if( statement_new_deffn( &($$), $2, $4, $7 ) !=
		 BASIC_ERROR_NONE ) {
	       free( $2 );
	       YYERROR;
	     }
	     free( $2 );
	   }
         | DIM_TOKEN FORVAR '(' numexplist ')'
           { if( statement_new_dim_numeric( &($$), $2, $4 ) !=
		 BASIC_ERROR_NONE ) {
	       free( $2 );
	       YYERROR;
	     }
	     free( $2 );
	   }
	 | INPUT_TOKEN printlist
           {
	     if( statement_new_input( &($$), $2 ) ) YYERROR;
	   }
;

optnumexp:   /* empty */ { $$ = NULL; }
	   | numexp      { $$ = $1;   }
;

numexp:   number { $$ = malloc( sizeof( *$$ ) );
                   $$->type = NUMEXP_NUMBER_ID; 
		   $$->types.number = $1;
                 }
	| numericvar
	  { if( numexp_new_variable( &($$), $1 ) != BASIC_ERROR_NONE ) {
	      YYERROR;
	      free( $1 );
	    }
	    free( $1 );
	  }
	| FORVAR '(' numexplist ')'
          { if( numexp_new_array( &($$), $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
	| NUMEXP_NO_ARG { if( numexp_new_noarg( &($$), $1 ) !=
			      BASIC_ERROR_NONE )
	                  YYERROR;
	                }
	| NUMEXP_ONE_ARG numexp
          { if( numexp_new_onearg( &($$), $1, $2 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | NOT_TOKEN numexp		/* Different precedence */
          { if( numexp_new_onearg( &($$), NOT, $2 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | '(' numexp ')' { $$ = $2; }
        | '+' numexp %prec NEG { $$ = $2; }
        | '-' numexp %prec NEG
          { if( numexp_new_onearg( &($$), '-', $2 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | numexp '+' numexp
          { if( numexp_new_twoarg( &($$), '+', $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | numexp '-' numexp
          { if( numexp_new_twoarg( &($$), '-', $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | numexp TIMES_DIVIDE numexp
          { if( numexp_new_twoarg( &($$), $2, $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | numexp '^' numexp
          { if( numexp_new_twoarg( &($$), '^', $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | numexp COMPARISON numexp
          { if( numexp_new_twoarg( &($$), $2, $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
	| numexp '=' numexp
          { if( numexp_new_twoarg( &($$), '=', $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | numexp OR_TOKEN numexp
          { if( numexp_new_twoarg( &($$), OR, $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | numexp AND_TOKEN numexp
          { if( numexp_new_twoarg( &($$), AND, $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | NUMEXP_TWO_ARG '(' numexp ',' numexp ')'
          { if( numexp_new_twoarg( &($$), $1, $3, $5 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | NUMEXP_STREXP strexp
	  { if( numexp_new_strexp( &($$), $1, $2 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
	| FN_TOKEN FORVAR '(' optexplist ')'
	  { if( numexp_new_fn( &($$), $2, $4 ) != BASIC_ERROR_NONE ) {
	      free( $2 );
	      YYERROR;
	    }
	    free( $2 );
	  }
;

number:   NUM      { $$ = $1; }
        | LINENUM  { $$ = $1; }
;

numericvar:   FORVAR { $$ = $1; }
	    | NUMVAR { $$ = $1; }
;

strexp:   STRING
          { if( strexp_new_string( &($$), $1 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | STRVAR
	  { if( strexp_new_variable( &($$), $1 ) != BASIC_ERROR_NONE ) {
	      free( $1 );
	      YYERROR;
	    }
	    free( $1 );
	  }
	| STREXP_NUMEXP numexp
	  { if( strexp_new_numexp( &($$), $1, $2 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | STREXP_STREXP strexp
	  { if( strexp_new_strexp( &($$), $1, $2 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
	| strexp '+' strexp
	  { if( strexp_new_twostrexp( &($$), '+', $1, $3 ) !=
		BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | '(' strexp ')' { $$ = $2; }
	| strexp slicer
	  { if( strexp_new_slicer( &($$), $1, $2 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
        | strexp AND_TOKEN numexp
	  { if( strexp_new_and( &($$), $1, $3 ) != BASIC_ERROR_NONE )
	      YYERROR;
	  }
;

optslicer:   /* empty */ { $$ = NULL; }
	   | slicer	 { $$ = $1;   }
;

slicer:   '(' optnumexp TO_TOKEN optnumexp ')'
	  { if( slicer_new( &($$), $2, $4 ) != BASIC_ERROR_NONE ) YYERROR; }
;

optexplist:   /* empty */
	      { if( explist_new( &($$) ) != BASIC_ERROR_NONE ) YYERROR; }
	    | explist     { $$ = $1;   }
;

explist:   numexp
	   { if( explist_new( &($$) ) != BASIC_ERROR_NONE ) YYERROR;
	     if( explist_append_numexp( $$, $1 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
	 | strexp
	   { if( explist_new( &($$) ) != BASIC_ERROR_NONE ) YYERROR;
	     if( explist_append_strexp( $$, $1 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
	 | explist ',' numexp
           { $$ = $1; 
	     if( explist_append_numexp( $$, $3 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
	 | explist ',' strexp
	   { $$ = $1;
	     if( explist_append_strexp( $$, $3 ) != BASIC_ERROR_NONE )
	       YYERROR;
	   }
;

numexplist:   numexp
	      { if( explist_new( &($$) ) != BASIC_ERROR_NONE ) YYERROR;
	        if( explist_append_numexp( $$, $1 ) != BASIC_ERROR_NONE )
	          YYERROR;
	      }
	    | numexplist ',' numexp
              { $$ = $1; 
	        if( explist_append_numexp( $$, $3 ) != BASIC_ERROR_NONE )
	          YYERROR;
	      }
;

printlist:   printlist1 { $$ = $1; }
           | printlist2 { $$ = $1; }
;

printlist1:   /* empty */
	      { if( printlist_new( &($$) ) != BASIC_ERROR_NONE ) YYERROR; }
	    | printlist printsep
	      {
		struct printitem *separator;

	        $$ = $1;

		if( printitem_new_separator( &separator, $2 ) !=
		    BASIC_ERROR_NONE )
		  YYERROR;
		if( printlist_append( $$, separator ) != BASIC_ERROR_NONE )
		  YYERROR;
	      }
;

printlist2:   printlist1 printitem
	      { if( printlist_append( $$, $2 ) != BASIC_ERROR_NONE ) YYERROR; }
;

printitem:   numexp
	     { if( printitem_new_numexp( &($$), $1 ) != BASIC_ERROR_NONE )
	         YYERROR;
	     }
	   | strexp
	     { if( printitem_new_strexp( &($$), $1 ) != BASIC_ERROR_NONE )
	         YYERROR;
	     }
;

printsep:   ';'  { $$ = ';';  }
	  | ','  { $$ = ',';  }
	  | '\'' { $$ = '\''; }
;

optstepexp:   /* empty */       { $$ = NULL; }
	    | STEP_TOKEN numexp { $$ = $2; }
;

optsvarlist:   /* empty */
	       { if( explist_new( &($$) ) != BASIC_ERROR_NONE ) YYERROR; }
	     | FORVAR
	       { if( explist_new( &($$) ) != BASIC_ERROR_NONE ) YYERROR;
	         if( explist_append_forvar( $$, $1 ) != BASIC_ERROR_NONE ) {
		   free( $1 );
		   YYERROR;
		 }
		 free( $1 );
	       }
	     | STRVAR
	       { if( explist_new( &($$) ) != BASIC_ERROR_NONE ) YYERROR;
	         if( explist_append_strvar( $$, $1 ) != BASIC_ERROR_NONE ) {
		   free( $1 );
		   YYERROR;
		 }
		 free( $1 );
	       }
	     | optsvarlist ',' FORVAR
	       { $$ = $1;
	         if( explist_append_forvar( $$, $3 ) != BASIC_ERROR_NONE ) {
		   free( $3 );
		   YYERROR;
		 }
		 free( $3 );
	       }
	     | optsvarlist ',' STRVAR
	       { $$ = $1;
	         if( explist_append_strvar( $$, $3 ) != BASIC_ERROR_NONE ) {
		   free( $3 );
		   YYERROR;
		 }
		 free( $3 );
	       }
;

%%
