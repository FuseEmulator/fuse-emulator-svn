/* xkeyboard.c: X routines for dealing with the keyboard
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
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>

#include <vga.h>
#include <vgakeyboard.h>

#include "display.h"
#include "keyboard.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"

static int exitcode=0;

typedef struct xkeyboard_key_info {
  unsigned long keysym;
  int port1; BYTE bit1;
  int port2; BYTE bit2;
} xkeyboard_key_info;

/* 
   The mappings from X keysyms to Spectrum keys. The keysym `keysym'
   maps to one or two Spectrum keys, specified by {port,bit}{1,2}. 
   Unused second keys are specified by having `bit2' being zero

   These mappings are ordered basically like a standard (English) PC keyboard,
   top to bottom, left to right, but with a few additions for other keys
*/

static xkeyboard_key_info xkeyboard_data[] = {

  { SCANCODE_1          , 3, 0x01, 0, 0x00 },
  { SCANCODE_2          , 3, 0x02, 0, 0x00 },
  { SCANCODE_3	  , 3, 0x04, 0, 0x00 },
  { SCANCODE_4	  , 3, 0x08, 0, 0x00 },
  { SCANCODE_5	  , 3, 0x10, 0, 0x00 },
  { SCANCODE_6	  , 4, 0x10, 0, 0x00 },
  { SCANCODE_7	  , 4, 0x08, 0, 0x00 },
  { SCANCODE_8	  , 4, 0x04, 0, 0x00 },
  { SCANCODE_9	  , 4, 0x02, 0, 0x00 },
  { SCANCODE_0	  , 4, 0x01, 0, 0x00 },
  { SCANCODE_MINUS      , 7, 0x02, 6, 0x08 }, /* SS + J */
  { SCANCODE_EQUAL      , 7, 0x02, 6, 0x02 }, /* SS + L */
  { SCANCODE_BACKSPACE  , 0, 0x01, 4, 0x01 }, /* CS + 0 */

  { SCANCODE_Q	  , 2, 0x01, 0, 0x00 },
  { SCANCODE_W	  , 2, 0x02, 0, 0x00 },
  { SCANCODE_E	  , 2, 0x04, 0, 0x00 },
  { SCANCODE_R	  , 2, 0x08, 0, 0x00 },
  { SCANCODE_T	  , 2, 0x10, 0, 0x00 },
  { SCANCODE_Y	  , 5, 0x10, 0, 0x00 },
  { SCANCODE_U	  , 5, 0x08, 0, 0x00 },
  { SCANCODE_I	  , 5, 0x04, 0, 0x00 },
  { SCANCODE_O	  , 5, 0x02, 0, 0x00 },
  { SCANCODE_P	  , 5, 0x01, 0, 0x00 },

  { SCANCODE_CAPSLOCK   , 0, 0x01, 3, 0x02 }, /* CS + 2 */
  { SCANCODE_A	  , 1, 0x01, 0, 0x00 },
  { SCANCODE_S	  , 1, 0x02, 0, 0x00 },
  { SCANCODE_D	  , 1, 0x04, 0, 0x00 },
  { SCANCODE_F	  , 1, 0x08, 0, 0x00 },
  { SCANCODE_G	  , 1, 0x10, 0, 0x00 },
  { SCANCODE_H	  , 6, 0x10, 0, 0x00 },
  { SCANCODE_J	  , 6, 0x08, 0, 0x00 },
  { SCANCODE_K	  , 6, 0x04, 0, 0x00 },
  { SCANCODE_L	  , 6, 0x02, 0, 0x00 },
  { SCANCODE_SEMICOLON  , 7, 0x02, 5, 0x02 }, /* SS + O */
  { SCANCODE_APOSTROPHE , 7, 0x02, 4, 0x08 }, /* SS + 7 */
//{ SCANCODE_numbersign , 7, 0x02, 3, 0x04 }, /* SS + 3 */
  { SCANCODE_ENTER     , 6, 0x01, 0, 0x00 },

  { SCANCODE_LEFTSHIFT , 0, 0x01, 0, 0x00 }, /* CS */
  { SCANCODE_Z	  , 0, 0x02, 0, 0x00 },
  { SCANCODE_X	  , 0, 0x04, 0, 0x00 },
  { SCANCODE_C	  , 0, 0x08, 0, 0x00 },
  { SCANCODE_V	  , 0, 0x10, 0, 0x00 },
  { SCANCODE_B	  , 7, 0x10, 0, 0x00 },
  { SCANCODE_N    , 7, 0x08, 0, 0x00 },
  { SCANCODE_M	  , 7, 0x04, 0, 0x00 },
  { SCANCODE_COMMA      , 7, 0x02, 7, 0x08 }, /* SS + N */
  { SCANCODE_PERIOD     , 7, 0x02, 7, 0x04 }, /* SS + M */
  { SCANCODE_SLASH      , 7, 0x02, 0, 0x10 }, /* SS + V */
  { SCANCODE_RIGHTSHIFT , 0, 0x01, 0, 0x00 }, /* CS */

  { SCANCODE_LEFTCONTROL , 7, 0x02, 0, 0x00 }, /* SS */
  { SCANCODE_LEFTALT , 7, 0x02, 0, 0x00 }, /* SS */
//{ SCANCODE_Meta_L	  , 7, 0x02, 0, 0x00 }, /* SS */
  { SCANCODE_SPACE	  , 7, 0x01, 0, 0x00 },
//{ SCANCODE_Meta_R     , 7, 0x02, 0, 0x00 }, /* SS */
  { SCANCODE_RIGHTALT   , 7, 0x02, 0, 0x00 }, /* SS */
  { SCANCODE_RIGHTCONTROL , 7, 0x02, 0, 0x00 }, /* SS */
//{ SCANCODE_Mode_switch, 7, 0x02, 0, 0x00 }, /* SS */

  { 0		  , 0, 0x00, 0, 0x00 } /* End marker: DO NOT MOVE! */

};
  
int xkeyboard_keypress(int keysym)
{
  xkeyboard_key_info *ptr;
  int foundKey=0;

  ptr=xkeyboard_data;

  while( ptr->keysym && !foundKey ) {
    if(keysym==ptr->keysym) {
      foundKey=1;
      keyboard_return_values[ptr->port1] &= ~(ptr->bit1);
      keyboard_return_values[ptr->port2] &= ~(ptr->bit2);
    }
    ptr++;
  }

  /* Now deal with the non-Speccy keys */
  switch(keysym) {
  case SCANCODE_F2:
    snapshot_write();
    break;
  case SCANCODE_F3:
    snapshot_read();
    display_refresh_all();
    break;
  case SCANCODE_F5:
    machine.reset();
    break;
  case SCANCODE_F7:
    tape_open();
    break;
  case SCANCODE_F9:
    switch(machine.machine) {
    case SPECTRUM_MACHINE_48:
      machine.machine=SPECTRUM_MACHINE_128;
      break;
    case SPECTRUM_MACHINE_128:
      machine.machine=SPECTRUM_MACHINE_PLUS2;
      break;
    case SPECTRUM_MACHINE_PLUS2:
      machine.machine=SPECTRUM_MACHINE_PLUS3;
      break;
    case SPECTRUM_MACHINE_PLUS3:
      machine.machine=SPECTRUM_MACHINE_48;
      break;
    }
    spectrum_init(); machine.reset();
    break;
  case SCANCODE_F10:
    exitcode=1;
    return 1;
  }

  return 0;

}

void xkeyboard_keyrelease(int keysym)
{
  xkeyboard_key_info *ptr;
  int foundKey=0;

  ptr=xkeyboard_data;

  while(ptr->keysym && !foundKey) {
    if(keysym==ptr->keysym) {
      foundKey=1;
      keyboard_return_values[ptr->port1] |= ptr->bit1;
      keyboard_return_values[ptr->port2] |= ptr->bit2;
    }
    ptr++;
  }
}

int x_event() {
    keyboard_update();
    return exitcode;
}
