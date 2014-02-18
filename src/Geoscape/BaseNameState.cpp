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

#include "BaseNameState.h"

#include "../Basescape/PlaceLiftState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in a Base Name window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to name.
 * @param globe Pointer to the Geoscape globe.
 * @param first Is this the first base in the game?
 */
BaseNameState::BaseNameState(
		Game* game,
		Base* base,
		Globe* globe,
		bool first)
	:
		State(game),
		_base(base),
		_globe(globe),
		_first(first)
{
	_globe->onMouseOver(0);

	_screen = false;

	_window		= new Window(this, 192, 88, 32, 60, POPUP_BOTH);
	_txtTitle	= new Text(182, 17, 37, 70);
	_edtName	= new TextEdit(127, 16, 59, 94);
	_btnOk		= new TextButton(162, 16, 47, 118);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_edtName);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BaseNameState::btnOkClick);
//	_btnOk->onKeyboardPress((ActionHandler)& BaseNameState::btnOkClick, (SDLKey)Options::getInt("keyOk"));
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseNameState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_btnOk->setVisible(false); // something must be in the name before it is acceptable

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_BASE_NAME"));

	_edtName->setColor(Palette::blockOffset(8)+5);
	_edtName->setBig();
	_edtName->focus();
	_edtName->onKeyboardPress((ActionHandler)& BaseNameState::edtNameKeyPress);
}

/**
 *
 */
BaseNameState::~BaseNameState()
{
}

/**
 *
 */
void BaseNameState::nameBase()
{
	_base->setName(_edtName->getText());

	_game->popState();
	_game->popState();


	if (!_first)
	{
		_game->popState();
	}

	if (!_first
		|| Options::getBool("customInitialBase"))
	{
		_game->pushState(new PlaceLiftState(
										_game,
										_base,
										_globe,
										_first));
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void BaseNameState::edtNameKeyPress(Action* action)
{
	if (action->getDetails()->key.keysym.sym == SDLK_RETURN
		|| action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		if (!_edtName->getText().empty())
		{
			nameBase();
		}
	}
	else
	{
		_btnOk->setVisible(!_edtName->getText().empty());
	}
}

/**
 * Returns to the previous screen
 * @param action Pointer to an action.
 */
void BaseNameState::btnOkClick(Action*)
{
	nameBase();
}

}
