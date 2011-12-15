#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=DirectFB-examples
TEST_TYPE=-f
FILE=src/df_andi.c

DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PROJECT."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake installed to compile $PROJECT."
	echo "Get ftp://ftp.cygnus.com/pub/home/tromey/automake-1.2d.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

check_version() {
: <<inline_doc
      Tests for a minimum version level. Compares to version numbers and forces an exit if minimum level not met.
      NOTE: This test will fail on versions containing alpha chars.
    
    usage:     check_version "2.6.2" "1.1.1"

    input vars: $1=current version
                $2=min acceptable version
    externals:  --
    modifies:   --
    returns:    nothing
    on error:  write text to console and dies
    on success: write text to console and returns
inline_doc

  declare -i major minor revision change
  declare -i ref_major ref_minor ref_revision ref_change

  ref_version=$2
  tst_version=$1

  if  echo $ref_version | grep [[:alpha:]] 2>&1 >/dev/null || 
      echo $tst_version | grep [[:alpha:]] 2>&1 >/dev/null ;then
    echo "Cannot test for text, 0.0.0a, version types, assuming 'success' "
    DIE=1
    return    
  fi
    
  write_error_and_die() {
     echo -e "\n\t\tversion -->${tst_version}<-- is too old.
                   ${ref_version} or greater is required\n"
     DIE=1
     exit 1
  }
  
  IFS=".-("   # Split up w.x.y.z as well as w.x.y-rc  (catch release candidates)
  set -- $ref_version # set postional parameters to minimum ver values
  ref_major=$1; ref_minor=$2; ref_revision=$3
  #
  set -- $tst_version # Set postional parameters to test version values
  major=$1; minor=$2; revision=$3
  #
  IFS=""
  # Compare against minimum acceptable version..
  (( major > ref_major )) && echo " ..OK" && return
  (( major < ref_major )) && write_error_and_die
    # major=ref_major
  (( minor < ref_minor )) && write_error_and_die
  (( minor > ref_minor )) && echo " ..OK" && return
    # minor=ref_minor
  (( revision >= ref_revision )) && echo " ..OK" && return

  # oops.. write error msg and die
  write_error_and_die
}

echo "I am testing that you have the required versions of autoconf, automake."
echo

echo "Testing autoconf... "
VER=`autoconf --version | grep -iw autoconf | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
echo "Found $VER"
check_version "$VER" 2.13

echo "Testing automake... "
VER=`automake --version | grep automake | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
echo "Found $VER"
check_version "$VER" 1.4

echo

if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

case $CC in
*xlc | *xlc\ * | *lcc | *lcc\ *) am_opt=--include-deps;;
esac

if test -z "$ACLOCAL_FLAGS"; then

        acdir=`aclocal --print-ac-dir`
        m4list="pkg.m4"

        for file in $m4list
        do
                if [ ! -f "$acdir/$file" ]; then
                        echo "WARNING: aclocal's directory is $acdir, but..."
                        echo "         no file $acdir/$file"
                        echo "         You may see fatal macro warnings below."
                        echo "         If these files are installed in /some/dir, set the ACLOCAL_FLAGS "
                        echo "         environment variable to \"-I /some/dir\", or install"
                        echo "         $acdir/$file."
                        echo ""
                fi
        done
fi

autogen_dirs="."

for i in $autogen_dirs; do
	echo "Processing $i..."

	cd $i
	aclocal $ACLOCAL_FLAGS

	# optionally feature autoheader
	if grep AM_CONFIG_HEADER configure.in >/dev/null ; then
		(autoheader --version)  < /dev/null > /dev/null 2>&1 && autoheader
	fi

	automake --add-missing $am_opt
	autoconf
done

cd $ORIGDIR

$srcdir/configure --enable-maintainer-mode "$@"

echo 
echo "Now type 'make' to compile $PROJECT."
