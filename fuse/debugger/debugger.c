/* debugger.c: Fuse's monitor/debugger
   Copyright (c) 2002-2008 Philip Kendall

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

#include <config.h>

#include "debugger.h"
#include "debugger_internals.h"
#include "event.h"
#include "fuse.h"
#include "memory.h"
#include "mempool.h"
#include "periph.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The current activity state of the debugger */
enum debugger_mode_t debugger_mode;

/* Which base should we display things in */
int debugger_output_base;

/* Memory pool used by the lexer and parser */
int debugger_memory_pool;

int
debugger_init( void )
{
  int error;

  debugger_breakpoints = NULL;
  debugger_output_base = 16;

  debugger_memory_pool = mempool_register_pool();
  if( debugger_memory_pool == -1 ) return 1;

  error = debugger_event_init();
  if( error ) return error;

  error = debugger_variable_init();
  if( error ) return error;

  error = debugger_reset();
  if( error ) return error;

  return 0;
}

int
debugger_reset( void )
{
  debugger_breakpoint_remove_all();
  debugger_mode = DEBUGGER_MODE_INACTIVE;

  return 0;
}

int
debugger_end( void )
{
  debugger_breakpoint_remove_all();
  return 0;
}

/* Activate the debugger */
int
debugger_trap( void )
{
  /* A breakpoint command could have deactivated the debugger */
  if( debugger_mode != DEBUGGER_MODE_HALTED ) return 0;

  return ui_debugger_activate();
}

/* Step one instruction */
int
debugger_step( void )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  ui_debugger_deactivate( 0 );
  return 0;
}

/* Step to the next instruction, ignoring CALLs etc */
int
debugger_next( void )
{
  size_t length;

  /* Find out how long the current instruction is */
  debugger_disassemble( NULL, 0, &length, PC );

  /* And add a breakpoint after that */
  debugger_breakpoint_add_address( DEBUGGER_BREAKPOINT_TYPE_EXECUTE, -1,
				   PC + length, 0,
				   DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL );

  debugger_run();

  return 0;
}

/* Set debugger_mode so that emulation will occur */
int
debugger_run( void )
{
  debugger_mode = debugger_breakpoints ?
                  DEBUGGER_MODE_ACTIVE :
                  DEBUGGER_MODE_INACTIVE;
  ui_debugger_deactivate( 1 );
  return 0;
}

/* Exit from the last CALL etc by setting a oneshot breakpoint at
   (SP) and then starting emulation */
int
debugger_breakpoint_exit( void )
{
  libspectrum_word target;

  target = readbyte_internal( SP ) + 0x100 * readbyte_internal( SP+1 );

  if( debugger_breakpoint_add_address(
        DEBUGGER_BREAKPOINT_TYPE_EXECUTE, -1, target, 0,
	DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL
      )
    )
    return 1;

  if( debugger_run() ) return 1;

  return 0;
}

/* Poke a value into RAM */
int
debugger_poke( libspectrum_word address, libspectrum_byte value )
{
  writebyte_internal( address, value );
  return 0;
}

/* Write a value to a port */
int
debugger_port_write( libspectrum_word port, libspectrum_byte value )
{
  writeport_internal( port, value );
  return 0;
}

/* Exit the emulator */
void
debugger_exit_emulator( void )
{
  fuse_exiting = 1;
  debugger_run();
}
