/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
/*************************************************************************/
/*                                                                       */
/*           StringLib (C) by Gerhard W. Gruber in Vienna 1992           */
/*                       All rights reserved                             */
/*                                                                       */
/*************************************************************************/

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
	bool d;
	bool start = ( Start != FALSE );

	/* If neither true nor false we strip both parts */
	if(Start != TRUE && Start != FALSE)
	{
		d = TRUE;
		Start = FALSE;
	}
	else
		d = start;

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
		d = start;
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
	bool d;
	bool start = ( Start != FALSE );


	/* If neither true nor false we strip both parts */
	if(Start != TRUE && Start != FALSE)
	{
		d = true;
		start = false;
	}
	else
		d = start;

	str[1] = 0;
	n = strlen((const char *)s);
	e = &s[n-1];

	if( d == true )
	{
		while(*s)
		{
			str[0] = *s;
			if(FindChar((UBYTE *)str, (UBYTE *)p, FALSE, TRUE, 0) != -1)
				s++;
			else
				break;
		}
		d = start;
	}

	// Check if we need a to strip at end as well.
	// This may not be simply the else branch, because we change this
	// when both parts are to be stripped.
	if( d == false )
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


