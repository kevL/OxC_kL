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

#include "PlaceFacilityState.h"

//#include <limits>

#include "BaseView.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Place Facility window.
 * @param base		- pointer to the base to get info from
 * @param facRule	- pointer to the facility ruleset to build
 */
PlaceFacilityState::PlaceFacilityState(
		Base* const base,
		const RuleBaseFacility* const facRule)
	:
		_base(base),
		_facRule(facRule)
{
	_screen = false;

	_window			= new Window(this, 128, 160, 192, 40);

	_view			= new BaseView(192, 192, 0, 8);

	_txtFacility	= new Text(110,  9, 202,  50);
	_txtCost		= new Text(110,  9, 202,  62);
	_numCost		= new Text(110, 17, 202,  70);
	_txtTime		= new Text(110,  9, 202,  90);
	_numTime		= new Text(110, 17, 202,  98);
	_txtMaintenance	= new Text(110,  9, 202, 118);
	_numMaintenance	= new Text(110, 17, 202, 126);

	_btnCancel		= new TextButton(112, 16, 200, 176);

	setInterface("placeFacility");

	add(_window,			"window",	"placeFacility");
	add(_view,				"baseView",	"basescape");
	add(_txtFacility,		"text",		"placeFacility");
	add(_txtCost,			"text",		"placeFacility");
	add(_numCost,			"numbers",	"placeFacility");
	add(_txtTime,			"text",		"placeFacility");
	add(_numTime,			"numbers",	"placeFacility");
	add(_txtMaintenance,	"text",		"placeFacility");
	add(_numMaintenance,	"numbers",	"placeFacility");
	add(_btnCancel,			"button",	"placeFacility");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_view->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_view->setDog(_game->getResourcePack()->getSurface("BASEDOG"));
	_view->setBase(_base);
	_view->setSelectable(_facRule->getSize());
	_view->onMouseClick((ActionHandler)& PlaceFacilityState::viewClick);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& PlaceFacilityState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& PlaceFacilityState::btnCancelClick,
					Options::keyCancel);

	_txtFacility->setText(tr(_facRule->getType()));

	_txtCost->setText(tr("STR_COST_UC"));

	_numCost->setBig();
	_numCost->setText(Text::formatFunding(_facRule->getBuildCost()));

	_txtTime->setText(tr("STR_CONSTRUCTION_TIME_UC"));

	_numTime->setBig();
	_numTime->setText(tr("STR_DAY", _facRule->getBuildTime()));

	_txtMaintenance->setText(tr("STR_MAINTENANCE_UC"));

	_numMaintenance->setBig();
	_numMaintenance->setText(Text::formatFunding(_facRule->getMonthlyCost()));
}

/**
 * dTor.
 */
PlaceFacilityState::~PlaceFacilityState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void PlaceFacilityState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Processes clicking on facilities.
 * @param action - pointer to an Action
 */
void PlaceFacilityState::viewClick(Action*)
{
	if (_view->isPlaceable(_facRule) == false)
	{
		_game->popState();
		_game->pushState(new ErrorMessageState(
											tr("STR_CANNOT_BUILD_HERE"),
											_palette,
											_game->getRuleset()->getInterface("placeFacility")->getElement("errorMessage")->color,
											"BACK01.SCR",
											_game->getRuleset()->getInterface("placeFacility")->getElement("errorPalette")->color));
	}
	else if (_game->getSavedGame()->getFunds() < _facRule->getBuildCost())
	{
		_game->popState();
		_game->pushState(new ErrorMessageState(
											tr("STR_NOT_ENOUGH_MONEY"),
											_palette,
											_game->getRuleset()->getInterface("placeFacility")->getElement("errorMessage")->color,
											"BACK01.SCR",
											_game->getRuleset()->getInterface("placeFacility")->getElement("errorPalette")->color));
	}
	else
	{
		BaseFacility* const fac = new BaseFacility(
												_facRule,
												_base);

		fac->setX(_view->getGridX());
		fac->setY(_view->getGridY());
		fac->setBuildTime(_facRule->getBuildTime());

		_base->getFacilities()->push_back(fac);

		if (Options::allowBuildingQueue == true)
		{
			if (_view->isQueuedBuilding(_facRule) == true)
				fac->setBuildTime(std::numeric_limits<int>::max());

			_view->reCalcQueuedBuildings();
		}

		const int cost = _facRule->getBuildCost();
		_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - static_cast<int64_t>(cost));
		_base->setCashSpent(cost);

		_game->popState();
	}
}

}
