#include "config.h"

#ifdef UI_QTOPIA 		/* Use this if we're using qtopia */

#include "specyscreen.h"
#include "keyboard.h"

#include <qpainter.h>
#include <qimage.h>
#include <stdio.h>
#include <stdlib.h>


specyScreen::specyScreen( QWidget *parent, const char *name )
		: QWidget( parent, name )
{
}

void specyScreen::create( int width, int height )
{
	// create my speccy screen
	image = new QImage();
	image->create(width,height, 32, 0,  QImage::LittleEndian );

	//build an array of RGB values for my speccy colours
	switch (image->depth())
	{
/*	case 8:
		colours[0]  = qRgb();
		colours[1]  = qRgb();
		colours[2]  = qRgb();
		colours[3]  = qRgb();
		colours[4]  = qRgb();
		colours[5]  = qRgb();
		colours[6]  = qRgb();
		colours[7]  = qRgb();
		colours[8]  = qRgb();
		colours[9]  = qRgb();
		colours[10] = qRgb();
		colours[11] = qRgb();
		colours[12] = qRgb();
		colours[13] = qRgb();
		colours[14] = qRgb();
		colours[15] = qRgb();
		break;
*/
	case 16:
		colours[0]  = qRgb(   0,   0,   0 );	//black
		colours[1]  = qRgb(   0,   0, 192 );	//blue
		colours[2]  = qRgb( 192,   0,   0 );	//red
		colours[3]  = qRgb( 255, 128, 255 );	//magenta
		colours[4]  = qRgb(   0, 192,   0 );	//green
		colours[5]  = qRgb(  64, 192, 255 );	//cyan
		colours[6]  = qRgb( 255, 223,  64 );	//yellow
		colours[7]  = qRgb( 192, 192, 192 );	//white
		colours[8]  = qRgb(   0,   0,   0 );	//black
		colours[9]  = qRgb(   0,   0, 255 );	//blue
		colours[10] = qRgb( 255,   0,   0 );	//red
		colours[11] = qRgb( 255,   0, 255 );	//magenta
		colours[12] = qRgb(   0, 255,   0 );	//green
		colours[13] = qRgb(  64, 255, 255 );	//cyan
		colours[14] = qRgb( 255, 255,   0 );	//yellow
		colours[15] = qRgb( 255, 255, 255 );	//white
		break;

	case 32:
		colours[0]  = qRgb(   0,   0,   0 );	//black
		colours[1]  = qRgb(   0,   0, 192 );	//blue
		colours[2]  = qRgb( 192,   0,   0 );	//red
		colours[3]  = qRgb( 255, 128, 255 );	//magenta
		colours[4]  = qRgb(   0, 192,   0 );	//green
		colours[5]  = qRgb(  64, 192, 255 );	//cyan
		colours[6]  = qRgb( 255, 223,  64 );	//yellow
		colours[7]  = qRgb( 192, 192, 192 );	//white
		colours[8]  = qRgb(   0,   0,   0 );	//black
		colours[9]  = qRgb(   0,   0, 255 );	//blue
		colours[10] = qRgb( 255,   0,   0 );	//red
		colours[11] = qRgb( 255,   0, 255 );	//magenta
		colours[12] = qRgb(   0, 255,   0 );	//green
		colours[13] = qRgb(  64, 255, 255 );	//cyan
		colours[14] = qRgb( 255, 255,   0 );	//yellow
		colours[15] = qRgb( 255, 255, 255 );	//white
		break;

	case 15:
	case 24:
	default:
		printf("i don't know what to do with %d bpp mode",
               image->depth());
        abort();
		break;
	}

}

void specyScreen::paint()
{
	QPainter painter( this );
	painter.drawImage( 5, 5, *image );
}

void specyScreen::paintEvent( QPaintEvent * )
{
	QPainter painter( this );
	painter.drawImage( 5, 5, *image );
}


void specyScreen::keyPressEvent( QKeyEvent *key )
{
	keyboard_press( keyboard_key_name(key->ascii()) );
}

void specyScreen::keyReleaseEvent( QKeyEvent *key )
{
	keyboard_release( keyboard_key_name(key->ascii()) );
}

void specyScreen::putPixel( int x, int y, int colour )
{
	image->setPixel( x, y, colours[colour] );
}


#endif				/* #ifdef UI_QTOPIA */
