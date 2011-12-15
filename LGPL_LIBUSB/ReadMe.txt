
Source Code
========
 * original(LGPL) version : libusb-0.1.12.tar.gz
 * modified version(used in the Samsung television): libusb.tar.gz
 

 Building
========

* Building libusb for the original version:
    * Unpack the libusb tarball and cd into it.
    * mkdir /tmp/samsung-inst
    * If you are using the emdebian toolchain,
	"./configure --host=arm-linux-gnueabi --build=x86_64-unknown-linux-gnu --prefix=/tmp/samsung-inst".
      With the MontaVista toolchain, use
	"./configure --host= --build=x86_64-unknown-linux-gnu --prefix=/tmp/samsung-inst"
      where "x86_64-unknown-linux-gnu" is replaced with your host type,
      maybe "i686-pc-linux-gnu".
    * Run "make".
    * Run "make install".  This will put libusb where it is accessible by libgphoto2.

* Building libusb for the modified version:
    * This software distribution consists of LGPL components used in the Samsung television
    * Unpack the libptp zipball and cd into it.
    * With emdebian,
	Run "make -f Makefile".
    * libusb.so used in the Samsung television.
    