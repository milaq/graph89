/* Hey EMACS -*- linux-c -*- */
/* $Id: missing.c 1693 2005-08-25 14:29:36Z roms $ */

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Various stuff missing in some OSes.
  *
  * Copyright 1997 Bernd Schmidt
  */

#include <stdlib.h>
#include <string.h>
#include "sysdeps.h"

#ifndef HAVE_STRDUP

//char *my_strdup (const char *s)
//{
//    /* The casts to char * are there to shut up the compiler on HPUX */
//    char *x = (char*)xmalloc(strlen((char *)s) + 1);
//    strcpy(x, (char *)s);
//    return x;
//}

#endif
