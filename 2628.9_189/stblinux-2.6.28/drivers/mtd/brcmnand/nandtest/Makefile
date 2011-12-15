# PKG needs to be exported in .bashrc 

TOOLCHAIN_DIR := /opt/toolchains/crosstools_linux-2.6.12.0_gcc-3.4.6_uclibc-0.9.28-20050817-20061009-19/bin

MY_PATH := $(PKG)/libsrc/common/lib

ARCH := mipsel-uclibc
CROSS := $(ARCH)-
CROSS_CFLAGS := -mabi=32 -mips32 -march=mips32 -mtune=mips32
CROSS_CXXFLAGS := $(CROSS_CFLAGS)
CROSS_STANDARD_INCLUDE := -I$(TOOLCHAIN_DIR)/include
CC := $(CROSS)gcc
LD := $(CROSS)ld


PATH := $(TOOLCHAIN_DIR)/bin:$(MY_PATH):${PATH}

default:
	@echo "please specify a target: all or clean"
	
SOURCES= \
	file_test.cpp
	

#BDVD_LIB := -lbdp -ldrm -lbdvd_crypt -lcairo -lcrypto -lfreetype -ljpeg -lpng12 -lxml2
#BDVD_LIB += -lbdp -lbdvd_drm -lcharset -ldirect -lfusion -lmng -lpng -lz
#BDVD_LIB += -lbdvd -lbdvd_png -lcommon -ldirectfb -liconv -lplayer -lsettop

ADDED_WARNINGS = -W -Wpointer-arith -Wall -Werror -Wno-unused -Wno-parentheses -Wno-format


CFLAGS = -g -DDEBUG -O2
CFLAGS += $(CROSS_CXXFLAGS) -DBSTD_CPU_ENDIAN=BSTD_ENDIAN_LITTLE -DBCHP_CHIP=7438 -DBCHP_VER=1 -Wall $(CROSS_STANDARD_INCLUDE)

CFLAGS+= -I../../../../../stage/usr/local/include
CFLAGS+= -I../../../../../stage/usr/local/include/settop 
CFLAGS+= -I../../../../../stage/usr/local/include/directfb
CFLAGS+= -I../../../../../stage/usr/local/include/common
CFLAGS+= -I../../../../../stage/usr/local/include/chip
CFLAGS+= -I../../../../../stage/usr/local/include/base
CFLAGS+= -I../../../../../stage/usr/local/include/libxml2
CFLAGS+= -I../../../../openssl/openssl-0.9.8/include

LDFLAGS += -W1 -L../../../../../stage/usr/local/lib $(BDVD_LIB) -lstdc++

TARGET= nand_flash_file_test
OBJECTS=$(SOURCES:.cpp=.o)

clean:
	rm -rf $(TARGET)
	rm -rf $(OBJECTS)
	rm -r *.o

all: $(TARGET)

install: DESTDIR all
	@echo "Installing $(TARGET) into $(DESTDIR)..."
	@mkdir -p $(DESTDIR)/lib
	@cp $(TARGET) $(DESTDIR)/lib/

$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) *.o -o $@
    
%.o: %.cpp
	@echo "Compiling $<..."
	$(CC)  $(CFLAGS) -c $<

