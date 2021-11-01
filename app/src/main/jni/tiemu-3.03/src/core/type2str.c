/* Hey EMACS -*- linux-c -*- */
/* $Id: type2str.c 2601 2007-07-14 08:49:30Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
    Type conversion routines
*/

#include <stdio.h>
#include <string.h>

#include "intl.h"
#include "ti68k_def.h"
#include "bkpts.h"

const char *ti68k_calctype_to_string(int type)
{
	switch(type)
	{
		case TI89:  return "TI89";
		case TI92:  return "TI92";
		case TI92p: return "TI92+";
		case V200:  return "V200PLT";
        case TI89t: return "TI89t";
		default:    return "none";
	}
}

int ti68k_string_to_calctype(const char *str)
{
	if(!strcmp(str, "TI89"))
		return TI89;
	else if(!strcmp(str, "TI92"))
		return TI92;
	else if(!strcmp(str, "TI92+"))
		return TI92p;
	else if(!strcmp(str, "V200PLT"))
		return V200;
    else if(!strcmp(str, "TI89t"))
		return TI89t;
       
	return 0;
}

const char *ti68k_romtype_to_string(int type)
{
	switch(type)
	{
		case 0:			return "EPROM";
		case FLASH_ROM:	return "FLASH";
	}
		
	return 0;
}

int         ti68k_string_to_romtype(const char *str)
{
	if(!strcmp(str, "EPROM"))
		return 0;
	else if(!strcmp(str, "FLASH"))
		return FLASH_ROM;
		
	return 0;
}

const char *ti68k_hwtype_to_string(int type)
{
	switch(type)
	{
		case HW1:   return "HW1";
		case HW2:   return "HW2";
        case HW3:   return "HW3";
		case HW4:   return "HW4";
		default:    return "none";
	}
}

int ti68k_string_to_hwtype(const char *str)
{
	if(!strcmp(str, "HW1"))
		return HW1;
	else if(!strcmp(str, "HW2"))
		return HW2;
    else if(!strcmp(str, "HW3"))
		return HW3;
	else if(!strcmp(str, "HW4"))
		return HW4;

	return 0;
}

const char *ti68k_exception_to_string(int number)
{
	switch(number)
	{
		case 0: return _("Initial SSP");
		case 1: return _("Initial PC");
		case 2: return _("Bus Error vector");
		case 3: return _("Address Error vector");
		case 4: return _("Illegal Instruction vector");
		case 5: return _("Zero Divide vector");
		case 6: return _("CHK Instruction vector");
		case 7: return _("TRAPV Instruction vector");
		case 8: return _("Privilege Violation vector");
		case 9: return _("Trace vector");
		case 10: return _("Line 1010 Emulator vectors");
		case 11: return _("Line 1111 Emulator vectors");
		case 12: return _("Unassigned, reserved");
		case 13: return _("Unassigned, reserved");
		case 14: return _("Unassigned, reserved");
		case 15: return _("Uninitialised Interrupt vector");
		case 16: return _("Unassigned, reserved");
		case 17: return _("Unassigned, reserved");
		case 18: return _("Unassigned, reserved");
		case 19: return _("Unassigned, reserved");
		case 20: return _("Unassigned, reserved");
		case 21: return _("Unassigned, reserved");
		case 22: return _("Unassigned, reserved");
		case 23: return _("Unassigned, reserved");
		case 24: return _("Spurious Interrupt vector");
		case 25: return _("Level 1 Interrupt auto-vectors");
		case 26: return _("Level 2 Interrupt auto-vectors");
		case 27: return _("Level 3 Interrupt auto-vectors");
		case 28: return _("Level 4 Interrupt auto-vectors");
		case 29: return _("Level 5 Interrupt auto-vectors");
		case 30: return _("Level 6 Interrupt auto-vectors");
		case 31: return _("Level 7 Interrupt auto-vectors");
		case 32: return _("TRAP #0 Instruction vectors");
		case 33: return _("TRAP #1 Instruction vectors");
		case 34: return _("TRAP #2 Instruction vectors");
		case 35: return _("TRAP #3 Instruction vectors");
		case 36: return _("TRAP #4 Instruction vectors");
		case 37: return _("TRAP #5 Instruction vectors");
		case 38: return _("TRAP #6 Instruction vectors");
		case 39: return _("TRAP #7 Instruction vectors");
		case 40: return _("TRAP #8 Instruction vectors");
		case 41: return _("TRAP #9 Instruction vectors");
		case 42: return _("TRAP #10 Instruction vectors");
		case 43: return _("TRAP #11 Instruction vectors");
		case 44: return _("TRAP #12 Instruction vectors");
		case 45: return _("TRAP #13 Instruction vectors");
		case 46: return _("TRAP #14 Instruction vectors");
		case 47: return _("TRAP #15 Instruction vectors");	
		case 48: 
		case 49: 
		case 50: 
		case 51: 
		case 52: 
		case 53: 
		case 54: 
		case 55: 
		case 56: 
		case 57: 
		case 58: 
		case 59: 
		case 60: 
		case 61: 
		case 62: 
		case 63: return _("Unassigned, reserved");
		case 64: return _("User Interrupt vectors");
		
		default: return _("User Interrupt vectors");
	}
}

const char *ti68k_bkpt_cause_to_string(int type)
{
	switch(type)
	{
	case BK_CAUSE_ACCESS:	return _("access");
	case BK_CAUSE_RANGE:	return _("access range");
	case BK_CAUSE_ADDRESS:	return _("address");
    case BK_CAUSE_EXCEPTION:return _("exception");
	case BK_CAUSE_PGMENTRY: return _("prgm entry");
	case BK_CAUSE_PROTECT:  return _("hw protection");
	case BK_CAUSE_BIT:		return _("bit change");
	default:				return _("unknown");
	}
}


const char *ti68k_bkpt_type_to_string(int type)
{
	switch(type)
	{
    case BK_TYPE_ACCESS:    return _("access");
    case BK_TYPE_RANGE:     return _("range");
    case BK_TYPE_CODE:      return _("code");
    case BK_TYPE_EXCEPTION: return _("exception");
	case BK_TYPE_PGMENTRY:	return _("prgm entry");
	case BK_TYPE_PROTECT:   return _("hw protection");
	case BK_TYPE_BIT:		return _("bit change");
	default:                return _("unknown");
	}
}

int ti68k_string_to_bkpt_type(const char *str)
{
	if(!strcmp(str, _("access")))
		return BK_TYPE_ACCESS;
	else if(!strcmp(str, _("range")))
		return BK_TYPE_RANGE;
	else if(!strcmp(str, _("code")))
		return BK_TYPE_CODE;
	else if(!strcmp(str, _("exception")))
		return BK_TYPE_EXCEPTION;
	else if(!strcmp(str, _("prgm entry")))
		return BK_TYPE_PGMENTRY;
	else if(!strcmp(str, _("hw protection")))
		return BK_TYPE_PROTECT;
	else if(!strcmp(str, _("bit change")))
		return BK_TYPE_BIT;

	return 0;
}

const char *ti68k_bkpt_mode_to_string(int type, int mode)
{
	// don't use type, it's implicit.
	if( (mode & BK_READ) && !(mode & BK_WRITE) )
	{
		if(mode & BK_BYTE)
			return "byte-read";
		else if(mode & BK_WORD)
			return "word-read";
		else if(mode & BK_LONG)
			return "long-read";
		else
			return "read";
	}
	else if( !(mode & BK_READ) && (mode & BK_WRITE) )
	{
		if(mode & BK_BYTE)
			return "byte-write";
		else if(mode & BK_WORD)
			return "word-write";
		else if(mode & BK_LONG)
			return "long-write";
		else
			return "write";
	}
	else if( (mode & BK_READ) && (mode & BK_WRITE) )
	{
		if(mode & BK_BYTE)
			return "r/w byte";
		else if(mode & BK_WORD)
			return "r/w word";
		else if(mode & BK_LONG)
			return "r/w long";
		else
			return "r/w";
	}

    return "unknown (bug)";
}

int ti68k_string_to_bkpt_mode(const char * str)
{
    if(!strcmp(str, "any") || !strcmp(str, "r/w"))
        return BK_READ | BK_WRITE;
    else if(!strcmp(str, "read"))
        return BK_READ;
    else if(!strcmp(str, "write"))
        return BK_WRITE;
    else if(!strcmp(str, "byte-read"))
        return BK_READ_BYTE;
    else if(!strcmp(str, "word-read"))
        return BK_READ_WORD;
    else if(!strcmp(str, "long-read"))
        return BK_READ_LONG;
    else if(!strcmp(str, "byte-write"))
        return BK_WRITE_BYTE;
    else if(!strcmp(str, "word-write"))
        return BK_WRITE_WORD;
    else if(!strcmp(str, "long-write"))
        return BK_WRITE_LONG;
	else if(!strcmp(str, "r/w byte"))
		return BK_RW_BYTE;
	else if(!strcmp(str, "r/w word"))
		return BK_RW_WORD;
	else if(!strcmp(str, "r/w long"))
		return BK_RW_LONG;
    else
        return 0;
}
