/* sdlui.c: Routines for dealing with the SDL user interface
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av, Fredrick Meunier

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

#include <stdio.h>
#include <SDL.h>

#include "display.h"
#include "fuse.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "settings.h"
#include "sdldisplay.h"
#include "sdljoystick.h"
#include "sdlkeyboard.h"
#include "ui/scaler/scaler.h"
#include "menu.h"

void
atexit_proc( void )
{ 
  SDL_ShowCursor( SDL_ENABLE );
  SDL_Quit();
}

int 
ui_init( int *argc, char ***argv )
{
  int error;

  if( ui_widget_init() ) return 1;

/* Comment out to Work around a bug in OS X 10.1 related to OpenGL in windowed
   mode */
  atexit(atexit_proc);

  error = SDL_Init( SDL_INIT_VIDEO );
  if ( error )
    return error;
#ifdef UI_SDL
#ifndef __MORPHOS__
  SDL_EnableUNICODE( 1 );
#endif				/* #ifndef __MORPHOS__ */
#endif

  sdlkeyboard_init();

  ui_mouse_present = 1;

  return 0;
}

int 
ui_event( void )
{
  SDL_Event event;

  while ( SDL_PollEvent( &event ) ) {
    switch ( event.type ) {
    case SDL_KEYDOWN:
      sdlkeyboard_keypress( &(event.key) );
      break;
    case SDL_KEYUP:
      sdlkeyboard_keyrelease( &(event.key) );
      break;
#ifdef UI_SDL2
    case SDL_TEXTINPUT:
      if( ui_widget_level >= 0 )
        sdlkeyboard_text( &(event.text) );
      break;
#endif
    case SDL_MOUSEBUTTONDOWN:
      ui_mouse_button( event.button.button, 1 );
      break;
    case SDL_MOUSEBUTTONUP:
      ui_mouse_button( event.button.button, 0 );
      break;
    case SDL_MOUSEMOTION:
      if( ui_mouse_grabbed ) {
        ui_mouse_motion( event.motion.x - 128, event.motion.y - 128 );
        if( event.motion.x != 128 || event.motion.y != 128 ) {
#ifdef UI_SDL2
          SDL_WarpMouseInWindow( sdlwin, 128, 128 );
#else
          SDL_WarpMouse( 128, 128 );
#endif
        }
      }	
      break;

#if defined USE_JOYSTICK && !defined HAVE_JSW_H

    case SDL_JOYBUTTONDOWN:
      sdljoystick_buttonpress( &(event.jbutton) );
      break;
    case SDL_JOYBUTTONUP:
      sdljoystick_buttonrelease( &(event.jbutton) );
      break;
    case SDL_JOYAXISMOTION:
      sdljoystick_axismove( &(event.jaxis) );
      break;

#endif			/* if defined USE_JOYSTICK && !defined HAVE_JSW_H */

    case SDL_QUIT:
      fuse_emulation_pause();
      menu_file_exit(0);
      fuse_emulation_unpause();
      break;
#ifdef UI_SDL2
    case SDL_WINDOWEVENT:
      if( event.window.event == SDL_WINDOWEVENT_EXPOSED )
        display_refresh_all();
      if( event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED )
        ui_mouse_resume();
      if( event.window.event == SDL_WINDOWEVENT_FOCUS_LOST )
        ui_mouse_suspend();
      if( event.window.event == SDL_WINDOWEVENT_RESIZED ) {
        if( settings_current.aspect_hint
#ifdef USE_WM_ASPECT_X11
            && !sdldisplay_use_wm_aspect_hint
#endif
        ) {

          int aspect = 3 * event.window.data1 - 4 * event.window.data2;

          if( aspect < 0 ) /* too high */
            SDL_SetWindowSize( sdlwin, event.window.data1,
                               3 * event.window.data1 / 4 );
          else if( aspect > 0 ) /* too wide */
            SDL_SetWindowSize( sdlwin, 4 * event.window.data2 / 3,
                               event.window.data2 );
        }
        SDL_RenderClear( sdlren );
        sdldisplay_force_full_refresh = 1;
        if( ui_widget_level >= 0 ) {
          uidisplay_frame_end();
        }
      }
#endif
#ifdef UI_SDL
    case SDL_VIDEOEXPOSE:
      display_refresh_all();
      break;
    case SDL_ACTIVEEVENT:
      if( event.active.state & SDL_APPINPUTFOCUS ) {
	if( event.active.gain ) ui_mouse_resume(); else ui_mouse_suspend();
      }
#endif
      break;
    default:
      break;
    }
  }

  return 0;
}

int 
ui_end( void )
{
  int error;

  error = uidisplay_end();
  if ( error )
    return error;

  sdlkeyboard_end();

  SDL_Quit();

  ui_widget_end();

  return 0;
}

int
ui_statusbar_update_speed( float speed )
{
  char buffer[15];
  const char fuse[] = "Fuse";

  snprintf( buffer, 15, "%s - %3.0f%%", fuse, speed );

  /* FIXME: Icon caption should be snapshot name? */
#ifdef UI_SDL2
  SDL_SetWindowTitle( sdlwin, buffer );
#else
  SDL_WM_SetCaption( buffer, fuse );
#endif
  return 0;
}

int
ui_mouse_grab( int startup )
{
  if( settings_current.full_screen ) {
#ifdef UI_SDL2
    SDL_WarpMouseInWindow( sdlwin, 128, 128 );
#else
    SDL_WarpMouse( 128, 128 );
#endif
    return 1;
  }
  if( startup ) return 0;

#ifdef UI_SDL2
  if( SDL_SetRelativeMouseMode( SDL_TRUE ) < 0 ) {
    ui_error( UI_ERROR_WARNING, "Mouse grab failed" );
    return 0;
  }
  return 1;
#else
  switch( SDL_WM_GrabInput( SDL_GRAB_ON ) ) {
  case SDL_GRAB_ON:
  case SDL_GRAB_FULLSCREEN:
    SDL_ShowCursor( SDL_DISABLE );
    SDL_WarpMouse( 128, 128 );
    return 1;
  default:
    ui_error( UI_ERROR_WARNING, "Mouse grab failed" );
    return 0;
  }
#endif				/* #ifdef UI_SDL2 */
}

int
ui_mouse_release( int suspend )
{
  if( settings_current.full_screen ) return !suspend;

#ifdef UI_SDL2
  SDL_SetRelativeMouseMode( SDL_FALSE );
#else
  SDL_WM_GrabInput( SDL_GRAB_OFF );
  SDL_ShowCursor( SDL_ENABLE );
#endif

  return 0;
}
