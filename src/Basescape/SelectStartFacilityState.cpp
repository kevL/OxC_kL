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

#include "SelectStartFacilityState.h"

#include "BasescapeState.h" // resetStoresWarning
#include "PlaceLiftState.h"
#include "PlaceStartFacilityState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Select Start Facility window.
 * @param base	- pointer to the Base to get info from
 * @param state	- pointer to the base State to refresh
 * @param globe	- pointer to the Globe to refresh
 */
SelectStartFacilityState::SelectStartFacilityState(
		Base* const base,
		State* const state,
		Globe* const globe)
	:
		BuildFacilitiesState(base, state),
		_globe(globe)
{
	_facilities = _game->getRuleset()->getCustomBaseFacilities();

	_btnOk->setText(tr("STR_RESET"));
	_btnOk->onMouseClick((ActionHandler)& SelectStartFacilityState::btnOkClick);
	_btnOk->onKeyboardPress(NULL, Options::keyCancel);

	_lstFacilities->onMouseClick((ActionHandler)& SelectStartFacilityState::lstFacilitiesClick);

	populateBuildList();
}

/**
 * dTor.
 */
SelectStartFacilityState::~SelectStartFacilityState()
{}

/**
 * Populates the build list from the currently available facilities.
 */
void SelectStartFacilityState::populateBuildList() // virtual. Cf, BuildFacilitiesState::PopulateBuildList()
{
	_lstFacilities->clearList();
	for (std::vector<RuleBaseFacility*>::iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		_lstFacilities->addRow(1, tr((*i)->getType()).c_str());
	}
}

/**
 * Resets the base-building procedure.
 * @param action - pointer to an Action
 */
void SelectStartFacilityState::btnOkClick(Action*)
{
	for (std::vector<BaseFacility*>::iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		delete *i;
	}

	_base->getFacilities()->clear();

	_game->popState();
	_game->popState();

	_game->pushState(new PlaceLiftState(_base, _globe, true));
}

/**
 * Places the selected facility.
 * @param action - pointer to an Action
 */
void SelectStartFacilityState::lstFacilitiesClick(Action*)
{
	_game->pushState(new PlaceStartFacilityState(
											_base, this,
											_facilities[_lstFacilities->getSelectedRow()]));
}

/**
 * Callback from PlaceStartFacilityState.
 * @note Removes placed facility from the list.
 */
void SelectStartFacilityState::facilityBuilt()
{
	_facilities.erase(_facilities.begin() + _lstFacilities->getSelectedRow());

	if (_facilities.empty() == true) // return to geoscape, force timer to start.
	{
		_game->popState();
		_game->popState();

		BasescapeState* const basescape = dynamic_cast<BasescapeState*>(_state);
		basescape->resetStoresWarning();
	}
	else
		populateBuildList();
}

}
