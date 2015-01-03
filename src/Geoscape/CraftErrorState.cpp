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

#include "CraftErrorState.h"

#include "GeoscapeState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in a Cannot Rearm window.
 * @param state	- pointer to the GeoscapeState
 * @param msg	- reference the error message
 */
CraftErrorState::CraftErrorState(
		GeoscapeState* state,
		const std::wstring& msg)
	:
		_state(state)
{
	_screen = false;

	_window		= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtMessage	= new Text(226, 118, 47, 30);
	_btnOk5Secs	= new TextButton(100, 18, 48, 150);
	_btnOk		= new TextButton(100, 18, 172, 150);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("geoCraftScreens")->getElement("palette")->color); //4

	add(_window, "window", "geoCraftScreens");
	add(_txtMessage, "text1", "geoCraftScreens");
	add(_btnOk5Secs, "button", "geoCraftScreens");
	add(_btnOk, "button", "geoCraftScreens");

	centerAllSurfaces();


//	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

//	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftErrorState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftErrorState::btnOkClick,
					Options::keyCancel);

//	_btnOk5Secs->setColor(Palette::blockOffset(8)+5);
	_btnOk5Secs->setText(tr("STR_OK_5_SECONDS"));
	_btnOk5Secs->onMouseClick((ActionHandler)& CraftErrorState::btnOk5SecsClick);
	_btnOk5Secs->onKeyboardPress(
					(ActionHandler)& CraftErrorState::btnOk5SecsClick,
					Options::keyOk);

//	_txtMessage->setColor(Palette::blockOffset(15)-1);
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();
	_txtMessage->setText(msg);
}

/**
 * dTor.
 */
CraftErrorState::~CraftErrorState()
{}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void CraftErrorState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void CraftErrorState::btnOk5SecsClick(Action*)
{
	_state->timerReset();
	_game->popState();
}

}
