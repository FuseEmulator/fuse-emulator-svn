
#include <config.h>

#ifdef UI_QTOPIA 		/* Use this if we're using qtopia */

#include <stdio.h>

#include <qpeapplication.h>
#include <qwidget.h>
#include <qimage.h>
#include <qpainter.h>

#include "fuse.h"
#include "display.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

#include "specyscreen.h"

static char * argv[1] = { "qtopia-fuse" };
static int argc = 1;
static QPEApplication qtopia_app(argc, argv);
static specyScreen screen;

int uidisplay_init(int width, int height)
{
	printf("uidisplay_init\n");
	qtopia_app.setMainWidget( &screen );
	screen.create(width, height);
//	screen.showMaximised();
	screen.show();

	return 0;
}

void uidisplay_putpixel(int x,int y,int colour)
{
	screen.putPixel(x, y, colour);
}

void uidisplay_line(int y)
{
	// I don't see why we really need this - so i'm gonna try ignoreing it :-P
}

void uidisplay_lines( int start, int end )
{
	int y;

	for( y=start; y<=end; y++)
		uidisplay_line(y);

}

void uidisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
	int x;

	for( x = pixel_from; x <= pixel_to; x++ )
		uidisplay_putpixel(x, line, colour);

}

//this function is called at the end of each frame - it switches frames (i think)
int frame_end(void)
{
	printf("frame_end\n");

	screen.paint();
	qtopia_app.processEvents(); // TODO - try time limiting this if we drop frames

	return 0;
}

int uidisplay_end(void)
{

	printf( "uidisplay_end" );

	// if the mainwindow is still showing, hide it
	if( screen.isVisible() ) screen.hide();

	return 0;
}

#endif				/* #ifdef UI_QTOPIA */
