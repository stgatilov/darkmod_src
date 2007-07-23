#ifndef __DIFF_H__
#define	__DIFF_H__

#pragma once

class CDifficultyMenu
{
public:
	CDifficultyMenu(void);
	~CDifficultyMenu(void);

	// handles main menu commands
	void HandleCommands(const char *menuCommand, idUserInterface *gui);
	void CDifficultyMenu::DisplayDifficulty(idUserInterface *gui);

};

#endif	/* !__DIFF_H__ */