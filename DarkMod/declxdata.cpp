/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.2  2006/03/23 14:13:38  gildoran
 * Added import command to xdata decls.
 *
 *
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "declxdata.h"

tdmDeclXData::tdmDeclXData()
{
}

tdmDeclXData::~tdmDeclXData()
{
	FreeData();
}

size_t tdmDeclXData::Size() const
{
	return sizeof(tdmDeclXData);
}

const char *tdmDeclXData::DefaultDefinition() const
{
	return "{}";
}

void tdmDeclXData::FreeData()
{
	m_data.Clear();
}

bool tdmDeclXData::Parse( const char *text, const int textLength )
{
	idLexer		src;
	idToken		tKey;
	idToken		tVal;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS & ~LEXFL_NOSTRINGESCAPECHARS );
	src.SkipUntilString( "{" );

	bool		precache = false;
	idStr		value;
	idDict		importKeys;
	const tdmDeclXData *xd;
	const idDict *importData;
	const idKeyValue *kv;
	const idKeyValue *kv2;
	int i;

	while (1)
	{
		// Quit upon EOF or closing brace.
		if ( !src.ReadToken( &tKey ) ||
			(tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACECLOSE) ) {
			break;
		}

		if ( tKey.type == TT_STRING ) {

			if ( !src.ReadToken( &tVal ) ||
				 tVal.type != TT_PUNCTUATION ||
				 tVal.subtype != P_COLON ) {
				src.Warning( "Abandoned key: %s", tKey.c_str() );
				MakeDefault();
				return false;
			}

			// We're parsing a key/value pair.
			if ( !src.ReadToken( &tVal ) ) {
				src.Warning("Unexpected EOF in key:value pair.");
				MakeDefault();
				return false;
			}

			if ( tVal.type == TT_STRING ) {

				// Set the key:value pair.
				m_data.Set( tKey.c_str(), tVal.c_str() );

			} else if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_BRACEOPEN ) {

				value = "";

				while (1) {
					if ( !src.ReadToken( &tVal ) ) {
						src.Warning("EOF encountered inside value block.");
						MakeDefault();
						return false;
					}

					if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_BRACECLOSE ) {
						break;
					}

					if ( tVal.type != TT_STRING ) {
						src.Warning( "Non-string encountered in value block: %s", tVal.c_str() );
						MakeDefault();
						return false;
					}

					value += tVal + "\n";
				}

				// Set the key:value pair.
				m_data.Set( tKey.c_str(), value.c_str() );

			} else {
				src.Warning( "Invalid value: %s", tVal.c_str() );
				MakeDefault();
				return false;
			}

		} else if ( tKey.type == TT_NAME ) {

			if ( tKey.Icmp("precache") == 0 ) {
				precache = true;
			} else if ( tKey.Icmp("import") == 0 ) {

				if ( !src.ReadToken( &tKey ) )
				{
					src.Warning("Unexpected EOF in import statement.");
					MakeDefault();
					return false;
				}

				if ( tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACEOPEN ) {

					// Initialize the list of keys to copy over.
					importKeys.Clear();

					while (1) {

						if ( !src.ReadToken( &tKey ) ) {
							src.Warning("Unexpected EOF in import block.");
							MakeDefault();
							return false;
						}

						if ( tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACECLOSE ) {
							break;
						}

						if ( tKey.type != TT_STRING ) {
							src.Warning( "Invalid source key: %s", tKey.c_str() );
							MakeDefault();
							return false;
						}

						if ( !src.ReadToken( &tVal ) ) {
							src.Warning("Unexpected EOF in import block.");
							MakeDefault();
							return false;
						}

						if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_POINTERREF ) {

							if ( !src.ReadToken( &tVal ) ) {
								src.Warning("Unexpected EOF in import block.");
								MakeDefault();
								return false;
							}

							if ( tVal.type != TT_STRING ) {
								src.Warning( "Invalid target key: %s", tVal.c_str() );
								MakeDefault();
								return false;
							}

							importKeys.Set( tKey.c_str(), tVal.c_str() );

						} else {

							// We accidently read too far.
							src.UnreadToken( &tVal );

							importKeys.Set( tKey.c_str(), tKey.c_str() );
						}

					}

					if ( !src.ReadToken( &tKey ) ||
						 tKey.type != TT_NAME ||
						 tKey.Icmp("from") != 0 ) {
						src.Warning( "Missing from statement.", tKey.c_str() );
						MakeDefault();
						return false;
					}

					if ( !src.ReadToken( &tKey ) ||
						 tKey.type != TT_STRING ) {
						src.Warning( "Invalid xdata for importation." );
						MakeDefault();
						return false;
					}

					xd = static_cast< const tdmDeclXData* >( declManager->FindType( DECL_XDATA, tKey.c_str(), false ) );
					if ( xd != NULL ) {

						importData = &(xd->m_data);

						i = importKeys.GetNumKeyVals();
						while (i--) {
							kv = importKeys.GetKeyVal(i);
							kv2 = importData->FindKey( kv->GetKey() );
							m_data.Set( kv->GetValue(), kv2->GetValue() );
						}

					} else {
						src.Warning( "Unable to load xdata for importation: %s", tKey.c_str() );
						MakeDefault();
						return false;
					}

				} else if ( tKey.type == TT_STRING ) {

					xd = static_cast< const tdmDeclXData* >( declManager->FindType( DECL_XDATA, tKey.c_str(), false ) );
					if ( xd != NULL ) {
						m_data.Copy( xd->m_data );
					} else {
						src.Warning( "Unable to load xdata for importation: %s", tKey.c_str() );
						MakeDefault();
						return false;
					}

				} else {
					src.Warning("Syntax error immediately after import statement.");
					MakeDefault();
					return false;
				}

			} else {
				src.Warning( "Unrecognized command: %s", tKey.c_str() );
				MakeDefault();
				return false;
			}

		}
	}

	if (precache) {
		gameLocal.CacheDictionaryMedia( &m_data );
	}

	return true;
}
