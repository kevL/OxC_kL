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
#include "ExplosionBState.h"			// kL, for terrain explosions
#include "TileEngine.h"					// kL, for terrain explosions
#include "Position.h"					// kL, for terrain explosions

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
//#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
//#include "../Interface/TurnCounter.h"	// kL, extern 'kL_TurnCount'
#include "../Interface/Window.h"

//#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"			// kL, for terrain explosions


namespace OpenXcom
{

/**
 * Initializes all the elements in the Next Turn screen.
 * @param battleSave		- pointer to the SavedBattleGame
 * @param state				- pointer to the BattlescapeState
 * @param aliensPacified	- true if all remaining aliens are mind-controlled (default false)
 */
NextTurnState::NextTurnState(
		SavedBattleGame* battleSave,
		BattlescapeState* state,
		bool aliensPacified)
	:
		_battleSave(battleSave),
		_state(state),
		_aliensPacified(aliensPacified)
//		_timer(NULL)
//		_turnCounter(NULL)
{
	_window = new Window(this, 320, 200);

	if (aliensPacified == false)
	{
		_txtTitle	= new Text(320, 17, 0,  68);
		_txtTurn	= new Text(320, 17, 0,  93);
		_txtSide	= new Text(320, 17, 0, 109);
		_txtMessage	= new Text(320, 17, 0, 149);
	}
	else
		_txtMessage	= new Text(320, 200);

//	_bg = new Surface(_game->getScreen()->getWidth(), _game->getScreen()->getHeight());

	setPalette("PAL_BATTLESCAPE");

	add(_window);
	add(_txtMessage,	"messageWindows", "battlescape");
	if (aliensPacified == false)
	{
		add(_txtTitle,	"messageWindows", "battlescape");
		add(_txtTurn,	"messageWindows", "battlescape");
		add(_txtSide,	"messageWindows", "battlescape");
	}
//	add(_bg);

	centerAllSurfaces();


/*	_bg->setX(0);
	_bg->setY(0);
	SDL_Rect rect;
	rect.w = _bg->getWidth();
	rect.h = _bg->getHeight();
	rect.x = rect.y = 0;
	_bg->drawRect(&rect, Palette::blockOffset(0)+15); */
/*	// line this screen up with the hidden movement screen
	const int msg_y = state->getMap()->getMessageY();
	_window->setY(msg_y);
	if (aliensPacified == false)
	{
		_txtTitle->setY(msg_y + 68);
		_txtTurn->setY(msg_y + 93);
		_txtSide->setY(msg_y + 109);
		_txtMessage->setY(msg_y + 149);
	}
	else _txtMessage->setY(msg_y); */

	_window->setBackground(_game->getResourcePack()->getSurface("TAC00.SCR"));
	_window->setHighContrast();

	if (aliensPacified == false)
	{
		_txtTitle->setBig();
		_txtTitle->setAlign(ALIGN_CENTER);
		_txtTitle->setHighContrast();
		_txtTitle->setText(tr("STR_OPENXCOM"));

		_txtTurn->setBig();
		_txtTurn->setAlign(ALIGN_CENTER);
		_txtTurn->setHighContrast();
		_txtTurn->setText(tr("STR_TURN").arg(_battleSave->getTurn()));

		_txtSide->setBig();
		_txtSide->setAlign(ALIGN_CENTER);
		_txtSide->setHighContrast();
		_txtSide->setText(tr("STR_SIDE")
							.arg(tr(_battleSave->getSide() == FACTION_PLAYER ? "STR_XCOM" : "STR_ALIENS")));
	}

	if (aliensPacified == false)
		_txtMessage->setText(tr("STR_PRESS_BUTTON_TO_CONTINUE"));
	else
	{
		_txtMessage->setText(tr("STR_ALIENS_PACIFIED"));
		_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	}
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setHighContrast();
	_txtMessage->setBig();

	_state->clearMouseScrollingState();

/*	if (Options::skipNextTurnScreen == true)
	{
		_timer = new Timer(NEXT_TURN_DELAY);
		_timer->onTimer((StateHandler)& NextTurnState::nextTurn);
		_timer->start();
	} */
}

/**
 * dTor.
 */
NextTurnState::~NextTurnState()
{
//	if (Options::skipNextTurnScreen == true)
//		delete _timer;
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
//		kL_TurnCount = _battleSave->getTurn();
//		_turnCounter = _state->getTurnCounter();
//		_turnCounter->update();

		_state->updateTurn();
		nextTurn();
	}
}

/*
 * Keeps the timer running.
 *
void NextTurnState::think()
{
	if (_timer != NULL) _timer->think(this, NULL);
} */

/**
 * Closes the window.
 */
void NextTurnState::nextTurn()
{
	static bool switchMusic = true;

	// Done here and in DebriefingState, but removed from ~BattlescapeGame (see)
	_battleSave->getBattleGame()->cleanupDeleted();
	_game->popState();

	int
		liveAliens,
		liveSoldiers;
	_state->getBattleGame()->tallyUnits(liveAliens, liveSoldiers);
	if ((liveAliens == 0 || liveSoldiers == 0)
		&& _battleSave->getObjectiveType() != MUST_DESTROY) // not the final mission and all xCom/aLiens dead.
	{
		switchMusic = true;
		_state->finishBattle(false, liveSoldiers);
	}
	else
	{
		_state->btnCenterClick(NULL);

		if (_battleSave->getSide() == FACTION_PLAYER)
		{
			_state->getBattleGame()->setupCursor();
			_game->getCursor()->setVisible();

			const int turn = _battleSave->getTurn();
/*			if (turn == 1 || (turn % Options::autosaveFrequency) == 0)
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
			} */
			if (turn != 1)
				_battleSave->getBattleGame()->setPlayerPanic();

			if (turn != 1
				&& switchMusic == true)
			{
				_game->getResourcePack()->fadeMusic(_game, 473);

				std::string
					music,
					terrain;
				_battleSave->calibrateMusic(
										music,
										terrain);
				_game->getResourcePack()->playMusic(
												music,
												terrain);
			}
			else
				switchMusic = true;


			Tile* const tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
			if (tile != NULL)
			{
				const Position pos = Position(
										tile->getPosition().x * 16 + 8,
										tile->getPosition().y * 16 + 8,
										tile->getPosition().z * 24 + 10);
				_battleSave->getBattleGame()->statePushBack(new ExplosionBState(
																			_battleSave->getBattleGame(),
																			pos,
																			NULL,NULL,
																			tile,
																			false,false,true));
			}
		}
		else // start non-Player turn
		{
			if (_aliensPacified == false)
			{
				_game->getResourcePack()->fadeMusic(_game, 473);
				_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_TAC_BATTLE_ALIENTURN);
			}
			else
				switchMusic = false;

			if (_battleSave->getDebugMode() == false)
				_game->getCursor()->setVisible(false);
		}
	}
}

/*
 *
 *
void NextTurnState::resize(int& dX, int& dY)
{
	State::resize(dX, dY);
//	_bg->setX(0);
//	_bg->setY(0);
} */

}
