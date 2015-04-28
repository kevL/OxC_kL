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

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"

#include "../Savegame/Base.h"
#include "../Savegame/Production.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Manufacture screen.
 * @param base	- pointer to the Base to get info from
 * @param state	- pointer to the BasescapeState (default NULL when geoscape-invoked)
 */
ManufactureState::ManufactureState(
		Base* base,
		BasescapeState* state)
	:
		_base(base),
		_state(state),
		_baseList(_game->getSavedGame()->getBases())
{
	_window			= new Window(this, 320, 200);
	_mini			= new MiniBaseView(128, 16, 180, 27, MBV_PRODUCTION);

	_txtTitle		= new Text(320, 17, 0, 10);
	_txtBaseLabel	= new Text(80, 9, 16, 10);
	_txtHoverBase	= new Text(80, 9, 224, 10);

	_txtAllocated	= new Text(60, 9, 16, 26);
	_txtAvailable	= new Text(60, 9, 16, 35);

	_txtSpace		= new Text(100, 9, 80, 26);
	_txtFunds		= new Text(100, 9, 80, 35);

	_txtItem		= new Text(120, 9, 16, 53);
	_txtEngineers	= new Text(45, 9, 145, 53);
	_txtProduced	= new Text(40, 9, 174, 45);
	_txtCost		= new Text(50, 17, 215, 45);
	_txtTimeLeft	= new Text(25, 17, 271, 45);

	_lstManufacture	= new TextList(285, 97, 16, 71);

	_btnNew			= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("manufactureMenu");

	add(_window,			"window",	"manufactureMenu");
	add(_mini,				"miniBase",	"basescape"); // <-
	add(_txtTitle,			"text1",	"manufactureMenu");
	add(_txtBaseLabel,		"text1",	"manufactureMenu");
	add(_txtHoverBase,		"numbers",	"baseInfo");
	add(_txtAvailable,		"text1",	"manufactureMenu");
	add(_txtAllocated,		"text1",	"manufactureMenu");
	add(_txtSpace,			"text1",	"manufactureMenu");
	add(_txtFunds,			"text1",	"manufactureMenu");
	add(_txtItem,			"text2",	"manufactureMenu");
	add(_txtEngineers,		"text2",	"manufactureMenu");
	add(_txtProduced,		"text2",	"manufactureMenu");
	add(_txtCost,			"text2",	"manufactureMenu");
	add(_txtTimeLeft,		"text2",	"manufactureMenu");
	add(_lstManufacture,	"list",		"manufactureMenu");
	add(_btnNew,			"button",	"manufactureMenu");
	add(_btnOk,				"button",	"manufactureMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_baseList);
	for (size_t
			i = 0;
			i != _baseList->size();
			++i)
	{
		if (_baseList->at(i) == _base)
		{
			_mini->setSelectedBase(i);
			break;
		}
	}
	_mini->onMouseClick(
					(ActionHandler)& ManufactureState::miniClick,
					SDL_BUTTON_LEFT);
	_mini->onMouseOver((ActionHandler)& ManufactureState::viewMouseOver);
	_mini->onMouseOut((ActionHandler)& ManufactureState::viewMouseOut);

	_txtHoverBase->setAlign(ALIGN_RIGHT);

	_btnNew->setText(tr("STR_NEW_PRODUCTION"));
	_btnNew->onMouseClick((ActionHandler)& ManufactureState::btnNewProductionClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ManufactureState::btnOkClick);
	_btnOk->onKeyboardPress(
						(ActionHandler)& ManufactureState::btnOkClick,
						Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_PRODUCTION"));

	_txtFunds->setText(tr("STR_CURRENT_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtItem->setText(tr("STR_ITEM"));

	_txtEngineers->setText(tr("STR_ENGINEERS__ALLOCATED"));

	_txtProduced->setText(tr("STR_UNITS_PRODUCED"));

	_txtCost->setText(tr("STR_COST__PER__UNIT"));

	_txtTimeLeft->setText(tr("STR_DAYS_HOURS_LEFT"));

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
			woststr1,
			woststr2,
			woststr3,
			woststr4;

		woststr1 << (*i)->getAssignedEngineers();

		if ((*i)->getSellItems() == true)
			woststr2 << "$";
		woststr2 << (*i)->getAmountProduced() << "/";
		if ((*i)->getInfiniteAmount() == true)
			woststr2 << "oo";
		else
			woststr2 << (*i)->getAmountTotal();

		woststr3 << Text::formatFunding((*i)->getRules()->getManufactureCost());

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

			int engineers = (*i)->getAssignedEngineers();
			if (Options::canManufactureMoreItemsPerHour == false)
				engineers = std::min(
								engineers,
								(*i)->getRules()->getManufactureTime());

			// ensure this is rounded up
			// since it takes an entire hour to manufacture any part of that hour's capacity
			hoursLeft = (hoursLeft + engineers - 1) / engineers;

			const int daysLeft = hoursLeft / 24;
			hoursLeft %= 24;
			woststr4 << daysLeft << "/" << hoursLeft;
		}
		else
			woststr4 << L"oo";

		_lstManufacture->addRow
							(5,
							tr((*i)->getRules()->getName()).c_str(),
							woststr1.str().c_str(),
							woststr2.str().c_str(),
							woststr3.str().c_str(),
							woststr4.str().c_str());
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
		const size_t baseId = _mini->getHoveredBase();

		if (baseId < _baseList->size())
		{
			Base* const base = _baseList->at(baseId);

			if (base != _base
				&& base->hasProduction() == true)
			{
				_txtHoverBase->setText(L"");

				_base = base;
				_mini->setSelectedBase(baseId);
				_state->setBase(_base);

				init();
			}
		}
	}
}

/**
 * Displays the name of the Base the mouse is over.
 * @param action - pointer to an Action
 */
void ManufactureState::viewMouseOver(Action*)
{
	const size_t baseId = _mini->getHoveredBase();

	if (baseId < _baseList->size()
		&& _base != _baseList->at(baseId)
		&& _baseList->at(baseId)->hasProduction() == true)
	{
		_txtHoverBase->setText(_baseList->at(baseId)->getName(_game->getLanguage()).c_str());
	}
	else
		_txtHoverBase->setText(L"");
}

/**
 * Clears the hovered Base name.
 * @param action - pointer to an Action
 */
void ManufactureState::viewMouseOut(Action*)
{
	_txtHoverBase->setText(L"");
}

}
