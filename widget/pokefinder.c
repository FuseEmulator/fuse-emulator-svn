/* pokefinder.c: The poke finder widget
   Copyright (c) 2004 Darren Salt

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
   Email: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <stdio.h>
#include <string.h>

#include "compat.h"
#include "debugger/debugger.h"
#include "pokefinder/pokefinder.h"
#include "ui/ui.h"
#include "widget.h"
#include "widget_internals.h"

#define MAX_POSSIBLE 8
#define FEW_ENOUGH() (pokefinder_count && pokefinder_count <= MAX_POSSIBLE)

static int value = 0;
static int possible_page[ MAX_POSSIBLE ];
static libspectrum_word possible_offset[ MAX_POSSIBLE ];
static int selected = 0;

static void update_possible( void );
static void display_possible( void );
static void display_value( void );

static const char *title = "Poke finder";

int
widget_pokefinder_draw( void *data )
{
  widget_dialog_with_border( 1, 2, 30, 12 );
  widget_printstring( 15 - strlen( title ) / 2, 2, WIDGET_COLOUR_FOREGROUND,
		      title );
  widget_printstring( 2, 4, WIDGET_COLOUR_FOREGROUND, "Possible: " );
  widget_printstring( 2, 5, WIDGET_COLOUR_FOREGROUND, "Value: " );

  update_possible();
  display_possible();
  display_value();

  widget_printstring( 2, 12, WIDGET_COLOUR_FOREGROUND,
		      "(I)nc'd (D)ec'd (S)earch" );
  widget_printstring( 2, 13, WIDGET_COLOUR_FOREGROUND, "(R)eset (C)lose" );

  widget_display_lines( 2, 12 );

  return 0;
}

static void
update_possible( void )
{
  size_t page, offset, i = 0;

  selected = 0;

  if( !FEW_ENOUGH() )
    return;

  for( page = 0; page < SPECTRUM_RAM_PAGES; ++page )
    for( offset = 0; offset < 0x4000; ++offset )
      if( ! (pokefinder_impossible[page][offset/8] & 1 << (offset & 7)) ) {
	possible_page[i] = page / 2;
	possible_offset[i] = offset + 8192 * (page & 1);
	if( ++i == pokefinder_count )
	  return;
      }
}

static void
display_possible( void )
{
  char buf[ 32 ];

  widget_rectangle(  96,  32,  48,  8, WIDGET_COLOUR_BACKGROUND );
  widget_rectangle(  16,  56, 128, 32, WIDGET_COLOUR_BACKGROUND );
  widget_rectangle(  16,  88, 136,  8, WIDGET_COLOUR_BACKGROUND );
  widget_rectangle( 144, 104,  56,  8, WIDGET_COLOUR_BACKGROUND );

  snprintf( buf, sizeof( buf ), "%d", pokefinder_count );
  widget_printstring( 12, 4, WIDGET_COLOUR_FOREGROUND, buf );

  if( FEW_ENOUGH() ) {
    size_t i;

    for( i = 0; i < pokefinder_count; i++ ) {
      int x = 2 + (i / 4) * 8;
      int y = 7 + (i % 4);
      int colour;

      if( i == selected ) {
	widget_rectangle( x * 8, y * 8, 56, 8, WIDGET_COLOUR_FOREGROUND );
	colour = WIDGET_COLOUR_BACKGROUND;
      } else {
	colour = WIDGET_COLOUR_FOREGROUND;
      }

      snprintf( buf, sizeof( buf ), "%2d:%04X", possible_page[i],
		possible_offset[i] );
      widget_printstring( x, y, colour, buf );
    }

    widget_printstring( 18, 13, WIDGET_COLOUR_FOREGROUND, "(B)reak" );
  }

  widget_display_lines( 4, 10 );
}

static void
display_value( void )
{
  char buf[16];

  snprintf( buf, sizeof( buf ), "%d", value );
  widget_rectangle( 72, 40, 24, 8, WIDGET_COLOUR_BACKGROUND );
  widget_printstring( 9, 5, WIDGET_COLOUR_FOREGROUND, buf );
  widget_display_lines( 5, 1 );
}

static void
scroll( int step )
{
  if( !FEW_ENOUGH() ) return;

  selected += step;
  if( selected < 0 )
    selected = 0;
  else if( selected >= pokefinder_count )
    selected = pokefinder_count - 1;

  display_possible();
}

void
widget_pokefinder_keyhandler( input_key key, long rawkey )
{
  switch ( key ) {
  case INPUT_KEY_Escape:	/* Close widget */
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    break;

  case INPUT_KEY_c:		/* Close widget */
    widget_end_all( WIDGET_FINISHED_OK );
    break;

  case INPUT_KEY_i:		/* Search for incremented */
    pokefinder_incremented();
    update_possible();
    display_possible();
    break;

  case INPUT_KEY_d:		/* Search for incremented */
    pokefinder_decremented();
    update_possible();
    display_possible();
    break;

  case INPUT_KEY_Return:
  case INPUT_KEY_s:		/* Search */
    if( value < 256 ) {
      pokefinder_search( value );
      update_possible();
      display_possible();
    }
    break;

  case INPUT_KEY_r:		/* Reset */
    pokefinder_clear();
    update_possible();
    display_possible();
    break;

  case INPUT_KEY_b:		/* Add breakpoint */
    if( FEW_ENOUGH() )
    {
      widget_rectangle( 128, 32, 112, 8, WIDGET_COLOUR_BACKGROUND );
      if( debugger_breakpoint_add_address(
            DEBUGGER_BREAKPOINT_TYPE_WRITE, possible_page[selected],
	    possible_offset[selected], 0, DEBUGGER_BREAKPOINT_LIFE_PERMANENT,
	    NULL
	  ) ) {
	widget_printstring( 2, 11, WIDGET_COLOUR_FOREGROUND,
			    "Breakpoint failed" );
      } else {
	widget_printstring( 2, 11, WIDGET_COLOUR_FOREGROUND,
			    "Breakpoint added" );
      }
      widget_display_lines( 11, 1 );
    }
    break;

  /* Address selection */
  case INPUT_KEY_Up:	scroll(  -1 ); break;
  case INPUT_KEY_Down:	scroll(   1 ); break;
  case INPUT_KEY_Left:	scroll(  -4 ); break;
  case INPUT_KEY_Right:	scroll(   4 ); break;
  case INPUT_KEY_Home:	scroll( -20 ); break;
  case INPUT_KEY_End:	scroll(  20 ); break;

  case INPUT_KEY_0 ... INPUT_KEY_9: /* Value alteration */
    value = (value % 100) * 10 + key - INPUT_KEY_0;
    display_value();
    break;

  case INPUT_KEY_BackSpace:	/* Value alteration */
    value /= 10;
    display_value();
    break;

  default:;
  }
}
