
#include <config.h>

#ifdef UI_QTOPIA 		/* Use this if we're using qtopia */

#include <stdio.h>

#include "qtopiadisplay.h"
#include "fuse.h"
#include "ui/ui.h"
#include "display.h"
//#include "uidisplay.h"
#include "ui/uidisplay.h"

int ui_init(int *argc, char ***argv, int width, int height)
{
  int error;

  error = uidisplay_init(width, height);
  if(error) return error;

//  error = qtopiakeyboard_init();
//  if(error) return error;

  return 0;
}

int ui_event()
{
//  keyboard_update();
  frame_end();

  return 0;
}

int ui_end(void)
{
  int error;

  //error = qtopiakeyboard_end();
  //if(error) return error;

  error = uidisplay_end();
  if(error) return error;

  return 0;
}

#endif				/* #ifdef UI_QTOPIA */
