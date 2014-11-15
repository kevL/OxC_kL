/*
 * Copyright 2010-2014 OpenXcom Developers.
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

#include <sstream>

#include "BattlescapeState.h"
#include "DebriefingState.h"
#include "Map.h" // kL, extern 'kL_noReveal'

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Menu/SaveGameState.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
//#include "../Interface/TurnCounter.h" // kL, extern 'kL_TurnCount'
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Next Turn screen.
 * @param battleGame	- pointer to the SavedBattleGame
 * @param state			- pointer to the BattlescapeState
 */
NextTurnState::NextTurnState(
		SavedBattleGame* battleGame,
		BattlescapeState* state)
	:
		_battleGame(battleGame),
		_state(state),
		_timer(NULL)
//		_turnCounter(NULL)
{
	//Log(LOG_INFO) << "Create NextTurnState";
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

	battleGame->setPaletteByDepth(this);

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
	int y = state->getMap()->getMessageY();

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
	_txtTurn->setText(tr("STR_TURN").arg(_battleGame->getTurn()));

//	_txtSide->setColor(Palette::blockOffset(0)-1);
	_txtSide->setBig();
	_txtSide->setAlign(ALIGN_CENTER);
	_txtSide->setHighContrast();
	_txtSide->setText(tr("STR_SIDE")
						.arg(tr(_battleGame->getSide() == FACTION_PLAYER? "STR_XCOM": "STR_ALIENS")));

//	_txtMessage->setColor(Palette::blockOffset(0)-1);
	_txtMessage->setBig();
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setHighContrast();
	_txtMessage->setText(tr("STR_PRESS_BUTTON_TO_CONTINUE"));

	_state->clearMouseScrollingState();

	kL_noReveal = true;
	//Log(LOG_INFO) << ". preReveal TRUE";

	if (Options::skipNextTurnScreen)
	{
		_timer = new Timer(NEXT_TURN_DELAY);
		_timer->onTimer((StateHandler)& NextTurnState::close);
		_timer->start();
	}
	//Log(LOG_INFO) << "Create NextTurnState EXIT";
}

/**
 * dTor.
 */
NextTurnState::~NextTurnState()
{
	if (Options::skipNextTurnScreen) // kL
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
//		kL_TurnCount = _battleGame->getTurn();
//		_turnCounter = _state->getTurnCounter();
//		_turnCounter->update();

		_state->updateTurn(); // kL
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
	_battleGame->getBattleGame()->cleanupDeleted();

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
		_state->finishBattle(false, liveSoldiers);
	}
	else
	{
		_state->btnCenterClick(NULL);

		if (_battleGame->getSide() == FACTION_PLAYER)		// kL
		{
			_state->getBattleGame()->setupCursor();			// kL
			_state->getGame()->getCursor()->setVisible();	// kL

			// Autosave every set amount of turns
			if (_battleGame->getTurn() == 1
				|| _battleGame->getTurn() %Options::autosaveFrequency == 0)
			{
				if (_game->getSavedGame()->isIronman() == true)
					_game->pushState(new SaveGameState(
													OPT_BATTLESCAPE,
													SAVE_IRONMAN,
													_palette));
				else if (Options::autosave)
					_game->pushState(new SaveGameState(
													OPT_BATTLESCAPE,
													SAVE_AUTO_BATTLESCAPE,
													_palette));
			}
		}
		else
			_state->getGame()->getCursor()->setVisible(false); // kL
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
