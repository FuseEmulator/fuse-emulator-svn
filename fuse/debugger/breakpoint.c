/* breakpoint.c: a debugger breakpoint
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

#include <config.h>

#include <libspectrum.h>

#include "debugger_internals.h"
#include "event.h"
#include "fuse.h"
#include "memory.h"
#include "ui/ui.h"

/* The current breakpoints */
GSList *debugger_breakpoints;

/* The next breakpoint ID to use */
static size_t next_breakpoint_id;

/* Textual represenations of the breakpoint types and lifetimes */
const char *debugger_breakpoint_type_text[] = {
  "Execute", "Read", "Write", "Port Read", "Port Write", "Time",
};

const char *debugger_breakpoint_life_text[] = {
  "Permanent", "One Shot",
};

static int breakpoint_add( debugger_breakpoint_type type,
			   debugger_breakpoint_value value, size_t ignore,
			   debugger_breakpoint_life life,
			   debugger_expression *condition );
static int breakpoint_check( debugger_breakpoint *bp,
			     debugger_breakpoint_type type,
			     libspectrum_dword value );
static debugger_breakpoint* get_breakpoint_by_id( size_t id );
static gint find_breakpoint_by_id( gconstpointer data,
				   gconstpointer user_data );
static void remove_time( gpointer data, gpointer user_data );
static gint find_breakpoint_by_id( gconstpointer data,
				   gconstpointer user_data );
static gint find_breakpoint_by_address( gconstpointer data,
					gconstpointer user_data );
static void free_breakpoint( gpointer data, gpointer user_data );
static void add_time_event( gpointer data, gpointer user_data );

/* Add a breakpoint */
int
debugger_breakpoint_add_address( debugger_breakpoint_type type, int page,
				 libspectrum_word offset, size_t ignore,
				 debugger_breakpoint_life life,
				 debugger_expression *condition )
{
  debugger_breakpoint_value value;

  switch( type ) {
  case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
  case DEBUGGER_BREAKPOINT_TYPE_READ:
  case DEBUGGER_BREAKPOINT_TYPE_WRITE:
    break;

  default:
    ui_error( UI_ERROR_ERROR, "debugger_breakpoint_add_address given type %d",
	      type );
    fuse_abort();
  }

  value.address.page = page;
  value.address.offset = offset;

  return breakpoint_add( type, value, ignore, life, condition );
}

int
debugger_breakpoint_add_port( debugger_breakpoint_type type,
			      libspectrum_word port, libspectrum_word mask,
			      size_t ignore, debugger_breakpoint_life life,
			      debugger_expression *condition )
{
  debugger_breakpoint_value value;

  switch( type ) {
  case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
  case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
    break;

  default:
    ui_error( UI_ERROR_ERROR, "debugger_breakpoint_add_port given type %d",
	      type );
    fuse_abort();
  }

  value.port.port = port;
  value.port.mask = mask;

  return breakpoint_add( type, value, ignore, life, condition );
}

int
debugger_breakpoint_add_time( debugger_breakpoint_type type,
			      libspectrum_dword tstates, size_t ignore,
			      debugger_breakpoint_life life,
			      debugger_expression *condition )
{
  debugger_breakpoint_value value;

  switch( type ) {
  case DEBUGGER_BREAKPOINT_TYPE_TIME:
    break;

  default:
    ui_error( UI_ERROR_ERROR, "debugger_breakpoint_add_time given type %d",
	      type );
    fuse_abort();
  }

  value.tstates = tstates;

  return breakpoint_add( type, value, ignore, life, condition );
}

static int
breakpoint_add( debugger_breakpoint_type type, debugger_breakpoint_value value,
		size_t ignore, debugger_breakpoint_life life,
		debugger_expression *condition )
{
  debugger_breakpoint *bp;

  bp = malloc( sizeof( debugger_breakpoint ) );
  if( !bp ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  bp->id = next_breakpoint_id++; bp->type = type;
  bp->value = value;
  bp->ignore = ignore; bp->life = life;
  bp->condition = condition;

  debugger_breakpoints = g_slist_append( debugger_breakpoints, bp );

  if( debugger_mode == DEBUGGER_MODE_INACTIVE )
    debugger_mode = DEBUGGER_MODE_ACTIVE;

  /* If this was a timed breakpoint, set an event to stop emulation
     at that point */
  if( type == DEBUGGER_BREAKPOINT_TYPE_TIME ) {
    int error;

    error = event_add( value.tstates, EVENT_TYPE_BREAKPOINT );
    if( error ) return error;
  }

  return 0;
}

/* Check whether the debugger should become active at this point */
int
debugger_check( debugger_breakpoint_type type, libspectrum_dword value )
{
  GSList *ptr; debugger_breakpoint *bp;

  switch( debugger_mode ) {

  case DEBUGGER_MODE_INACTIVE: return 0;

  case DEBUGGER_MODE_ACTIVE:
    for( ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {

      bp = ptr->data;

      if( breakpoint_check( bp, type, value ) ) {
	debugger_mode = DEBUGGER_MODE_HALTED;
	return 1;
      }

    }
    return 0;

  case DEBUGGER_MODE_HALTED: return 1;

  }
  return 0;	/* Keep gcc happy */
}

/* Check whether 'bp' should trigger if we're looking for a breakpoint
   of 'type' with parameter 'value'. Returns non-zero if we should trigger */
static int
breakpoint_check( debugger_breakpoint *bp, debugger_breakpoint_type type,
		  libspectrum_dword value )
{
  int page;

  if( bp->type != type ) return 0;

  switch( bp->type ) {

  case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
  case DEBUGGER_BREAKPOINT_TYPE_READ:
  case DEBUGGER_BREAKPOINT_TYPE_WRITE:

    page = bp->value.address.page;

    /* If page == -1, value must match exactly; otherwise, the page and
       the offset must match */
    if( page == -1 ) {
      if( bp->value.address.offset != value ) return 0;
    } else {

      /* FIXME: handle different banks */
      if( page != memory_map[ value >> 13 ].page_num ) return 0;

      if( bp->value.address.offset != ( value & 0x3fff ) ) return 0;
    }
    break;

    /* Port values must match after masking */
  case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
  case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
    if( ( value & bp->value.port.mask ) != bp->value.port.port ) return 0;
    break;

    /* Timed breakpoints trigger if we're past the relevant time */
  case DEBUGGER_BREAKPOINT_TYPE_TIME:
    if( bp->value.tstates < tstates ) return 0;
    break;

  default:
    ui_error( UI_ERROR_ERROR, "Unknown breakpoint type %d", bp->type );
    fuse_abort();

  }

  if( bp->ignore ) { bp->ignore--; return 0; }

  if( bp->condition && !debugger_expression_evaluate( bp->condition ) )
    return 0;

  if( bp->life == DEBUGGER_BREAKPOINT_LIFE_ONESHOT ) {
    debugger_breakpoints = g_slist_remove( debugger_breakpoints, bp );
    free( bp );
  }

  return 1;
}

struct remove_t {

  libspectrum_dword tstates;
  int done;

};

/* Remove breakpoint with the given ID */
int
debugger_breakpoint_remove( size_t id )
{
  debugger_breakpoint *bp;

  bp = get_breakpoint_by_id( id ); if( !bp ) return 1;

  debugger_breakpoints = g_slist_remove( debugger_breakpoints, bp );
  if( debugger_mode == DEBUGGER_MODE_ACTIVE && !debugger_breakpoints )
    debugger_mode = DEBUGGER_MODE_INACTIVE;

  /* If this was a timed breakpoint, remove the event as well */
  if( bp->type == DEBUGGER_BREAKPOINT_TYPE_TIME ) {

    struct remove_t remove;

    remove.tstates = bp->value.tstates;
    remove.done = 0;

    event_foreach( remove_time, &remove );
  }

  free( bp );

  return 0;
}

static debugger_breakpoint*
get_breakpoint_by_id( size_t id )
{
  GSList *ptr;

  ptr = g_slist_find_custom( debugger_breakpoints, &id,
			     find_breakpoint_by_id );
  if( !ptr ) {
    ui_error( UI_ERROR_ERROR, "Breakpoint %ld does not exist",
	      (unsigned long)id );
    return NULL;
  }

  return ptr->data;
}

static gint
find_breakpoint_by_id( gconstpointer data, gconstpointer user_data )
{
  const debugger_breakpoint *bp = data;
  size_t id = *(const size_t*)user_data;

  return bp->id - id;
}

static void
remove_time( gpointer data, gpointer user_data )
{
  event_t *event;
  struct remove_t *remove;

  event = data; remove = user_data;

  if( remove->done ) return;

  if( event->type == EVENT_TYPE_BREAKPOINT &&
      event->tstates == remove->tstates ) {
    event->type = EVENT_TYPE_NULL;
    remove->done = 1;
  }
}

/* Remove all breakpoints at the given address */
int
debugger_breakpoint_clear( libspectrum_word address )
{
  GSList *ptr;

  int found = 0;

  while( 1 ) {

    ptr = g_slist_find_custom( debugger_breakpoints, &address,
			       find_breakpoint_by_address );
    if( !ptr ) break;

    found++;

    debugger_breakpoints = g_slist_remove( debugger_breakpoints, ptr->data );
    if( debugger_mode == DEBUGGER_MODE_ACTIVE && !debugger_breakpoints )
      debugger_mode = DEBUGGER_MODE_INACTIVE;

    free( ptr->data );
  }

  if( !found ) {
    if( debugger_output_base == 10 ) {
      ui_error( UI_ERROR_ERROR, "No breakpoint at %d", address );
    } else {
      ui_error( UI_ERROR_ERROR, "No breakpoint at 0x%04x", address );
    }
  }

  return 0;
}

static gint
find_breakpoint_by_address( gconstpointer data, gconstpointer user_data )
{
  const debugger_breakpoint *bp = data;
  libspectrum_word address = *(const libspectrum_word*)user_data;

  if( bp->type != DEBUGGER_BREAKPOINT_TYPE_EXECUTE &&
      bp->type != DEBUGGER_BREAKPOINT_TYPE_READ    &&
      bp->type != DEBUGGER_BREAKPOINT_TYPE_WRITE      )
    return 1;

  /* Ignore all page-specific breakpoints */
  if( bp->value.address.page != -1 ) return 1;

  return bp->value.address.offset - address;
}

/* Remove all breakpoints */
int
debugger_breakpoint_remove_all( void )
{
  g_slist_foreach( debugger_breakpoints, free_breakpoint, NULL );
  g_slist_free( debugger_breakpoints ); debugger_breakpoints = NULL;

  if( debugger_mode == DEBUGGER_MODE_ACTIVE )
    debugger_mode = DEBUGGER_MODE_INACTIVE;

  /* Restart the breakpoint numbering */
  next_breakpoint_id = 1;

  return 0;
}

static void
free_breakpoint( gpointer data, gpointer user_data GCC_UNUSED )
{
  debugger_breakpoint *bp = data;

  if( bp->condition ) debugger_expression_delete( bp->condition );

  free( bp );
}

/* Ignore breakpoint 'id' the next 'ignore' times it hits */
int
debugger_breakpoint_ignore( size_t id, size_t ignore )
{
  debugger_breakpoint *bp;

  bp = get_breakpoint_by_id( id ); if( !bp ) return 1;

  bp->ignore = ignore;

  return 0;
}

/* Set the breakpoint's conditional expression */
int
debugger_breakpoint_set_condition( size_t id, debugger_expression *condition )
{
  debugger_breakpoint *bp;

  bp = get_breakpoint_by_id( id ); if( !bp ) return 1;

  if( bp->condition ) debugger_expression_delete( bp->condition );

  bp->condition = condition;

  return 0;
}

/* Add events corresponding to all the time events to happen during
   this frame */
int
debugger_add_time_events( void )
{
  g_slist_foreach( debugger_breakpoints, add_time_event, NULL );
  return 0;
}

static void
add_time_event( gpointer data, gpointer user_data GCC_UNUSED )
{
  debugger_breakpoint *bp = data;

  if( bp->type == DEBUGGER_BREAKPOINT_TYPE_TIME )
    event_add( bp->value.tstates, EVENT_TYPE_BREAKPOINT );
}
