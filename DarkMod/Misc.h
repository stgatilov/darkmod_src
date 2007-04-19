/******************************************************************************/
/*                                                                            */
/*               Misc  (C) by Gerhard W. Gruber in Vienna 2003                */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

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

#ifdef __linux__
#define stricmp     strcasecmp
#define chsize      ftruncate
#endif


LONG FindChar(UBYTE *s, UBYTE *p, BOOL Case, BOOL Find, UBYTE SkipChar);
UBYTE *Strip(UBYTE *s, int Start);
UBYTE *StrStrip(UBYTE *s, int Start, char *p);

#endif // _MISC_H
