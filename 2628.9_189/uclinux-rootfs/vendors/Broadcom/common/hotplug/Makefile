
EXEC = mount.hotplug
OBJS = mount_hotplug.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs: all
	[ -d $(ROMFSDIR)/etc/hotplug.d ] || mkdir -p $(ROMFSDIR)/etc/hotplug.d
	[ -d $(ROMFSDIR)/etc/hotplug.d/default ] || mkdir -p $(ROMFSDIR)/etc/hotplug.d/default
	[ -d $(ROMFSDIR)/etc/hotplug.d/usb ] || mkdir -p $(ROMFSDIR)/etc/hotplug.d/usb
	[ -d $(ROMFSDIR)/etc/hotplug.d/block ] || mkdir -p $(ROMFSDIR)/etc/hotplug.d/block
	$(ROMFSINST) hotplug /sbin/hotplug
	$(ROMFSINST) /etc/hotplug.d/block/$(EXEC)
	$(ROMFSINST) /etc/hotplug.d/usb/brcm_wifi.hotplug

clean:
	-rm -f $(EXEC) *.gdb *.elf *.o

