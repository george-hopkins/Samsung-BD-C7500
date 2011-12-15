

brcm_crypt-objs := crypt.o

obj-$(CONFIG_CRYPTO_BRCM) := brcm_crypt.o

clean: 
	@find . \
	\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~'\
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
	-type f -print | xargs rm -f

tags:
	rm -f TAGS
	@find . -name \*.[ch] | xargs etags -a


#EXTRA_CFLAGS += -g

