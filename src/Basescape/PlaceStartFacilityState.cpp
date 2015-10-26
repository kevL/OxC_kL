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

#include "PlaceStartFacilityState.h"

#include "BaseView.h"
#include "SelectStartFacilityState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"

#include "../Interface/Text.h"

#include "../Menu/ErrorMessageState.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Place Start Facility window.
 * @param base		- pointer to the base to get info from
 * @param select	- pointer to the selection state
 * @param facRule	- pointer to the facility ruleset to build
 */
PlaceStartFacilityState::PlaceStartFacilityState(
		Base* const base,
		SelectStartFacilityState* const select,
		const RuleBaseFacility* const facRule)
	:
		PlaceFacilityState(base, facRule),
		_select(select)
{
	_view->onMouseClick((ActionHandler)& PlaceStartFacilityState::viewClick);

	_numCost->setText(tr("STR_NONE"));
	_numTime->setText(tr("STR_NONE"));
}

/**
 * dTor.
 */
PlaceStartFacilityState::~PlaceStartFacilityState()
{}

/**
 * Processes clicking on facilities.
 * @param action - pointer to an Action
 */
void PlaceStartFacilityState::viewClick(Action*)
{
	if (_view->isPlaceable(_facRule) == true)
	{
		BaseFacility* const fac = new BaseFacility(_facRule, _base);
		fac->setX(_view->getGridX());
		fac->setY(_view->getGridY());

		_base->getFacilities()->push_back(fac);
		_game->popState();
		_select->facilityBuilt();
	}
	else
	{
		_game->popState();
		_game->pushState(new ErrorMessageState(
											tr("STR_CANNOT_BUILD_HERE"),
											_palette,
											_game->getRuleset()->getInterface("basescape")->getElement("errorMessage")->color,
											"BACK01.SCR",
											_game->getRuleset()->getInterface("basescape")->getElement("errorPalette")->color));
	}
}

}
