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

#include "ManufactureState.h"

//#include <limits>
//#include <sstream>

#include "BasescapeState.h"
#include "ManufactureInfoState.h"
#include "MiniBaseView.h"
#include "NewManufactureListState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Savegame/Base.h"
#include "../Savegame/Production.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Manufacture screen.
 * @param base	- pointer to the Base to get info from
 * @param state	- pointer to the BasescapeState, can be NULL if geoscape-invoked
 */
ManufactureState::ManufactureState(
		Base* base,
		BasescapeState* state)
	:
		_base(base),
		_state(state)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_mini			= new MiniBaseView(128, 16, 180, 26);

	_txtTitle		= new Text(300, 17, 16, 9);
	_txtBaseLabel	= new Text(80, 9, 16, 9);

	_txtAllocated	= new Text(60, 9, 16, 25);
	_txtAvailable	= new Text(60, 9, 16, 34);

	_txtSpace		= new Text(100, 9, 80, 25);
	_txtFunds		= new Text(100, 9, 80, 34);

	_txtItem		= new Text(120, 9, 16, 52);
	_txtEngineers	= new Text(45, 9, 145, 52);
	_txtProduced	= new Text(40, 9, 174, 44);
	_txtCost		= new Text(50, 17, 215, 44);
	_txtTimeLeft	= new Text(25, 17, 271, 44);

	_lstManufacture	= new TextList(285, 97, 16, 70);

	_btnNew			= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setPalette("PAL_BASESCAPE", 6);

	add(_window);
	add(_mini);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtAvailable);
	add(_txtAllocated);
	add(_txtSpace);
	add(_txtFunds);
	add(_txtItem);
	add(_txtEngineers);
	add(_txtProduced);
	add(_txtCost);
	add(_txtTimeLeft);
	add(_lstManufacture);
	add(_btnNew);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+6);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_game->getSavedGame()->getBases());
	for (size_t
			i = 0;
			i < _game->getSavedGame()->getBases()->size();
			++i)
	{
		if (_game->getSavedGame()->getBases()->at(i) == base)
		{
			_mini->setSelectedBase(i);
			break;
		}
	}
	_mini->onMouseClick(
					(ActionHandler)& ManufactureState::miniClick,
					SDL_BUTTON_LEFT);

	_btnNew->setColor(Palette::blockOffset(13)+10);
	_btnNew->setText(tr("STR_NEW_PRODUCTION"));
	_btnNew->onMouseClick((ActionHandler)& ManufactureState::btnNewProductionClick);

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ManufactureState::btnOkClick);
	_btnOk->onKeyboardPress(
						(ActionHandler)& ManufactureState::btnOkClick,
						Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(15)+6);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_PRODUCTION"));

	_txtBaseLabel->setColor(Palette::blockOffset(15)+6);

	_txtAvailable->setColor(Palette::blockOffset(15)+6);
	_txtAvailable->setSecondaryColor(Palette::blockOffset(13));

	_txtAllocated->setColor(Palette::blockOffset(15)+6);
	_txtAllocated->setSecondaryColor(Palette::blockOffset(13));

	_txtSpace->setColor(Palette::blockOffset(15)+6);
	_txtSpace->setSecondaryColor(Palette::blockOffset(13));

	_txtFunds->setColor(Palette::blockOffset(15)+6);
	_txtFunds->setSecondaryColor(Palette::blockOffset(13));
	_txtFunds->setText(tr("STR_CURRENT_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtItem->setColor(Palette::blockOffset(15)+1);
	_txtItem->setText(tr("STR_ITEM"));

	_txtEngineers->setColor(Palette::blockOffset(15)+1);
	_txtEngineers->setText(tr("STR_ENGINEERS__ALLOCATED"));
	_txtEngineers->setVerticalAlign(ALIGN_BOTTOM);

	_txtProduced->setColor(Palette::blockOffset(15)+1);
	_txtProduced->setText(tr("STR_UNITS_PRODUCED"));
	_txtProduced->setVerticalAlign(ALIGN_BOTTOM);

	_txtCost->setColor(Palette::blockOffset(15)+1);
	_txtCost->setText(tr("STR_COST__PER__UNIT"));
	_txtCost->setVerticalAlign(ALIGN_BOTTOM);

	_txtTimeLeft->setColor(Palette::blockOffset(15)+1);
	_txtTimeLeft->setText(tr("STR_DAYS_HOURS_LEFT"));
	_txtTimeLeft->setVerticalAlign(ALIGN_BOTTOM);

	_lstManufacture->setColor(Palette::blockOffset(13)+10);
	_lstManufacture->setArrowColor(Palette::blockOffset(15)+9);
	_lstManufacture->setColumns(5, 121, 29, 41, 56, 30);
	_lstManufacture->setSelectable();
	_lstManufacture->setBackground(_window);
	_lstManufacture->setMargin();
	_lstManufacture->onMouseClick((ActionHandler)& ManufactureState::lstManufactureClick);
}

/**
 * dTor.
 */
ManufactureState::~ManufactureState()
{}

/**
 * Updates the production list after going to other screens.
 */
void ManufactureState::init()
{
	State::init();

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));
	fillProductionList();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the screen with the list of possible productions.
 * @param action - pointer to an Action
 */
void ManufactureState::btnNewProductionClick(Action*)
{
	_game->pushState(new NewManufactureListState(_base));
}

/**
 * Fills the list of base productions.
 */
void ManufactureState::fillProductionList()
{
	_lstManufacture->clearList();

	const std::vector<Production*> productions(_base->getProductions());
	for (std::vector<Production*>::const_iterator
			i = productions.begin();
			i != productions.end();
			++i)
	{
		std::wostringstream
			s1,
			s2,
			s3,
			s4;

		s1 << (*i)->getAssignedEngineers();

		if ((*i)->getSellItems() == true)
			s2 << "$";
		s2 << (*i)->getAmountProduced() << "/";
		if ((*i)->getInfiniteAmount() == true)
			s2 << "oo";
		else
			s2 << (*i)->getAmountTotal();

		s3 << Text::formatFunding((*i)->getRules()->getManufactureCost());

		if ((*i)->getAssignedEngineers() > 0)
		{
			int hoursLeft;
			if ((*i)->getSellItems() == true
				|| (*i)->getInfiniteAmount() == true)
			{
				hoursLeft = ((*i)->getAmountProduced() + 1) * (*i)->getRules()->getManufactureTime()
						  - (*i)->getTimeSpent();
			}
			else
				hoursLeft = (*i)->getAmountTotal() * (*i)->getRules()->getManufactureTime()
							- (*i)->getTimeSpent();

			int engs = (*i)->getAssignedEngineers();
			if (Options::canManufactureMoreItemsPerHour == false)
				engs = std::min(
							engs,
							(*i)->getRules()->getManufactureTime());

			// ensure this is rounded up
			// since it takes an entire hour to manufacture any part of that hour's capacity
			hoursLeft = (hoursLeft + engs - 1) / engs;

			const int daysLeft = hoursLeft / 24;
			hoursLeft %= 24;
			s4 << daysLeft << "/" << hoursLeft;
		}
		else
			s4 << L"oo";

		_lstManufacture->addRow
							(5,
							tr((*i)->getRules()->getName()).c_str(),
							s1.str().c_str(),
							s2.str().c_str(),
							s3.str().c_str(),
							s4.str().c_str());
	}

	_txtAvailable->setText(tr("STR_ENGINEERS_AVAILABLE")
							.arg(_base->getEngineers()));
	_txtAllocated->setText(tr("STR_ENGINEERS_ALLOCATED")
							.arg(_base->getAllocatedEngineers()));
	_txtSpace->setText(tr("STR_WORKSHOP_SPACE_AVAILABLE")
							.arg(_base->getFreeWorkshops()));
}

/**
 * Opens the screen displaying production settings.
 * @param action - pointer to an Action
 */
void ManufactureState::lstManufactureClick(Action*)
{
	const std::vector<Production*> productions(_base->getProductions());
	_game->pushState(new ManufactureInfoState(
											_base,
											productions[_lstManufacture->getSelectedRow()]));
}

/**
 * Selects a new base to display.
 * @param action - pointer to an Action
 */
void ManufactureState::miniClick(Action*)
{
	if (_state != NULL) // cannot switch bases if coming from geoscape.
	{
		size_t base = _mini->getHoveredBase();

		if (base < _game->getSavedGame()->getBases()->size()
			&& _base != _game->getSavedGame()->getBases()->at(base))
		{
			_mini->setSelectedBase(base);

			_base = _game->getSavedGame()->getBases()->at(base);
			_state->setBase(_base);

			init();
		}
	}
}

}
