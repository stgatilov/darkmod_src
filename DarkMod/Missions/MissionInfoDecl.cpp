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
	idLexer		src;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS & ~LEXFL_NOSTRINGESCAPECHARS );

	// greebo: Skip the declaration type and name
	src.SkipUntilString("{");

	idToken		key;
	idToken		value;

	while (true)
	{
		// If there's an EOF, this is an error.
		if (!src.ReadToken(&key))
		{
			src.Warning("Unclosed mission info declaration.");
			return false;
		}

		// Quit upon encountering the closing brace.
		if (key.type == TT_PUNCTUATION && key.subtype == P_BRACECLOSE)
		{
			break;
		}
		else if (key.type == TT_STRING)
		{
			// Found a string, this must be a key
			if (!src.ReadToken(&value))
			{
				src.Warning("Unexpected EOF in key/value pair.");
				return false;
			}

			if (value.type == TT_STRING)
			{
				// Save the key:value pair.
				data.Set(key.c_str(), value.c_str());
			}
			else
			{
				src.Warning("Invalid value: %s", value.c_str());
				continue;
			}
		}
		else 
		{
			src.Warning("Unrecognized token: %s", key.c_str());
			continue;
		}
	}

	return true;
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
