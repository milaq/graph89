 /*
  * UAE - The Un*x Amiga Emulator
  *
  * memory management - reimplementation for TiEmu
  *
  *  TiEmu - Tiemu Is an EMUlator
  *
  *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
  *  Copyright (c) 2001-2003, Romain Lievin
  *  Copyright (c) 2003, Julien Blache
  *  Copyright (c) 2004, Romain Liévin
  *  Copyright (c) 2005, Romain Liévin, Kevin Kofler
  *  $Id: memory.h 2268 2006-11-06 17:18:51Z roms $
  */

#ifndef BUILD_UAE
#include "../ti_hw/mem.h"
#endif

//wrappers
#define get_byte(addr)	hw_get_byte(addr)
#define get_word(addr)	hw_get_word(addr)
#define get_long(addr)	hw_get_long(addr)

#define put_long(adr, arg)	hw_put_long(adr, arg)
#define put_word(adr, arg)	hw_put_word(adr, arg)
#define put_byte(adr, arg)	hw_put_byte(adr, arg)
 
#define get_real_address(addr)	hw_get_real_address(addr)
#define valid_address(addr, size)	1
