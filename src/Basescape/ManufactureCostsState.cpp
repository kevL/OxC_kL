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

#include "ManufactureCostsState.h"

#include "../Engine/Game.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleManufacture.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Screen that displays a table of manufacturing costs.
 */
ManufactureCostsState::ManufactureCostsState()
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 17, 10, 10);

	_txtItem		= new Text(96, 9,  10, 30);
	_txtCost		= new Text(50, 9, 111, 30);
	_txtManHours	= new Text(30, 9, 161, 30);
	_txtSpace		= new Text(15, 9, 196, 30);
	_txtRequired	= new Text(95, 9, 206, 30);

	_lstProduction	= new TextList(291, 129, 10, 40);

	_btnCancel		= new TextButton(288, 16, 16, 177);


	setInterface("allocateManufacture");

	add(_window,		"window",	"allocateManufacture"); // <- using allocateManufacture colors ->
	add(_txtTitle,		"text",		"allocateManufacture");
	add(_txtItem,		"text",		"allocateManufacture");
	add(_txtManHours,	"text",		"allocateManufacture");
	add(_txtSpace,		"text",		"allocateManufacture");
	add(_txtCost,		"text",		"allocateManufacture");
	add(_txtRequired,	"text",		"allocateManufacture");
	add(_lstProduction,	"list",		"allocateManufacture");
	add(_btnCancel,		"button",	"allocateManufacture");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr("STR_COSTS_UC"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	const Uint8 color = static_cast<Uint8>(_game->getRuleset()->getInterface("allocateManufacture")->getElement("text")->color2);

	_txtItem->setText(tr("STR_LIST_ITEM"));
	_txtItem->setColor(color);

	_txtManHours->setText(tr("STR_HOURS_PER"));
	_txtManHours->setColor(color);

	_txtSpace->setText(tr("STR_SPACE_LC"));
	_txtSpace->setColor(color);

	_txtCost->setText(tr("STR_COST_LC"));
	_txtCost->setColor(color);

	_txtRequired->setText(tr("STR_REQUIRED_LC"));
	_txtRequired->setColor(color);

	_lstProduction->setColumns(5, 96, 50, 30, 15, 95);
	_lstProduction->setMargin(5);
	_lstProduction->setBackground(_window);
	_lstProduction->setSelectable();

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& ManufactureCostsState::btnCancelClick);
	_btnCancel->onKeyboardPress(
							(ActionHandler)& ManufactureCostsState::btnCancelClick,
							Options::keyCancel);
}

/**
 * dTor.
 */
ManufactureCostsState::~ManufactureCostsState()
{}

/**
 * Populates the table with manufacture information.
 */
void ManufactureCostsState::init()
{
	std::vector<RuleManufacture*> prodRules;
	RuleManufacture* prodRule;

	const std::vector<std::string>& prodList = _game->getRuleset()->getManufactureList();
	for (std::vector<std::string>::const_iterator
			i = prodList.begin();
			i != prodList.end();
			++i)
	{
		prodRule = _game->getRuleset()->getManufacture(*i);

		if (_game->getSavedGame()->isResearched(prodRule->getRequirements()) == true)
			prodRules.push_back(prodRule);
	}

	size_t row = 0;
	std::wostringstream woststr;
	int
		profit,
		requiredCosts,
		salesCost;
	float profitAspect;

	for (std::vector<RuleManufacture*>::const_iterator
			i = prodRules.begin();
			i != prodRules.end();
			++i,
				row += 3)
	{
		woststr.str(L"");
		woststr << L"> " << tr((*i)->getName());

		_lstProduction->addRow(
							5,
							woststr.str().c_str(),
							Text::formatFunding((*i)->getManufactureCost()).c_str(),
							Text::formatNumber((*i)->getManufactureTime()).c_str(),
							Text::formatNumber((*i)->getRequiredSpace()).c_str(),
							L"");
//							tr((*i)->getCategory ()).c_str());
		_lstProduction->setCellColor(
								row,
								0,
								213); // yellow, Palette::blockOffset(13)+5
		requiredCosts = 0;

		std::map<std::string, int> required = (*i)->getRequiredItems();
		for (std::map<std::string, int>::const_iterator
				j = required.begin();
				j != required.end();
				++j)
		{
			requiredCosts += _game->getRuleset()->getItem((*j).first)->getSellCost() * (*j).second;

			woststr.str(L"");
			woststr << L"(" << (*j).second << L") " << tr((*j).first);

			if (j == required.begin())
				_lstProduction->setCellText(
										row,
										4,
										woststr.str());
			else
			{
				_lstProduction->addRow(
									5,
									L">",L"",L"",L"",
									woststr.str());
				++row;
			}
		}

		profit = 0;

		std::map<std::string, int> producedItems = (*i)->getProducedItems();
		for (std::map<std::string, int>::const_iterator
				j = producedItems.begin();
				j != producedItems.end();
				++j)
		{
			woststr.str(L"");
			woststr << L"< " << tr((*j).first);

			if ((*i)->getCategory() == "STR_CRAFT")
				salesCost = _game->getRuleset()->getCraft((*j).first)->getSellCost();
			else
				salesCost = _game->getRuleset()->getItem((*j).first)->getSellCost();

			salesCost *= (*j).second;

			profit += salesCost;

			_lstProduction->addRow(
								5,
								woststr.str().c_str(),
								Text::formatFunding(salesCost).c_str(),
								Text::formatNumber((*j).second).c_str(),
								L"",L"");
			++row;
		}

		profit -= (*i)->getManufactureCost();
		profit -= requiredCosts;
		profitAspect = static_cast<float>(profit) / static_cast<float>((*i)->getManufactureTime());

		woststr.str(L"");
		woststr << std::fixed << std::setprecision(2) << profitAspect;

		_lstProduction->addRow(
							5,
							L"<",
							woststr.str().c_str(),
							L"",L"",
							Text::formatFunding(profit).c_str());

		_lstProduction->addRow(5, L"",L"",L"",L"",L"");
	}
}

/**
 * Returns to previous screen.
 * @param action - pointer to an Action
 */
void ManufactureCostsState::btnCancelClick(Action*)
{
	_game->popState();
}

}
