include $(ROOTDIR)/config.arch

TOOL_DIR_PREFIX=$(shell dirname $(shell which $(CC)))/..

LIBSTDC=$(shell basename $(TOOL_DIR_PREFIX)/$(TARGET)-uclibc/lib/libstdc++.so.*.*)
LIBSTDC1=$(shell basename $(TOOL_DIR_PREFIX)/$(TARGET)-uclibc/lib/libstdc++.so.*)


ifneq (,$(findstring linux,$(CROSS)))
	TARGET=$(CROSS:linux-=linux)
	TARGET_ARCH ?= $(CROSS:-linux-=)
else
	ifneq (,$(findstring uclibc,$(CROSS)))
		TARGET_ARCH ?= $(CROSS:-uclibc-=)
		TARGET=$(TARGET_ARCH)-linux
	else
		echo "FIXME: Cannot determine target architecture"
		exit 99
	endif
endif


all:
	echo "libc++: making all"

clean:

distclean: clean

romfs:
	echo "libc++: making romfs"
	$(ROMFSINST) $(TOOL_DIR_PREFIX)/$(TARGET)-uclibc/lib/$(LIBSTDC) /lib/$(LIBSTDC)
	$(ROMFSINST) -s ./$(LIBSTDC) /lib/$(LIBSTDC1)
	$(ROMFSINST) -s ./$(LIBSTDC) /lib/libstdc++.so
	$(ROMFSINST) $(TOOL_DIR_PREFIX)/$(TARGET)-uclibc/lib/libgcc_s.so.1 /lib/libgcc_s.so.1
	$(ROMFSINST) -s ./libgcc_s.so.1 /lib/libgcc_s.so
