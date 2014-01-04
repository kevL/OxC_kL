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

#include "AbandonGameState.h"

#include <sstream>

#include "MainMenuState.h"
#include "SaveState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Abandon Game screen.
 * @param game Pointer to the core game.
 * @param origin Game section that originated this state.
 */
AbandonGameState::AbandonGameState(
		Game* game,
		OptionsOrigin origin)
	:
		State(game),
		_origin(origin)
{
	_screen = false;

	int x;
	if (_origin == OPT_GEOSCAPE)
	{
		x = 20;
	}
	else
	{
		x = 52;
	}


	_window		= new Window(this, 216, 160, x, 20, POPUP_BOTH);
	_txtTitle	= new Text(206, 33, x + 5, 73);

	_btnNo		= new TextButton(55, 20, x + 30, 132);
	_btnYes		= new TextButton(55, 20, x + 131, 132);

	add(_window);
	add(_txtTitle);
	add(_btnNo);
	add(_btnYes);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnYes->setColor(Palette::blockOffset(15)-1);
	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)& AbandonGameState::btnYesClick);
	_btnYes->onKeyboardPress(
					(ActionHandler)& AbandonGameState::btnYesClick,
					(SDLKey)Options::getInt("keyOk"));

	_btnNo->setColor(Palette::blockOffset(15)-1);
	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick((ActionHandler)& AbandonGameState::btnNoClick);
	_btnNo->onKeyboardPress(
					(ActionHandler)& AbandonGameState::btnNoClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_ABANDON_GAME_QUESTION"));

	if (_origin == OPT_BATTLESCAPE)
	{
		applyBattlescapeTheme();
	}
}

/**
 *
 */
AbandonGameState::~AbandonGameState()
{
}

/**
 * Goes back to the Main Menu.
 * @param action Pointer to an action.
 */
void AbandonGameState::btnYesClick(Action*)
{
	if (Options::getInt("autosave") == 3)
	{
		SaveState* ss = new SaveState(
									_game,
									_origin,
									false);
		delete ss;
	}

	_game->setState(new MainMenuState(_game));
	_game->setSavedGame(0);
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void AbandonGameState::btnNoClick(Action*)
{
	_game->popState();
}

}
