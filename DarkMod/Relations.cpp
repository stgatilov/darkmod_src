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

void CRelations::Clear()
{
	m_RelMat.Clear();
}

bool CRelations::IsCleared()
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
	// Check if the indices exceed the current bounds?
	if (i >= m_RelMat.Dim() || j >= m_RelMat.Dim())
	{
		// At least one of the indices exceeds the current bounds.
		// Extend the matrix and initialise the new ground.
		ExtendRelationsMatrixToDim( (i > j) ? i+1 : j+1 );
	}

	m_RelMat.Set(i, j, rel);
}

void CRelations::ExtendRelationsMatrixToDim(int newDim)
{
	// Go through the new columns
	for (int n = m_RelMat.Dim(); n < newDim; ++n)
	{
		// Fill in the new column and the new row
		for (int i = 0; i <= n; ++i)
		{
			if (i == n)
			{
				m_RelMat.Set(i, i, s_DefaultSameTeamRel);
			}
			else 
			{
				// Fill the new column
				m_RelMat.Set(i, n, s_DefaultRelation);
				// Fill the new row
				m_RelMat.Set(n, i, s_DefaultRelation);
			}
		}
	}
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

CRelations::SEntryData CRelations::ParseEntryData(const idKeyValue* kv)
{
	const idStr& tempKey = kv->GetKey();
	const idStr& val = kv->GetValue();

	int keylen = tempKey.Length();
	
	// parse it
	int start = 4;
	int end = tempKey.Find(',');
	end--;

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: start = %d, end(comma-1) = %d\r", start, end);

	if (end < 0 )
	{
		throw std::runtime_error(std::string("Could not find comma in spawnarg: ") + tempKey.c_str());
	}
	
	idStr row = tempKey.Mid( start, (end - start + 1) );

	start = end + 2;

	if( start > keylen )
	{
		throw std::runtime_error(std::string("Could not find second team in spawnarg: ") + tempKey.c_str());
	}

	idStr col = tempKey.Mid( start, keylen - start + 1 );

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: row = %s, col = %s, val = %d\r", row.c_str(), col.c_str(), atoi(val.c_str()) );

	if( !row.IsNumeric() || !col.IsNumeric() || !val.IsNumeric() )
	{
		throw std::runtime_error(std::string("One of these values is not numeric: ") + tempKey.c_str());
	}

	return SEntryData( atoi(row.c_str()), atoi(col.c_str()), atoi(val.c_str()) );
}

bool CRelations::SetFromArgs(const idDict& args)
{
	idList<SEntryData> entries;
	idList<int> addedDiags;
	
	// TODO: Remove this, matrix parsing never "fails" in this sense
	m_bMatFailed = false;
	
	for (const idKeyValue* kv = args.MatchPrefix("rel ", NULL ); kv != NULL; kv = args.MatchPrefix("rel ", kv)) 
	{
		try
		{
			// Try to parse the entry, this will throw on errors
			SEntryData entry = ParseEntryData(kv);

			// Successfully parsed, add to list
			entries.Append(entry);

			// Will the current matrix dimension be increased by this entry?
			if (entry.row < m_RelMat.Dim() && entry.col < m_RelMat.Dim())
			{
				// No, matrix will not be extended, no need to check for 
				// diagonals and asymmetric values
				continue;
			}

			// The matrix will be extended by this entry, let's check for diagonals

			// Check for diagonal element of the ROW team
			if (args.FindKeyIndex( va("rel %d,%d", entry.row, entry.row) ) == -1 && 
				addedDiags.FindIndex(entry.row) == -1)
			{
				// ROW team diagonal not set, fill with default team relation entry
				SEntryData defaultDiagonal(entry.row, entry.row, s_DefaultSameTeamRel);
				entries.Append(defaultDiagonal);

				// Remember the diagonal number, so that we don't add it a second time
				addedDiags.Append(entry.row);

				DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing diagonal %d, %d\r", entry.row, entry.row);
			}

			// Check for diagonal element of the COLUMN team
			if (args.FindKeyIndex( va("rel %d,%d", entry.col, entry.col) ) == -1 && 
				addedDiags.FindIndex(entry.col) == -1)
			{
				// COLUMN team diagonal not set, fill with default team relation entry
				SEntryData defaultDiagonal(entry.col, entry.col, s_DefaultSameTeamRel);
				entries.Append(defaultDiagonal);

				// Remember the diagonal number, so that we don't add it a second time
				addedDiags.Append(entry.col);

				DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing diagonal %d, %d\r", entry.col, entry.col);
			}

			// Check for asymmetric element and append one with same value if
			// it is not set on this dictionary
			if (args.FindKeyIndex( va("rel %d,%d", entry.col, entry.row) ) == -1)
			{
				// Pass col as first arg, row as second to define the asymmetric value
				SEntryData asymmRel(entry.col, entry.row, entry.val);
				entries.Append(asymmRel);

				DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing asymmetric element %d, %d\r", entry.row, entry.col );
			}
		}
		catch (std::runtime_error e)
		{
			gameLocal.Warning("Parse error: %s", e.what());
		}
	}

	// Commit the found values to the relations matrix
	for (int i = 0; i < entries.Num(); ++i)
	{
		const SEntryData& entry = entries[i];

		// Use the SetRel() method, which automatically takes care of
		// initialising new teams
		SetRel(entry.row, entry.col, entry.val);
	}

	return true;
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
