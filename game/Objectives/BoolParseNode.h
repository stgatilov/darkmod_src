/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#ifndef TDM_OBJECTIVE_BOOLPARSENODE_H
#define TDM_OBJECTIVE_BOOLPARSENODE_H

#include "precompiled_game.h"

/**
* Structure for parsing boolean logic
**/
struct SBoolParseNode
{
	int Ident;
	bool bNotted; // set to true if this node is NOTed

	idList< idList< SBoolParseNode > > Cols; // list of columns, each can contain a different number of rows

	// Link back to previous node this one branched off from
	SBoolParseNode* PrevNode;

	// matrix coordinates of this node within the matrix of the previous node
	int PrevCol; 
	int PrevRow;

	// Functions:

	SBoolParseNode()
	{ 
		Clear();
	}

	~SBoolParseNode()
	{
		Clear();
	}

	bool IsEmpty() const
	{ 
		return (Cols.Num() == 0 && Ident == -1);
	}

	/**
	* Clear the parse node
	**/
	void Clear( void )
	{
		Ident = -1;
		PrevCol = -1;
		PrevRow = -1;

		bNotted = false;
		Cols.Clear();
		PrevNode = NULL;
	}
};

#endif
