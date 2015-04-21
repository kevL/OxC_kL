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

#include "ConfirmDestinationState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Destination window.
 * @param craft		- pointer to the Craft to retarget
 * @param target	- pointer to the selected Target (NULL if it's just a point on the globe)
 */
ConfirmDestinationState::ConfirmDestinationState(
		Craft* craft,
		Target* target)
	:
		_craft(craft),
		_target(target)
{
	const Waypoint* const wp = dynamic_cast<Waypoint*>(_target);

	_screen = false;

	_window		= new Window(this, 224, 72, 16, 64);
	_txtTarget	= new Text(192, 32, 32, 75);
	_btnCancel	= new TextButton(75, 16, 51, 111);
	_btnOk		= new TextButton(75, 16, 130, 111);

	setInterface(
			"confirmDestination",
			wp != NULL
				&& wp->getId() == 0);

	add(_window,	"window",	"confirmDestination");
	add(_txtTarget,	"text",		"confirmDestination");
	add(_btnCancel,	"button",	"confirmDestination");
	add(_btnOk,		"button",	"confirmDestination");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_txtTarget->setBig();
	_txtTarget->setAlign(ALIGN_CENTER);
	_txtTarget->setVerticalAlign(ALIGN_MIDDLE);
	if (wp != NULL
		&& wp->getId() == 0)
	{
		_txtTarget->setText(tr("STR_TARGET_WAY_POINT"));
	}
	else
		_txtTarget->setText(tr("STR_TARGET").arg(_target->getName(_game->getLanguage())));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ConfirmDestinationState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ConfirmDestinationState::btnOkClick,
					Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& ConfirmDestinationState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& ConfirmDestinationState::btnCancelClick,
					Options::keyCancel);
}

/**
 * dTor.
 */
ConfirmDestinationState::~ConfirmDestinationState()
{}

/**
 * Confirms the selected target for the craft.
 * @param action - pointer to an Action
 */
void ConfirmDestinationState::btnOkClick(Action*)
{
	Waypoint* const wp = dynamic_cast<Waypoint*>(_target);
	if (wp != NULL
		&& wp->getId() == 0)
	{
		wp->setId(_game->getSavedGame()->getId("STR_WAYPOINT"));
		_game->getSavedGame()->getWaypoints()->push_back(wp);
	}

	_craft->setDestination(_target);
	_craft->setStatus("STR_OUT");

/*	if (_craft->getFlightOrder() == 0)
	{
		int
			order = 0,
			testVar;
		for (std::vector<Base*>::const_iterator
				i = _game->getSavedGame()->getBases()->begin();
				i != _game->getSavedGame()->getBases()->end();
				++i)
		{
			for (std::vector<Craft*>::const_iterator
					j = (*i)->getCrafts()->begin();
					j != (*i)->getCrafts()->end();
					++j)
			{
				testVar = (*j)->getFlightOrder();
				if (testVar > order)
					order = testVar;
			}
		}

		_craft->setFlightOrder(++order);
	} */

	_game->popState();
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ConfirmDestinationState::btnCancelClick(Action*)
{
	const Waypoint* const wp = dynamic_cast<Waypoint*>(_target);
	if (wp != NULL
		&& wp->getId() == 0)
	{
		delete wp;
	}

	_game->popState();
}

}
