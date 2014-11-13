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

#include "ScannerState.h"

#include <iostream>
#include <sstream>

#include "ScannerView.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes the Scanner State.
 * @param action - pointer to an Action
 */
ScannerState::ScannerState(BattleAction* action)
	:
		_action(action)
{
/*kL
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */

	_bg				= new InteractiveSurface(320, 200);
	_scan			= new InteractiveSurface(320, 200);
	_scannerView	= new ScannerView(
									152,
									152,
									56,
									24,
									_game,
									_action->actor);

	if (_game->getScreen()->getDY() > 50)
		_screen = false;

	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_scan);
	add(_scannerView);
	add(_bg);

	centerAllSurfaces();

	_game->getResourcePack()->getSurface("DETBORD.PCK")->blit(_bg);
	_game->getResourcePack()->getSurface("DETBORD2.PCK")->blit(_scan);

	_bg->onMouseClick(
					(ActionHandler)& ScannerState::exitClick,
					SDL_BUTTON_RIGHT);
	_bg->onKeyboardPress(
					(ActionHandler)& ScannerState::exitClick,
					Options::keyCancel);

	_timerAnimate = new Timer(125);
	_timerAnimate->onTimer((StateHandler)& ScannerState::animate);
	_timerAnimate->start();

	update();
}

/**
 * dTor.
 */
ScannerState::~ScannerState()
{
	delete _timerAnimate;
}

/**
 * Closes the window on right-click.
 * @param action - pointer to an Action
 */
void ScannerState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		exitClick(action);
	}
}

/**
 * Updates scanner state.
 */
void ScannerState::update()
{
//	_scannerView->draw();
}

/**
 * Animation handler. Updates the minimap view animation.
*/
void ScannerState::animate()
{
	_scannerView->animate();
}

/**
 * Handles timers.
*/
void ScannerState::think()
{
	State::think();
	_timerAnimate->think(this, NULL);
}

/**
 * Exits the screen.
 * @param action - pointer to an Action
 */
void ScannerState::exitClick(Action*)
{
/*kL
	if (Options::maximizeInfoScreens)
	{
		Screen::updateScale(
						Options::battlescapeScale,
						Options::battlescapeScale,
						Options::baseXBattlescape,
						Options::baseYBattlescape,
						true);
		_game->getScreen()->resetDisplay(false);
	} */

	_game->popState();
}

}
