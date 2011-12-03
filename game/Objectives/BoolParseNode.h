/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef TDM_OBJECTIVE_BOOLPARSENODE_H
#define TDM_OBJECTIVE_BOOLPARSENODE_H

#include "../idlib/precompiled.h"

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
