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
#include "GeoscapeState.h"
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
 * @param craft	- pointer to Craft
 * @param globe	- pointer to Globe
 * @param geo	- pointer to GeoscapeState
 */
CraftPatrolState::CraftPatrolState(
		Craft* craft,
		Globe* globe,
		GeoscapeState* geo)
	:
		_craft(craft),
		_globe(globe),
		_geo(geo)
{
	_screen = false;

	_window			= new Window(this, 224, 168, 16, 16, POPUP_BOTH);

	_txtDestination	= new Text(224, 78, 16, 32);
	_txtPatrolling	= new Text(224, 17, 16, 100);

	_btnOk			= new TextButton(192, 16, 32, 121);
	_btnInfo		= new TextButton(192, 16, 32, 140);

	_btnBase		= new TextButton(62, 16, 32, 159);
	_btnCenter		= new TextButton(62, 16, 97, 159);
	_btnRedirect	= new TextButton(62, 16, 162, 159);

	setPalette("PAL_GEOSCAPE", 4);

	add(_window);
	add(_txtDestination);
	add(_txtPatrolling);
	add(_btnOk);
	add(_btnInfo);
	add(_btnBase);
	add(_btnCenter);
	add(_btnRedirect);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_txtDestination->setColor(Palette::blockOffset(15)-1);
	_txtDestination->setBig();
	_txtDestination->setAlign(ALIGN_CENTER);
	_txtDestination->setWordWrap();
	_txtDestination->setText(tr("STR_CRAFT_HAS_REACHED_DESTINATION")
								 .arg(_craft->getName(_game->getLanguage()))
								 .arg(_craft->getDestination()->getName(_game->getLanguage())));

	_txtPatrolling->setColor(Palette::blockOffset(15)+11);
	_txtPatrolling->setBig();
	_txtPatrolling->setAlign(ALIGN_CENTER);
	_txtPatrolling->setText(tr("STR_NOW_PATROLLING"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftPatrolState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftPatrolState::btnOkClick,
					Options::keyCancel);

	_btnInfo->setColor(Palette::blockOffset(8)+5);
	_btnInfo->setText(tr("STR_EQUIP_CRAFT"));
	_btnInfo->onMouseClick((ActionHandler)& CraftPatrolState::btnInfoClick);

	_btnBase->setColor(Palette::blockOffset(8)+5);
	_btnBase->setText(tr("STR_RETURN_TO_BASE"));
	_btnBase->onMouseClick((ActionHandler)& CraftPatrolState::btnBaseClick);

	_btnCenter->setColor(Palette::blockOffset(8)+5);
	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick((ActionHandler)& CraftPatrolState::btnCenterClick);

	_btnRedirect->setColor(Palette::blockOffset(8)+5);
	_btnRedirect->setText(tr("STR_REDIRECT_CRAFT"));
	_btnRedirect->onMouseClick((ActionHandler)& CraftPatrolState::btnRedirectClick);
	_btnRedirect->onKeyboardPress(
					(ActionHandler)& CraftPatrolState::btnRedirectClick,
					Options::keyOk);
}

/**
 * dTor.
 */
CraftPatrolState::~CraftPatrolState()
{
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the craft info window.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnInfoClick(Action*)
{
	_game->popState();

	_game->pushState(new GeoscapeCraftState(
										_craft,
										_globe,
										NULL,
										_geo,
										true));
}

/**
 * Returns the craft back to its base.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnBaseClick(Action*)
{
	_game->popState();

	_craft->returnToBase();
}

/**
 * Centers the craft on the globe.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnCenterClick(Action*)
{
	_game->popState();

	_globe->center(
				_craft->getLongitude(),
				_craft->getLatitude());
}

/**
 * Opens the SelectDestination window.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnRedirectClick(Action*)
{
	_game->popState();

	_game->pushState(new SelectDestinationState(
											_craft,
											_globe,
											_geo));
}

}
