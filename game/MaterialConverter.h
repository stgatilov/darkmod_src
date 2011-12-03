/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef TDM_MATERIAL_CONVERTER_H_
#define TDM_MATERIAL_CONVERTER_H_

//-------------------------------------------------------------
// We do not account for centerScale or Scroll for now.
typedef struct _ImageInfo{
	idStr m_strImageName;
	idStr m_strUVScale;

	_ImageInfo() :
	m_strImageName(),
		m_strUVScale("1, 1")
	{
	}
}ImageInfo_s;

enum eVertexBlendType {
	eVertexBlendType_None,
	eVertexBlendType_VertexColored,
	eVertexBlendType_InvVertexColored
};

enum eMaterialConversionStatus
{
	eMaterialConversionStatus_InvalidMaterial,
	eMaterialConversionStatus_Skipped_NotDynamicallyLit,
	eMaterialConversionStatus_Skipped_AlreadyConverted,
	eMaterialConversionStatus_Skipped_CouldNotFindEndOfBlock,
	eMaterialConversionStatus_Skipped_ErrorsInConvertedMaterial,
	eMaterialConversionStatus_SuccessFullyConverted
};

//------------------------------------------------------------------
// Struct to use for material conversion report generation
//------------------------------------------------------------------
struct ConversionStat_s
{
	idStr m_strErrorMessage;
	idStr m_strStatusReport;
	unsigned int m_uiCount;

	ConversionStat_s()
	{
	}

	ConversionStat_s( const char *a_strDesc, const char * a_strStatusReport ) :
	m_strErrorMessage	( a_strDesc ),
		m_strStatusReport	( a_strStatusReport ),
		m_uiCount(0)
	{
	}
	ID_INLINE void operator ++( int )
	{
		m_uiCount ++;
	}
};
//------------------------------------------------------------------
typedef std::map< eMaterialConversionStatus, ConversionStat_s > ErrorMessageMap;
typedef std::multimap< eVertexBlendType, ImageInfo_s > ImageInfoMap;
//------------------------------------------------------------------


//------------------------------------------------------------------
// A simple status report generator of material conversion process
//------------------------------------------------------------------
class MaterialConversionStatusReport
{
private:
	ErrorMessageMap arrErrorMessages;

public:
	MaterialConversionStatusReport();

	ID_INLINE ConversionStat_s &operator [] ( eMaterialConversionStatus a_matStatus  )
	{
			return arrErrorMessages[ a_matStatus ];
	}
};

//------------------------------------------------------------------
// A helper class that can be used for any material parser
//------------------------------------------------------------------
class MaterialParsingHelper
{
public:
	static bool GetValidStageExpression		( idLexer &a_lexSource, idStr & a_strStageTextureName );
	static void GetMaterialStageInfo		( const char* a_strMatStageName, idLexer &a_lexSource, ImageInfoMap & a_arrMatStageInfo );
	static bool FindBlockContainingWords	( const char *a_text, std::vector<idStr>& a_arrSearchWords, unsigned int & a_uiStartOffset, unsigned int & a_uiEndOffset,
											  const char a_cBlockStart = '{', const char a_cBlockEnd = '}' );
};

//------------------------------------------------------------------
// A helper class that can be used for any material parser
//------------------------------------------------------------------
class MaterialConverter
{
public:
	static void							CreateNewAmbientBlock			( const ImageInfoMap& a_arrDiffusemapInfo, const ImageInfoMap& a_arrBumpmapInfo, const ImageInfoMap& a_arrSpecularmapInfo, 
																		  std::vector<char>& a_arrCharNewAmbientBlock );
	static eMaterialConversionStatus	ConvertMaterial					( idMaterial *a_pMaterial, bool a_bForceUpdate = false );


	static void							Cmd_BatchConvertMaterials_f		( const idCmdArgs& args );
	static void							Cmd_ConvertMaterialsFromFile_f	( const idCmdArgs& args );
	static void							Cmd_ConvertMaterial_f			( const idCmdArgs& args );

	ID_INLINE static void				ArgCompletion_MaterialFileName	( const idCmdArgs &args, void(*callback)( const char *s ) )
	{
		cmdSystem->ArgCompletion_FolderExtension( args, callback, "materials/", false, ".mtr", NULL );
	}
};
//------------------------------------------------------------------

#endif
