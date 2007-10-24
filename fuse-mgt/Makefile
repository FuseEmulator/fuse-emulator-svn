CFLAGS=-g -Wall `pkg-config --cflags fuse` -DFUSE_USE_VERSION=25
LDFLAGS=`pkg-config --libs fuse`

objects := fuse-mgt

all: $(objects)

clean:
	rm -f *.o $(objects)
