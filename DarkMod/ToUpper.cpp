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
		b += (UBYTE)('A'-'a');
	else
	switch(b)
	{
		case 0x94:		/* oe */
			b = (UBYTE)0x99;
		break;

		case 0x81:  	/* ue */
			b = (UBYTE)0x9A;
		break;

		case 0x84:
			b = (UBYTE)0x8E;
		break;
	}

	return(b);
}


