
# CFLAGS += -I../libc/include

#SRC = nand_utils.c crc32.c
#OBJS = nand_utils.o crc32.o

SRC = nand_utils.c
OBJS = nand_utils.o

LIBNANDUTILS = libnandutils.a
LIBNANDUTILS_SO = libnandutils.so

CFLAGS += -fPIC

all: $(LIBNANDUTILS)

$(LIBNANDUTILS): $(LIBNANDUTILS)($(OBJS))
	$(RANLIB) $(LIBNANDUTILS)

$(LIBNANDUTILS)($(OBJS)): $(SRC)
	$(CC) $(CFLAGS) -DL_$(basename $*) -o $(basename $*).o -c $^
	$(CC) -shared -o $(LIBNANDUTILS_SO) -Wl,--whole-archive $(OBJS) -Wl,--no-whole-archive
	$(AR) rv $(LIBNANDUTILS) $(basename $*).o
romfs:
	$(ROMFSINST) $(LIBNANDUTILS) /usr/lib/$(LIBNANDUTILS)
	$(ROMFSINST) $(LIBNANDUTILS_SO) /usr/lib/$(LIBNANDUTILS_SO)
clean:
	-rm -f $(LIBNANDUTILS) $(LIBNANDUTILS_SO) *.o

