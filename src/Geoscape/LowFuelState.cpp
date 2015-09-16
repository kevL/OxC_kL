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

#include "LowFuelState.h"

//#include <sstream>

#include "GeoscapeState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Craft.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Low Fuel window.
 * @param craft - pointer to the Craft to display
 * @param state - pointer to GeoscapeState
 */
LowFuelState::LowFuelState(
		Craft* craft,
		GeoscapeState* state)
	:
		_craft(craft),
		_state(state)
{
	_screen = false;

	_window		= new Window(this, 224, 120, 16, 40, POPUP_BOTH);

	_txtTitle	= new Text(214, 17, 21, 51);
	_txtMessage	= new Text(214, 50, 21, 68);
	_btnOk5Secs	= new TextButton(90, 18,  30, 134);
	_btnOk		= new TextButton(90, 18, 136, 134);

	_blinkTimer = new Timer(325);
	_blinkTimer->onTimer((StateHandler)& LowFuelState::blink);
	_blinkTimer->start();

	setInterface("lowFuel");

	add(_window,		"window",	"lowFuel");
	add(_txtTitle,		"text",		"lowFuel");
	add(_txtMessage,	"text2",	"lowFuel");
	add(_btnOk5Secs,	"button",	"lowFuel");
	add(_btnOk,			"button",	"lowFuel");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(_craft->getName(_game->getLanguage()));

	_txtMessage->setText(tr("STR_IS_LOW_ON_FUEL_RETURNING_TO_BASE"));
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setVisible(false); // wait for blink.

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& LowFuelState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& LowFuelState::btnOkClick,
					Options::keyCancel);

	if (_state->is5Sec() == false)
	{
		_btnOk5Secs->setText(tr("STR_OK_5_SECONDS"));
		_btnOk5Secs->onMouseClick((ActionHandler)& LowFuelState::btnOk5SecsClick);
		_btnOk5Secs->onKeyboardPress(
						(ActionHandler)& LowFuelState::btnOk5SecsClick,
						Options::keyOk);
		_btnOk5Secs->onKeyboardPress(
						(ActionHandler)& LowFuelState::btnOk5SecsClick,
						Options::keyOkKeypad);
	}
	else
	{
		_btnOk5Secs->setVisible(false);

		_btnOk->onKeyboardPress(
						(ActionHandler)& LowFuelState::btnOkClick,
						Options::keyOk);
		_btnOk->onKeyboardPress(
						(ActionHandler)& LowFuelState::btnOkClick,
						Options::keyOkKeypad);
	}
}

/**
 * dTor.
 */
LowFuelState::~LowFuelState()
{
	delete _blinkTimer;
}

/**
 * Initializes the state.
 */
/* void LowFuelState::init()
{
	State::init();
	_btnOk5Secs->setVisible(_state->is5Sec() == false);
} */

/**
 * Runs the blink timer.
 */
void LowFuelState::think()
{
	if (_window->isPopupDone() == false)
		_window->think();
	else
		_blinkTimer->think(this, NULL);
}

/**
 * Blinks the message text.
 */
void LowFuelState::blink()
{
	_txtMessage->setVisible(!_txtMessage->getVisible());
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void LowFuelState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Closes the window and sets the timer to 5 Secs.
 * @param action - pointer to an Action
 */
void LowFuelState::btnOk5SecsClick(Action*)
{
	_state->resetTimer();
	_game->popState();
}

}
