
Source Code
========
 * original(LGPL) version : libgphoto2-2.3.1.tar.gz
 * modified version(used in the Samsung Product): libptp.tar.gz


Building
========

* Building libgphoto2 for the original version:
    * Unpack the libgphoto2 zipball and cd into it.
    * "chmod a+x configure", in case libgphoto2 came shipped
      as .zip and thus lost file permissions.
    * With emdebian,
      "PKG_CONFIG_PATH=/tmp/samsung-inst/lib/pkgconfig/ ./configure --host=arm-linux-gnueabi --build=x86_64-unknown-linux-gnu --prefix=/tmp/samsung-inst--without-libusb"
      With MontaVista
      "PKG_CONFIG_PATH=/tmp/samsung-inst/lib/pkgconfig/ ./configure --host=armv5tl-montavista-linux-gnueabi --build=x86_64-unknown-linux-gnu --prefix=/tmp/samsung-inst--without-libusb"
      where "x86_64-unknown-linux-gnu" is replaced with your host type,
      maybe "i686-pc-linux-gnu".
    * Run "make".
    * Run "make install".

* Building libgphoto2 for the modified version:
    * This software distribution consists of LGPL components used in the Samsung Product.
    * Unpack the libptp zipball and cd into it.
    * With emdebian,
	Run "make -f Makefile".
     * libptp.so used in the Samsung Product.

