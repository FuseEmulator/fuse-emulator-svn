/* xdisplay.c: Routines for dealing with the X display
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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <vga.h>
#include <vgakeyboard.h>

#include "display.h"
#include "x.h"
#include "xdisplay.h"
#include "skeyboard.h"

static char* progname;

static unsigned char *image;

static int colours[16];

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int xdisplay_current_size=1;

static int xdisplay_allocate_colours(int numColours, int *colours);
static int xdisplay_allocate_image(int width, int height);

void myhandler(int scancode, int press)  {
    if(press) xkeyboard_keypress(scancode); else
        xkeyboard_keyrelease(scancode);
}

int xdisplay_init(int width, int height)
{

  vga_init();
  vga_setmode(G320x240x256V);
  keyboard_init();
  keyboard_seteventhandler(myhandler);
  
  xdisplay_allocate_image(320, 240);
  xdisplay_allocate_colours(16, colours);  

  return 0;
}

static int xdisplay_allocate_colours(int numColours, int *colours)
{
  char *colour_names[] = {
    "black",
    "blue3",
    "red3",
    "magenta3",
    "green3",
    "cyan3",
    "yellow3",
    "gray80",
    "black",
    "blue",
    "red",
    "magenta",
    "green",
    "cyan",
    "yellow",
    "white",
  };
  int colour_palette[] = {
  0,0,0,
  0,0,128,
  128,0,0,
  128,0,128,
  0,128,0,
  0,128,128,
  128,128,0,
  128,128,128,
  0,0,0,
  0,0,255,
  255,0,0,
  255,0,255,
  0,255,0,
  0,255,255,
  255,255,0,
  255,255,255
  };
  
  int i;

  for(i=0;i<numColours;i++) {
    colours[i]=i;
    vga_setpalette(i,colour_palette[i*3]>>2,colour_palette[i*3+1]>>2,colour_palette[i*3+2]>>2);
  }

  return 0;
}
  
static int xdisplay_allocate_image(int width, int height)
{
  image=malloc(width*height);

  if(!image) {
    fprintf(stderr,"%s: couldn't create image\n",progname);
    return 1;
  }

  return 0;
}

int xdisplay_configure_notify(int width, int height)
{
  int y,size;

  size = width / DISPLAY_SCREEN_WIDTH;

  /* If we're the same size as before, nothing special needed */
  if( size == xdisplay_current_size ) return 0;

  /* Else set ourselves to the new height */
  xdisplay_current_size=size;

  /* Redraw the entire screen... */
  display_refresh_all();

  /* And the entire border */
  for(y=0;y<DISPLAY_BORDER_HEIGHT;y++) {
    xdisplay_set_border(y,0,DISPLAY_SCREEN_WIDTH,display_border);
    xdisplay_set_border(DISPLAY_BORDER_HEIGHT+DISPLAY_HEIGHT+y,0,
			DISPLAY_SCREEN_WIDTH,display_border);
  }

  for(y=DISPLAY_BORDER_HEIGHT;y<DISPLAY_BORDER_HEIGHT+DISPLAY_HEIGHT;y++) {
    xdisplay_set_border(y,0,DISPLAY_BORDER_WIDTH,display_border);
    xdisplay_set_border(y,DISPLAY_BORDER_WIDTH+DISPLAY_WIDTH,
			DISPLAY_SCREEN_WIDTH,display_border);
  }

  return 0;
}

void xdisplay_putpixel(int x,int y,int colour)
{
  *(image+x+y*320)=colour;
}

void xdisplay_line(int y)
{
    vga_drawscansegment(image+y*320,0,y,320);
}

void xdisplay_area(int x, int y, int width, int height)
{
    int yy;
    for(yy=y; yy<y+height; yy++)
        vga_drawscansegment(image+yy*320+x,x,yy,width);
}

void xdisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
    int x;
  
    for(x=pixel_from;x<pixel_to;x++)
        *(image+line*320+x)=colours[colour];
}

int xdisplay_end(void)
{
    vga_setmode(TEXT);
    keyboard_close();

    return 0;
}
