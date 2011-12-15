#!/bin/sh
if [ -d autoconf_links ]; then
  rm -Rf autoconf_links
fi
mkdir autoconf_links
cd autoconf_links
ln -sf /usr/bin/autoscan-2.53 autoscan
ln -sf /usr/bin/autom4te-2.53 autom4te
ln -sf /usr/bin/autoreconf-2.53 autoreconf
ln -sf /usr/bin/autoconf-2.53 autoconf
ln -sf /usr/bin/autoheader-2.53 autoheader
ln -sf /usr/bin/autoupdate-2.53 autoupdate
cd ..
