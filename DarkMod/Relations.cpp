/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
/******************************************************************************/
/*                                                                            */
/*         Dark Mod AI Relationships (C) by Chris Sarantos in USA 2005		  */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
*
*	Class for storing and managing AI to Player and AI to AI relationships
*	See header file for complete description.
*
*****************************************************************************/

#pragma hdrstop

#include "../game/game_local.h"

static bool init_version = FileVersionList("$Id$", init_version);

#pragma warning(disable : 4996)

#include "Relations.h"
#include "MatrixSq.h"


/**
* TODO: Move these constants to def file or .ini file
**/
static const int s_DefaultRelation = -1;

static const int s_DefaultSameTeamRel = 5;


CLASS_DECLARATION( idClass, CRelations )
END_CLASS

CRelations::CRelations()
{
	m_bMatFailed = false;
}

CRelations::~CRelations()
{
	Clear();
}

CRelations &CRelations::operator=(const CRelations &in)
{
	if(!m_RelMat.IsCleared())
		Clear();

	m_RelMat = in.m_RelMat;
	m_bMatFailed = in.m_bMatFailed;

	return *this;
}

void CRelations::Clear( void )
{
	if( !m_RelMat.IsCleared() )
	{
		m_RelMat.Clear();
	}
}

bool CRelations::IsCleared( void )
{
	return m_RelMat.IsCleared();
}

int CRelations::Size( void )
{
	return m_RelMat.Dim();
}

int CRelations::GetRelNum(int i, int j)
{
	// uncomment for debugging of relationship checks
	// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Checking relationship matrix for team %d towards team %d.\r", i, j);
	assert(i >= 0 && j >= 0);
	
	// return the default and don't attempt to check the matrix 
	// if it failed to load or indices are out of bounds
	if (m_bMatFailed || i >= m_RelMat.Dim() || j >= m_RelMat.Dim())
	{
		if( i == j )
		{
			return s_DefaultSameTeamRel;
		}
		else
		{
			return s_DefaultRelation;
		}
	}
	
	return m_RelMat.Get(static_cast<std::size_t>(i), static_cast<std::size_t>(j));
}

int CRelations::GetRelType(int i, int j)
{
	int returnval(0), relNum;

	relNum = GetRelNum(i, j);

	if ( relNum<0 )
		returnval = E_ENEMY;

	else if ( relNum>0 )
		returnval = E_FRIEND;
	
	else if ( relNum == 0 )
		returnval = E_NEUTRAL;

	return returnval;
}

void CRelations::SetRel(int i, int j, int rel)
{
	m_RelMat.Set( i, j, rel );
}

void CRelations::ChangeRel( int i, int j, int offset)
{
	int entry, val;

	entry = GetRelNum( i, j );
	val = entry + offset;

	SetRel( i, j, val );
}

bool CRelations::IsFriend( int i, int j)
{
	if ( GetRelNum(i, j) > 0 )
		return true;
	else
		return false;
}

bool CRelations::IsEnemy( int i, int j)
{
	if ( GetRelNum(i, j) < 0 )
		return true;
	else
		return false;
}

bool CRelations::IsNeutral( int i, int j)
{
	if ( GetRelNum(i, j) == 0 )
		return true;
	else
		return false;
}

bool CRelations::SetFromArgs( idDict *args )
{
	idList<SEntryData> EntryList;
	idList<int> DiagsAdded;
	int num(1), maxrow(0);

	bool hadSynError(false), hadLogicError(false);

	m_bMatFailed = false;
	
	for (const idKeyValue* kv = args->MatchPrefix("rel ", NULL ); kv != NULL; kv = args->MatchPrefix("rel ", kv)) 
	{
		idStr tempKey = kv->GetKey();
		idStr val = kv->GetValue();

		int keylen = tempKey.Length();
		
		// parse it
		int start = 4;
		int end = tempKey.Find(',');
		end--;

		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: arg %d, start = %d, end(comma-1) = %d\r", num, start, end);

		if (end < 0 )
		{
			hadSynError = true;
			goto Quit;
		}
		
		idStr row = tempKey.Mid( start, (end - start + 1) );

		start = end + 2;

		if( start > keylen )
		{
			hadSynError = true;
			goto Quit;
		}

		idStr col = tempKey.Mid( start, keylen - start + 1 );

		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: arg %d, row = %s, col = %s, val = %d\r", num, row.c_str(), col.c_str(), atoi(val.c_str()) );

		if( !row.IsNumeric() || !col.IsNumeric() || !val.IsNumeric() )
		{
			hadSynError = true;
			goto Quit;
		}

		//set up the Entry data
		SEntryData entry;
		entry.row = atoi( row.c_str() );
		entry.col = atoi( col.c_str() );
		entry.val = atoi( val.c_str() );

		// Keep track of the maximum row count (TODO: remove maxrow)
		if (entry.row > maxrow )
		{
			maxrow = entry.row;
		}

		EntryList.Append(entry);

		// Check for diagonal element of the ROW team, fill it in with
		// the default diagonal relation if it does not exist.
		if (args->FindKeyIndex( va("rel %d,%d", entry.row, entry.row) ) == -1
								&& DiagsAdded.FindIndex(entry.row) == -1 )
		{
			SEntryData defaultDiagonal(entry.row, entry.row, s_DefaultSameTeamRel);
			EntryList.Append(defaultDiagonal);
			DiagsAdded.Append(entry.row);

			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing diagonal %d, %d\r", entry.row, entry.row );
		}

		// Check for diagonal element of the COLUMN team, fill it in with
		// the default diagonal relation if it does not exist.
		if (args->FindKeyIndex( va("rel %d,%d", entry.col, entry.col) ) == -1
								&& DiagsAdded.FindIndex(entry.col) == -1 )
		{
			SEntryData defaultDiagonal(entry.col, entry.col, s_DefaultSameTeamRel);

			EntryList.Append(defaultDiagonal);
			DiagsAdded.Append(entry.col);

			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing diagonal %d, %d\r", entry.col, entry.col );

			if( entry.col > maxrow )
			{
				maxrow = entry.col;
			}
		}

		// Check for asymmetric element and append one with same value if
		// it does not exist.
		if ( args->FindKeyIndex( va("rel %d,%d", entry.col, entry.row) ) == -1 )
		{
			// Pass col as first arg, row as second to define the asymmetric value
			SEntryData asymmRel(entry.col, entry.row, entry.val);

			EntryList.Append(asymmRel);
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing asymmetric element %d, %d\r", entry.row, entry.col );
		}

		num++;
	}

	EntryList.Condense();

	maxrow++;
	if (EntryList.Num() > (maxrow*maxrow))
	{
		hadLogicError = true;
		goto Quit;
	}

	if (EntryList.Num() == 0)
	{
		goto Quit;
	}

	if ( !m_RelMat.Init(maxrow) )
	{
		hadLogicError = true;
		goto Quit;
	}

	// angua: Fill matrix with defaults
	m_RelMat.Fill(s_DefaultRelation);

	// Set the default relationship between teams
	for (int i = 0; i < maxrow; i++)
	{
		m_RelMat.Set(i, i, s_DefaultSameTeamRel);
	}

	// angua: Set values from list
	for( int i = 0; i < EntryList.Num(); i++ )
	{
		if ( !m_RelMat.Set(EntryList[i].row, EntryList[i].col, EntryList[i].val ) )
		{
			hadLogicError = true;
			goto Quit;
		}
	}

Quit:
	if(hadSynError)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("[AI Relations] Syntax error when parsing Worldspawn args to Relationship Manager (arg number %d from the top)\r", num);
		idLib::common->Warning("[AI Relations] Syntax error when parsing Worldspawn args to Relationship Manager (arg number %d from the top)\r", num);
	}

// Don't output the error if there are no matrix entries
	if(hadLogicError && EntryList.Num() )
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("[AI Relations] Logical error when parsing Worldspawn args to Relationship Manager (matrix indices are incorrect or missing)\r");
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("[AI Relations] (number of elements = %d, required elements = %d)\r", EntryList.Num(), (maxrow*maxrow));
		idLib::common->Warning("[AI Relations] Logical error when parsing Worldspawn args to Relationship Manager (matrix indices are incorrect or missing)\r");
	}

	if ( hadSynError || hadLogicError )
		m_bMatFailed = true;

	return !(hadSynError || hadLogicError);
}


void CRelations::Save( idSaveGame *save ) const
{
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Saving Relationship Matrix data\r");
	m_RelMat.Save( save );
}

void CRelations::Restore( idRestoreGame *save )
{
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Loading Relationship Matrix data from save\r");

	m_RelMat.Restore( save );
	
	// TODO?
	CopyThisToGlobal();
}

void CRelations::CopyThisToGlobal( void )
{
	g_globalRelations = *this;
}

void CRelations::DebugPrintMat( void )
{
	//idLib::common->Printf("Printing Relations Matrix with %d elements:\n", m_RelMat.NumFilled() );
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("DebugPrintMat: m_RelMat::IsCleared = %d\r", m_RelMat.IsCleared() );
	if( m_RelMat.IsCleared() )
	{
		idLib::common->Printf("Relations Matrix is Empty.\n");
		return;
	}

	for (std::size_t i = 0; i < m_RelMat.size(); ++i)
	{
		for (std::size_t j = 0; j < m_RelMat.size(); ++j)
		{
			int value = m_RelMat.Get(i, j);
			idLib::common->Printf("[Relations Matrix] Element %d,%d: %d\n", static_cast<int>(i), static_cast<int>(j), value);
		}
	}
}

// ----------------------------- Relations entity ----------------------- 

CLASS_DECLARATION( idEntity, CRelationsEntity )
	// Events go here
END_CLASS

// Constructor, does nothing
CRelationsEntity::CRelationsEntity()
{}

void CRelationsEntity::Spawn()
{
	// Copy the values from our dictionary to the global relations matrix manager
	
}
