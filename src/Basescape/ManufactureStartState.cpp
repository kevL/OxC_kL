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

#include "ManufactureStartState.h"

//#include <sstream>

#include "ManufactureInfoState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the productions start screen.
 * @param base - pointer to the Base to get info from
 * @param manufRule - pointer to RuleManufacture to produce
 */
ManufactureStartState::ManufactureStartState(
		Base* base,
		const RuleManufacture* const manufRule)
	:
		_base(base),
		_manufRule(manufRule)
{
	_screen = false;

	_window					= new Window(this, 320, 170, 0, 15);
	_txtTitle				= new Text(300, 16, 10, 26);

	_txtManHour				= new Text(280, 9, 20, 45);
	_txtCost				= new Text(280, 9, 20, 55);
	_txtWorkSpace			= new Text(280, 9, 20, 65);

	_txtRequiredItemsTitle	= new Text(280, 9, 20, 75);

	_txtItemNameColumn		= new Text(60, 9, 40, 85);
	_txtUnitRequiredColumn	= new Text(60, 9, 180, 85);
	_txtUnitAvailableColumn	= new Text(60, 9, 240, 85);

	_lstRequiredItems		= new TextList(240, 57, 40, 100);

	_btnCancel				= new TextButton(130, 16, 20, 160);
	_btnStart				= new TextButton(130, 16, 170, 160);

	setInterface("allocateManufacture");

	add(_window,		"window",	"allocateManufacture");
	add(_txtTitle,		"text",		"allocateManufacture");
	add(_txtManHour,	"text",		"allocateManufacture");
	add(_txtCost,		"text",		"allocateManufacture");
	add(_txtWorkSpace,	"text",		"allocateManufacture");

	add(_txtRequiredItemsTitle,		"text", "allocateManufacture");
	add(_txtItemNameColumn,			"text", "allocateManufacture");
	add(_txtUnitRequiredColumn,		"text", "allocateManufacture");
	add(_txtUnitAvailableColumn,	"text", "allocateManufacture");
	add(_lstRequiredItems,			"list", "allocateManufacture");

	add(_btnCancel,	"button", "allocateManufacture");
	add(_btnStart,	"button", "allocateManufacture");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr(_manufRule->getName()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtManHour->setText(tr("STR_ENGINEER_HOURS_TO_PRODUCE_ONE_UNIT")
							.arg(_manufRule->getManufactureTime()));

	_txtCost->setText(tr("STR_COST_PER_UNIT_")
							.arg(Text::formatFunding(_manufRule->getManufactureCost())));

	_txtWorkSpace->setText(tr("STR_WORK_SPACE_REQUIRED")
							.arg(_manufRule->getRequiredSpace()));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& ManufactureStartState::btnCancelClick);
	_btnCancel->onKeyboardPress(
							(ActionHandler)& ManufactureStartState::btnCancelClick,
							Options::keyCancel);

	const std::map<std::string, int>& requiredItems (_manufRule->getRequiredItems()); // init
	const int availableWorkSpace = _base->getFreeWorkshops();
	bool productionPossible (_game->getSavedGame()->getFunds() > _manufRule->getManufactureCost());	// init
	productionPossible &= (availableWorkSpace > 0);													// nifty.

	_txtRequiredItemsTitle->setText(tr("STR_SPECIAL_MATERIALS_REQUIRED"));

	_txtItemNameColumn->setText(tr("STR_ITEM_REQUIRED"));

	_txtUnitRequiredColumn->setText(tr("STR_UNITS_REQUIRED"));

	_txtUnitAvailableColumn->setText(tr("STR_UNITS_AVAILABLE"));

	_lstRequiredItems->setColumns(3, 140, 60, 40);

	const ItemContainer* const itemContainer (base->getItems()); // init.
//	int row = 0;
	for (std::map<std::string, int>::const_iterator
			i = requiredItems.begin();
			i != requiredItems.end();
			++i)
	{
		std::wostringstream
			woststr1,
			woststr2;
		woststr1 << L'\x01' << i->second;
		woststr2 << L'\x01' << itemContainer->getItem(i->first);
		productionPossible &= (itemContainer->getItem(i->first) >= i->second);
		_lstRequiredItems->addRow(
								3,
								tr(i->first).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());
//		_lstRequiredItems->setCellColor(row++, 0, Palette::blockOffset(13)+10);
	}

	const bool vis = (requiredItems.empty() == false);
	_txtRequiredItemsTitle->setVisible(vis);
	_txtItemNameColumn->setVisible(vis);
	_txtUnitRequiredColumn->setVisible(vis);
	_txtUnitAvailableColumn->setVisible(vis);
	_lstRequiredItems->setVisible(vis);

	_btnStart->setText(tr("STR_START_PRODUCTION"));
	_btnStart->onMouseClick((ActionHandler)& ManufactureStartState::btnStartClick);
	_btnStart->onKeyboardPress(
					(ActionHandler)& ManufactureStartState::btnStartClick,
					Options::keyOk);
	_btnStart->setVisible(productionPossible);
}

/**
 * Returns to previous screen.
 * @param action - pointer to an Action
 */
void ManufactureStartState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Go to the Production settings screen.
 * @param action - pointer to an Action
 */
void ManufactureStartState::btnStartClick(Action*)
{
	_game->pushState(new ManufactureInfoState(
											_base,
											_manufRule));
}

}
