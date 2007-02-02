/* win32internals.h: stuff internal to the Win32 UI
   Copyright (c) 2004 Marek Januszewski

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_WIN32INTERNALS_H
#define FUSE_WIN32INTERNALS_H

#include <windows.h>
#include <ddraw.h>
#include <commctrl.h>

#define ID_STATUSBAR 200

/* window handler */
HWND fuse_hWnd;

/* application instance */
HINSTANCE fuse_hInstance;

/* status bar handle */
HWND fuse_hStatusWindow;

/* pokefinder window handle */
HWND fuse_hPFWnd;

/* debugger window handle */
HWND fuse_hDBGWnd;

/* DirectDraw objects */
LPDIRECTDRAW lpdd;
DDSURFACEDESC ddsd;
LPDIRECTDRAWSURFACE lpdds, /* primary surface */
                    lpdds2; /* off screen surface */
LPDIRECTDRAWCLIPPER lpddc; /* clipper for the window */

#define DD_ERROR( msg )   if ( ddres != DD_OK ) {\
    uidisplay_end();\
    ui_error( UI_ERROR_ERROR, msg, NULL );\
    return 1;\
  }

int win32statusbar_create();

int win32statusbar_set_visibility( int visible );

void menu_machine_pokefinder( int action );

void win32_verror( int is_error );

#endif				/* #ifndef FUSE_WIN32INTERNALS_H */
