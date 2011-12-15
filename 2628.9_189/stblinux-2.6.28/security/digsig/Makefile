

digsig-lib-objs := ./gnupg/mpi/generic/mpih-lshift.o \
	./gnupg/mpi/generic/mpih-mul1.o ./gnupg/mpi/generic/mpih-mul2.o \
	./gnupg/mpi/generic/mpih-mul3.o ./gnupg/mpi/generic/mpih-rshift.o \
	./gnupg/mpi/generic/mpih-sub1.o ./gnupg/mpi/generic/udiv-w-sdiv.o \
	./gnupg/mpi/generic/mpih-add1.o ./gnupg/mpi/mpicoder.o ./gnupg/mpi/mpi-add.o \
	./gnupg/mpi/mpi-bit.o ./gnupg/mpi/mpi-div.o ./gnupg/mpi/mpi-cmp.o ./gnupg/mpi/mpi-gcd.o \
	./gnupg/mpi/mpih-cmp.o ./gnupg/mpi/mpih-div.o ./gnupg/mpi/mpih-mul.o ./gnupg/mpi/mpi-inline.o \
	./gnupg/mpi/mpi-inv.o ./gnupg/mpi/mpi-mpow.o ./gnupg/mpi/mpi-mul.o ./gnupg/mpi/mpi-pow.o \
	./gnupg/mpi/mpi-scan.o ./gnupg/mpi/mpiutil.o ./gnupg/cipher/rsa-verify.o

EXTRA_CFLAGS += -DDIGSIG_REVOCATION  

ifdef CONFIG_SECURITY_DIGSIG_DEBUG
EXTRA_CFLAGS += -DDIGSIG_DEBUG -DDIGSIG_LOG 
endif

digsig_verif-objs := digsig.o digsig_cache.o digsig_revocation.o dsi_sig_verify.o 	

digsig_verif-objs += $(digsig-lib-objs)

ifeq ($(CONFIG_SECURITY_DIGSIG),m)
digsig_verif-objs +=  dsi_sysfs.o
endif

obj-$(CONFIG_SECURITY_DIGSIG) := digsig_verif.o


clean:
	@find . \
	\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' -o -name '*~'\
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
	-type f -print | xargs rm -f

tags:
	rm -f TAGS
	@find . -name \*.[ch] | xargs etags -a
