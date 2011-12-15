#!/bin/sh
# Install rtai/ltt
# Klaas Gadeyne 
# TODO : A lot of things

KERNELVERSION=2.4.16
LTTVERSION=0.9.5pre6
RTAIVERSION=24.1.8
LTTRTAIVERSION=24.1.8
LTTKERNELVERSION=2.4.16
LTTPATCHVERSION=020317-1.14
RTHALVERSION=rthal5f
OLDCONFIGKERNELVERSION=2.4.17 #Where can we find and .config file to
			      #ease the kernel configuration proces
cd /usr/src

# Get necessary source (wget can use both the ftp and http protocol...)
test -e linux-${KERNELVERSION}.tar.bz2 || wget ftp://ftp.be.kernel.org/pub/linux/kernel/v2.4/linux-${KERNELVERSION}.tar.bz2
test -e TraceToolkit-${LTTVERSION}.tgz || wget ftp://ftp.opersys.com/pub/LTT/TraceToolkit-${LTTVERSION}.tgz
test -e rtai-${RTAIVERSION}.tgz || wget http://www.aero.polimi.it/RTAI/rtai-${RTAIVERSION}.tgz

rm linux
rm -rf rtai-${RTAIVERSION}-trace
rm -rf linux-${KERNELVERSION}-rthal-trace

#  unzip everything, if you don't like the verbosity, disable it...
tar xjvf linux-${KERNELVERSION}.tar.bz2
tar xzvf TraceToolkit-${LTTVERSION}.tgz
tar xzvf rtai-${RTAIVERSION}.tgz

mv linux linux-${KERNELVERSION}-rthal-trace
ln -s linux-${KERNELVERSION}-rthal-trace linux
cd linux
patch -p1 < ../TraceToolkit-${LTTVERSION}/Patches/patch-ltt-linux-${LTTKERNELVERSION}-${RTHALVERSION}-${LTTPATCHVERSION}

# No rejects in this one :)

# Is this still distribution specific (kernel-source is debian specific)?  You should at least respect
# some naming conventions, ic. linux-$VERSION-NUMBER...
if test -e ../kernel-source-${OLDCONFIGKERNELVERSION}/.config
then
    cp ../kernel-source-${OLDCONFIGKERNELVERSION}/.config .
    make oldconfig
else if test -e ../linux-${OLDCONFIGKERNELVERSION}/.config
    then
    cp ../linux-${OLDCONFIGKERNELVERSION}/.config .
    make oldconfig
    fi
fi

# Answer Yes to the RTHAL question
# Answer m to the tracer module question
# Answer y to CONFIG_RTAI_TRACE support
# And if necessary some other questions

echo "disable APM/APCI and the CONFIG_MODVERSIONS options"

# Forgot how to put a "wait 5 seconds" loop in the bash script

make menuconfig

# disable APM/ACPI and the CONFIG_MODVERSIONS option!!
# This is critical, unless you _want_ to obtain unresolved symbols :-) 
# You should do a full configuration here if you didn't have an old
# config file

# Because of the LTT patch, you cannot immediately compile your
# kernel.  However, you must set the dependencies first...

make dep && make clean

######################################################################

# Now RTAI support

cd /usr/src
mv rtai-${RTAIVERSION} rtai-${RTAIVERSION}-trace
rm rtai
ln -s rtai-${RTAIVERSION}-trace rtai
cd rtai
patch -p1 < ../TraceToolkit-${LTTVERSION}/Patches/patch-ltt-rtai-${LTTRTAIVERSION}-${LTTPATCHVERSION}

# No more rejects here too :-)

make menuconfig

# As far as I remember, ltt has no support for smp yet.  Correct me if
# I'm wrong!

make dep && make && make install

# Make if necessary the necessary nods...
# Doesn't do any harm to do this twice I suppose, errors are ignored anyway
make dev


######################################################################

# Now we proceed with the kernel compilation

cd /usr/src/linux
make clean && make bzImage && make modules
cp arch/i386/boot/bzImage /boot/vmlinuz-${KERNELVERSION}-rthal5-TRACE
cp System.map /boot/System.map-${KERNELVERSION}-rthal5-TRACE

# To avoid problems with the modules_install, we move the rtaisyms
# file out of the way...
# Doesn't seem neccesary anymore.
# 
# mv /lib/modules/${KERNELVERSION}-rthal5-TRACE/rtaisyms /root/

make modules_install

# 
# I _do_ still get some unresolved symbols though.  Should check this
# out
# depmod: *** Unresolved symbols in
# # /lib/modules/2.4.16-rthal5-TRACE/rtai/aio_misc.o
# depmod:         aio_idle_time
# depmod:         lxk_create_thread
# depmod:         aio_num_requests
# depmod:         rt_create_thread
# depmod:         aio_threads
# make: *** [_modinst_post] Error 1
# 

# If everything went fine we can now reboot... Don't mind all the
# unresolved symbols, they're not true :-)

echo "If you're using grub, you can now safely reboot, and try your new kernel"
echo "Else first edit /etc/lilo.conf and run lilo"

######################################################################

# Installing the Tracetoolkit, version ${LTTVERSION}

cd /usr/src/TraceToolkit-${LTTVERSION}/
./configure

# You need to have libgtk1.2-dev installed to be able to compile
# (not another version, ic 1.3 doesn't work)
# If necessary (and if debian :)
# apt-get install libgtk-dev 

# again, if necessary
# Manually add some links to avoid errors during compile time
# ln -s /usr/include/gtk-1.2/gtk /usr/include/gtk
# ln -s /usr/include/gtk-1.2/gdk /usr/include/gdk
# ln -s /usr/include/pango-1.0/ /usr/include/pango

# Try to make
make

# No more compile errors

make install

######################################################################
