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

#include "PlaceLiftState.h"

#include <sstream>

#include "BasescapeState.h"
#include "BaseView.h"
#include "SelectStartFacilityState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"

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
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param globe Pointer to the Geoscape globe.
 * @param first Is this a custom starting base?
 */
PlaceLiftState::PlaceLiftState(
		Base* base,
		Globe* globe,
		bool first)
	:
		_base(base),
		_globe(globe),
		_first(first)
{
	_view		= new BaseView(192, 192, 0, 8);
	_txtTitle	= new Text(320, 9, 0, 0);

	setPalette("PAL_BASESCAPE");

	add(_view);
	add(_txtTitle);

	centerAllSurfaces();

	_view->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_view->setBase(_base);
	_view->setSelectable(_game->getRuleset()->getBaseFacility("STR_ACCESS_LIFT")->getSize());
	_view->onMouseClick((ActionHandler)& PlaceLiftState::viewClick);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setText(tr("STR_SELECT_POSITION_FOR_ACCESS_LIFT"));
}

/**
 *
 */
PlaceLiftState::~PlaceLiftState()
{
}

/**
 * Processes clicking on facilities.
 * @param action Pointer to an action.
 */
void PlaceLiftState::viewClick(Action*)
{
	BaseFacility* fac = new BaseFacility(
									_game->getRuleset()->getBaseFacility("STR_ACCESS_LIFT"),
									_base);
	fac->setX(_view->getGridX());
	fac->setY(_view->getGridY());

	_base->getFacilities()->push_back(fac);

	_game->popState();

	BasescapeState* bState = new BasescapeState(
											_base,
											_globe);
//kL	_game->getSavedGame()->setSelectedBase(_game->getSavedGame()->getBases()->size() - 1);

	_game->pushState(bState);

	if (_first)
	{
		_game->pushState(new SelectStartFacilityState(
													_base,
													bState,
													_globe));
	}
}

}
