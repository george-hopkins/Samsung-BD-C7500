/* Definition of locale datatype.
   Copyright (C) 1997,2000,02 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _XLOCALE_H
#define _XLOCALE_H	1

#include <features.h>

#ifndef __UCLIBC_HAS_XLOCALE__
#error Attempted to include xlocale.h when uClibc built without extended locale support.
#endif /* __UCLIBC_HAS_XLOCALE__ */

#include <bits/uClibc_locale.h>
/* #include <bits/uClibc_touplow.h> */

#if 0
/* Structure for reentrant locale using functions.  This is an
   (almost) opaque type for the user level programs.  The file and
   this data structure is not standardized.  Don't rely on it.  It can
   go away without warning.  */
typedef struct __locale_struct
{
#if 0
  /* Note: LC_ALL is not a valid index into this array.  */
  struct locale_data *__locales[13]; /* 13 = __LC_LAST. */
#endif

  /* To increase the speed of this solution we add some special members.  */
/*   const unsigned short int *__ctype_b; */
/*   const int *__ctype_tolower; */
/*   const int *__ctype_toupper; */
  const __uint16_t *__ctype_b;
  const __ctype_touplow_t *__ctype_tolower;
  const __ctype_touplow_t *__ctype_toupper;

  __uclibc_locale_t *__locale_ptr;

#if 0
  /* Note: LC_ALL is not a valid index into this array.  */
  const char *__names[13];
#endif
} *__locale_t;
#endif

#endif /* xlocale.h */
