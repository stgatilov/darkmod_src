/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.3  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.2  2006/03/25 09:52:43  gildoran
 * Altered the parse functions for the decls I wrote to adhere to our coding standards.
 *
 * Revision 1.1  2006/03/25 08:13:46  gildoran
 * New update for declarations... Improved the documentation/etc for xdata decls, and added some basic code for tdm_matinfo decls.
 *
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "decltdm_matinfo.h"

tdmDeclTDM_MatInfo::tdmDeclTDM_MatInfo()
{
}

tdmDeclTDM_MatInfo::~tdmDeclTDM_MatInfo()
{
	FreeData();
}

size_t tdmDeclTDM_MatInfo::Size() const
{
	return sizeof(tdmDeclTDM_MatInfo);
}

const char *tdmDeclTDM_MatInfo::DefaultDefinition() const
{
	return "{}";
}

void tdmDeclTDM_MatInfo::FreeData()
{
	surfaceType = "";
}

// Note Our coding standards require using gotos in this sort of code.
bool tdmDeclTDM_MatInfo::Parse( const char *text, const int textLength )
{
	// Only set to true if we have successfully parsed the decl.
	bool		successfulParse = false;

	idLexer		src;
	idToken		token;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );

	// Only consider explicitly defined tdm_matinfo declarations...
	// All others are actual materials.
	if ( !src.ReadToken( &token ) ||
		 token.type != TT_NAME ||
		 token.Icmp( "tdm_matinfo" ) != 0 ) {
		goto Quit;
	}

	// Skip until the opening brace. I don't trust using the
	// skipUntilString function, since it could be fooled by
	// a string containing a brace.
	do {
		if ( !src.ReadToken( &token ) ) {
			MakeDefault();
			return false;
		}
	} while ( token.type != TT_PUNCTUATION || token.subtype != P_BRACEOPEN );
	//src.SkipUntilString( "{" );

	while (1)
	{
		// If there's an EOF, fail to load.
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Unclosed tdm_matinfo decl." );
			goto Quit;
		}

		// Quit upon encountering the closing brace.
		if ( token.type == TT_PUNCTUATION && token.subtype == P_BRACECLOSE) {
			break;
		}

		if ( token.type == TT_NAME ) {
			if ( token.Icmp( "surfacetype" ) == 0 ) {

				// surfacetype keyword
				// Syntax:  surfacetype <typename>
				// Example: surfacetype wood

				if ( !src.ReadToken( &token ) ) {
					src.Warning( "Unexpected EOF encountered." );
					goto Quit;
				} else if ( token.type != TT_NAME ) {
					src.Warning( "Invalid surface type: %s", token.c_str() );
					goto Quit;
				}

				if ( surfaceType.Length() > 0 ) {
					src.Warning( "Redefinition of surfaceType." );
				}

				surfaceType = token;

			} else {
				src.Warning( "Unrecognized keyword: %s", token.c_str() );
				goto Quit;
			}
		} else {
			src.Warning( "Invalid keyword: %s", token.c_str() );
			goto Quit;
		}
	}

	successfulParse = true;

	Quit:
	if (!successfulParse) {
		MakeDefault();
	}
	return successfulParse;
}
