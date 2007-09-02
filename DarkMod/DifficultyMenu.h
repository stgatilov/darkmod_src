#ifndef __DIFF_H__
#define	__DIFF_H__

#pragma once

// number of difficulty levels
#define DIFFICULTY_COUNT 3

class CDifficultyMenu
{
public:
	CDifficultyMenu();
	~CDifficultyMenu();

	// handles main menu commands
	void HandleCommands(const char *menuCommand, idUserInterface *gui);

	// initialize the difficulty data (read in objectives)
	void InitializeDifficulty(idUserInterface *gui, const char * mapName);

	// set the GUI variables to display difficulty/objectives
	void DisplayDifficulty(idUserInterface *gui);

	// generate the list of objectives based on difficulty choice
	void GenerateObjectivesDisplay();

private:
	idList<const char *> diffObjectives[DIFFICULTY_COUNT]; 
	idStr	objectivesDisplay;
	int		scrollPos;
};

#endif	/* !__DIFF_H__ */
