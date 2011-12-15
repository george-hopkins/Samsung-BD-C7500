obj-m	:= custom1.o custom2.o

KDIR	:= /usr/local/src/linux
PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.o *.ko *~ core *mod.c *mod.o .*.cmd