/* scld.c: Routines for handling the Timex SCLD
   Copyright (c) 2002 Fredrick Meunier

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

   E-mail: fredm@spamcop.net

*/

#include <config.h>

#include <stdio.h>

#include "scld.h"
#include "display.h"

BYTE is_altdfile     = 0;
BYTE is_extcolour    = 0;
BYTE is_hires        = 0;
BYTE is_intdisable   = 0;
BYTE is_altmembank   = 0;

BYTE scld_screenmode = 0;

BYTE last_scld_dec   = 0;           /* The last byte sent to Timex DEC port */

BYTE last_scld_hsr   = 0;           /* The last byte sent to Timex HSR port */

BYTE scld_dec_read(WORD port)
{
  return last_scld_dec;
}

void scld_dec_write(WORD port, BYTE b)
{
  int old_dec     = last_scld_dec;
  BYTE old_hirescol = hires_get_attr();
  BYTE ink,paper;

  old_dec        &= (ALTDFILE|EXTCOLOUR|HIRES);

  last_scld_dec   = b;

  is_altdfile     = last_scld_dec & ALTDFILE;
  is_extcolour    = last_scld_dec & EXTCOLOUR;
  is_hires        = last_scld_dec & HIRES;
  is_intdisable   = last_scld_dec & INTDISABLE;
  is_altmembank   = last_scld_dec & ALTMEMBANK;

  scld_screenmode = last_scld_dec & (ALTDFILE|EXTCOLOUR|HIRES);

  /* If we changed the active screen, or change the colour in hires
   * mode mark the entire display file as dirty so we redraw it on
   * the next pass */
  if((scld_screenmode != old_dec)
       || (is_hires && (old_hirescol != hires_get_attr()))) {
    display_refresh_all();
  }

  display_parse_attr(hires_get_attr(),&ink,&paper);
  display_set_hires_border(paper);
}

void scld_reset(void)
{
  is_altdfile     = 0;
  is_extcolour    = 0;
  is_hires        = 0;
  is_intdisable   = 0;
  is_altmembank   = 0;

  scld_screenmode = 0;

  last_scld_dec   = 0;
}

void scld_hsr_write (WORD port, BYTE b)
{
  last_scld_hsr   = b;
}

BYTE scld_hsr_read (WORD port)
{
  return last_scld_hsr;
}

BYTE hires_get_attr(void)
{
  switch (last_scld_dec & HIRESCOLMASK)
  {
    case YELLOWBLUE:
      return (BYTE) 0x31;
      break;
    case CYANRED:
      return (BYTE) 0x2A;
      break;
    case GREENMAGENTA:
      return (BYTE) 0x23;
      break;
    case MAGENTAGREEN:
      return (BYTE) 0x1C;
      break;
    case REDCYAN:
      return (BYTE) 0x15;
      break;
    case BLUEYELLOW:
      return (BYTE) 0x0E;
      break;
    case BLACKWHITE:
      return (BYTE) 0x07;
      break;
    default:  /* WHITEBLACK */
      return (BYTE) 0x38;
  }
}
