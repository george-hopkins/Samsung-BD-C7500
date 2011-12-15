dnl AC_LTT_CONFIG -- process LTT configuration options

AC_DEFUN(AC_LTT_CONFIG,
[
AC_MSG_CHECKING(debug mode enabled)
AC_ARG_ENABLE(debug,
	[  --enable-debug          Enable debug support],
	[CFLAGS="$CFLAGS -g -O2"; with_debug=yes],
	[CFLAGS="$CFLAGS -O2"; with_debug=no])
test x$with_debug = xyes && LTT_DEBUG=1 || LTT_DEBUG=0
AC_SUBST(LTT_DEBUG)
AM_CONDITIONAL(ENABLE_LTT_DEBUG,test ${LTT_DEBUG} = 1)
AC_MSG_RESULT($with_debug)

AC_MSG_CHECKING(RTAI support)
AC_ARG_WITH(rtai,
[  --with-rtai             RTAI support [default=yes]],
test "$withval" = no || with_rtai=yes, with_rtai=yes)
test x$with_rtai = xyes && SUPP_RTAI=1 || SUPP_RTAI=0
AC_SUBST(SUPP_RTAI)
AM_CONDITIONAL(WITH_SUPP_RTAI,test ${SUPP_RTAI} = 1)
AC_MSG_RESULT($with_rtai)

AC_MSG_CHECKING(GTK interface enabled)
AC_ARG_WITH(gtk,
[  --with-gtk              Use GTK interface [default=yes]],
test "$withval" = no || with_gtk=yes, with_gtk=yes)
test x$with_gtk = xyes && GTK_ENV=1 || GTK_ENV=0
AC_SUBST(GTK_ENV)
AM_CONDITIONAL(WITH_GTK_ENV,test ${GTK_ENV} = 1)
AC_MSG_RESULT($with_gtk)

AC_MSG_CHECKING(target native mode enabled)
AC_ARG_ENABLE(target-native,
	[  --enable-target-native  Enable target native mode],
	[use_native=yes],
	[use_native=no])
test x$use_native = xyes && TARGET_NATIVE=1 || TARGET_NATIVE=0
AC_SUBST(TARGET_NATIVE)
AM_CONDITIONAL(ENABLE_TARGET_NATIVE,test ${TARGET_NATIVE} = 1)
AC_MSG_RESULT($use_native)

AC_MSG_CHECKING(unpacked structs)
AC_ARG_ENABLE(unpacked-structs,
	[  --enable-unpacked-structs
                          Use unpacked structs],
	[use_unpacked=yes],
	[use_unpacked=no])
test x$use_unpacked = xyes && LTT_UNPACKED_STRUCTS=1 || LTT_UNPACKED_STRUCTS=0
AC_SUBST(LTT_UNPACKED_STRUCTS)
AM_CONDITIONAL(ENABLE_LTT_UNPACKED_STRUCTS,test ${LTT_UNPACKED_STRUCTS} = 1)
AC_MSG_RESULT($use_unpacked)
])
