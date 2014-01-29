/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "NextTurnState.h"

#include <sstream>

#include "BattlescapeState.h"
#include "DebriefingState.h"
#include "Map.h" // kL, global 'kL_preReveal'

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TurnCounter.h" // kL, global 'kL_TurnCount'
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Next Turn screen.
 * @param game Pointer to the core game.
 * @param battleGame Pointer to the saved game.
 * @param state Pointer to the Battlescape state.
 */
NextTurnState::NextTurnState(
		Game* game,
		SavedBattleGame* battleGame,
		BattlescapeState* state)
	:
		State(game),
		_battleGame(battleGame),
		_state(state),
		_timer(0)
{
	Log(LOG_INFO) << "\nCreate NextTurnState";

	_window		= new Window(this, 320, 200, 0, 0);
	_txtTitle	= new Text(320, 17, 0, 68);
	_txtTurn	= new Text(320, 17, 0, 93);
	_txtSide	= new Text(320, 17, 0, 109);
	_txtMessage	= new Text(320, 17, 0, 149);

	add(_window);
	add(_txtTitle);
	add(_txtTurn);
	add(_txtSide);
	add(_txtMessage);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(0)-1);
	_window->setHighContrast(true);
	_window->setBackground(_game->getResourcePack()->getSurface("TAC00.SCR"));

	_txtTitle->setColor(Palette::blockOffset(0)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setHighContrast(true);
	_txtTitle->setText(tr("STR_OPENXCOM"));

	_txtTurn->setColor(Palette::blockOffset(0)-1);
	_txtTurn->setBig();
	_txtTurn->setAlign(ALIGN_CENTER);
	_txtTurn->setHighContrast(true);
	_txtTurn->setText(tr("STR_TURN").arg(_battleGame->getTurn()));
	//Log(LOG_INFO) << ". NextTurnState -> SavedBattleGame::getTurn() : " << _battleGame->getTurn();

	_txtSide->setColor(Palette::blockOffset(0)-1);
	_txtSide->setBig();
	_txtSide->setAlign(ALIGN_CENTER);
	_txtSide->setHighContrast(true);
	_txtSide->setText(tr("STR_SIDE")
						.arg(tr(_battleGame->getSide() == FACTION_PLAYER? "STR_XCOM": "STR_ALIENS")));

	_txtMessage->setColor(Palette::blockOffset(0)-1);
	_txtMessage->setBig();
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setHighContrast(true);
	_txtMessage->setText(tr("STR_PRESS_BUTTON_TO_CONTINUE"));

	_state->clearMouseScrollingState();

	kL_preReveal = true;	// kL
	//Log(LOG_INFO) << ". . set kL_preReveal TRUE";

	if (Options::getBool("skipNextTurnScreen"))
	{
		_timer = new Timer(NEXT_TURN_DELAY);
		_timer->onTimer((StateHandler)& NextTurnState::close);
		_timer->start();
	}

	Log(LOG_INFO) << "Create NextTurnState EXIT";
}

/**
 *
 */
NextTurnState::~NextTurnState()
{
	//Log(LOG_INFO) << "Delete NextTurnState";
	delete _timer;
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void NextTurnState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN
		|| action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
//	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) // kL
		// That causes the aLien NextTurn screen to auto-disappear
		// if all aLiens are Mc'd by xCom... bah!!
	{
		Log(LOG_INFO) << "NextTurnState::handle(RMB)";

		kL_TurnCount = _battleGame->getTurn();

		_turnCounter = _state->getTurnCounter();
		_turnCounter->update();

		close();
	}
}

/**
 * Keeps the timer running.
 */
void NextTurnState::think()
{
	if (_timer)
		_timer->think(this, 0);
}

/**
 * Closes the window.
 */
void NextTurnState::close()
{
	Log(LOG_INFO) << "NextTurnState::close()";

	_game->popState();

	int liveAliens = 0;
	int liveSoldiers = 0;

	_state->getBattleGame()->tallyUnits(
									liveAliens,
									liveSoldiers,
									false);

	if (liveAliens == 0
		|| liveSoldiers == 0)
	{
		_state->finishBattle(false, liveSoldiers);
	}
	else
	{
		_state->btnCenterClick(0);
	}
}

}
