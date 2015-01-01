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

#include "AlienTerrorState.h"

//#include <sstream>

#include "GeoscapeState.h"
#include "Globe.h"
#include "InterceptState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedGame.h"
#include "../Savegame/TerrorSite.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Aliens Terrorise window.
 * @param terror	- pointer to the respective Terror Site
 * @param city		- reference the terrorized city's name
 * @param state		- pointer to GeoscapeState
 */
AlienTerrorState::AlienTerrorState(
		TerrorSite* terror,
		const std::string& city,
		GeoscapeState* state)
	:
		_terror(terror),
		_state(state)
{
	_screen = false;

	_window			= new Window(this, 256, 200, 0, 0, POPUP_BOTH);
	_txtTitle		= new Text(246, 32, 5, 48);

	_txtCity		= new Text(246, 17, 5, 80);

	_btnIntercept	= new TextButton(200, 16, 28, 130);
	_btnCentre		= new TextButton(200, 16, 28, 150);
	_btnCancel		= new TextButton(200, 16, 28, 170);

	setPalette("PAL_GEOSCAPE", 3);

	add(_window);
	add(_btnIntercept);
	add(_btnCentre);
	add(_btnCancel);
	add(_txtTitle);
	add(_txtCity);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK03.SCR"));

	_btnIntercept->setColor(Palette::blockOffset(8)+5);
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& AlienTerrorState::btnInterceptClick);

	_btnCentre->setColor(Palette::blockOffset(8)+5);
	_btnCentre->setText(tr("STR_CENTER_ON_SITE_TIME_5_SECONDS"));
	_btnCentre->onMouseClick((ActionHandler)& AlienTerrorState::btnCentreClick);

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& AlienTerrorState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& AlienTerrorState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setWordWrap();
	_txtTitle->setText(tr("STR_ALIENS_TERRORISE"));

	_txtCity->setColor(Palette::blockOffset(8)+5);
	_txtCity->setBig();
	_txtCity->setAlign(ALIGN_CENTER);
	_txtCity->setText(tr(city));
}

/**
 * dTor.
 */
AlienTerrorState::~AlienTerrorState()
{
}

/**
 * Picks a craft to intercept the UFO.
 * @param action - pointer to an Action
 */
void AlienTerrorState::btnInterceptClick(Action*)
{
	_state->timerReset();
//	_state->getGlobe()->center(
//							_terror->getLongitude(),
//							_terror->getLatitude());
	_game->popState();

	_game->pushState(new InterceptState(
									_state->getGlobe(),
									NULL,
//									_terror,
									_state)); // kL_add.
}

/**
 * Centers on the UFO and returns to the previous screen.
 * @param action - pointer to an Action
 */
void AlienTerrorState::btnCentreClick(Action*)
{
	_state->timerReset();

	_state->getGlobe()->center(
							_terror->getLongitude(),
							_terror->getLatitude());

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void AlienTerrorState::btnCancelClick(Action*)
{
	_game->popState();
}

}
