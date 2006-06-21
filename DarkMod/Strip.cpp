
/*************************************************************************/
/*                                                                       */
/*           StringLib (C) by Gerhard W. Gruber in Vienna 1992           */
/*                       All rights reserved                             */
/*                                                                       */
/*************************************************************************/

/******************************************************************************
 *
 * PROJECT: LaunchMenu
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.2  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.1  2004/10/30 17:06:36  sparhawk
 * DarkMod added to project.
 *
 * Revision 1.2  2003/10/04 22:01:14  lightweave
 * Modifications for compiling the sources under Linux.
 *
 * Revision 1.1.1.1  2003/10/04 17:13:16  lightweave
 * Initial Release to CVS.
 *
 *
 * DESCRIPTION: Strips a string from whitespaces.
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

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include <string.h>

#include "Misc.h"

LONG FindChar(UBYTE *s, UBYTE *p, BOOL Case, BOOL Find, UBYTE SkipChar);

/**
 *
 * FUNCTION: strip(UBYTE *String, int Start)
 *
 * INPUT:    UBYTE     [IN/OUT]	String	- String that should be stripped
 *                     [IN]		Start	- TRUE if strip from start of string
 *										  FALSE if stripping from end. Any other
 *										  value means both sides are stripped (i.e. 2).
 *
 * OUTPUT:   BYTE      Byte in Grossschreibung
 *
 * DESCRIPTION: This function strips the string from whitespaces
 *
 */
UBYTE *Strip(UBYTE *s, int Start)
{
	unsigned long n;
	UBYTE *e;
	BOOL d;

	/* If neither true nor false we strip both parts */
	if(Start != TRUE && Start != FALSE)
	{
		d = TRUE;
		Start = FALSE;
	}
	else
		d = Start;

	n = strlen((const char *)s);
	e = &s[n-1];

	if(d == TRUE)
	{
		while(*s)
		{
			if(*s == ' ' || *s == '\t')
				s++;
			else
				break;
		}
		d = Start;
	}

	// Check if we need a to strip at end as well.
	// This may not be simply the else branch, because we change this
	// when both parts are to be stripped.
	if(d == FALSE)
	{
		while(e > s)
		{
			if(*e == ' ' || *e == '\t')
			{
				*e = 0;
				e--;
			}
			else
				break;
		}
	}

	return(s);
}


/**
 *
 * FUNCTION: strstrip(UBYTE *String, int Start, UBYTE *Pattern)
 *
 * INPUT:    UBYTE     [IN/OUT]	String	- String that should be stripped
 *                     [IN]		Start	- TRUE if strip from start of string
 *										  FALSE if stripping from end. Any other
 *										  value means both sides are stripped (i.e. 2).
 *						[IN		Pattern	- This string contains a list of characteres
 *										  that should be removed from the string.
 *
 * OUTPUT:   BYTE      Byte in Grossschreibung
 *
 * DESCRIPTION: This function strips the string from all characteres listed in patter
 * which are at the start or end of the string.
 *
 */
UBYTE *StrStrip(UBYTE *s, int Start, char *p)
{
	unsigned long n;
	UBYTE *e, str[2];
	BOOL d;

	/* If neither true nor false we strip both parts */
	if(Start != TRUE && Start != FALSE)
	{
		d = TRUE;
		Start = FALSE;
	}
	else
		d = Start;

	str[1] = 0;
	n = strlen((const char *)s);
	e = &s[n-1];

	if(d == TRUE)
	{
		while(*s)
		{
			str[0] = *s;
			if(FindChar((UBYTE *)str, (UBYTE *)p, FALSE, TRUE, 0) != -1)
				s++;
			else
				break;
		}
		d = Start;
	}

	// Check if we need a to strip at end as well.
	// This may not be simply the else branch, because we change this
	// when both parts are to be stripped.
	if(d == FALSE)
	{
		while(e > s)
		{
			str[0] = *e;
			if(FindChar((UBYTE *)str, (UBYTE *)p, FALSE, TRUE, 0) != -1)
			{
				*e = 0;
				e--;
			}
			else
				break;
		}
	}

	return(s);
}


