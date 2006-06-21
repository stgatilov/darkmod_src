
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
 * Revision 1.3  2003/10/15 19:59:27  lightweave
 * Fixed a warning about values out of range.
 *
 * Revision 1.2  2003/10/04 22:01:14  lightweave
 * Modifications for compiling the sources under Linux.
 *
 * Revision 1.1.1.1  2003/10/04 17:13:12  lightweave
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

#include "Misc.h"


/**********************************************************************/
/*                                                                    */
/* FUNCTION: ToUpper(BYTE)                                            */
/*                                                                    */
/* INPUT:    BYTE      Byte to convert to uppercase.                  */
/*                                                                    */
/* OUTPUT:   BYTE      Uppercase character                            */
/*                                                                    */
/* DESCRIPTION: Converts a character to uppercase and also taks into  */
/* account the german umlauts.                                        */
/*                                                                    */
/**********************************************************************/

BYTE ToUpper(UBYTE b)
{
	if(b >= 'a' && b <= 'z')
		b += 'A'-'a';
	else
	switch(b)
	{
		case 0x94:		/* oe */
			b = (BYTE)0x99;
		break;

		case 0x81:  	/* ue */
			b = (BYTE)0x9A;
		break;

		case 0x84:
			b = (BYTE)0x8E;
		break;
	}

	return(b);
}


