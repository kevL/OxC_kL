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

#include "NewManufactureListState.h"

#include <algorithm>

#include "ManufactureStartState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Production.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the productions list screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
NewManufactureListState::NewManufactureListState(
		Base* base)
	:
		_base(base)
{
	_screen = false;

	_window			= new Window(this, 320, 156, 0, 22, POPUP_BOTH);
	_txtTitle		= new Text(300, 16, 10, 30);

//	_cbxCategory	= new ComboBox(this, 146, 16, 166, 46);

	_txtItem		= new Text(80, 9, 16, 46);
	_txtCategory	= new Text(80, 9, 172, 46);

	_lstManufacture	= new TextList(293, 96, 8, 55);

	_btnCancel		= new TextButton(288, 16, 16, 154);

	setPalette("PAL_BASESCAPE", 6);

	add(_window);
	add(_txtTitle);
//	add(_cbxCategory);
	add(_txtItem);
	add(_txtCategory);
	add(_lstManufacture);
	add(_btnCancel);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setColor(Palette::blockOffset(15)+1);
	_txtTitle->setText(tr("STR_PRODUCTION_ITEMS"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtItem->setColor(Palette::blockOffset(15)+1);
	_txtItem->setText(tr("STR_ITEM"));

	_txtCategory->setColor(Palette::blockOffset(15)+1);
	_txtCategory->setText(tr("STR_CATEGORY"));

	_lstManufacture->setColumns(2, 148, 129);
	_lstManufacture->setSelectable();
	_lstManufacture->setBackground(_window);
	_lstManufacture->setMargin(16);
	_lstManufacture->setColor(Palette::blockOffset(13));
	_lstManufacture->setArrowColor(Palette::blockOffset(15)+1);
	_lstManufacture->onMouseClick((ActionHandler)& NewManufactureListState::lstProdClick);

	_btnCancel->setColor(Palette::blockOffset(13)+10);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& NewManufactureListState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewManufactureListState::btnCancelClick,
					Options::keyCancel);

/*	_possibleProductions.clear();
	_game->getSavedGame()->getAvailableProductions(
												_possibleProductions,
												_game->getRuleset(),
												_base);
	_catStrings.push_back("STR_ALL_ITEMS");

	for (std::vector<RuleManufacture*>::iterator
			it = _possibleProductions.begin();
			it != _possibleProductions.end();
			++it)
	{
		bool addCategory = true;
		for (size_t
				x = 0;
				x < _catStrings.size();
				++x)
		{
			if ((*it)->getCategory().c_str() == _catStrings[x])
			{
				addCategory = false;

				break;
			}
		}

		if (addCategory)
			_catStrings.push_back((*it)->getCategory().c_str());
	}

	_cbxCategory->setColor(Palette::blockOffset(15)+1);
	_cbxCategory->setOptions(_catStrings);
	_cbxCategory->onChange((ActionHandler)& NewManufactureListState::cbxCategoryChange); */
}

/**
 * Initializes state (fills list of possible productions).
 */
void NewManufactureListState::init()
{
	State::init();

	fillProductionList();
}

/**
 * Returns to the previous screen.
 * @param action A pointer to an Action.
 */
void NewManufactureListState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Opens the Production settings screen.
 * @param action A pointer to an Action.
*/
void NewManufactureListState::lstProdClick(Action*)
{
/*	RuleManufacture* rule = NULL;
	for (std::vector<RuleManufacture*>::iterator
			it = _possibleProductions.begin();
			it != _possibleProductions.end();
			++it)
	{
		if ((*it)->getName().c_str() == _displayedStrings[_lstManufacture->getSelectedRow()])
		{
			rule = *it;

			break;
		}
	} */

	RuleManufacture* rule = _possibleProductions[_lstManufacture->getSelectedRow()];
	if (rule->getCategory() == "STR_CRAFT"
		&& _base->getAvailableHangars() - _base->getUsedHangars() == 0)
	{
		_game->pushState(new ErrorMessageState(
											tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"),
											_palette,
											Palette::blockOffset(15)+1,
											"BACK17.SCR",
											6));
	}
	else if (rule->getRequiredSpace() > _base->getFreeWorkshops())
	{
		_game->pushState(new ErrorMessageState(
											tr("STR_NOT_ENOUGH_WORK_SPACE"),
											_palette,
											Palette::blockOffset(15)+1,
											"BACK17.SCR",
											6));
	}
	else
	{
		_game->pushState(new ManufactureStartState(
												_base,
												rule));
	}
}

/**
 * Updates the production list to match the category filter
 */
/* void NewManufactureListState::cbxCategoryChange(Action*)
{
	fillProductionList();
} */

/**
 * Fills the list of possible productions.
 */
void NewManufactureListState::fillProductionList()
{
	_lstManufacture->clearList();
	_possibleProductions.clear();
//	_displayedStrings.clear();

	_game->getSavedGame()->getAvailableProductions(
												_possibleProductions,
												_game->getRuleset(),
												_base);

	for (std::vector<RuleManufacture *>::iterator
			it = _possibleProductions.begin();
			it != _possibleProductions.end();
			++it)
	{
		_lstManufacture->addRow(
							2,
							tr((*it)->getName()).c_str(),
							tr((*it)->getCategory ()).c_str());
/*		if ((*it)->getCategory().c_str() == _catStrings[_cbxCategory->getSelected()]
			|| _catStrings[_cbxCategory->getSelected()] == "STR_ALL_ITEMS")
		{
			_lstManufacture->addRow(
								2,
								tr((*it)->getName()).c_str(),
								tr((*it)->getCategory ()).c_str());
			_displayedStrings.push_back((*it)->getName().c_str());
		} */
	}
}

}
