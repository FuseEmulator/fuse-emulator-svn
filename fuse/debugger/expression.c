/* expression.c: A numeric expression
   Copyright (c) 2003 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "debugger_internals.h"
#include "fuse.h"
#include "ui/ui.h"

typedef enum expression_type {

  DEBUGGER_EXPRESSION_TYPE_INTEGER,
  DEBUGGER_EXPRESSION_TYPE_REGISTER,
  DEBUGGER_EXPRESSION_TYPE_UNARYOP,
  DEBUGGER_EXPRESSION_TYPE_BINARYOP,

} expression_type;

enum precedence_t {

  /* Lowest precedence */
  PRECEDENCE_LOGICAL_OR,
  PRECEDENCE_LOGICAL_AND,
  PRECEDENCE_BITWISE_OR,
  PRECEDENCE_BITWISE_XOR,
  PRECEDENCE_BITWISE_AND,
  PRECEDENCE_EQUALITY,
  PRECEDENCE_COMPARISION,
  PRECEDENCE_ADDITION,
  PRECEDENCE_MULTIPLICATION,
  PRECEDENCE_NEGATE,

  PRECEDENCE_ATOMIC,
  /* Highest precedence */

};

struct unaryop_type {

  int operation;
  debugger_expression *op;

};

struct binaryop_type {

  int operation;
  debugger_expression *op1, *op2;

};

struct debugger_expression {

  expression_type type;
  enum precedence_t precedence;

  union {
    int integer;
    int reg;
    struct unaryop_type unaryop;
    struct binaryop_type binaryop;
  } types;

};

static libspectrum_dword evaluate_unaryop( struct unaryop_type *unaryop );
static libspectrum_dword evaluate_binaryop( struct binaryop_type *binary );

static int deparse_unaryop( char *buffer, size_t length,
			    const struct unaryop_type *unaryop );
static int deparse_binaryop( char *buffer, size_t length,
			     const struct binaryop_type *binaryop );
static int
brackets_necessary( int top_operation, debugger_expression *operand );
static int is_non_associative( int operation );

static enum precedence_t
unaryop_precedence( int operation )
{
  switch( operation ) {

  case '!': case '~': case '-': return PRECEDENCE_NEGATE;

  default:
    ui_error( UI_ERROR_ERROR, "unknown unary operator %d", operation );
    fuse_abort();
  }
}

static enum precedence_t
binaryop_precedence( int operation )
{
  switch( operation ) {

  case 0x2228:		    return PRECEDENCE_LOGICAL_OR;
  case 0x2227:		    return PRECEDENCE_LOGICAL_AND;
  case    '|':		    return PRECEDENCE_BITWISE_OR;
  case    '^':		    return PRECEDENCE_BITWISE_XOR;
  case    '&':		    return PRECEDENCE_BITWISE_AND;
  case    '+': case    '-': return PRECEDENCE_ADDITION;
  case    '*': case    '/': return PRECEDENCE_MULTIPLICATION;
  case 0x225f: case 0x2260: return PRECEDENCE_EQUALITY;
  case    '<': case    '>':
  case 0x2264: case 0x2265: return PRECEDENCE_COMPARISION;

  default:
    ui_error( UI_ERROR_ERROR, "unknown binary operator %d", operation );
    fuse_abort();
  }
}

debugger_expression*
debugger_expression_new_number( libspectrum_dword number )
{
  debugger_expression *exp;

  exp = malloc( sizeof( debugger_expression ) );
  if( !exp ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  exp->type = DEBUGGER_EXPRESSION_TYPE_INTEGER;
  exp->precedence = PRECEDENCE_ATOMIC;
  exp->types.integer = number;

  return exp;
}

debugger_expression*
debugger_expression_new_register( int which )
{
  debugger_expression *exp;

  exp = malloc( sizeof( debugger_expression ) );
  if( !exp ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  exp->type = DEBUGGER_EXPRESSION_TYPE_REGISTER;
  exp->precedence = PRECEDENCE_ATOMIC;
  exp->types.reg = which;

  return exp;
}

debugger_expression*
debugger_expression_new_binaryop( int operation, debugger_expression *operand1,
				  debugger_expression *operand2 )
{
  debugger_expression *exp;

  exp = malloc( sizeof( debugger_expression ) );
  if( !exp ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  exp->type = DEBUGGER_EXPRESSION_TYPE_BINARYOP;
  exp->precedence = binaryop_precedence( operation );

  exp->types.binaryop.operation = operation;
  exp->types.binaryop.op1 = operand1;
  exp->types.binaryop.op2 = operand2;

  return exp;
}


debugger_expression*
debugger_expression_new_unaryop( int operation, debugger_expression *operand )
{
  debugger_expression *exp;

  exp = malloc( sizeof( debugger_expression ) );
  if( !exp ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  exp->type = DEBUGGER_EXPRESSION_TYPE_UNARYOP;
  exp->precedence = unaryop_precedence( operation );

  exp->types.unaryop.operation = operation;
  exp->types.unaryop.op = operand;

  return exp;
}


void
debugger_expression_delete( debugger_expression *exp )
{
  switch( exp->type ) {
    
  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
  case DEBUGGER_EXPRESSION_TYPE_REGISTER:
    break;

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    debugger_expression_delete( exp->types.unaryop.op );
    break;

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    debugger_expression_delete( exp->types.binaryop.op1 );
    debugger_expression_delete( exp->types.binaryop.op2 );
    break;

  }
    
  free( exp );
}

libspectrum_dword
debugger_expression_evaluate( debugger_expression *exp )
{
  switch( exp->type ) {

  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
    return exp->types.integer;

  case DEBUGGER_EXPRESSION_TYPE_REGISTER:
    return debugger_register_get( exp->types.reg );

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    return evaluate_unaryop( &( exp->types.unaryop ) );

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    return evaluate_binaryop( &( exp->types.binaryop ) );

  }

  ui_error( UI_ERROR_ERROR, "unknown expression type %d", exp->type );
  fuse_abort();
}

static libspectrum_dword
evaluate_unaryop( struct unaryop_type *unary )
{
  switch( unary->operation ) {

  case '!': return !debugger_expression_evaluate( unary->op );
  case '~': return ~debugger_expression_evaluate( unary->op );
  case '-': return -debugger_expression_evaluate( unary->op );

  }

  ui_error( UI_ERROR_ERROR, "unknown unary operator %d", unary->operation );
  fuse_abort();
}

static libspectrum_dword
evaluate_binaryop( struct binaryop_type *binary )
{
  switch( binary->operation ) {

  case '+': return debugger_expression_evaluate( binary->op1 ) +
		   debugger_expression_evaluate( binary->op2 );

  case '-': return debugger_expression_evaluate( binary->op1 ) -
		   debugger_expression_evaluate( binary->op2 );

  case '*': return debugger_expression_evaluate( binary->op1 ) *
		   debugger_expression_evaluate( binary->op2 );

  case '/': return debugger_expression_evaluate( binary->op1 ) /
		   debugger_expression_evaluate( binary->op2 );

  case 0x225f: return debugger_expression_evaluate( binary->op1 ) ==
		      debugger_expression_evaluate( binary->op2 );

  case 0x2260: return debugger_expression_evaluate( binary->op1 ) !=
		      debugger_expression_evaluate( binary->op2 );

  case '>': return debugger_expression_evaluate( binary->op1 ) >
		   debugger_expression_evaluate( binary->op2 );

  case '<': return debugger_expression_evaluate( binary->op1 ) <
	           debugger_expression_evaluate( binary->op2 );

  case 0x2264: return debugger_expression_evaluate( binary->op1 ) <=
		      debugger_expression_evaluate( binary->op2 );

  case 0x2265: return debugger_expression_evaluate( binary->op1 ) >=
		      debugger_expression_evaluate( binary->op2 );

  case '&': return debugger_expression_evaluate( binary->op1 ) &
	           debugger_expression_evaluate( binary->op2 );

  case '^': return debugger_expression_evaluate( binary->op1 ) ^
		   debugger_expression_evaluate( binary->op2 );

  case '|': return debugger_expression_evaluate( binary->op1 ) |
		   debugger_expression_evaluate( binary->op2 );

  case 0x2227: return debugger_expression_evaluate( binary->op1 ) &&
		   debugger_expression_evaluate( binary->op2 );

  case 0x2228: return debugger_expression_evaluate( binary->op1 ) ||
		   debugger_expression_evaluate( binary->op2 );

  }

  ui_error( UI_ERROR_ERROR, "unknown binary operator %d", binary->operation );
  fuse_abort();
}

int
debugger_expression_deparse( char *buffer, size_t length,
			     const debugger_expression *exp )
{
  switch( exp->type ) {

  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
    if( debugger_output_base == 10 ) {
      snprintf( buffer, length, "%d", exp->types.integer );
    } else {
      snprintf( buffer, length, "0x%x", exp->types.integer );
    }
    return 0;

  case DEBUGGER_EXPRESSION_TYPE_REGISTER:
    snprintf( buffer, length, "%s", debugger_register_text( exp->types.reg ) );
    return 0;

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    return deparse_unaryop( buffer, length, &( exp->types.unaryop ) );

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    return deparse_binaryop( buffer, length, &( exp->types.binaryop ) );

  }

  ui_error( UI_ERROR_ERROR, "unknown expression type %d", exp->type );
  fuse_abort();
}
  
static int
deparse_unaryop( char *buffer, size_t length,
		 const struct unaryop_type *unaryop )
{
  char *operand_buffer; const char *operation_string = NULL;
  int brackets;

  int error;

  operand_buffer = malloc( length );
  if( !operand_buffer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  error = debugger_expression_deparse( operand_buffer, length, unaryop->op );
  if( error ) { free( operand_buffer ); return error; }

  switch( unaryop->operation ) {
  case '!': operation_string = "!"; break;
  case '~': operation_string = "~"; break;
  case '-': operation_string = "-"; break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown unary operation %d",
	      unaryop->operation );
    fuse_abort();
  }

  brackets = ( unaryop->op->precedence                  < 
	       unaryop_precedence( unaryop->operation )   );
    
  snprintf( buffer, length, "%s%s%s%s", operation_string,
	    brackets ? "( " : "", operand_buffer,
	    brackets ? " )" : "" );

  free( operand_buffer );

  return 0;
}

static int
deparse_binaryop( char *buffer, size_t length,
		  const struct binaryop_type *binaryop )
{
  char *operand1_buffer, *operand2_buffer; const char *operation_string = NULL;
  int brackets_necessary1, brackets_necessary2;

  int error;

  operand1_buffer = malloc( 2 * length );
  if( !operand1_buffer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }
  operand2_buffer = &operand1_buffer[ length ];

  error = debugger_expression_deparse( operand1_buffer, length,
				       binaryop->op1 );
  if( error ) { free( operand1_buffer ); return error; }

  error = debugger_expression_deparse( operand2_buffer, length,
				       binaryop->op2 );
  if( error ) { free( operand1_buffer ); return error; }

  switch( binaryop->operation ) {
  case    '+': operation_string = "+";  break;
  case    '-': operation_string = "-";  break;
  case    '*': operation_string = "*";  break;
  case    '/': operation_string = "/";  break;
  case 0x225f: operation_string = "=="; break;
  case 0x2260: operation_string = "!="; break;
  case    '<': operation_string = "<";  break;
  case    '>': operation_string = ">";  break;
  case 0x2264: operation_string = "<="; break;
  case 0x2265: operation_string = ">="; break;
  case    '&': operation_string = "&";  break;
  case    '^': operation_string = "^";  break;
  case    '|': operation_string = "|";  break;
  case 0x2227: operation_string = "&&"; break;
  case 0x2228: operation_string = "||"; break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown binary operation %d",
	      binaryop->operation );
    fuse_abort();
  }

  brackets_necessary1 = brackets_necessary( binaryop->operation,
					    binaryop->op1 );
  brackets_necessary2 = brackets_necessary( binaryop->operation,
					    binaryop->op2 );

  snprintf( buffer, length, "%s%s%s %s %s%s%s",
	    brackets_necessary1 ? "( " : "", operand1_buffer,
	    brackets_necessary1 ? " )" : "",
	    operation_string,
	    brackets_necessary2 ? "( " : "", operand2_buffer,
	    brackets_necessary2 ? " )" : "" );

  free( operand1_buffer );

  return 0;
}

/* When deparsing, do we need to put brackets around `operand' when
   being used as an operand of the binary operation `top_operation'? */
static int
brackets_necessary( int top_operation, debugger_expression *operand )
{
  enum precedence_t top_precedence, bottom_precedence;
  
  top_precedence = binaryop_precedence( top_operation );
  bottom_precedence = operand->precedence;

  /* If the top level operation has a higher precedence than the
     bottom level operation, we always need brackets */
  if( top_precedence > bottom_precedence ) return 1;

  /* If the two operations are of equal precedence, we need brackets
     i) if the top level operation is non-associative, or
     ii) if the operand is a non-associative operation

     Note the assumption here that all things with a precedence equal to
     a binary operator are also binary operators

     Strictly, we don't need brackets in either of these cases, but
     otherwise the user is going to have to remember operator
     left-right associativity; I think things are clearer with
     brackets in.
  */
  if( top_precedence == bottom_precedence ) {

    if( is_non_associative( top_operation ) ) return 1;

    /* Sanity check */
    if( operand->type != DEBUGGER_EXPRESSION_TYPE_BINARYOP ) {
      ui_error( UI_ERROR_ERROR,
		"binary operator has same precedence as non-binary operator" );
      fuse_abort();
    }

    return is_non_associative( operand->types.binaryop.operation );
  }

  /* Otherwise (ie if the top level operation is of lower precedence
     than the bottom, or both operators have equal precedence and
     everything is associative) we don't need brackets */
  return 0;
}

/* Is a binary operator non-associative? */
static int
is_non_associative( int operation )
{
  switch( operation ) {

  /* Simple cases */
  case '+': case '*': return 0;
  case '-': case '/': return 1;

  /* None of the comparision operators are associative due to them
     returning truth values */
  case 0x225f: case 0x2260: case '<': case '>': case 0x2264: case 0x2265:
    return 1;

  /* The logical operators are associative */
  case 0x2227: return 0;
  case 0x2228: return 0;

  /* The bitwise operators are also associative (consider them as
     vectorised logical operators) */
  case    '&': return 0;
  case    '^': return 0;
  case    '|': return 0;

  }

  /* Should never get here */
  ui_error( UI_ERROR_ERROR, "unknown binary operation %d", operation );
  fuse_abort();
}

