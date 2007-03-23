/*************************************************************************/
/*                                                                       */
/*           StringLib (C) by Gerhard W. Gruber in Vienna 1992           */
/*                       All rights reserved                             */
/*                                                                       */
/*************************************************************************/

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

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <memory.h> /* memset */

#include "Misc.h"

#define MAX_BUFFER		1024

static UBYTE ReadBuffer[MAX_BUFFER+1];
static ULONG BufferIndex = 0, len = 0;

LONG ReadLine(FILE *strm, void *Buffer, LONG Lines, LONG MaxLen, WORD TabExpand)
{
	LONG rc = -1, i, x = 0, Anz = 0, ReadBufferLen = MAX_BUFFER;
	UBYTE *ptr = (UBYTE *)Buffer;
	UWORD	tablen;

	if(!strm)
	{
		errno = EBADF;
		goto Quit;
	}

	/* Wenn die Anzahl der Zeilen gleich 0 sein soll oder die maximale
		Zeilenlaenge gleich 0 ist, kann nichts gelesen werden.
	*/
	if(Lines == 0 || MaxLen == 0)
	{
		rc = 0;
		goto Quit;
	}

	i = 0;
	while(i < Lines)
	{
		/* Wenn der Buffer leer ist, dann muss ein neuer Teil eingelesen werden. */
		if(BufferIndex == len)
		{
			if(feof(strm))
			{
				if(Anz)
					i++;

				ptr[x] = 0;
				break;
			}

			memset(ReadBuffer, 0, ReadBufferLen+1);
			if((len = fread(ReadBuffer, sizeof(char), ReadBufferLen, strm)) != (size_t)ReadBufferLen)
			{
				if(!feof(strm))
				{
					rc = -1;
					goto Quit;
				}
			}

			BufferIndex = 0;
		}

		while(x < MaxLen && BufferIndex < len && i < Lines)
		{
			/* 0x0A = LF wird als Zeilenende interpretiert. */
			if(ReadBuffer[BufferIndex] == '\n')		/* 0x0A = LF wird ueberlesen. */
			{
				/* Wenn von stdin gelesen wird, wird bei Eingabe von Enter nur
				   LF gelesen ohne CR.
				*/
				if(x > 0 && ptr[x-1] == '\r')
					ptr[x-1] = 0;									/* Zeilenende */
				else
					ptr[x] = 0;
				i++;
				Anz = 0;
			}
			else
			if(ReadBuffer[BufferIndex] != '\r')
			{
				if(TabExpand && ReadBuffer[BufferIndex] == '\t')
				{
					/* Tabulatoren in ' ' umwandeln */
					tablen = TabExpand - (x % TabExpand);
					x += sprintf((char *)&ptr[x], (const char *)"%*.*s", tablen, tablen, "")-1;
				}
				else
					ptr[x++] = ReadBuffer[BufferIndex];

				Anz++;
			}

			BufferIndex++;
		}

		if(x >= MaxLen)
		{
			i++;
			break;
		}

		if(BufferIndex == len)
			ptr[x] = 0;
	}

	rc = i;

Quit:
	return(rc);
}
