INCLUDEDIR = .
ZLIBDIR=$(ROOTDIR)/lib/zlib

# THT: We only build the Host side tool
BUILD_CC := gcc

BUILD_CFLAGS := -I$(INCLUDEDIR) -I$(ZLIBDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -O2
OBJECTS = mksquashfs.o read_fs.o sort.o

BUILD_OBJECTS = $(patsubst %,build/%,$(OBJECTS))

TARGETS=mksquashfs
BUILD_TARGETS=$(patsubst %,build/%,$(TARGETS))

# all: mksquashfs unsquashfs
all: build $(BUILD_TARGETS)

build/mksquashfs: $(BUILD_OBJECTS)
	$(BUILD_CC)  $(BUILD_CFLAGS) $(BUILD_OBJECTS) -lz -lpthread -lm -o $@


.PHONY:
build:
	[ -d build ] || mkdir build

mksquashfs.o: mksquashfs.c squashfs_fs.h mksquashfs.h global.h sort.h

read_fs.o: read_fs.c squashfs_fs.h read_fs.h global.h

sort.o: sort.c squashfs_fs.h global.h sort.h

unsquashfs: unsquashfs.o
	$(CC) unsquashfs.o -lz -lpthread -lm -o $@

unsquashfs.o: unsquashfs.c squashfs_fs.h read_fs.h global.h

build/%.o: %.c
	$(BUILD_CC) $(BUILD_CFLAGS) -c -o $@ $<

build/%: %.c
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $<

# Nothing for romfs, this is strictly host side
romfs:

clean:
	$(RM) -rf *.o build
