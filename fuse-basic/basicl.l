/* basicl.l: lexical scanner for the Spectrum BASIC interpreter
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

%{

#include "config.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "spectrum-string.h"
#include "token.h"

#include "basicy.h"

#define YY_INPUT(buf,result,max_size) \
{ \
  if( !parse_fill_buffer( buf, &result, max_size ) ) result = YY_NULL; \
}

%}

%option noyywrap

DIGIT	[0-9]

%%

\n		{ if( 0 ) unput( '\0' ); /* Prevent warning about yyunput
					    being defined but not used */
		  return '\n'; }

":"		{ return ':'; }

";"		{ return ';'; }
","		{ return ','; }
"'"		{ return '\''; }

"("		{ return '('; }
")"		{ return ')'; }
"="		{ return '='; }

"+"		{ return '+'; }
"-"		{ return '-'; }
"*"		{ yylval.token = '*'; return TIMES_DIVIDE; }
"/"		{ yylval.token = '/'; return TIMES_DIVIDE; }
"^"		{ return '^'; }

"<"		{ yylval.token = '<'; return COMPARISON; }
">"		{ yylval.token = '>'; return COMPARISON; }
"<>"		{ yylval.token = NE; return COMPARISON; }
"<="		{ yylval.token = LE; return COMPARISON; }
">="		{ yylval.token = GE; return COMPARISON; }

RND		{ yylval.token = RND; return NUMEXP_NO_ARG; }
PI		{ yylval.token = PI; return NUMEXP_NO_ARG; }
FN		{ return FN_TOKEN; }
"VAL$"		{ yylval.token = VALS; return STREXP_STREXP; }
CODE		{ yylval.token = CODE; return NUMEXP_STREXP; }
VAL		{ yylval.token = VAL; return NUMEXP_STREXP; }
LEN		{ yylval.token = LEN; return NUMEXP_STREXP; }
SIN		{ yylval.token = SIN; return NUMEXP_ONE_ARG; }
COS		{ yylval.token = COS; return NUMEXP_ONE_ARG; }
TAN		{ yylval.token = TAN; return NUMEXP_ONE_ARG; }
ASN		{ yylval.token = ASN; return NUMEXP_ONE_ARG; }
ACS		{ yylval.token = ACS; return NUMEXP_ONE_ARG; }
ATN		{ yylval.token = ATN; return NUMEXP_ONE_ARG; }
LN		{ yylval.token = LN; return NUMEXP_ONE_ARG; }
EXP		{ yylval.token = EXP; return NUMEXP_ONE_ARG; }
INT		{ yylval.token = INT; return NUMEXP_ONE_ARG; }
SQR		{ yylval.token = SQR; return NUMEXP_ONE_ARG; }
SGN		{ yylval.token = SGN; return NUMEXP_ONE_ARG; }
ABS		{ yylval.token = SPECTRUM_ABS; return NUMEXP_ONE_ARG; }
"STR$"		{ yylval.token = STRS; return STREXP_NUMEXP; }
"CHR$"		{ yylval.token = CHRS; return STREXP_NUMEXP; }
NOT		{ return NOT_TOKEN; }
OR		{ return OR_TOKEN; }
AND		{ return AND_TOKEN; }
TO		{ return TO_TOKEN; }

{DIGIT}+	{ yylval.integer = atoi( yytext ); return LINENUM; }
{DIGIT}+"."{DIGIT}* { yylval.real = atof( yytext ); return NUM; }

\"([^"]|\"\")*\" {
                   string_new( &yylval.string );
		   string_create( yylval.string, &yytext[1],
				  strlen( yytext ) - 2 );
		   return STRING;
		 }

[ ]+		/* Swallow whitespace */

THEN		{ return THEN_TOKEN; }
STEP		{ return STEP_TOKEN; }
"DEF FN"	{ return DEFFN_TOKEN; }
MERGE		{ yylval.token = MERGE; return STREXP_COMMAND; }
BEEP		{ yylval.token = BEEP; return TWONUMEXP_COMMAND; }
STOP		{ yylval.token = STOP; return NOARG_COMMAND; }
RESTORE		{ yylval.token = RESTORE; return OPTNUMEXP_COMMAND; }
DIM		{ return DIM_TOKEN; }
FOR		{ return FOR_TOKEN; }
GO" "?TO	{ yylval.token = GOTO; return NUMEXP_COMMAND; }
GO" "?SUB	{ yylval.token = GOSUB; return NUMEXP_COMMAND; }
INPUT		{ return INPUT_TOKEN; }
LET		{ return LET_TOKEN; }
PAUSE		{ yylval.token = PAUSE; return NUMEXP_COMMAND; }
NEXT		{ return NEXT_TOKEN; }
PRINT		{ return PRINT_TOKEN; }
RANDOMIZE	{ yylval.token = RANDOMIZE; return OPTNUMEXP_COMMAND; }
IF		{ return IF_TOKEN; }
CLEAR		{ yylval.token = CLEAR; return OPTNUMEXP_COMMAND; }
RETURN		{ yylval.token = RETURN; return NOARG_COMMAND; }

[[:alpha:]]\$	{ yylval.varname = strdup( yytext ); yylval.varname[1] = '\0';
	          return STRVAR;
		}
[[:alpha:]][[:alnum:]]+ { yylval.varname = strdup( yytext ); return NUMVAR; }
[[:alpha:]]	{ yylval.varname = strdup( yytext ); return FORVAR; }
