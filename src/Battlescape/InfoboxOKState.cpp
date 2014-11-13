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

#include "InfoboxOKState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Cursor.h"
#include "../Interface/Frame.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements.
 * @param msg - reference the message string
 */
InfoboxOKState::InfoboxOKState(const std::wstring& msg)
{
	_screen = false;

	_frame		= new Frame(260, 90, 30, 27);
	_txtTitle	= new Text(250, 58, 35, 34);
	_btnOk		= new TextButton(120, 16, 100, 94);

	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_frame, "infoBoxOK", "battlescape");
	add(_txtTitle, "infoBoxOK", "battlescape");
	add(_btnOk, "infoBoxOK", "battlescape");

	centerAllSurfaces();

//	_frame->setColor(Palette::blockOffset(6)+3);
//	_frame->setBackground(Palette::blockOffset(6)+12);
	_frame->setThickness();
	_frame->setHighContrast();

//	_txtTitle->setColor(Palette::blockOffset(1)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setHighContrast();
//kL	_txtTitle->setWordWrap();
	_txtTitle->setText(msg);

//	_btnOk->setColor(Palette::blockOffset(1)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& InfoboxOKState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InfoboxOKState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InfoboxOKState::btnOkClick,
					Options::keyCancel);
	_btnOk->setHighContrast();

	_game->getCursor()->setVisible();
}

/**
 * dTor.
 */
InfoboxOKState::~InfoboxOKState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void InfoboxOKState::btnOkClick(Action*)
{
	_game->popState();
}

}
