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
#include "Missions/Download.h"
#include "Missions/DownloadManager.h"

namespace
{
	inline const char* GetPlural(int num, const char* singular, const char* plural)
	{
		return (num == 1) ? singular : plural;
	}
}

CDownloadMenu::CDownloadMenu() :
	_availListTop(0),
	_selectedListTop(0)
{}

void CDownloadMenu::HandleCommands(const idStr& cmd, idUserInterface* gui)
{
	if (cmd == "mainmenu_heartbeat")
	{
		// Update download progress
		if (_selectedMissions.Num() > 0)
		{
			UpdateDownloadProgress(gui);
		}
	}
	else if (cmd == "refreshAvailableMissionList")
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

		_selectedMissions.Clear();

		UpdateGUI(gui);
		UpdateDownloadProgress(gui);
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
		UpdateDownloadProgress(gui);
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
	else if (cmd == "onSelectedMissionScrollUp")
	{
		int itemsPerPage = gui->GetStateInt("selectedPackagesPerPage", "5");
		_selectedListTop -= itemsPerPage;

		if (_selectedListTop < 0) _selectedListTop = 0;

		UpdateGUI(gui);
	}
	else if (cmd == "onSelectedMissionScrollDown")
	{
		int itemsPerPage = gui->GetStateInt("selectedPackagesPerPage", "5");
		_selectedListTop += itemsPerPage;

		UpdateGUI(gui);
	}
	else if (cmd == "onStartDownload")
	{
		StartDownload(gui);
		UpdateDownloadProgress(gui); // do this first
		UpdateGUI(gui);
	}
}

void CDownloadMenu::StartDownload(idUserInterface* gui)
{
	// Add a new download for each selected mission
	const DownloadableMissionList& missions = gameLocal.m_MissionManager->GetDownloadableMissions();

	for (int i = 0; i < _selectedMissions.Num(); ++i)
	{
		int missionIndex = _selectedMissions[i];

		if (missionIndex > missions.Num()) continue;

		const DownloadableMission& mission = missions[missionIndex];

		// Take the filename from the first URL
		idStr url = mission.downloadLocations[0];
		idStr filename;
		url.ExtractFileName(filename);

		idStr targetPath = g_Global.GetDarkmodPath().c_str();
		targetPath += "/";
		targetPath += cv_tdm_fm_path.GetString();
		targetPath += filename;

		CDownloadPtr download(new CDownload(mission.downloadLocations, targetPath));

		// Check for valid PK4 files after download
		download->EnableValidPK4Check(true);

		int id = gameLocal.m_DownloadManager->AddDownload(download);

		// Store this ID
		_downloads[missionIndex] = id;
	}

	// Let the download manager start its downloads
	gameLocal.m_DownloadManager->ProcessDownloads();
}

void CDownloadMenu::UpdateGUI(idUserInterface* gui)
{
	const DownloadableMissionList& missions = gameLocal.m_MissionManager->GetDownloadableMissions();

	gui->SetStateBool("av_no_download_available", missions.Num() == 0);

	bool downloadInProgress = gui->GetStateBool("mission_download_in_progress");

	int numMissionsPerPage = gui->GetStateInt("packagesPerPage", "5");

	for (int i = 0; i < numMissionsPerPage; ++i)
	{
		// Apply page offset
		int missionIndex = i + _availListTop;
		bool missionExists = missionIndex < missions.Num();

		bool missionSelected = _selectedMissions.FindIndex(missionIndex) != -1;
		gui->SetStateBool(va("av_mission_avail_%d", i), missionExists && !missionSelected && !downloadInProgress);
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

	gui->SetStateBool("dl_mission_scroll_up_visible", _selectedListTop > 0);
	gui->SetStateBool("dl_mission_scroll_down_visible", _selectedListTop + numSelectedMissionsPerPage < _selectedMissions.Num());

	gui->SetStateInt("dl_mission_count", _selectedMissions.Num());
	gui->SetStateBool("dl_button_available", _selectedMissions.Num() > 0 && !downloadInProgress);
	gui->SetStateBool("dl_button_visible", !downloadInProgress);

	gui->HandleNamedEvent("UpdateAvailableMissionColours");
	gui->HandleNamedEvent("UpdateSelectedMissionColours");
}

void CDownloadMenu::UpdateDownloadProgress(idUserInterface* gui)
{
	int numSelectedMissionsPerPage = gui->GetStateInt("selectedPackagesPerPage", "5");

	// Missions in the download queue
	for (int i = 0; i < numSelectedMissionsPerPage; ++i)
	{
		// Apply page offset
		int listIndex = i + _selectedListTop;
		bool listItemExists = listIndex < _selectedMissions.Num();

		// Get the referenced mission index, -1 ==> no mission
		int missionIndex = listItemExists ? _selectedMissions[listIndex] : -1;

		if (!listItemExists || missionIndex == -1)
		{
			gui->SetStateString(va("dl_mission_progress_%d", i), "");
			continue;
		}

		// Find the download ID
		ActiveDownloads::const_iterator it = _downloads.find(missionIndex);

		if (it == _downloads.end())
		{
			gui->SetStateString(va("dl_mission_progress_%d", i), "queued ");
			continue;
		}
		
		CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(it->second);

		if (download == NULL) continue;

		switch (download->GetStatus())
		{
		case CDownload::NOT_STARTED_YET:
			gui->SetStateString(va("dl_mission_progress_%d", i), "queued ");
			break;
		case CDownload::FAILED:
			gui->SetStateString(va("dl_mission_progress_%d", i), "failed ");
			break;
		case CDownload::IN_PROGRESS:
			gui->SetStateString(va("dl_mission_progress_%d", i), va("%0.0f%s", download->GetProgressFraction()*100, "% "));
			break;
		case CDownload::SUCCESS:
			gui->SetStateString(va("dl_mission_progress_%d", i), "100% ");
			break;
		};
	}

	bool previousValue = gui->GetStateBool("mission_download_in_progress");
	bool newValue = gameLocal.m_DownloadManager->DownloadInProgress();

	gui->SetStateBool("mission_download_in_progress", newValue);

	if (previousValue != newValue)
	{
		gui->HandleNamedEvent("UpdateAvailableMissionColours");

		if (newValue == false)
		{
			// Fire the "finished downloaded" event
			ShowDownloadResult(gui);
		}
	}
}

void CDownloadMenu::ShowDownloadResult(idUserInterface* gui)
{
	int successfulDownloads = 0;
	int failedDownloads = 0;

	for (ActiveDownloads::iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(i->second);

		if (download == NULL) continue;

		switch (download->GetStatus())
		{
		case CDownload::NOT_STARTED_YET:
			// ??
			gameLocal.Warning("Some downloads haven't been processed?");
			break;
		case CDownload::FAILED:
			failedDownloads++;
			break;
		case CDownload::IN_PROGRESS:
			gameLocal.Warning("Some downloads still in progress?");
			break;
		case CDownload::SUCCESS:
			successfulDownloads++;
			break;
		};
	}

	gameLocal.Printf("Successful downloads: %d\nFailed downloads: %d\n", successfulDownloads, failedDownloads);

	// Display the popup box
	GuiMessage msg;
	msg.type = GuiMessage::MSG_OK;
	msg.okCmd = "close_msg_box;refreshAvailableMissionList";
	msg.title = "Mission Download Result";
	msg.message = "";

	if (successfulDownloads > 0)
	{
		msg.message += va("%d %s been successfully downloaded. "
						  "You'll find the %s in the 'New Mission' page.", 
						  successfulDownloads, 
						  GetPlural(successfulDownloads, "mission has", "missions have"),
						  GetPlural(successfulDownloads, "mission", "missions")); 
	}
	
	if (failedDownloads > 0)
	{
		msg.message += va("\n%d %s couldn't be downloaded. "
						  "Please check your disk space (or maybe some file is "
						  "write protected) and try again.", failedDownloads,
						  GetPlural(failedDownloads, "mission", "missions"));
	}

	gameLocal.AddMainMenuMessage(msg);

	// Remove all downloads
	for (ActiveDownloads::iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		gameLocal.m_DownloadManager->RemoveDownload(i->second);
	}
}
