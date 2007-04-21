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

CRelations::CRelations( void )
{
	m_RelMat = new CMatrixSq<int>;
	m_bMatFailed = false;
}

CRelations::~CRelations( void )
{
	Clear();
	delete m_RelMat;
}

CRelations &CRelations::operator=(const CRelations &in)
{
	if(!m_RelMat->IsCleared())
		Clear();

	m_RelMat->Copy( in.m_RelMat );

	return *this;
}

void CRelations::Clear( void )
{
	if( !m_RelMat->IsCleared() )
	{
		m_RelMat->Clear();
	}
}

bool CRelations::IsCleared( void )
{
	return m_RelMat->IsCleared();
}

int CRelations::Size( void )
{
	return m_RelMat->Dim();
}

int CRelations::GetRelNum(int i, int j)
{
	int returnval = 0;
	int *pval;

	// uncomment for debugging of relationship checks
	// DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Checking relationship matrix for team %d towards team %d.\r", i, j);
	
	// return the default and don't attempt to check the matrix if it failed to load
	if( m_bMatFailed )
	{
		if( i == j )
			returnval = s_DefaultSameTeamRel;
		else
			returnval = s_DefaultRelation;

		goto Quit;
	}
	
	pval = m_RelMat->Get( i , j );

	if ( pval == NULL )
	{
		// uncomment for reporting errors when doing relationship checks
		// DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Bad indices used to query relationship matrix: %d, col: %d.\r", i, j);
		
		returnval = s_DefaultRelation;
		goto Quit;
	}

	returnval = *pval;

Quit:
	return returnval;
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
	m_RelMat->Set( i, j, rel );
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
	SEntryData EntryDat;
	int start, end, num(1), tempint, tempint2, maxrow(0), keylen;
	idStr tempKey, tempVal, row, col, val;

	bool hadSynError(false), hadLogicError(false);

	m_bMatFailed = false;
	
	const idKeyValue *Entry = args->MatchPrefix( "rel ", NULL );
	while ( Entry ) 
	{
		tempKey = Entry->GetKey();
		tempVal = Entry->GetValue();

		keylen = tempKey.Length();
		
		// parse it
		start = 4;
		end = tempKey.Find(',');
		end--;

		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: arg %d, start = %d, end(comma-1) = %d\r", num, start, end);

		if(end < 0 )
		{
			hadSynError = true;
			goto Quit;
		}
		
		row = tempKey.Mid( start, (end - start + 1) );

		start = end+2;

		if( start > keylen )
		{
			hadSynError = true;
			goto Quit;
		}

		col = tempKey.Mid( start, keylen - start + 1 );

		val = args->GetString( tempKey.c_str(), "" );

		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: arg %d, row = %s, col = %s, val = %d\r", num, row.c_str(), col.c_str(), atoi(val.c_str()) );

		if( !row.IsNumeric() || !col.IsNumeric() || !val.IsNumeric() )
		{
			hadSynError = true;
			goto Quit;
		}

		//set up the Entry data
		EntryDat.row = atoi( row.c_str() );
		EntryDat.col = atoi( col.c_str() );
		EntryDat.val = atoi( val.c_str() );

		if (EntryDat.row > maxrow )
			maxrow = EntryDat.row;

		EntryList.Append(EntryDat);

		// Check for diagonal element of the ROW team, fill it in with
		// the default diagonal relation if it does not exist.

		if (args->FindKeyIndex( va("rel %d,%d", EntryDat.row, EntryDat.row) ) == -1
								&& DiagsAdded.FindIndex(EntryDat.row) == -1 )
		{
			tempint = EntryDat.col;
			tempint2 = EntryDat.val;

			EntryDat.col = EntryDat.row;
			EntryDat.val = s_DefaultSameTeamRel;

			EntryList.Append(EntryDat);
			DiagsAdded.Append(EntryDat.row);

			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing diagonal %d, %d\r", EntryDat.row, EntryDat.row );

			//set EntryDat back the way it was for further checks
			EntryDat.col = tempint;
			EntryDat.val = tempint2;
		}

		// Check for diagonal element of the COLUMN team, fill it in with
		// the default diagonal relation if it does not exist.

		if (args->FindKeyIndex( va("rel %d,%d", EntryDat.col, EntryDat.col) ) == -1
								&& DiagsAdded.FindIndex(EntryDat.col) == -1 )
		{
			tempint = EntryDat.row;
			tempint2 = EntryDat.val;
			
			if( EntryDat.col > maxrow )
				maxrow = EntryDat.col;

			EntryDat.row = EntryDat.col;
			EntryDat.val = s_DefaultSameTeamRel;
			EntryList.Append(EntryDat);
			DiagsAdded.Append(EntryDat.col);

			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing diagonal %d, %d\r", EntryDat.col, EntryDat.col );

			//set EntryDat back the way it was for further checks
			EntryDat.row = tempint;
			EntryDat.val = tempint2;
		}

		// Check for asymmetric element and append one with same value if
		// it does not exist.
		if ( args->FindKeyIndex( va("rel %d,%d", EntryDat.col, EntryDat.row) ) == -1 )
		{
			tempint = EntryDat.row;
			EntryDat.row = EntryDat.col;
			EntryDat.col = tempint;

			EntryList.Append(EntryDat);
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Relmat Parser: Added missing asymmetric element %d, %d\r", EntryDat.row, EntryDat.col );
		}

		Entry = args->MatchPrefix( "rel ", Entry );
		num++;
	}

	EntryList.Condense();

	maxrow++;
	if (EntryList.Num() != (maxrow*maxrow))
	{
		hadLogicError = true;
		goto Quit;
	}

	if ( !m_RelMat->Init(maxrow) )
	{
		hadLogicError = true;
		goto Quit;
	}

	for( int i=0; i<EntryList.Num(); i++ )
	{
		if ( !m_RelMat->Set(EntryList[i].row, EntryList[i].col, EntryList[i].val ) )
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
	m_RelMat->SaveMatrixSq( save );
}

void CRelations::Restore( idRestoreGame *save )
{
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Loading Relationship Matrix data from save\r");

	if( !m_RelMat->RestoreMatrixSq( save ) )
	{
		// attempted to load empty matrix
		m_bMatFailed = true;
	}
	
	CopyThisToGlobal();
}

void CRelations::CopyThisToGlobal( void )
{
	g_globalRelations = *this;
}

void CRelations::DebugPrintMat( void )
{
	int i, row, col, val;
	
	idLib::common->Printf("Printing Relations Matrix with %d elements:\n", m_RelMat->NumFilled() );
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("DebugPrintMat: m_RelMat::IsCleared = %d\r", m_RelMat->IsCleared() );
	if( m_RelMat->IsCleared() )
	{
		idLib::common->Printf("Relations Matrix is Empty.\n");
		goto Quit;
	}

	for(i=0; i < m_RelMat->NumFilled(); i++)
	{
		row = i / m_RelMat->Dim();
		col = i % m_RelMat->Dim();
		val = *m_RelMat->Get1d(i);
		idLib::common->Printf("[Relations Matrix] Element %d,%d: %d\n", row, col, val );
	}

Quit:
	return;
}