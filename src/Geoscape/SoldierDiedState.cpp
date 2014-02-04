/**
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SoldierDiedState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Died window.
 * @param game, Pointer to the core game.
 * @param name, Name of the Soldier.
 */
SoldierDiedState::SoldierDiedState(
		Game* game,
		std::wstring name)
	:
		State(game),
		_name(name)
{
	Log(LOG_INFO) << "create SoldierDiedState";

	_screen = false;

	_window		= new Window(this, 192, 104, 32, 48, POPUP_BOTH);
	_txtTitle	= new Text(160, 30, 48, 72);
	_btnOk		= new TextButton(80, 14, 88, 114);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierDiedState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiedState::btnOkClick,
					(SDLKey)Options::getInt("keyOk"));
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiedState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	std::wstring msg = _name;
	msg += L'\n';
	msg += tr("STR_SOLDIER_DIED");
	_txtTitle->setText(msg);

	//Log(LOG_INFO) << "create SoldierDiedState EXIT";
}

/**
 *
 */
SoldierDiedState::~SoldierDiedState()
{
}

/**
 * Resets the palette.
 */
void SoldierDiedState::init()
{
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
				Palette::backPos,
				16);
}

/**
 * Returns to the previous screen.
 * @param action, Pointer to an action.
 */
void SoldierDiedState::btnOkClick(Action*)
{
	_game->popState();
}

}

