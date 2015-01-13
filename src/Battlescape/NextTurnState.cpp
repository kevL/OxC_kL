/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NextTurnState.h"

//#include <sstream>

#include "BattlescapeGame.h"			// kL, for terrain explosions
#include "BattlescapeState.h"
#include "DebriefingState.h"
#include "ExplosionBState.h"			// kL, for terrain explosions
#include "Map.h"						// kL, extern 'kL_noReveal'
#include "TileEngine.h"					// kL, for terrain explosions
#include "Position.h"					// kL, for terrain explosions

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Menu/SaveGameState.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
//#include "../Interface/TurnCounter.h"	// kL, extern 'kL_TurnCount'
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"			// kL, for terrain explosions


namespace OpenXcom
{

/**
 * Initializes all the elements in the Next Turn screen.
 * @param savedBattle	- pointer to the SavedBattleGame
 * @param state			- pointer to the BattlescapeState
 */
NextTurnState::NextTurnState(
		SavedBattleGame* savedBattle,
		BattlescapeState* state)
	:
		_savedBattle(savedBattle),
		_state(state),
		_timer(NULL)
//		_turnCounter(NULL)
{
	_window		= new Window(this, 320, 200, 0, 0);
	_txtTitle	= new Text(320, 17, 0, 68);
	_txtTurn	= new Text(320, 17, 0, 93);
	_txtSide	= new Text(320, 17, 0, 109);
	_txtMessage	= new Text(320, 17, 0, 149);
//	_bg			= new Surface(
//							_game->getScreen()->getWidth(),
//							_game->getScreen()->getHeight(),
//							0,
//							0);

	_savedBattle->setPaletteByDepth(this);

	add(_window);
	add(_txtTitle, "messageWindows", "battlescape");
	add(_txtTurn, "messageWindows", "battlescape");
	add(_txtSide, "messageWindows", "battlescape");
	add(_txtMessage, "messageWindows", "battlescape");
//	add(_bg);

	centerAllSurfaces();


/*	_bg->setX(0);
	_bg->setY(0);
	SDL_Rect rect;
	rect.w = _bg->getWidth();
	rect.h = _bg->getHeight();
	rect.x = rect.y = 0;
	_bg->drawRect(
				&rect,
				Palette::blockOffset(0)+15); */

	// make this screen line up with the hidden movement screen
	const int y = state->getMap()->getMessageY();

	_window->setY(y);
	_txtTitle->setY(y + 68);
	_txtTurn->setY(y + 93);
	_txtSide->setY(y + 109);
	_txtMessage->setY(y + 149);

	_window->setColor(Palette::blockOffset(0)-1);
	_window->setHighContrast();
	_window->setBackground(_game->getResourcePack()->getSurface("TAC00.SCR"));

//	_txtTitle->setColor(Palette::blockOffset(0)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setHighContrast();
	_txtTitle->setText(tr("STR_OPENXCOM"));

//	_txtTurn->setColor(Palette::blockOffset(0)-1);
	_txtTurn->setBig();
	_txtTurn->setAlign(ALIGN_CENTER);
	_txtTurn->setHighContrast();
	_txtTurn->setText(tr("STR_TURN").arg(_savedBattle->getTurn()));

//	_txtSide->setColor(Palette::blockOffset(0)-1);
	_txtSide->setBig();
	_txtSide->setAlign(ALIGN_CENTER);
	_txtSide->setHighContrast();
	_txtSide->setText(tr("STR_SIDE")
						.arg(tr(_savedBattle->getSide() == FACTION_PLAYER? "STR_XCOM": "STR_ALIENS")));

//	_txtMessage->setColor(Palette::blockOffset(0)-1);
	_txtMessage->setBig();
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setHighContrast();
	_txtMessage->setText(tr("STR_PRESS_BUTTON_TO_CONTINUE"));

	_state->clearMouseScrollingState();

	kL_noReveal = true;

	if (Options::skipNextTurnScreen == true)
	{
		_timer = new Timer(NEXT_TURN_DELAY);
		_timer->onTimer((StateHandler)& NextTurnState::close);
		_timer->start();
	}
}

/**
 * dTor.
 */
NextTurnState::~NextTurnState()
{
	if (Options::skipNextTurnScreen == true)
		delete _timer;
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void NextTurnState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN
		|| action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
//		kL_TurnCount = _savedBattle->getTurn();
//		_turnCounter = _state->getTurnCounter();
//		_turnCounter->update();

		_state->updateTurn();
		close();
	}
}

/**
 * Keeps the timer running.
 */
void NextTurnState::think()
{
	if (_timer != NULL)
		_timer->think(this, NULL);
}

/**
 * Closes the window.
 */
void NextTurnState::close()
{
	// Done here and in DebriefingState, but removed from ~BattlescapeGame (see)
	_savedBattle->getBattleGame()->cleanupDeleted();

	_game->popState();

	int
		liveAliens = 0,
		liveSoldiers = 0;
	_state->getBattleGame()->tallyUnits(
									liveAliens,
									liveSoldiers);
	if (liveAliens == 0
		|| liveSoldiers == 0)
	{
		_state->finishBattle(
						false,
						liveSoldiers);
	}
	else
	{
		_state->btnCenterClick(NULL);

		if (_savedBattle->getSide() == FACTION_PLAYER)
		{
//			if (_savedBattle->getDebugMode() == false)
//			{
			_state->getBattleGame()->setupCursor();
			_state->getGame()->getCursor()->setVisible();
//			}

			const int turn = _savedBattle->getTurn();

			if (turn == 1
				|| (turn %Options::autosaveFrequency) == 0)
			{
				if (_game->getSavedGame()->isIronman() == true)
					_game->pushState(new SaveGameState(
													OPT_BATTLESCAPE,
													SAVE_IRONMAN,
													_palette));
				else if (Options::autosave == true)
					_game->pushState(new SaveGameState(
													OPT_BATTLESCAPE,
													SAVE_AUTO_BATTLESCAPE,
													_palette));
			}

			if (turn != 1)
			{
				std::string music = OpenXcom::res_MUSIC_TAC_BATTLE; // default/ safety.
				std::string terrain;

				const std::string mission = _savedBattle->getMissionType();
				if (mission == "STR_UFO_CRASH_RECOVERY")
				{
					music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOCRASHED;
					terrain = _savedBattle->getTerrain();
				}
				else if (mission == "STR_UFO_GROUND_ASSAULT")
				{
					music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOLANDED;
					terrain = _savedBattle->getTerrain();
				}
				else if (mission == "STR_ALIEN_BASE_ASSAULT")
					music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEASSAULT;
				else if (mission == "STR_BASE_DEFENSE")
					music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEDEFENSE;
				else if (mission == "STR_TERROR_MISSION")
					music = OpenXcom::res_MUSIC_TAC_BATTLE_TERRORSITE;
				else if (mission == "STR_MARS_CYDONIA_LANDING")
					music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS1;
				else if (mission == "STR_MARS_THE_FINAL_ASSAULT")
					music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS2;

				_game->getResourcePack()->fadeMusic(_game, 473);
				_game->getResourcePack()->playMusic(
												music,
												terrain);
			}
		}
		else
		{
			_game->getResourcePack()->fadeMusic(_game, 473);
			_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_TAC_BATTLE_ALIENTURN);

			if (_savedBattle->getDebugMode() == false)
				_state->getGame()->getCursor()->setVisible(false);
		}


		Tile* const tile = _savedBattle->getTileEngine()->checkForTerrainExplosions();
		if (tile != NULL)
		{
			const Position pos = Position(
									tile->getPosition().x * 16 + 8,
									tile->getPosition().y * 16 + 8,
									tile->getPosition().z * 24 + 10);
			_savedBattle->getBattleGame()->statePushBack(new ExplosionBState(
																		_savedBattle->getBattleGame(),
																		pos,
																		NULL,
																		NULL,
																		tile,
																		false,
																		false,
																		true));
		}
	}
}

/**
 *
 */
/*void NextTurnState::resize(
		int& dX,
		int& dY)
{
	State::resize(dX, dY);

//	_bg->setX(0);
//	_bg->setY(0);
} */

}
