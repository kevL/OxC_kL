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

#include "ConfirmLoadState.h"

#include "LoadGameState.h"

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
 * Initializes all the elements in the Confirm Load screen.
 * @param origin	- game section that originated this state
 * @param fileName	- reference the name of the save file without extension
 */
ConfirmLoadState::ConfirmLoadState(
		OptionsOrigin origin,
		const std::string& fileName)
	:
		_origin(origin),
		_fileName(fileName)
{
	_screen = false;

	_window		= new Window(this, 216, 100, 52, 50, POPUP_BOTH);
	_txtText	= new Text(180, 60, 70, 60);

	_btnNo		= new TextButton(60, 20, 65, 122);
	_btnYes		= new TextButton(60, 20, 195, 122);

	if (_origin == OPT_BATTLESCAPE)
		setPalette("PAL_BATTLESCAPE");
	else
		setPalette("PAL_GEOSCAPE", 6);

	add(_window);
	add(_btnYes);
	add(_btnNo);
	add(_txtText);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnYes->setColor(Palette::blockOffset(15)-1);
	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)& ConfirmLoadState::btnYesClick);
	_btnYes->onKeyboardPress(
					(ActionHandler)& ConfirmLoadState::btnYesClick,
					Options::keyOk);

	_btnNo->setColor(Palette::blockOffset(15)-1);
	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick((ActionHandler)& ConfirmLoadState::btnNoClick);
	_btnNo->onKeyboardPress(
					(ActionHandler)& ConfirmLoadState::btnNoClick,
					Options::keyCancel);

	_txtText->setColor(Palette::blockOffset(15)-1);
	_txtText->setAlign(ALIGN_CENTER);
	_txtText->setBig();
	_txtText->setWordWrap();
	_txtText->setText(tr("STR_MISSING_CONTENT_PROMPT"));

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeTheme();
}

/**
 * Cleans up the confirmation state.
 */
ConfirmLoadState::~ConfirmLoadState()
{
}

/**
 * Proceed to load the save.
 * @param action - pointer to an action
 */
void ConfirmLoadState::btnYesClick(Action*)
{
	_game->popState();

	_game->pushState(new LoadGameState(
									_origin,
									_fileName,
									_palette));
}

/**
 * Abort loading and return to save list.
 * @param action - pointer to an action
 */
void ConfirmLoadState::btnNoClick(Action*)
{
	_game->popState();
}

}
