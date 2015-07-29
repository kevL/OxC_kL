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

#include "ErrorMessageState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in an error window.
 * @param id		- reference the language ID for the message to display
 * @param palette	- pointer to the parent state palette
 * @param color		- color of the UI controls
 * @param bg		- reference the background image
 * @param bgColor	- background color (-1 for Battlescape)
 */
ErrorMessageState::ErrorMessageState(
		const std::string& id,
		SDL_Color* palette,
		int color,
		const std::string& bg,
		int bgColor)
{
	create(
		id,
		L"",
		palette,
		static_cast<Uint8>(color),
		bg,
		bgColor);
}

/**
 * Initializes all the elements in an error window.
 * @param msg		- reference the text string for the message to display
 * @param palette	- pointer to the parent state palette
 * @param color		- color of the UI controls
 * @param bg		- reference the background image
 * @param bgColor	- background color (-1 for Battlescape)
 */
ErrorMessageState::ErrorMessageState(
		const std::wstring& msg,
		SDL_Color* palette,
		int color,
		const std::string& bg,
		int bgColor)
{
	create(
		"",
		msg,
		palette,
		static_cast<Uint8>(color),
		bg,
		bgColor);
}

/**
 * dTor.
 */
ErrorMessageState::~ErrorMessageState()
{}

/**
 * Creates the elements in an error window.
 * @param id		- reference the language ID for the message to display
 * @param msg		- reference the text string for the message to display
 * @param palette	- pointer to the parent state palette
 * @param color		- color of the UI controls
 * @param bg		- background image
 * @param bgColor	- background color (-1 for Battlescape)
 */
void ErrorMessageState::create(
		const std::string& id,
		const std::wstring& msg,
		SDL_Color* palette,
		Uint8 color,
		const std::string& bg,
		int bgColor)
{
	_screen = false;

	_window		= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtMessage	= new Text(200, 120, 60, 30);
	_btnOk		= new TextButton(120, 18, 100, 154);

	setPalette(palette);
	if (bgColor != -1)
		setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(
					static_cast<int>(Palette::blockOffset(static_cast<Uint8>(bgColor)))),
				Palette::PAL_bgID,
				16);

	add(_window);
	add(_btnOk);
	add(_txtMessage);

	centerAllSurfaces();


	_window->setColor(color);
	_window->setBackground(_game->getResourcePack()->getSurface(bg));

	_btnOk->setColor(color);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ErrorMessageState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ErrorMessageState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ErrorMessageState::btnOkClick,
					SDLK_KP_ENTER);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ErrorMessageState::btnOkClick,
					Options::keyCancel);

	if (id.empty() == true)
		_txtMessage->setText(msg);
	else
		_txtMessage->setText(tr(id));

	_txtMessage->setColor(color);
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();

	if (bgColor == -1)
	{
		_window->setHighContrast();
		_btnOk->setHighContrast();
		_txtMessage->setHighContrast();
	}
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void ErrorMessageState::btnOkClick(Action*)
{
	_game->popState();
}

}
