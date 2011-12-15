romfs:
ifneq (,$(findstring mipsel,$(CROSS)))
	$(ROMFSINST) -e CONFIG_USER_WIRELESS_SUPPORT  le/applications/wl /bin/wl
	$(ROMFSINST) -e CONFIG_USER_WIRELESS_SUPPORT  le/drivers/4320usb/usb-cdc.o /lib/modules/usb-cdc.o
else
	$(ROMFSINST) -e CONFIG_USER_WIRELESS_SUPPORT  be/applications/wl /bin/wl
	$(ROMFSINST) -e CONFIG_USER_WIRELESS_SUPPORT  be/drivers/4320usb/usb-cdc.o /lib/modules/usb-cdc.o
endif

clean:
