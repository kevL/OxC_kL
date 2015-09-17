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

#include "NewManufactureListState.h"

#include "ManufactureCostsState.h"
#include "ManufactureStartState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the productions list screen.
 * @param base - pointer to the Base to get info from
 */
NewManufactureListState::NewManufactureListState(
		Base* const base)
	:
		_base(base),
		_scroll(0)
{
	_screen = false;

	_window			= new Window(this, 320, 156, 0, 22, POPUP_BOTH);

	_txtTitle		= new Text(320, 16, 16, 30);

	_txtItem		= new Text(80, 9,  16, 46);
	_txtCategory	= new Text(80, 9, 156, 46);

	_cbxCategory	= new ComboBox(this, 134, 16, 179, 30);

	_lstManufacture	= new TextList(285, 97, 16, 55);

	_btnCostTable	= new TextButton(142, 16,  16, 154);
	_btnCancel		= new TextButton(142, 16, 162, 154);

	setInterface("selectNewManufacture");

	add(_window,			"window",	"selectNewManufacture");
	add(_txtTitle,			"text",		"selectNewManufacture");
	add(_txtItem,			"text",		"selectNewManufacture");
	add(_txtCategory,		"text",		"selectNewManufacture");
	add(_lstManufacture,	"list",		"selectNewManufacture");
	add(_cbxCategory,		"catBox",	"selectNewManufacture");
	add(_btnCostTable,		"button",	"selectNewManufacture");
	add(_btnCancel,			"button",	"selectNewManufacture");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr("STR_PRODUCTION_ITEMS"));
	_txtTitle->setBig();

	_txtItem->setText(tr("STR_ITEM"));

	_txtCategory->setText(tr("STR_CATEGORY"));

	_lstManufacture->setColumns(2, 132, 145);
	_lstManufacture->setBackground(_window);
	_lstManufacture->setSelectable();
	_lstManufacture->setMargin(8);
	_lstManufacture->onMouseClick((ActionHandler)& NewManufactureListState::lstProdClick);

	_btnCostTable->setText(tr("STR_PRODUCTION_COSTS"));
	_btnCostTable->onMouseClick((ActionHandler)& NewManufactureListState::btnCostsClick);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& NewManufactureListState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewManufactureListState::btnCancelClick,
					Options::keyCancel);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewManufactureListState::btnCancelClick,
					Options::keyOk);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewManufactureListState::btnCancelClick,
					Options::keyOkKeypad);


	_game->getSavedGame()->getAvailableProductions(
											_possibleProductions,
											_game->getRuleset(),
											_base);
	_catStrings.push_back("STR_ALL_ITEMS");

	std::string cat;
	for (std::vector<RuleManufacture*>::iterator
			i = _possibleProductions.begin();
			i != _possibleProductions.end();
			++i)
	{
		cat = (*i)->getCategory().c_str();

		if (std::find(
				_catStrings.begin(),
				_catStrings.end(),
				cat) == _catStrings.end())
		{
			_catStrings.push_back(cat);
		}
	}

	_cbxCategory->setBackgroundFill(57); // green <- TODO: put this in Interfaces.rul
	_cbxCategory->setOptions(_catStrings);
	_cbxCategory->onChange((ActionHandler)& NewManufactureListState::cbxCategoryChange);
}

/**
 * Initializes state - fills list with possible productions.
 */
void NewManufactureListState::init()
{
	State::init();

	fillProductionList();
	_lstManufacture->scrollTo(_scroll);
}

/**
 * Go to the Costs table.
 * @param action - pointer to an Action
 */
void NewManufactureListState::btnCostsClick(Action*)
{
	_game->pushState(new ManufactureCostsState());
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void NewManufactureListState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Opens the Production settings screen.
 * @param action - pointer to an Action
*/
void NewManufactureListState::lstProdClick(Action*)
{
	_scroll = _lstManufacture->getScroll();

	const RuleManufacture* manufRule = NULL;
	for (std::vector<RuleManufacture*>::iterator
			i = _possibleProductions.begin();
			i != _possibleProductions.end();
			++i)
	{
		if ((*i)->getName().c_str() == _displayedStrings[_lstManufacture->getSelectedRow()])
		{
			manufRule = *i;
			break;
		}
	}

	if (manufRule->getCategory() == "STR_CRAFT"
		&& _base->getAvailableHangars() - _base->getUsedHangars() < 1)
	{
		_game->pushState(new ErrorMessageState(
										tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"),
										_palette,
										_game->getRuleset()->getInterface("basescape")->getElement("errorMessage")->color,
										"BACK17.SCR",
										_game->getRuleset()->getInterface("basescape")->getElement("errorPalette")->color));
	}
	else if (manufRule->getRequiredSpace() > _base->getFreeWorkshops())
		_game->pushState(new ErrorMessageState(
										tr("STR_NOT_ENOUGH_WORK_SPACE"),
										_palette,
										_game->getRuleset()->getInterface("basescape")->getElement("errorMessage")->color,
										"BACK17.SCR",
										_game->getRuleset()->getInterface("basescape")->getElement("errorPalette")->color));
	else
		_game->pushState(new ManufactureStartState(_base, manufRule));
}

/**
 * Updates the production list to match the category filter.
 */
void NewManufactureListState::cbxCategoryChange(Action*)
{
	fillProductionList();
}

/**
 * Fills the list of possible productions.
 */
void NewManufactureListState::fillProductionList()
{
	_lstManufacture->clearList();
	_possibleProductions.clear();
	_displayedStrings.clear();

	_game->getSavedGame()->getAvailableProductions(
											_possibleProductions,
											_game->getRuleset(),
											_base);

	for (std::vector<RuleManufacture*>::const_iterator
			i = _possibleProductions.begin();
			i != _possibleProductions.end();
			++i)
	{
		if (_catStrings[_cbxCategory->getSelected()] == "STR_ALL_ITEMS"
			|| (*i)->getCategory().c_str() == _catStrings[_cbxCategory->getSelected()])
		{
			_lstManufacture->addRow(
								2,
								tr((*i)->getName()).c_str(),
								tr((*i)->getCategory ()).c_str());
			_displayedStrings.push_back((*i)->getName().c_str());
		}
	}
}

}
