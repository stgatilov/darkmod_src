/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "MissionInfoDecl.h"

const char* const CMissionInfoDecl::TYPE_NAME = "tdm_missioninfo";

CMissionInfoDecl::~CMissionInfoDecl()
{
	FreeData();
}

size_t CMissionInfoDecl::Size() const
{
	return sizeof(CMissionInfoDecl);
}

const char* CMissionInfoDecl::DefaultDefinition() const
{
	return "{}";
}

void CMissionInfoDecl::FreeData()
{
	data.Clear();
}

bool CMissionInfoDecl::Parse( const char *text, const int textLength )
{
	// Only set to true if we have successfully parsed the decl.
	bool		successfulParse = false;

	/*idLexer		src;
	idToken		tKey;
	idToken		tVal;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS & ~LEXFL_NOSTRINGESCAPECHARS );

	bool		precache = false;
	idStr		value;
	idDict		importKeys;
	const tdmDeclXData *xd;
	const idDict *importData;
	const idKeyValue *kv;
	const idKeyValue *kv2;
	int i;

	// Skip until the opening brace. I don't trust using the
	// skipUntilString function, since it could be fooled by
	// a string containing a brace.
	do {
		if ( !src.ReadToken( &tKey ) ) {
			goto Quit;
		}
	} while ( tKey.type != TT_PUNCTUATION || tKey.subtype != P_BRACEOPEN );
	//src.SkipUntilString( "{" );

	while (1)
	{
		// If there's an EOF, fail to load.
		if ( !src.ReadToken( &tKey ) ) {
			src.Warning( "Unclosed xdata decl." );
			goto Quit;
		}

		// Quit upon encountering the closing brace.
		if ( tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACECLOSE) {
			break;
		}

		if ( tKey.type == TT_STRING ) {

			if ( !src.ReadToken( &tVal ) ||
				 tVal.type != TT_PUNCTUATION ||
				 tVal.subtype != P_COLON ) {
				src.Warning( "Abandoned key: %s", tKey.c_str() );
				goto Quit;
			}

			// We're parsing a key/value pair.
			if ( !src.ReadToken( &tVal ) ) {
				src.Warning("Unexpected EOF in key:value pair.");
				goto Quit;
			}

			if ( tVal.type == TT_STRING ) {

				// Set the key:value pair.
				m_data.Set( tKey.c_str(), tVal.c_str() );

			} else if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_BRACEOPEN ) {

				value = "";

				while (1) {
					if ( !src.ReadToken( &tVal ) ) {
						src.Warning("EOF encountered inside value block.");
						goto Quit;
					}

					if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_BRACECLOSE ) {
						break;
					}

					if ( tVal.type != TT_STRING ) {
						src.Warning( "Non-string encountered in value block: %s", tVal.c_str() );
						goto Quit;
					}

					value += tVal + "\n";
				}

				// Set the key:value pair.
				m_data.Set( tKey.c_str(), value.c_str() );

			} else {
				src.Warning( "Invalid value: %s", tVal.c_str() );
				goto Quit;
			}

		} else if ( tKey.type == TT_NAME ) {

			if ( tKey.Icmp("precache") == 0 ) {
				precache = true;
			} else if ( tKey.Icmp("import") == 0 ) {

				if ( !src.ReadToken( &tKey ) )
				{
					src.Warning("Unexpected EOF in import statement.");
					goto Quit;
				}

				if ( tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACEOPEN ) {

					// Initialize the list of keys to copy over.
					importKeys.Clear();

					while (1) {

						if ( !src.ReadToken( &tKey ) ) {
							src.Warning("Unexpected EOF in import block.");
							goto Quit;
						}

						if ( tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACECLOSE ) {
							break;
						}

						if ( tKey.type != TT_STRING ) {
							src.Warning( "Invalid source key: %s", tKey.c_str() );
							goto Quit;
						}

						if ( !src.ReadToken( &tVal ) ) {
							src.Warning("Unexpected EOF in import block.");
							goto Quit;
						}

						if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_POINTERREF ) {

							if ( !src.ReadToken( &tVal ) ) {
								src.Warning("Unexpected EOF in import block.");
								goto Quit;
							}

							if ( tVal.type != TT_STRING ) {
								src.Warning( "Invalid target key: %s", tVal.c_str() );
								goto Quit;
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
						src.Warning( "Missing from statement: %s.", tKey.c_str() );
						goto Quit;
					}

					if ( !src.ReadToken( &tKey ) ||
						 tKey.type != TT_STRING ) {
						src.Warning( "Invalid xdata for importation." );
						goto Quit;
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
						goto Quit;
					}

				} else if ( tKey.type == TT_STRING ) {

					xd = static_cast< const tdmDeclXData* >( declManager->FindType( DECL_XDATA, tKey.c_str(), false ) );
					if ( xd != NULL ) {
						m_data.Copy( xd->m_data );
					} else {
						src.Warning( "Unable to load xdata for importation: %s", tKey.c_str() );
						goto Quit;
					}

				} else {
					src.Warning("Syntax error immediately after import statement.");
					goto Quit;
				}

			} else {
				src.Warning( "Unrecognized command: %s", tKey.c_str() );
				goto Quit;
			}

		}
	}

	if (precache) {
		gameLocal.CacheDictionaryMedia( &m_data );
	}

	successfulParse = true;

	Quit:
	if (!successfulParse) {
		MakeDefault();
	}*/
	return successfulParse;
}

void CMissionInfoDecl::Update(const idStr& name)
{
	idStr body;
	
	body += TYPE_NAME;
	body += " " + name;
	body += "\n{\n";

	// Dump the keyvalues
	for (int i = 0; i < data.GetNumKeyVals(); ++i)
	{
		const idKeyValue* kv = data.GetKeyVal(i);

		body += "\t\"" + kv->GetKey() + "\"";
		body += "\t\"" + kv->GetValue() + "\"\n";
	}

	body += " test ";

	body += "\n}\n\n";

	this->SetText(body.c_str());
}

CMissionInfoDecl* CMissionInfoDecl::Find(const idStr& name)
{
	return const_cast<CMissionInfoDecl*>(static_cast<const CMissionInfoDecl*>(
		declManager->FindType(DECL_TDM_MISSIONINFO, name.c_str(), false)
	));
}

// Creates a new declaration with the given name, in the given filename
CMissionInfoDecl* CMissionInfoDecl::Create(const idStr& name)
{
	return static_cast<CMissionInfoDecl*>(
		declManager->CreateNewDecl(DECL_TDM_MISSIONINFO, name.c_str(), cv_default_mission_info_file.GetString())
	);
}

CMissionInfoDecl* CMissionInfoDecl::FindOrCreate(const idStr& name)
{
	CMissionInfoDecl* decl = Find(name);
	
	if (decl == NULL)
	{
		decl = Create(name);
	}

	return decl;
}
