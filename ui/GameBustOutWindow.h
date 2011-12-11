/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/
#ifndef __GAME_BUSTOUT_WINDOW_H__
#define __GAME_BUSTOUT_WINDOW_H__

class idGameBustOutWindow;

typedef enum {
	POWERUP_NONE = 0,
	POWERUP_BIGPADDLE,
	POWERUP_MULTIBALL
} powerupType_t;

class BOEntity {
public:
	bool					visible;

	idStr					materialName;
	const idMaterial *		material;
	float					width, height;
	idVec4					color;
	idVec2					position;
	idVec2					velocity;

	powerupType_t			powerup;

	bool					removed;
	bool					fadeOut;

	idGameBustOutWindow *	game;
	
public:
							BOEntity(idGameBustOutWindow* _game);
	virtual					~BOEntity();

	virtual void			WriteToSaveGame( idFile *savefile );
	virtual void			ReadFromSaveGame( idFile *savefile, idGameBustOutWindow* _game );

	void					SetMaterial(const char* name);
	void					SetSize( float _width, float _height );
	void					SetColor( float r, float g, float b, float a );
	void					SetVisible( bool isVisible );

	virtual void			Update( float timeslice, int guiTime );
	virtual void			Draw(idDeviceContext *dc);

private:
};

typedef enum {
	COLLIDE_NONE = 0,
	COLLIDE_DOWN,
	COLLIDE_UP,
	COLLIDE_LEFT,
	COLLIDE_RIGHT
} collideDir_t;

class BOBrick {
public:
	float			x;
	float			y;
	float			width;
	float			height;
	powerupType_t	powerup;

	bool			isBroken;

	BOEntity		*ent;

public:
					BOBrick();
					BOBrick( BOEntity *_ent, float _x, float _y, float _width, float _height );
					~BOBrick();

	virtual void	WriteToSaveGame( idFile *savefile );
	virtual void	ReadFromSaveGame( idFile *savefile, idGameBustOutWindow *game );

	void			SetColor( idVec4 bcolor );
	collideDir_t	checkCollision( idVec2 pos, idVec2 vel );

private:
};

#define BOARD_ROWS 12

class idGameBustOutWindow : public idWindow {
public:
	idGameBustOutWindow(idUserInterfaceLocal *gui);
	idGameBustOutWindow(idDeviceContext *d, idUserInterfaceLocal *gui);
	~idGameBustOutWindow();

	virtual void		WriteToSaveGame( idFile *savefile );
	virtual void		ReadFromSaveGame( idFile *savefile );

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw(int time, float x, float y);
	virtual const char*	Activate(bool activate);
	virtual idWinVar *	GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);

	idList<BOEntity*>	entities;

private:
	void				CommonInit();
	void				ResetGameState();

	void				ClearBoard();
	void				ClearPowerups();
	void				ClearBalls();

	void				LoadBoardFiles();
	void				SetCurrentBoard();
	void				UpdateGame();
	void				UpdatePowerups();
	void				UpdatePaddle();
	void				UpdateBall();
	void				UpdateScore();

	BOEntity *			CreateNewBall();
	BOEntity *			CreatePowerup( BOBrick *brick );

	virtual bool		ParseInternalVar(const char *name, idParser *src);

private:

	idWinBool			gamerunning;
	idWinBool			onFire;
	idWinBool			onContinue;
	idWinBool			onNewGame;
	idWinBool			onNewLevel;

	float				timeSlice;
	bool				gameOver;

	int					numLevels;
	byte *				levelBoardData;
	bool				boardDataLoaded;

	int					numBricks;
	int					currentLevel;

	bool				updateScore;
	int					gameScore;
	int					nextBallScore;

	int					bigPaddleTime;
	float				paddleVelocity;

	float				ballSpeed;
	int					ballsRemaining;
	int					ballsInPlay;
	bool				ballHitCeiling;

	idList<BOEntity*>	balls;
	idList<BOEntity*>	powerUps;

	BOBrick				*paddle;
	idList<BOBrick*>	board[BOARD_ROWS];
};

#endif //__GAME_BUSTOUT_WINDOW_H__
