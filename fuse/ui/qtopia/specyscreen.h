#ifndef SPECYSCREEN_H
#define SPECYSCREEN_H

#include <qwidget.h>


class specyScreen : public QWidget
{
	Q_OBJECT
public:
	specyScreen( QWidget *parent=0, const char *name=0 );
	void create( int width, int height );
	void putPixel( int x, int y, int colour );
	void paint();

protected:
	void paintEvent( QPaintEvent * );
	void keyPressEvent( QKeyEvent * );
	void keyReleaseEvent( QKeyEvent * );

private:
	QImage *image;
	int colours[16];

};

#endif // SPECYSCREEN_H

