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

#include "PlaceLiftState.h"

#include "BasescapeState.h"
#include "BaseView.h"
#include "SelectStartFacilityState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
//kL #include "../Savegame/SavedGame.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Place Lift screen.
 * @param base		- pointer to the Base to get info from
 * @param globe		- pointer to the geoscape Globe
 * @param firstBase	- true if this a custom starting base
 */
PlaceLiftState::PlaceLiftState(
		Base* base,
		Globe* globe,
		bool firstBase)
	:
		_base(base),
		_globe(globe),
		_firstBase(firstBase)
{
	_view		= new BaseView(192, 192, 0, 8);
	_txtTitle	= new Text(320, 9);

	setInterface("placeFacility");

	add(_view,		"baseView",	"basescape");
	add(_txtTitle,	"text",		"placeFacility");

	centerAllSurfaces();


	_view->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_view->setBase(_base);
	for (std::vector<std::string>::const_iterator
			i = _game->getRuleset()->getBaseFacilitiesList().begin();
			i != _game->getRuleset()->getBaseFacilitiesList().end();
			++i)
	{
		if (_game->getRuleset()->getBaseFacility(*i)->isLift() == true)
		{
			_lift = _game->getRuleset()->getBaseFacility(*i);
			break;
		}
	}
	_view->setSelectable(_lift->getSize());
	_view->onMouseClick((ActionHandler)& PlaceLiftState::viewClick);

	_txtTitle->setText(tr("STR_SELECT_POSITION_FOR_ACCESS_LIFT"));
}

/**
 * dTor.
 */
PlaceLiftState::~PlaceLiftState()
{}

/**
 * Processes clicking on facilities.
 * @param action - pointer to an Action
 */
void PlaceLiftState::viewClick(Action*)
{
	BaseFacility* const fac = new BaseFacility(
											_lift,
											_base);
	fac->setX(_view->getGridX());
	fac->setY(_view->getGridY());

	_base->getFacilities()->push_back(fac);

	_game->popState();
	BasescapeState* const bState = new BasescapeState(
											_base,
											_globe);
//	_game->getSavedGame()->setSelectedBase(_game->getSavedGame()->getBases()->size() - 1);

	_game->pushState(bState);

	if (_firstBase == true)
		_game->pushState(new SelectStartFacilityState(
													_base,
													bState,
													_globe));
}

}
