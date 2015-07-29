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

#include "OptionsConfirmState.h"

//#include <sstream>
//#include <iomanip>

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Display Options screen.
 * @param origin - game section that originated this state.
 */
OptionsConfirmState::OptionsConfirmState(OptionsOrigin origin)
	:
		_origin(origin),
		_countdown(15)
{
	_screen = false;

	_window		= new Window(this, 216, 100, 52, 50, POPUP_BOTH);
	_btnYes		= new TextButton(50, 20, 70, 120);
	_btnNo		= new TextButton(50, 20, 200, 120);
	_txtTitle	= new Text(206, 20, 57, 70);
	_txtTimer	= new Text(206, 20, 57, 100);
	_timer		= new Timer(1000);

	setInterface(
			"mainMenu",
			false,
			_origin == OPT_BATTLESCAPE);

	add(_window,	"confirmVideo", "mainMenu");
	add(_btnYes,	"confirmVideo", "mainMenu");
	add(_btnNo,		"confirmVideo", "mainMenu");
	add(_txtTitle,	"confirmVideo", "mainMenu");
	add(_txtTimer,	"confirmVideo", "mainMenu");

	centerAllSurfaces();

	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)& OptionsConfirmState::btnYesClick);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick((ActionHandler)& OptionsConfirmState::btnNoClick);
	// no keyboard shortcuts to make sure users can see the message

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setWordWrap();
	_txtTitle->setText(tr("STR_DISPLAY_OPTIONS_CONFIRM"));

	_txtTimer->setAlign(ALIGN_CENTER);
	_txtTimer->setWordWrap();
	_txtTimer->setText(tr("STR_DISPLAY_OPTIONS_REVERT").arg(_countdown));

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeTheme();

	_timer->onTimer((StateHandler)& OptionsConfirmState::countdown);
	_timer->start();
}

/**
 * dTor.
 */
OptionsConfirmState::~OptionsConfirmState()
{
	delete _timer;
}

/**
 * Runs the countdown timer.
 */
void OptionsConfirmState::think()
{
	State::think();

	_timer->think(this, NULL);
}

/**
 * Counts down the timer for reverting display options.
 */
void OptionsConfirmState::countdown()
{
	--_countdown;

	std::wostringstream ss;
	ss << std::setfill(L'0') << std::setw(2) << _countdown;
	_txtTimer->setText(tr("STR_DISPLAY_OPTIONS_REVERT").arg(ss.str()));

	if (_countdown == 0)
		btnNoClick(NULL);
}

/**
 * Goes back to the Main Menu.
 * @param action - pointer to an Action
 */
void OptionsConfirmState::btnYesClick(Action*)
{
	_game->popState();
	OptionsBaseState::restart(_origin);
}

/**
 * Restores the original display options.
 * @param action - pointer to an Action
 */
void OptionsConfirmState::btnNoClick(Action*)
{
	Options::switchDisplay();
	Options::save();

	_game->getScreen()->resetDisplay();
	_game->popState();

	OptionsBaseState::restart(_origin);
}

}
