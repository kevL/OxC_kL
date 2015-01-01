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

#include "InfoboxState.h"

//#include <string>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

#include "../Interface/Frame.h"
#include "../Interface/Text.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements.
 * @param msg - reference a message string
 */
InfoboxState::InfoboxState(const std::wstring& msg)
{
	_screen = false;

	_frame	= new Frame(260, 90, 30, 27);
	_text	= new Text(250, 80, 35, 32);

	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_frame, "infoBox", "battlescape");
	add(_text, "infoBox", "battlescape");

	centerAllSurfaces();


//	_frame->setColor(Palette::blockOffset(0)+7);
//	_frame->setBackground(Palette::blockOffset(0)+14);
	_frame->setHighContrast();
	_frame->setThickness(8);

	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
	_text->setBig();
	_text->setWordWrap();
	_text->setText(msg);
	_text->setHighContrast();
//	_text->setColor(Palette::blockOffset(0)-1);
//	_text->setPalette(_frame->getPalette());

	_timer = new Timer(INFOBOX_DELAY);
	_timer->onTimer((StateHandler)& InfoboxState::close);
	_timer->start();
}

/**
 * dTor.
 */
InfoboxState::~InfoboxState()
{
	delete _timer;
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void InfoboxState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN
		|| action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		close();
	}
}

/**
 * Keeps the animation timers running.
 */
void InfoboxState::think()
{
	_timer->think(this, NULL);
}

/**
 * Closes the window.
 */
void InfoboxState::close()
{
	_game->popState();
}

}
