/* ui.h: General UI event handling routines
   Copyright (c) 2000-2002 Philip Kendall

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

#ifndef FUSE_UI_H
#define FUSE_UI_H

#include <stdarg.h>

#include <libspectrum.h>

#ifndef FUSE_FUSE_H
#include "fuse.h"
#endif

#ifndef FUSE_KEYBOARD_H
#include "keyboard.h"
#endif			        /* #ifndef FUSE_KEYBOARD_H */

/* The various severities of error level, increasing downwards */
typedef enum ui_error_level {

  UI_ERROR_INFO,		/* Informational message */
  UI_ERROR_ERROR,		/* An actual error */

} ui_error_level;

extern const keysyms_key_info keysyms_data[];

int ui_init(int *argc, char ***argv, int width, int height);
int ui_event(void);
int ui_verror( ui_error_level severity, const char *format, va_list ap )
     GCC_PRINTF( 2, 0 );
int ui_end(void);

int ui_debugger_activate( void );

/* Functions defined in ../ui.c */
int ui_error( ui_error_level severity, const char *format, ... )
     GCC_PRINTF( 2, 3 );
libspectrum_error ui_libspectrum_error( libspectrum_error error,
					const char *format, va_list ap );

#endif			/* #ifndef FUSE_UI_H */
