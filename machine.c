/* machine.c: Routines for handling the various machine types
   Copyright (c) 1999-2002 Philip Kendall

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

#include <config.h>

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "event.h"
#include "fuse.h"
#include "machine.h"
#include "spec48.h"
#include "spec128.h"
#include "specplus2.h"
#include "specplus3.h"
#include "tape.h"
#include "ui/ui.h"
#include "utils.h"
#include "z80/z80.h"

#define ERROR_MESSAGE_MAX_LENGTH 1024
#define PATHNAME_MAX_LENGTH 1024

machine_info **machine_types = NULL; /* Array of available machines */
int machine_count = 0;

machine_info *machine_current;  /* The currently selected machine */
static int machine_location;	/* Where is the current machine in
				   machine_types[...]? */

static int machine_add_machine( int (*init_function)(machine_info *machine) );
static int machine_select_machine( machine_info *machine );
static int machine_free_machine( machine_info *machine );

int machine_init_machines( void )
{
  int error;

  error = machine_add_machine( spec48_init    );
  if (error ) return error;
  error = machine_add_machine( spec128_init   );
  if (error ) return error;
  error = machine_add_machine( specplus2_init );
  if (error ) return error;
  error = machine_add_machine( specplus3_init );
  if (error ) return error;

  return 0;
}

static int machine_add_machine( int (*init_function)( machine_info *machine ) )
{
  int error;

  machine_count++;

  machine_types = 
    (machine_info**)realloc( machine_types, 
			     machine_count * sizeof( machine_info* ) );
  if( machine_types == NULL ) {
    ui_error( "out of memory at %s:%d\n", __FILE__, __LINE__ );
    return 1;
  }

  machine_types[ machine_count - 1 ] =
    (machine_info*)malloc( sizeof( machine_info ) );
  if( machine_types[ machine_count - 1 ] == NULL ) {
    ui_error( "out of memory at %s:%d\n", __FILE__, __LINE__ );
    return 1;
  }

  error = init_function( machine_types[ machine_count - 1 ] );
  if( error ) return error;

  return 0;
}

int machine_select( int type )
{
  int i;
  int error;

  for( i=0; i < machine_count; i++ ) {
    if( machine_types[i]->machine == type ) {
      machine_location = i;
      error = machine_select_machine( machine_types[i] );
      if( error ) return error;
      return 0;
    }
  }

  ui_error( "Machine type `%d' unavailable\n", type );
  return 1;
}

int machine_select_id( const char *id )
{
  int i, error;

  for( i=0; i < machine_count; i++ ) {
    if( ! strcmp( machine_types[i]->id, id ) ) {
      error = machine_select_machine( machine_types[i] );
      if( error ) return error;
      return 0;
    }
  }

  ui_error( "Machine id `%s' unknown\n", id );
  return 1;
}

static int machine_select_machine( machine_info *machine )
{
  machine_current = machine;

  tstates = 0;

  /* Reset the event stack */
  event_reset();
  if( event_add( machine->timings.cycles_per_frame, EVENT_TYPE_INTERRUPT) )
    return 1;
  if( event_add( machine->line_times[0], EVENT_TYPE_LINE) ) return 1;

  /* Stop the tape playing */
  tape_stop();

  readbyte = machine->ram.read_memory;
  read_screen_memory = machine->ram.read_screen;
  writebyte = machine->ram.write_memory;
  contend_memory = machine->ram.contend_memory;
  contend_port = machine->ram.contend_port;
  
  ROM = machine->roms;

  machine->reset();

  return 0;
}

void machine_set_timings( machine_info *machine, DWORD hz,
			  WORD left_border_cycles,  WORD screen_cycles,
			  WORD right_border_cycles, WORD retrace_cycles,
			  WORD lines_per_frame, DWORD first_line)
{
  int y;

  machine->timings.hz=hz;

  machine->timings.left_border_cycles  = left_border_cycles;
  machine->timings.screen_cycles       = screen_cycles;
  machine->timings.right_border_cycles = right_border_cycles;
  machine->timings.retrace_cycles      = retrace_cycles;

  machine->timings.cycles_per_line     = left_border_cycles +
                                         screen_cycles +
                                         right_border_cycles +
					 retrace_cycles;

  machine->timings.lines_per_frame     = lines_per_frame;

  machine->timings.cycles_per_frame    = 
    machine->timings.cycles_per_line * 
    (DWORD)lines_per_frame;

  machine->line_times[0]=first_line;
  for( y=1; y<DISPLAY_SCREEN_HEIGHT+1; y++ ) {
    machine->line_times[y] = machine->line_times[y-1] + 
                             machine->timings.cycles_per_line;
  }

}

int machine_allocate_roms( machine_info *machine, size_t count )
{
  machine->rom_count = count;

  machine->roms = (BYTE**)malloc( count * sizeof(BYTE*) );
  if( machine->roms == NULL ) {
    ui_error( "out of memory at %s:%d\n", __FILE__, __LINE__ );
    return 1;
  }

  machine->rom_lengths = (size_t*)malloc( count * sizeof(size_t) );
  if( machine->rom_lengths == NULL ) {
    ui_error( "out of memory at %s:%d\n", __FILE__, __LINE__ );
    free( machine->roms );
    return 1;
  }

  return 0;
}

int machine_read_rom( machine_info *machine, size_t number,
		      const char* filename )
{
  int fd;

  int error;

  assert( number < machine->rom_count );

  fd = machine_find_rom( filename );
  if( fd == -1 ) {
    ui_error( "couldn't find ROM `%s'\n", filename );
    return 1;
  }

  error = utils_read_fd( fd, filename, &(machine->roms[number]),
			 &(machine->rom_lengths[number]) );
  if( error ) return error;

  return 0;

}

/* Find a ROM called `filename' in some likely locations; returns a fd
   for the ROM on success or -1 if it couldn't find the ROM */
int machine_find_rom( const char *filename )
{
  int fd;

  char path[ PATHNAME_MAX_LENGTH ];

  /* First look off the current directory */
  snprintf( path, PATHNAME_MAX_LENGTH, "roms/%s", filename );
  fd = open( path, O_RDONLY );
  if( fd != -1 ) return fd;

  /* Then look where Fuse may have installed the ROMs */
  snprintf( path, PATHNAME_MAX_LENGTH, "%s/%s", DATADIR, filename );
  fd = open( path, O_RDONLY );
  if( fd != -1 ) return fd;

#ifdef ROMSDIR
  /* Finally look in a system-wide directory for ROMs. Debian uses
     /usr/share/spectrum-roms/ here */
  snprintf( path, PATHNAME_MAX_LENGTH, "%s/%s", ROMSDIR, filename );
  fd = open( path, O_RDONLY );
  if( fd != -1 ) return fd;
#endif

  return -1;
}

int machine_end( void )
{
  int i;
  int error;

  for( i=0; i<machine_count; i++ ) {
    error = machine_free_machine( machine_types[i] );
    if( error ) return error;
    free( machine_types[i] );
  }

  free( machine_types );

  return 0;
}

static int machine_free_machine( machine_info *machine )
{
  size_t i;

  char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  for( i=0; i<machine->rom_count; i++ ) {

    if( munmap( machine->roms[i], machine->rom_lengths[i] ) == -1 ) {
      snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
		"%s: couldn't munmap ROM %lu",
		fuse_progname, (unsigned long)i );
      perror( error_message );
      return 1;
    }
  }

  return 0;
}
