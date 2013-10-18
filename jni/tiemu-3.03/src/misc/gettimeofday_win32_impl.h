/* Copied from kdewin32
   Copyright (C) 2003-2005 Jaroslaw Staniek <js@iidea.pl>
   Copyright (C) 2005 Christian Ehrlicher <Ch.Ehrlicher@gmx.de>
   Copyright (C) 2006 Ralf Habacker <ralf.habacker@freenet.de>

   Modified for TiEmu
   Copyright (C) 2007 Kevin Kofler

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <winsock2.h> /* struct timeval */
#include <windows.h>

/* support ancient M$VC versions -- Kevin Kofler */
#ifdef _MSC_VER
#define GETTIMEOFDAY_INT64 __int64
#define KDE_SECONDS_SINCE_1601  11644473600I64
#define KDE_USEC_IN_SEC         1000000I64
#else
#define GETTIMEOFDAY_INT64 long long
#define KDE_SECONDS_SINCE_1601  11644473600LL
#define KDE_USEC_IN_SEC         1000000LL
#endif

int gettimeofday(struct timeval *__p, void *__t)
{
        union {
                unsigned GETTIMEOFDAY_INT64 ns100; /*time since 1 Jan 1601 in 100ns units */
                FILETIME ft;
        } now;
        
        GetSystemTimeAsFileTime (&now.ft);
        __p->tv_usec = (long) ((now.ns100 / (GETTIMEOFDAY_INT64)10) % KDE_USEC_IN_SEC);
        __p->tv_sec  = (long)(((now.ns100 / (GETTIMEOFDAY_INT64)10 ) / KDE_USEC_IN_SEC) - KDE_SECONDS_SINCE_1601);
        
        return (0); 
}
