# Definitions
CC     = mipsel-linux-gcc
CFLAGS = -O2 -I../../Include
LFLAGS = -lusertrace

# Files
INCLUDES =
OBJS = custom1.o custom2.o

# Rules
.c.o: ${INCLUDES}
	$(CC) $(CFLAGS) -c $<

all: ${OBJS}
	$(CC) -o custom1 custom1.o $(LFLAGS)
	$(CC) -o custom2 custom2.o $(LFLAGS)

clean: 
	rm -rf *.o custom1 custom2 core

distclean:
	rm -rf *~
	rm -rf *.o custom1 custom2 core
