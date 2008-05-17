/* widget_orig.c: Simple keyboard-driven dialog boxes for all user interfaces.
   Copyright (c) 2001-2005 Matan Ziv-Av, Philip Kendall, Russell Marks

   $Id: widget.c 3288 2007-11-09 18:12:45Z zubzero $

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

#include "fuse.h"
#include "input.h"
#include "menu.h"
#include "options.h"
#include "pokefinder/pokefinder.h"
#include "widget.h"
#include "widget_standard.h"
#include "widget_internals.h"

/* Arrows for any scrollable widget */
void
widget_up_arrow( int x, int y, int colour )
{
  int i, j;
  x = x * 8;
  y = y * 8 + 7;
  for( j = 6; j; --j ) {
    for( i = (j + 1) / 2; i < 4; ++i ) {
      widget_putpixel( x +     i, y - j, colour );
      widget_putpixel( x + 7 - i, y - j, colour );
    }
  }
}

void
widget_down_arrow( int x, int y, int colour )
{
  int i, j;
  x = x * 8;
  y = y * 8;
  for( j = 6; j; --j ) {
    for( i = (j + 1) / 2; i < 4; ++i ) {
      widget_putpixel( x +     i, y + j, colour );
      widget_putpixel( x + 7 - i, y + j, colour );
    }
  }
}

void widget_print_checkbox( int x, int y, int value )
{
    static const int CHECK_COLOR=7;
    int z;

    y += 2;
    x += 6;
    widget_rectangle( x, y - 1, 3, 3, WIDGET_COLOUR_BACKGROUND );
    widget_rectangle( x - 5, y, 5, 5, 0 );
    widget_rectangle( x - 4, y + 1, 3, 3, WIDGET_COLOUR_BACKGROUND );
    if( value ) {	/* checked */
      for( z = -1; z < 3; z++ ) {
        widget_putpixel( x - z, y + z, CHECK_COLOR );
        widget_putpixel( x - z + 1, y + z, CHECK_COLOR );
      }
      widget_putpixel( x - z + 1, y + z, CHECK_COLOR );
      widget_putpixel( x - z, y + z - 1, CHECK_COLOR );
      widget_putpixel( x - z, y + z - 2, CHECK_COLOR );
      widget_putpixel( x - z - 1, y + z - 2, CHECK_COLOR );
    }
}

int widget_dialog( int x, int y, int width, int height )
{
  widget_rectangle( 8*x, 8*y, 8*width, 8*height, WIDGET_COLOUR_BACKGROUND );
  return 0;
}

int widget_dialog_with_border( int x, int y, int width, int height )
{
  int i;

  widget_rectangle( 8*x-5, 8*y, 8*width+10, 8*height, WIDGET_COLOUR_BACKGROUND );

  for( i = 0; i < 5; ++i) {
    widget_rectangle (8*x+i-5, 8*y-i-1, 8*width+10-2*i, 1, WIDGET_COLOUR_BACKGROUND );
    widget_rectangle (8*x+i-5, 8*(y+height)+i, 8*width+10-2*i, 1, WIDGET_COLOUR_BACKGROUND );
  }

  for( i=(8*x)-1; i<(8*(x+width))+1; i++ ) {
    widget_putpixel( i, 8 *   y            - 4, WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( i, 8 * ( y + height ) + 3, WIDGET_COLOUR_FOREGROUND );
  }

  for( i=(8*y)-1; i<(8*(y+height))+1; i++ ) {
    widget_putpixel( 8 *   x           - 4, i, WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( 8 * ( x + width ) + 3, i, WIDGET_COLOUR_FOREGROUND );
  }

  for( i=0; i<2; i++ ) {
    widget_putpixel( 8 *   x           - 3 + i, 8 *   y            - 2 - i,
		     WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( 8 * ( x + width ) + 2 - i, 8 *   y            - 2 - i,
		     WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( 8 *   x           - 3 + i, 8 * ( y + height ) + 1 + i,
		     WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( 8 * ( x + width ) + 2 - i, 8 * ( y + height ) + 1 + i,
		     WIDGET_COLOUR_FOREGROUND );
  }

  widget_display_lines( y-1, height+2 );

  return 0;
}

/* Look for widget keyboard shortcuts */
void widget_shortcuts( input_key key )
{
  switch( key ) {
  case INPUT_KEY_F1:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_MENU, &widget_menu );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F2:
    fuse_emulation_pause();
    menu_file_savesnapshot( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F3:
    fuse_emulation_pause();
    menu_file_open( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F4:
    fuse_emulation_pause();
    menu_options_general( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F5:
    fuse_emulation_pause();
    menu_machine_reset( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F6:
    fuse_emulation_pause();
    menu_media_tape_write( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F7:
    fuse_emulation_pause();
    menu_media_tape_open( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F8:
    menu_media_tape_play( 0 );
    break;
  case INPUT_KEY_F9:
    fuse_emulation_pause();
    menu_machine_select( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F10:
    fuse_emulation_pause();
    menu_file_exit( 0 );
    fuse_emulation_unpause();
    break;

  default: break;		/* Remove gcc warning */

  }
}

ui_confirm_save_t
ui_confirm_save_specific( const char *message )
{
  if( widget_do( WIDGET_TYPE_QUERY_SAVE, (void *) message ) )
    return UI_CONFIRM_SAVE_CANCEL;
  return widget_query.save;
}

static const char *joystick_connection[] = {
  "None",
  "Keyboard",
  "Joystick 1",
  "Joystick 2",
};

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
  widget_select_t info;
  int error;
  char title[80];

  if( !settings_current.joy_prompt ) return UI_CONFIRM_JOYSTICK_NONE;

  snprintf( title, sizeof( title ), "Configure %s joystick",
	    libspectrum_joystick_name( libspectrum_type ) );

  info.title = title;
  info.options = joystick_connection;
  info.count = sizeof( joystick_connection    ) /
	       sizeof( joystick_connection[0] );
  info.current = UI_CONFIRM_JOYSTICK_NONE;

  error = widget_do( WIDGET_TYPE_SELECT, &info );
  if( error ) return UI_CONFIRM_JOYSTICK_NONE;

  return (ui_confirm_joystick_t)info.result;
}

/* The widgets actually available. Make sure the order here matches the
   order defined in enum widget_type (widget.h) */

widget_t widget_data[] = {

  { widget_filesel_load_draw, widget_filesel_finish, widget_filesel_keyhandler  },
  { widget_filesel_save_draw, widget_filesel_finish, widget_filesel_keyhandler  },
  { widget_general_draw,  widget_options_finish, widget_general_keyhandler  },
  { widget_picture_draw,  NULL,                  widget_picture_keyhandler  },
  { widget_menu_draw,	  NULL,			 widget_menu_keyhandler     },
  { widget_select_draw,   widget_select_finish,  widget_select_keyhandler   },
  { widget_sound_draw,	  widget_options_finish, widget_sound_keyhandler    },
  { widget_error_draw,	  NULL,			 widget_error_keyhandler    },
  { widget_rzx_draw,      widget_options_finish, widget_rzx_keyhandler      },
  { widget_browse_draw,   widget_browse_finish,  widget_browse_keyhandler   },
  { widget_text_draw,	  widget_text_finish,	 widget_text_keyhandler     },
  { widget_debugger_draw, NULL,			 widget_debugger_keyhandler },
  { widget_pokefinder_draw, NULL,		 widget_pokefinder_keyhandler },
  { widget_memory_draw,   NULL,			 widget_memory_keyhandler   },
  { widget_roms_draw,     widget_roms_finish,	 widget_roms_keyhandler     },
  { widget_peripherals_draw, widget_options_finish,
			                      widget_peripherals_keyhandler },
  { widget_query_draw,    NULL,			 widget_query_keyhandler    },
  { widget_query_save_draw,NULL,		 widget_query_save_keyhandler },
};

#ifndef UI_SDL
/* The statusbar handling functions */
/* TODO: make these do something useful */
int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  return 0;
}

#ifndef UI_X
int
ui_statusbar_update_speed( float speed )
{
  return 0;
}
#endif
#endif                          /* #ifndef UI_SDL */

/* Tape browser update function. The dialog box is created every time it
   is displayed, so no need to do anything here */
int
ui_tape_browser_update( ui_tape_browser_update_type change,
                        libspectrum_tape_block *block )
{
  return 0;
}

/* FIXME: make this do something useful */
int
ui_get_rollback_point( GSList *points )
{
  return -1;
}

int
ui_widgets_reset( void )
{
  pokefinder_clear();
  return 0;
}
