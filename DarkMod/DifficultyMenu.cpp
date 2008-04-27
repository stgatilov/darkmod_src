#include "../idlib/precompiled.h"
#pragma hdrstop
#include "DifficultyMenu.h"
#include "../DarkMod/shop.h"
#include "../DarkMod/MissionData.h"

CDifficultyMenu::CDifficultyMenu()
{
}

CDifficultyMenu::~CDifficultyMenu()
{
}


// Handle mainmenu commands
void CDifficultyMenu::HandleCommands(const char *menuCommand, idUserInterface *gui)
{
	if (idStr::Icmp(menuCommand, "diffLoad") == 0)
	{
		// type-in field for map name (temporary)
		idCVar tdm_mapName( "tdm_mapName", "", CVAR_GUI, "" );

		// New game, determine the map
		char * mapName = NULL;
		char * startingMap = NULL;
		idLib::fileSystem->ReadFile("startingMap.txt", (void**) &startingMap);
		
		if (mapName == NULL) {
			gameLocal.Warning( "Couldn't open startingMap.txt file" );
			mapName = va("%s", tdm_mapName.GetString());
		} else {
			mapName = startingMap;
		}
		InitializeDifficulty(gui, mapName);
		if (startingMap != NULL) {
			idLib::fileSystem->FreeFile((void*)startingMap);
		}

		// show the top of the "easy" list
		scrollPos = 0;
		g_skill.SetInteger(0);
		GenerateObjectivesDisplay();
		DisplayDifficulty(gui);
	}
	else if (idStr::Icmp(menuCommand, "diffSelect") == 0)
	{
		// change the difficulty (skill) level to selected value, redisplay objectives
		scrollPos = 0;
		g_skill.SetInteger(gui->GetStateInt("diffSelect", "0"));

		// greebo: Tell the Difficulty Manager the chosen difficulty
		gameLocal.m_DifficultyManager.SetDifficultyLevel(gui->GetStateInt("diffSelect", "0"));

		GenerateObjectivesDisplay();
		DisplayDifficulty(gui);
	}
	else if (idStr::Icmp(menuCommand, "diffUp") == 0)
	{
		// scroll up one objective
		scrollPos -= scrollPos == 0 ? 0 : 1;
		GenerateObjectivesDisplay();
		DisplayDifficulty(gui);
	}
	else if (idStr::Icmp(menuCommand, "diffDown") == 0)
	{
		// scroll down one objective
		scrollPos += scrollPos + 1 >= diffObjectives[g_skill.GetInteger()].Num() ? 0 : 1;
		GenerateObjectivesDisplay();
		DisplayDifficulty(gui);
	}
}

// set the "objectives" GUI variable to the difficulty-based objectives string
void CDifficultyMenu::DisplayDifficulty(idUserInterface *gui)
{
	gui->SetStateString("objectives", objectivesDisplay);
}

// generate a list of objective description strings for each difficulty level
void CDifficultyMenu::InitializeDifficulty(idUserInterface *gui, const char * mapName)
{
	// clear out objectives
	for (int i = 0; i < DIFFICULTY_COUNT; i++) {
		diffObjectives[i].Clear();
	}

	// read the map
	const char * filename = va("maps/%s", mapName);
	scrollPos = 0;
	idMapFile* mapFile = new idMapFile;
	if ( !mapFile->Parse( idStr( filename ) + ".map") ) {
		delete mapFile;
		mapFile = NULL;
		gameLocal.Warning( "Couldn't load %s", filename );
		return;
	}
	gui->SetStateString("mapStartCmd", va("exec 'map %s'", mapName));
	idMapEntity* worldSpawn = mapFile->GetEntity( 0 );
	idDict mapDict = worldSpawn->epairs;

	if (mapDict.GetInt("shop_skip", "0") == 1) {
		// skip the shop, so define the map start command now
		gui->SetStateString("mapStartCmdNow", va("exec 'map %s'", mapName));
	} else {
		// there will be a shop, so don't run the map right away
		gui->SetStateString("mapStartCmdNow", "");
	}

	// Load the difficulty level strings.
	for (int diffLevel = 0; diffLevel < DIFFICULTY_COUNT; diffLevel++)
	{
		gui->SetStateString(va("diff%dName",diffLevel), gameLocal.m_DifficultyManager.GetDifficultyName(diffLevel).c_str());
	}

	// Show/hide appropriate menus
	gui->SetStateInt("isDiffMenuVisible", 1);
	gui->SetStateInt("isNewGameRootMenuVisible", 0);

	// Search map for objectives
	idMapEntity* objectiveEnt = NULL;
	for (int entNum = 0; entNum < mapFile->GetNumEntities(); entNum++)
	{
		idMapEntity* ent = mapFile->GetEntity(entNum);
		if (ent != NULL && idStr::Icmp(ent->epairs.GetString("classname"), "target_tdm_addobjectives") == 0
			&& !ent->epairs.GetBool("wait_for_trigger"))
		{
			objectiveEnt = ent;
			break;
		}
	}

	// found some objectives?
	if (objectiveEnt != NULL)
	{
		idDict objDict = objectiveEnt->epairs;
		int objCount = 0;
		idStr prefix;
		idStr diffs;
		idLexer	src;
		idToken	token;

		// loop through each objective
		while (true)
		{
			objCount++;
			prefix = va("obj%d_", objCount);

			if (objDict.FindKeyIndex(prefix + "desc") == -1)
			{
				// no more objectives, exit
				break;
			}

			// If this objective is:
			//  * not waiting for a trigger, and
			//  * visible, and
			//  * state is incomplete, or state is complete and it is ongoing
			// then it can be displayed
			if (!objDict.GetBool(prefix + "wait_for_trigger", "0") &&
				objDict.GetBool(prefix + "visible", "1") &&
				(objDict.GetInt(prefix + "state", "0") == STATE_INCOMPLETE ||
				(objDict.GetInt(prefix + "state", "0") == STATE_COMPLETE &&
				 objDict.GetBool(prefix + "ongoing", "0"))))
			{
				// Parse difficulty level. If difficulty not specified, then
				// this objective applies to all levels.
				diffs = objDict.GetString( prefix + "difficulty", "" );
				bool diffApplies[DIFFICULTY_COUNT] = {true};
				for (int diffCount = 0; diffCount < DIFFICULTY_COUNT; diffCount++)
				{
					diffApplies[diffCount] = true;
				}
				if (diffs.Length() > 0) {
					// Difficulties are specified for this objective, so initialize to false
					for (int diffCount = 0; diffCount < DIFFICULTY_COUNT; diffCount++)
					{
						diffApplies[diffCount] = false;
					}
					src.LoadMemory( diffs.c_str(), diffs.Length(), "" );
					while( src.ReadToken( &token ) )
					{
						if( token.IsNumeric() )
						{
							int diffValue = token.GetIntValue();
							if (diffValue >= 0 && diffValue <= DIFFICULTY_COUNT-1)
							{
								// Objective applies to specified level of difficulty
								diffApplies[diffValue] = true;
							}
						}
					}
					src.FreeSource();
				}
				
				// Add objective descriptoin to each difficulties' list of objectives
				for (int diffV = 0; diffV < DIFFICULTY_COUNT; diffV++)
				{
					if (diffApplies[diffV])
					{
						diffObjectives[diffV].Append(objDict.GetString(prefix + "desc", "0"));
					}
				}
			}
		}
	}
}

// Generate a single string containing all objective descriptions for the current
// difficulty level. Separate each objective description with a blank line.
void CDifficultyMenu::GenerateObjectivesDisplay()
{
	objectivesDisplay.Clear();
	int difficulty = g_skill.GetInteger();
	int numObjs = diffObjectives[difficulty].Num();
	for (int i = scrollPos; i < numObjs; i++)
	{
		if (i > scrollPos)
		{
			objectivesDisplay.Append("\n\n");
		}
		objectivesDisplay.Append(diffObjectives[difficulty][i]);
	}
}
