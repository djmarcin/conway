CC = g++
DEBUG = -g
CFLAGS = -std=c++11 -Wall -Wno-deprecated -c -O2 $(DEBUG)
LFLAGS = -std=c++11 -Wall $(DEBUG)

OBJS = life.o driver.o

life : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o life -framework GLUT -framework OpenGL

life.o : life.h life.cc
	$(CC) $(CFLAGS) life.cc

driver.o : driver.cc life.h
	$(CC) $(CFLAGS) driver.cc

clean:
	rm -f *.o *~ life
