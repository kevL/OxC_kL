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

#include "CraftPatrolState.h"

#include <string>

#include "GeoscapeCraftState.h"
#include "Globe.h"
#include "SelectDestinationState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Craft.h"
#include "../Savegame/Target.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Patrol window.
 * @param game Pointer to the core game.
 * @param craft Pointer to the craft to display.
 * @param globe Pointer to the Geoscape globe.
 */
CraftPatrolState::CraftPatrolState(
		Craft* craft,
		Globe* globe)
	:
		_craft(craft),
		_globe(globe)
{
	_screen = false;

	_window			= new Window(this, 224, 168, 16, 16, POPUP_BOTH);

	_txtDestination	= new Text(224, 78, 16, 32);
	_txtPatrolling	= new Text(224, 17, 16, 100);

	_btnOk			= new TextButton(144, 16, 58, 121);
	_btnCenter		= new TextButton(144, 16, 58, 140);
	_btnRedirect	= new TextButton(144, 16, 58, 159);

	setPalette("PAL_GEOSCAPE", 4);

	add(_window);
	add(_txtDestination);
	add(_txtPatrolling);
	add(_btnOk);
	add(_btnCenter);
	add(_btnRedirect);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_txtDestination->setColor(Palette::blockOffset(15)-1);
	_txtDestination->setBig();
	_txtDestination->setAlign(ALIGN_CENTER);
	_txtDestination->setWordWrap(true);
	_txtDestination->setText(tr("STR_CRAFT_HAS_REACHED_DESTINATION")
								 .arg(_craft->getName(_game->getLanguage()))
								 .arg(_craft->getDestination()->getName(_game->getLanguage())));

//kL	_txtPatrolling->setColor(Palette::blockOffset(15)-1);
	_txtPatrolling->setColor(Palette::blockOffset(15)+11); // kL
	_txtPatrolling->setBig();
	_txtPatrolling->setAlign(ALIGN_CENTER);
	_txtPatrolling->setText(tr("STR_NOW_PATROLLING"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftPatrolState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftPatrolState::btnOkClick,
					Options::keyCancel);

	// kL_begin:
	_btnCenter->setColor(Palette::blockOffset(8)+5);
	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick((ActionHandler)& CraftPatrolState::btnCenterClick);
	// kL_end.

	_btnRedirect->setColor(Palette::blockOffset(8)+5);
	_btnRedirect->setText(tr("STR_REDIRECT_CRAFT"));
	_btnRedirect->onMouseClick((ActionHandler)& CraftPatrolState::btnRedirectClick);
	_btnRedirect->onKeyboardPress(
					(ActionHandler)& CraftPatrolState::btnRedirectClick,
					Options::keyOk);
}

/**
 *
 */
CraftPatrolState::~CraftPatrolState()
{
}

/**
 * Closes the window.
 * @param action - pointer to an action
 */
void CraftPatrolState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * kL. Centers the craft on the globe.
 * @param action - pointer to an action
 */
void CraftPatrolState::btnCenterClick(Action*) // kL
{
	_game->popState();

	_globe->center(
				_craft->getLongitude(),
				_craft->getLatitude());
}

/**
 * Opens up the Craft window.
 * @param action - pointer to an action
 */
void CraftPatrolState::btnRedirectClick(Action*)
{
	_game->popState();

	_game->pushState(new SelectDestinationState(
											_craft,
											_globe));
//	_game->pushState(new GeoscapeCraftState(
//										_craft,
//										_globe,
//										NULL));
}

}
