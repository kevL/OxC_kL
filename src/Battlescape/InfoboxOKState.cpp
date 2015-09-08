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

#include "InfoboxOKState.h"

#include "Map.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

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
	:
		_cursorVisible(_game->getCursor()->getVisible()),
		_cursorHidden(_game->getCursor()->getHidden())
{
	_screen = false;

	_frame		= new Frame(260, 90, 30, 27);
	_txtTitle	= new Text(250, 58, 35, 34);
	_btnOk		= new TextButton(120, 16, 100, 94);

//	_game->getSavedGame()->getBattleSave()->setPaletteByDepth(this);
	setPalette("PAL_BATTLESCAPE");

	add(_frame,		"infoBoxOK",		"battlescape");
	add(_txtTitle,	"infoBoxOK",		"battlescape");
	add(_btnOk,		"infoBoxOKButton",	"battlescape");

	centerAllSurfaces();


	_frame->setHighContrast();

	_txtTitle->setText(msg);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setHighContrast();
	_txtTitle->setWordWrap();
	_txtTitle->setBig();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& InfoboxOKState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InfoboxOKState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InfoboxOKState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InfoboxOKState::btnOkClick,
					Options::keyCancel);
	_btnOk->setHighContrast();

	_game->getCursor()->setVisible();
	_game->getCursor()->setHidden(false);
}

/**
 * dTor.
 */
InfoboxOKState::~InfoboxOKState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void InfoboxOKState::btnOkClick(Action*)
{
/*	if (_game->getSavedGame()->getBattleSave()->getSide() != FACTION_PLAYER
		&& _game->getSavedGame()->getBattleSave()->getDebugMode() == false)
	{
		_game->getCursor()->setVisible(false);
	} */
	_game->getCursor()->setVisible(_cursorVisible);
	_game->getCursor()->setHidden(_cursorHidden);

	_game->popState();
}

}
