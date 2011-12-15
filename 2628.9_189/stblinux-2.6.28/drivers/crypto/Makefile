obj-$(CONFIG_CRYPTO_DEV_PADLOCK_AES) += padlock-aes.o
obj-$(CONFIG_CRYPTO_DEV_PADLOCK_SHA) += padlock-sha.o
obj-$(CONFIG_CRYPTO_DEV_GEODE) += geode-aes.o
obj-$(CONFIG_CRYPTO_DEV_HIFN_795X) += hifn_795x.o
obj-$(CONFIG_CRYPTO_DEV_TALITOS) += talitos.o
obj-$(CONFIG_CRYPTO_DEV_IXP4XX) += ixp4xx_crypto.o

subdir-$(CONFIG_CRYPTO_BRCM)	+= brcm_crypt

ifeq ($(CONFIG_CRYPTO_BRCM),y)
obj-$(CONFIG_CRYPTO_BRCM)	+= brcm_crypt/built-in.o
endif
