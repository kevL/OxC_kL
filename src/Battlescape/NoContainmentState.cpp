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

#include "NoContainmentState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the No Containment screen.
 */
NoContainmentState::NoContainmentState()
{
	_window		= new Window(this, 320, 200, 0, 0);
	_txtTitle	= new Text(300, 160, 10, 8);
	_btnOk		= new TextButton(120, 16, 100, 177);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& NoContainmentState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NoContainmentState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NoContainmentState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setText(tr("STR_ALIEN_DIES_NO_ALIEN_CONTAINMENT_FACILITY"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();
}

/**
 * dTor.
 */
NoContainmentState::~NoContainmentState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an action
 */
void NoContainmentState::btnOkClick(Action*)
{
	_game->popState();
}

}
