/* breakpoint.h: a debugger breakpoint
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

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_DEBUGGER_BREAKPOINT_H
#define FUSE_DEBUGGER_BREAKPOINT_H

/* Types of breakpoint */
typedef enum debugger_breakpoint_type {
  DEBUGGER_BREAKPOINT_TYPE_EXECUTE,
  DEBUGGER_BREAKPOINT_TYPE_READ,
  DEBUGGER_BREAKPOINT_TYPE_WRITE,
  DEBUGGER_BREAKPOINT_TYPE_PORT_READ,
  DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE,
  DEBUGGER_BREAKPOINT_TYPE_TIME,
} debugger_breakpoint_type;

extern const char *debugger_breakpoint_type_text[];

/* Lifetime of a breakpoint */
typedef enum debugger_breakpoint_life {
  DEBUGGER_BREAKPOINT_LIFE_PERMANENT,
  DEBUGGER_BREAKPOINT_LIFE_ONESHOT,
} debugger_breakpoint_life;

extern const char *debugger_breakpoint_life_text[];

typedef struct debugger_breakpoint_address {

  int page;
  libspectrum_word offset;

} debugger_breakpoint_address;

typedef union debugger_breakpoint_value {

  debugger_breakpoint_address address;
  libspectrum_word port;
  libspectrum_dword tstates;

} debugger_breakpoint_value;

typedef struct debugger_expression debugger_expression;

/* The breakpoint structure */
typedef struct debugger_breakpoint {
  size_t id;

  debugger_breakpoint_type type;
  debugger_breakpoint_value value;

  size_t ignore;		/* Ignore this breakpoint this many times */
  debugger_breakpoint_life life;
  debugger_expression *condition; /* Conditional expression to activate this
				     breakpoint */
} debugger_breakpoint;

/* The current breakpoints */
extern GSList *debugger_breakpoints;

int debugger_check( debugger_breakpoint_type type, libspectrum_dword value );

/* Add a new breakpoint */
int
debugger_breakpoint_add_address(
  debugger_breakpoint_type type, int page, libspectrum_word offset,
  size_t ignore, debugger_breakpoint_life life, debugger_expression *condition
);

int
debugger_breakpoint_add_port(
  debugger_breakpoint_type type, libspectrum_word port,
  size_t ignore, debugger_breakpoint_life life, debugger_expression *condition
);

int
debugger_breakpoint_add_time(
  debugger_breakpoint_type type, libspectrum_dword tstates,
  size_t ignore, debugger_breakpoint_life life, debugger_expression *condition
);

/* Add events corresponding to all the time breakpoints to happen
   during this frame */
int debugger_add_time_events( void );

#endif				/* #ifndef FUSE_DEBUGGER_BREAKPOINT_H */
