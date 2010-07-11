/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "DownloadMenu.h"
#include "Missions/MissionManager.h"

CDownloadMenu::CDownloadMenu() :
	_availListTop(0),
	_selectedListTop(0)
{}

void CDownloadMenu::HandleCommands(const idStr& cmd, idUserInterface* gui)
{
	if (cmd == "refreshAvailableMissionList")
	{
		gui->HandleNamedEvent("onAvailableMissionsRefreshed"); // hide progress dialog in any case

		if (!cv_tdm_allow_http_access.GetBool() || gameLocal.m_HttpConnection == NULL)
		{
			// HTTP Access disallowed, display message
			gameLocal.Printf("HTTP requests disabled, cannot check for available missions.\n");

			GuiMessage msg;
			msg.type = GuiMessage::MSG_OK;
			msg.okCmd = "close_msg_box";
			msg.title = "Unable to contact Mission Archive";
			msg.message = "HTTP Requests have been disabled,\n cannot check for available missions.";

			gameLocal.AddMainMenuMessage(msg);

			return;
		}

		// Refresh list
		gameLocal.m_MissionManager->ReloadDownloadableMissions();

		UpdateGUI(gui);
	}
	else if (cmd == "onDownloadableMissionSelected")
	{
		int selectedMission = gui->GetStateInt("av_mission_selected");

		int missionIndex = selectedMission + _availListTop;

		// Update mission details
		const DownloadableMissionList& missions = gameLocal.m_MissionManager->GetDownloadableMissions();

		if (missionIndex > missions.Num()) return;

		gui->SetStateString("av_mission_title", missions[missionIndex].title);
		gui->SetStateString("av_mission_author", missions[missionIndex].author);
		gui->SetStateString("av_mission_release_date", missions[missionIndex].releaseDate);
		gui->SetStateString("av_mission_size", va("%0.1f MB", missions[missionIndex].sizeMB));

		gui->SetStateBool("av_mission_details_visible", true);

		gui->HandleNamedEvent("UpdateAvailableMissionColours");
	}
	else if (cmd == "onSelectMissionForDownload")
	{
		int selectedMission = gui->GetStateInt("av_mission_selected");
		int missionIndex = selectedMission + _availListTop;

		_selectedMissions.AddUnique(missionIndex);

		gui->SetStateInt("av_mission_selected", -1);
		gui->SetStateBool("av_mission_details_visible", false);

		UpdateGUI(gui);
	}
	else if (cmd == "onDeselectMissionForDownload")
	{
		int selectedItem = gui->GetStateInt("dl_mission_selected");
		int index = selectedItem + _selectedListTop;

		if (index > _selectedMissions.Num()) return;

		_selectedMissions.Remove(_selectedMissions[index]);

		UpdateGUI(gui);
	}
	else if (cmd == "onDownloadableMissionScrollUp")
	{
		int numMissionsPerPage = gui->GetStateInt("packagesPerPage", "5");
		_availListTop -= numMissionsPerPage;

		if (_availListTop < 0) _availListTop = 0;

		UpdateGUI(gui);
	}
	else if (cmd == "onDownloadableMissionScrollDown")
	{
		int numMissionsPerPage = gui->GetStateInt("packagesPerPage", "5");
		_availListTop += numMissionsPerPage;

		UpdateGUI(gui);
	}
}

void CDownloadMenu::UpdateGUI(idUserInterface* gui)
{
	const DownloadableMissionList& missions = gameLocal.m_MissionManager->GetDownloadableMissions();

	int numMissionsPerPage = gui->GetStateInt("packagesPerPage", "5");

	for (int i = 0; i < numMissionsPerPage; ++i)
	{
		// Apply page offset
		int missionIndex = i + _availListTop;
		bool missionExists = missionIndex < missions.Num();

		bool missionSelected = _selectedMissions.FindIndex(missionIndex) != -1;
		gui->SetStateBool(va("av_mission_avail_%d", i), missionExists && !missionSelected);
		gui->SetStateBool(va("av_mission_selected_%d", i), missionSelected);
		gui->SetStateString(va("av_mission_name_%d", i), missionExists ? missions[missionIndex].title : "");
	}

	gui->SetStateBool("av_mission_scroll_up_visible", _availListTop > 0);
	gui->SetStateBool("av_mission_scroll_down_visible", _availListTop + numMissionsPerPage < missions.Num());

	int numSelectedMissionsPerPage = gui->GetStateInt("selectedPackagesPerPage", "5");

	// Missions in the download queue
	for (int i = 0; i < numSelectedMissionsPerPage; ++i)
	{
		// Apply page offset
		int listIndex = i + _selectedListTop;
		bool listItemExists = listIndex < _selectedMissions.Num();

		// Get the referenced mission index, -1 ==> no mission
		int missionIndex = listItemExists ? _selectedMissions[listIndex] : -1;

		gui->SetStateBool(va("dl_mission_avail_%d", i), listItemExists);
		gui->SetStateString(va("dl_mission_name_%d", i), missionIndex != -1 ? missions[missionIndex].title : "");
	}

	gui->HandleNamedEvent("UpdateAvailableMissionColours");
}
