/******************************************************************************/
/*                                                                            */
/*               Misc  (C) by Gerhard W. Gruber in Vienna 2003                */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * PROJECT: Misc definitions
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.2  2005/01/07 02:01:10  sparhawk
 * Lightgem updates
 *
 * Revision 1.1  2004/10/30 17:06:36  sparhawk
 * DarkMod added to project.
 *
 * Revision 1.3  2003/11/09 10:48:36  lightweave
 * Fixes to make linux version work. In Detail:
 * .) Added a window in linux.cpp which is aligned at the specified border.
 *
 * .) Added a timer that keeps the winod on top of all others (linux only).
 *
 * .) Changed ButtonUp to ButtonDown message. This will work nicer on linux and
 *    doesn't effect the Windows version.
 *
 * Revision 1.2  2003/10/04 22:01:14  lightweave
 * Modifications for compiling the sources under Linux.
 *
 * Revision 1.1.1.1  2003/10/04 17:13:12  lightweave
 * Initial Release to CVS.
 *
 *
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _MISC_H
#define _MISC_H

#ifndef LONG
#define LONG signed long
#endif

#ifndef ULONG
#define ULONG unsigned long
#endif

#ifndef WORD
#define WORD signed short
#endif

#ifndef UWORD
#define UWORD unsigned short
#endif

#ifndef UBYTE
#define UBYTE unsigned char
#endif

#ifndef BYTE
#define BYTE signed char
#endif

#undef BOOL
#ifndef BOOL
#ifdef __cplusplus
#define BOOL bool
#else
#define BOOL int
#endif
#endif

#ifndef TRUE
#ifdef __cplusplus
#define TRUE true
#else
#define TRUE ((int)1)
#endif
#endif

#ifndef FALSE
#ifdef __cplusplus
#define FALSE false
#else
#define FALSE ((int)0)
#endif
#endif

#ifndef NULL
#define NULL 0L
#endif

#define PTR_SIZE	sizeof(void *)

#ifdef _LINUX_
#define stricmp     strcasecmp
#define chsize      ftruncate
#endif


LONG FindChar(UBYTE *s, UBYTE *p, BOOL Case, BOOL Find, UBYTE SkipChar);
UBYTE *Strip(UBYTE *s, int Start);
UBYTE *StrStrip(UBYTE *s, int Start, char *p);

#endif // _MISC_H
