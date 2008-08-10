/* token.h: Spectrum BASIC tokens
   Copyright (C) 2002 Philip Kendall

   $Id: token.h,v 1.4 2002-07-25 11:08:06 pak21 Exp $

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

#ifndef BASIC_TOKEN_H
#define BASIC_TOKEN_H

enum basic_token {

  RND = 165,
  INKEYS,		/* INKEY$ */
  PI,
  FN,
  POINT,
  SCREENS,		/* SCREEN$ */
  ATTR,
  AT,
  TAB,
  VALS,			/* VAL$ */
  CODE,
  VAL,
  LEN,
  SIN,
  COS,
  TAN,
  ASN,
  ACS,
  ATN,
  LN,
  EXP,
  INT,
  SQR,
  SGN,
  SPECTRUM_ABS,		/* glib defines ABS */
  PEEK,
  IN,
  USR,
  STRS,			/* STR$ */
  CHRS,			/* CHR$ */
  NOT,
  BIN,
  OR,
  AND,
  LE,			/* <= */
  GE,			/* >= */
  NE,			/* <> */
  LINE,
  THEN,
  TO,
  STEP,
  DEFFN,		/* DEF FN */
  CAT,
  FORMAT,
  MOVE,
  ERASE,
  OPEN,			/* OPEN # */
  CLOSE,		/* CLOSE # */
  MERGE,
  VERIFY,
  BEEP,
  CIRCLE,
  INK,
  PAPER,
  FLASH,
  BRIGHT,
  INVERSE,
  OVER,
  OUT,
  LPRINT,
  LLIST,
  STOP,
  READ,
  DATA,
  RESTORE,
  NEW,
  BORDER,
  CONTINUE,
  DIM,
  REM,
  FOR,
  GOTO,			/* GO TO */
  GOSUB,		/* GO SUB */
  INPUT,
  LOAD,
  LIST,
  LET,
  PAUSE,
  NEXT,
  POKE,
  PRINT,
  PLOT,
  RUN,
  SAVE,
  RANDOMIZE,
  IF,
  CLS,
  DRAW,
  CLEAR,
  RETURN,
  COPY,

};

#endif				/* #ifndef BASIC_TOKEN_H */
