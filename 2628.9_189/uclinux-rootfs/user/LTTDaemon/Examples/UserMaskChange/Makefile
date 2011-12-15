# Definitions
CC     = gcc
CFLAGS = -O2 -I../../Include
LFLAGS = -lusertrace

# Files
INCLUDES =
OBJS = maskchange.o

# Rules
.c.o: ${INCLUDES}
	$(CC) $(CFLAGS) -c $<

all: ${OBJS}
	$(CC) -o maskchange maskchange.o $(LFLAGS)

clean: 
	rm -rf *.o maskchange core

distclean:
	rm -rf *~
	rm -rf *.o maskchange core
