#include "../idlib/precompiled.h"
#pragma hdrstop
#include ".\difficultymenu.h"

CDifficultyMenu::CDifficultyMenu(void)
{
}

CDifficultyMenu::~CDifficultyMenu(void)
{
}

void CDifficultyMenu::HandleCommands(const char *menuCommand, idUserInterface *gui)
{
	if (idStr::Icmp(menuCommand, "diffLoad") == 0)
	{
		// Initialize the shop
		//Init();
		DisplayDifficulty(gui);
	}
}

void CDifficultyMenu::DisplayDifficulty(idUserInterface *gui)
{
	const char * mapName = tdm_mapName.GetString();
	const char * filename = va("maps/%s", mapName);

	idMapFile* mapFile = new idMapFile;
	if ( !mapFile->Parse( idStr( filename ) + ".map") ) {
		delete mapFile;
		mapFile = NULL;
		gameLocal.Warning( "Couldn't load %s", filename );
		return;
	}
	idMapEntity* mapEnt = mapFile->GetEntity( 0 );
	idDict mapDict = mapEnt->epairs;

	const idDecl * diffDecl = declManager->FindType(DECL_ENTITYDEF, "difficultyMenu", false);
	const idDeclEntityDef *diffDef = static_cast<const idDeclEntityDef *>( diffDecl );
	const char* diff0 = mapDict.GetString("difficulty0Name", diffDef->dict.GetString("diff0default", ""));
	const char* diff1 = mapDict.GetString("difficulty1Name", diffDef->dict.GetString("diff0default", ""));
	const char* diff2 = mapDict.GetString("difficulty2Name", diffDef->dict.GetString("diff0default", ""));
	gui->SetStateInt("isDiffMenuVisible", 1);
	gui->SetStateInt("isNewGameRootMenuVisible", 0);
	gui->SetStateString("diff0", diff0);
	gui->SetStateString("diff1", diff1);
	gui->SetStateString("diff2", diff2);
}