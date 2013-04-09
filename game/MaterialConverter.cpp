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

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "Game_local.h"
#include "MaterialConverter.h"

MaterialConversionStatusReport::MaterialConversionStatusReport()
{
	// Add error messages and status report messages to the map for different enum states. 
	arrErrorMessages.insert( ErrorMessageMap::value_type( eMaterialConversionStatus_Skipped_AlreadyConverted,	
		ConversionStat_s( "Skipping this material. Reason: Ambient blocks already exists and 'Force Update' is not used. \n",
		"Total materials skipped with existing ambient blocks: %u \n" )  ) );
	arrErrorMessages.insert( ErrorMessageMap::value_type( eMaterialConversionStatus_Skipped_NotDynamicallyLit,			
		ConversionStat_s( "Skipping this material. Reason: Material does not represent a dynamically lit surface. \n",
		"Total materials skipped that are not dynamically lit surfaces: %u \n" ) ) );
	arrErrorMessages.insert( ErrorMessageMap::value_type( eMaterialConversionStatus_Skipped_CouldNotFindEndOfBlock,		
		ConversionStat_s( "Skipping this material. Reason: Could not find closing block of this Material. \n",
		"" ) ) );
	arrErrorMessages.insert( ErrorMessageMap::value_type( eMaterialConversionStatus_SuccessFullyConverted,				
		ConversionStat_s( "Material converted successfully. \n",
		"Total materials converted: %u \n" ) ) );
	arrErrorMessages.insert( ErrorMessageMap::value_type( eMaterialConversionStatus_Skipped_ErrorsInConvertedMaterial,	
		ConversionStat_s( "There are errors in converted material. Please report this as bug. \n", 
		"") ) );
}



//-------------------------------------------------------------
// MaterialParsingHelper::GetValidStageExpression
//
// Description: Obtains a valid texture name with trailing white spaces and curly brackets (if found any) removed.
//				A check for valid number of opening and closing brackets performed.
//-------------------------------------------------------------
bool MaterialParsingHelper::GetValidStageExpression( idLexer &a_lexSource, idStr & a_strStageTextureName )
{
	int iOffset;
	a_strStageTextureName.Empty();

	std::vector< idStr > arrStrInvalidTokens;

	arrStrInvalidTokens.push_back( "bumpmap" );
	arrStrInvalidTokens.push_back( "diffusemap" );
	arrStrInvalidTokens.push_back( "specularmap" );
	arrStrInvalidTokens.push_back( "map" );

	// 	gameLocal.Printf("Entering loop. \n" );

	idToken tknParsedLine;

	for(int nBrackets = 0 ; !a_lexSource.EndOfFile() ; )
	{
		while(a_lexSource.ReadToken( &tknParsedLine )) 
		{
			if ( tknParsedLine.linesCrossed ) {
				// 				gameLocal.Printf("End of the line reached. \n" );
				break;
			}
			if ( a_strStageTextureName.Length() ) {
				a_strStageTextureName += " ";
			}
			bool bIsValidToken = true;
			for( std::vector<idStr>::iterator iter = arrStrInvalidTokens.begin() ; arrStrInvalidTokens.end() != iter; iter ++ )
			{
				if( 0 != tknParsedLine.Icmp( *iter ) )
					continue;

				bIsValidToken = false;
				// 				gameLocal.Printf("Invalid token found. \n" );
				break;
			}
			if( bIsValidToken )
			{
				a_strStageTextureName += tknParsedLine;
				// 				gameLocal.Printf("constructing \"%s\" with a valid token. \n", a_strStageTextureName.c_str() );
			}
			else
			{
				// 				gameLocal.Printf("%s is an invalid token. \n", tknParsedLine.c_str() );
				break;
			}
		}

		a_strStageTextureName.Strip('\n');
		a_strStageTextureName.Strip('\t');
		a_strStageTextureName.Strip('\r');
		a_strStageTextureName.Strip(' ');

		// Make sure that we have equal number of opening bracket and closing brackets.
		for(iOffset = 0, nBrackets = 0; 0 <= (iOffset = a_strStageTextureName.Find( '(', iOffset )); nBrackets ++, iOffset ++ );
		for(iOffset = 0; 0 <= (iOffset = a_strStageTextureName.Find( ')', iOffset )); nBrackets --, iOffset ++ );

		if ( 0 == nBrackets )
		{
			// We have gone one token ahead than needed in the lexer so offset it back again.
			// 			gameLocal.Printf("Unreading token %s. \n", tknParsedLine.c_str() );
			a_lexSource.UnreadToken( &tknParsedLine );

			a_strStageTextureName.Strip('}');
			a_strStageTextureName.Strip('{');
			if( a_strStageTextureName.Length() <= 0 )
				return false;

			gameLocal.Printf(" Found valid expression: %s \n ", a_strStageTextureName.c_str() );
			return true;
		}

		// Append the first token of the newly read line.
		a_strStageTextureName += tknParsedLine;
	}

	return false;
}

//-------------------------------------------------------------
// MaterialParsingHelper::GetMaterialStageInfo
//
// Description: For a given material stage (any one of "diffusemap", "bumpmap" & specular map ) finds out number of 
//				textures (pathnames) being used along with their UV scales. 
//-------------------------------------------------------------
void MaterialParsingHelper::GetMaterialStageInfo( const char* a_strMatStageName, idLexer &a_lexSource, ImageInfoMap & a_arrMatStageInfo )
{
	a_lexSource.Reset();
	// 	gameLocal.Printf( "Looking for valid %s stage information (w/o blend). \n", a_strMatStageName );
	while ( 1 == a_lexSource.SkipUntilString( a_strMatStageName ) )
	{
		ImageInfo_s currentImageInfo ;

		if( true == GetValidStageExpression( a_lexSource, currentImageInfo.m_strImageName ) )
		{
			// 			gameLocal.Printf( "Found valid %s stage information (w/o blend). \n", a_strMatStageName );
			a_arrMatStageInfo.insert( ImageInfoMap::value_type( eVertexBlendType_None, currentImageInfo ) );
		}
	}
	// 	if( a_arrMatStageInfo.size() == 0 )
	// 		gameLocal.Printf( "Could not find valid %s stage information w/o blend. \n", a_strMatStageName);

	a_lexSource.Reset();

	// 	gameLocal.Printf( "Looking for %s stage information with blend. \n", a_strMatStageName );
	while ( 1 == a_lexSource.SkipUntilString( "blend" ) )
	{
		idToken tknMatStage;
		if( 1 == a_lexSource.ReadToken( &tknMatStage ) && 0 == tknMatStage.Icmp(a_strMatStageName) )
		{
			idToken tknMap;
			if( 0 != a_lexSource.ReadToken( &tknMap ) && 0 == tknMap.Icmp("map") )
			{
				ImageInfo_s currentImageInfo;
				eVertexBlendType vertBlendType = eVertexBlendType_None;

				if( true == GetValidStageExpression( a_lexSource, currentImageInfo.m_strImageName ) )
				{
					bool bIsScaleFound = false;
					while( "}" != tknMatStage )
					{
						if( !a_lexSource.ReadToken( &tknMatStage ) )
						{
							gameLocal.Warning( "Unexpected end of material when trying to obtain scale. ");
							break;
						}
						// Not using expectTokenString anymore since "Map" is treated as different token than "map". 
						// So doing a manual non-case sensitive checks instead.
						if( !bIsScaleFound && 0 == tknMatStage.Icmp("scale") )
						{
							idStr strScale;
							if( true == GetValidStageExpression( a_lexSource, strScale ) )
								currentImageInfo.m_strUVScale = strScale;

							gameLocal.Printf(" Scale: %s \n ", strScale.c_str() );
							bIsScaleFound = true;
						}

						if( eVertexBlendType_None == vertBlendType )  
						{
							if( 0 == tknMatStage.Icmp("vertexColor") )
							{
								gameLocal.Printf(" The stage is vertex-colored \n " );
								vertBlendType = eVertexBlendType_VertexColored;
							}
							else if( 0 == tknMatStage.Icmp("inverseVertexColor") )
							{
								gameLocal.Printf(" The stage is inverse vertex-colored \n " );
								vertBlendType = eVertexBlendType_InvVertexColored;
							}
						}
						else if( bIsScaleFound )
							break;

					}
					a_arrMatStageInfo.insert( ImageInfoMap::value_type( vertBlendType, currentImageInfo ) );

				}
			}
		}
	}
}

//-------------------------------------------------------------
// MaterialParsingHelper::FindBlockContainingWords
//
// Description: Inside a specified material shader block, finds the character offsets for start & end of the block 
//				that contains the words (specified by vector of strings) in their exact order.
//	Return value: True if the block is found.
//-------------------------------------------------------------

bool MaterialParsingHelper::FindBlockContainingWords(  const char *a_text, std::vector<idStr>& a_arrSearchWords, unsigned int & a_uiStartOffset, unsigned int & a_uiEndOffset,
													 const char a_cBlockStart /*= '{'*/, const char a_cBlockEnd /*= '}'*/ )
{
	int	uiSearchIndex;
	unsigned int uiSearchOffset = 0;
	unsigned int iTextLength = idStr::Length(a_text);
	bool bAreAllWordsFound = false;

	for(std::vector<idStr>::iterator iter = a_arrSearchWords.begin(); ; )
	{

		uiSearchIndex = idStr::FindText( a_text, (*iter).c_str(), false, uiSearchOffset );
		//  		gameLocal.Printf( " Searched %s from offset %d and found index %d \n", (*iter).c_str(),uiSearchOffset, uiSearchIndex );

		if( uiSearchIndex < 0 )
		{
			//  			gameLocal.Warning( " Could not find search word %s", (*iter).c_str() );
			return false;
		}

		bAreAllWordsFound = true;

		// Make sure that, this is not the first word we have found.
		if ( a_arrSearchWords.begin() != iter )
		{
			if ( uiSearchIndex != uiSearchOffset )
			{
				//  				gameLocal.Warning( " Could not find search word %s in the expected order", (*iter).c_str() );

				//Start the search from the first word again, since all of the search words are important.
				bAreAllWordsFound = false;
				iter = a_arrSearchWords.begin();
				continue;
			}
		}

		// Increment the iterator.
		iter ++;

		if( a_arrSearchWords.end() == iter )
			break;

		//Read white spaces and adjust the search offset accordingly for the next search.
		for( uiSearchOffset = uiSearchIndex + (*(iter-1)).Length(); uiSearchOffset < iTextLength; uiSearchOffset++ )
		{
			if( ' ' == a_text[ uiSearchOffset ] || '\t' == a_text[ uiSearchOffset ] || '\r' == a_text[ uiSearchOffset ] )
				continue;

			break;
		}
	}

	if( bAreAllWordsFound )
	{
		// 		gameLocal.Printf( " Total %d word(s) found \n", a_arrSearchWords.size() );

		if( a_arrSearchWords.size() == 1 )
		{
			uiSearchOffset = uiSearchIndex;
		}

		// Start tracking offsets to the opening and closing of the block from the last search-Index.
		bool bIsStartOffsetFound	= false;
		bool bIsEndOffsetFound		= false;
		for( a_uiStartOffset = a_uiEndOffset = uiSearchOffset ; a_uiStartOffset > 0 &&  a_uiEndOffset < iTextLength ; )
		{
			if( a_cBlockStart == a_text[ a_uiStartOffset ] )
				bIsStartOffsetFound = true;
			else
				a_uiStartOffset --;

			if( a_cBlockEnd == a_text[ a_uiEndOffset ] )
				bIsEndOffsetFound = true;
			else
				a_uiEndOffset ++;

			if( bIsStartOffsetFound && bIsEndOffsetFound )
			{
				// Adjust end offset by one extra character to make sure that we account the closing block.
				a_uiEndOffset ++;
				//  				idStr myBlock( a_text, a_uiStartOffset, a_uiEndOffset );
				//  				gameLocal.Printf( "Found block from %d to %d, search offset is %d\n", a_uiStartOffset, a_uiEndOffset, uiSearchOffset );
				//  				gameLocal.Printf( "%s \n", myBlock.c_str() );
				return true;
			}
		}
		// 		gameLocal.Warning( " Block start found:%d Block End Found:%d, Returning false.", (int)bIsStartOffsetFound, (int)bIsEndOffsetFound );
	}

	// 	if( !bAreAllWordsFound )
	//  		gameLocal.Warning( " Returning false since given words can't be found in the exact given order." );
	return false;
}

//-------------------------------------------------------------
// MaterialConverter::CreateNewAmbientBlock
//
// Description: Creates a new ambient block from gathered dynamic light stages. Also handles cases of vertex based color blending. 
//-------------------------------------------------------------
void MaterialConverter::CreateNewAmbientBlock( const ImageInfoMap& a_arrDiffusemapInfo, const ImageInfoMap& a_arrBumpmapInfo, const ImageInfoMap& a_arrSpecularmapInfo, std::vector<char>& a_arrCharNewAmbientBlock )
{
	static const char newAmbientBlock[] = {	
		"\n	{							\n"
		"		if (global5 == 1)		\n"
		"		blend add				\n"
		"		map				%s		\n"
		"		scale			%s		\n"
		"		red				global2	\n"
		"		green			global3	\n"
		"		blue			global4	\n"
		"	}							\n"
		"	{							\n"
		"		if (global5 == 2)		\n"
		"		blend add				\n"
		"		program	ambientEnvironment.vfp	\n"
		"		vertexParm		0		%s, %s		// UV Scales for Diffuse and Bump	\n"
		"		vertexParm		1		%s, 1, 1	// (X,Y) UV Scale for specular		\n"
		"		vertexParm		2		global2, global3, global4, 1	\n"
		"																\n"
		"		fragmentMap		0		cubeMap env/gen1				\n"
		"		fragmentMap		1		%s			// Bump				\n"
		"		fragmentMap		2		%s			// Diffuse			\n"
		"		fragmentMap		3		%s			// Specular			\n"
		"	}"
	};

	static const char newAmbientBlockVertColorBlended[] = {	
		"\n	{							\n"
		"		if (global5 == 1)		\n"
		"		blend add				\n"
		"		map				%s		\n"
		"		scale			%s		\n"
		"		red				global2	\n"
		"		green			global3	\n"
		"		blue			global4	\n"
		"		vertexColor				\n"
		"	}							\n"
		"	{							\n"
		"		if (global5 == 1)		\n"
		"		blend add				\n"
		"		map				%s		\n"
		"		scale			%s		\n"
		"		red				global2	\n"
		"		green			global3	\n"
		"		blue			global4	\n"
		"		inverseVertexColor		\n"
		"	}							\n"
		"	{							\n"
		"		if (global5 == 2)		\n"
		"		blend add				\n"
		"		program	ambientEnvVertexBlend.vfp	\n"
		"		vertexParm		0		%s, %s		// UV Scales for Diffuse1 and Bump1	resp.	\n"
		"		vertexParm		1		%s, %s		// UV Scale for specular1 and Diffuse2 resp.\n"
		"		vertexParm		2		%s, %s		// UV Scale for Bump2 and specular2 resp.	\n"
		"		vertexParm		3		global2, global3, global4, 1	\n"
		"		//----------- VertexColored -------------------			\n"
		"		fragmentMap		0		cubeMap env/gen1				\n"
		"		fragmentMap		1		%s			// Bump1			\n"
		"		fragmentMap		2		%s			// Diffuse1			\n"
		"		fragmentMap		3		%s			// Specular1		\n"
		"		//----------- InverseVertexColored ------------			\n"
		"		fragmentMap		4		%s			// Bump2			\n"
		"		fragmentMap		5		%s			// Diffuse2			\n"
		"		fragmentMap		6		%s			// Specular2		\n"
		"	}"
	};



	ImageInfoMap::const_iterator itrDiffusemapInfoVertexColored		= a_arrDiffusemapInfo.find( eVertexBlendType_VertexColored );
	ImageInfoMap::const_iterator itrBumpmapInfoVertexColored		= a_arrBumpmapInfo.find( eVertexBlendType_VertexColored );
	ImageInfoMap::const_iterator itrSpecularmapInfoVertexColored	= a_arrSpecularmapInfo.find( eVertexBlendType_VertexColored );

	ImageInfoMap::const_iterator itrDiffusemapInfoInvVertexColored	= a_arrDiffusemapInfo.find( eVertexBlendType_InvVertexColored );
	ImageInfoMap::const_iterator itrBumpmapInfoInvVertexColored		= a_arrBumpmapInfo.find( eVertexBlendType_InvVertexColored );
	ImageInfoMap::const_iterator itrSpecularmapInfoInvVertexColored	= a_arrSpecularmapInfo.find( eVertexBlendType_InvVertexColored );

	bool bIsVertexColorBlended = ( ( a_arrDiffusemapInfo.end() != itrDiffusemapInfoVertexColored || a_arrBumpmapInfo.end() != itrBumpmapInfoVertexColored || 
		a_arrSpecularmapInfo.end() != itrSpecularmapInfoVertexColored )	&&
		(a_arrDiffusemapInfo.end() != itrDiffusemapInfoInvVertexColored || a_arrBumpmapInfo.end() != itrBumpmapInfoInvVertexColored || 
		a_arrSpecularmapInfo.end() != itrSpecularmapInfoInvVertexColored ) );

	ImageInfoMap::const_iterator itrDiffusemapInfo	= a_arrDiffusemapInfo.find( eVertexBlendType_None );
	ImageInfoMap::const_iterator itrBumpmapInfo		= a_arrBumpmapInfo.find( eVertexBlendType_None );
	ImageInfoMap::const_iterator itrSpecularmapInfo	= a_arrSpecularmapInfo.find( eVertexBlendType_None );


	if ( bIsVertexColorBlended )
	{
		gameLocal.Printf("This material is vertex-color blended. \n");

		// Find out normal maps for vertex blending.

		// Handle cases where vertexColor is not used for bumpmaps
		if ( a_arrBumpmapInfo.end() == itrBumpmapInfoVertexColored )
			itrBumpmapInfoVertexColored = itrBumpmapInfo;

		if ( a_arrBumpmapInfo.end() == itrBumpmapInfoInvVertexColored )
		{
			ImageInfoMap::const_iterator itrBumpmapInfo2 = itrBumpmapInfo;
			if( a_arrBumpmapInfo.end() != itrBumpmapInfo2 )
			{
				// Try and see if there's a second bumpmap with no vertex-color blend.
				++itrBumpmapInfo2;
				itrBumpmapInfoInvVertexColored =  a_arrBumpmapInfo.end() != itrBumpmapInfo2 ? ( eVertexBlendType_None == (*itrBumpmapInfo2).first ? itrBumpmapInfo2 : itrBumpmapInfo ) : itrBumpmapInfo;  
			}
		}
		unsigned int uiBlockSize =	idStr::Length( newAmbientBlockVertColorBlended ) + 1 + 
			//------------------ For vertexColor ----------------
			(a_arrDiffusemapInfo.end() != itrDiffusemapInfoVertexColored	? (*itrDiffusemapInfoVertexColored).second.m_strImageName.Length()	: idStr::Length("_black")	) * 2	+
			(a_arrDiffusemapInfo.end() != itrDiffusemapInfoVertexColored	? (*itrDiffusemapInfoVertexColored).second.m_strUVScale.Length()	: idStr::Length("1, 1")		) * 2	+ 
			(	a_arrBumpmapInfo.end() != itrBumpmapInfoVertexColored		? (*itrBumpmapInfoVertexColored).second.m_strImageName.Length()		: idStr::Length("_flat")	) 		+ 
			(	a_arrBumpmapInfo.end() != itrBumpmapInfoVertexColored		? (*itrBumpmapInfoVertexColored).second.m_strUVScale.Length()		: idStr::Length("1, 1")		)		+ 
			(a_arrSpecularmapInfo.end() != itrSpecularmapInfoVertexColored	? (*itrSpecularmapInfoVertexColored).second.m_strImageName.Length()	: idStr::Length("_black")	)		+ 
			(a_arrSpecularmapInfo.end() != itrSpecularmapInfoVertexColored	? (*itrSpecularmapInfoVertexColored).second.m_strUVScale.Length()	: idStr::Length("1, 1")		)		+ 
			//------------------ For inverseVertexColor ---------
			(a_arrDiffusemapInfo.end() != itrDiffusemapInfoInvVertexColored		? (*itrDiffusemapInfoInvVertexColored).second.m_strImageName.Length()	: idStr::Length("_black")	) * 2	+
			(a_arrDiffusemapInfo.end() != itrDiffusemapInfoInvVertexColored		? (*itrDiffusemapInfoInvVertexColored).second.m_strUVScale.Length()		: idStr::Length("1, 1")		) * 2	+ 
			(	a_arrBumpmapInfo.end() != itrBumpmapInfoInvVertexColored		? (*itrBumpmapInfoInvVertexColored).second.m_strImageName.Length()		: idStr::Length("_flat")	) 		+ 
			(	a_arrBumpmapInfo.end() != itrBumpmapInfoInvVertexColored		? (*itrBumpmapInfoInvVertexColored).second.m_strUVScale.Length()		: idStr::Length("1, 1")		)		+ 
			(a_arrSpecularmapInfo.end() != itrSpecularmapInfoInvVertexColored	? (*itrSpecularmapInfoInvVertexColored).second.m_strImageName.Length()	: idStr::Length("_black")	)		+ 
			(a_arrSpecularmapInfo.end() != itrSpecularmapInfoInvVertexColored	? (*itrSpecularmapInfoInvVertexColored).second.m_strUVScale.Length()	: idStr::Length("1, 1")		);


		a_arrCharNewAmbientBlock.resize( uiBlockSize, 0 );

		idStr::snPrintf( &a_arrCharNewAmbientBlock[0], uiBlockSize, newAmbientBlockVertColorBlended, 
			//------------------ For vertexColor ----------------
			a_arrDiffusemapInfo.end() != itrDiffusemapInfoVertexColored ?	(*itrDiffusemapInfoVertexColored).second.m_strImageName.c_str()				: "_black", 
			a_arrDiffusemapInfo.end() != itrDiffusemapInfoVertexColored ?	(*itrDiffusemapInfoVertexColored).second.m_strUVScale.c_str()				: "1, 1", 
			//------------------ For inverseVertexColor ---------
			a_arrDiffusemapInfo.end() != itrDiffusemapInfoInvVertexColored ?	(*itrDiffusemapInfoInvVertexColored).second.m_strImageName.c_str()		: "_black", 
			a_arrDiffusemapInfo.end() != itrDiffusemapInfoInvVertexColored ?	(*itrDiffusemapInfoInvVertexColored).second.m_strUVScale.c_str()		: "1, 1", 
			//---------------------------------------------------

			//------------------ For vertexColor ----------------
			a_arrDiffusemapInfo.end() != itrDiffusemapInfoVertexColored ?	(*itrDiffusemapInfoVertexColored).second.m_strUVScale.c_str()			: "1, 1", 
			a_arrBumpmapInfo.end() != itrBumpmapInfoVertexColored ?			(*itrBumpmapInfoVertexColored).second.m_strUVScale.c_str()				: "1, 1", 
			a_arrSpecularmapInfo.end() != itrSpecularmapInfoVertexColored ?	(*itrSpecularmapInfoVertexColored).second.m_strUVScale.c_str()			: "1, 1", 
			//------------------ For inverseVertexColor ---------
			a_arrDiffusemapInfo.end() != itrDiffusemapInfoInvVertexColored ?	(*itrDiffusemapInfoInvVertexColored).second.m_strUVScale.c_str()	: "1, 1", 
			a_arrBumpmapInfo.end() != itrBumpmapInfoInvVertexColored ?			(*itrBumpmapInfoInvVertexColored).second.m_strUVScale.c_str()		: "1, 1", 
			a_arrSpecularmapInfo.end() != itrSpecularmapInfoInvVertexColored ?	(*itrSpecularmapInfoInvVertexColored).second.m_strUVScale.c_str()	: "1, 1", 
			//---------------------------------------------------

			//------------------ For vertexColor ----------------
			a_arrBumpmapInfo.end() != itrBumpmapInfoVertexColored ?			(*itrBumpmapInfoVertexColored).second.m_strImageName.c_str()			: "_flat", 
			a_arrDiffusemapInfo.end() != itrDiffusemapInfoVertexColored ?	(*itrDiffusemapInfoVertexColored).second.m_strImageName.c_str()			: "_black", 
			a_arrSpecularmapInfo.end() != itrSpecularmapInfoVertexColored ?	(*itrSpecularmapInfoVertexColored).second.m_strImageName.c_str()		: "_black",
			//------------------ For inverseVertexColor ---------
			a_arrBumpmapInfo.end() != itrBumpmapInfoInvVertexColored ?			(*itrBumpmapInfoInvVertexColored).second.m_strImageName.c_str()			: "_flat", 
			a_arrDiffusemapInfo.end() != itrDiffusemapInfoInvVertexColored ?	(*itrDiffusemapInfoInvVertexColored).second.m_strImageName.c_str()		: "_black", 
			a_arrSpecularmapInfo.end() != itrSpecularmapInfoInvVertexColored ?	(*itrSpecularmapInfoInvVertexColored).second.m_strImageName.c_str()		: "_black"
			//---------------------------------------------------
			);
	}
	else
	{
		gameLocal.Printf("This material is vertex-color blended. \n");
		unsigned int uiBlockSize =	idStr::Length( newAmbientBlock ) + 1 + 
			(a_arrDiffusemapInfo.end() != itrDiffusemapInfo		? (*itrDiffusemapInfo).second.m_strImageName.Length()	: idStr::Length("_black")	) * 2	+
			(a_arrDiffusemapInfo.end() != itrDiffusemapInfo		? (*itrDiffusemapInfo).second.m_strUVScale.Length()		: idStr::Length("1, 1")		) * 2	+ 
			(	a_arrBumpmapInfo.end() != itrBumpmapInfo		? (*itrBumpmapInfo).second.m_strImageName.Length()		: idStr::Length("_flat")	)		+ 
			(	a_arrBumpmapInfo.end() != itrBumpmapInfo		? (*itrBumpmapInfo).second.m_strUVScale.Length()		: idStr::Length("1, 1")		)		+ 
			(a_arrSpecularmapInfo.end() != itrSpecularmapInfo	? (*itrSpecularmapInfo).second.m_strImageName.Length()	: idStr::Length("_black")	)		+ 
			(a_arrSpecularmapInfo.end() != itrSpecularmapInfo	? (*itrSpecularmapInfo).second.m_strUVScale.Length()	: idStr::Length("1, 1")		); 

		a_arrCharNewAmbientBlock.resize( uiBlockSize, 0 );

		idStr::snPrintf( &a_arrCharNewAmbientBlock[0], uiBlockSize, newAmbientBlock, 
			a_arrDiffusemapInfo.end() != itrDiffusemapInfo ?	(*itrDiffusemapInfo).second.m_strImageName.c_str()		: "_black", 
			a_arrDiffusemapInfo.end() != itrDiffusemapInfo ?	(*itrDiffusemapInfo).second.m_strUVScale.c_str()		: "1, 1", 
			a_arrDiffusemapInfo.end() != itrDiffusemapInfo ?	(*itrDiffusemapInfo).second.m_strUVScale.c_str()		: "1, 1", 
			a_arrBumpmapInfo.end() != itrBumpmapInfo ?			(*itrBumpmapInfo).second.m_strUVScale.c_str()			: "1, 1", 
			a_arrSpecularmapInfo.end() != itrSpecularmapInfo ?	(*itrSpecularmapInfo).second.m_strUVScale.c_str()		: "1, 1", 
			a_arrBumpmapInfo.end() != itrBumpmapInfo ?			(*itrBumpmapInfo).second.m_strImageName.c_str()			: "_flat", 
			a_arrDiffusemapInfo.end() != itrDiffusemapInfo ?	(*itrDiffusemapInfo).second.m_strImageName.c_str()		: "_black", 
			a_arrSpecularmapInfo.end() != itrSpecularmapInfo ?	(*itrSpecularmapInfo).second.m_strImageName.c_str()		: "_black"
			);
	}

}

//-------------------------------------------------------------
// MaterialConverter::ConvertMaterial
//
// Description: Converts a material to support new ambient lighting. 
//
// Return: Returns an enum that conveys status of conversion. 
//-------------------------------------------------------------
eMaterialConversionStatus MaterialConverter::ConvertMaterial( idMaterial *a_pMaterial, bool a_bForceUpdate /*= false */ )
{
	if( NULL == a_pMaterial )
		return eMaterialConversionStatus_InvalidMaterial;

	gameLocal.Printf("Material %s loaded. \n", a_pMaterial->GetName() );

	std::vector< char > charBuffer; 

	ImageInfoMap arrDiffusemapInfo;	
	ImageInfoMap arrBumpMapInfo;	
	ImageInfoMap arrSpecularmapInfo;	

	charBuffer.resize(  a_pMaterial->GetTextLength() + 1, 0 );
	a_pMaterial->GetText( &charBuffer[0] );

	idLexer lexSource( &charBuffer[0], charBuffer.size(), a_pMaterial->GetName(), LEXFL_NOFATALERRORS | LEXFL_ALLOWPATHNAMES );

	gameLocal.Printf("Finding out shader stages... \n" );

	bool bAreBumpmapsExtracted		=false;
	bool bAreDiffusemapsExtracted	=false;
	bool bAreSpecularmapsExtracted	=false;
	for ( int j=0; j < a_pMaterial->GetNumStages(); j++ )
	{
		const shaderStage_t *pShaderStage = a_pMaterial->GetStage(j);

		if( NULL == pShaderStage )
		{
			// 				a_pMaterial->Invalidate();
			// 				a_pMaterial->FreeData();			
			continue;
		}

		//GetMaterialStageInfo extracts all the stages in material of given type so don't loop again for multiple similar stages. 
		if ( bAreBumpmapsExtracted && bAreDiffusemapsExtracted && bAreSpecularmapsExtracted	)
			break;


		switch( pShaderStage->lighting )
		{
		case SL_BUMP:
			if( bAreBumpmapsExtracted )
				continue;

			gameLocal.Printf("Bumpmap stage found, extracting bumpmap information... \n" );
			 MaterialParsingHelper::GetMaterialStageInfo( "bumpmap", lexSource, arrBumpMapInfo );
			bAreBumpmapsExtracted = true;
			break;
		case SL_DIFFUSE:
			if( bAreDiffusemapsExtracted )
				continue;

			gameLocal.Printf("Diffusemap stage found, extracting diffusemap information... \n" );
			 MaterialParsingHelper::GetMaterialStageInfo( "diffusemap", lexSource, arrDiffusemapInfo );
			bAreDiffusemapsExtracted = true;
			break;
		case SL_SPECULAR:
			if( bAreSpecularmapsExtracted )
				continue;

			gameLocal.Printf("Specularmap stage found, extracting specularmap information... \n" );
			 MaterialParsingHelper::GetMaterialStageInfo( "specularmap", lexSource, arrSpecularmapInfo );
			bAreSpecularmapsExtracted = true;
			break;
		default:
			continue;
		}
	}
	gameLocal.Printf("Done. \n" );
	// 		break; // remove me!!!

	if( arrBumpMapInfo.size() == 0 && arrDiffusemapInfo.size() == 0 && arrSpecularmapInfo.size() == 0 )
	{
		// 			a_pMaterial->Invalidate();
		// 			a_pMaterial->FreeData();
		return eMaterialConversionStatus_Skipped_NotDynamicallyLit;
	}
	unsigned int uiBlockStartOffset, uiBlockEndOffset;
	std::vector< idStr >arrSearchWords;
	bool bIsAmbientBlockFound = false;

	// Note that, the spaces in the string:"if (global5 == 1)" and an externally modified material may not match. 
	// So avoid changing new ambient lighting blocks by hand, at least "if (global5 == 1)" part.
	arrSearchWords.clear();
	arrSearchWords.push_back("if (global5 == 1)");
	bIsAmbientBlockFound =  MaterialParsingHelper::FindBlockContainingWords( &charBuffer[0], arrSearchWords, uiBlockStartOffset, uiBlockEndOffset );

	gameLocal.Printf( "ForceUpdate is: %s\n", a_bForceUpdate? "true": "false" );
	if( bIsAmbientBlockFound )
	{
		gameLocal.Printf( "Found new ambient block\n" );

		charBuffer.erase( charBuffer.begin() + uiBlockStartOffset, charBuffer.begin() + uiBlockEndOffset );

		// Try the search again with global5 == 1 in case the material is vertex color blended.
		bool bIsSecondAmbientBlockFound =  MaterialParsingHelper::FindBlockContainingWords( &charBuffer[0], arrSearchWords, uiBlockStartOffset, uiBlockEndOffset );

		if( bIsSecondAmbientBlockFound )
			charBuffer.erase( charBuffer.begin() + uiBlockStartOffset, charBuffer.begin() + uiBlockEndOffset );

		arrSearchWords.clear();
		arrSearchWords.push_back("if (global5 == 2)");
		bIsAmbientBlockFound =  MaterialParsingHelper::FindBlockContainingWords( &charBuffer[0], arrSearchWords, uiBlockStartOffset, uiBlockEndOffset );
		if( bIsAmbientBlockFound  )
			charBuffer.erase( charBuffer.begin() + uiBlockStartOffset, charBuffer.begin() + uiBlockEndOffset );
	}

	//Some materials may have old ambient block along with the new one. So remove it if found.
	{
		bool bIsOldAmbientBlockFound;
		arrSearchWords.clear();
		arrSearchWords.push_back("red");
		arrSearchWords.push_back("global2");
		bIsOldAmbientBlockFound =  MaterialParsingHelper::FindBlockContainingWords( &charBuffer[0], arrSearchWords, uiBlockStartOffset, uiBlockEndOffset );
		if( bIsOldAmbientBlockFound  )
		{
			gameLocal.Printf( "Found old ambient block\n" );
			gameLocal.Printf( "Removing old ambient block\n" );
			charBuffer.erase( charBuffer.begin() + uiBlockStartOffset, charBuffer.begin() + uiBlockEndOffset );
			bIsAmbientBlockFound = true;
		}

		// Try the search again, in case the material is vertex color blended and there is second inverse-vertex-colored ambient block.
		bIsOldAmbientBlockFound =  MaterialParsingHelper::FindBlockContainingWords( &charBuffer[0], arrSearchWords, uiBlockStartOffset, uiBlockEndOffset );

		if( bIsOldAmbientBlockFound  )
			charBuffer.erase( charBuffer.begin() + uiBlockStartOffset, charBuffer.begin() + uiBlockEndOffset );

		// If we couldn't find old ambient block and we have new ambient block in place, 
		// then we can safely skip this material.
		else if( !a_bForceUpdate && bIsAmbientBlockFound )
			return eMaterialConversionStatus_Skipped_AlreadyConverted;
	}


	unsigned int uiOffset = 0;

	if( bIsAmbientBlockFound )
	{
		//Remove the old comment.
		unsigned int uiCommentStart, uiCommentEnd;
		arrSearchWords.clear();

		arrSearchWords.push_back("TDM");
		arrSearchWords.push_back("Ambient");
		arrSearchWords.push_back("Method");
		arrSearchWords.push_back("Related");
		if (  MaterialParsingHelper::FindBlockContainingWords( &charBuffer[0], arrSearchWords, uiCommentStart, uiCommentEnd, '\n', '\n' ) )
		{
			charBuffer.erase( charBuffer.begin() + uiCommentStart, charBuffer.begin() + uiCommentEnd );
			gameLocal.Printf( " Ambient method related comment found and removed. \n" );
		}
	}
	//------------------------------------
	int i;
	unsigned int uiEndoftheBlock = 0;
	for( i= charBuffer.size() - 1; i > 0; i-- )
	{
		if( '}' == charBuffer[i] )
		{
			uiEndoftheBlock = i--;
			// Find additional white spaces and new line characters before end of the block.
			while( '\n' == charBuffer[i] || ' ' == charBuffer[i] || '\t' == charBuffer[i] || '\r' == charBuffer[i] ) 
			{
				if(0 >= i)
					break;

				i--;
			}
			// Remove white spaces and new line characters that are found before end of the block.
			if( unsigned(i + 1) <= uiEndoftheBlock - 1 )
			{
				charBuffer.erase( charBuffer.begin() + i + 1 , charBuffer.begin() + uiEndoftheBlock - 1 );
				gameLocal.Printf( "%d trailing white spaces found at end of the block and are removed. %d, %d, %c \n", uiEndoftheBlock - i - 1, uiEndoftheBlock, i + 1, charBuffer[i+2] );
				// Update end of the block's position.
				uiEndoftheBlock = i + 2;
			}
			else
			{
				gameLocal.Printf( "No trailing white spaces found at end of the block. %c\n", charBuffer[i] );
			}
			break;
		}
	}

	idStr strMatTextWithNewBlock( &charBuffer[0] );

	if( i > 1 )
	{
		strMatTextWithNewBlock.Insert( "\n\n	// TDM Ambient Method Related ", uiEndoftheBlock - 1 );
		uiOffset = uiEndoftheBlock - 1 + idStr::Length(  "\n\n	// TDM Ambient Method Related " );
		//strMatTextWithNewBlock.Insert( '\n', uiOffset );
	}
	else
	{
		gameLocal.Warning( "Could not determine end of the material block. Skipping this material." );
		// 				a_pMaterial->Invalidate();
		// 				a_pMaterial->FreeData();
		return eMaterialConversionStatus_Skipped_CouldNotFindEndOfBlock;
	}


	gameLocal.Printf( "Processing Material %s \n", a_pMaterial->GetName() );

	std::vector<char> arrCharNewAmbientBlock;

	CreateNewAmbientBlock( arrDiffusemapInfo, arrBumpMapInfo, arrSpecularmapInfo, arrCharNewAmbientBlock );

	strMatTextWithNewBlock.Insert( &arrCharNewAmbientBlock[0], uiOffset );

	// Update the material text and save to the file.
	a_pMaterial->SetText( strMatTextWithNewBlock.c_str() );

	if( !a_pMaterial->Parse( strMatTextWithNewBlock.c_str(), strMatTextWithNewBlock.Length() ) )
	{
		return eMaterialConversionStatus_Skipped_ErrorsInConvertedMaterial;
	}
	a_pMaterial->ReplaceSourceFileText();
	a_pMaterial->Invalidate();
	a_pMaterial->FreeData();

	return eMaterialConversionStatus_SuccessFullyConverted;
}

//-------------------------------------------------------------
// MaterialConverter::Cmd_BatchConvertMaterials_f
//
// Description: Console command callback for tdm_batchConvertMaterials. 
//-------------------------------------------------------------
void MaterialConverter::Cmd_BatchConvertMaterials_f( const idCmdArgs& args )
{

	if( args.Argc() < 3 )
	{
		gameLocal.Printf( "Usage: tdm_batchConvertMaterials <StartIndex> <nMaterials> [forceUpdateAll] \n" );
		return;
	}

	bool bForceUpdateAllMaterials = false;
	if( args.Argc() > 3 )
	{
		bForceUpdateAllMaterials = (0 == idStr::Icmp( args.Argv(3), "forceupdateall" ));
	}

	const unsigned int uiStartIndex			= atoi(args.Argv(1));
	const unsigned int uiMaterialsToProcess	= atoi(args.Argv(2));

	const unsigned long uiTotalMats = declManager->GetNumDecls( DECL_MATERIAL );

	gameLocal.Printf("Parsing %lu materials, this may take few minutes...\n", uiTotalMats );

	unsigned long i = uiStartIndex > (uiTotalMats - 1) ? uiTotalMats : uiStartIndex;
	const unsigned uiMaxMaterialsToProcess = i + uiMaterialsToProcess;

	MaterialConversionStatusReport matStatusReport;

	for ( ; i < uiTotalMats && i < uiMaxMaterialsToProcess; i++ )
	{
		// Note: forceparse parameter of declManager->MaterialByIndex needs to be set to true to be able to retrieve the material source.
		idMaterial *pMat = const_cast<idMaterial *>(declManager->MaterialByIndex( i ));

		if( NULL == pMat )
			continue;

		eMaterialConversionStatus matConversionStatus = ConvertMaterial( pMat, bForceUpdateAllMaterials );

		// for testing
		//idMaterial *pMat = const_cast<idMaterial *>(declManager->FindMaterial( "textures/base_wall/xiantex3_dark_burn" ));

		gameLocal.Printf( "%s", matStatusReport[matConversionStatus].m_strErrorMessage.c_str() );

		if( eMaterialConversionStatus_Skipped_ErrorsInConvertedMaterial == matConversionStatus )
		{
			gameLocal.Printf(" Aborting Batch conversion at %s.\n", pMat->GetName() );
			break;
		}

		// Update the statistics for material conversion report.
		matStatusReport[matConversionStatus]++;
	}
	gameLocal.Printf( "Total materials processed: %lu\n", i );

	// Print the material conversion status report to console. 
	gameLocal.Printf( matStatusReport[eMaterialConversionStatus_SuccessFullyConverted].m_strStatusReport, matStatusReport[eMaterialConversionStatus_SuccessFullyConverted].m_uiCount );
	gameLocal.Printf( matStatusReport[eMaterialConversionStatus_Skipped_AlreadyConverted].m_strStatusReport, matStatusReport[eMaterialConversionStatus_Skipped_AlreadyConverted].m_uiCount );
	gameLocal.Printf( matStatusReport[eMaterialConversionStatus_Skipped_NotDynamicallyLit].m_strStatusReport, matStatusReport[eMaterialConversionStatus_Skipped_NotDynamicallyLit].m_uiCount );
}


//-------------------------------------------------------------
// MaterialConverter::Cmd_ConvertMaterialsFromFile_f
//
// Description: Console command callback for tdm_convertMaterialsFromFile. 
//-------------------------------------------------------------
void MaterialConverter::Cmd_ConvertMaterialsFromFile_f( const idCmdArgs& args )
{

	if( args.Argc() < 2 )
	{
		gameLocal.Printf( "Usage: tdm_convertMaterialsFromFile <material-file> [forceUpdateAll] \n" );
		return;
	}

 	bool bForceUpdateAllMaterials = false;
 	if( args.Argc() > 2 )
 	{
 		bForceUpdateAllMaterials = (0 == idStr::Icmp( args.Argv(2), "forceupdateall" ));
 	}

 
	const unsigned long uiTotalMats = declManager->GetNumDecls( DECL_MATERIAL );

	gameLocal.Printf( "Finding materials from file %s \n", args.Argv(1) );

	std::vector<idStr> astrMatNames;
	unsigned int i;
	for( i = 0; i < uiTotalMats; i++ )
	{
		const idDecl *pDeclMaterial = declManager->DeclByIndex( DECL_MATERIAL, i, false );

		if( pDeclMaterial && 0 == idStr::Icmp( pDeclMaterial->GetFileName(), args.Argv(1) ) )
		{
			astrMatNames.push_back( idStr(pDeclMaterial->GetName()) );
		}
	}

	if( astrMatNames.size() <= 0 )
	{
		gameLocal.Printf( "Could not find any valid materials. Aborting. \n" );
		return;
	}

	gameLocal.Printf( "Found %u materials in total. \n", astrMatNames.size() );


	gameLocal.Printf("Parsing materials...\n" );

	MaterialConversionStatusReport matStatusReport;

	for ( std::vector<idStr>::iterator itr = astrMatNames.begin(); itr != astrMatNames.end(); itr++ )
	{

		idMaterial *pMat = const_cast<idMaterial *>(declManager->FindMaterial( *itr ));

		if( NULL == pMat )
			continue;

		eMaterialConversionStatus matConversionStatus = ConvertMaterial( pMat, bForceUpdateAllMaterials );

		gameLocal.Printf( "%s", matStatusReport[matConversionStatus].m_strErrorMessage.c_str() );

		if( eMaterialConversionStatus_Skipped_ErrorsInConvertedMaterial == matConversionStatus )
		{
			gameLocal.Printf(" Aborting Batch conversion at %s.\n", pMat->GetName() );
			break;
		}

		// Update the statistics for material conversion report.
		matStatusReport[matConversionStatus]++;
	}
	gameLocal.Printf( "Total materials processed: %u\n", astrMatNames.size() );

	// Print the material conversion status report to console. 
	gameLocal.Printf( matStatusReport[eMaterialConversionStatus_SuccessFullyConverted].m_strStatusReport, matStatusReport[eMaterialConversionStatus_SuccessFullyConverted].m_uiCount );
	gameLocal.Printf( matStatusReport[eMaterialConversionStatus_Skipped_AlreadyConverted].m_strStatusReport, matStatusReport[eMaterialConversionStatus_Skipped_AlreadyConverted].m_uiCount );
	gameLocal.Printf( matStatusReport[eMaterialConversionStatus_Skipped_NotDynamicallyLit].m_strStatusReport, matStatusReport[eMaterialConversionStatus_Skipped_NotDynamicallyLit].m_uiCount );
}

//-------------------------------------------------------------
// MaterialConverter::Cmd_ConvertMaterial_f
//
// Description: Console command callback for tdm_convertMaterial. 
//-------------------------------------------------------------
void MaterialConverter::Cmd_ConvertMaterial_f( const idCmdArgs& args )
{

	if( args.Argc() < 2 )
	{
		gameLocal.Printf( "Usage: tdm_convertMaterial <material-name> [forceUpdate] \n" );
		return;
	}

	bool bForceUpdate = false;
	if( args.Argc() > 2 )
	{
		bForceUpdate = (0 == idStr::Icmp( args.Argv(2), "forceupdate" ));
	}

	MaterialConversionStatusReport matStatusReport;

	idMaterial *pMat = const_cast<idMaterial *>(declManager->FindMaterial( args.Argv(1) ));

	if( NULL == pMat )
	{
		gameLocal.Printf( "Could not load material %s", args.Argv(1) );
		return;
	}
	eMaterialConversionStatus matConversionStatus = ConvertMaterial( pMat, bForceUpdate );

	gameLocal.Printf( "%s", matStatusReport[matConversionStatus].m_strErrorMessage.c_str() );
}


