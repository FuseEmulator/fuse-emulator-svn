/* x.c: Routines for dealing with X events
   Copyright (c) 2000 Philip Kendall

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

   E-mail: pak21-fuse.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_X			/* Use this iff we're using Xlib */

#include <X11/Xlib.h>

#include "xdisplay.h"
#include "xkeyboard.h"

Display *display;		/* Which display are we connected to */

Bool x_trueFunction();

int x_event(void)
{
  XEvent event;

  while(XCheckIfEvent(display,&event,x_trueFunction,NULL)) {
    switch(event.type) {
    case ConfigureNotify:
      xdisplay_configure_notify(event.xconfigure.width,
				event.xconfigure.height);
      break;
    case Expose:
      xdisplay_area(event.xexpose.x,event.xexpose.y,
		    event.xexpose.width,event.xexpose.height);
      break;
    case KeyPress:
      return xkeyboard_keypress(&(event.xkey));
    case KeyRelease:
      xkeyboard_keyrelease(&(event.xkey));
    }
  }
  return 0;
}
    
Bool x_trueFunction()
{
  return True;
}

#endif				/* #ifdef UI_X */
