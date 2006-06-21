
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
 * Revision 1.2  2003/10/04 22:01:15  lightweave
 * Modifications for compiling the sources under Linux.
 *
 * Revision 1.1.1.1  2003/10/04 17:13:13  lightweave
 * Initial Release to CVS.
 *
 *
 * DESCRIPTION: 
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

#define __STRING__

#include "Misc.h"

BYTE ToUpper(UBYTE b);

/**********************************************************************/
/*                                                                    */
/* FUNCTION: FindChar(UBYTE *Z, UBYTE *P, BOOL C, BOOL F, UBYTE S)    */
/*                                                                    */
/* INPUT:    Z    String                                              */
/*           P    Patternstring of allowed characters                 */
/*           C    Case sensitivity                                    */
/*           F    Find the character[s] in the pattern?               */
/*           S    Escapecharacter                                     */
/*                                                                    */
/* OUTPUT:   LONG      Index to the found character or -1             */
/*                                                                    */
/* DESCRIPTION: This function is searching specified characters in    */
/* given string. If the character(s) can not be found -1 is returned, */
/* otherwise the index of the character. If Find == TRUE then the     */
/* search will break as soon as one of the characters have been found.*/
/* If Find == FALSE the search will break as soon as any character is */
/* found that is NOT in the patternstring.                            */
/* If C is TRUE then the string will be searched casesensitive,       */
/* otherwise the case will be ignored.                                */
/* Additionaly you can specify an escape character S. This is a single*/
/* character which indicates that the next character should be ignored*/
/* i.E. C:\\test\\ S=\ will skip the second \ whenever it occurs.     */
/* This is usefull when looking for wildcards, where the wildcard     */
/* itself may also be part of the string.                             */
/*                                                                    */
/**********************************************************************/

LONG FindChar(UBYTE *s, UBYTE *p, BOOL Case, BOOL Find, UBYTE SkipChar)
{
	LONG rc = -1, i, len, x;
	BYTE c1, c2, deakt;

	len = strlen((const char *)p);
	deakt = FALSE;

	for(i = 0; s[i]; i++)
	{
		/* Skip character */
		if(deakt == TRUE)
		{
			deakt = FALSE;
			continue;
		}

		c1 = s[i];

		if(!Case)
			c1 = ToUpper((UBYTE)c1);

		/* If the current char is the escape character, we skip the next character. */
		if(SkipChar == 0x00 || (SkipChar != 0x00 && c1 != SkipChar))
		{
			/* loop over the pattern string */
			for(x = 0; x < len; x++)
			{
				c2 = p[x];

				if(!Case)
					c2 = ToUpper((UBYTE)c2);

				/* Should the character be found? */
				if(Find == TRUE)
				{
					if(c1 == c2)
					{
						rc = i;
						goto Quit;
					}
				}
				else
				{
					if(c1 != c2)
					{
						rc = i;
						goto Quit;
					}
				}
			}
		}
		else
			deakt = TRUE;
	}

Quit:
	return(rc);
}
