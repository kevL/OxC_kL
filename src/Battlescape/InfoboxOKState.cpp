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

#include "InfoboxOKState.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/TextButton.h"
#include "../Interface/Frame.h"
#include "../Interface/Text.h"
#include "../Interface/Cursor.h"
#include "../Engine/Options.h"


namespace OpenXcom
{

/**
 * Initializes all the elements.
 * @param game Pointer to the core game.
 * @param msg Message string.
 */
InfoboxOKState::InfoboxOKState(Game* game, const std::wstring& msg)
	:
		State(game)
{
	_screen = false;

	_frame		= new Frame(260, 90, 30, 27);
	_txtTitle	= new Text(250, 60, 35, 32);
	_btnOk		= new TextButton(120, 16, 100, 94);

	add(_frame);
	add(_txtTitle);
	add(_btnOk);

	centerAllSurfaces();


	_frame->setColor(Palette::blockOffset(6)+3);
	_frame->setBackground(Palette::blockOffset(6)+12);
	_frame->setThickness(3);
	_frame->setHighContrast(true);

	_txtTitle->setColor(Palette::blockOffset(1)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setHighContrast(true);
//kL	_txtTitle->setWordWrap(true);
	_txtTitle->setText(msg);

	_btnOk->setColor(Palette::blockOffset(1)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& InfoboxOKState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)& InfoboxOKState::btnOkClick, (SDLKey)Options::getInt("keyOk"));
	_btnOk->onKeyboardPress((ActionHandler)& InfoboxOKState::btnOkClick, (SDLKey)Options::getInt("keyCancel"));
	_btnOk->setHighContrast(true);

	_game->getCursor()->setVisible(true);
}

/**
 *
 */
InfoboxOKState::~InfoboxOKState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void InfoboxOKState::btnOkClick(Action*)
{
	_game->popState();
}

}
